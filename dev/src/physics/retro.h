#ifndef PHYSICS_RETRO_H_
#define PHYSICS_RETRO_H_

#include <vector>

#include <glm/vec2.hpp>

namespace physics {
namespace retro {

class BlockGrid {
 public:
  BlockGrid(glm::ivec2 dims, double l);
  enum class BlockType {
    NONE,
    // Solid square
    FULL,
    // Only is solid if contacted from the top edge (moved into with positive y)
    ONE_WAY_UP
  };

  void PutBlock(glm::ivec2 p, BlockType block);
  BlockType GetBlock(glm::ivec2 p);

  // Collides a rectangle at position p of size s moving over delta v, with the
  // block grid positioned s.t. the upper left corner is at 0,0. Returns the
  // clipped v. If the rectangle and grid are interpenetrating, the result is
  // undefined.
  glm::dvec2 ClipMovingRect(glm::dvec2 p, glm::dvec2 s, glm::dvec2 v);

 private:
  std::vector<BlockType> blocks_;
  const double l_;
  const glm::ivec2 dims_;
};

}  // namespace retro
}  // namespace physics

#endif  // PHYSICS_RETRO_H_
