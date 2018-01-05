#ifndef THREAD_THREAD_POOL_H_
#define THREAD_THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>

#include "util/loan.h"
#include "thread/thread_reservoir.h"

namespace thread {

// A pool of threads from a ThreadReservoir. This makes the grouping of threads
// more convenient. 
// 
// This class is thread safe.
//
class ThreadReservoir;
class ThreadPool {
 public:
  virtual ~ThreadPool();

  // Schedule an operation to be run by the underlying reservoir.
  void Schedule(std::function<void()> func);

  // Wait for all running threads in this pool to complete.
  void Join();

  // ThreadPool is neither copyable nor assignable.
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

 private:
  ThreadPool(Loan<ThreadReservoir>&& reservoir);

  std::mutex m_;
  std::condition_variable cv_;
  util::Loan<ThreadReservoir> reservoir_;
  std::atomic<int> active_count_;
};
}  // namespace thread

#endif  // THREAD_THREAD_POOL_H_

