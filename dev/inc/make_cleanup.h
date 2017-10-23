#ifndef UTIL_MAKE_CLEANUP_H_
#define UTIL_MAKE_CLEANUP_H_

#include <functional>

// A utility class that defers a function object until the owning MakeCleanup
// object is destroyed. Great for making sure things get resolved like:
//
// 
// int* int_array = new int[4];
// util::MakeCleanup delete_int_array([int_array]() { delete[] int_array; );
// ...
// /* some error handling code that exits early */
// ...
//
//
namespace util {

class MakeCleanup {
 public:
  explicit MakeCleanup(std::function<void(void)> cleanup_func);
  ~MakeCleanup();
 private:
  const std::function<void(void)> cleanup_func_;
};

} // namespace util

#endif // UTIL_MAKE_CLEANUP_H_