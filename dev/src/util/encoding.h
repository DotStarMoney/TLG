#ifndef UTIL_ENCODING_H_
#define UTIL_ENCODING_H_

#include <stdint.h>
#include <string_view>

// Utilities for encoding/decoding bytes stored as text

namespace util {
namespace base64 {

// Given a base64 encoded string, return the allocation size in bytes for the
// char array passed to Decode.
size_t GetDecodedAllocationSize(size_t encoded_size);

// Decode base64 text to a destination byte array. Returns the number of bytes
// resolved from the encoded text. This is based on the MIME base64 encoding.
size_t Decode(std::string_view text, char* dest);

}  // namespace base64
}  // namespace util
#endif  // UTIL_ENCODING_H_
