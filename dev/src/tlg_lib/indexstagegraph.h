#ifndef TLG_LIB_INDEXSTAGEGRAPH_H_
#define TLG_LIB_INDEXSTAGEGRAPH_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/string_view.h"
#include "glog/logging.h"
#include "util/loan.h"
#include "util/noncopyable.h"

namespace tlg_lib {
template <typename STATIC_C, typename C>
class IndexStageGraph;

template <typename STATIC_C, typename C>
class StageGraph : public util::NonCopyable {
  friend class IndexStageGraph<STATIC_C, C>;
  static constexpr int kInitialIndexLevel = 0;
  typedef uint32_t Level;

 public:
  StageGraph() {}

  static std::unique_ptr<StageGraph<STATIC_C, C>> Create() {
    return std::make_unique<StageGraph<STATIC_C, C>>();
  }

  void AddStage(const std::string& name,
                std::unique_ptr<STATIC_C> static_content,
                std::unique_ptr<C> content) {
    auto stage_insert =
        stages_.emplace(name, Stage(name, std::move(static_content)));
    CHECK(stage_insert.second) << "Stage '" << name << "' already exists.";
    Stage& stage = stage_insert.first->second;
    auto& cell =
        stage.cells_.emplace(0, std::map<Level, Scene>()).first->second;
    cell.emplace(0, Scene(std::move(content)));
    cleanup_index_.insert(
        DeletingSceneRef(stage.MakeLoanForDeletingReference(), 0, 0));
  }

  // For testing. Returns the number of scenes across all stages and cells.
  int size() const { return cleanup_index_.size(); }

  class Index {
    friend class IndexStageGraph<STATIC_C, C>;
    friend class StageGraph<STATIC_C, C>;

   public:
    Index()
        : stage_(""), cell_id_(kInitialIndexCell), level_(kInitialIndexLevel) {}
    bool invalid() const { return stage_.empty(); }

   private:
    void Invalidate() { stage_.clear(); }

    std::string stage_;
    CellId cell_id_;
    Level level_;
  };

 private:
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
          content_(std::move(other.content_)) {}

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
   public:
    Stage(absl::string_view name, std::unique_ptr<STATIC_C> static_content)
        : name_(name), static_content_(std::move(static_content)) {}

    Stage(Stage&& other)
        : name_(std::move(other.name_)),
          cells_(std::move(other.cells_)),
          static_content_(std::move(other.static_content_)) {
      // Invalidate Stage "other"
      other.name_.clear();
    }

    util::Loan<Stage> MakeLoanForDeletingReference() {
      CHECK(!name_.empty()) << "Stage invalidated, was it moved?";
      return MakeLoan<Stage>();
    }

    Scene& GetScene(Index index) {
      CHECK(!name_.empty()) << "Stage invalidated, was it moved?";

      auto cell_iter = cells_.lower_bound(index.cell_id_);
      if ((cell_iter == cells_.end()) || (cell_iter->first > index.cell_id_)) {
        --cell_iter;
      }
      CHECK(cell_iter != cells_.end())
          << "Cell id lower than that of all cells in stage.";

      std::map<Level, Scene>& cell = cell_iter->second;
      auto scene_iter = cell.lower_bound(index.level_);
      if ((scene_iter == cell.end()) || (scene_iter->first > index.level_)) {
        --scene_iter;
      }
      CHECK(scene_iter != cell.end())
          << "Index level lower than that of all scenes in cell.";

      return scene_iter->second;
    }

    // Delete a scene at the specified level from the cell at the specified id.
    // If deleting this scene empties the cell, delete the cell as well.
    void DeleteScene(CellId cell_id, Level level) {
      CHECK(!name_.empty()) << "Stage invalidated, was it moved?";
      auto stack_i = cells_.find(cell_id);
      CHECK(stack_i != cells_.end())
          << "When deleting scene, cell_id: " << cell_id
          << " isn't valid for stage: " << name_;
      auto& stack = stack_i->second;
      // Sanity check to stop hard to find bugs considering a failed erase
      // doesn't report anything back.
      CHECK(stack.find(level) != stack.end())
          << "When deleting scene, level: " << level
          << " isn't valid for cell_id: " << cell_id << " in stage: " << name_;
      stack.erase(level);
      if (stack.empty()) cells_.erase(stack_i);
    }

    std::string name_;
    std::vector<Scene> scenes_;
    std::unique_ptr<STATIC_C> static_content_;
  };

  Stage& GetStage(Index index) {
    auto stage_iter = stages_.find(index.stage_);
    CHECK(stage_iter != stages_.end()) << "Index points to non-existant stage.";
    return stage_iter->second;
  }

