#include "util/deleterptr.h"

#include "gtest/gtest.h"

TEST(DeleterPtrTest, RunOnDelete) {
  int32_t dummy = 56;
  {
    // Create the deleter ptr and let it go out of scope so hopefully its
    // deleter function runs.
    util::deleter_ptr<int32_t> x(&dummy, [&dummy](int32_t* ptr) {
      EXPECT_EQ(*ptr, dummy);
      // Set the dummy variable to 20 so we can check to see if this delete
      // function was actually ran.
      dummy = 20;
    });
		EXPECT_EQ(dummy, 56);
  }
  EXPECT_EQ(dummy, 20);
}
