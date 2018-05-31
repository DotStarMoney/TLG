#include "base/fixed32.h"

#include "gtest/gtest.h"

using base::Fixed32;

TEST(Fixed32Test, Construction) {
  Fixed32<8> x_int(10);
  ASSERT_EQ(x_int.raw(), 2560);

  Fixed32<7> x_float(10.125f);
  ASSERT_EQ(x_float.raw(), 1296);
}

TEST(Fixed32Test, Assignment) {
  Fixed32<8> x_int;
  x_int = 10;
  ASSERT_EQ(x_int.raw(), 2560);

  Fixed32<7> x_float;
  x_float = 10.125f;
  ASSERT_EQ(x_float.raw(), 1296);
}

TEST(Fixed32Test, Casting) {
  Fixed32<6> x_int;
  x_int = 63;
  int y_int = x_int;
  ASSERT_EQ(y_int, 63);

  Fixed32<6> x_double;
  x_double = 88.5;
  double y_double = x_double;
  ASSERT_EQ(y_double, 88.5);
}

TEST(Fixed32Test, PrimitiveOps) {
  Fixed32<5> x;
  Fixed32<5> y;

  x = 8.125;
  y = 11.5;

  ASSERT_EQ(x + y, 19.625);
  ASSERT_EQ(x - y, -3.375);

  x = 81.25;
  y = 115;
  ASSERT_EQ(x * y, 9343.75);

  x = -1;
  y = 2.5;
  ASSERT_EQ(x / y, -0.4);
}

TEST(Fixed32Test, SaneLimit) { ASSERT_EQ(Fixed32<8>(-0.00390625).raw(), -1); }

TEST(Fixed32Test, CompareOps) {
  Fixed32<8> x;
  Fixed32<8> y;

  x = 0.00390625;
  y = -800;

  ASSERT_TRUE(x != y);
  ASSERT_FALSE(x == y);
  ASSERT_TRUE(x > y);
  ASSERT_TRUE(x >= y);
  ASSERT_FALSE(x < y);
  ASSERT_FALSE(x <= y);
}
