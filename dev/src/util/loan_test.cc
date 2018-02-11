#include "util/loan.h"

#include <vector>

#include "gtest/gtest.h"

class TestLender : public util::Lender {
 public:
  TestLender() { cats_ = 0; }
  virtual ~TestLender() { TerminateLoans(); }

  util::Loan<TestLender> TakeLoan() { return MakeLoan<TestLender>(); }
  void PushLoan() { loans_.push_back(MakeLoan<TestLender>()); }
  void PopLoan() {
    auto loan = std::move(loans_.back());
    loans_.pop_back();
    // Let the loan destruct naturally
  }

  int cats_;

 private:
  std::vector<util::Loan<TestLender>> loans_;
};

TEST(LoanTest, DoNothing) { TestLender lender; }

TEST(LoanTest, ReturnAllLoans) {
  TestLender lender;

  int i;
  for (i = 0; i < 3; ++i) lender.PushLoan();
  for (i = 0; i < 3; ++i) lender.PopLoan();
}

TEST(LoanTest, LoanIsReference) {
  TestLender lender;
  lender.cats_ = 7;
  {
    auto loan = lender.TakeLoan();
    EXPECT_EQ(loan->cats_, 7);
    EXPECT_EQ((*loan).cats_, 7);
    EXPECT_EQ(loan.get()->cats_, 7);

    lender.cats_ = 30;

    EXPECT_EQ(loan->cats_, 30);
    EXPECT_EQ((*loan).cats_, 30);
    EXPECT_EQ(loan.get()->cats_, 30);
  }
}

TEST(LoanTest, MoveInvalidatesLoan) {
  TestLender lender;
  {
    auto loan1 = lender.TakeLoan();
    EXPECT_NE(loan1.get(), nullptr);

    auto loan2 = std::move(loan1);
    EXPECT_EQ(loan1.get(), nullptr);
    EXPECT_NE(loan2.get(), nullptr);

    EXPECT_DEATH({ int i = loan1->cats_; }, "Loaned ptr invalidated");
    EXPECT_DEATH({ int i = (*loan1).cats_; }, "Loaned ptr invalidated");
  }
}

TEST(LoanTest, OutstandingLoans) {
  EXPECT_DEATH(
      {
        TestLender lender;
        lender.PushLoan();
        lender.PushLoan();

        lender.PopLoan();
      },
      "Loans remained.*");
}
