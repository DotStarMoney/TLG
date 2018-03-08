#include "retro/fbgfx.h"

using absl::string_view;
using glm::ivec2;
using std::string;
using std::unique_ptr;
using util::deleter_ptr;

namespace retro {
FbGfx::Cleanup FbGfx::cleanup_;
deleter_ptr<SDL_Window> FbGfx::window_ = nullptr;
deleter_ptr<SDL_Renderer> FbGfx::renderer_ = nullptr;
unique_ptr<FbImg> FbGfx::basic_font_ = nullptr;

void FbGfx::Screen(ivec2 res, bool fullscreen, const string& title,
                   ivec2 physical_res) {
  CHECK(!is_init()) << "Cannot initialize FbGfx more than once.";

  sdl_util::Cleanup::RegisterModule();
  SDL_Init(SDL_INIT_VIDEO);

  if ((physical_res.x == -1) || (physical_res.y == -1)) physical_res = res;
  // We open the window initially hidden (and then reveal it once all of this
  // setup is out of the way)
  deleter_ptr<SDL_Window> window(
      SDL_CreateWindow(
          title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
          physical_res.x, physical_res.y,
          (fullscreen ? SDL_WINDOW_FULLSCREEN : 0) | SDL_WINDOW_HIDDEN),
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

  // Load the system font
  basic_font_ = FbImg::FromFile(kSystemFontPath);

  // Start off with a fresh black window;
  Cls();
  Flip();

  // Reveal our window, all ready to go!
  SDL_ShowWindow(window.get());
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

// Put & PutEx

void FbGfx::Put(const FbImg& src, ivec2 p, ivec2 src_a, ivec2 src_b) {
  CheckInit(__func__);
  InternalPut(nullptr, src.texture_.get(), {src.width, src.height}, p,
              PutOptions(), src_a, src_b);
}
void FbGfx::Put(const FbImg& target, const FbImg& src, ivec2 p, ivec2 src_a,
                ivec2 src_b) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalPut(target.texture_.get(), src.texture_.get(),
              {src.width, src.height}, p, PutOptions(), src_a, src_b);
}

void FbGfx::PutEx(const FbImg& src, ivec2 p, PutOptions opts, ivec2 src_a,
                  ivec2 src_b) {
  CheckInit(__func__);
  InternalPut(nullptr, src.texture_.get(), {src.width, src.height}, p, opts,
              src_a, src_b);
}
void FbGfx::PutEx(const FbImg& target, const FbImg& src, ivec2 p,
                  PutOptions opts, ivec2 src_a, ivec2 src_b) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalPut(target.texture_.get(), src.texture_.get(),
              {src.width, src.height}, p, opts, src_a, src_b);
}

inline SDL_BlendMode GetSdlBlendMode(FbGfx::PutOptions::BlendMode m) {
  switch (m) {
    case FbGfx::PutOptions::BLEND_NONE:
      return SDL_BLENDMODE_NONE;
    case FbGfx::PutOptions::BLEND_ALPHA:
      return SDL_BLENDMODE_BLEND;
    case FbGfx::PutOptions::BLEND_ADD:
      return SDL_BLENDMODE_ADD;
    case FbGfx::PutOptions::BLEND_MOD:
      return SDL_BLENDMODE_MOD;
    default:
      CHECK(false) << "Not a real blend mode: " << m;
  }
}

void FbGfx::InternalPut(SDL_Texture* dest, SDL_Texture* src, ivec2 src_dims,
                        ivec2 p, PutOptions opts, ivec2 src_a, ivec2 src_b) {
  SetRenderTarget(dest);
  CHECK_EQ(SDL_SetTextureBlendMode(src, GetSdlBlendMode(opts.blend)), 0)
      << "SDL error (SDL_SetTextureBlendMode): " << SDL_GetError();
  CHECK_EQ(SDL_SetTextureColorMod(src, opts.mod.channel.r, opts.mod.channel.g,
                                  opts.mod.channel.b),
           0)
      << "SDL error (SDL_SetTextureColorMod): " << SDL_GetError();
  CHECK_EQ(SDL_SetTextureAlphaMod(src, opts.mod.channel.a), 0)
      << "SDL error (SDL_SetTextureAlphaMod): " << SDL_GetError();

  SDL_Rect dst_rect;
  SDL_Rect src_rect;

  dst_rect.x = p.x;
  dst_rect.y = p.y;

  SDL_Rect* src_rect_target = &src_rect;
  if ((src_a.x == -1) || (src_a.y == -1) || (src_b.x == -1) ||
      (src_b.y == -1)) {
    src_rect_target == nullptr;
    dst_rect.w = src_dims.x;
    dst_rect.h = src_dims.y;
  } else {
    src_rect.x = src_a.x;
    src_rect.y = src_a.y;
    src_rect.w = src_b.x - src_a.x + 1;
    src_rect.h = src_b.y - src_a.y + 1;
    dst_rect.w = src_rect.w;
    dst_rect.h = src_rect.h;
  }

  CHECK_EQ(SDL_RenderCopy(renderer_.get(), src, src_rect_target, &dst_rect), 0)
      << "SDL error (SDL_RenderCopy): " << SDL_GetError();
}

// TextLine

}  // namespace retro
