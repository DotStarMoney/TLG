#ifndef UTIL_MAKE_CLEANUP_H_
#define UTIL_MAKE_CLEANUP_H_

#include <functional>

// A utility class that defers calling a function object until the owning 
// MakeCleanup object is destroyed. Great for making sure things get resolved
// like:
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
  // Used to run the clean-up function early and prevent it from running again
  // upon object destruction.
  void cleanup();
 private:
  bool has_cleaned_up_;
  const std::function<void(void)> cleanup_func_;
};

} // namespace util

#endif // UTIL_MAKE_CLEANUP_H_