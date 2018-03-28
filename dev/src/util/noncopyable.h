#ifndef UTIL_NONCOPYABLE_H_
#define UTIL_NONCOPYABLE_H_

// Utility class that, when inherited, disables copy and assignment for the
// inheritor.
//
// Use it like this:
//
// class PleaseDontCopyMe : public NonCopyable {
//   ...
// }
//
namespace util {

class NonCopyable {
 public:
  NonCopyable() = default;

  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
};

}  // namespace util

#endif  // UTIL_NONCOPYABLE_H_
