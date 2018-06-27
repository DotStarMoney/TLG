#include "xml.h"

#include <fstream>
#include <streambuf>
#include <string>

#include "glog/logging.h"

namespace rapidxml {
constexpr uint32_t kMaxContextChars = 64;
// Since we don't do exceptions, define an error handler using glog that prints
// the next 64 characters after an XML parsing error.
void parse_error_handler(const char *what, void *where) {
  char *error_context = static_cast<char *>(where);

  // Make sure there's a null terminator in at least the next kMaxContentChars
  // characters.
  for (uint32_t i = 0; i < kMaxContextChars; ++i, ++error_context) {
    if (*error_context == '\0') break;
  }
  *error_context = '\0';

  CHECK(false) << "RapidXML parser error: '" << what
               << "', (next 64 characters) '"
               << static_cast<char *>(where) << "'";
}
}  // namespace rapidxml

namespace util {

void XmlNode::Descriptor::addAttribute(const char *field_name, AttrType type,
                                       size_t field_offset) {
  attributes.insert(std::pair<std::string_view, Attribute>(
      std::string_view(field_name), {type, field_offset}));
}

void XmlNode::Descriptor::addChild(const char *child_name, ParseFunc parser,
                                   size_t field_offset) {
  children.insert(std::pair<std::string_view, Child>(
      std::string_view(child_name), {parser, field_offset}));
}
void XmlNode::Descriptor::addData(size_t field_offset) {
  data_field_offset = field_offset;
}

void XmlNode::PopulateFromNode(rapidxml::xml_node<> *node) {
  // We expect uintptr_t to be of actual pointer width
  static_assert(sizeof(uintptr_t) >= sizeof(void *),
                "No suitable integer type for conversion from pointer type");
  const Descriptor &desc = get_descriptor();

  if ((node->value_size() != 0) && (desc.data_field_offset != 0)) {
    *reinterpret_cast<std::string_view *>(reinterpret_cast<uintptr_t>(this) +
                                           desc.data_field_offset) =
        std::string_view(node->value());
  }

  for (rapidxml::xml_attribute<> *attr = node->first_attribute();
       attr != nullptr; attr = attr->next_attribute()) {
    auto &attr_i = desc.attributes.find(std::string_view(attr->name()));
    // Its not an error if we don't find an attribute, we just move along to the
    // next one
    if (attr_i == desc.attributes.end()) continue;

    auto &attr_desc = attr_i->second;
    switch (attr_desc.type) {
      case AttrType::INT:
        *reinterpret_cast<int32_t *>(reinterpret_cast<uintptr_t>(this) +
                                      attr_desc.offset) =
            std::atoi(attr->value());
        break;
      case AttrType::FLOAT:
        *reinterpret_cast<float *>(reinterpret_cast<uintptr_t>(this) +
                                   attr_desc.offset) = std::atof(attr->value());
        break;
      case AttrType::STRING_VIEW:
        *reinterpret_cast<std::string_view *>(
            reinterpret_cast<uintptr_t>(this) + attr_desc.offset) =
            std::string_view(attr->value());
        break;
      default:
        CHECK(false) << "Unknown XML attribute type.";
    }
  }

  for (rapidxml::xml_node<> *child_node = node->first_node();
       child_node != nullptr; child_node = child_node->next_sibling()) {
    auto &child_i = desc.children.find(std::string_view(child_node->name()));
    // Same as for attributes, finding an unknown child isn't an error.
    if (child_i == desc.children.end()) continue;
    auto &child_desc = child_i->second;
    child_desc.parser(
        child_node, reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(this) +
                                             child_desc.offset));
  }
}

}  // namespace util
