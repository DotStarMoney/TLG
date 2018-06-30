#include "encoding.h"

#include <string.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace util {

TEST(EncodingTest, testBase64DecodeAllocationSize) {
  EXPECT_EQ(base64::GetDecodedAllocationSize(4), 4);
  EXPECT_EQ(base64::GetDecodedAllocationSize(8), 8);
  EXPECT_EQ(base64::GetDecodedAllocationSize(12), 12);
  EXPECT_EQ(base64::GetDecodedAllocationSize(16), 12);
  EXPECT_EQ(base64::GetDecodedAllocationSize(20), 16);
  EXPECT_EQ(base64::GetDecodedAllocationSize(24), 20);
  EXPECT_EQ(base64::GetDecodedAllocationSize(256), 192);
}

TEST(EncodingTest, testBase64DecodeEmpty) {
  EXPECT_EQ(base64::Decode("", nullptr), 0);
}

constexpr char kEncodedString[] =
    "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0a"
    "GlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3"
    "Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGU"
    "gY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBl"
    "eGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";

constexpr char kExpectedString[] =
    "Man is distinguished, not only by his reason, but by this singular "
    "passion from other animals, which is a lust of the mind, that by a "
    "perseverance of delight in the continued and indefatigable generation of "
    "knowledge, exceeds the short vehemence of any carnal pleasure.";

TEST(EncodingTest, testBase64Decode) {
  size_t length = strlen(kEncodedString);

  char* dest = new char[base64::GetDecodedAllocationSize(length) + 1];
  size_t chars = base64::Decode(kEncodedString, dest);

  ASSERT_EQ(chars, strlen(kExpectedString));

  dest[chars] = '\0';
  EXPECT_EQ(std::string_view(dest), std::string_view(kExpectedString));
}

TEST(EncodingTest, testBase64MultipleOf4) {
  EXPECT_DEATH({ base64::Decode("123", nullptr); }, "multiple of 4");
}

}  // namespace util
