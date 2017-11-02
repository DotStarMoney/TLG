#ifndef UTIL_STRCAT_H_
#define UTIL_STRCAT_H_

#include <string>
#include "tostring.h"

namespace util {
namespace internal {

template <class T>
void StrCat_recursive(std::string* base, T first) {
  std::string buffer;
  std::string_view src_string = util::AsString(first, &buffer);
  if (buffer.empty()) {
    base->append(src_string);
  } else {
    base->append(buffer);
  }
}
template <class T, class... ARGS>
void StrCat_recursive(std::string* base, T first, ARGS... args) {
  std::string buffer;
  std::string_view src_string = util::AsString(first, &buffer);
  if (buffer.empty()) {
    base->append(src_string);
  } else {
    base->append(buffer);
  }
  StrCat_recursive(base, args...);
}

} // namespace internal

template <class T0>
std::string StrCat(T0 p0) {
  return util::ToString(s0);
}
template <class T0, class T1>
std::string StrCat(T0 p0, T1 p1) {
  std::string concat = util::ToString(p0);
  std::string buffer;
  std::string_view src_string = AsString(p1, &buffer);
  if (buffer.empty()) {
    concat.reserve(concat.size() + src_string.length());
    return concat.append(src_string);
  } else {
    concat.reserve(concat.size() + buffer.length());
    return concat.append(buffer);
  }
}
template <class T, class... ARGS>
std::string StrCat(T first, ARGS... args) {
  std::string concat = util::ToString(first);
  internal::StrCat_recursive(&concat, args...);
  return concat;
}

} // namespace util

#endif // UTIL_STRCAT_H_