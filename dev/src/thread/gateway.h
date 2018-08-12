#ifndef THREAD_GATEWAY_H_
#define THREAD_GATEWAY_H_

#include <atomic>
#include <condition_variable>
#include <shared_mutex>

#include "absl/base/thread_annotations.h"
#include "util/noncopyable.h"

namespace thread {

// A synchronization gateway. Threads will block on Enter(), only to be released
// when a thread calls Unlock(). Further calls to Enter() after Unlock() will
// not block. Multiple calls to Unlock() are idempotent.
class Gateway : public util::NonCopyable {
 public:
  Gateway();

  void Enter();

  void Unlock();

 private:
  std::atomic_bool blocking_;
  
  // Used to resolve issues where threads may soon block on the gateway while 
  // a different thread releases the gateway and those that would soon block are
  // technically released.
  bool guarded_blocking_ GUARDED_BY(m_);

  std::shared_mutex m_;
  std::condition_variable_any cv_ GUARDED_BY(m_);
};
}  // namespace thread

#endif  // THREAD_GATEWAY_H_
