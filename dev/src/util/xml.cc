#include "xml.h"

#include <fstream>
#include <streambuf>
#include <string>

#include "glog/logging.h"
#include "util/cannonical_errors.h"
#include "util/statusor.h"

namespace rapidxml {
constexpr uint32_t kMaxContextChars = 64;
void parse_error_handler(const char *what, void *where) {
  char *error_context = static_cast<char *>(where);

  // Make sure there's a null terminator in at least the next kMaxContentChars
  // characters.
  for (uint32_t i = 0; i < kMaxContextChars; ++i, ++error_context) {
    if (*error_context == '\0') break;
  }
  *error_context = '\0';

  CHECK(false) << "RapidXML parser error: '" << what
               << "', (context 64 characters) '..."
               << static_cast<char *>(where) << "...'";
}
}  // namespace rapidxml

namespace {
char *ReadWholeFile(const std::string &path) {
  FILE *f = fopen(path.c_str(), "rb");
  CHECK_NE(f, static_cast<FILE *>(nullptr)) << "Failed to open file.";

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  // An extra character for the null terminator
  char *contents = (char *)malloc(fsize + 1);
  fread(contents, fsize, 1, f);
  fclose(f);

  // Add null terminator
  contents[fsize] = '\0';

  return contents;
}

void FreeWholeFile(char *whole_file) { free(whole_file); }
}  // namespace

namespace util {

void XmlNode::Descriptor::addAttribute(const char *field_name,
                                       XmlAttributeType type,
                                       size_t field_offset) {
  attributes.insert(std::pair<absl::string_view, Attribute>(
      absl::string_view(field_name), {type, field_offset}));
}

void XmlNode::Descriptor::addChild(const char *child_name, ParseFunc parser,
                                   size_t field_offset) {
  children.insert(std::pair<absl::string_view, Child>(
      absl::string_view(child_name), {parser, field_offset}));
}
void XmlNode::Descriptor::addData(size_t field_offset) {
  data_field_offset = field_offset;
}

void XmlNode::PopulateFromNode(rapidxml::xml_node<> *node) {
  static_assert(sizeof(uintptr_t) >= sizeof(void *),
                "No suitable integer type for conversion from pointer type");
  const Descriptor &desc = get_descriptor();

  if ((node->value_size() != 0) && (desc.data_field_offset != 0)) {
    *reinterpret_cast<absl::string_view *>(reinterpret_cast<uintptr_t>(this) +
                                           desc.data_field_offset) =
        absl::string_view(node->value());
  }

  for (rapidxml::xml_attribute<> *attr = node->first_attribute();
       attr != nullptr; attr = attr->next_attribute()) {
    auto &attr_i = desc.attributes.find(absl::string_view(attr->name()));
    if (attr_i == desc.attributes.end()) continue;
    auto &attr_desc = attr_i->second;
    switch (attr_desc.type) {
      case XmlAttributeType::INT:
        *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(this) +
                                      attr_desc.offset) =
            std::atoi(attr->value());
        break;
      case XmlAttributeType::FLOAT:
        *reinterpret_cast<float *>(reinterpret_cast<uintptr_t>(this) +
                                   attr_desc.offset) = std::atof(attr->value());
        break;
      case XmlAttributeType::STRING_VIEW:
        *reinterpret_cast<absl::string_view *>(
            reinterpret_cast<uintptr_t>(this) + attr_desc.offset) =
            absl::string_view(attr->value());
        break;
      default:
        CHECK(false) << "Unknown XML attribute type.";
    }
  }

  for (rapidxml::xml_node<> *child_node = node->first_node();
       child_node != nullptr; child_node = child_node->next_sibling()) {
    auto &child_i = desc.children.find(absl::string_view(child_node->name()));
    if (child_i == desc.children.end()) continue;
    auto &child_desc = child_i->second;
    child_desc.parser(
        child_node, reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(this) +
                                             child_desc.offset));
  }
}

}  // namespace util
