#ifndef UTIL_STATUSOR_H_
#define UTIL_STATUSOR_H_

#include <type_traits>
#include <variant>

#include "assert.h"
#include "status.h"

namespace util {

// Used as a return type from functions that could both return a value, or an
// error.
template <class T>
class StatusOr final {
  static_assert(!std::is_same<T, Status>::value, "Cannot construct a StatusOr "
      "with type Status.");
 public:
  template <class ForwardT>
  StatusOr(ForwardT&& x) : statusor_(std::forward<ForwardT>(x)) {
    static_assert(std::is_convertible<ForwardT, T>::value ||
      std::is_same<ForwardT, Status>::value, "This StatusOr cannot be "
          "constructed from the provided type.");
    if constexpr(std::is_same<ForwardT, Status>::value) {
      // TODO(?): Provide a useful log from this assertion
      ASSERT(!std::get<Status>(statusor_).ok());
    }
  }
  StatusOr(StatusOr<T>&& x) : statusor_(std::move(x.statusor_)) {}
  
  ~StatusOr() {}

  // Disallow copy and assign.
  StatusOr & operator=(const StatusOr&) = delete;
  StatusOr(const StatusOr&) = delete;

  // *IMPORTANT* ConsumeValueOrDie transfers ownership of its value to the 
  // caller; calling this method twice is undefined.
  T&& ConsumeValueOrDie() {
    // Obviously, if we hold a status, its no use trying to return a T.
    // TODO(?): Provide a useful log from this assertion
    ASSERT_NE(statusor_.index(), 0); 
    return std::move(std::get<T>(statusor_));
  }

  // *IMPORTANT* ConsumeValue is unsafe and should ONLY be called when it is
  // known for sure that this StatusOr was constructed with a T and not a
  // Status.
  // *IMPORTANT* ConsumeValue transfers ownership of its value to the caller;
  // calling this method twice is undefined.
  T&& ConsumeValue() {
    return std::move(std::get<T>(statusor_));
  }

  Status status() const {
    if (statusor_.index() == 0) {
      return std::get<Status>(statusor_); 
    }
    // It must be that, if we hold a value, our status is ok (the default
    // status).
    return util::OkStatus;
  }

  bool ok() const {
    return statusor_.index() != 0;
  }
 private:
  std::variant<Status, T> statusor_;
};

} // namespace util


#endif // UTIL_STATUSOR_H_