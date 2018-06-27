#include "tileset.h"

#include <string>

#include "glog/logging.h"
#include "third_party/rapidxml.hpp"

using ::rapidxml::xml_document;
using ::rapidxml::xml_node;
using std::string;

namespace tlg_lib {

std::unique_ptr<Tileset> Tileset::FromTsx(const string& tileset_path) {
  return nullptr;


}

Tileset::Tileset(const string& image_path, uint32_t tile_w, uint32_t tile_h,
                 std::string&& name)
    : tile_w_(tile_w),
      tile_h_(tile_h),
      name_(name),
      image_(retro::FbImg::FromFile(image_path)) {}

}  // namespace tlg_lib
