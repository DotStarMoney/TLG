#ifndef TLG_LIB_INDEXSTAGEGRAPH_H_
#define TLG_LIB_INDEXSTAGEGRAPH_H_

#include <algorithm>
#include <atomic>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/strings/string_view.h"
#include "base/static_type_assert.h"
#include "glog/logging.h"
#include "util/deleterptr.h"
#include "util/loan.h"
#include "util/noncopyable.h"

namespace tlg_lib {

// StageGraph implements a world state embedding mechanic. A StageGraph keeps an
// immutable list of stages to which an Index can be created. The basic idea is
// to create a minimal memory footprint representation of different versions of
// a world. If Index A tracks the world at some state, and we clone it to make
// Index B, any content change by Index B will not be seen by Index A as they
// track different versions of the world.
//
// Furthermore, we allow embedding of an index to an existing one. This way,
// Index A can store Index B as the content of a stage, and later it or a
// different Index can retrieve embedded Index B and replace itself or "jump" to
// it.
//
// - SharedC is the type of the content shared by all Indexes to a Stage
// - C is the type of the content that can change across Indexes
//
template <typename SharedC, typename C>
class StageGraph : public util::Lender {
 private:
  class Index;
  friend class Index;

  typedef uint32_t VersionId;

  static constexpr VersionId kNoVersion = std::numeric_limits<VersionId>::max();

  // A Version is one vertex in the DAG of versions.
  struct Version : public util::NonCopyable {
    friend class Index;

    Version(VersionId vers, VersionId base, StageGraph *const stage_graph)
        : vers_(vers),
          base_vers_(basis),
          parent_count_(0),  // A new version cannot have a parent.
          refs_(1),          // If we create a version, it's assumed an Index
                             //     references it.
          pinning_refs_(1),  // Ditto.
          stage_graph_(stage_graph) {}

    Version(Version &&version)
        : parent_count_(version.parent_count_),
          vers_(version.vers_),
          base_vers_(version.base_vers_),
          refs_(version.refs_),
          pinning_refs_(version.pinning_refs_),
          stage_graph_(version.stage_graph_) {}

    ~Version() {
      // Delete any scenes at this version in the stage map. As Versions are
      // ALWAYS deleted in descending order, this will not invalidate any
      // inter-scene loans
      for (const auto &stage : in_stages_) {
        auto &stages_i = stage_graph_->stages_.find(stage);
        CHECK_NE(stages_i, stage_graph_->stages_.end())
            << "While deleting version " << vers_
            << ", tried to delete in-stage '" << stage
            << "' which does not exist.";
        stages_i.second->DeleteScene(vers_);
      }
      // Any children of this version just lost a parent :'(
      for (const auto &link_i : links_) {
        Version &v = stage_graph_->GetVersionOrDie(link_i.first);
        v.parent_count_ -= link_i.second;
      }
      Version &v = stage_graph_->GetVersionOrDie(base_vers_);
      --(v.parent_count_);
      // We don't need to modify outgoing links to this version in other
      // versions as we'll never remove a version before its parent.
    }

    // Increase the total of count of Indexes/embedded Indexes that reference
    // this version.
    void inc() { ++refs_; }
    // Decrease ...
    void dec() {
      CHECK_NE(refs--, 0) << "Reference count underflow in version " << vers_;
    }

    // Increase the total count of Indexes (not embedded) that reference this
    // version.
    void pin() { ++pinning_refs_; }
    // Decrease ...
    void unpin() {
      CHECK_NE(--pinning_refs, 0) << "Pin count underflow in version " << vers_;
    }

    // Is this Version pinned in the graph? That is to say: "is there an Index
    // that references this Version?"
    bool pinned() const { return pinning_refs_ >= 1; }
    // If evaluated by an Index that references this version, are we safe to
    // just overwrite the old content with the new content in any scenes at this
    // version? (The alternative being we have to create a new version with the
    // new content.)
    bool overwritable_by_ref() const {
      return (refs_ <= 1) && (parent_count_ > 0);
    }

