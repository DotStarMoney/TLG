#include "tlg_lib/indexstagegraph.h"

#include <memory>

#include "gtest/gtest.h"

using tlg_lib::IndexStageGraph;
using tlg_lib::StageGraph;

struct SimpleContent {
  StageGraph<int, SimpleContent>::Index index_a;
  StageGraph<int, SimpleContent>::Index index_b;
  int x;
  SimpleContent(int x) : x(x) {}
  SimpleContent(int x, StageGraph<int, SimpleContent>::Index index_a)
      : index_a(index_a), x(x) {}
  SimpleContent(int x, StageGraph<int, SimpleContent>::Index index_a,
                StageGraph<int, SimpleContent>::Index index_b)
      : index_a(index_a), index_b(index_b), x(x) {}
};

TEST(IndexStageGraphTest, CommonUseCase) {
  auto test_graph = StageGraph<int, SimpleContent>::Create();
  test_graph->AddStage("StageA", std::make_unique<int>(11),
                       std::make_unique<SimpleContent>(111110));
  test_graph->AddStage("StageB", std::make_unique<int>(222),
                       std::make_unique<SimpleContent>(222220));
  test_graph->AddStage("StageC", std::make_unique<int>(3333),
                       std::make_unique<SimpleContent>(333330));

  auto size = [graph = test_graph.get()]() { return graph->size(); };

  ASSERT_EQ(size(), 3);

  IndexStageGraph<int, SimpleContent> graph(std::move(test_graph));

  SimpleContent content(0);
  auto fetch_content = [&graph, &content]() -> const SimpleContent& {
    graph.GetStageContent(
        [&content](const SimpleContent* ns) { content = *ns; });
    return content;
  };

  graph.GoStage("StageC");
  graph.UpdateStageContent(std::make_unique<SimpleContent>(333339));

  ASSERT_EQ(fetch_content().x, 333339);
  ASSERT_EQ(size(), 4);

  graph.Retain();
  graph.UpdateStageContent(
      std::make_unique<SimpleContent>(333338, graph.retained()), false, true);

  auto replace_content = fetch_content();
  ASSERT_EQ(replace_content.x, 333338);
  ASSERT_EQ(size(), 5);

  replace_content.x = 233338;
  auto replace_content_ptr = std::make_unique<SimpleContent>(0);
  *replace_content_ptr = replace_content;
  graph.UpdateStageContent(std::move(replace_content_ptr));

  ASSERT_EQ(fetch_content().x, 233338);
  ASSERT_EQ(size(), 5);

  graph.GoStage("StageB");

  graph.UpdateStageContent(std::make_unique<SimpleContent>(222229));
  
  ASSERT_EQ(fetch_content().x, 222229);
  ASSERT_EQ(size(), 6);

  graph.GoStage("StageA");
  graph.Retain();
  graph.UpdateStageContent(
      std::make_unique<SimpleContent>(111119, graph.retained()), false, true);

  ASSERT_EQ(fetch_content().x, 111119);
  ASSERT_EQ(size(), 7);

  graph.GoStage("StageC");
  graph.UpdateStageContent(std::make_unique<SimpleContent>(333337));

  ASSERT_EQ(fetch_content().x, 333337);
  ASSERT_EQ(size(), 8);

  graph.GoStage("StageA");

  auto current_content = fetch_content();
  ASSERT_EQ(current_content.x, 111119);

  graph.GoIndex(current_content.index_a);

  /*
  graph.GoStage("StageB");
  graph.GetStageContent([&noad](const SimpleContent* ns) { noad = *ns; });

  LOG(INFO) << noad.x;

  graph.GoStage("StageC");
  graph.GetStageContent([&noad](const SimpleContent* ns) { noad = *ns; });
  // was 39
  graph.UpdateStageContent(
      std::make_unique<SimpleContent>(333336, noad.index_a));

  graph.GetStageContent([&noad](const SimpleContent* ns) { noad = *ns; });
  graph.Retain();
  LOG(INFO) << noad.x;

  graph.GoIndex(noad.index_a);

  graph.GetStageContent([&noad](const SimpleContent* ns) { noad = *ns; });
  LOG(INFO) << graph.stage() << ", " << noad.x;

  graph.UpdateStageContent(
      std::make_unique<SimpleContent>(333335, graph.retained()), false, true);
  graph.GetStageContent([&noad](const SimpleContent* ns) { noad = *ns; });
  graph.GoIndex(noad.index_a);

  LOG(INFO) << noad.x;

  graph.GetStageContent([&noad](const SimpleContent* ns) { noad = *ns; });
  graph.GoIndex(noad.index_a);

  graph.GetStageContent([&noad](const SimpleContent* ns) { noad = *ns; });
  LOG(INFO) << noad.x;
  */
}
