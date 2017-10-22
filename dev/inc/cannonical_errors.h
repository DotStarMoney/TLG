#ifndef UTIL_CANNONICAL_ERRORS_H_
#define UTIL_CANNONICAL_ERRORS_H_

#include <string_view>

namespace util {
namespace error {

// The convention is that all error codes are of type int.
enum CannonicalErrors : int {
  // This indicates a failure of error handling code and should not be used 
  // explicitly. 
  UNKNOWN = 0,
  // Something needed to be true before a function was called, and it was not.
  FAILED_PRECONDITION = 1,
  // The argument(s) passed into a function were invalid for this invocation.
  INVALID_ARGUMENT = 2,
  // While waiting for something, we ran out of time.
  TIMEOUT = 3,
  // A more specific "RESOURCE_UNOBTAINABLE:" we were unable to obtain memory.
  OUT_OF_MEMORY = 4,
  // We tried to access something which which does not exist.
  OUT_OF_BOUNDS = 5,
  // An error occured that could have resulted from incorrect code logic
  // (it may have been detectable by inspection).
  LOGIC_ERROR = 6,
  // A resource request made to the OS failed.
  RESOURCE_UNOBTAINABLE = 7,
  // A valid code path is not yet implemented.
  UNIMPLEMENTED_ERROR = 8,
  // Some sort of structure that purported to follow a specific format did not.
  FORMAT_MISTMATCH = 9,
  // A generic error when an io operation failed.
  IO_ERROR = 10
};

// The cannonical errors but as strings
extern const std::string_view CannonicalErrorString[];

} // namespace error
} // namespace util

#endif // UTIL_CANNONICAL_ERRORS_H_