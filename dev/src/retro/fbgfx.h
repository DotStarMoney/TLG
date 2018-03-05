#ifndef RETRO_FBGFX_H_

#include "SDL.h"
#include "absl/strings/string_view.h"
#include "glm/vec2.hpp"
#include "glog/logging.h"
#include "retro/fbimg.h"
#include "sdl_util/cleanup.h"
#include "util/deleterptr.h"

namespace retro {

class FbImg;
class FbGfx final {
  friend class FbImg;
 public:
  // Call only once
  static void Screen(glm::ivec2 res, bool fullscreen = false,
                     glm::ivec2 logical_res = {0, 0});

  static glm::ivec2 GetResolution();

  static glm::ivec2 GetLogicalResolution();
  static void SetLogicalResolution(glm::ivec2 res);

  static bool IsFullscreen();
  static void SetFullscreen(bool fullscreen);

  // Includes vsync delay
  static void Flip();

  // All with variation for rendering to texture instead of the screen
  // Line
  // Rect
  // TextLine
  // TextParagraph
  // Put

 private:
  FbGfx() { CHECK(false) << "An instance of FbGfx should not be constructed."; }
  static void CheckInit(absl::string_view meth_name) {
    CHECK(IsInit()) << "Cannot call " << meth_name << " before FbGfx::Screen.";
  }
  static void InitGfx();
  static bool IsInit();
  static util::deleter_ptr<SDL_Window> window_;
  static util::deleter_ptr<SDL_Renderer> renderer_;

  struct Cleanup {
    ~Cleanup() { sdl_util::Cleanup::UnregisterModule(); }
  };
  static Cleanup cleanup_;
};

}  // namespace retro

#endif  // RETRO_FBGFX_H_