    // Use universal reference so we can eat the incoming string when possible.
    template <typename StringT>
    void AddStage(StringT &&stage) {
      base::type_assert::AssertIsConvertibleTo<std::string, StringT>();
      in_stages_.push_back(std::forward<StringT>(stage));
    }

    // Add, or increase the counter of: an edge to a version embedded in this
    // version. In other words, make link_vers a child of this version in the
    // version DAG.
    void IncLink(VersionId link_vers) {
      auto &link_i = links_.find(link_vers);
      if (link_i == links_.end()) link_i = links_.insert(link_vers, 0);
      ++(link_i->second);
    }

    // Remove, or decrease the counter of: an edge to a ...
    void DecLink(VersionId link_vers) {
      auto &link_i = links_.find(link_vers);
      CHECK_NE(link_i, links_.end())
          << "Linked version " << link_vers
          << " not present in links map of version " << vers_;
      if (--(link_i->second) == 0) links_.erase(link_i);
    }

    VersionId base() const { return base_vers_; }
    VersionId vers() const { return vers_; }

    const std::unordered_map<VersionId, uint32_t> &links() const {
      return links_;
    }

    // This version
    const VersionId vers_;
    // The version from which this version was created, also a child of this
    // version in the version DAG.
    const VersionId base_vers_;

    // Protects all non-const class members.
    std::mutex m_;

    // Number of parents
    uint32_t parent_count_;

    // Child versions that are embedded in this version. The map value is just
    // a counter in the event that this version has multiple embeddings of the
    // same version. These are children of this version in the DAG.
    std::unordered_map<VersionId, uint32_t> links_;

    // Stages that have a Scene at this version. Only ever grows, never shrinks
    // until destruction.
    std::vector<std::string> in_stages_;

    // Number of Indexes/embedded Indexes that reference this version.
    uint32_t refs_;
    // Number of Indexes that reference this version. In other words, numbed of
    // Indexes that "pin" this Version in the DAG during compaction.
    uint32_t pinning_refs_;

    StageGraph *const stage_graph_;
  };

  // A unit of content storage at a version in a stage. This is a lender so that
  // versions can be chained together in the event that the user of StageGraph
  // chooses to rebuild the content of a stage incrementally (see
  // Index::GetContent)
  struct Scene : public util::Lender {
    util::Loan<Scene> MakeLoan() { return MakeLoan<Scene>(); }

    Scene(Scene &&scene)
        : basis_(std::move(scene.basis_)),
          content_(std::move(scene.content_)) {}

    // The scene from which this scene was created. Can be nullptr if we choose
    // not to keep track of that information.
    util::Loan<Scene> basis_;

    C content_;
  };

  // A map of versions to scenes. Also stores shared content that is shared
  // across all indexes to this stage.
  struct Stage : public util::NonCopyable {
    friend class Index;

    // Universal ref so we can eat the string
    template <typename StringT>
    Stage(StringT &&name, std::unique_ptr<SharedC> shared_content,
          Scene &&base_scene, StageGraph *const graph)
        : name_(std::forward<StringT>(name)),
          shared_content_refs_(std::make_unique<std::atomic<int>>(0)),
          shared_content_(std::move(shared_content)),
          stage_graph_(graph) {
      scenes_.insert(0, base_scene);
    }

    ~Stage() {
      CHECK_EQ(shared_content_refs_->load(), 0)
          << "Cannot destruct Stage while outstanding references to it's "
             "shared content are held.";
      // We must explicitly delete scenes in this order to avoid invalidating
      // outstanding loans between scenes
      while (!scenes_.empty()) {
        scenes_.erase(scenes_.rbegin());
      }
    }

