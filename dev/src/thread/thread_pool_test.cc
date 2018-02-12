#include "thread/thread_pool.h"

#include <atomic>
#include <functional>
#include <future>

#include "gtest/gtest.h"
#if defined(_WIN32)
#define NOMINMAX
#endif
#include "absl/synchronization/barrier.h"

template <typename F>
void MeetDeadlineOrDie(F f, int32_t msecs) {
  auto async_future = std::async(std::launch::async, f);
  if (async_future.wait_for(std::chrono::milliseconds(msecs)) ==
      std::future_status::timeout) {
    CHECK(false) << "Aborting: threads stuck.";
  }
}

constexpr int DEFAULT_DEADLINE_MSECS = 3000;

TEST(ThreadPoolTest, Basic) {
  MeetDeadlineOrDie(
      []() {
        thread::ThreadReservoir reservoir(4);

        // Will be destructed first
        auto pool = reservoir.GetPool();

        std::atomic<int32_t> i = 0;

        pool->Schedule([&i]() { ++i; });
        pool->Schedule([&i]() { ++i; });
        pool->Schedule([&i]() { ++i; });
        pool->Schedule([&i]() { ++i; });

        reservoir.Start();
        pool->Join();

        EXPECT_EQ(i.load(), 4);
      },
      DEFAULT_DEADLINE_MSECS);
}

TEST(ThreadPoolTest, ThreadsAreNotSerialized) {
  MeetDeadlineOrDie(
      []() {
        thread::ThreadReservoir reservoir(2);
        reservoir.Start();

        absl::Barrier* barrier = new absl::Barrier(3);

        auto pool = reservoir.GetPool();
        auto test_func = [&barrier]() {
          if (barrier->Block()) delete barrier;
        };
        pool->Schedule(test_func);
        pool->Schedule(test_func);

        if (barrier->Block()) delete barrier;

        pool->Join();
      },
      DEFAULT_DEADLINE_MSECS);
}

TEST(ThreadPoolTest, ResizeUpReservoir) {
  MeetDeadlineOrDie(
      []() {
        thread::ThreadReservoir res(4);
        res.Start();

        int acc1 = 0;
        int acc2 = 0;
        absl::Barrier* started_barrier = new absl::Barrier(5);
        absl::Barrier* barrier = new absl::Barrier(5);

        auto blocking_func = [&acc1, started_barrier, barrier]() {
          if (started_barrier->Block()) delete started_barrier;
          ++acc1;
          if (barrier->Block()) delete barrier;
        };

        absl::Barrier* completed_barrier = new absl::Barrier(2);
        auto non_blocking_func = [&acc2, completed_barrier]() {
          acc2++;
          if (completed_barrier->Block()) delete completed_barrier;
        };

        auto pool = res.GetPool();

        for (int i = 0; i < 4; ++i) pool->Schedule(blocking_func);
        // Wait until we know our 4 workers have picked up the 4 tasks
        if (started_barrier->Block()) delete started_barrier;
        pool->Schedule(non_blocking_func);

        // Add an extra worker
        res.Resize(5);

        if (completed_barrier->Block()) delete completed_barrier;
        EXPECT_EQ(acc2, 1);

        if (barrier->Block()) delete barrier;
        EXPECT_EQ(acc1, 4);

        pool->Join();
      },
      DEFAULT_DEADLINE_MSECS);
}

TEST(ThreadPoolTest, ResizeDownReservoir) {
  MeetDeadlineOrDie(
      []() {
        thread::ThreadReservoir res(4);
        res.Start();



      },
      DEFAULT_DEADLINE_MSECS);
}

// check failures, multiple pools, loan failures