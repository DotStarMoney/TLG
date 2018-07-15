#include "stage.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"
#include "util/encoding.h"
#include "util/xml.h"

// doc it up

using glm::ivec2;
using physics::retro::BlockGrid;
using std::is_same;
using std::string;
using std::string_view;
using std::vector;
using util::XmlNode;

namespace tlg_lib {
namespace {
// XML structure of Tiled (https://www.mapeditor.org/ accessed 7/2/2018 version
// 1.1.5) map editor maps with extension ".tmx"
// -----------------------------------------------------------------------------
struct PropertyXmlN : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("name", AttrType::STRING_VIEW, offsetof(PropertyXmlN, name));
      addAttribute("type", AttrType::STRING_VIEW, offsetof(PropertyXmlN, type));
      addAttribute("value", AttrType::STRING_VIEW,
                   offsetof(PropertyXmlN, value));
    }
    string_view name() const override { return "property"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  string_view name;
  string_view value;
  string_view type;
};
const PropertyXmlN::Descriptor PropertyXmlN::descriptor;

struct PropertiesXmlN : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addChild("properties",
               XmlNode::Descriptor::MakeChildParser<PropertyXmlN>(),
               offsetof(PropertiesXmlN, properties));
    }
    string_view name() const override { return "properties"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  vector<PropertyXmlN> properties;
};
const PropertiesXmlN::Descriptor PropertiesXmlN::descriptor;

struct TilesetXmlN : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("firstgid", AttrType::INT, offsetof(TilesetXmlN, firstid));
      addAttribute("source", AttrType::STRING_VIEW,
                   offsetof(TilesetXmlN, source));
    }
    string_view name() const override { return "tileset"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  TilesetXmlN() : firstid(0) {}

  int32_t firstid;
  string_view source;
};
const TilesetXmlN::Descriptor TilesetXmlN::descriptor;

struct DataXmlN : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("encoding", AttrType::STRING_VIEW,
                   offsetof(DataXmlN, encoding));
      addData(offsetof(DataXmlN, data));
    }
    string_view name() const override { return "data"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  string_view encoding;
  string_view data;
};
const DataXmlN::Descriptor DataXmlN::descriptor;

struct LayerXmlN : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("name", AttrType::STRING_VIEW, offsetof(LayerXmlN, name));
      addChild("properties",
               XmlNode::Descriptor::MakeChildParser<PropertiesXmlN>(),
               offsetof(LayerXmlN, properties));
      addChild("data", XmlNode::Descriptor::MakeChildParser<DataXmlN>(),
               offsetof(LayerXmlN, data));
    }
    string_view name() const override { return "layer"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  string_view name;

  vector<PropertiesXmlN> properties;
  vector<DataXmlN> data;
};
const LayerXmlN::Descriptor LayerXmlN::descriptor;

struct ObjectXmlN : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("name", AttrType::STRING_VIEW, offsetof(ObjectXmlN, id));
      addAttribute("x", AttrType::INT, offsetof(ObjectXmlN, x));
      addAttribute("y", AttrType::INT, offsetof(ObjectXmlN, y));
      addAttribute("width", AttrType::INT, offsetof(ObjectXmlN, width));
      addAttribute("height", AttrType::INT, offsetof(ObjectXmlN, height));

      addChild("properties",
               XmlNode::Descriptor::MakeChildParser<PropertiesXmlN>(),
               offsetof(ObjectXmlN, properties));
    }
    string_view name() const override { return "object"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  ObjectXmlN() : x(0), y(0), width(0), height(0) {}

  string_view id;
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;

  vector<PropertiesXmlN> properties;
};
const ObjectXmlN::Descriptor ObjectXmlN::descriptor;

struct ObjectGroupXmlN : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("name", AttrType::STRING_VIEW,
                   offsetof(ObjectGroupXmlN, name));

      addChild("object", XmlNode::Descriptor::MakeChildParser<ObjectXmlN>(),
               offsetof(ObjectGroupXmlN, objects));
    }
    string_view name() const override { return "objectgroup"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  string_view name;

  vector<ObjectXmlN> objects;
};
const ObjectGroupXmlN::Descriptor ObjectGroupXmlN::descriptor;

