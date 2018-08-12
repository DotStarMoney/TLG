#include <math.h>
#include <algorithm>
#include <limits>
#include <memory>
#include <random>
#include <vector>

#include <iostream>

#include "absl/base/internal/cycleclock.h"
#include "glog/logging.h"
#include "thread/affinitizingscheduler.h"
#include "thread/workqueue.h"

using std::unique_ptr;
using std::vector;
using thread::AffinitizingScheduler;
using thread::WorkQueue;

namespace {
constexpr uint32_t kQueues = 4;
constexpr uint32_t kActors = 10;

const uint32_t kActorChunkSize =
    static_cast<uint32_t>(std::ceil(static_cast<double>(kActors) / kQueues));

constexpr double kLoadMean = 3000;
constexpr double kLoadStd = 400;

std::default_random_engine generator;
std::normal_distribution<double> norm_rnd(kLoadMean, kLoadStd);
uint32_t GetLoad() {
  double load = norm_rnd(generator);
  if (load < 0) {
    load = 0;
  } else if (load > static_cast<double>(std::numeric_limits<uint32_t>::max())) {
    load = static_cast<double>(std::numeric_limits<uint32_t>::max());
  }
  return static_cast<uint32_t>(load);
}

struct Actor {
  Actor(const AffinitizingScheduler::Token& t, uint32_t load)
      : t(t), load(load) {}
  AffinitizingScheduler::Token t;
  uint32_t load;
  void work() const {
    for (uint32_t i = 0; i < load; ++i) {
      // Spin
    }
  }
};

double GetClockSeconds() {
  return absl::base_internal::CycleClock::Now() /
         absl::base_internal::CycleClock::Frequency();
}
}  // namespace

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);

  vector<unique_ptr<WorkQueue>> queues;
  for (int i = 0; i < kQueues; ++i) {
    queues.push_back(std::make_unique<WorkQueue>(16));
  }

  vector<WorkQueue*> just_queues;
  for (auto& q : queues) {
    just_queues.push_back(q.get());
  }
  AffinitizingScheduler scheduler(just_queues);

  vector<Actor> actors;
  for (int i = 0; i < kActors; ++i) {
    actors.push_back(Actor(AffinitizingScheduler::GetToken(), GetLoad()));
  }

  double end_time = GetClockSeconds() + 1.0;

  while (GetClockSeconds() < end_time) {
    for (int q = 0; q < queues.size(); ++q) {
      queues[q]->AddWork([q, &scheduler, &actors]() {
        uint32_t l = q * kActorChunkSize;
        uint32_t h = std::min(l + kActorChunkSize, kActors);
        for (; l < h; ++l) {
          scheduler.Schedule(&(actors[l].t),
                             std::bind(&Actor::work, &(actors[l])));
        }
      });
    }

    scheduler.WaitForGroupCompletion();

    auto working_seconds = scheduler.GetWorkingTime();

    scheduler.Sync();
  }

  return 0;
}
