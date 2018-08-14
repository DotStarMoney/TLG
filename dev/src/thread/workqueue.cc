#include "thread/workqueue.h"

#include <functional>
#include <thread>

#include "glog/logging.h"

namespace thread {
WorkQueue::WorkQueue(uint32_t queue_length)
    : buffer_avail_(queue_length),
      buffer_elem_remain_(0),
      exit_(false),
      slot_(0),
      slot_working_(0),
      buffer_(queue_length),
      worker_(new std::thread([this] {
        worker_id_ = std::this_thread::get_id();
        worker_id_gate_.Unlock();
        for (;;) {
          buffer_elem_remain_.P();
          if (exit_) return;
          WorkElement& work_element = buffer_[slot_working_++ % buffer_.size()];
          while (!work_element.ready.load(std::memory_order_relaxed)) {
          }
          work_element.work();
          work_element.ready.exchange(false, std::memory_order_release);
          buffer_avail_.V();
        }
      })) {
  CHECK_GT(queue_length, 0u);
}

WorkQueue::~WorkQueue() {
  exit_ = true;
  buffer_elem_remain_.Drain();
  worker_->join();
}

void WorkQueue::AddWork(std::function<void(void)> f) {
  buffer_avail_.P();
  AddWorkInternal(f);
}

bool WorkQueue::TryAddWork(std::function<void(void)> f) {
  if (!buffer_avail_.TryP()) return false;
  AddWorkInternal(f);
  return true;
}

void WorkQueue::AddWorkInternal(std::function<void(void)> f) {
  WorkElement& work_element =
      buffer_[slot_.fetch_add(1, std::memory_order_relaxed) % buffer_.size()];
  work_element.work = f;
  work_element.ready.exchange(true, std::memory_order_acquire);
  buffer_elem_remain_.V();
}

std::thread::id WorkQueue::GetWorkerThreadId() const {
  worker_id_gate_.Enter();
  return worker_id_;
}
}  // namespace thread