struct MapXmlN : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("tiledversion", AttrType::STRING_VIEW,
                   offsetof(MapXmlN, version));
      addAttribute("orientation", AttrType::STRING_VIEW,
                   offsetof(MapXmlN, orientation));
      addAttribute("width", AttrType::INT, offsetof(MapXmlN, width));
      addAttribute("height", AttrType::INT, offsetof(MapXmlN, height));
      addAttribute("tilewidth", AttrType::INT, offsetof(MapXmlN, tilewidth));
      addAttribute("tileheight", AttrType::INT, offsetof(MapXmlN, tileheight));

      addChild("properties",
               XmlNode::Descriptor::MakeChildParser<PropertiesXmlN>(),
               offsetof(MapXmlN, properties));
      addChild("tileset", XmlNode::Descriptor::MakeChildParser<TilesetXmlN>(),
               offsetof(MapXmlN, tilesets));
      addChild("layer", XmlNode::Descriptor::MakeChildParser<LayerXmlN>(),
               offsetof(MapXmlN, layers));
      addChild("objectgroup",
               XmlNode::Descriptor::MakeChildParser<ObjectGroupXmlN>(),
               offsetof(MapXmlN, object_groups));
    }
    string_view name() const override { return "map"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  MapXmlN() : width(0), height(0), tilewidth(0), tileheight(0) {}

  string_view version;
  string_view orientation;
  int32_t width;
  int32_t height;
  int32_t tilewidth;
  int32_t tileheight;

  vector<PropertyXmlN> properties;
  vector<TilesetXmlN> tilesets;
  vector<LayerXmlN> layers;
  vector<ObjectGroupXmlN> object_groups;
};
const MapXmlN::Descriptor MapXmlN::descriptor;
// -----------------------------------------------------------------------------

constexpr uint32_t kMinRequiredMajorTmxVersion = 1;
constexpr uint32_t kMinRequiredMinorTmxVersion = 1;
constexpr uint32_t kMinRequiredPatchTmxVersion = 5;

constexpr char kIntPropertyTypeName[] = "int";
constexpr char kFloatPropertyTypeName[] = "float";
constexpr char kBoolPropertyTypeName[] = "bool";

constexpr char kBase64LayerEncoding[] = "base64";

constexpr char kMetaProperty[] = "meta";

std::tuple<uint32_t, uint32_t, uint32_t> SplitVersionString(
    const string& vers) {
  vector<string> parts = absl::StrSplit(vers, ".");
  CHECK_EQ(parts, 3) << "Version expected in 3 parts. Found " << parts.size();
  return {atoi(parts[0].c_str()), atoi(parts[1].c_str()),
          atoi(parts[2].c_str())};
}

void CheckTmxVersion(string_view version) {
  auto [major, minor, patch] = SplitVersionString(string(version));
  CHECK_GE(major, kMinRequiredMajorTmxVersion)
      << "Min required major TMX version = " << kMinRequiredMajorTmxVersion;
  if (major == kMinRequiredMajorTmxVersion) {
    CHECK_GE(minor, kMinRequiredMinorTmxVersion)
        << "Min required minor TMX version = " << kMinRequiredMinorTmxVersion;
    if (minor == kMinRequiredMinorTmxVersion) {
      CHECK_GE(patch, kMinRequiredPatchTmxVersion)
          << "Min required patch TMX version = " << kMinRequiredPatchTmxVersion;
    }
  }
}

typedef std::unordered_map<string_view, const PropertyXmlN&> PropertyMap;

PropertyMap MakePropertyMap(const vector<PropertiesXmlN>& props_v) {
  if (props_v.empty()) return {};
  CHECK(props_v.size() == 1) << "Cannot have more than one 'properties' node.";
  const auto& props = props_v[0];
  std::unordered_map<string_view, const PropertyXmlN&> prop_map;
  for (const auto& prop : props.properties) {
    prop_map.insert({prop.name, prop});
  }
  return prop_map;
}

template <typename T>
std::optional<T> GetPropertyValue(const PropertyMap& props, string_view key) {
  const auto& prop_i = props.find(key);
  if (prop_i == props.end()) return std::nullopt;
  const auto& prop = prop_i->second;
  if constexpr (is_same<T, int32_t>::value || is_same<T, int64_t>::value) {
    CHECK_EQ(prop.type, kIntPropertyTypeName);
    int32_t result = 0;
    CHECK(absl::SimpleAtoi(prop.value, &result)) << "Int parsing failed.";
    return result;
  } else if constexpr (is_same<T, float>::value || is_same<T, double>::value) {
    CHECK_EQ(prop.type, kFloatPropertyTypeName);
    float result = 0f;
    CHECK(absl::SimpleAtof(prop.value, &result)) << "Float parsing failed.";
    return result;
  } else if constexpr (is_same<T, bool>::value) {
    CHECK_EQ(prop.type, kBoolPropertyTypeName);
    bool result = false;
    CHECK(absl::SimpleAtob(prop.value, &result)) << "Bool parsing failed.";
    return result;
  } else if constexpr (is_same<T, string_view>::value) {
    CHECK(prop.type.empty());
    return prop.value;
  } else {
    static_assert(false);
  }
}

// Bit field layout of a single 32bit .tmx tile id
struct TmxTile {
  uint32_t gid : 29, flip_d : 1, flip_v : 1, flip_h : 1;
};

// Bit field of layout of stage tile descriptor
struct TileDescriptor {
  uint32_t tile_i : 24, set_i : 8;
};

TileDescriptor ToTileDescriptor(const vector<TilesetXmlN>& tilesets,
                                uint32_t meta_index, TmxTile tile) {
  CHECK_EQ(tile.flip_d | tile.flip_v | tile.flip_h, 0)
      << "Tile flip flags are not allowed.";
  for (uint32_t i = tilesets.size() - 1; i >= 0; --i) {
    if (tilesets[i].firstid < tile.gid) {
      // Include the fact that we don't hold on to the meta tileset in the
      // tileset list by subtracting 1 from every tileset index above it.
      return {tile.gid - tilesets[i].firstid + 1, i - (i > meta_index ? 1 : 0)};
    }
  }
}

