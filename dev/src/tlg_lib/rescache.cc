#include "tlg_lib/rescache.h"

#include <string>
#include <string_view>

namespace tlg_lib {
thread_local std::string_view ResCache::relative_path_;

std::string_view ResCache::ExtractRelativePath(const std::string& uri) {
  size_t path_end_i = uri.find_last_of('/');
  return path_end_i != std::string::npos
             ? std::string_view(uri).substr(0, path_end_i + 1)
             : "";
}

}  // namespace tlg_lib
