#include "util/xml.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace util {
constexpr char testXml[] = R"(
<?xml version="1.0" encoding="UTF-8"?>
<root attrA = "a" attrB = "3.14">
  <child attrC = "8">
    first value
  </child>
  <child attrC = "4">
    second value
  </child>
</root>
)";

struct ChildXmlNode : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("attrC", AttrType::INT, offsetof(ChildXmlNode, c));
      addData(offsetof(ChildXmlNode, text));
    }
    std::string_view name() const { return "child"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  int32_t c;
  std::string_view text;
};
const ChildXmlNode::Descriptor ChildXmlNode::descriptor;

struct RootXmlNode : public XmlNode {
  struct Descriptor : public XmlNode::Descriptor {
    Descriptor() {
      addAttribute("attrA", AttrType::STRING_VIEW, offsetof(RootXmlNode, a));
      addAttribute("attrB", AttrType::FLOAT, offsetof(RootXmlNode, b));
      addChild("child", XmlNode::Descriptor::MakeChildParser<ChildXmlNode>(),
               offsetof(RootXmlNode, children));
    }
    std::string_view name() const { return "root"; }
  };
  static const Descriptor descriptor;
  const Descriptor& get_descriptor() const override { return descriptor; }

  std::string_view a;
  float b;

  std::vector<ChildXmlNode> children;
};
const RootXmlNode::Descriptor RootXmlNode::descriptor;

class TestXmlCopier {
 public:
  TestXmlCopier(const char* source) {
    contents = new char[strlen(source) + 1];
    strcpy(contents, source);
  }
  ~TestXmlCopier() { delete (contents); }

  char* get() { return contents; }

 private:
  char* contents;
};

TEST(XmlTest, BasicParse) {
  TestXmlCopier contents(testXml);
  ScopedXmlDocument<util::RootXmlNode> doc(contents.get());
  const auto& root = doc.get();

  EXPECT_EQ(root.a, "a");
  EXPECT_EQ(root.b, 3.14f);
  ASSERT_EQ(root.children.size(), 2);

  const auto& child1 = root.children[0];
  EXPECT_EQ(child1.c, 8);
  EXPECT_THAT(std::string(child1.text), testing::HasSubstr("first value"));

  const auto& child2 = root.children[1];
  EXPECT_EQ(child2.c, 4);
  EXPECT_THAT(std::string(child2.text), testing::HasSubstr("second value"));
}

constexpr char badXml[] = "<><>bad";

TEST(XmlTest, ParseError) {
  EXPECT_DEATH(
      {
        TestXmlCopier contents(badXml);
        ScopedXmlDocument<util::RootXmlNode> doc(contents.get());
      },
      "bad");
}

constexpr char emptyXml[] = "";

TEST(XmlTest, EmptyDoc) {
  TestXmlCopier contents(emptyXml);
  ScopedXmlDocument<util::RootXmlNode> doc(contents.get());
  EXPECT_EQ(std::string(doc.get().a), "");
}

constexpr char noRootXML[] = R"(
<not_named_root>
</not_named_root>
)";

TEST(XmlTest, MissingRoot) {
  TestXmlCopier contents(noRootXML);
  ScopedXmlDocument<util::RootXmlNode> doc(contents.get());
  EXPECT_EQ(std::string(doc.get().a), "");
}

}  // namespace util
