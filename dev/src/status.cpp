#include "status.h"

#include <iostream>

#include "cannonical_errors.h"

namespace util {

Status::~Status() {
  MaybeDereference(handle_);
}

Status::Status() : handle_(nullptr) {}

Status::Status(const Status& status) : handle_(status.handle_) {
  MaybeReference(handle_);
}

Status& Status::operator=(const Status& status) {
  if (status.handle_ != handle_) {
    MaybeReference(status.handle_);
    MaybeDereference(handle_);
    handle_ = status.handle_;
  }
  return *this;
}

Status::Status(Status&& status) : handle_(status.handle_) {
  status.handle_ = nullptr;
}

Status& Status::operator=(Status&& status) {
  MaybeDereference(handle_);
  handle_ = status.handle_;
  status.handle_ = nullptr;
  return *this;
}

bool Status::ok() const { return handle_ == nullptr; }

std::string_view Status::message() const { 
  if (handle_ == nullptr) return "";
  return handle_->message;
}
error::CannonicalErrors Status::cannonical_error_code() const {
  return handle_ == nullptr ? error::UNKNOWN : handle_->cannonical_error_code;
}

void Status::MaybeDereference(Payload* handle_) {
  if(handle_ == nullptr) return;
  // Since loading the ref count should be a singular "mov," we can hopefully
  // avoid the atomic fetch_sub by doing a check == 1 first.
  if ((handle_->refs.load() == 1) || (handle_->refs.fetch_sub(1) == 0)) {
    delete handle_;
  }
}
void Status::MaybeReference(Payload* handle_) {
  if (handle_ == nullptr) return;
  handle_->refs++;
}
bool Status::SlowComparePayloadsForEquality(Payload* lhs, Payload* rhs) {
  if ((lhs == nullptr) || (rhs == nullptr)) return false;
  if (lhs->cannonical_error_code != rhs->cannonical_error_code) return false;
  if (lhs->message.compare(rhs->message) != 0) return false;
  return true;
}

// First try and check pointers, if no luck we have to compare them the long 
// way.
bool operator==(const Status& lhs, const Status& rhs) {
  if ((lhs.handle_ == rhs.handle_) ||
      Status::SlowComparePayloadsForEquality(lhs.handle_, rhs.handle_)) {
    return true;
  }
  return false;
}
bool operator!=(const Status& lhs, const Status& rhs) {
  return !(lhs == rhs);
}

const Status OkStatus;

} // namespace util

