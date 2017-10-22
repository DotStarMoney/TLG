#ifndef UTIL_STATUS_MACROS_H_
#define UTIL_STATUS_MACROS_H_

#include "status.h"
#include "statusor.h"

// Used for functions returning util::Status:
//   If the function would return a not-ok Status, return that status. 
#define RETURN_IF_ERROR(FUNC_) do { \
util::Status status = FUNC_; \
if (!status.ok()) return status; } while (0)


#define __ASSIGN_OR_RETURN_CONCAT(a, b) a ## b
#define __ASSIGN_OR_RETURN_UNIQUEVAR(VAR_) \
__ASSIGN_OR_RETURN_CONCAT(VAR_, __LINE__)
// Used for functions returning util::StatusOr:
//   If the function would return a not-ok StatusOr, return the status.
//   Otherwise, assign the value from StatusOr to ASSIGNEE_. 
#define ASSIGN_OR_RETURN(ASSIGNEE_, FUNC_) \
auto __ASSIGN_OR_RETURN_UNIQUEVAR(ASSIGN_OR_RETURN_TMP) = FUNC_; \
if (!__ASSIGN_OR_RETURN_UNIQUEVAR(ASSIGN_OR_RETURN_TMP).ok()) { \
  return __ASSIGN_OR_RETURN_UNIQUEVAR(ASSIGN_OR_RETURN_TMP).status(); \
} \
ASSIGNEE_ = __ASSIGN_OR_RETURN_UNIQUEVAR(ASSIGN_OR_RETURN_TMP).ConsumeValue();

#endif // UTIL_STATUS_MACROS_H_