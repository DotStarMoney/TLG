#include "tlg_lib/indexstagegraph.h"

#include <functional>
#include <memory>

#include "gtest/gtest.h"

using tlg_lib::StageGraph;


TEST(IndexStageGraphTest, SimpleJumpWithoutRetention) {
  auto graph_builder = StageGraph<int, int>::builder();
}

