#ifndef RETRO_FBGFX_H_

#include "SDL.h"
#include "absl/strings/string_view.h"
#include "glm/vec2.hpp"
#include "glog/logging.h"
#include "retro/fbimg.h"
#include "sdl_util/cleanup.h"
#include "util/deleterptr.h"

// Single context, micro graphics library to mimic the venerable fbgfx.bi of
// FreeBASIC.

namespace retro {

class FbImg;
class FbGfx final {
  friend class FbImg;

 public:
  // Must be called to use graphics functionality, can can only be called once.
  // Resolution is the physical resolution of the drawing area, whereas the
  // logical resolution is the resolution at which the pixels are displayed.
  static void Screen(glm::ivec2 res, bool fullscreen = false,
                     glm::ivec2 logical_res = {-1, -1});

  static glm::ivec2 GetResolution();

  static glm::ivec2 GetLogicalResolution();
  static void SetLogicalResolution(glm::ivec2 res);

  static bool IsFullscreen();
  static void SetFullscreen(bool fullscreen);

  // Includes vsync delay
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
    enum BlendMode { BLEND_NONE, BLEND_ALPHA, BLEND_ADD };
    BlendMode blend_ = BLEND_NONE;
    FbColor32 mod_ = FbColor32::WHITE;
    PutOptions& SetBlend(BlendMode blend) {
      blend_ = blend;
      return *this;
    }
    PutOptions& SetMod(FbColor32 mod) {
      mod_ = mod;
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
  static void InitGfx();
  static bool is_init();
  static util::deleter_ptr<SDL_Window> window_;
  static util::deleter_ptr<SDL_Renderer> renderer_;

  struct Cleanup {
    ~Cleanup() { sdl_util::Cleanup::UnregisterModule(); }
  };  // namespace retro
  static Cleanup cleanup_;
};

}  // namespace retro

#endif  // RETRO_FBGFX_H_
