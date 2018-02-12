#ifndef THREAD_THREAD_POOL_H_
#define THREAD_THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>

#include "thread/thread_reservoir.h"
#include "util/loan.h"
#include "util/noncopyable.h"

namespace thread {

// A pool of threads from a ThreadReservoir. This makes the grouping of threads
// more convenient.
//
// This class is thread safe.
//
class ThreadPool : public util::NonCopyable {
  friend class ThreadReservoir;

 public:
  virtual ~ThreadPool();

  // Schedule an operation to be run by the underlying reservoir.
  void Schedule(std::function<void()> func);

  // Wait for all running threads in this pool to complete.
  void Join();

  int pending_count() const {
    return pending_count_.load();
  }

 private:
  ThreadPool(util::Loan<ThreadReservoir>&& reservoir);

  std::mutex m_;
  std::condition_variable cv_;
  util::Loan<ThreadReservoir> reservoir_;
  std::atomic<int> pending_count_;
};
}  // namespace thread

#endif  // THREAD_THREAD_POOL_H_
