#ifndef UTIL_LOAN_H_
#define UTIL_LOAN_H_

#include <atomic>

#include "glog/logging.h"
#include "util/noncopyable.h"

namespace util {

// Loan and Lender together help safely implementing the generator pattern.
// One class, a Lender, can create Loan objects that that must not outlive the
// Lender (and are safely caught using reference counting if so). Loans should
// only be accessed from a single thread.
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
    if (loaned_ptr_ != nullptr) --(*lender_ref_);
  }

  Loan() : lender_ref_(nullptr), loaned_ptr_(nullptr) {}

  // The move constructor invalidates the old reference.
  Loan(Loan&& loan) 
      : lender_ref_(loan.lender_ref_), loaned_ptr_(loan.loaned_ptr_) {
    loan.loaned_ptr_ = nullptr;
  }

  // Move assignment invalidates the old reference and calls the destructor
  Loan& operator=(Loan&& loan) {
    this->~Loan();
    lender_ref_ = loan.lender_ref_;
    loaned_ptr_ = loan.loaned_ptr_;
    loan.loaned_ptr_ = nullptr;
    return *this;
  }

  // Because these operations involve a comparison, its best to store the
  // the pointer using get() and access a loaned object that way.
  T& operator*() const {
    CHECK_NE(loaned_ptr_, reinterpret_cast<T*>(nullptr))
        << "Loaned ptr invalid.";
    return *loaned_ptr_;
  }
  T* operator->() const {
    CHECK_NE(loaned_ptr_, reinterpret_cast<T*>(nullptr))
        << "Loaned ptr invalid.";
    return loaned_ptr_;
  }
  T* get() const { return loaned_ptr_; }

 private:
  // Take and increment a reference to a lender.
  Loan(std::atomic<int>* lender_ref, T* loaned_ptr)
      : lender_ref_(lender_ref), loaned_ptr_(loaned_ptr) {
    ++(*lender_ref_);
  }
  std::atomic<int> mutable* lender_ref_;
  T* loaned_ptr_;
};

class Lender : public util::NonCopyable {
 public:
  Lender() : ref_(0) {}
  virtual ~Lender() { TerminateLoans(); }

 protected:
  template <class T>
  Loan<T> MakeLoan() {
    return Loan<T>(&ref_, reinterpret_cast<T*>(this));
  }

  template <class T>
  Loan<T> MakeLoan(T* source) const {
    return Loan<T>(&ref_, source);
  }

  void TerminateLoans() const {
    CHECK_EQ(ref_.load(), 0) << "Loans remained when terminating.";
  }

 private:
  mutable std::atomic<int> ref_;
};
}  // namespace util

#endif  // UTIL_LOAN_H_
