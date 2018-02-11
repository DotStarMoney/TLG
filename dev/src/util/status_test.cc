#include "util/status.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(StatusTest, BasicConstructor) {
  util::Status status(util::error::UNIMPLEMENTED_ERROR, "Basic error.");
  EXPECT_EQ(status.message(), "Basic error.");
  EXPECT_EQ(status.cannonical_error_code(), util::error::UNIMPLEMENTED_ERROR);
  EXPECT_FALSE(status.ok());

  status = util::Status();  // Effectively util::OkStatus
  EXPECT_EQ(status.message(), "");
  EXPECT_EQ(status.cannonical_error_code(), util::error::UNKNOWN);
  EXPECT_TRUE(status.ok());

  EXPECT_DEATH(
      { util::Status bad_status(util::error::UNKNOWN, "please clap"); },
      ".*not be unknown");

  EXPECT_DEATH(
      { util::Status bad_status(util::error::FAILED_PRECONDITION, ""); },
      ".*not be empty");
}

TEST(StatusTest, CopyAndAssign) {
  util::Status base_status(util::error::LOGIC_ERROR, "X");

  // Copy constructor
  util::Status test_status(base_status);
  EXPECT_EQ(base_status, test_status);
  // Assignment
  test_status = base_status;
  EXPECT_EQ(base_status, test_status);

  util::Status moved_status(base_status);
  EXPECT_EQ(base_status, moved_status);
  // RValue ref assignment
  test_status = std::move(moved_status);
  EXPECT_EQ(base_status, test_status);
  EXPECT_NE(base_status, moved_status);

  // RValue ref constructor
  util::Status move_constructed_status(std::move(test_status));
  EXPECT_NE(base_status, test_status);
  EXPECT_EQ(base_status, move_constructed_status);
}

TEST(StatusTest, ToString) {
  util::Status status = util::FormatMismatchError("Y");
  EXPECT_THAT(status.ToString(), testing::HasSubstr("Format mismatch"));
}
