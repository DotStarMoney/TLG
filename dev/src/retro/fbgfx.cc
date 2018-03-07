#include "retro/fbgfx.h"

using absl::string_view;
using glm::ivec2;
using std::string;
using util::deleter_ptr;

namespace retro {
FbGfx::Cleanup FbGfx::cleanup_;
deleter_ptr<SDL_Window> FbGfx::window_ = nullptr;
deleter_ptr<SDL_Renderer> FbGfx::renderer_ = nullptr;

void FbGfx::Screen(ivec2 res, bool fullscreen, const string& title,
                   ivec2 physical_res) {
  CHECK(!is_init()) << "Cannot initialize FbGfx more than once.";

  sdl_util::Cleanup::RegisterModule();
  SDL_Init(SDL_INIT_VIDEO);

  if ((physical_res.x == -1) || (physical_res.y == -1)) physical_res = res;
  deleter_ptr<SDL_Window> window(
      SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, physical_res.x, physical_res.y,
                       fullscreen ? SDL_WINDOW_FULLSCREEN : 0),
      [](SDL_Window* w) { SDL_DestroyWindow(w); });
  CHECK_NE(window.get(), nullptr)
      << "SDL error (SDL_CreateWindow): " << SDL_GetError();
  deleter_ptr<SDL_Renderer> renderer(
      SDL_CreateRenderer(window.get(), -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC |
                             SDL_RENDERER_TARGETTEXTURE),
      [](SDL_Renderer* r) { SDL_DestroyRenderer(r); });
  CHECK_NE(renderer.get(), nullptr)
      << "SDL error (SDL_CreateRenderer): " << SDL_GetError();

  CHECK_EQ(SDL_RenderSetLogicalSize(renderer.get(), res.x, res.y), 0)
      << "SDL error (SDL_RenderSetLogicalSize): " << SDL_GetError();

  CHECK_EQ(SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND), 0)
      << "SDL error (SDL_SetRenderDrawBlendMode): " << SDL_GetError();

  // Start off with a fresh, black window.
  Cls();
  Flip();
}

void FbGfx::SetRenderTarget(SDL_Texture* target) {
  CHECK_EQ(SDL_SetRenderTarget(renderer_.get(), target), 0)
      << "SDL error (SDL_SetRenderTarget): " << SDL_GetError();
}

void FbGfx::SetRenderColor(FbColor32 col) {
  CHECK_EQ(SDL_SetRenderDrawColor(renderer_.get(), col.channel.r, col.channel.g,
                                  col.channel.b, col.channel.a),
           0)
      << "SDL error (SDL_SetRenderDrawColor): " << SDL_GetError();
}

bool FbGfx::IsFullscreen() {
  CheckInit(__func__);
  return SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_FULLSCREEN;
}

void FbGfx::SetFullscreen(bool fullscreen) {
  CheckInit(__func__);
  CHECK_EQ(SDL_SetWindowFullscreen(window_.get(),
                                   fullscreen ? SDL_WINDOW_FULLSCREEN : 0),
           0)
      << "SDL error (SDL_SetWindowFullscreen): " << SDL_GetError();
}

ivec2 FbGfx::GetResolution() {
  CheckInit("GetResolution");
  ivec2 res;
  SDL_GetWindowSize(window_.get(), &res.x, &res.y);
  return res;
}

void FbGfx::Flip() { SDL_RenderPresent(renderer_.get()); }

// Cls

void FbGfx::Cls(const FbImg& target, FbColor32 col) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalCls(target.texture_.get(), col);
}
void FbGfx::Cls(FbColor32 col) {
  CheckInit(__func__);
  InternalCls(nullptr, col);
}
void FbGfx::InternalCls(SDL_Texture* texture, FbColor32 col) {
  SetRenderTarget(texture);
  SetRenderColor(col);
  CHECK_EQ(SDL_RenderClear(renderer_.get()), 0)
      << "SDL error (SDL_RenderClear): " << SDL_GetError();
}

// Line

void FbGfx::Line(ivec2 a, ivec2 b, FbColor32 color) {
  CheckInit(__func__);
  InternalLine(nullptr, a, b, color);
}
void FbGfx::Line(const FbImg& target, ivec2 a, ivec2 b, FbColor32 color) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalLine(target.texture_.get(), a, b, color);
}
void FbGfx::InternalLine(SDL_Texture* texture, ivec2 a, ivec2 b,
                         FbColor32 color) {
  SetRenderTarget(texture);
  SetRenderColor(color);
  CHECK_EQ(SDL_RenderDrawLine(renderer_.get(), a.x, a.y, b.x, b.y), 0)
      << "SDL error (SDL_RenderDrawLine): " << SDL_GetError();
}

// Rect

void FbGfx::Rect(ivec2 a, ivec2 b, FbColor32 color) {
  CheckInit(__func__);
  InternalRect(nullptr, a, b, color);
}
void FbGfx::Rect(const FbImg& target, ivec2 a, ivec2 b, FbColor32 color) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalRect(target.texture_.get(), a, b, color);
}
void FbGfx::InternalRect(SDL_Texture* texture, ivec2 a, ivec2 b,
                         FbColor32 color) {
  SetRenderTarget(texture);
  SetRenderColor(color);
  SDL_Rect rect{a.x, a.y, b.x, b.y};
  CHECK_EQ(SDL_RenderDrawRect(renderer_.get(), &rect), 0)
      << "SDL error (SDL_RenderDrawRect): " << SDL_GetError();
}

// FillRect

void FbGfx::FillRect(ivec2 a, ivec2 b, FbColor32 color) {
  CheckInit(__func__);
  InternalFillRect(nullptr, a, b, color);
}
void FbGfx::FillRect(const FbImg& target, ivec2 a, ivec2 b, FbColor32 color) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalFillRect(target.texture_.get(), a, b, color);
}
void FbGfx::InternalFillRect(SDL_Texture* texture, ivec2 a, ivec2 b,
                             FbColor32 color) {
  SetRenderTarget(texture);
  SetRenderColor(color);
  SDL_Rect rect{a.x, a.y, b.x, b.y};
  CHECK_EQ(SDL_RenderFillRect(renderer_.get(), &rect), 0)
      << "SDL error (SDL_RenderFillRect): " << SDL_GetError();
}

}  // namespace retro
