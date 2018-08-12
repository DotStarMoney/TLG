#ifndef THREAD_SEMAPHORE_H_
#define THREAD_SEMAPHORE_H_

#include <atomic>
#include <condition_variable>
#include <shared_mutex>

#include "absl/base/thread_annotations.h"
#include "util/noncopyable.h"

namespace thread {

// A bog-standard semaphore. The C++ standards committee thinks we're too stupid
// to have one, so here we are...
class Semaphore : public util::NonCopyable {
 public:
  Semaphore(int32_t init_resource);

  void P();
  bool TryP();

  void V();

  void Drain();

 private:
  std::shared_mutex m_;
  std::condition_variable_any cv_ GUARDED_BY(m_);

  std::atomic_int32_t r_;
};
}  // namespace thread

#endif  // THREAD_SEMAPHORE_H_
