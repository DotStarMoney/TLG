#ifndef UTIL_BATCHENFORCER_H_
#define UTIL_BATCHENFORCER_H_

#include <atomic>

#include "assert.h"
#include "status.h"

namespace util {

class BatchEnforcer {
 public:
  BatchEnforcer() : sentry_(0) {}

  template <class F>
  util::Status ModSafe(F f) {
    if (sentry_.fetch_add(1) & kHighBit) {
      return util::FailedPreconditionError(
          "Cannot modify while batch processing.");
    }
    f();
    --sentry_;
  }

  template <class F>
  util::Status BatchSafe(F f) {
    if (sentry_.fetch_xor(-1) != -1) {
      return util::FailedPreconditionError(
          "Cannot modify while batch processing.");

    }
  }

  template <class F>
  void Mod(F f) {

  }

  template <class F>
  void Batch(F f) {

  }

 private:
  static const int64_t kHighBit = 1 << 63;

  std::atomic_int64_t sentry_;
};


} // namespace util

#endif // UTIL_BATCHENFORCER_H_