    void DeleteScene(VersionId vers) {
      CHECK_NE(vers, 0) << "Cannot delete version 0. Stage '" << name_ << "'.";
      CHECK_NE(scenes_.find(vers), scenes_.end())
          << "Version " << vers << " does not exist in stage '" << name_
          << "'.";
      scenes_.erase(vers);
    }

    void AddScene(VersionId vers, Scene &&scene) {
      CHECK_EQ(scenes_.find(vers), scenes_.end())
          << "Version " << vers << " already exists in stage '" << name_
          << "'.";
      scenes_.insert(vers, base_scene);
    }

    // Scene retrieval is implemented so that getting a scene at a version that
    // doesn't exist in this stage gets the next scene chronologically before
    // the one we ask for. This is implemented by walking the version graph
    // backward until we hit a scene that exists in this stage. Note that simply
    // going to "the next lowest" version won't work here as versions can branch
    // from any other version forming a more complex structure than a
    // chronological list.
    //
    // Return the scene and version at which we found it.
    std::pair<Scene &, VersionId> GetScene(VersionId vers) {
      std::unordered_map<VersionId, Scene>::iterator scene_i;
      VersionId at_version = vers;
      for (;;) {
        scene_i = scenes_.find(vers);
        if (scene_i != scenes_.end()) break;

        auto vers_i = stage_graph_->version_graph_.find(vers);
        CHECK_NE(vers_i, stage_graph_->version_graph_.end())
            << "Version graph exhausted at version " << vers
            << " while searching in stage '" << name_ << "'.";
        at_version = version;
        vers = vers_i->second.base();
      }
      return {scene_i->second, at_version};
    }

    void UpdateSharedContent(SharedC &&shared_content) {
      CHECK_EQ(shared_content_refs_->load(), 0)
          << "Cannot update shared content while outstanding references to the "
             "old value are held.";
      std::unique_lock<std::mutex> lock(shared_content_mu_);
      *(shared_content_.get()) = shared_content;
    }

    // Provide a deleter_ptr that decs the ref count to the shared content when
    // released.
    util::deleter_ptr<SharedC> shared_content() const {
      std::shared_lock<std::mutex> slock(shared_content_mu_);
      ++(*shared_content_refs_);
      return util::deleter_ptr<SharedC>(
          shared_content_.get(),
          [refs = shared_content_refs_.get()](SharedC *c) { --(*refs); });
    }

    const std::string name_;
    mutable std::atomic<uint32_t> shared_content_refs_;

    // Protects only the shared content. We can give this it's own mutex as
    // shared content won't be modified when the StageGraph takes it's lock.
    std::mutex shared_content_mu_;
    std::unique_ptr<SharedC> shared_content_;

    // Mutated only when the parent StageGraph locks it's mutex.
    std::map<VersionId, Scene> scenes_;

    StageGraph *const stage_graph_;
  };

  // Used to provide references to indexes.
  util::Loan<StageGraph> MakeLoan() { return MakeLoan<StageGraph>(); }

  Stage &GetStageOrDie(const std::string &name) {
    auto &stage_i = stages_.find(name);
    CHECK_NE(stage_i, stages_.end()) << "Stage " << name << " not found.";
    return stage_i->second;
  }

  Version &GetVersionOrDie(VersionId vers) {
    auto &vers_i = version_graph_.find(vers);
    CHECK_NE(vers_i, version_graph_.end())
        << "Version " << vers << " not in version graph.";
    return vers_i->second;
  }

  // Given a version: decrease it's parent count and if it is both parent-less
  // after decrement and not up for deletion, add it to the frontier
  void DecParentsAndMaybeAddToFrontier(Version *v,
                                       const std::set<VersionId> deleted_set) {
    if ((--(v->parent_count) == 0) &&
        (delete_vers.find(v->vers()) == delete_vers.end())) {
      frontier_.insert(v->vers());
    }
  }

