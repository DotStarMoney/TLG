#ifndef PHYSICS_GEOMETRY2_H_
#define PHYSICS_GEOMETRY2_H_

#include <tuple>

#include <glm/vec2.hpp>

// Utilities for manipulating simple 2D geometric primitives.

namespace physics {
namespace geometry2 {

class Operations2;
class Line2 {
  friend class Operations2;

 public:
  Line2(glm::dvec2 start, glm::dvec2 end);

  void Update(glm::dvec2 start, glm::dvec2 end);

  const glm::dvec2& start() const { return p_[0]; }
  const glm::dvec2& end() const { return p_[1]; }

 private:
  glm::dvec2 delta_;
  glm::dvec2 p_[2];
};

class Ray2 {
  friend class Operations2;

 public:
  Ray2(glm::dvec2 start, glm::dvec2 delta);

  void Update(glm::dvec2 start, glm::dvec2 delta);

  const glm::dvec2& start() const { return p_; }
  const glm::dvec2& delta() const { return delta_; }

 private:
  glm::dvec2 delta_;
  glm::dvec2 p_;
};

// Set the tolerance used when checking the "zero-ness" of a scalar when
// manipulating geometries. We'll consider X == 0 if fabs(x) <= e. This has
// all sorts of issues when X grows large, but should be effectively what we
// want for now (9/3/2018).
void set_epsilon(double e);

class Operations2 {
 public:
  static std::tuple<bool, glm::dvec2> Intersects(const Line2& line,
                                                 const Ray2& ray);
  static std::tuple<bool, glm::dvec2> Intersects(const Ray2& ray,
                                                 const Line2& line);
};

}  // namespace geometry2
}  // namespace physics

#endif  // PHYSICS_GEOMETRY2_H_
