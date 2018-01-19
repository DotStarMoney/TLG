#include "base/static_type_assert.h"

#include <string>
#include <string_view>

#include "gtest/gtest.h"

// These point of these tests is to simply fail compilation if something is
// amiss in static_type_assert.

TEST(StaticTypeAssertTest, AssertIsIntegral) {
  // A representative (enough) set of integral types
  base::type_assert::AssertIsIntegral<
      char, unsigned char, short, unsigned short, int, unsigned int, long,
      unsigned long, long long, unsigned long long, int8_t, uint8_t, int16_t,
      uint16_t, int32_t, uint32_t, int64_t, uint64_t>();
}

TEST(StaticTypeAssertTest, AssertIsFloating) {
  base::type_assert::AssertIsFloating<float, double>();
}

TEST(StaticTypeAssertTest, AssertIsConvertibleTo) {
  base::type_assert::AssertIsConvertibleTo<std::string_view, std::string,
                                           const char*>();
  base::type_assert::AssertIsConvertibleTo<float, int, double>();
}
