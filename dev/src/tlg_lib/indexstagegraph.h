#ifndef TLG_LIB_INDEXSTAGEGRAPH_H_
#define TLG_LIB_INDEXSTAGEGRAPH_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/string_view.h"
#include "base/static_type_assert.h"
#include "glog/logging.h"
#include "util/loan.h"
#include "util/noncopyable.h"

namespace tlg_lib {

template <typename SHARED_C, typename C>
class StageGraph : public util::NonCopyable {
  typedef uint32_t VersionId;

 public:
  StageGraph() {}

 private:
  class Version : public util::NonCopyable {
   public:
    template <typename StringT>
    Version(VersionId base, StringT&& orig_stage) : base_vers_(basis) {
      AddStage(std::forward<StringT>(orig_stage));
    }

    ~Version() {
      // delete stages_ versions
    }

    template <typename StringT>
    void AddStage(StringT&& stage) {
      base::type_assert::AssertIsConvertibleTo<std::string, StringT>();
      stage_graph_.push_back(std::forward<StringT>(stage));
    }

    void AddLink(VersionId version) { aux_links_.push_back(version); }
    VersionId basis() const { return base_vers_; }
    const std::vector<VersionId>& links() const { return aux_links_; }

   private:
    const VersionId base_vers_;
    std::vector<VersionId> aux_links_;
    std::vector<std::string> stages_;
  };

  class Scene : public util::Lender {};

  class Stage : public util::Lender {
   public:
    Stage(std::unique_ptr<SHARED_C> shared_content, Scene&& base_scene) {
      UpdateSharedContent(shared_content);
      scenes_.insert(0, base_scene);
    }

    Scene& GetScene(VersionId vers) {
      std::unordered_map<VersionId, Scene>::iterator scene_i;
      for (;;) {
        scene_i = scenes_.find(vers);
        if (scene_i != scenes_.end()) break;

        auto vers_i = version_graph_.find(vers);
        CHECK_NE(vers_i, version_graph_.end())
            << "Version " << vers << " does not exist.";
        vers = vers_i->second.basis();
      }
      return scene_i->second;
    }

    void UpdateSharedContent(std::unique_ptr<SHARED_C> shared_content) {
      shared_content_.reset(shared_content);
    }

    util::Loan<SHARED_C> shared_content() const {
      return MakeLoan<SHARED_C>(shared_content_.get()));
    }

   private:
    std::unique_ptr<SHARED_C> shared_content_;
    std::unordered_map<VersionId, Scene> scenes_;
  };

  std::unordered_map<VersionId, Version> version_graph_;
  std::unordered_map<std::string, Stage> stage_graph_;
};

