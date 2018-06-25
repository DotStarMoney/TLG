#ifndef TLG_LIB_TILESET_H_
#define TLG_LIB_TILESET_H_

#include <string>

#include "retro/fbimg.h"
#include "util/loan.h"

namespace tlg_lib {

class Tileset : public util::Lender {
 public:
  static std::unique_ptr<Tileset> FromTsx(const std::string& tileset_path);

  const std::string& name() const { return name_; }
  uint32_t tile_w() const { return tile_w_; }
  uint32_t tile_h() const { return tile_h_; }
  uint32_t w() const { return image_->width(); }
  uint32_t h() const { return image_->height(); }

 private:
  Tileset(const std::string& image_path, uint32_t tile_w, uint32_t tile_h,
          std::string&& name);

  const uint32_t tile_w_;
  const uint32_t tile_h_;
  const std::string name_;
  std::unique_ptr<retro::FbImg> image_;
};

}  // namespace tlg_lib
#endif  // TLG_LIB_TILESET_H_