  // Trim any inaccessible versions and their associated scenes. This is
  // O(n*log(n)) execution and O(n) memory with n the number of versions. We get
  // this complexity by tracking the version graph "frontier," i.e., versions
  // (vertexes) without parents.
  //
  // To compact, from each frontier vertex, we depth-first search down the DAG
  // marking versions as inaccessible and therefore delete-able until we hit a
  // version that's pinned (accessible). We mark all descendent's of that
  // version as accessible. We finish by deleting all inaccessible versions.
  //
  void Compact() {
    // Make a set of all versions. Versions that persist in this set after the
    // frontier search are to be deleted.
    std::set<VersionId> delete_vers;
    for (const auto &version_i : version_graph_) {
      // Never try to delete version 0.
      if (version_i->first.vers() != 0) {
        delete_vers.insert(version_i->first.vers());
      }
    }

    // From every frontier, find versions that are inaccessible from pins
    for (const auto &frontier_i = frontier_.begin();
         (frontier_i != frontier_.end()) && (!delete_vers.empty());
         ++frontier_i) {
      // A stack of versions left to visit and whether their parent's were
      // accessible.
      std::vector<std::pair<VersionId, bool>> persue;

      Version *v = &GetVersionOrDie(persue.back(*frontier_i));
      persue.push_back({*frontier_i, v->pinned()});

      while (!persue.empty()) {
        // This check and later setting of v to nullptr allows us to skip an
        // extra GetVersionOrDie on the first iteration when we already have
        // what we need.
        if (v == nullptr) v = &GetVersionOrDie(persue.back().first);
        // We are accessible and therefore un-deletable if one of our parent's
        // are, or we are.
        const bool accessible = persue.back().second | v->pinned();
        persue.pop_back();

        for (const auto &link_i : v->links()) {
          // Don't try to visit version 0
          if (link_i->first != 0) persue.push_back({link_i->first, accessible});
        }
        // Don't try to visit version 0
        if (v->vers() != 0) persue.push_back({v->base(), accessible});

        // If we get here while searching down from any frontier, we remove the
        // version from the chopping block list.
        if (accessible) delete_vers.erase(v->vers());
        v = nullptr;
      }
    }

    // Delete remaining inaccessible versions and rebuild frontiers.
    for (const auto &vers = delete_vers.rbegin(); vers != delete_vers.rend();
         ++vers) {
      Version *v = &GetVersionOrDie(*vers);
      if (frontier_.find(v->vers())) frontier_.erase(v->vers());

      // Remove a parent from all descendent's of this version, and if any wind
      // up with 0 parents, make them frontiers.
      DecParentsAndMaybeAddToFrontier(&GetVersionOrDie(v->base()), delete_vers);
      for (auto &link_v : v->links()) {
        DecParentsAndMaybeAddToFrontier(&GetVersionOrDie(link_v->first),
                                        delete_vers);
      }

      version_graph_.erase(v->vers());
    }
  }

  VersionId CreateVersion() { return ++vers_generator_; }

  // Write-locked when any of the underlying data structures are modified. Since
  // Indexes perform most of the locking themselves, we leave all methods
  // unguarded.
  std::mutex m_;

  // Map to keep deletion in descending chronological order to preserve loans
  // between scenes.
  std::map<VersionId, Version> version_graph_;
  std::unordered_set<VersionId> frontier_;

  std::unordered_map<std::string, Stage> stages_;

  VersionId vers_generator_;

 public:
  StageGraph(StageGraph &&stage_graph)
      : m_(std::move(stage_graph.m_)),
        version_graph_(std::move(stage_graph.version_graph_)),
        frontier_(std::move(stage_graph.frontier_)),
        stages_(std::move(stage_graph.stages_)),
        vers_generator_(stage_graph.vers_generator_) {}

  // Deletion order matters. See comment on version_graph_.
  ~StageGraph() {
    while (!version_graph_.empty()) {
      version_graph_.erase(version_graph_.rbegin());
    }
  }

