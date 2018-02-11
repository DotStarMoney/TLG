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
 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;

  NonCopyable(NonCopyable const&) = delete;
  void operator=(NonCopyable const&) = delete;
};

}  // namespace util

#endif  // UTIL_NONCOPYABLE_H_
