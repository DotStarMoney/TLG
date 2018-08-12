#include "thread/affinitizingscheduler.h"

#include <mutex>

#include <iostream>

#include "absl/base/internal/cycleclock.h"
#include "util/random.h"

using std::vector;

namespace thread {
namespace {
int32_t GetTokenId() { return static_cast<int32_t>(util::rnd()); }
}  // namespace

AffinitizingScheduler::AffinitizingScheduler(const vector<WorkQueue*>& queues)
    : cycle_(0) {
  for (auto queue : queues) {
    workers_.emplace_back(queue);
  }
}

AffinitizingScheduler::WorkerInfo& AffinitizingScheduler::GetWorkerFromToken(
    const Token& token) {
  return workers_[token.id_ % workers_.size()];
}

void AffinitizingScheduler::Schedule(Token* token, std::function<void()> work) {
  if (token->last_active_cycle_.load(std::memory_order_relaxed) != cycle_) {
    std::unique_lock<std::mutex> lock(token->m_);
    if (token->last_active_cycle_.load(std::memory_order_relaxed) != cycle_) {
      token->consumes_ =
          token->consumes_ * 0.8 + token->last_cycle_consumed_ * 0.2;
      token->last_cycle_consumed_ = 0;

      WorkerInfo& cur_worker = GetWorkerFromToken(*token);
      if (cur_worker.evacuate) {
        const double time_contribution =
            token->consumes_ / cur_worker.work_seconds;
        if ((time_contribution < 0.6) &&
            !util::TrueWithChance(time_contribution)) {
          token->id_ = GetTokenId();
        }
        token->last_active_cycle_.store(cycle_, std::memory_order_relaxed);
      }
    }
  }

  WorkerInfo& cur_worker = GetWorkerFromToken(*token);
  cur_worker.active_n.fetch_add(1, std::memory_order_relaxed);
  cur_worker.worker->AddWork([this, &cur_worker, token, work]() {
    int64_t start_elapsed_cycles = absl::base_internal::CycleClock::Now();

    work();

    int64_t elapsed_cycles =
        absl::base_internal::CycleClock::Now() - start_elapsed_cycles;
    const double elapsed_secs =
        elapsed_cycles / absl::base_internal::CycleClock::Frequency();

    token->last_cycle_consumed_ += elapsed_secs;
    cur_worker.work_seconds += elapsed_secs;

    cur_worker.last_completed_cycle.store(cycle_, std::memory_order_relaxed);
    cur_worker.active_n.fetch_sub(1, std::memory_order_relaxed);
  });
}

constexpr double kOverAveragePercent = 0.2;

void AffinitizingScheduler::Sync() {
  ++cycle_;

  double avg_secs = 0;
  for (auto& worker_info : workers_) {
    avg_secs += worker_info.work_seconds;
  }
  avg_secs /= (double)workers_.size();

  for (auto& worker_info : workers_) {
    worker_info.evacuate =
        worker_info.work_seconds > (avg_secs * (1.0 + kOverAveragePercent));
    worker_info.work_seconds = 0;
  }
}

void AffinitizingScheduler::WaitForGroupCompletion() const {
  // Requires a re-think:
  //   A just blocking when we run out of queue space isn't a great plan as we
  //     run the risk of deadlock in a few different scenarios
  //   B waiting for cycle completion the way we do now requires that at least
  //     SOMETHING has been scheduled on each worker. Maybe we make this the
  //     responsibility of the caller?
  //   C A worker thread can schedule work on a different worker AFTER the
  //     different worker has already set last_completed_cycle and active_n = 0,
  //     this means we may stop blocking while work is still being done.

  // Work that isn't scheduled in the context of AffinitizedScheduler is causing
  // all sorts of problems. 
  //
  // B and C can be solved by adding a "ScheduleOn" method and never directly
  // scheduling stuff on workqueues (outside of AS). Also a monolithic active_n
  // is necessary, and it is on this that we block... or we can be clever and
  // pass foward an active_n ... hmmm
  //
  // Solving A is doable the bad way by making any single workqueue big enough
  // to accommodate all actors... but this isn't a great plan.

  for (const auto& worker : workers_) {
    while ((worker.last_completed_cycle.load(std::memory_order_relaxed) <
            cycle_)) {
      // Spin
    }
    while (worker.active_n.load(std::memory_order_relaxed) > 0) {
      // Spin
    }
  }
}

std::vector<double> AffinitizingScheduler::GetWorkingTime() const {
  std::vector<double> seconds(workers_.size(), 0);
  for (uint32_t i = 0; i < workers_.size(); ++i) {
    seconds[i] = workers_[i].work_seconds;
  }
  return seconds;
}

AffinitizingScheduler::Token AffinitizingScheduler::GetToken() {
  return Token(GetTokenId());
}

}  // namespace thread
