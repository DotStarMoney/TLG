#include "util/pstruct.h"

#include "gtest/gtest.h"

template <typename T0, typename T1>
void CompareAddr(T0* a, long offset, T1* b) {
  EXPECT_EQ(reinterpret_cast<unsigned long long>(a) + offset,
            reinterpret_cast<unsigned long long>(b));
}

pstruct Packed1 {
  uint8_t field1;
  uint16_t field2;
  uint32_t field3;
  uint64_t field4;
  uint8_t field5;
  uint32_t field6;
};

TEST(PStructTest, PackedByByte) {
  Packed1 p;
  CompareAddr(&p.field1, 1, &p.field2);
  CompareAddr(&p.field2, 2, &p.field3);
  CompareAddr(&p.field3, 4, &p.field4);
  CompareAddr(&p.field4, 8, &p.field5);
  CompareAddr(&p.field5, 1, &p.field6);
  EXPECT_EQ(sizeof(Packed1), 20);
}

aligned_struct(2) Packed2 {
  uint8_t field1;
  uint16_t field2;
  uint32_t field3;
  uint64_t field4;
  uint8_t field5;
  uint32_t field6;
};

TEST(PStructTest, PackedByWord) {
  Packed2 p;
  CompareAddr(&p.field1, 2, &p.field2);
  CompareAddr(&p.field2, 2, &p.field3);
  CompareAddr(&p.field3, 4, &p.field4);
  CompareAddr(&p.field4, 8, &p.field5);
  CompareAddr(&p.field5, 2, &p.field6);
  EXPECT_EQ(sizeof(Packed2), 22);
}