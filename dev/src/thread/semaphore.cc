#include "thread/semaphore.h"

#include <atomic>
#include <condition_variable>
#include <limits>
#include <shared_mutex>

#include "glog/logging.h"

namespace thread {
Semaphore::Semaphore(int32_t init_resource) : r_(init_resource) {
  CHECK_GE(init_resource, 0);
}

// Using an atomic resource count here allows us to avoid locking in the common
// case. Multiple threads can acquire resource simultaneously lock free. The
// only lock contention will occur when releasing resource and threads are
// waiting.
//
// If there is plenty of resource to go around however, no locking will take
// place.

void Semaphore::P() {
  std::shared_lock<std::shared_mutex> s_lock(m_);
  if (r_.fetch_add(-1, std::memory_order_relaxed) <= 0) cv_.wait(s_lock);
}

bool Semaphore::TryP() {
  if (r_.fetch_add(-1, std::memory_order_relaxed) <= 0) {
    std::unique_lock<std::shared_mutex> lock(m_);
    r_.fetch_add(1, std::memory_order_relaxed);
    cv_.notify_one();
    return false;
  }
  return true;
}

void Semaphore::V() {
  if (r_.fetch_add(1, std::memory_order_relaxed) < 0) {
    std::unique_lock<std::shared_mutex> lock(m_);
    cv_.notify_one();
  }
}

void Semaphore::Drain() {
  r_ = std::numeric_limits<int32_t>::max();
  cv_.notify_all();
}
}  // namespace thread
