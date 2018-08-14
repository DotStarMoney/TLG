#include "thread/workqueue.h"

#include <atomic>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#if defined(_WIN32)
#define NOMINMAX
#endif
#include "absl/synchronization/barrier.h"

namespace thread {

TEST(WorkQueueTest, DiesFromInvalidBufferLength) {
  EXPECT_DEATH({ WorkQueue q(0); }, ".*");
}

#define NEW_BARRIER(_X_) new absl::Barrier(_X_);
#define SYNC(_BARRIER_) \
  if ((_BARRIER_)->Block()) delete (_BARRIER_);

TEST(WorkQueueTest, CompletesAUnitOfWork) {
  auto b = NEW_BARRIER(2);

  int counter = 0;
  {
    WorkQueue q(1);
    q.AddWork([&counter, b]() {
      ++counter;
      SYNC(b);
    });
    SYNC(b);
  }

  EXPECT_EQ(counter, 1);
}

TEST(WorkQueueTest, WorkersResumeFromBlock) {
  auto b = NEW_BARRIER(5);

  std::atomic_int32_t counter = 0;
  {
    WorkQueue q(3);

    auto f = [b, &q, &counter]() {
      SYNC(b);
      for (int i = 0; i < 100; ++i) {
        q.AddWork([&counter]() { ++counter; });
      }
    };

    std::thread t0(f);
    std::thread t1(f);
    std::thread t2(f);
    std::thread t3(f);

    SYNC(b);

    t0.join();
    t1.join();
    t2.join();
    t3.join();
  }
  EXPECT_EQ(counter, 400);
}

TEST(WorkQueueTest, TryAdd) {
  WorkQueue q(1);
  auto b = NEW_BARRIER(2);

  EXPECT_TRUE(q.TryAddWork([b]() { SYNC(b); }));
  EXPECT_FALSE(q.TryAddWork([]() {}));
  SYNC(b);
}
}  // namespace thread
