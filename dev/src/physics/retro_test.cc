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

TEST(RetroTest, BlockGridPartiallyTouching) {
  BlockGrid grid({5, 5}, 1);

  grid.PutBlock({2, 2}, BlockGrid::BlockType::FULL);

  // Test running into only a section of a block
  TestClip(grid, {0, 1.5}, {1, 1}, {8, -2}, {1, -0.25},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {2.5, 0}, {1, 1}, {2, 8}, {0.25, 1},
           BlockGrid::ClipResult::X_ALIGNED);
  TestClip(grid, {4, 2.5}, {1, 1}, {-8, 2}, {-1, 0.25},
           BlockGrid::ClipResult::Y_ALIGNED);
  TestClip(grid, {1.5, 4}, {1, 1}, {-2, -8}, {-0.25, -1},
           BlockGrid::ClipResult::X_ALIGNED);
}

TEST(RetroTest, BlockGridThroughGap) {
  BlockGrid grid({5, 5}, 1);

  grid.PutBlock({1, 1}, BlockGrid::BlockType::FULL);
  grid.PutBlock({3, 1}, BlockGrid::BlockType::FULL);
  grid.PutBlock({3, 3}, BlockGrid::BlockType::FULL);
  grid.PutBlock({1, 3}, BlockGrid::BlockType::FULL);

  // Test moving through gaps that are exactly as wide as we are
  TestClip(grid, {0, 2}, {1, 1}, {4, 0}, {4, 0},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {2, 4}, {1, 1}, {0, -4}, {0, -4},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {4, 2}, {1, 1}, {-4, 0}, {-4, 0},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {2, 0}, {1, 1}, {0, 4}, {0, 4},
           BlockGrid::ClipResult::NO_COLLISION);
}

TEST(RetroTest, BlockGridNoCollision) {
  BlockGrid grid({6, 6}, 1);

  MakeBlockDoughnut(6, &grid);

  // Test slightly skewed movement into the walls that ends exactly on a block
  TestClip(grid, {2.5, 2.5}, {1, 1}, {1.5, 0.75}, {1.5, 0.75},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {-0.75, 1.5}, {-0.75, 1.5},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {-1.5, -0.75}, {-1.5, -0.75},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {0.75, -1.5}, {0.75, -1.5},
           BlockGrid::ClipResult::NO_COLLISION);
  // Test never moving far enough to pass through a grid line
  TestClip(grid, {2.5, 2.5}, {1, 1}, {0.25, 0.125}, {0.25, 0.125},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {-0.125, 0.25}, {-0.125, 0.25},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {-0.25, -0.125}, {-0.25, -0.125},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {2.5, 2.5}, {1, 1}, {0.125, -0.25}, {0.125, -0.25},
           BlockGrid::ClipResult::NO_COLLISION);
}

TEST(RetroTest, BlockGridOutOfBounds) {
  // Test passing through grid colliding with nothing
  {
    BlockGrid grid({3, 3}, 1);
    TestClip(grid, {-1, -1}, {1, 1}, {10, 10}, {10, 10},
             BlockGrid::ClipResult::NO_COLLISION);
    TestClip(grid, {3, -1}, {1, 1}, {-10, 10}, {-10, 10},
             BlockGrid::ClipResult::NO_COLLISION);
    TestClip(grid, {3, 3}, {1, 1}, {-10, -10}, {-10, -10},
             BlockGrid::ClipResult::NO_COLLISION);
    TestClip(grid, {-1, 3}, {1, 1}, {10, -10}, {10, -10},
             BlockGrid::ClipResult::NO_COLLISION);
  }

  {
    BlockGrid grid({3, 3}, 1);
    grid.PutBlock({1, 1}, BlockGrid::BlockType::FULL);
    // Test passing through grid with collision
    TestClip(grid, {-1, -1}, {1, 1}, {10, 10}, {1, 1},
             BlockGrid::ClipResult::Y_ALIGNED);
    TestClip(grid, {3, -1}, {1, 1}, {-10, 10}, {-1, 1},
             BlockGrid::ClipResult::Y_ALIGNED);
    TestClip(grid, {3, 3}, {1, 1}, {-10, -10}, {-1, -1},
             BlockGrid::ClipResult::Y_ALIGNED);
    TestClip(grid, {-1, 3}, {1, 1}, {10, -10}, {1, -1},
             BlockGrid::ClipResult::Y_ALIGNED);

    // Test moving completely outside of grid
    TestClip(grid, {-2, -1}, {1, 1}, {0, -1}, {0, -1},
             BlockGrid::ClipResult::NO_COLLISION);
    TestClip(grid, {3, -2}, {1, 1}, {1, 0}, {1, 0},
             BlockGrid::ClipResult::NO_COLLISION);
    TestClip(grid, {4, 3}, {1, 1}, {0, 1}, {0, 1},
             BlockGrid::ClipResult::NO_COLLISION);
    TestClip(grid, {-1, -4}, {1, 1}, {-1, 0}, {-1, 0},
             BlockGrid::ClipResult::NO_COLLISION);
  }

  // Test clipping into lone block
  {
    BlockGrid grid({1, 1}, 1);
    grid.PutBlock({0, 0}, BlockGrid::BlockType::FULL);
    TestClip(grid, {-2, -0.5}, {1, 1}, {16, -4}, {1, -0.25},
             BlockGrid::ClipResult::Y_ALIGNED);
    TestClip(grid, {-0.5, 2}, {1, 1}, {-4, -16}, {-0.25, -1},
             BlockGrid::ClipResult::X_ALIGNED);
    TestClip(grid, {2, 0.5}, {1, 1}, {-16, 4}, {-1, 0.25},
             BlockGrid::ClipResult::Y_ALIGNED);
    TestClip(grid, {0.5, -2}, {1, 1}, {4, 16}, {0.25, 1},
             BlockGrid::ClipResult::X_ALIGNED);
  }
}

TEST(RetroTest, BlockGridOneWayPlatform) {
  // Test that only downward movement counts as a collision
  BlockGrid grid({3, 3}, 1.3);
  grid.PutBlock({1, 1}, BlockGrid::BlockType::ONE_WAY_UP);
  TestClip(grid, {1.3, -2.6}, {1, 2.5}, {0, 10}, {0, 1.4},
           BlockGrid::ClipResult::X_ALIGNED);
  TestClip(grid, {3.1, 1.3}, {2.5, 1}, {-10, 0}, {-10, 0},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {1.3, 3.1}, {1, 2.5}, {0, -10}, {0, -10},
           BlockGrid::ClipResult::NO_COLLISION);
  TestClip(grid, {-2.6, 1.3}, {2.5, 1}, {10, 0}, {10, 0},
           BlockGrid::ClipResult::NO_COLLISION);
}
