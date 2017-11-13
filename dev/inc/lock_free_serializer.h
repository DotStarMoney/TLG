#ifndef UTIL_LOCK_FREE_SERIALIZER_H_
#define UTIL_LOCK_FREE_SERIALIZER_H_

#include <atomic>

#include "status.h"
#include "strcat.h"
#include "status_macros.h"

//
// A lock-free container for the access pattern where many threads may queue 
// elements, then a single thread drains the queue one element at a time FIFO.
//
// Synchronization is ensured 
//
namespace util {

template <class T>
class lock_free_serializer {
 public:
  explicit lock_free_serializer(uint32_t capacity) : capacity_(capacity),
      buffer_(malloc(sizeof(T) * capacity_ * 2)), size_(0), sentry_(0) {}

  template <class ForwardT>
  util::Status queue(ForwardT&& element) {
    if (sentry_.fetch_add(1) & kSentryDrainBit) {
      // Attempting to queue while draining is not allowed, though many threads
      // can queue simultaneously
      ASSERT(false);
    }
    
    // Atomicly get the value of size, then increment it
    const uint32_t index = size_.fetch_add(1);
    if (index > capacity_) {
      return util::OutOfBoundsError(
          util::StrCat("Outside queue range ( > ", capacity_, ").");
    }
    buffer_[index] = std::forward<ForwardT>(element);

    --sentry_;
  }

  util::Status drain(std::is_function<util::Status(const T& element)> f) {
    const int buffer_index = -buffer_flag_.fetch_xor(-1);

    // We can load/store size_ non-atomically here and below as we know other
    // threads can't be messing with it right now
    for (uint32_t index = 0; index < size_.load(std::memory_order_relaxed); 
        ++index) {
      RETURN_IF_ERROR(f(buffer_[index]));
    }
    size_.store(0, std::memory_order_relaxed);

  }

 private:
  static const int32_t kSentryDrainBit = 1 << 31;

  const uint32_t capacity_;  
  T* buffer_;
  std::atomic_uint32_t size_;
  std::atomic_int32_t buffer_flag_;

};

} // namespace util

#endif // UTIL_LOCK_FREE_SERIALIZER_H_