  // Returns an index to unmodified stages.
  Index CreateIndex(const std::string &stage_name) {
    Version &v = GetVersionOrDie(0);
    v.pin();
    v.ref();
    return Index(0, &GetStageOrDie(stage_name), MakeLoan<StageGraph>());
  }

  // Since stages cannot be added/removed, we create a StageGraph through a
  // builder.
  class Builder : util::NonCopyable {
    friend class StageGraph<SharedC, C>;

   public:
    Builder(Builder &&builder) { fragments_ = std::move(builder.fragments); }

    template <typename StringT>
    Builder &push(StringT &&name, SharedC &&shared_content, C &&content) {
      base::type_assert::AssertIsConvertibleTo<std::string, StringT>();
      fragments_.push_back(
          StageFragment{std::forward<StringT>(name), shared_content, content});
      return *this;
    }

    uint32_t size() const { return fragments_.size(); }

    StageGraph<SharedC, C> buildAndClear() {
      auto sg = StageGraph<SharedC, C>(*this);
      fragments_.clear();
      return std::move(sg);
    }

   private:
    struct StageFragment {
      std::string name;
      SharedC shared_content;
      C content;
    };
    std::vector<StageFragment> fragments_;

    Builder() {}
  };

  static Builder builder() { return Builder(); }

  // An index that doesn't pin any versions. This can be thought of as a version
  // embedded in another version.
  class EmbeddedIndex : util::NonCopyable {
    friend class Index;
    EmbeddedIndex(EmbeddedIndex &&embedded_index)
        : stage_(std::move(embedded_index.stage_)),
          src_vers_(embedded_index.src_vers_),
          dst_vers_(embedded_index.dst_vers_) {}

   private:
    EmbeddedIndex(const std::string &stage, VersionId src_vers,
                  VersionId dst_vers)
        : stage_(stage), src_vers_(src_vers), dst_vers_(dst_vers) {}

    // src_vers_ and stage_ exist entirely for sanity checking of embed/unembed
    // calls.
    std::string stage_;
    VersionId src_vers_;
    VersionId dst_vers_;
  };

  // An Index captures a consistent state of the stages. It provides methods
  // to access/mutate a stage's content, and methods to manipulate Indexes.
  class Index : public util::NonCopyable {
    friend class StageGraph<SharedC, C>;

   private:
    Index(VersionId vers, Stage *stage, util::Loan<StageGraph> stage_graph)
        : vers_(vers), stage_(stage), stage_graph_(std::move(stage_graph)) {}

    void invalidate() { stage_ = nullptr; }

   public:
    Index(Index &&other)
        : vers_(other.vers_),
          stage_(other.stage_),
          stage_graph_(std::move(other.stage_graph_)) {}

    ~Index() {
      if (valid()) Release();
    }

    bool valid() const { return stage_ != nullptr; }

    // Since accessing shared content doesn't require locking the whole
    // StageGraph, we let a stage's shared content methods take care of the
    // locking. Also note the stage map won't change after construction of a
    // StageGraph; another reason we needn't lock the entire StageGraph.
    util::deleter_ptr<SharedC> GetSharedContent() const {
      CHECK(valid()) << "Index invalid.";
      return stage_->shared_content();
    }

    // See above comment...
    void SetSharedContent(SharedC &&shared_content) {
      CHECK(valid()) << "Index invalid.";
      stage_->UpdateSharedContent(shared_content);
    }