  std::unordered_map<std::string, Stage> stages_;
};
/*
template <typename STATIC_C, typename C>
class IndexStageGraph : public util::NonCopyable {
  typedef typename StageGraph<STATIC_C, C>::Scene GraphScene;
  typedef typename StageGraph<STATIC_C, C>::Stage GraphStage;
  typedef typename StageGraph<STATIC_C, C>::Level Level;
  typedef typename StageGraph<STATIC_C, C>::CellId CellId;

 public:
  typedef typename StageGraph<STATIC_C, C>::Index GraphIndex;

  IndexStageGraph(std::unique_ptr<StageGraph<STATIC_C, C>> stage_graph)
      : stage_graph_(std::move(stage_graph)),
        actual_cell_(StageGraph<STATIC_C, C>::kInitialIndexCell) {
    // An unreachable cell as the base state
    index_.cell_id_ = StageGraph<STATIC_C, C>::kInitialIndexCell - 1;
  }

  virtual ~IndexStageGraph() {}

  // Go to a different stage, automatically determining the scene.
  void GoStage(const std::string& stage) { index_.stage_ = stage; }

  // Go to a specific scene index.
  void GoIndex(const GraphIndex& index) {
    CHECK(!index_.invalid()) << "Index invalid.";
    if (!retained().invalid()) {
      stage_graph_->cleanup_index_.DeleteCells(actual_cell_, index.level_);
    } else {
      stage_graph_->cleanup_index_.DeleteLevels(index.cell_id_, index.level_);
    }
    ++actual_cell_;
    index_ = index;
  }

  absl::string_view stage() const {
    CHECK(!index_.invalid()) << "Index invalid.";
    return index_.stage_;
  }

  // Starting with the current scene, resolver_fn will be called on the content
  // of each scene in the chain of bases ending at the first empty basis.
  void GetStageContent(std::function<void(const C* content)> resolver_fn,
                       int32_t max_depth = -1) const {
    CHECK(!index_.invalid()) << "Index invalid.";
    const GraphScene* scene =
        &(stage_graph_->GetStage(index_).GetScene(index_));
    while (scene != nullptr) {
      resolver_fn(scene->content());
      scene = scene->basis();
      if (--max_depth == 0) return;
    }
  }

  // Get the static content of a stage
  const STATIC_C* GetStageStaticContent() const {
    CHECK(!index_.invalid()) << "Index invalid.";
    return stage_graph_->GetStage(index_).static_content_.get();
  }

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
    CHECK(!index_.invalid()) << "Index invalid.";
    GraphStage& stage = stage_graph_->GetStage(index_);
    auto cell_iter = stage.cells_.find(actual_cell_);
    if (cell_iter == stage.cells_.end()) {
      cell_iter =
          stage.cells_.emplace(actual_cell_, std::map<Level, GraphScene>())
              .first;
    }
    auto& cell = cell_iter->second;
    auto scene_iter = cell.find(index_.level_);

    if (scene_iter == cell.end()) {
      if (link) {
        GraphScene& prev_scene = stage.GetScene(index_);
        cell.emplace(
            index_.level_,
            GraphScene(std::move(prev_scene.MakeBasis()), std::move(update)));
      } else {
        cell.emplace(index_.level_, std::move(update));
      }
      stage_graph_->cleanup_index_.insert(
          StageGraph<STATIC_C, C>::DeletingSceneRef(
              stage.MakeLoanForDeletingReference(), actual_cell_,
              index_.level_));
    } else {
      scene_iter->second.ResetContent(std::move(update));
    }
    index_.cell_id_ = actual_cell_;
    if (release) Release();
  }

  // Retain the index of the current scene. If already retaining when called,
  // the old retained index is discarded.
  const GraphIndex& Retain() {
    CHECK(!index_.invalid()) << "Index invalid.";
    const Level prev_level = index_.level_;
    if (retained().invalid()) ++index_.level_;
    retained_index_ = index_;
    retained_index_.level_ = prev_level;
    return retained_index_;
  }

  const GraphIndex& retained() const {
    CHECK(!index_.invalid()) << "Index invalid.";
    return retained_index_;
  };

 private:
  // Relase the retained index; asserts that an index was retained.
  void Release() {
    CHECK(!index_.invalid()) << "Index invalid.";
    retained_index_.Invalidate();
  }

  std::unique_ptr<StageGraph<STATIC_C, C>> stage_graph_;
  GraphIndex index_;
  CellId actual_cell_;
  GraphIndex retained_index_;
};
*/

}  // namespace tlg_lib
#endif  // TLG_LIB_INDEXSTAGEGRAPH_H_
