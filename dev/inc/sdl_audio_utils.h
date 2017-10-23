#ifndef SDL_AUDIO_UTILS_
#define SDL_AUDIO_UTILS_

#include <string_view>

#include "status.h"

namespace sdl {
namespace util {

::util::Status WavToBRR(std::string_view source_file, 
    std::string_view dest_file, uint32_t loop_start = 0, uint32_t loop_end = 0,
    uint16_t attack = 0, uint16_t decay = 0, uint8_t sustain = 0,
    uint16_t release = 0);

} // namespace util
} // namespace sdl

#endif // SDL_AUDIO_UTILS_