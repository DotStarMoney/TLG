#include "util/status_macros.h"

#include "gtest/gtest.h"

TEST(StatusMacrosTest, ReturnIfError) {
  int marker = 0;
  util::Status status = [&marker]() -> util::Status {
    RETURN_IF_ERROR([]() { return util::InvalidArgumentError("bad"); }());
    marker = 1;
    return util::OkStatus;
  }();
  EXPECT_EQ(marker, 0);
  EXPECT_EQ(status, util::InvalidArgumentError("bad"));

  marker = 0;
  status = [&marker]() -> util::Status {
    RETURN_IF_ERROR([]() { return util::OkStatus; }());
    marker = 1;
    return util::OkStatus;
  }();
  EXPECT_EQ(marker, 1);
  EXPECT_EQ(status, util::OkStatus);

  marker = 0;
  status = [&marker]() -> util::Status {
    RETURN_IF_ERROR([]() -> util::StatusOr<int> {
      return util::FailedPreconditionError("still bad");
    }());
    marker = 1;
    return util::OkStatus;
  }();
  EXPECT_EQ(marker, 0);
  EXPECT_EQ(status, util::FailedPreconditionError("still bad"));

  marker = 0;
  status = [&marker]() -> util::Status {
    RETURN_IF_ERROR([]() -> util::StatusOr<int> { return 10; }());
    marker = 1;
    return util::OkStatus;
  }();
  EXPECT_EQ(marker, 1);
  EXPECT_EQ(status, util::OkStatus);
}

TEST(StatusMacrosTest, AssignOrReturn) {
  int marker = 0;
  util::Status status = [&marker]() -> util::Status {
    ASSIGN_OR_RETURN(int dummy, []() -> util::StatusOr<int> {
      return util::FailedPreconditionError("still bad");
    }());
    marker = 1;
    return util::OkStatus;
  }();
  EXPECT_EQ(marker, 0);
  EXPECT_EQ(status, util::FailedPreconditionError("still bad"));

  marker = 0;
  status = [&marker]() -> util::Status {
    ASSIGN_OR_RETURN(int dummy, []() -> util::StatusOr<int> { return 1; }());
    marker = dummy;
    return util::OkStatus;
  }();
  EXPECT_EQ(marker, 1);
  EXPECT_EQ(status, util::OkStatus);
}

TEST(StatusMacrosTest, AssignOrReturnMacroCreatedVariableNaming) {
  int marker = 0;
  util::Status status = [&marker]() -> util::Status {
    ASSIGN_OR_RETURN(int dummy, []() -> util::StatusOr<int> { return 1; }());
    marker += dummy;
    ASSIGN_OR_RETURN(dummy, []() -> util::StatusOr<int> { return 2; }());
    marker += dummy;
    return util::OkStatus;
  }();
  EXPECT_EQ(marker, 3);
  EXPECT_EQ(status, util::OkStatus);
}