    // In chronological version order, visit up to depth scenes before the scene
    // we reference in this stage. Since we need to read lock the StageGraph,
    // manipulation is done via a consumer method.
    //
    // Visiting of scenes older than the one we reference is enabled via linking
    // scenes in SetContent. This functionality can be used to incrementally
    // resolve the content of a stage.
    void GetContent(std::function<void(const C &)> consumer,
                    uint32_t depth = std::numeric_limits<uint32_t>::max()) {
      CHECK(valid()) << "Index invalid.";
      std::shared_lock<std::mutex> slock(stage_graph_->m_);

      std::vector<C *> content_list;
      Scene *scene = &(s->GetScene(vers_));
      while ((scene != nullptr) && (depth > 0)) {
        content_list.push_back(&(scene->content));
        scene = scene->basis_.get();
        --depth;
      }

      for (const auto &content_i = content_list.rbegin();
           content_i != content_list.rend(); ++content_i) {
        // First * gives us the content pointer from the iterator, second
        // dereferences it.
        consumer(**content_i);
      }
    }

    // Sets our stage pointer, "goes" to the stage.
    void Go(const std::string &stage_name) {
      CHECK(valid()) << "Index invalid.";
      std::shared_lock<std::mutex> slock(stage_graph_->m_);
      stage_ = &(stage_graph_->GetStageOrDie(stage_name));
    }

    // Sets the content of the current stage. Since views of the underlying
    // StageGraph are treated as completely unique between indices, a
    // considerable amount of work may have to be done to provide a separate
    // view of the underlying graph to the index updating the content.
    //
    // link_base can be set to chain versions together so that version content
    // can be incrementally constructed in GetContent (if content is heavy this
    // is a technique to avoid storing large amounts of data in every scene)
    void SetContent(C &&content, bool link_base = false) {
      CHECK(valid()) << "Index invalid.";
      std::unique_lock<std::mutex> lock(stage_graph_->m_);

      Version &v = stage_graph_->GetVersionOrDie(vers_);

      std::pair<Scene &, VersionId> base_info = stage_->GetScene(vers_);
      Scene &base_scene = base_info.first;
      const VersionId base_vers = base_info.second;

      // If nobody else cares about the scene we're currently looking at (and it
      // already exists) we can just overwrite the content
      if (v.overwritable_by_ref() && (base_vers == vers_)) {
        base_scene.content = content;
        return;
      }

      VersionId new_vers = vers_;
      // If we can't just drop a new scene into this stage at the current
      // version...
      if (!v.overwritable_by_ref()) {
        // Since this index is leaving this version...
        v.unpin();
        v.dec();
        ++(v.parent_count_);

        // Create a new version
        new_vers = stage_graph_->CreateVersion();
        stage_graph_->version_graph_
            .insert(new_vers, Version(new_vers, vers_, stage_graph_))
            .first->second.AddStage(std::string(stage_->name_));

        // Update the frontier
        auto &frontier_i = stage_graph_->frontier_.find(vers_);
        if (frontier_i != stage_graph_->frontier_.end()) {
          stage_graph_->frontier_.erase(frontier_i);
        }
        stage_graph_->frontier.insert(new_vers);
      } else {
        // We can reuse our current version since nobody else references it,
        // but we'll still need to add a new scene. Tell the version about the
        // stage with this scene.
        v.AddStage(std::string(stage_->name_));
      }

      stage_->AddScene(
          new_vers,
          Scene{link_base ? base_scene.MakeLoan() : util::Loan(), content});
      vers_ = new_vers;
    }

    // Consume a different index, "embedding" it in this index. This internally
    // sets the version of the consumed index to be a descendant of the version
    // in the calling index. We don't need to compact since by definition, this
    // index's version is accessible, therefore the embedded index version is
    // also accessible as it is our descendant.
    EmbeddedIndex Embed(Index &&index) {
      CHECK(valid()) << "Index invalid.";
      CHECK_NE(index, this) << "Cannot embed index within itself.";
      CHECK_GE(index.vers_, vers_)
          << "Embedding will introduce cycle in version graph.";

      {
        std::shared_lock<std::mutex> slock(stage_graph_->m_);

        Version &embed_v = stage_graph_->GetVersionOrDie(index.vers_);
        Version &v = stage_graph_->GetVersionOrDie(vers_);

        std::unique_lock<std::mutex> embed_lock(embed_v.m_, std::defer_lock);
        std::unique_lock<std::mutex> lock(v.m_, std::defer_lock);
        std::lock(embed_lock, lock);

        // The version we embed is still "referenced," hence we don't call
        // "dec()". However since the embeded version is now only as accessible
        // as is this version, we unpin it.
        embed_v.unpin();

        // There's no point adding an edge to the version graph that starts and
        // ends at the same version.
        if (vers_ != index.vers_) {
          ++(embed_v.parent_count_);
          v.IncLink()
        }
      }

      index.invalidate();
      return EmbeddedIndex(stage_->name_, vers_, index.vers_);
    }

