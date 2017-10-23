#include "make_cleanup.h"

namespace util {

MakeCleanup::MakeCleanup(std::function<void(void)> cleanup_func) : 
    cleanup_func_(cleanup_func) {}

MakeCleanup::~MakeCleanup() {
  cleanup_func_();
}

} // namespace util