#ifndef THREAD_THREAD_RESERVOIR_H_
#define THREAD_THREAD_RESERVOIR_H_

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "thread/thread_pool.h"
#include "util/loan.h"

namespace thread {

// A reservoir of threads made accessible by ThreadPool.
//
class ThreadReservoir : public util::Lender {
  friend class ThreadPool;

 public:
  // Must be at least 1.
  ThreadReservoir(int size = 1);
  virtual ~ThreadReservoir();

  // Not thread safe.
  void Start();

  // Resize the number of threads in the pool. If reducing the pool size, this
  // method should be run when you know all running threads in this reservoir
  // are sleeping or will be soon. It will block until some arbitrary subset of
  // the threads in the pool (the ones that will be removed) have completed
  // their work.
  //
  // Not thread safe.
  void Resize(int size);

  // Make a thread pool that references this reservoir.
  std::unique_ptr<ThreadPool> GetPool();

 private:
  static void Dispatcher(ThreadReservoir* reservoir, int64_t id);

  // True if a thread with the given id should terminate.
  bool TerminateThreadCriteria(int64_t id) const;

  void Schedule(std::function<void()> func);

  std::mutex m_;
  std::condition_variable cv_;
  std::queue<std::function<void()>> task_queue_;  // Guarded by m_
  std::vector<std::thread> threads_;
  bool terminate_;   // Guarded by m_
  int target_size_;  // Part of non-thread safe api, so no need to guard
};
}  // namespace thread

#endif  // THREAD_THREAD_RESERVOIR_H_
