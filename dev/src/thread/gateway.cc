#include "thread/gateway.h"

#include <atomic>
#include <condition_variable>
#include <shared_mutex>

#include "glog/logging.h"

namespace thread {
Gateway::Gateway() : blocking_(true), guarded_blocking_(true) {}

void Gateway::Enter() {
  if (blocking_.load(std::memory_order_relaxed)) {
    std::shared_lock<std::shared_mutex> s_lock(m_);
    if (guarded_blocking_) cv_.wait(m_);
  }
}

void Gateway::Unlock() {
  if (blocking_.exchange(false, std::memory_order_relaxed)) {
    std::unique_lock<std::shared_mutex> lock(m_);
    guarded_blocking_ = false;
    cv_.notify_all();
  }
}

}  // namespace thread
