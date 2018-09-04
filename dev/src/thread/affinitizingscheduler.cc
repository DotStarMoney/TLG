#include "thread/affinitizingscheduler.h"

#include <iostream>

#include <algorithm>
#include <mutex>

#include "absl/base/internal/cycleclock.h"
#include "glog/logging.h"
#include "util/random.h"

using std::vector;

namespace thread {

namespace {
int32_t GetTokenId() { return static_cast<int32_t>(util::rnd()); }
constexpr double kWorkTimeSmoothing = 0.8;
}  // namespace

AffinitizingScheduler::AffinitizingScheduler(const vector<WorkQueue*>& queues)
    : cycle_(0),
      balance_rdc_min_(1.0 / queues.size()),
      balance_rdc_scale_(queues.size() / (queues.size() - 1.0)) {
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
      if (!util::TrueWithChance(GetWorkerFromToken(*token).balance_f)) {
        token->id_ = GetTokenId();
      }
      token->last_active_cycle_.store(cycle_, std::memory_order_relaxed);
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

    cur_worker.work_seconds += elapsed_secs;

    cur_worker.active_n.fetch_sub(1, std::memory_order_release);
  };
  CHECK(cur_worker.worker->TryAddWork(w))
      << "Cannot block scheduling on full work queue: deadlock possible.";
}

void AffinitizingScheduler::Sync() {
  ++cycle_;

  double avg_secs = 0;
  for (auto& worker_info : workers_) {
    worker_info.last_work_seconds =
        worker_info.last_work_seconds * kWorkTimeSmoothing +
        worker_info.work_seconds * (1 - kWorkTimeSmoothing);
    avg_secs += worker_info.last_work_seconds;
    worker_info.work_seconds = 0;
  }

  avg_secs /= (double)workers_.size();

  for (auto& worker_info : workers_) {
    worker_info.balance_f =
        ((avg_secs / worker_info.last_work_seconds) - balance_rdc_min_) *
        balance_rdc_scale_;

    worker_info.balance_f = worker_info.balance_f +
                            (1 - worker_info.balance_f) * kWorkTimeSmoothing;
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
    seconds[i] = workers_[i].last_work_seconds;
  }
  return seconds;
}

AffinitizingScheduler::Token AffinitizingScheduler::GetToken() {
  return Token(GetTokenId());
}
}  // namespace thread
