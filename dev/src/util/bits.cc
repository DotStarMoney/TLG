#include "util/bits.h"

namespace util {
namespace varint {

uint16_t GetVarintAndInc(const uint8_t** stream) {
  const uint8_t*& stream_data = *stream;

  if (*stream_data & 0x80) {
    const uint8_t lo_bits = *(stream_data++) & 0x7f;
    return lo_bits | static_cast<uint16_t>(*(stream_data++)) << 7;
  }
  return *(stream_data++);
}

} // namespace varint
} // namespace util