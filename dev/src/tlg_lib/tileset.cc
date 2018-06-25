#include "tileset.h"

#include <string>

#include "glog/logging.h"
#include "third_party/rapidxml.hpp"
#include "tlg_lib/tiled_utils.h"

using ::rapidxml::xml_document;
using ::rapidxml::xml_node;
using std::string;

namespace tlg_lib {

std::unique_ptr<Tileset> Tileset::FromTsx(const string& tileset_path) {
  ScopedXmlDocument tileset(tileset_path);
  xml_node<>* node = tileset.doc().first_node();

  while ((node != nullptr) && strcmp(node->name(), "tileset") != 0) {
    node = node->next_sibling();
  }
  CHECK_NE(node, static_cast<xml_node<>*>(nullptr))
      << "No XML node named 'tileset' found.";
  
  
  


}

Tileset::Tileset(const string& image_path, uint32_t tile_w, uint32_t tile_h,
                 std::string&& name)
    : tile_w_(tile_w),
      tile_h_(tile_h),
      name_(name),
      image_(retro::FbImg::FromFile(image_path)) {}

}  // namespace tlg_lib
