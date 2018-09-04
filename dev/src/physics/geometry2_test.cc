#include "physics/geometry2.h"

#include "glm/vec2.hpp"
#include "gtest/gtest.h"

using glm::dvec2;

namespace physics {
namespace geometry2 {

namespace {
void testIntersects(const Line2& line, const Ray2& ray, const dvec2& point) {
  auto [result, location] = Operations2::Intersects(line, ray);
  EXPECT_TRUE(result);
  EXPECT_DOUBLE_EQ(location.x, point.x);
  EXPECT_DOUBLE_EQ(location.y, point.y);
}

void testDoesNotIntersect(const Line2& line, const Ray2& ray) {
  auto [result, ignored] = Operations2::Intersects(line, ray);
  EXPECT_FALSE(result);
}
}  // namespace

TEST(Geometry2Test, intersectsLineAndRay_returnsTrue_whenIntersection) {
  // Line (-1, 0) -> (1, 0)
  // Ray (0, -5) >> (0, 1)
  // Intersect at (0, 0)
  testIntersects({{-1, 0}, {1, 0}}, {{0, -5}, {0, 1}}, {0, 0});
  // Line (-3, -3) -> (3, 3)
  // Ray (1, 0) >> (-1, 1)
  // Intersect at (0.5, 0.5)
  testIntersects({{-3, -3}, {3, 3}}, {{1, 0}, {-1, 1}}, {0.5, 0.5});
  // Line (0, 0) -> (-2, -2)
  // Ray (-1, 1) >> (0, -1)
  // Intersect at (-1, -1)
  testIntersects({{0, 0}, {-2, -2}}, {{-1, 1}, {0, -1}}, {-1, -1});
}

TEST(Geometry2Test, intersectsLineAndRay_returnsFalse_whenNoIntersection) {
  // Line (-1, 0) -> (1, 0)
  // Ray (0, -5) >> (1, 1000)
  testDoesNotIntersect({{-1, 0}, {1, 0}}, {{0, -5}, {1000, 1}});
  // Line (-3, -3) -> (3, 3)
  // Ray (1, 0) >> (1, 1)
  testDoesNotIntersect({{-3, -3}, {3, 3}}, {{1, 0}, {1, 1}});
  // Line (0, 0) -> (-0.5, -0.5)
  // Ray (-1, 1) >> (0, -1)
  testDoesNotIntersect({{0, 0}, {-0.5, -0.5}}, {{-1, 1}, {0, -1}});
}

}  // namespace geometry2
}  // namespace physics
