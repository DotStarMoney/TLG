#ifndef THREAD_AFFINITIZINGSCHEDULER_H_
#define THREAD_AFFINITIZINGSCHEDULER_H_

#include <atomic>
#include <mutex>
#include <vector>

#include "thread/workqueue.h"
#include "util/noncopyable.h"

namespace thread {

class AffinitizingScheduler : public util::NonCopyable {
 public:
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

    int32_t id_;
    double consumes_;
    double last_cycle_consumed_;
    std::mutex m_;
    std::atomic<int32_t> last_active_cycle_;
  };

  void Schedule(Token* token, std::function<void()> work);

  void Sync();

  void WaitForGroupCompletion() const;

  std::vector<double> GetWorkingTime() const;

  static Token GetToken();

 private:
  struct WorkerInfo {
    WorkerInfo(WorkQueue* const queue)
        : worker(queue),
          work_seconds(0),
          evacuate(false),
          active_n(0),
          last_completed_cycle(-1) {}

    WorkerInfo(const WorkerInfo& worker_info)
        : worker(worker_info.worker),
          work_seconds(worker_info.work_seconds),
          evacuate(worker_info.evacuate) {
      active_n.store(worker_info.active_n, std::memory_order_relaxed);
      last_completed_cycle.store(worker_info.last_completed_cycle,
                                 std::memory_order_relaxed);
    }

    WorkQueue* const worker;
    double work_seconds;
    bool evacuate;
    std::atomic<int32_t> active_n;
    std::atomic<int32_t> last_completed_cycle;
  };

  WorkerInfo& GetWorkerFromToken(const Token& token);

  std::vector<WorkerInfo> workers_;
  int32_t cycle_;
};

}  // namespace thread

#endif  // THREAD_AFFINITIZINGSCHEDULER_H_
