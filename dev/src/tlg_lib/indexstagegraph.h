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

/*

auto b = StageGraph<int, int>::builder();
b.push("lvl0").push(...).push(...);

StageGraph<int, int> sg = b.buildAndClear();

auto index1 = sg.MakeIndex("lvl0");

auto index2 = index1.Clone();

index1.SetContent(5);

auto e = index1.Embed(index2);

index1.SetContent(e)


*/

namespace tlg_lib {

template <typename SharedC, typename C>
class StageGraph : public util::Lender {
 private:
  class Index;
  friend class Index;

  typedef uint32_t VersionId;

  struct Version : public util::NonCopyable {
    friend class Index;

    Version(VersionId vers, VersionId base, StageGraph *const stage_graph)
        : parent_count_(0),
          vers_(vers),
          base_vers_(basis),
          refs_(1),
          pinning_refs_(1),
          stage_graph_(stage_graph) {}

    Version(Version &&version)
        : parent_count_(version.parent_count_),
          vers_(version.vers_),
          base_vers_(version.base_vers_),
          refs_(version.refs_),
          pinning_refs_(version.pinning_refs_),
          stage_graph_(version.stage_graph_) {}

    ~Version() {
      for (const auto &stage : in_stages_) {
        auto &stages_i = stage_graph_->stages_.find(stage);
        CHECK_NE(stages_i, stage_graph_->stages_.end())
            << "While deleting version " << vers_
            << ", tried to delete in-stage '" << stage
            << "' which does not exist.";
        stages_i.second->DeleteScene(vers_);
      }
      for (const auto &link_i : links_) {
        Version &v = stage_graph_->GetVersionOrDie(link_i.first);
        v.parent_count_ -= link_i.second;
      }
      // We don't need to modify outgoing links to this version in other
      // versions as we'll never remove a version before its parent.
    }

    void inc() { ++refs_; }
    void dec() {
      CHECK_NE(refs--, 0) << "Reference count underflow in version " << vers_;
    }

    void pin() { ++pinning_refs_; }
    void unpin() {
      CHECK_NE(--pinning_refs, 0) << "Pin count underflow in version " << vers_;
    }

    bool pinned() const { return pinning_refs_ >= 1; }
    bool overwritable_by_ref() const {
      return (refs_ <= 1) && (parent_count_ > 0);
    }

    template <typename StringT>
    void AddStage(StringT &&stage) {
      base::type_assert::AssertIsConvertibleTo<std::string, StringT>();
      in_stages_.push_back(std::forward<StringT>(stage));
    }

    void IncLink(VersionId link_vers) {
      auto &link_i = links_.find(link_vers);
      if (link_i == links_.end()) link_i = links_.insert(link_vers, 0);
      ++(link_i->second);
    }

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

    std::mutex m_;

    uint32_t parent_count_;

    const VersionId base_vers_;
    const VersionId vers_;

    std::unordered_map<VersionId, uint32_t> links_;
    // Only ever grows, never shrinks
    std::vector<std::string> in_stages_;
    uint32_t refs_;
    uint32_t pinning_refs_;

    StageGraph *const stage_graph_;
  };

  struct Scene : public util::Lender {
    util::Loan<Scene> MakeLoan() { return MakeLoan<Scene>(); }

    Scene(Scene &&scene)
        : basis_(std::move(scene.basis_)),
          content_(std::move(scene.content_)) {}
    util::Loan<Scene> basis_;
    C content_;
  };

  struct Stage : public util::NonCopyable {
    friend class Index;

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

    util::deleter_ptr<SharedC> shared_content() const {
      std::shared_lock<std::mutex> slock(shared_content_mu_);
      ++(*shared_content_refs_);
      return util::deleter_ptr<SharedC>(
          shared_content_.get(),
          [refs = shared_content_refs_.get()](SharedC *c) { --(*refs); });
    }

    const std::string name_;
    mutable std::atomic<uint32_t> shared_content_refs_;

    std::mutex shared_content_mu_;
    std::unique_ptr<SharedC> shared_content_;

    std::map<VersionId, Scene> scenes_;
    StageGraph *const stage_graph_;
  };

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

  void DecParentsAndMaybeAddToFrontier(Version *v,
                                       const std::set<VersionId> deleted_set) {
    if ((--(v->parent_count) == 0) &&
        (delete_vers.find(v->vers()) == delete_vers.end())) {
      frontier_.insert(v->vers());
    }
  }

