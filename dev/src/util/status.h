#ifndef UTIL_STATUS_H_
#define UTIL_STATUS_H_

#include <atomic>

#include "absl/strings/string_view.h"
#include "base/static_type_assert.h"
#include "glog/logging.h"
#include "util/cannonical_errors.h"

namespace util {

class Status final {
 public:
  // Default status object is an OK status.
  Status();
  // err_code must not be UNKNOWN and msg must not be "".
  template <class STRING_T>
  Status(error::CannonicalErrors err_code, STRING_T&& msg) {
    base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
    CHECK_NE(err_code, error::UNKNOWN) << "Error type must not be unknown.";
    handle_ = new Payload(err_code, std::forward<STRING_T>(msg));
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
  absl::string_view message() const;
  error::CannonicalErrors cannonical_error_code() const;
  Status& status() { return *this; }

  std::string ToString() const;

  friend bool operator==(const Status& lhs, const Status& rhs);
  friend bool operator!=(const Status& lhs, const Status& rhs);

 private:
  struct Payload {
    // Create a new payload with an automatic reference count of 1.
    template <class STRING_T>
    Payload(error::CannonicalErrors err_code, STRING_T&& msg)
        : cannonical_error_code(err_code),
          message(std::forward<STRING_T>(msg)),
          refs(1) {
      base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
      CHECK(!message.empty()) << "Message must not be empty.";
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

template <class STRING_T>
Status FailedPreconditionError(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::FAILED_PRECONDITION, std::forward<STRING_T>(msg));
}

template <class STRING_T>
Status InvalidArgumentError(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::INVALID_ARGUMENT, std::forward<STRING_T>(msg));
}

template <class STRING_T>
Status TimeoutError(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::TIMEOUT, std::forward<STRING_T>(msg));
}

template <class STRING_T>
Status OutOfMemoryError(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::OUT_OF_MEMORY, std::forward<STRING_T>(msg));
}

template <class STRING_T>
Status OutOfBoundsError(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::OUT_OF_BOUNDS, std::forward<STRING_T>(msg));
}

template <class STRING_T>
Status LogicError(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::LOGIC_ERROR, std::forward<STRING_T>(msg));
}

template <class STRING_T>
Status ResourceUnobtainable(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::RESOURCE_UNOBTAINABLE, std::forward<STRING_T>(msg));
}

template <class STRING_T>
Status UnimplementedError(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::UNIMPLEMENTED_ERROR, std::forward<STRING_T>(msg));
}

template <class STRING_T>
Status FormatMismatchError(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::FORMAT_MISTMATCH, std::forward<STRING_T>(msg));
}

template <class STRING_T>
Status IOError(STRING_T&& msg) {
  base::type_assert::AssertIsConvertibleTo<std::string, STRING_T>();
  return Status(error::IO_ERROR, std::forward<STRING_T>(msg));
}

}  // namespace util

#endif  // UTIL_STATUS_H_
