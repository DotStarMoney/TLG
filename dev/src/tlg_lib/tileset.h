#ifndef TLG_LIB_TILESET_H_
#define TLG_LIB_TILESET_H_

#include <memory>
#include <string>

#include "retro/fbimg.h"
#include "tlg_lib/rescache.h"

namespace tlg_lib {

class Tileset : public Loadable {
 public:
  static constexpr uint64_t kTypeId = 0x2c4265a422b2f5c8;
  uint64_t type_id() const override { return kTypeId; }

  static std::unique_ptr<Tileset> Load(const std::string& uri, ResCache* cache);

  const std::string& name() const { return name_; }
  uint32_t tile_w() const { return tile_w_; }
  uint32_t tile_h() const { return tile_h_; }
  uint32_t w() const {
    CHECK(image_) << "Meta tileset.";
    return image_->width();
  }
  uint32_t h() const {
    CHECK(image_) << "Meta tileset.";
    return image_->height();
  }
  const retro::FbImg& image() const {
    CHECK(image_) << "Meta tileset.";
    return *(image_.get());
  }
  bool is_meta() const { return !image_; }

 private:
  Tileset(const std::string& image_path, uint32_t tile_w, uint32_t tile_h,
          const std::string& name);

  Tileset(uint32_t tile_w, uint32_t tile_h, const std::string& name);

  const uint32_t tile_w_;
  const uint32_t tile_h_;
  const std::string name_;
  std::unique_ptr<retro::FbImg> image_;
};

}  // namespace tlg_lib
#endif  // TLG_LIB_TILESET_H_