std::tuple<vector<util::Loan<const Tileset>>, uint32_t> LoadTilesets(
    const vector<TilesetXmlN>& xml_tilesets, ResCache* cache) {
  vector<util::Loan<const Tileset>> tilesets;

  uint32_t meta_tileset_count = 0;
  int32_t meta_index = -1;
  for (uint32_t tileset_i = 0; tileset_i < xml_tilesets.size(); ++tileset_i) {
    const auto& tileset = xml_tilesets[tileset_i];
    auto tileset_ptr = cache->Load<Tileset>(string(tileset.source));

    if (!tileset_ptr->is_meta()) {
      tilesets.push_back(std::move(tileset_ptr));
    } else {
      CHECK_LT(++meta_tileset_count, 2) << "Multiple meta tilesets.";
      meta_index = tileset_i;
    }
  }
  CHECK_NE(meta_index, -1) << "Meta tileset not found.";
  return {std::move(tilesets), meta_index};
}

void PopulateBlockGrid(const TileDescriptor* tiles, BlockGrid* blocks) {
  const uint32_t total_blocks = blocks->dims().x * blocks->dims().y;
  for (int i = 0; i < total_blocks; ++i) {
    auto tile = tiles[i];

    // TODO(?): This mapping should be made more official in some way.
    BlockGrid::BlockType block_type = BlockGrid::BlockType::FULL;
    switch (tile.tile_i) {
      case 1:
        block_type = BlockGrid::BlockType::FULL;
        break;
      case 58:
        block_type = BlockGrid::BlockType::ONE_WAY_UP;
        break;
      default:
        block_type = BlockGrid::BlockType::NONE;
    }

    blocks->PutBlock(i, block_type);
  }
}
}  // namespace

std::unique_ptr<StageContent> StageContent::Load(const string& uri,
                                                 ResCache* cache) {
  static_assert(sizeof(TmxTile) == sizeof(int32_t));
  static_assert(sizeof(TileDescriptor) == sizeof(int32_t));

  util::ScopedXmlDocument<MapXmlN> doc(uri);
  const auto& map = doc.get();

  CheckTmxVersion(map.version);
  CHECK_EQ(map.orientation, "orthogonal")
      << "Map orientation must be orthogonal." << map.orientation;
  CHECK_EQ(map.tilewidth, map.tileheight) << "Tiles must be square";
  CHECK_GE(map.layers, 2) << "Map needs at least 2 layers (1 meta, 1 graphic).";

  auto [tilesets, meta_index] = LoadTilesets(map.tilesets, cache);

  BlockGrid collision(ivec2{map.width, map.height}, map.tilewidth);

  // Minus one for the meta layer
  vector<vector<TileDescriptor>> stage_layers(
      map.layers.size() - 1, vector<TileDescriptor>(map.width * map.height));

  const uint32_t total_layer_tiles = map.width * map.height;
  const uint32_t total_layer_bytes = total_layer_tiles * sizeof(int32_t);
  uint32_t cur_stage_layer = 0;
  {
    std::unique_ptr<int32_t> layer_data(new int32_t[total_layer_tiles]);
    for (const auto& layer : map.layers) {
      CHECK_EQ(layer.data.size(), 1);
      CHECK_EQ(layer.data[0].encoding, kBase64LayerEncoding);

      // This check is just to preempt the possibility of an unchecked buffer
      // overflow.
      CHECK_EQ(
          util::base64::GetDecodedAllocationSize(layer.data[0].data.size()),
          total_layer_bytes);
      util::base64::Decode(layer.data[0].data,
                           reinterpret_cast<char*>(layer_data.get()));

      // Convert the tile dwords to TileDescriptor form.
      for (uint32_t i = 0; i < total_layer_tiles; ++i) {
        layer_data.get()[i] = util::force_cast<uint32_t>(
            ToTileDescriptor(map.tilesets, meta_index,
                             util::force_cast<TmxTile>(layer_data.get()[i])));
      }

      const auto props = MakePropertyMap(layer.properties);
      if (GetPropertyValue<bool>(props, kMetaProperty).value_or(false)) {
        PopulateBlockGrid(reinterpret_cast<TileDescriptor*>(layer_data.get()),
                          &collision);
        continue;
      }

      CHECK_LT(cur_stage_layer, stage_layers.size()) << "Meta layer not found.";
      auto& stage_layer = stage_layers[cur_stage_layer++];
      for (uint32_t i = 0; i < total_layer_tiles; ++i) {
        stage_layer[i] = util::force_cast<TileDescriptor>(layer_data.get()[i]);
      }
    }
  }

  return std::make_unique<StageContent>(
      std::move(tilesets), std::move(stage_layers), std::move(collision));
}
}  // namespace tlg_lib
