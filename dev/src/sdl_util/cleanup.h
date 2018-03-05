#ifndef SDL_UTIL_CLEANUP_H_
#define SDL_UTIL_CLEANUP_H_

#include "SDL.h"
#include "glog/logging.h"

// Forward declarations of classes that can use Cleanup
namespace retro {
class FbGfx;
}

namespace sdl_util {
class Cleanup {
  friend class retro::FbGfx;
  static int remaining_modules_;
  static void RegisterModule() { ++remaining_modules_; }
  static void UnregisterModule() {
    if (--remaining_modules_ == 0) {
      SDL_Quit();
    }
  }
  Cleanup() {
    CHECK(false) << "An instance of Cleanup should not be constructed.";
  }
};
}  // namespace sdl_util

#endif  // SDL_UTIL_CLEANUP_H_
