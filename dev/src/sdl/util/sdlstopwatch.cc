#include "util/stopwatch.h"

#include <SDL.h>

namespace util {

SDLStopwatch::SDLStopwatch() : last_ticks_(0) {
  (void) Lap();
}

double SDLStopwatch::Lap() {
  const uint64_t current_ticks = SDL_GetPerformanceCounter();
  const uint64_t elapsed_ticks = current_ticks - last_ticks_;
  last_ticks_ = current_ticks;

  return elapsed_ticks * Precision();
}

double SDLStopwatch::Precision() {
  return 1.0 / static_cast<double>(SDL_GetPerformanceFrequency());
}

} // namespace util