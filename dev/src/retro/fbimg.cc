#include "retro/fbimg.h"

#include "glog/logging.h"
#include "retro/fbgfx.h"
#include "stb_image.h"
#include "util/deleterptr.h"

using glm::ivec2;
using std::string;
using std::unique_ptr;
using util::deleter_ptr;

namespace retro {

FbImg::FbImg(deleter_ptr<SDL_Texture> texture, int w, int h, bool is_target)
    : texture_(std::move(texture)), w_(w), h_(h), is_target_(is_target) {}

unique_ptr<FbImg> FbImg::OfSize(ivec2 dimensions, FbColor32 fill_color) {
  FbGfx::CheckInit(__func__);

  deleter_ptr<SDL_Texture> texture(
      SDL_CreateTexture(FbGfx::renderer_.get(), SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, dimensions.x, dimensions.y),
      [](SDL_Texture* t) { SDL_DestroyTexture(t); });
  CHECK_NE(texture.get(), nullptr)
      << "SDL error (SDL_CreateTexture): " << SDL_GetError();
  return unique_ptr<FbImg>(
      new FbImg(std::move(texture), dimensions.x, dimensions.y, true));
}

deleter_ptr<SDL_Texture> FbImg::TextureFromSurface(SDL_Surface* surface) {
  deleter_ptr<SDL_Texture> texture(
      SDL_CreateTextureFromSurface(FbGfx::renderer_.get(), surface),
      [](SDL_Texture* t) { SDL_DestroyTexture(t); });
  CHECK_NE(texture.get(), nullptr)
      << "SDL error (SDL_CreateTextureFromSurface): " << SDL_GetError();
  return std::move(texture);
}

unique_ptr<FbImg> FbImg::FromFile(const string& filename) {
  FbGfx::CheckInit(__func__);

  int w;
  int h;
  int orig_format_unused;
  deleter_ptr<StbImageData> image_data(
      stbi_load(filename.c_str(), &w, &h, &orig_format_unused, STBI_rgb_alpha),
      [](StbImageData* d) { stbi_image_free(d); });
  CHECK_NE(image_data.get(), nullptr)
      << "stb_image error (stbi_load): " << stbi_failure_reason();

  deleter_ptr<SDL_Surface> surface(
      SDL_CreateRGBSurfaceWithFormatFrom(image_data.get(), w, h, 32, 4 * w,
                                         SDL_PIXELFORMAT_RGBA8888),
      [](SDL_Surface* s) { SDL_FreeSurface(s); });
  CHECK_NE(surface.get(), nullptr)
      << "SDL error (SDL_CreateRGBSurfaceWithFormatFrom): " << SDL_GetError();

  return unique_ptr<FbImg>(
      new FbImg(TextureFromSurface(surface.get()), w, h, false));
}

}  // namespace retro