/*
template <typename STATIC_C, typename C>
class IndexStageGraph;

template <typename STATIC_C, typename C>
class StageGraph : public util::NonCopyable {
  friend class IndexStageGraph<STATIC_C, C>;
  static constexpr int kInitialIndexLevel = 0;
  typedef int32_t Level;

 public:
  static std::unique_ptr<StageGraph<STATIC_C, C>> Create() {
    return std::unique_ptr<StageGraph<STATIC_C, C>>(
        new StageGraph<STATIC_C, C>());
  }

  StageGraph(StageGraph&& stage_graph)
      : stage_order_(std::move(stage_graph.stage_order_)),
        stages_(std::move(stage_graph.stages_)) {}

  void AddStage(const std::string& name,
                std::unique_ptr<STATIC_C> static_content,
                std::unique_ptr<C> content) {
    auto stage_insert =
        stages_.emplace(name, Stage(name, std::move(static_content),
                                    static_cast<int>(stage_order_.size())));
    CHECK(stage_insert.second) << "Stage '" << name << "' already exists.";
    Stage& stage = stage_insert.first->second;
    stage_order_.emplace_back(stage.MakeStageLoan());
    stage.scenes_.emplace_back(0, std::move(content));
  }

  // For testing: total number of scenes
  int size() const {
    int scenes = 0;
    for (const auto& stage_i : stages_) {
      scenes += static_cast<int>(stage_i.second.scenes_.size());
    }
    return scenes;
  }

 private:
  StageGraph() {}

  class Scene : util::Lender {
   public:
    Scene(Level level, std::unique_ptr<C> content)
        : level_(level), content_(std::move(content)) {}
    Scene(Level level, util::Loan<Scene>&& basis, std::unique_ptr<C> content)
        : level_(level),
          basis_(std::move(basis)),
          content_(std::move(content)) {}

    // This should invalidate the content_ ptr
    Scene(Scene&& other)
        : basis_(std::move(other.basis_)),
          content_(std::move(other.content_)),
          level_(other.level_) {}

    util::Loan<Scene> MakeBasis() { return MakeLoan<Scene>(); }

    void ResetContent(std::unique_ptr<C> new_content) {
      content_.reset();
      content_ = std::move(new_content);
    }

    const C* content() const { return content_.get(); }
    const Scene* basis() const { return basis_.get(); }
    Level level() const { return level_; }

   private:
    const Level level_;
    util::Loan<Scene> basis_;
    std::unique_ptr<C> content_;
  };

  class Stage : public util::Lender {
    friend class StageGraph<STATIC_C, C>;

   public:
    Stage(absl::string_view name, std::unique_ptr<STATIC_C> static_content,
          int order_index)
        : name_(name),
          static_content_(std::move(static_content)),
          order_index_(order_index) {}

    Stage(Stage&& other)
        : name_(std::move(other.name_)),
          static_content_(std::move(other.static_content_)),
          scenes_(std::move(other.scenes_)),
          order_index_(other.order_index_) {
      // Invalidate Stage "other"
    }

    ~Stage() {
      // To ensure we don't screw up any of the loans given to higher scenes
      // from lower scenes, we must explicitly delete things in this order.
      while (!scenes_.empty()) scenes_.pop_back();
    }

    Scene& GetScene(int level_lo, int level_hi) {
      CHECK(!name_.empty()) << "Stage invalidated.";
      CHECK_GE(level_lo, 0);
      CHECK_GE(level_hi, level_lo);
      if (max_level() >= level_hi) return scenes_.back();
      return GetSceneIndexAtOrBelow(level_lo);
    }

    int max_level() const { return scenes_.back().level(); }

    STATIC_C* static_content() const { return static_content_.get(); }

   private:
    std::string name_;
    std::unique_ptr<STATIC_C> static_content_;
    std::vector<Scene> scenes_;
    int order_index_;

    util::Loan<Stage> MakeStageLoan() {
      CHECK(!name_.empty()) << "Stage invalidated.";
      return MakeLoan<Stage>();
    }

    // Basically an implementation of binary search that fits our
    // calling/level comparison semantics
    Scene& GetSceneIndexAtOrBelow(int level) {
      CHECK(!name_.empty()) << "Stage invalidated.";
      int lo = 0;
      int hi = static_cast<int>(scenes_.size() - 1);
      while ((hi - lo) > 1) {
        int mid = (lo + hi) >> 1;
        if (scenes_[mid].level() > level) {
          hi = mid;
          continue;
        } else if (scenes_[mid].level() < level) {
          lo = mid;
          continue;
        }
        return scenes_[mid];
      }
      if (scenes_[hi].level() == level) return scenes_[hi];
      return scenes_[lo];
    }
  };

  Stage& GetStage(const std::string& stage) {
    auto stage_iter = stages_.find(stage);
    CHECK(stage_iter != stages_.end()) << "Stage does not exist.";
    return stage_iter->second;
  }

  void DeleteScenesAboveLevel(Level level) {
    CHECK_GE(level, 0) << "Cannot delete at or below level 0.";
    for (int s = static_cast<int>(stage_order_.size() - 1); s >= 0; --s) {
      auto& stage = *stage_order_[s].get();
      if (stage.max_level() <= level) return;
      while (stage.max_level() > level) stage.scenes_.pop_back();
    }
  }

  void AddScene(Stage& stage, int level, util::Loan<Scene> basis,
                std::unique_ptr<C> content) {
    CHECK_GT(level, stage.max_level()) << "New scene predates latest in stage.";
    stage.scenes_.emplace_back(level, std::move(basis), std::move(content));
    // Adjust stage position in order: swap with stages above us until sorted
    // order is maintained
    while ((stage.order_index_ < (stage_order_.size() - 1)) &&
           (stage_order_[stage.order_index_ + 1]->max_level() < level)) {
      Stage& higher_stage = *(stage_order_[stage.order_index_ + 1].get());
      std::swap(stage_order_[higher_stage.order_index_],
                stage_order_[stage.order_index_]);
      --higher_stage.order_index_;
      ++stage.order_index_;
    }
  }

  std::unordered_map<std::string, Stage> stages_;
  // Sorted order of stages by highest level from least to greatest
  std::vector<util::Loan<Stage>> stage_order_;
};

template <typename STATIC_C, typename C>
class IndexStageGraph : public util::NonCopyable {
  typedef typename StageGraph<STATIC_C, C>::Scene GraphScene;
  typedef typename StageGraph<STATIC_C, C>::Stage GraphStage;
  typedef typename StageGraph<STATIC_C, C>::Level Level;

 public:
  struct Index {
    Index() : level(0) {}
    Index(const std::string& stage, Level level) : stage(stage), level(level) {}

    std::string stage;
    Level level;
    bool valid() const { return !stage.empty(); }
    void Invalidate() { stage.clear(); }
  };

  IndexStageGraph(std::unique_ptr<StageGraph<STATIC_C, C>> graph_)
      : graph_(std::move(graph_)),
        visiting_("", 1),
        cur_(1),
        cur_last_jump_(1) {}

  IndexStageGraph(IndexStageGraph&& index_graph)
      : graph_(std::move(index_graph.graph_)),
        visiting_(index_graph.visiting_),
        cur_(index_graph.cur_),
        cur_last_jump_(index_graph.cur_last_jump_),
        retained_(index_graph.retained_) {}

  virtual ~IndexStageGraph() {}

  // Go to a different stage, automatically determining the scene.
  void GoStage(const std::string& stage) { visiting_.stage = stage; }

  // Go to a specific scene index.
  void GoIndex(const Index& index) {
    CHECK(visiting_.valid()) << "Index invalid.";
    CHECK(index.valid()) << "Jump index invalid.";
    if (retained_.valid()) {
      // Delete everything at the current level
      graph_->DeleteScenesAboveLevel(cur_ - 1);
    } else {
      // Delete everything above the level to which we're jumping
      graph_->DeleteScenesAboveLevel(index.level);
      cur_ = index.level;
    }
    cur_last_jump_ = cur_;
    visiting_ = index;
  }

  absl::string_view stage() const {
    CHECK(visiting_.valid()) << "Index invalid.";
    return visiting_.stage;
  }

  // Starting with the current scene, resolver_fn will be called on the content
  // of each scene in the chain of bases ending at the first empty basis.
  void GetStageContent(std::function<void(const C* content)> resolver_fn,
                       int32_t max_depth = -1) const {
    CHECK(visiting_.valid()) << "Index invalid.";
    // Get the scene either at the visiting level, or at a newer level if we've
    // updated it since.
    const GraphScene* scene = &(graph_->GetStage(visiting_.stage)
                                    .GetScene(visiting_.level, cur_last_jump_));
    while (scene != nullptr) {
      resolver_fn(scene->content());
      scene = scene->basis();
      if (--max_depth == 0) return;
    }
  }

  // Get the static content of a stage
  STATIC_C* GetStageStaticContent() {
    CHECK(visiting_.valid()) << "Index invalid.";
    return stage_graph_->GetStage(index_).static_content();
  }

  // Retain the index of the current scene. If already retaining when called,
  // the old retained index is discarded and the current level will not increase
  const Index& Retain() {
    CHECK(visiting_.valid()) << "Index invalid.";
    if (!retained_.valid()) ++cur_;
    retained_ = visiting_;
    return retained_;
  }

  const Index& retained() const {
    CHECK(!index_.invalid()) << "Index invalid.";
    return retained_index_;
  };

  // Update the content of a scene, potentially creating a new scene and
  // assigning the index to it.
  //
  // Also optionally creates a link to the current
  // scene in the new scene should it be created. A link means the scene from
  // which one was created will be "visited" when calling GetStageContent. This
  // is useful when implementing some variant of incremental re-construction of
  // a scene from its predecessors.
  //
  // If release is true, the current retained index will be invalidated.
  void UpdateStageContent(std::unique_ptr<C> update, bool link = false,
                          bool release = false) {
    CHECK(visiting_.valid()) << "Index invalid.";
    GraphStage& stage = graph_->GetStage(visiting_.stage);
    if (stage.max_level() == cur_) {
      GraphScene* scene = &(stage.GetScene(visiting_.level, cur_last_jump_));
      scene->ResetContent(std::move(update));
      return;
    }

    graph_->AddScene(
        stage, cur_,
        link ? stage.GetScene(visiting_.level, cur_last_jump_).MakeBasis()
             : util::Loan<GraphScene>(),
        std::move(update));

    if (release) Release();
  }

 private:
  std::unique_ptr<StageGraph<STATIC_C, C>> graph_;
  Index visiting_;
  Level cur_;
  Level cur_last_jump_;

  Index retained_;

  // Relase the retained index; asserts that an index was retained.
  void Release() {
    CHECK(retained_.valid()) << "Retained index invalid.";
    retained_.Invalidate();
  }
};
*/
}  // namespace tlg_lib
#endif  // TLG_LIB_INDEXSTAGEGRAPH_H_
