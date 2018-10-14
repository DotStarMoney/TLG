#include "geometry2.h"

#include <math.h>
#include <tuple>

#include "glm/geometric.hpp"
#include "glm/vec2.hpp"

using glm::dvec2;
using std::tuple;

namespace physics {
namespace geometry2 {

namespace {
// This is the original fudge factor and has no rigorous basis... sorry : /
constexpr double kDefaultEpsilon = std::numeric_limits<float>::epsilon();

double epsilon = kDefaultEpsilon;

bool CloseToZero(double x) { return std::fabs(x) <= epsilon; }

// Compute the magnitude of the cross product of two 2d vectors.
double Cross2Mag(const dvec2& a, const dvec2& b) {
  return a.x * b.y - a.y * b.x;
}
}  // namespace

void set_epsilon(double e) { epsilon = e; }

Line2::Line2() { Update({0, 0}, {0, 0}); }
Line2::Line2(dvec2 start, dvec2 end) { Update(start, end); }

void Line2::Update(dvec2 start, dvec2 end) {
  p_[0] = start;
  p_[1] = end;
  delta_ = end - start;
}

Ray2::Ray2() { Update({ 0, 0 }, { 0, 0 }); }
Ray2::Ray2(dvec2 start, dvec2 delta) { Update(start, delta); }

void Ray2::Update(dvec2 start, dvec2 delta) {
  p_ = start;
  delta_ = delta;
}

tuple<bool, dvec2> Operations2::Intersects(const Ray2& ray, const Line2& line) {
  return Intersects(line, ray);
}

tuple<bool, dvec2> Operations2::Intersects(const Line2& line, const Ray2& ray) {
  const double project = Cross2Mag(line.delta_, ray.delta());
  // Fail if parallel...
  if (CloseToZero(project)) return {false, dvec2(0.0, 0.0)};

  const double line_parameter =
      Cross2Mag(ray.start() - line.start(), ray.delta()) / project;
  // Fail if intersection point is not on line segment...
  if ((line_parameter < 0.0) || (line_parameter >= 1.0)) {
    return {false, dvec2(0.0, 0.0)};
  }
  const dvec2 intersect_point = line.start() + line.delta_ * line_parameter;
  // Fail if intersection point is behind ray...
  if (glm::dot((intersect_point - ray.start()), ray.delta()) <= 0) {
    return { false, dvec2(0.0, 0.0) };
  }
  return {true, intersect_point};
}

}  // namespace geometry2
}  // namespace physics
