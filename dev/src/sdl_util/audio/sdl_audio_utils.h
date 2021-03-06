#ifndef SDL_AUDIO_UTILS_
#define SDL_AUDIO_UTILS_

#include <string_view>

#include "status.h"

namespace sdl {
namespace util {

::util::Status WavToBRR(std::string_view source_file, 
    std::string_view dest_file, bool opt_for_resynth);

} // namespace util
} // namespace sdl

#endif // SDL_AUDIO_UTILS_