    // Reverse the embedding process.
    Index Unembed(const EmbeddedIndex &embedded_index) {
      CHECK(valid()) << "Index invalid.";
      CHECK_EQ(embedded_index.src_vers_, vers_)
          << "Index must be unembedded from the same VERSION into which it was "
             "embedded.";
      CHECK_EQ(embedded_index.stage_, stage_->name_)
          << "Index must be unembedded from the same STAGE into which it was "
             "embedded.";
      {
        std::shared_lock<std::mutex> slock(stage_graph_->m_);

        Version &unembed_v =
            stage_graph_->GetVersionOrDie(embedded_index.dst_vers_);
        Version &v = stage_graph_->GetVersionOrDie(vers_);

        std::unique_lock<std::mutex> unembed_lock(unembed_v.m_,
                                                  std::defer_lock);
        std::unique_lock<std::mutex> lock(v.m_, std::defer_lock);
        std::lock(unembed_lock, lock);

        unembed_v.pin();

        // See comment in Embed...
        if (vers_ != embedded_index.dst_vers_) {
          --(unembed_v.parent_count_);
          v.DecLink()
        }
      }

      return Index(embedded_index.dst_vers_, stage_, stage_graph_->MakeLoan());
    }

    // Form a new index from this one at the same stage and version.
    Index Clone() {
      CHECK(valid()) << "Index invalid.";
      std::shared_lock<std::mutex> slock(stage_graph_->m_);
      Version &v = stage_graph_->GetVersionOrDie(vers_);
      std::unique_lock<std::mutex> lock(v.m_);
      v.inc();
      v.pin();
      return Index(vers_, stage_, stage_graph_->MakeLoan());
    }

    // Release this index, and replace ourselves with the incoming index.
    void Replace(Index &&index) {
      Release();
      *this = index;
    }

    // Decrement the ref/pin in our version and run compaction. Any newly
    // inaccessible versions resulting from this release will be removed, as
    // will any scenes at those versions.
    void Release() {
      if (!valid()) return;
      invalidate();

      std::unique_lock<std::mutex> lock(stage_graph_->m_);
      Version &v = stage_graph_->GetVersionOrDie(vers_);
      v.dec();
      v.unpin();

      stage_graph_->Compact();
    }

   private:
    VersionId vers_;

    // We're safe to keep a pointer to the stage as the StageGraph's map of
    // stages won't be modified after construction.
    Stage *stage_;

    util::Loan<StageGraph> stage_graph_;
  };

 private:
  StageGraph(Builder &builder) {
    // Insert the base version. The... "original" version.
    Version &v =
        version_graph_.insert(0, Version(0, kNoVersion, this)).second->first;
    for (auto &frag : builder.fragments_) {
      // Note: we copy frag.name into two different strings and then move it
      // into the version stage list for 2 string copies instead of 3.
      stages_.insert(
          std::string(frag.name),
          Stage(std::string(frag.name), std::move(frag.shared_content),
                Scene{nullptr, std::move(frag.content)}, this));
      v.AddStage(std::move(frag.name));
    }
  }
};

}  // namespace tlg_lib
#endif  // TLG_LIB_INDEXSTAGEGRAPH_H_
