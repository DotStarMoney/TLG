#include "thread/thread_pool.h"

#include <atomic>
#include <functional>
#include <future>

#include "gtest/gtest.h"
#if defined(_WIN32)
#define NOMINMAX
#endif
#include "absl/synchronization/barrier.h"

namespace {
template <typename F>
void MeetDeadlineOrDie(F f, int32_t msecs) {
  auto async_future = std::async(std::launch::async, f);
  if (async_future.wait_for(std::chrono::milliseconds(msecs)) ==
      std::future_status::timeout) {
    CHECK(false) << "Aborting: threads probably stuck.";
  }
}
}  // namespace

// We stick a deadline on some of these tests due to paranoia about deadlock.
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

        auto barrier = new absl::Barrier(3);

        auto pool = reservoir.GetPool();
        auto test_func = [barrier]() {
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

        std::atomic<int> acc1 = 0;
        std::atomic<int> acc2 = 0;
        auto started_barrier = new absl::Barrier(5);
        auto barrier = new absl::Barrier(5);

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
        EXPECT_EQ(acc2.load(), 1);

        if (barrier->Block()) delete barrier;
        EXPECT_EQ(acc1.load(), 4);

        pool->Join();
      },
      DEFAULT_DEADLINE_MSECS);
}

TEST(ThreadPoolTest, ResizeDownReservoir) {
  MeetDeadlineOrDie(
      []() {
        thread::ThreadReservoir res(2);
        res.Start();

        auto barrier = new absl::Barrier(3);
        auto pool = res.GetPool();
        auto test_func = [barrier]() {
          if (barrier->Block()) delete barrier;
          // Using a delay isn't the greatest test writing here, but we don't
          // have another option.
          std::this_thread::sleep_for(std::chrono::seconds(1));
        };
        // Everyone should take their time getting to the join that will be
        // called in Resize.
        pool->Schedule(test_func);
        pool->Schedule(test_func);
        if (barrier->Block()) delete barrier;
        // Will block until the thread we need to join on completes.
        res.Resize(1);
        pool->Join();
      },
      DEFAULT_DEADLINE_MSECS + 1000);
}

TEST(ThreadPoolTest, MultiplePools) {
  MeetDeadlineOrDie(
      []() {
        thread::ThreadReservoir res(2);

        auto pool1 = res.GetPool();
        auto pool2 = res.GetPool();

        std::atomic<int> acc(0);
        auto add_func = [&acc]() { ++acc; };

        pool1->Schedule(add_func);
        pool2->Schedule(add_func);
        pool1->Schedule(add_func);
        pool2->Schedule(add_func);
        pool2->Schedule(add_func);

        res.Start();

        EXPECT_EQ(acc.load(), 5);
      },
      DEFAULT_DEADLINE_MSECS);
}

TEST(ThreadPoolTest, BadResSize) {
  EXPECT_DEATH({ thread::ThreadReservoir res(0); }, ".*at least 1.*");
  EXPECT_DEATH(
      {
        thread::ThreadReservoir res(4);
        res.Resize(-1);
      },
      ".*less than 1.*");
}

TEST(ThreadPoolTest, StartMoreThanOnce) {
  EXPECT_DEATH(
      {
        thread::ThreadReservoir res(4);
        res.Start();
        res.Start();
      },
      ".*already called.*");
}

TEST(ThreadPoolTest, OutstandingRefToPool) {
  EXPECT_DEATH(
      {
        auto res = std::make_unique<thread::ThreadReservoir>(4);
        auto pool = res->GetPool();
        res.reset(nullptr);
      },
      "Loans remained.*");
}

TEST(ThreadPoolTest, DestructingPoolWithoutJoin) {
  EXPECT_DEATH(
      {
        thread::ThreadReservoir res(4);
        auto pool = res.GetPool();
        pool->Schedule(
            []() { std::this_thread::sleep_for(std::chrono::seconds(1)); });
      },
      ".*with active threads.*");
}
