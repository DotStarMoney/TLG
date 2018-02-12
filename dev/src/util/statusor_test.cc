#include "util/statusor.h"

#include "gtest/gtest.h"
#include "util/status.h"

TEST(StatusOrTest, BasicConstruction) {
  util::StatusOr<int> x(1);
  EXPECT_TRUE(x.ok());
  EXPECT_EQ(x.status(), util::OkStatus);
  EXPECT_EQ(x.ConsumeValue(), 1);

  util::StatusOr<int> y(util::LogicError("please clap"));
  EXPECT_FALSE(y.ok());
  EXPECT_EQ(y.status(), util::LogicError("please clap"));
}

class MoveWatcher {
 public:
  MoveWatcher() : move_count_(0), copy_count_(0) {}
  MoveWatcher(MoveWatcher& in) {
    this->move_count_ = in.move_count_;
    this->copy_count_ = in.copy_count_ + 1;
  }
  MoveWatcher(MoveWatcher&& in) {
    this->move_count_ = in.move_count_ + 1;
    this->copy_count_ = in.copy_count_;
  }
  int move_count_;
  int copy_count_;
};

inline util::StatusOr<MoveWatcher> MoveOnConsumeTestFunc() {
  return MoveWatcher();
}

TEST(StatusOrTest, MoveOnConsume) {
  util::StatusOr<MoveWatcher> x = MoveOnConsumeTestFunc();
  MoveWatcher y = x.ConsumeValue();
  EXPECT_EQ(y.move_count_, 2);
  EXPECT_EQ(y.copy_count_, 0);
}

TEST(StatusOrTest, TheDiePartOfConsumeValueOrDie) {
  util::StatusOr<int> x(util::OutOfMemoryError("out o' gas"));
  EXPECT_DEATH({ int y = x.ConsumeValueOrDie(); }, "Attempting to consume.*");
}

TEST(StatusOrTest, ConstructWithOkStatus) {
  EXPECT_DEATH({ util::StatusOr<int> statusor(util::OkStatus); },
               ".*from an Ok Status.");
}
