#include "make_cleanup.h"

namespace util {

MakeCleanup::MakeCleanup(std::function<void(void)> cleanup_func) : 
    has_cleaned_up_(false), cleanup_func_(cleanup_func) {}

MakeCleanup::~MakeCleanup() {
  if (!has_cleaned_up_) cleanup_func_();
}

void MakeCleanup::cleanup() {
  has_cleaned_up_ = true;
  cleanup_func_();
}

} // namespace util