#ifndef UTIL_BITS_H_
#define UTIL_BITS_H_

#include <cstdint>

#include "base/static_type_assert.h"

namespace util {

template <class T>
inline T AllOnes() {
  return ~static_cast<T>(0);
}

// Unused bits of value can be garbage.
template <class T, class V>
inline T SignExtend(V value, int bits) {
  base::type_assert::AssertIsIntegral<T, V>();
  T mask = AllOnes<T>() << bits;
  T high_bit = ((value >> (bits - 1)) & 1);
  return (value & ~mask) | (mask * high_bit);
}

// A varint is a 1-2 byte unsigned integer in [0, 32767]. Since its width is
// variable, it is not suitable for random access reading situations and should
// be read as part of a stream of bytes.
//
// **HOW TO READ A VARINT**
//
// Read a byte. If the high order bit of that byte is 1, than the value of the
// varint is the first 7 bits of the first byte with the bits of the next byte
// appended to them (little endian style). If the high order bit of the first
// byte is cleared, the value or the Varint is made up of the first 7 bits of
// the first byte only.
//
namespace varint {
// Given a stream pointing to a varint, return the value of the varint while
// advancing the stream pointer to the next value.
uint16_t GetVarintAndInc(const uint8_t** stream);
}  // namespace varint
}  // namespace util

#endif  // UTIL_BITS_H_
