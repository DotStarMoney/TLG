#ifndef BASE_ABORT_H_
#define BASE_ABORT_H_

#include <cstdlib>
#include <string_view>
#include <iostream>

namespace base {

inline void abort(const std::string_view& msg) {
  std::cerr << "Aborted: " << msg << std::endl;
  std::abort();
}

} // namespace base

#endif // BASE_ABORT_H_
