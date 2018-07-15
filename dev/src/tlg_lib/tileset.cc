#include "tileset.h"

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "glog/logging.h"
#include "util/xml.h"

namespace tlg_lib {
namespace {

struct ImageXmlNode : public util::XmlNode {
  struct Descriptor : public util::XmlNode::Descriptor {
    Descriptor() {
      addAttribute("source", AttrType::STRING_VIEW,
                   offsetof(ImageXmlNode, source));
    }
    std::string_view name() const override { return "image"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  std::string_view source;
};
const ImageXmlNode::Descriptor ImageXmlNode::descriptor;

struct TilesetXmlNode : public util::XmlNode {
  struct Descriptor : public util::XmlNode::Descriptor {
    Descriptor() {
      addAttribute("tilewidth", AttrType::INT,
                   offsetof(TilesetXmlNode, tile_w));
      addAttribute("tileheight", AttrType::INT,
                   offsetof(TilesetXmlNode, tile_h));
      addAttribute("name", AttrType::STRING_VIEW,
                   offsetof(TilesetXmlNode, name));
      addChild("image",
               util::XmlNode::Descriptor::MakeChildParser<ImageXmlNode>(),
               offsetof(TilesetXmlNode, images));
    }
    std::string_view name() const override { return "tileset"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  TilesetXmlNode() : tile_w(0), tile_h(0) {}

  int32_t tile_w;
  int32_t tile_h;
  std::string_view name;
  std::vector<ImageXmlNode> images;
};
const TilesetXmlNode::Descriptor ImageXmlNode::descriptor;

const std::unordered_set<std::string_view> kMetaTilesetNames{
    "collision", "cshapes", "cshapes2", "cshapes3"};
}  // namespace

std::unique_ptr<Tileset> Tileset::Load(const std::string& uri,
                                       ResCache* cache) {
  util::ScopedXmlDocument<TilesetXmlNode> doc(uri);
  const auto& tileset = doc.get();

  CHECK_EQ(tileset.images.size(), 1)
      << "Each tileset must have exactly 1 image";
  CHECK_GT(tileset.tile_w, 0) << "Bad tile width: " << tileset.tile_w;
  CHECK_GT(tileset.tile_h, 0) << "Bad tile height: " << tileset.tile_h;

  if (kMetaTilesetNames.find(tileset.name) != kMetaTilesetNames.end()) {
    // This is a meta tileset, so we don't actually load an image for it.
    return std::make_unique<Tileset>(tileset.tile_w, tileset.tile_h,
                                     tileset.name);
  }

  return std::make_unique<Tileset>(tileset.images[0].source, tileset.tile_w,
                                   tileset.tile_h, tileset.name);
}

Tileset::Tileset(const std::string& image_path, uint32_t tile_w,
                 uint32_t tile_h, const std::string& name)
    : tile_w_(tile_w),
      tile_h_(tile_h),
      name_(name),
      image_(retro::FbImg::FromFile(image_path)) {}

Tileset::Tileset(uint32_t tile_w, uint32_t tile_h, const std::string& name)
    : tile_w_(tile_w), tile_h_(tile_h), name_(name), image_(nullptr) {}
}  // namespace tlg_lib
