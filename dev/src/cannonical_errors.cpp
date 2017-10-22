#include "cannonical_errors.h"

#include <string_view>

namespace util {
namespace error {

// NOTE: MUST be kept in sync with enum in cannonical_errors.h
const std::string_view CannonicalErrorString[] = {
  "Unknown",
  "Failed precondition",
  "Invalid argument",
  "Timeout",
  "Out of memory",
  "Out of bounds",
  "Logic error",
  "Resource unobtainable",
  "Unimplemented",
  "Format mismatch"
};

} // namespace error
} // namespace util