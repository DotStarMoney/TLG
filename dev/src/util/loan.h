#ifndef UTIL_LOAN_H_
#define UTIL_LOAN_H_

#include <atomic>

#include "base/assert.h"
#include "util/noncopyable.h"

namespace util {

// Loan and Lender together help safely implementing the generator pattern.
// One class, a Lender, can create Loan objects that that must not outlive the 
// Lender (and are safely caught using reference counting if so).
//
// A common example is a parent class creating an object for a client that
// references the parent. In this case, the parent class subclasses lender,
// and the created object has a private constructor the parent has access to
// that takes a loan. The parent (lender) loans a pointer of itself to the
// newly created class. Should the lender destruct before the loan, this is
// safely caught.
//
class Lender;
template <class T>
class Loan : public NonCopyable {
  friend class Lender;

 public:
  ~Loan() {
    // If we hae been "moved out of," we'll have nothing to decrement.
    if (lender_ref != nullptr) --(*lender_ref_);
  }

  // The move constructor invalidates the old reference.
  Loan(Loan&& loan)
      : lender_ref_(loan.lender_ref_), loaned_ptr_(loan.loaned_ptr_) {
    loan.lender_ref = nullptr;
  }
  // Because these operations involve a comparison, its best to store the
  // the pointer using get() and access a loaned object that way.
  T& operator*() const { 
    ASSERT_NE(lender_ref_, nullptr); 
    return *loaned_ptr_;
  }
  T* operator->() const {
    ASSERT_NE(lender_ref_, nullptr);
    return loaned_ptr_; 
  }
  T* get() const { return loaned_ptr_; }

 private:
  // Take and increment a reference to a lender.
  Loan(std::atomic<int>* lender_ref, T* loaned_ptr)
      : lender_ref_(lender_ref), loaned_ptr_(loaned_ptr) {
    ++(*lender_ref_);
  }
  std::atomic<int>* lender_ref_;
  T* loaned_ptr_;
};

class Lender : public NonCopyable {
 public:
  Lender() : ref_(0) {}
  virtual ~Lender() { TerminateLoans(); }

 protected:
  template <class T>
  Lease<T> MakeLoan() const {
    return Loan(&ref_, reinterpret_cast<T*>(this));
  }

  void TerminateLoans() const { ASSERT_EQ(ref_.load(), 0); }
 private:
  mutable std::atomic<int> ref_;
};
}  // namespace util

#endif  // UTIL_LOAN_H_

