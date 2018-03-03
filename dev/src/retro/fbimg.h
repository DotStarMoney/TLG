#ifndef RETRO_FBIMG_H_

#include <memory>
#include <string>

#include "SDL_surface.h"
#include "deleterptr.h"
#include "glm/vec2.hpp"
#include "retro/fbcore.h"

namespace retro {

class FbGfx;
// Fixed size 32bit image class, basically a wrapper around SDL_Surface and the
// image loading library.
class FbImg {
  friend class FbGfx;

 public:
  // Load an image from a file.
  static std::unique_ptr<FbImg> FromFile(const std::string& filename);
  // Create an image of the provided dimensions filled with a color constant.
  static std::unique_ptr<FbImg> OfSize(glm::ivec2 dimensions,
                                       FbColor32 fill_color = 0);

  int width() const { return surface_->w; }
  int height() const { return surface_->h; }

 private:
  typedef unsigned char StbImageData;
  FbImg(util::deleter_ptr<SDL_Surface> surface,
        util::deleter_ptr<StbImageData> image_data);
  virtual ~FbImg() {}

  // Required when an image is loaded using stb_image and freeing the surface
  // doesn't also free the pixel data.
  util::deleter_ptr<StbImageData> image_data_;
  util::deleter_ptr<SDL_Surface> surface_;
};

}  // namespace retro

#endif  // RETRO_FBIMG_H_
