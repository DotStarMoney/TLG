#include "tlg_lib/indexstagegraph.h"

#include <functional>
#include <memory>

#include "gtest/gtest.h"

using std::string;
using tlg_lib::StageGraph;

namespace {
StageGraph<string, string> makeSimpleGraph() {
  auto graph_builder = StageGraph<string, string>::builder();
  graph_builder.push("a", "shared_content_a", "Content_a")
      .push("b", "shared_content_b", "Content_b")
      .push("c", "shared_content_c", "Content_c");

  return graph_builder.buildAndClear();
}
}  // namespace

TEST(IndexStageGraphTest, SimpleJumpWithoutRetention) {
  auto graph = makeSimpleGraph();

}
