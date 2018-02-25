#include "physics/retro.h"

#include <math.h>

using glm::dvec2;
using glm::ivec2;

namespace physics {
namespace retro {

BlockGrid::BlockGrid(ivec2 dims, double l)
    : blocks_(dims.x * dims.y, BlockType::NONE), l_(l), dims_(dims) {}

void BlockGrid::PutBlock(ivec2 p, BlockType block) {
  blocks_[p.y * dims_.x + p.x] = block;
}

BlockGrid::BlockType BlockGrid::GetBlock(ivec2 p) {
  return blocks_[p.y * dims_.x + p.x];
}

namespace {
// In a 1D block lattice, given a segment p -> p_extent moving by v in a lattice
// of length "max", get the range of indices of blocks we could collide with
// moving over v. Returns false if the grid will never overlap the moving
// segment. If the segment could not overlap an adjacent block, the second index
// in range will preceed the first in the sign of v.
bool GetCollidingGridRange(double p, double p_extent, double v, int max,
                           int range[2]) {
  int range[2];
  if (v >= 0) {
    range[0] = static_cast<int>(std::ceil(p_extent));
    range[1] = static_cast<int>(std::ceil(p_extent + v)) - 1;
    if ((range[0] >= max) || (range[1] < 0)) return false;
    if (range[0] < 0) range[0] = 0;
    if (range[1] >= max) range[1] = max - 1;
  } else {
    range[0] = static_cast<int>(std::floor(p)) - 1;
    range[1] = static_cast<int>(std::floor(p - v));
    if ((range[0] < 0) || (range[1] >= max)) return false;
    if (range[0] >= max) range[0] = max - 1;
    if (range[1] < 0) range[1] = 0;
  }
  return true;
}
}  // namespace

dvec2 BlockGrid::ClipMovingRect(dvec2 p, dvec2 s, dvec2 v) {
  const dvec2 rect_extent = p + s;

  int x_range[2];
  if (!GetCollidingGridRange(p.x, rect_extent.x, v.x, dims_.x, x_range)) {
    return v;
  }
  int y_range[2];
  if (!GetCollidingGridRange(p.y, rect_extent.y, v.y, dims_.y, y_range)) {
    return v;
  }

  int x_index = x_range[0];
  int y_index = y_range[0];
  int x_line = x_index + (v.x >= 0 ? 0 : 1);
  int y_line = y_index + (v.y >= 0 ? 0 : 1);
  int x_inc = v.x >= 0 ? 1 : -1;
  int y_inc = v.y >= 0 ? 1 : -1;

  // The vertical/horizontal components of the lines along the colliding
  // boundary of the rectangle
  double x_rect_front = p.x + (v.x <= 0 ? 0 : s.x);
  double y_rect_front = p.y + (v.y <= 0 ? 0 : s.y);

  const double o_x_rect_front = x_rect_front;
  const double o_y_rect_front = y_rect_front;

  // Precompute constants that, when added to the fronts, provide the lo to high
  // y/x bounds of the colliding vertical/horizontal rect segment when testing
  // the collision of an vert/horiz rect edge
  double x_front_b_y[2];
  if (v.y >= 0) {
    x_front_b_y[0] = -s.y;
    x_front_b_y[1] = 0;
  } else {
    x_front_b_y[0] = 0;
    x_front_b_y[1] = s.y;
  }

  double y_front_b_x[2];
  if (v.x >= 0) {
    y_front_b_x[0] = -s.x;
    y_front_b_x[1] = 0;
  } else {
    y_front_b_x[0] = 0;
    y_front_b_x[1] = s.x;
  }

  // Track (x and y) rectangle "fronts" and grid "lines." Along v, find the
  // in-order sequence of occasions that x/y fronts and lines meet, and then
  // check for blocks behind the grid lines that meet with the fronts. Depending
  // on what blocks are behind a line meeting a front, we have collided.
  for (;;) {
    const double x_line_p = x_line * l_;
    const double y_line_p = y_line * l_;

    const double rel_mag_x = (x_line_p - x_rect_front) / v.x;
    const double rel_mag_y = (y_line_p - y_rect_front) / v.y;

    if (rel_mag_x <= rel_mag_y) {
      const double y_shift = rel_mag_x * v.y;

      x_rect_front = x_line_p;
      y_rect_front += y_shift;

      int y_front[2];
      y_front[0] = static_cast<int>(std::floor(y_rect_front + x_front_b_y[0]));
      y_front[1] =
          static_cast<int>(std::ceil(y_rect_front + x_front_b_y[1])) - 1;

      for (int y = y_front[0]; y <= y_front[1]; ++y) {
        // The rect's vertical edge can only collide with "FULL" blocks
        if (GetBlock({x_index, y}) == BlockType::FULL) {
          // Return the difference between the moved rect fronts and the
          // originals
          return dvec2(x_rect_front - o_x_rect_front,
                       y_rect_front - o_y_rect_front);
        }
      }

      x_line += x_inc;
      x_index += x_inc;

      // If we "exceed" the range of block column indexes we can check...
      if (((x_range[1] - x_index) * x_inc) < 0) return v;
    } else {
      const double x_shift = rel_mag_y * v.x;

      x_rect_front += x_shift;
      y_rect_front = y_line_p;

      int x_front[2];
      x_front[0] = static_cast<int>(std::floor(x_rect_front + y_front_b_x[0]));
      x_front[1] =
          static_cast<int>(std::ceil(x_rect_front + y_front_b_x[1])) - 1;

      for (int x = x_front[0]; x <= x_front[1]; ++x) {
        const BlockType block = GetBlock({x, y_index});
        // The rect's horizontal edge can collide with "FULL" blocks, and
        // occasionally one way platforms.
        if ((block == BlockType::FULL) ||
            ((block == BlockType::ONE_WAY_UP) && (v.y > 0))) {
          // See above comment...
          return dvec2(x_rect_front - o_x_rect_front,
                       y_rect_front - o_y_rect_front);
        }
      }

      y_line += y_inc;
      y_index += y_inc;

      // If we "exceed" the range of block row indexes we can check...
      if (((y_range[1] - y_index) * y_inc) < 0) return v;
    }
  }
}
}  // namespace retro
}  // namespace physics
