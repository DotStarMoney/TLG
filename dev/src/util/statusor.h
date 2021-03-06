#ifndef UTIL_STATUSOR_H_
#define UTIL_STATUSOR_H_

#include <type_traits>
#include <variant>

#include "glog/logging.h"
#include "util/status.h"
#include "util/noncopyable.h"

namespace util {

// Used as a return type from functions that could both return a value, or a
// Status.
template <class T>
class StatusOr final : public util::NonCopyable {
  static_assert(!std::is_same<T, Status>::value,
                "Cannot create a StatusOr with type Status.");

 public:
  template <class ForwardT>
  StatusOr(ForwardT&& x) : statusor_(std::forward<ForwardT>(x)) {
    static_assert(
        std::is_convertible<ForwardT, T>::value ||
            std::is_convertible<ForwardT, Status>::value,
        "This StatusOr cannot be constructed from the provided type.");
    if constexpr (std::is_convertible<ForwardT, Status>::value) {
      CHECK(!std::get<Status>(statusor_).ok())
          << "Cannot construct a StatusOr from an Ok Status.";
    }
  }
  StatusOr(StatusOr<T>&& x) : statusor_(std::move(x.statusor_)) {}

  ~StatusOr() {}

  // *IMPORTANT* ConsumeValueOrDie transfers ownership of its value to the
  // caller; calling this method twice is undefined.
  T&& ConsumeValueOrDie() {
    // Obviously, if we hold a status, its no use trying to return a T.
    CHECK_NE(statusor_.index(), 0)
        << "Attempting to consume value while holding a Status";
    return std::move(std::get<T>(statusor_));
  }

  // *IMPORTANT* ConsumeValue is unsafe and should ONLY be called when it is
  // known for sure that this StatusOr was constructed with a T and not a
  // Status.
  // *IMPORTANT* ConsumeValue transfers ownership of its value to the caller;
  // calling this method twice is undefined.
  T&& ConsumeValue() { return std::move(std::get<T>(statusor_)); }

  const Status& status() const {
    if (statusor_.index() == 0) {
      return std::get<Status>(statusor_);
    }
    // It must be that, if we hold a value, our status is ok (the default
    // status).
    return util::OkStatus;
  }

  bool ok() const { return statusor_.index() != 0; }

 private:
  std::variant<Status, T> statusor_;
};

}  // namespace util

#endif  // UTIL_STATUSOR_H_
