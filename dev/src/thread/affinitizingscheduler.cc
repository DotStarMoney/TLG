#include "thread/affinitizingscheduler.h"

#include <mutex>

#include "absl/base/internal/cycleclock.h"
#include "glog/logging.h"
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

void AffinitizingScheduler::CommitScheduling(WorkerInfo* worker_info) {
  bool lateral = false;
  if (std::this_thread::get_id() != worker_info->worker->GetWorkerThreadId()) {
    lateral =
        worker_info->lateral_schedule.exchange(true, std::memory_order_relaxed);
  }
  worker_info->active_n.fetch_add(1, std::memory_order_acquire);
}

void AffinitizingScheduler::Schedule(uint32_t worker,
                                     std::function<void()> work) {
  CHECK_LT(worker, workers_.size()) << "Worker index out of range.";
  WorkerInfo& cur_worker = workers_[worker];

  CommitScheduling(&cur_worker);
  auto w = [&cur_worker, work]() {
    work();
    cur_worker.active_n.fetch_sub(1, std::memory_order_release);
  };
  CHECK(cur_worker.worker->TryAddWork(w))
      << "Cannot block scheduling on full work queue: deadlock possible.";
}

void AffinitizingScheduler::Schedule(Token* token, std::function<void()> work) {
  // We guard this condition with last_active_cycle_ so that we won't always
  // take the mutex. This re-balance will happen once per token scheduling
  // between two calls to Sync().
  if (token->last_active_cycle_.load(std::memory_order_relaxed) != cycle_) {
    std::unique_lock<std::mutex> lock(token->m_);
    if (token->last_active_cycle_.load(std::memory_order_relaxed) != cycle_) {
      token->consumes_ =
          token->consumes_ * 0.8 + token->last_cycle_consumed_ * 0.2;
      token->last_cycle_consumed_ = 0;

      WorkerInfo& cur_worker = GetWorkerFromToken(*token);
      // If this worker wants to purge...
      if (cur_worker.evacuate) {
        // Get a temporally-smoothed percentage of time this token contributed
        // to this thread's work. Since the value of consumes is based on its
        // previous value, this can exceed 1.
        const double time_contribution =
            token->consumes_ / cur_worker.last_work_seconds;
        // This token may schedule on a new worker if work scheduled with it
        // occupies less than 60% of the total work on this worker. If this is
        // true, we randomly pick a new worker thread (picking with a
        // probability equal to this token's work percentage).
        if ((time_contribution < 0.6) &&
            !util::TrueWithChance(time_contribution)) {
          token->id_ = GetTokenId();
        }
        token->last_active_cycle_.store(cycle_, std::memory_order_relaxed);
      }
    }
  }

  WorkerInfo& cur_worker = GetWorkerFromToken(*token);
  CommitScheduling(&cur_worker);
  auto w = [this, &cur_worker, token, work]() {
    int64_t start_elapsed_cycles = absl::base_internal::CycleClock::Now();

    work();

    int64_t elapsed_cycles =
        absl::base_internal::CycleClock::Now() - start_elapsed_cycles;
    const double elapsed_secs =
        elapsed_cycles / absl::base_internal::CycleClock::Frequency();

    token->last_cycle_consumed_ += elapsed_secs;
    cur_worker.work_seconds += elapsed_secs;

    cur_worker.active_n.fetch_sub(1, std::memory_order_release);
  };
  CHECK(cur_worker.worker->TryAddWork(w))
      << "Cannot block scheduling on full work queue: deadlock possible.";
}

// The balancing algorithm is very simple: if a worker works over 20% longer
// than the average working time across all workers, we mark it and any tokens
// currently scheduling with it may schedule on new workers.
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
    worker_info.last_work_seconds = worker_info.work_seconds;
    worker_info.work_seconds = 0;
  }
}

void AffinitizingScheduler::Join() {
  int32_t cur_worker = 0;
  // We use a polling loop that checks each worker to see if it has outstanding
  // work items. If a worker doesn't have outstanding work items, we check to
  // see if the worker has scheduled work on a different worker since we last
  // checked its state. If it has, we have to recheck the other threads since
  // they may now have running work items.
  while (cur_worker < workers_.size()) {
    WorkerInfo& worker_info = workers_[cur_worker];
    if (worker_info.active_n.load(std::memory_order_relaxed) == 0) {
      if (worker_info.lateral_schedule.load(std::memory_order_relaxed)) {
        cur_worker = 0;
        worker_info.lateral_schedule.exchange(false, std::memory_order_relaxed);
        continue;
      }
      ++cur_worker;
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
