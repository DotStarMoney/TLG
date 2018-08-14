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
constexpr uint32_t kActors = 1000;

const uint32_t kActorChunkSize =
    static_cast<uint32_t>(std::ceil(static_cast<double>(kActors) / kQueues));

constexpr double kLoadMean = 1000;
constexpr double kLoadStd = 3000;

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
      : t(t), load(load), count(0) {}
  AffinitizingScheduler::Token t;
  uint32_t load;
  uint32_t count;
  void work() {
    for (uint32_t i = 0; i < load; ++i) {
      count = (count + 1) * 2 - 1;
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
    queues.push_back(std::make_unique<WorkQueue>(1024));
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

  double end_time = GetClockSeconds() + 10.0;

  uint32_t cycles = 0;
  while (GetClockSeconds() < end_time) {
    for (int q = 0; q < queues.size(); ++q) {
      scheduler.Schedule(q, [q, &scheduler, &actors]() {
        uint32_t l = q * kActorChunkSize;
        uint32_t h = std::min(l + kActorChunkSize, kActors);
        for (; l < h; ++l) {
          scheduler.Schedule(&(actors[l].t),
                             std::bind(&Actor::work, &(actors[l])));
        }
      });
    }

    scheduler.Join();

    scheduler.Sync();
    ++cycles;
  }

  uint64_t total = 0;
  for (auto& actor : actors) {
    total += actor.count;
  }
  std::cout << "Cycles/s: " << (double) cycles / 10.0;
  std::cout << total << std::endl;

  return 0;
}
