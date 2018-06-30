#include "util/encoding.h"

#include <math.h>
#include <string>

#include "glog/logging.h"

namespace util {
namespace base64 {
namespace {
constexpr uint32_t kAsciiTable[] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
   0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
   0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
}  // namespace

size_t GetDecodedAllocationSize(size_t encoded_size) {
  return static_cast<size_t>(ceil(ceil(encoded_size * 0.75) / 4.0) * 4.0);
}

size_t Decode(std::string_view text, char* dest) {
  static_assert(sizeof(uintptr_t) >= sizeof(void*),
                "No suitable integer type for conversion from pointer type");
  CHECK_EQ(text.size() & 3, 0) << "Base64 text must be a multiple of 4 in size";
  if (text.empty()) return 0;

  const char* source = text.data();
  const char* end_ptr = source + static_cast<uintptr_t>(text.size());
  while (source < end_ptr) {
    char lookup1 = kAsciiTable[source[1]];
    char lookup2 = kAsciiTable[source[2]];

    uint32_t coded_bytes =
        ((kAsciiTable[source[0]] << 2) | (lookup1 >> 4)) |
        (((lookup1 & 0x0f) << 12) | ((lookup2 >> 2) << 8)) |
        (((lookup2 & 0x03) << 22) | (kAsciiTable[source[3]] << 16));
    
    *reinterpret_cast<uint32_t*>(dest) = coded_bytes;
    dest += 3;
    source += 4;
  }

  size_t output_size = (text.size() >> 2) * 3;
  if (*(end_ptr - 1) == '=') {
    if (*(end_ptr - 2) == '=') {
      return output_size - 2;
    }
    return output_size - 1;
  }
  return output_size;
}

}  // namespace base64
}  // namespace util
