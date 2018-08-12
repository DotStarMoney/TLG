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
          buffer_[slot_working_++ % buffer_.size()]();
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
  buffer_[slot_++ % buffer_.size()] = f;
  buffer_elem_remain_.V();
}

bool WorkQueue::TryAddWork(std::function<void(void)> f) {
  if (!buffer_avail_.TryP()) return false;
  buffer_[slot_++ % buffer_.size()] = f;
  buffer_elem_remain_.V();
  return true;
}

std::thread::id WorkQueue::GetWorkerThreadId() const {
  worker_id_gate_.Enter();
  return worker_id_;
}
}  // namespace thread
