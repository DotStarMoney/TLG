#include "thread/affinitizingscheduler.h"

#include <atomic>
#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "thread/gateway.h"
#include "thread/workqueue.h"
#include "util/random.h"

namespace thread {

class AffinitizingSchedulerTest : public ::testing::Test {
 protected:
  void Init(uint32_t queues, uint32_t depth) {
    for (uint32_t i = 0; i < queues; ++i) {
      work_queues.push_back(std::make_unique<WorkQueue>(depth));
    }
    std::vector<WorkQueue*> just_queues;
    for (auto& q : work_queues) {
      just_queues.push_back(q.get());
    }
    scheduler = std::make_unique<AffinitizingScheduler>(just_queues);
    ASSERT_EQ(scheduler->size(), queues);
  }

 private:
  std::vector<std::unique_ptr<WorkQueue>> work_queues;

 protected:
  std::unique_ptr<AffinitizingScheduler> scheduler;
};

TEST_F(AffinitizingSchedulerTest, TestScheduleWorkManually) {
  Init(1, 1);

  bool flag = false;

  scheduler->Schedule(static_cast<uint32_t>(0), [&]() { flag = true; });
  scheduler->Join();

  EXPECT_TRUE(flag);
}

TEST_F(AffinitizingSchedulerTest, TestScheduleWorkBalanced) {
  Init(1, 1);

  bool flag = false;
  AffinitizingScheduler::Token t = AffinitizingScheduler::GetToken();

  scheduler->Schedule(&t, [&]() { flag = true; });
  scheduler->Join();

  EXPECT_TRUE(flag);
}

TEST_F(AffinitizingSchedulerTest, TestRandomBalancingBehavior) {
  Init(4, 512);

  std::atomic<uint32_t> counter = 0;

  util::srnd(0);

  for (int i = 0; i < 1000; ++i) {
    AffinitizingScheduler::Token t = AffinitizingScheduler::GetToken();
    scheduler->Schedule(&t, [&]() { counter++; });
  }
  scheduler->Join();
  EXPECT_EQ(counter, 1000);

  for (double work_time : scheduler->GetWorkingTime()) {
    EXPECT_GT(work_time, 0.0);
  }
}

TEST_F(AffinitizingSchedulerTest, TestLateralScheduling) {
  Init(2, 1);

  bool flag = false;

  scheduler->Schedule(static_cast<uint32_t>(1), [&]() {
    scheduler->Schedule(static_cast<uint32_t>(0), [&]() {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      flag = true;
    });
  });
  scheduler->Join();

  EXPECT_TRUE(flag);
}

TEST_F(AffinitizingSchedulerTest, TestManualSchedulingOutOfRange) {
  Init(3, 3);
  EXPECT_DEATH({ scheduler->Schedule(static_cast<uint32_t>(4), []() {}); },
               "out of range");
}

TEST_F(AffinitizingSchedulerTest, TestQueueFull) {
  Init(1, 1);

  Gateway blocker;
  scheduler->Schedule(static_cast<uint32_t>(0), [&]() { blocker.Enter(); });

  EXPECT_DEATH(
      { scheduler->Schedule(static_cast<uint32_t>(0), []() { FAIL(); }); },
      "deadlock possible");
  blocker.Unlock();

  scheduler->Join();
}

TEST_F(AffinitizingSchedulerTest, TestLoadBalancing) {
  Init(4, 16);

  // The first four pseudo-random values produced by the stream resulting from
  // this seed are identical mod 4 after casting to int32_t.
  util::srnd(269);

  std::vector<AffinitizingScheduler::Token> tokens;
  for (int i = 0; i < 4; ++i) {
    tokens.push_back(AffinitizingScheduler::GetToken());
  }

  // Expect that after 10 iterations, each work item will have its own thread.
  // Keep going for 10 more iterations to ensure this does not change.
  constexpr int kIterations = 20;
  constexpr int kAssumeFixedBy = 10;

  for (int i = 0; i < kIterations; ++i) {
    for (auto& t : tokens) {
      scheduler->Schedule(&t, []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      });
    }
    scheduler->Join();
    if (i >= (kAssumeFixedBy - 1)) {
      for (double work_time : scheduler->GetWorkingTime()) {
        EXPECT_GT(work_time, 0.0);
      }
    }
    scheduler->Sync();
  }
}

TEST_F(AffinitizingSchedulerTest, TestHeavyWorkItemBalancing) {
  Init(4, 16);

  // See comment in TestLoadBalancing...
  util::srnd(269);

  std::vector<AffinitizingScheduler::Token> tokens;
  for (int i = 0; i < 4; ++i) {
    tokens.push_back(AffinitizingScheduler::GetToken());
    // These values, assuming the OS isn't havin' some issues, should be close
    // to the amounts that we sleep below.
    tokens.back().set_consumes(i > 0 ? 0.001 : 0.01);
  }
  constexpr int kIterations = 20;

  const int32_t id0 = tokens[0].id();

  for (int i = 0; i < kIterations; ++i) {
    for (int q = 0; q < 4; ++q) {
      auto& t = tokens[q];
      scheduler->Schedule(&t, [q]() {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(q > 0 ? 1 : 10));
      });
    }
    scheduler->Join();
    scheduler->Sync();
    // Make sure work scheduled with token 0 never re-balances
    EXPECT_EQ(id0, tokens[0].id());
  }
}

}  // namespace thread
