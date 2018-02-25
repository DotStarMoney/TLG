#include "thread/thread_pool.h"

#include "glog/logging.h"

namespace thread {
ThreadPool::ThreadPool(util::Loan<ThreadReservoir>&& reservoir)
    : reservoir_(std::move(reservoir)), pending_count_(0) {}

ThreadPool::~ThreadPool() {
  CHECK_EQ(pending_count_.load(), 0)
      << "Attempting to destruct a thread pool with active threads.";
}

void ThreadPool::Join() {
  std::unique_lock<std::mutex> lock(m_);
  cv_.wait(lock, [this]() { return pending_count_.load() == 0; });
}

void ThreadPool::Schedule(std::function<void()> func) {
  ++pending_count_;
  // We wrap the incoming function in another function that decrements the
  // active thread count and notifies any sleeping Joins.
  reservoir_->Schedule([this, func]() {
    func();
    --pending_count_;
    cv_.notify_all();
  });
}
}  // namespace thread