#ifndef THREAD_AFFINITIZINGSCHEDULER_H_
#define THREAD_AFFINITIZINGSCHEDULER_H_

#include <atomic>
#include <mutex>
#include <vector>

#include "thread/workqueue.h"
#include "util/noncopyable.h"

namespace thread {

// A scheduler that tries to load balance scheduled work over a number of work
// queues. AffinitizingScheduler also guarantees that any work scheduled with a
// single token will be serialized between calls to Sync.
//
// The usage pattern is to make calls to Schedule(...) in thread X, Join() in
// thread X, then call Sync() in thread X to update the load balancing state.
//
// For expected Join() behavior, all work should be scheduled from the thread
// in which Join() will be called, or more commonly from work scheduled via
// the AffinitizingScheduler (in general, as long as all work is scheduled
// via the same AffinitizingScheduler, Join() will block until all work is
// finished).
class AffinitizingScheduler : public util::NonCopyable {
 public:
  // This does not take ownership of these queues.
  AffinitizingScheduler(const std::vector<WorkQueue*>& queues);

  class Token {
    friend class AffinitizingScheduler;

   public:
    Token(const Token& token)
        : id_(token.id_),
          consumes_(token.consumes_),
          last_cycle_consumed_(token.last_cycle_consumed_) {
      last_active_cycle_.store(token.last_active_cycle_,
                               std::memory_order_relaxed);
    }

   private:
    Token(int32_t id)
        : id_(id),
          consumes_(0),
          last_cycle_consumed_(0),
          last_active_cycle_(-1) {}

    // An id_ used to pick the queue on which we'll schedule.
    int32_t id_;

    // A prediction of the amount of time work scheduled with this token takes
    // to run in seconds.
    double consumes_;

    // The amount of time consumed by work scheduled with this token since
    // Sync() was previously called in seconds.
    double last_cycle_consumed_;

    std::mutex m_;

    // The last value of AffinitizingScheduler::cycle_ with which work was run
    // using this token.
    std::atomic<int32_t> last_active_cycle_;
  };

  // Schedule work using a token. This overload allows the scheduler to balance
  // work across the work queues. The token _will_ be updated, and these updates
  // are thread safe. Work scheduled using the same token will be run entirely
  // on the same queue between calls to Sync(), and will therefore be
  // serialized.
  void Schedule(Token* token, std::function<void()> work);

  // Schedule work on a specific queue. This does not change the state of the
  // load balancing.
  void Schedule(uint32_t worker, std::function<void()> work);

  uint32_t size() const { return workers_.size(); }

  // Update the load balancing state. This should be called after a call to
  // Join().
  void Sync();

  // Block until all scheduled work has completed.
  void Join();

  // Get a list of durations in seconds that work was running on the work 
  // queues since the last call to Sync().
  std::vector<double> GetWorkingTime() const;

  // Get a token to use for load-balanced scheduling.
  static Token GetToken();

 private:
  struct WorkerInfo {
    WorkerInfo(WorkQueue* const queue)
        : worker(queue),
          work_seconds(0),
          evacuate(false),
          active_n(0),
          lateral_schedule(false) {}

    WorkerInfo(const WorkerInfo& worker_info)
        : worker(worker_info.worker),
          work_seconds(worker_info.work_seconds),
          evacuate(worker_info.evacuate) {
      active_n.store(worker_info.active_n, std::memory_order_relaxed);
      lateral_schedule.store(worker_info.active_n, std::memory_order_relaxed);
    }

    WorkQueue* const worker;

    // The amount of time scheduled on this queue since the last call to Sync().
    double work_seconds;

    // True if we should "rehash" the tokens scheduled to run on this queue.
    bool evacuate;

    // The number of incomplete work items scheduled on this queue. 
    std::atomic<int32_t> active_n;

    // True if work on this queue scheduled work on a different worker queue.
    // Reset by Join().
    std::atomic<bool> lateral_schedule;
  };

  WorkerInfo& GetWorkerFromToken(const Token& token);

  std::vector<WorkerInfo> workers_;

  // A cycle_ used to track calls to Sync() so that we don't re-balance tokens
  // more than once between calls to Sync().
  int32_t cycle_;
};

}  // namespace thread

#endif  // THREAD_AFFINITIZINGSCHEDULER_H_
