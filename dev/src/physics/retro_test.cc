#include "physics/retro.h"

#include "glm/geometric.hpp"
#include "gtest/gtest.h"

using glm::dvec2;
using physics::retro::BlockGrid;

constexpr double kEpsilon = 0.0000001;

namespace {
void TestClip(const BlockGrid& grid, dvec2 p, dvec2 s, dvec2 v, dvec2 expected,
              BlockGrid::ClipResult::Axis expected_axis) {
  auto res = grid.ClipMovingRect(p, s, v, true);
  EXPECT_LT(glm::length(res.v - expected), kEpsilon);
  EXPECT_EQ(res.axis, expected_axis);
}

// Make a doughnut of blocks missing corner pieces
void MakeBlockDoughnut(int side_length, BlockGrid* grid) {
  for (int i = 1; i < (side_length - 1); ++i) {
    grid->PutBlock({i, 0}, BlockGrid::BlockType::FULL);
    grid->PutBlock({i, (side_length - 1)}, BlockGrid::BlockType::FULL);
    grid->PutBlock({0, i}, BlockGrid::BlockType::FULL);
    grid->PutBlock({(side_length - 1), i}, BlockGrid::BlockType::FULL);
  }
}
}  // namespace

TEST(RetroTest, BlockGridBasic) {
  BlockGrid grid({6, 6}, 1);

  MakeBlockDoughnut(6, &grid);

  // With the rect in the middle of the doughnut...

  // Test no movement.
  TestClip(grid, {2.5, 2.5}, {1, 1}, {0, 0}, {0, 0},
           BlockGrid::ClipResult::NO_COLLISION);
  // Test axis-aligned movement
  TestClip(grid, {2.5, 2.5}, {1, 1}, {10, 0}, {1.5, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {0, 10}, {0, 1.5},
           BlockGrid::ClipResult::X_ALIGNED);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {-10, 0}, {-1.5, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {0, -10}, {0, -1.5},
           BlockGrid::ClipResult::X_ALIGNED);
  // Test slightly skewed movement into the walls
  TestClip(grid, {2.5, 2.5}, {1, 1}, {10, 5}, {1.5, 0.75},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {-5, 10}, {-0.75, 1.5},
           BlockGrid::ClipResult::X_ALIGNED);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {-10, -5}, {-1.5, -0.75},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {5, -10}, {0.75, -1.5},
           BlockGrid::ClipResult::X_ALIGNED);
  // Test movement into the corners
  TestClip(grid, {2.5, 2.5}, {1, 1}, {10, 10}, {1.5, 1.5},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {-10, 10}, {-1.5, 1.5},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {-10, -10}, {-1.5, -1.5},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {10, -10}, {1.5, -1.5},
           BlockGrid::ClipResult::Y_ALIGNED);
}

TEST(RetroTest, BlockGridClipTouchingEdge) {
  BlockGrid grid({6, 6}, 1);

  MakeBlockDoughnut(6, &grid);

  // Test moving away from an edge we're touching
  TestClip(grid, {2.5, 1}, {1, 1}, {-1, 1}, {-1, 1},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {4, 2.5}, {1, 1}, {-1, -1}, {-1, -1},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {2.5, 4}, {1, 1}, {1, -1}, {1, -1},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {1, 2.5}, {1, 1}, {1, 1}, {1, 1},
           BlockGrid::ClipResult::NO_COLLISION);
  // Test moving away from a corner we're touching
  TestClip(grid, {1, 1}, {1, 1}, {1, 1}, {1, 1},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {4, 1}, {1, 1}, {-1, 1}, {-1, 1},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {4, 4}, {1, 1}, {-1, -1}, {-1, -1},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {1, 4}, {1, 1}, {1, -1}, {1, -1},
           BlockGrid::ClipResult::NO_COLLISION);
  // Test moving into an edge we're touching
  TestClip(grid, {2.5, 1}, {1, 1}, {-1, -1}, {0, 0},
           BlockGrid::ClipResult::X_ALIGNED);
  TestClip(grid, {4, 2.5}, {1, 1}, {1, -1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2.5, 4}, {1, 1}, {1, 1}, {0, 0},
           BlockGrid::ClipResult::X_ALIGNED);
  TestClip(grid, {1, 2.5}, {1, 1}, {-1, 1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  // Test moving into a corner we're touching
  TestClip(grid, {1, 1}, {1, 1}, {-1, -1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {4, 1}, {1, 1}, {1, -1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {4, 4}, {1, 1}, {1, 1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {1, 4}, {1, 1}, {-1, 1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
}

TEST(RetroTest, BlockGridMoveIntoCorner) {
  BlockGrid grid({3, 3}, 1);

  grid.PutBlock({1, 1}, BlockGrid::BlockType::FULL);

  // Test moving directly into corners
  TestClip(grid, {0, 0}, {1, 1}, {1, 1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2, 0}, {1, 1}, {-1, 1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2, 2}, {1, 1}, {-1, -1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {0, 2}, {1, 1}, {1, -1}, {0, 0},
           BlockGrid::ClipResult::Y_ALIGNED);
}

TEST(RetroTest, BlockGridPartiallyTouching) {}

TEST(RetroTest, BlockGridThroughGap) {}

TEST(RetroTest, BlockGridOutOfBounds) {}
