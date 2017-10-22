#ifndef BASE_ASSERT_H_
#define BASE_ASSERT_H_

#include "abort.h"
#include "strcat.h"

#define ASSERT(_X_) \
if (!(_X_)) base::abort("ASSERT failed.");

#define ASSERT_FALSE(_X_) \
if (_X_) base::abort("ASSERT_FALSE failed.");

#define ASSERT_EQ(_X_, _Y_) \
if ((_X_) != (_Y_)) { \
  base::abort(util::StrCat("ASSERT_EQ failed: ", (_X_), " != ", (_Y_))); \
}
#define ASSERT_NE(_X_, _Y_) \
if ((_X_) == (_Y_)) { \
  base::abort(util::StrCat("ASSERT_NE failed: ", (_X_), " == ", (_Y_))); \
}
#define ASSERT_GT(_X_, _Y_) \
if ((_X_) <= (_Y_)) { \
  base::abort(util::StrCat("ASSERT_GT failed: ", (_X_), " <= ", (_Y_))); \
}
#define ASSERT_LT(_X_, _Y_) \
if ((_X_) >= (_Y_)) { \
  base::abort(util::StrCat("ASSERT_LT failed: ", (_X_), " >= ", (_Y_))); \
}
#define ASSERT_GE(_X_, _Y_) \
if ((_X_) < (_Y_)) { \
  base::abort(util::StrCat("ASSERT_GE failed: ", (_X_), " < ", (_Y_))); \
}
#define ASSERT_LE(_X_, _Y_) \
if ((_X_) > (_Y_)) { \
  base::abort(util::StrCat("ASSERT_LE failed: ", (_X_), " > ", (_Y_))); \
}

#endif // BASE_ASSERT_H_
