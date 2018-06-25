#ifndef UTIL_XML_H_
#define UTIL_XML_H_

#include <functional>
#include <string>

#include "absl/strings/string_view.h"
#include "glog/logging.h"

#define RAPIDXML_NO_EXCEPTIONS
#include "third_party/rapidxml.hpp"

#include "util/noncopyable.h"

// finish docs on header and impl, then write tests

namespace util {

/*
The following utilities are for parsing XML documents into structures.


struct ImageXmlNode : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("source", XmlAttributeType::STRING_VIEW,
        offsetof(ImageXmlNode, source));
    }
    absl::string_view name() const override { return "image"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  absl::string_view source;
};

struct TilesetXmlNode : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("tilewidth", XmlAttributeType::INT,
        offsetof(TilesetXmlNode, tile_w));
      addAttribute("tileheight", XmlAttributeType::INT,
        offsetof(TilesetXmlNode, tile_h));
      addAttribute("name", XmlAttributeType::STRING_VIEW,
        offsetof(TilesetXmlNode, name));
      addChild("image", XmlNode::Descriptor::MakeChildParser<ImageXmlNode>(),
        offsetof(TilesetXmlNode, images));
    }
    absl::string_view name() const override { return "tileset"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  uint32_t tile_w;
  uint32_t tile_h;
  absl::string_view name;
  std::vector<ImageXmlNode> images;
};
*/

struct XmlNode {
 public:
  enum class XmlAttributeType { INT, FLOAT, STRING_VIEW };

 protected:
  struct Descriptor {
    Descriptor() : data_field_offset(0) {}
    struct Attribute {
      XmlAttributeType type;
      size_t offset;
    };

    typedef std::function<void(rapidxml::xml_node<>* node, void* field)>
        ParseFunc;
    struct Child {
      ParseFunc parser;
      size_t offset;
    };

    void addAttribute(const char* field_name, XmlAttributeType type,
                      size_t field_offset);
    void addChild(const char* child_name, ParseFunc parser,
                  size_t field_offset);
    void addData(size_t field_offset);

    template <typename T>
    static ParseFunc MakeChildParser() {
      return [](xml_node<>* node, void* field) {
        auto* cast_field = reinterpret_cast<std::vector<T>*>(field);
        cast_field->push_back(T().PopulateFromNode(node))
      }
    }

    virtual absl::string_view name() const = 0;

    size_t data_field_offset;
    std::unordered_map<absl::string_view, Attribute> attributes;
    std::unordered_map<absl::string_view, Child> children;
  };

  virtual const Descriptor& get_descriptor() const = 0;

 public:
  void PopulateFromNode(rapidxml::xml_node<>* node);
};

template <typename T>
class ScopedXmlDocument : public util::NonCopyable {
 public:
  ScopedXmlDocument(const std::string& path) {
    auto contents_or_error = ReadWholeFile(path);
    CHECK(contents_or_error.ok()) << contents_or_error.status().ToString();
    doc_contents_ = contents_or_error.ConsumeValue();

    rapidxml::xml_document<> doc_;
    doc_.parse<0>(doc_contents_);

    rapidxml::xml_node<>* root = doc_.first_node();
    if (root == nullptr) return;
    if (absl::string_view(root->name()) != T.get_descriptor().name()) return;

    root_node_.PopulateFromNode(root);
  }
  ~ScopedXmlDocument() { FreeWholeFile(doc_contents_); }

  const T& get() const { return root_node_; }

 private:
  T root_node_;
  char* doc_contents_;
};

}  // namespace util
#endif  // UTIL_XML_H_
