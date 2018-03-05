#ifndef RETRO_FBIMG_H_

#include <memory>
#include <string>

#include "SDL.h"
#include "util/deleterptr.h"
#include "glm/vec2.hpp"
#include "retro/fbcore.h"

// Rewrite so that textures loaded from a file are static, but ones created on
// the fly can be render targets. Also create method for testing the render-ability
// of an image.


namespace retro {

class FbGfx;
// Fixed size 32bit image class, basically a wrapper around SDL_Texture and an
// image loading library.
class FbImg {
  friend class FbGfx;

 public:
  // Load an image from a file.
  static std::unique_ptr<FbImg> FromFile(const std::string& filename);
  // Create an image of the provided dimensions filled with a color constant.
  static std::unique_ptr<FbImg> OfSize(glm::ivec2 dimensions,
                                       FbColor32 fill_color = 0);

  int width() const { return w_; }
  int height() const { return h_; }

 private:
  typedef unsigned char StbImageData;
  FbImg(util::deleter_ptr<SDL_Texture> texture, int w, int h);

  virtual ~FbImg() {}

  static util::deleter_ptr<SDL_Texture> TextureFromSurface(
      SDL_Surface* surface);

  const util::deleter_ptr<SDL_Texture> texture_;
  const int w_;
  const int h_;
};

}  // namespace retro

#endif  // RETRO_FBIMG_H_
