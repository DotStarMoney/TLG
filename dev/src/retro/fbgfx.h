#ifndef RETRO_FBGFX_H_
#define RETRO_FBGFX_H_

#include "SDL.h"
#include "absl/strings/string_view.h"
#include "glm/vec2.hpp"
#include "glog/logging.h"
#include "retro/fbcore.h"
#include "sdl_util/cleanup.h"
#include "util/deleterptr.h"

// Single context, micro graphics library to mimic the venerable fbgfx.bi of
// FreeBASIC.

namespace retro {

constexpr char kSystemFontPath[] = "res/system_font_.png";

class FbImg;
class FbGfx final {
  friend class FbImg;

 public:
  // Must be called to use graphics functionality, can only be called once.
  // Resolution is the physical resolution of the drawing area whereas the
  // logical resolution is the resolution at which the pixels are displayed.
  static void Screen(glm::ivec2 res, bool fullscreen = false,
                     const std::string& title = "FB Gfx",
                     glm::ivec2 physical_res = {-1, -1});

  // Clear the screen (optionally to a color)
  static void Cls(FbColor32 col = FbColor32::BLACK);
  static void Cls(const FbImg& target, FbColor32 col = FbColor32::BLACK);

  static glm::ivec2 GetResolution();

  static bool IsFullscreen();
  static void SetFullscreen(bool fullscreen);

  // Updates the screen after waiting for vsync, clobbering the back buffer
  // in the process (be sure to Cls if you don't plan on overwriting the whole
  // backbuffer)
  static void Flip();

  static void Line(glm::ivec2 a, glm::ivec2 b,
                   FbColor32 color = FbColor32::WHITE);
  static void Line(const FbImg& target, glm::ivec2 a, glm::ivec2 b,
                   FbColor32 color = FbColor32::WHITE);

  static void Rect(glm::ivec2 a, glm::ivec2 b,
                   FbColor32 color = FbColor32::WHITE);
  static void Rect(const FbImg& target, glm::ivec2 a, glm::ivec2 b,
                   FbColor32 color = FbColor32::WHITE);

  static void FillRect(glm::ivec2 a, glm::ivec2 b,
                       FbColor32 color = FbColor32::WHITE);
  static void FillRect(const FbImg& target, glm::ivec2 a, glm::ivec2 b,
                       FbColor32 color = FbColor32::WHITE);

  enum TextHAlign {
    TEXT_ALIGN_H_LEFT = 0,
    TEXT_ALIGN_H_CENTER = 1,
    TEXT_ALIGN_H_RIGHT = 2
  };
  enum TextVAlign {
    TEXT_ALIGN_V_TOP = 0,
    TEXT_ALIGN_V_CENTER = 1,
    TEXT_ALIGN_V_BOTTOM = 2
  };
  static void TextLine(absl::string_view text, glm::ivec2 p,
                       FbColor32 color = FbColor32::WHITE,
                       TextHAlign h_align = TEXT_ALIGN_H_LEFT,
                       TextVAlign v_align = TEXT_ALIGN_V_TOP);
  static void TextLine(const FbImg& target, absl::string_view text,
                       glm::ivec2 p, FbColor32 color = FbColor32::WHITE,
                       TextHAlign h_align = TEXT_ALIGN_H_LEFT,
                       TextVAlign v_align = TEXT_ALIGN_V_TOP);

  static void TextParagraph(absl::string_view text, glm::ivec2 a, glm::ivec2 b,
                            FbColor32 color = FbColor32::WHITE,
                            TextHAlign h_align = TEXT_ALIGN_H_LEFT,
                            TextVAlign v_align = TEXT_ALIGN_V_TOP);
  static void TextParagraph(const FbImg& target, absl::string_view text,
                            glm::ivec2 a, glm::ivec2 b,
                            FbColor32 color = FbColor32::WHITE,
                            TextHAlign h_align = TEXT_ALIGN_H_LEFT,
                            TextVAlign v_align = TEXT_ALIGN_V_TOP);

  static void Put(const FbImg& src, glm::ivec2 p, glm::ivec2 src_a = {-1, -1},
                  glm::ivec2 src_b = {-1, -1});
  static void Put(const FbImg& target, const FbImg& src, glm::ivec2 p,
                  glm::ivec2 src_a = {-1, -1}, glm::ivec2 src_b = {-1, -1});

  struct PutOptions {
   public:
    enum BlendMode { BLEND_NONE, BLEND_ALPHA, BLEND_ADD, BLEND_MOD };
    BlendMode blend = BLEND_ALPHA;
    FbColor32 mod = FbColor32::WHITE;
    PutOptions& SetBlend(BlendMode blend) {
      this->blend = blend;
      return *this;
    }
    PutOptions& SetMod(FbColor32 mod) {
      this->mod = mod;
      return *this;
    }
  };
  static void PutEx(const FbImg& src, glm::ivec2 p, PutOptions opts,
                    glm::ivec2 src_a = {-1, -1}, glm::ivec2 src_b = {-1, -1});
  static void PutEx(const FbImg& target, const FbImg& src, glm::ivec2 p,
                    PutOptions opts, glm::ivec2 src_a = {-1, -1},
                    glm::ivec2 src_b = {-1, -1});

 private:
  FbGfx() { CHECK(false) << "An instance of FbGfx should not be constructed."; }
  static void CheckInit(absl::string_view meth_name) {
    CHECK(is_init()) << "Cannot call " << meth_name << " before FbGfx::Screen.";
  }

  static void PrepareFont();

  // Using SetRender* methods assumes that CheckInit has already been called.
  static void SetRenderTarget(SDL_Texture* target);
  static void SetRenderColor(FbColor32 col);

  static void InternalCls(SDL_Texture* texture, FbColor32 col);
  static void InternalLine(SDL_Texture* texture, glm::ivec2 a, glm::ivec2 b,
                           FbColor32 color);
  static void InternalRect(SDL_Texture* texture, glm::ivec2 a, glm::ivec2 b,
                           FbColor32 color);
  static void InternalFillRect(SDL_Texture* texture, glm::ivec2 a, glm::ivec2 b,
                               FbColor32 color);
  static void InternalPut(SDL_Texture* dest, SDL_Texture* src,
                          glm::ivec2 src_dims, glm::ivec2 p, PutOptions opts,
                          glm::ivec2 src_a, glm::ivec2 src_b);
  static void InternalTextLine(SDL_Texture* texture, absl::string_view text,
                               glm::ivec2 p, FbColor32 color,
                               TextHAlign h_align, TextVAlign v_align);
  static void InternalTextParagraph(SDL_Texture* texture,
                                    absl::string_view text, glm::ivec2 a,
                                    glm::ivec2 b, FbColor32 color,
                                    TextHAlign h_align, TextVAlign v_align);

  static bool is_init() { return window_.get() != nullptr; }
  static util::deleter_ptr<SDL_Window> window_;
  static util::deleter_ptr<SDL_Renderer> renderer_;

  static std::unique_ptr<FbImg> basic_font_;

  struct Cleanup {
    ~Cleanup() { sdl_util::Cleanup::UnregisterModule(); }
  };
  static Cleanup cleanup_;
};

}  // namespace retro

#endif  // RETRO_FBGFX_H_
