#include "retro/fbgfx.h"

using util::deleter_ptr;

namespace retro {
FbGfx::Cleanup FbGfx::cleanup_;
deleter_ptr<SDL_Window> FbGfx::window_ = nullptr;
deleter_ptr<SDL_Renderer> FbGfx::renderer_ = nullptr;

bool FbGfx::is_init() { return window_.get() != nullptr; }

void FbGfx::InitGfx() {
  CHECK(!is_init()) << "Cannot initialize FbGfx more than once.";
  sdl_util::Cleanup::RegisterModule();
  SDL_Init(SDL_INIT_VIDEO);

  // open window to some initial size, then create renderer etc...
}

}  // namespace retro
