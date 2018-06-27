#ifndef UTIL_XML_H_
#define UTIL_XML_H_

#include <functional>
#include <string_view>

#include "glog/logging.h"

#define RAPIDXML_NO_EXCEPTIONS
#include "third_party/rapidxml.hpp"

#include "util/noncopyable.h"

namespace util {

// The following utilities are for parsing XML documents into structures. By
// creating classes that extend XmlNode, we can populate the fields of those
// classes by associating the fields with XML element attributes and values.
//
// The general procedure is to set up classes extending XmlNode with descriptors
// that describe the mapping of XML node children / attributes / data to the
// fields in the classes, then calling ScopedXmlDocument with a top level
// XmlNode as the template parameter to parse the document.
//
// Notes:
//
//   1. When ScopedXmlDocument destructs, any references created from it become
//      invalid.
//
//   2. The only types permitted for XmlNode fields read from a document are
//      int32_t, float, and string_view
//
//   3. Child nodes are always stored in std::vectors in their parents. (see
//      example)
//
//   4. If a child of an unknown name or attribute of an unknown name is found,
//      that element/attribute is skipped.
//
//   5. atoi and atof are used directly to parse strings into numeric values.
//
// Lets demonstrate usage by way of example:
//
//   Given the following XML document:
//
//   <?xml version="1.0" encoding="UTF-8"?>
//   <root attrA = "a" attrB = "b">
//     <child attrC = "c">
//       first value
//     </child>
//     <child attrC = "c">
//       second value
//     </child>
//   </root>
//
//   We now create the following classes that can read this XML structure:
//
//   struct RootXmlNode : public XmlNode {
//     struct Descriptor : public XmlNode::Descriptor {
//       Descriptor() {
//         addAttribute("attrA", AttrType::STRING_VIEW,
//                      offsetof(RootXmlNode, a));
//         addAttribute("attrB", AttrType::STRING_VIEW,
//                      offsetof(RootXmlNode, b));
//         addChild("child",
//                  XmlNode::Descriptor::MakeChildParser<ChildXmlNode>(),
//                  offsetof(RootXmlNode, children));
//       };
//
//       std::string_view name() const { return "root"; }
//     }
//
//     // The following just provides the above descriptor to the base class
//     static const Descriptor descriptor;
//     const Descriptor& get_descriptor() const override {
//       return descriptor;
//     }
//
//     std::string_view a;
//     std::string_view b;
//
//     std::vector<ChildXmlNode> children;
//
//   };
//
//   struct ChildXmlNode : public XmlNode {
//     struct Descriptor : public XmlNode::Descriptor {
//       Descriptor() {
//         addAttribute("attrC", AttrType::STRING_VIEW,
//                      offsetof(ChildXmlNode, c));
//         addData(offsetof(ChildXmlNode, text));
//       }
//       std::string_view name() const { return "child"; }
//     };
//
//     static const Descriptor descriptor;
//     const Descriptor& get_descriptor() const override {
//       return descriptor;
//     }
//
//     std::string_view c;
//     std::string_view text;
//   };
//
//   Now we can read this document with a ScopedXmlDocument:
//
//   {
//     ScopedXmlDocument<RootXmlNode> doc;
//
//     const RootXmlNode& contents = doc.get()
//
//     // do stuff with contents...
//   }
//

struct XmlNode {
 public:
  enum class AttrType { INT, FLOAT, STRING_VIEW };

 protected:
  // Does most of the work: maps class fields to XML element
  // attributes/children/values
  struct Descriptor {
    Descriptor() : data_field_offset(0) {}
    struct Attribute {
      AttrType type;
      size_t offset;
    };

    typedef std::function<void(rapidxml::xml_node<>* node, void* field)>
        ParseFunc;
    struct Child {
      ParseFunc parser;
      size_t offset;
    };

    void addAttribute(const char* field_name, AttrType type,
                      size_t field_offset);
    void addChild(const char* child_name, ParseFunc parser,
                  size_t field_offset);
    void addData(size_t field_offset);

    // This is used to create ParseFuncs for reading XML element children
    template <typename T>
    static ParseFunc MakeChildParser() {
      return [](rapidxml::xml_node<>* node, void* field) {
        auto* cast_field = reinterpret_cast<std::vector<T>*>(field);
        cast_field->push_back(T());
        cast_field->back().PopulateFromNode(node);
      };
    }

    virtual std::string_view name() const = 0;

    size_t data_field_offset;
    std::unordered_map<std::string_view, Attribute> attributes;
    std::unordered_map<std::string_view, Child> children;
  };

  virtual const Descriptor& get_descriptor() const = 0;

 public:
  // Triggers the actual tree parsing.
  void PopulateFromNode(rapidxml::xml_node<>* node);
};

template <typename T>
class ScopedXmlDocument : public util::NonCopyable {
 public:
  // Parse an XML document at path into the XmlNode structure with root T. If
  // the root XmlNode name doesn't match the actual XML root name, or there is
  // no root, nothing is parsed.
  ScopedXmlDocument(const std::string& path) {
    // We can't just load the file contents into a string and use c_str() as
    // RapidXML may need to edit the data that backs its parsed structure,
    // therefore we have to do our own C string memory management.
    auto contents_or_error = ReadWholeFile(path);
    CHECK(contents_or_error.ok()) << contents_or_error.status().ToString();
    doc_contents_ = contents_or_error.ConsumeValue();

    parse(doc_contents_);
  }

  // An overload for in-memory XML files. Used for testing.
  ScopedXmlDocument(char* contents) : doc_contents_(nullptr) {
    parse(contents);
  }

  ~ScopedXmlDocument() { FreeWholeFile(doc_contents_); }

  const T& get() const { return root_node_; }

 private:
  void parse(char* contents) {
    rapidxml::xml_document<> doc_;
    doc_.parse<0>(contents);

    rapidxml::xml_node<>* root = doc_.first_node();
    if (root == nullptr) return;
    if (std::string_view(root->name()) != root_node_.get_descriptor().name()) {
      return;
    }

    root_node_.PopulateFromNode(root);
  }

  char* ReadWholeFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    CHECK_NE(f, static_cast<FILE*>(nullptr)) << "Failed to open file.";

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // An extra character for the null terminator
    char* contents = (char*)malloc(fsize + 1);
    fread(contents, fsize, 1, f);
    fclose(f);

    // Add null terminator
    contents[fsize] = '\0';

    return contents;
  }

  void FreeWholeFile(char* contents) {
    if (contents != nullptr) free(contents);
  }

  T root_node_;
  char* doc_contents_;
};

}  // namespace util
#endif  // UTIL_XML_H_
