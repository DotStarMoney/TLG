#include "base/abort.h"

#include "gtest/gtest.h"

TEST(AbortTest, Abort) {
  ASSERT_DEATH({ base::abort("message"); }, "message");
}
