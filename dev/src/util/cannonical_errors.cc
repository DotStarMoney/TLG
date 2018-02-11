#include "util/cannonical_errors.h"

#include <string_view>

namespace util {
namespace error {

// NOTE: MUST be kept in sync with enum in cannonical_errors.h
const absl::string_view CannonicalErrorString[] = {
    "Unknown",          "Failed precondition",
    "Invalid argument", "Timeout",
    "Out of memory",    "Out of bounds",
    "Logic error",      "Resource unobtainable",
    "Unimplemented",    "Format mismatch",
    "IO error"};

}  // namespace error
}  // namespace util
