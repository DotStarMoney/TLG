#include "thread/thread_pool.h"

#include <atomic>

#include "gtest/gtest.h"

TEST(ThreadPoolTest, Basic) {
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
}

TEST(ThreadPoolTest, AreActualThreads) {
  thread::ThreadReservoir reservoir(2);
  reservoir.Start();

  // absl::Barrier is having a hard time compiling here, so we use a spin lock
  // for now.
  std::atomic<int32_t> spin_barrier(2);

  auto pool = reservoir.GetPool();
  auto test_func = [&spin_barrier]() {
    --spin_barrier;
    while (spin_barrier.load() > 0);
  };
  pool->Schedule(test_func);
  pool->Schedule(test_func);

  // If the scheduled work was run serially, this test would never complete.
  while (spin_barrier.load() > 0);
  pool->Join();
}

// Test resize, check failures, multiple pools, loan failures, any other
// combinations
