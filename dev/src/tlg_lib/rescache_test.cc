#include "tlg_lib/rescache.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace tlg_lib {

// A loadable that just stores the uri passed into it.
struct TestResource : public Loadable {
  static constexpr uint64_t kTypeId = 1;

  uint64_t type_id() const { return kTypeId; }

  static std::unique_ptr<TestResource> Load(const std::string& uri) {
    ++times_loaded;
    return std::make_unique<TestResource>(uri);
  }

  TestResource(const std::string& x) : x(x) {}

  const std::string x;

  static uint32_t times_loaded;
};
uint32_t TestResource::times_loaded = 0;

TEST(ResCacheTest, testBasicCaching) {
  ResCache c;

  EXPECT_EQ(TestResource::times_loaded, 0);

  util::Loan<const TestResource> res1 = c.Load<TestResource>("i'm a uri");
  EXPECT_EQ(res1->x, "i'm a uri");
  EXPECT_EQ(TestResource::times_loaded, 1);

  util::Loan<const TestResource> res2 = c.Load<TestResource>("i'm a uri");
  EXPECT_EQ(res2->x, "i'm a uri");
  EXPECT_EQ(TestResource::times_loaded, 1);
}

// A loadable just like TestResource, however it has a differing kTypeId;
struct OtherTestResource : public Loadable {
  static constexpr uint64_t kTypeId = 2;
  uint64_t type_id() const { return kTypeId; }
  static std::unique_ptr<TestResource> Load(const std::string& uri) {
    return std::make_unique<TestResource>(uri);
  }
  OtherTestResource(const std::string& x) : x(x) {}
  const std::string x;
};

TEST(ResCacheTest, testMismatchedTypes) {
  ResCache c;

  util::Loan<const TestResource> res1 = c.Load<TestResource>("i'm a uri");
  EXPECT_EQ(res1->x, "i'm a uri");
  EXPECT_DEATH(
      {
        util::Loan<const OtherTestResource> res2 =
            c.Load<OtherTestResource>("i'm a uri");
      },
      "does not match");
}

}  // namespace tlg_lib
