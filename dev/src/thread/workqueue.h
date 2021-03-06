#ifndef THREAD_WORKQUEUE_H_
#define THREAD_WORKQUEUE_H_

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "thread/gateway.h"
#include "thread/semaphore.h"
#include "util/noncopyable.h"

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
  WorkQueue(uint32_t queue_length = 1);

  ~WorkQueue();

  // Adds work to the queue. This will block while the queue is full.
  void AddWork(std::function<void(void)> f);

  // Adds work to the queue. Returns false iff the queue is full.
  bool TryAddWork(std::function<void(void)> f);

  std::thread::id GetWorkerThreadId() const;

 private:
  void AddWorkInternal(std::function<void(void)> f);

  Semaphore buffer_avail_;
  Semaphore buffer_elem_remain_;
  std::atomic_bool exit_;

  std::atomic_uint32_t slot_;
  uint32_t slot_working_;
  
  struct WorkElement {
    WorkElement() : ready(false) {}
    std::function<void(void)> work;
    std::atomic<bool> ready;
  };
  std::vector<WorkElement> buffer_;

  mutable Gateway worker_id_gate_;
  std::thread::id worker_id_;

  std::unique_ptr<std::thread> worker_;
};
}  // namespace thread

#endif  // THREAD_WORKQUEUE_H_
