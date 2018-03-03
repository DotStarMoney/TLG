#include "fbimg.h"

#include "glog/logging.h"
#include "stb_image.h"
#include "util/deleterptr.h"

using glm::ivec2;
using std::string;
using std::unique_ptr;
using util::deleter_ptr;

namespace retro {

FbImg::FbImg(deleter_ptr<SDL_Surface> surface,
             deleter_ptr<StbImageData> image_data)
    : surface_(std::move(surface)), image_data_(std::move(image_data)) {}

unique_ptr<FbImg> FbImg::OfSize(ivec2 dimensions, FbColor32 fill_color) {
  deleter_ptr<SDL_Surface> surface(
      SDL_CreateRGBSurface(0, dimensions.x, dimensions.y, 32, 0x00ff000000,
                           0x0000ff00, 0x000000ff, 0xff000000),
      [](SDL_Surface* s) { SDL_FreeSurface(s); });
  CHECK_NE(surface.get(), nullptr) << "SDL error: " << SDL_GetError();
  CHECK_EQ(SDL_FillRect(surface.get(), NULL, fill_color), 0)
      << "SDL Error: " << SDL_GetError();
  return unique_ptr<FbImg>(new FbImg(std::move(surface), nullptr));
}

unique_ptr<FbImg> FbImg::FromFile(const string& filename) {
  int width;
  int height;
  int orig_format_unused;
  deleter_ptr<StbImageData> image_data(
      stbi_load(filename.c_str(), &width, &height, &orig_format_unused,
                STBI_rgb_alpha),
      [](StbImageData* d) { stbi_image_free(d); });
  CHECK_NE(image_data.get(), nullptr)
      << "stb_image error: " << stbi_failure_reason();
  deleter_ptr<SDL_Surface> surface(
      SDL_CreateRGBSurfaceWithFormatFrom(image_data.get(), width, height, 32,
                                         4 * width, SDL_PIXELFORMAT_RGBA32),
      [](SDL_Surface* s) { SDL_FreeSurface(s); });
  CHECK_NE(surface.get(), nullptr) << "SDL error: " << SDL_GetError();
  return unique_ptr<FbImg>(
      new FbImg(std::move(surface), std::move(image_data)));
}

}  // namespace retro
