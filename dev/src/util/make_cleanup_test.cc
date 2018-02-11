#include "util/make_cleanup.h"

#include "gtest/gtest.h"

TEST(MakeCleanupTest, BasicFunctionality) {
  int i = 0;
  {
    util::MakeCleanup cleanup([&i]() { ++i; });
    EXPECT_EQ(i, 0);
  }
  EXPECT_EQ(i, 1);
}

TEST(MakeCleanupTest, PreemptCleanup) {
  int i = 0;
  {
    util::MakeCleanup cleanup([&i]() { ++i; });
    EXPECT_EQ(i, 0);
    cleanup.cleanup();
    EXPECT_EQ(i, 1);
  }
  EXPECT_EQ(i, 1);
}
