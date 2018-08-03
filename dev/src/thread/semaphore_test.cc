#include "thread/semaphore.h"

#if defined(_WIN32)
#define NOMINMAX
#endif
#include "absl/synchronization/barrier.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define NEW_BARRIER(_THREADS_) new absl::Barrier((_THREADS_));
#define BLOCK(_BARRIER_) if ((_BARRIER_)->Block()) delete (_BARRIER_);


namespace thread {

TEST(SemaphoreTest, Basic) {
  Semaphore x(1); 
  auto barrier = NEW_BARRIER(2);

  std::thread taker1([&x, &barrier](){
    x.P();
    BLOCK(barrier);
    x.P();
  });

  BLOCK(barrier);


}

}  // namespace thread
