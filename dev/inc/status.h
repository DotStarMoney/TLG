#ifndef UTIL_STATUS_H_
#define UTIL_STATUS_H_

#include <atomic>
#include <string_view>

#include "static_type_assert.h"
#include "cannonical_errors.h"

namespace util {

// The state of some execution or return result. Status should be used as a
// return type for methods that would usually throw exceptions.
//
// As Needed:
// TODO(?): Add stack trace
// TODO(?): Add error spaces
// TODO(?): Add update method
class Status final {
 public:
  // Default status object is an OK status.
  Status();
  // err_code must not be UNKNOWN and msg must not be "".
  template <class StringT>
  Status(error::CannonicalErrors err_code, StringT&& msg) {
    type_assert::IsConvertibleTo<std::string, StringT>();
    ASSERT_NE(err_code, error::UNKNOWN);
    ASSERT_NE(msg, "");
    handle_ = new Payload(err_code, std::forward<StringT>(msg));
  }

  ~Status();

  // Copy and assignment operators use reference counting.
  Status(const Status& status);
  Status& operator=(const Status& status);
  // Move constructor assigns our handle to the incoming handle.
  Status(Status&& status);
  Status& operator=(Status&& status);

  bool ok() const;
  // An ok status returns ""
  std::string_view message() const;
  error::CannonicalErrors cannonical_error_code() const;

  friend bool operator==(const Status& lhs, const Status& rhs);
  friend bool operator!=(const Status& lhs, const Status& rhs);
 private:
  struct Payload {
    // Create a new payload with an automatic reference count of 1.
    template <class StringT>
    Payload(error::CannonicalErrors err_code, StringT&& msg) : 
        cannonical_error_code(err_code), message(std::forward<StringT>(msg)),
        refs(1) {
      type_assert::IsConvertibleTo<std::string, StringT>();
    }
    // The core type of error
    const error::CannonicalErrors cannonical_error_code;
    const std::string message;
    std::atomic_uint32_t refs;
  };
  // Decrease the reference count of handle if it is not null.
  static void MaybeDereference(Payload* handle);
  // Increase the reference count of handle if it is not null.
  static void MaybeReference(Payload* handle);
  // Go through string compare and error code compare. This is used when
  // comparing pointers fails. Precondition that lhs and rhs are not equal.
  static bool SlowComparePayloadsForEquality(Payload* lhs, Payload* rhs);

  // When handle is nullptr, this status is happy.
  Payload* handle_;
};

bool operator==(const Status& lhs, const Status& rhs);
bool operator!=(const Status& lhs, const Status& rhs);

extern const Status OkStatus;

// Convenience methods for making statuses from cannonical errors.

template <class StringT>
Status FailedPreconditionError(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::FAILED_PRECONDITION, std::forward<StringT>(msg));
}

template <class StringT>
Status InvalidArgumentError(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::INVALID_ARGUMENT, std::forward<StringT>(msg));
}

template <class StringT>
Status TimeoutError(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::TIMEOUT, std::forward<StringT>(msg));
}

template <class StringT>
Status OutOfMemoryError(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::OUT_OF_MEMORY, std::forward<StringT>(msg));
}

template <class StringT>
Status OutOfBoundsError(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::OUT_OF_BOUNDS, std::forward<StringT>(msg));
}

template <class StringT>
Status LogicError(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::LOGIC_ERROR, std::forward<StringT>(msg));
}

template <class StringT>
Status ResourceUnobtainable(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::RESOURCE_UNOBTAINABLE, std::forward<StringT>(msg));
}

template <class StringT>
Status UnimplementedError(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::UNIMPLEMENTED_ERROR, std::forward<StringT>(msg));
}

template <class StringT>
Status FormatMismatchError(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::FORMAT_MISTMATCH, std::forward<StringT>(msg));
}

template <class StringT>
Status IOError(StringT&& msg) {
  type_assert::IsConvertibleTo<std::string, StringT>();
  return Status(error::IO_ERROR, std::forward<StringT>(msg));
}

} // namespace util

#endif // UTIL_STATUS_H_