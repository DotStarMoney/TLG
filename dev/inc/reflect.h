#ifndef UTIL_REFLECT_H_
#define UTIL_REFLECT_H_

#include <type_traits>
#include <string>

namespace util {
namespace reflect {

template<typename T>
class HasToString {
 private:
  template<typename U, std::string (U::*)() const> struct SFINAE {};
  template<typename U> static int8_t Test(SFINAE<U, &U::ToString>*);
  template<typename U> static int32_t Test(...);
 public:
  static constexpr bool value = sizeof(Test<T>(0)) == sizeof(int8_t);
};

} // namespace reflect
} // namespace util

#endif // UTIL_REFLECT_H_