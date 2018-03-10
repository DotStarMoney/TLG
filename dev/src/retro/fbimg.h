#ifndef RETRO_FBIMG_H_
#define RETRO_FBIMG_H_

#include <memory>
#include <string>

#include "SDL.h"
#include "absl/strings/string_view.h"
#include "glm/vec2.hpp"
#include "glog/logging.h"
#include "retro/fbcore.h"
#include "util/deleterptr.h"

namespace retro {

class FbGfx;
// Fixed size 32bit image class, basically a wrapper around SDL_Texture and an
// image loading library.
class FbImg {
  friend class FbGfx;

 public:
  virtual ~FbImg() {}

  // Load an image from a file.
  static std::unique_ptr<FbImg> FromFile(const std::string& filename);
  // Create an image of the provided dimensions filled with a color constant.
  static std::unique_ptr<FbImg> OfSize(glm::ivec2 dimensions,
                                       FbColor32 fill_color = 0);

  int width() const { return w_; }
  int height() const { return h_; }
  bool is_render_target() const { return is_target_; }

 private:
  typedef unsigned char StbImageData;
  FbImg(util::deleter_ptr<SDL_Texture> texture, int w, int h, bool is_target);


  static util::deleter_ptr<SDL_Texture> TextureFromSurface(
      SDL_Surface* surface);

  void CheckTarget(absl::string_view meth_name) const {
    CHECK(is_target_) << "Image cannot be the target of drawing operation "
                      << meth_name << ".";
  }

  const util::deleter_ptr<SDL_Texture> texture_;
  const int w_;
  const int h_;
  const bool is_target_;
};

}  // namespace retro

#endif  // RETRO_FBIMG_H_
