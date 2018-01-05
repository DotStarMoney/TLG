#ifndef UTIL_TOSTRING_H_
#define UTIL_TOSTRING_H_

#include <string>
#include <string_view>
#include <type_traits>

#include "util/reflect.h"

// Provides AsString and ToString, where the former can be used to avoid
// creating new strings in some cases and ToString always creates a new string.
//
// If as string does not create a new string, it will return an empty string.
//
namespace util {
namespace internal {
template <class T>
inline const std::string_view AsString(const T& val, std::string* buffer) {
  *buffer = std::to_string(val);
  return "";
}
template <class T>
inline const std::string_view AsString(T* val, std::string* buffer) {
  *buffer = std::to_string(reinterpret_cast<long>(val));
  return "";
}
inline const std::string_view AsString(std::nullptr_t val,
    std::string* buffer) {
  return "0";
}
inline const std::string_view AsString(const std::string& val,
    std::string* buffer) {
  return val;
}
inline const std::string_view AsString(const std::string_view& val,
    std::string* buffer) {
  return val;
}
inline const std::string_view AsString(const char* val, std::string* buffer) {
  return val;
}

template <class T>
inline std::string ToString(const T& val) {
  return std::to_string(val);
}
template <class T>
inline std::string ToString(T* val) {
  return std::to_string(reinterpret_cast<long>(val));
}
inline std::string ToString(std::nullptr_t val) {
  return "0";
}
inline std::string ToString(const std::string& val) {
  return std::string(val);
}
inline std::string ToString(const std::string_view& val) {
  return std::string(val);
}
inline std::string ToString(const char* val) {
  return std::string(val);
}

template <class T>
struct IsStringablePrimitiveBase {
  static constexpr bool value = std::is_integral<T>::value || 
      std::is_floating_point<T>::value ||
      std::is_pointer<T>::value ||
      std::is_same<T, std::string>::value ||
      std::is_same<T, char*>::value ||
      std::is_same<T, std::string_view>::value;
};
template <class T>
struct IsStringablePrimitive {
  static constexpr bool value =
      IsStringablePrimitiveBase<std::decay<T>::type>::value;
};
} // namespace internal


template <class T>
inline const std::string_view AsString(T val, std::string* buffer) {
  if constexpr(reflect::HasToString<T>::value) {
    *buffer = val.ToString();
    return "";
  } else if constexpr(internal::IsStringablePrimitive<T>::value) {
    return internal::AsString(val, buffer);
  } else {
    return "??";
  }
}

template <class T>
inline std::string ToString(T val) { 
  if constexpr(reflect::HasToString<T>::value) {
    return val.ToString();
  } else if constexpr(internal::IsStringablePrimitive<T>::value) {
    return internal::ToString(val);
  } else {
    return "??";
  }
}

} // namespace util

#endif // UTIL_TOSTRING_H_