  void Compact() {
    std::set<VersionId> delete_vers;
    for (const auto &version_i : version_graph_) {
      if (version_i->first != 0) delete_vers.insert(version_i->first)
    }

    // From every frontier, find versions that are inaccessible from pins
    for (const auto &frontier_i = frontier_.begin();
         (frontier_i != frontier_.end()) && (!delete_vers.empty());
         ++frontier_i) {
      std::vector<VersionId> persue;
      persue.push_back(*frontier_i);

      bool inacessible = true;
      while (!persue.empty()) {
        Version *v = &GetVersionOrDie(persue.back());
        persue.pop_back();

        persue.push_back(v->base());
        for (const auto &link_i : v->links()) persue.push_back(link_i->first);

        if (v->pinned()) inacessible = false;
        if (!inacessible) delete_vers.erase(v->vers());
      }
    }

    // Delete remaining inaccessible versions and rebuild frontiers.
    for (const auto &vers = delete_vers.rbegin(); vers != delete_vers.rend();
         ++vers) {
      Version *v = &GetVersionOrDie(*vers);
      if (frontier_.find(v->vers())) frontier_.erase(v->vers());

      DecParentsAndMaybeAddToFrontier(&GetVersionOrDie(v->base()), delete_vers);
      for (auto &link_v : v->links()) {
        DecParentsAndMaybeAddToFrontier(&GetVersionOrDie(link_v->first),
                                        delete_vers);
      }

      version_graph_.erase(v->vers());
    }
  }

  VersionId CreateVersion() { return ++vers_generator_; }

  std::mutex m_;

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

  ~StageGraph() {
    while (!version_graph_.empty()) {
      scenes_.erase(version_graph_.rbegin());
    }
  }

  Index CreateIndex(const std::string &stage_name) {
    Version &v = GetVersionOrDie(0);
    v.pin();
    v.ref();
    return Index(0, &GetStageOrDie(stage_name), MakeLoan<StageGraph>());
  }

  class Builder : util::NonCopyable {
    friend class StageGraph<SharedC, C>;

   public:
    template <typename StringT>
    Builder &push(StringT &&name, SharedC &&shared_content, C &&content) {
      base::type_assert::AssertIsConvertibleTo<std::string, StringT>();
      fragments_.push_back(
          StageFragment{std::forward<StringT>(name), shared_content, content});
      return *this;
    }

    Builder &pop() {
      fragments_.pop_back();
      return *this;
    }

    uint32_t size() const { return fragments_.size(); }

    StageGraph<SharedC, C> buildAndClear() {
      auto sg = StageGraph<SharedC, C>(*this);
      fragments_.clear();
      return sg;
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

    std::string stage_;
    VersionId src_vers_;
    VersionId dst_vers_;
  };

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

    util::deleter_ptr<SharedC> GetSharedContent() const {
      CHECK(valid()) << "Index invalid.";
      return stage_->shared_content();
    }

    void SetSharedContent(SharedC &&shared_content) {
      CHECK(valid()) << "Index invalid.";
      stage_->UpdateSharedContent(shared_content);
    }

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
        consumer(**content_i);
      }
    }

    void Go(const std::string &stage_name) {
      CHECK(valid()) << "Index invalid.";
      std::shared_lock<std::mutex> slock(stage_graph_->m_);
      stage_ = &(stage_graph_->GetStageOrDie(stage_name));
    }

    void SetContent(C &&content, bool link_base = false) {
      CHECK(valid()) << "Index invalid.";
      std::unique_lock<std::mutex> lock(stage_graph_->m_);

      Version &v = stage_graph_->GetVersionOrDie(vers_);

      std::pair<Scene &, VersionId> base_info = stage_->GetScene(vers_);
      Scene &base_scene = base_info.first;
      const VersionId base_vers = base_info.second;

      if (v.overwritable_by_ref() && (base_vers == vers_)) {
        base_scene.content = content;
        return;
      }

      VersionId new_vers = vers_;
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
        v.AddStage(std::string(stage_->name_));
      }

      stage_->AddScene(
          new_vers,
          Scene{link_base ? base_scene.MakeLoan() : util::Loan(), content});
      vers_ = new_vers;
    }

    EmbeddedIndex Embed(Index &&index) {
      CHECK(valid()) << "Index invalid.";
      CHECK_NE(index, this) << "Cannot embed index within itself.";

      {
        std::shared_lock<std::mutex> slock(stage_graph_->m_);

        Version &embed_v = stage_graph_->GetVersionOrDie(index.vers_);
        Version &v = stage_graph_->GetVersionOrDie(vers_);

        std::unique_lock<std::mutex> embed_lock(embed_v.m_, std::defer_lock);
        std::unique_lock<std::mutex> lock(v.m_, std::defer_lock);
        std::lock(embed_lock, lock);

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

    Index Clone() {
      CHECK(valid()) << "Index invalid.";
      std::shared_lock<std::mutex> slock(stage_graph_->m_);
      Version &v = stage_graph_->GetVersionOrDie(vers_);
      std::unique_lock<std::mutex> lock(v.m_);
      v.inc();
      v.pin();
      return Index(vers_, stage_, stage_graph_->MakeLoan());
    }

    void Replace(Index &&index) {
      Release();
      *this = index;
    }

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
    Stage *stage_;

    mutable util::Loan<StageGraph> stage_graph_;
  };

 private:
  StageGraph(Builder &builder) {
    Version &v =
        version_graph_
            .insert(0, Version(0, std::numeric_limits<VersionId>::max(), this))
            .second->first;
    for (auto &frag : builder.fragments_) {
      // Note: we copy frag.name into two different strings and then move it
      // into the version stage set for 2 copies instead of 3.
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
