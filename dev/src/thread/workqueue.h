#ifndef THREAD_WORKQUEUE_H_
#define THREAD_WORKQUEUE_H_

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "thread/semaphore.h"
#include "util/noncopyable.h"

// note that any work scheduled cannot require resources that the queue
// will outlive

namespace thread {

// A multi-producer, single-consumer work queue. Uses RAII to start the worker
// thread. Producers will block when calling AddWork while the queue is full.
// All references in added work must outlive the work itself. The queue will
// block during destruction until all work has completed.
//
// All methods are thread safe.
class WorkQueue : public util::NonCopyable {
 public:
  // Starts the worker with the given queue length.
  WorkQueue(uint32_t queue_length);

  ~WorkQueue();

  // Adds work to the queue. This will block while the queue is full.
  void AddWork(std::function<void(void)> f);

 private:
  Semaphore buffer_avail_;
  Semaphore buffer_elem_remain_;
  std::atomic_bool exit_;

  std::atomic_uint32_t slot_;
  uint32_t slot_working_;

  std::vector<std::function<void(void)>> buffer_;
  std::unique_ptr<std::thread> worker_;
};

}  // namespace thread

#endif  // THREAD_WORKQUEUE_H_
