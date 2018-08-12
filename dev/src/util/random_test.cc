#include "util/random.h"

#include <thread>

#include "gtest/gtest.h"

namespace util {
TEST(RandomTest, RndProducesExpectedStream) {
  srnd(7);
  EXPECT_EQ(rnd(), 7502184425954839352);
  EXPECT_EQ(rnd(), 13784590019079701008);
  EXPECT_EQ(rnd(), 15622261382321576264);
  EXPECT_EQ(rnd(), 14755305849839382545);
  EXPECT_EQ(rnd(), 17183097442131438481);
  EXPECT_EQ(rnd(), 16247413184472007261);
  EXPECT_EQ(rnd(), 15875529455150490703);
  EXPECT_EQ(rnd(), 11493385281396468525);
}

TEST(RandomTest, RndUniqueStreamPerThread) {
  std::vector<std::vector<int64_t>> stream(3, std::vector<int64_t>(1000, 0));
  auto stream_pop = [](std::vector<int64_t>* s) {
    for (auto& v : *s) { v = rnd(); }
  };

  std::thread t0(std::bind(stream_pop, &(stream[0])));
  std::thread t1(std::bind(stream_pop, &(stream[1])));
  std::thread t2(std::bind(stream_pop, &(stream[2])));

  t0.join();
  t1.join();
  t2.join();

  EXPECT_NE(stream[0], stream[1]);
  EXPECT_NE(stream[1], stream[2]);
  EXPECT_NE(stream[2], stream[0]);
}
}  // namespace util
