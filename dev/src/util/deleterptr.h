#ifndef UTIL_DELETERPTR_H_
#define UTIL_DELETERPTR_H_

#include <functional>
#include <memory>

// A convenience alias for creating unique_ptr's with cutom deleters.
// Use it like this:
//
// deleter_ptr<FILE> file(fopen("file.txt", "r"), [](FILE* f){ fclose(f); });
//
//
namespace util {
template <class T>
using deleter_ptr = std::unique_ptr<T, std::function<void(T*)>>;
}  // namespace util

#endif  // UTIL_DELETERPTR_H_
