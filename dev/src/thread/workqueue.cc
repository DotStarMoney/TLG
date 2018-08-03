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
        for (;;) {
          buffer_elem_remain_.P();
          if (exit_) return;
          buffer_[slot_working_++ % buffer_.size()]();
          buffer_avail_.V();
        }
      })) {
  CHECK_GT(queue_length, 0);
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
}  // namespace thread
