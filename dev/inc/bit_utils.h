#ifndef UTIL_BIT_UTILS_H_
#define UTIL_BIT_UTILS_H_

#include "static_type_assert.h"

namespace util {

template <class T>
inline T AllOnes() { return ~static_cast<T>(0); }

// Unused bits of value can be garbage.
template <class T, class V>
inline T SignExtend(V value, int bits) {
  type_assert::IsIntegral<T, V>();
  T mask = AllOnes<T>() << bits;
  T high_bit = ((value >> (bits - 1)) & 1);
  return (value & ~mask) | (mask * high_bit);
}

} // namespace util

#endif // UTIL_BIT_UTILS_H_