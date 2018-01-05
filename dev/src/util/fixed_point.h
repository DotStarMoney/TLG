#ifndef UTIL_FIXED_POINT_H_
#define UTIL_FIXED_POINT_H_

#include <cstdint>

namespace util {
// Fixed point integer types used for more consistently describing values to be
// treated as "fixed point" values. The format is:
//
// fixAA_BB
//
// Where AA represents the number of bits dedicated to the integer part of the
// value and BB represents the number of bits dedicated to the fractional part
// of the value.

// 32 bit types
typedef int32_t fix16_16;

// 16 bit types
typedef int16_t fix8_8;
typedef uint16_t ufix_16;
} // namespace util

#endif // UTIL_FIXED_POINT_H_