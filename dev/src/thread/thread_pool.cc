#include "thread/thread_pool.h"

#include "base/assert.h"

namespace util {
ThreadPool::ThreadPool(Loan<ThreadReservoir>&& reservoir)
    : reservoir_(std::move(reservoir)), active_count_(0) {}

ThreadPool::~ThreadPool() { 
  ASSERT_EQ(active_count_.load(), 0);
}

void ThreadPool::Join() {
  std::unique_lock<std::mutex> lock(m_);
  cv_.wait(lock, [this]() { return active_count_.load() == 0; });
}

void ThreadPool::Schedule(std::function<void()> func) {
  ++active_count_;
  // We wrap the incoming function in another function that decrements the
  // active thread count and notifies any sleeping Joins.
  reservoir_->Schedule([this, func]() {
    func();
    --active_count_;
    cv_.notify_all();
  });
}
}  // namespace util
