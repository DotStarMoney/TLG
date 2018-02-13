#include "thread/thread_reservoir.h"

#include "glog/logging.h"

namespace thread {
ThreadReservoir::ThreadReservoir(int size)
    : terminate_(false), target_size_(size) {
  CHECK_GT(size, 0)
      << "Must construct thread reservoir with at least 1 worker.";
}

ThreadReservoir::~ThreadReservoir() {
  // First be sure that there are no outstanding thread pools...
  TerminateLoans();
  // Fly the terminate flag, and wake all threads up (all threads should be
  // asleep)
  {
    std::unique_lock<std::mutex> lock(m_);
    terminate_ = true;
  }
  cv_.notify_all();
  // Wait for everyone to catch up.
  for (auto& thread : threads_) {
    thread.join();
  }
}

void ThreadReservoir::Start() {
  CHECK_EQ(threads_.size(), 0) << "Start was already called.";
  while (threads_.size() < target_size_) {
    threads_.emplace_back(Dispatcher, this, threads_.size());
  }
}

std::unique_ptr<ThreadPool> ThreadReservoir::GetPool() {
  // Make a fresh thread pool with a loaned reference to us.
  return std::unique_ptr<ThreadPool>(
      new ThreadPool(MakeLoan<ThreadReservoir>()));
}

void ThreadReservoir::Resize(int size) {
  CHECK_GT(size, 0) << "Cannot resize a reservoir to less than 1 worker.";
  if (size > target_size_) {
    // We can blindly just add new threads since they aren't yet doing
    // anything and increasing the value of target_size_ won't effect the
    // outcome of worker termination checks (assuming Resize is externally
    // synchronized).
    while (target_size_++ < size) {
      threads_.emplace_back(Dispatcher, this, threads_.size());
    }
  } else if (size < target_size_) {
    // Tell threads whose id is above the new size cutoff to terminate.
    int threads_to_terminate = target_size_ - size;
    {
      std::unique_lock<std::mutex> lock(m_);
      target_size_ = size;
    }
    cv_.notify_all();
    while (--threads_to_terminate >= 0) {
      // For each thread we want to be rid of, wait for it to finish, then
      // remove it from the reservoir.
      threads_.back().join();
      threads_.pop_back();
    }
  }
}

void ThreadReservoir::Schedule(std::function<void()> func) {
  {
    std::unique_lock<std::mutex> lock(m_);
    task_queue_.emplace(func);
  }
  cv_.notify_one();
}

bool ThreadReservoir::TerminateThreadCriteria(int64_t id) const {
  // Should terminate if either the terminate_ flag is set, or our id is no
  // longer wanted in the thread pool as a result of a call to Resize.
  return terminate_ || (id > (target_size_ - 1));
}

void ThreadReservoir::Dispatcher(ThreadReservoir* reservoir, int64_t id) {
  for (;;) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(reservoir->m_);

      // Sleep while there's no work to do and no reason to terminate.
      reservoir->cv_.wait(lock, [reservoir, id]() {
        return reservoir->TerminateThreadCriteria(id) ||
               !reservoir->task_queue_.empty();
      });

      // If we're awake because we met termination criteria, terminate.
      if (reservoir->TerminateThreadCriteria(id)) return;

      task = reservoir->task_queue_.front();
      reservoir->task_queue_.pop();
    }
    task();
  }
}
}  // namespace thread
