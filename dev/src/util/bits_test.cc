#include "bits.h"

#include "gtest/gtest.h"

TEST(BitsTest, AllOnes) {
  // Signed-ness shouldn't matter
  EXPECT_EQ(util::AllOnes<int32_t>(), 0xffffffff);
  EXPECT_EQ(util::AllOnes<uint32_t>(), 0xffffffff);

  EXPECT_EQ(util::AllOnes<int8_t>(), static_cast<int8_t>(0xff));
  EXPECT_EQ(util::AllOnes<int64_t>(), 0xffffffffffffffff);
  EXPECT_EQ(util::AllOnes<uint16_t>(), 0xffff);
}

TEST(BitsTest, SignExtend) {
  // Sign extend a nibble that = -1
  int32_t x = 0x0f;
  EXPECT_EQ(util::SignExtend<int64_t>(x, 4), -1);
  // Sign extend a 3 bit value that = -1
  x = 0x07;
  EXPECT_EQ(util::SignExtend<int64_t>(x, 3), -1);
  // When treating the above 3 bit value as a nibble, we get bin(7) = 111;
  EXPECT_EQ(util::SignExtend<int64_t>(x, 4), 7);
  uint8_t y = -13;
  EXPECT_EQ(util::SignExtend<uint16_t>(y, 8), static_cast<uint16_t>(-13));
}

TEST(BitsTest, Varint) {
  // Layout of values:      [        ]  [        ]  [        ]  [  ]  [  ]
  const uint8_t stream[] = {0xff, 0x01, 0xff, 0x00, 0xff, 0xff, 0x7f, 0x00};
  const uint16_t expected[] = {255, 127, 32767, 127, 0};

  const uint8_t* stream_ptr = &(stream[0]);
  for (int test_values = 0; test_values < (sizeof(expected) / sizeof(uint16_t));
       ++test_values) {
    EXPECT_EQ(util::varint::GetVarintAndInc(&stream_ptr),
              expected[test_values]);
  }
}
