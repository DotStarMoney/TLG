#ifndef BASE_STATIC_TYPE_ASSERT_H_
#define BASE_STATIC_TYPE_ASSERT_H_

#include <type_traits>

namespace base {
namespace internal {

template<typename... Conds>
struct And_ : std::true_type {};

template<typename Cond, typename... Conds>
struct And_<Cond, Conds...> : std::conditional<Cond::value, And_<Conds...>,
    std::false_type>::type {};

} // namespace internal

// Macro for generating variadic template type assertions, e.g.:
//
// base::IsIntegral<T0, T1, T2, ... >();
//
#define __STATIC_TYPE_ASSERT_VAR_TYPE_ASSERT(COND_, FUNC_, MSG_) \
namespace internal { \
template <class... Args> \
using FUNC_##_and = And_<COND_##<Args>...>; \
} /* namespace internal */ \
template <class... Args> \
void FUNC_##() { \
  static_assert(internal::##FUNC_##_and<Args...>::value, MSG_); \
}

// Usage:
// IsIntegral<TemplateParameters...>()
__STATIC_TYPE_ASSERT_VAR_TYPE_ASSERT(std::is_integral, IsIntegral,
    "Integral type(s) expected.");
// Usage:
// IsFloating<TemplateParameters...>()
__STATIC_TYPE_ASSERT_VAR_TYPE_ASSERT(std::is_floating_point,
    IsFloating, "Floating point type(s) expected.");

#undef __STATIC_TYPE_ASSERT_VAR_TYPE_ASSERT

// Usage:
// IsConvertibleTo<Type, TemplateParameters...>() 
namespace internal { 
template <class T, class... Args> 
using IsConvertibleTo_and = And_<std::is_convertible<Args, T>...>;
} // namespace internal
template <class T, class... Args> 
void IsConvertibleTo() { 
  static_assert(internal::IsConvertibleTo_and<T, Args...>::value, "Cannot "
      "convert argument(s) to type.");
}
} // namespace base

#endif // BASE_STATIC_TYPE_ASSERT_H_
