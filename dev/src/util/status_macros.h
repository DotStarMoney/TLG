#ifndef UTIL_STATUS_MACROS_H_
#define UTIL_STATUS_MACROS_H_

#include "util/status.h"
#include "util/statusor.h"

// Used for functions returning util::Status:
//   If the function would return a not-ok Status, return that status.
#define RETURN_IF_ERROR(func)                 \
  do {                                        \
    auto status = func;                       \
    if (!status.ok()) return status.status(); \
  } while (0)

#define __ASSIGN_OR_RETURN_CAT1(a, b) a##b
#define __ASSIGN_OR_RETURN_CAT2(a, b) __ASSIGN_OR_RETURN_CAT1(a, b)
#define __ASSIGN_OR_RETURN_UNIQUEVAR(x) __ASSIGN_OR_RETURN_CAT2(x, __LINE__)
// Used for functions returning util::StatusOr:
//   If the function would return a not-ok StatusOr, return the status.
//   Otherwise, assign the value from StatusOr to ASSIGNEE_.
#define ASSIGN_OR_RETURN(assignee, func)                                \
  auto __ASSIGN_OR_RETURN_UNIQUEVAR(ASSIGN_OR_RETURN_TMP) = func;       \
  if (!__ASSIGN_OR_RETURN_UNIQUEVAR(ASSIGN_OR_RETURN_TMP).ok()) {       \
    return __ASSIGN_OR_RETURN_UNIQUEVAR(ASSIGN_OR_RETURN_TMP).status(); \
  }                                                                     \
  assignee = __ASSIGN_OR_RETURN_UNIQUEVAR(ASSIGN_OR_RETURN_TMP).ConsumeValue();

#endif  // UTIL_STATUS_MACROS_H_
