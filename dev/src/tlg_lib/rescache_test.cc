#include "tlg_lib/rescache.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace tlg_lib {

// A loadable that just stores the uri passed into it.
struct TestResource : public Loadable {
  static constexpr uint64_t kTypeId = 1;

  uint64_t type_id() const override { return kTypeId; }

  static std::unique_ptr<TestResource> Load(const std::string& uri,
                                            ResCache* cache) {
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
  uint64_t type_id() const override { return kTypeId; }
  static std::unique_ptr<TestResource> Load(const std::string& uri,
                                            ResCache* cache) {
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

// A loadable that will call Load for another instance of itself.
struct ChainingTestResource : public Loadable {
  static constexpr uint64_t kTypeId = 1;
  uint64_t type_id() const override { return kTypeId; }

  static std::unique_ptr<ChainingTestResource> Load(const std::string& uri,
                                                    ResCache* cache) {
    static bool passed_through = false;

    if (!passed_through) {
      passed_through = true;
      // Note the URI specified here does not have a basename; one should be
      // appended for it.
      cache->Load<ChainingTestResource>("local_path.xml");
      x0 = uri;
      return std::make_unique<ChainingTestResource>();
    } else {
      x1 = uri;
      return std::make_unique<ChainingTestResource>();
    }
  }

  static std::string x0;
  static std::string x1;
};
std::string ChainingTestResource::x0;
std::string ChainingTestResource::x1;

TEST(ResCacheTest, testRecursiveLoadRelativePath) {
  ResCache c;

  c.Load<ChainingTestResource>("path/to/resource/bag.xml");

  EXPECT_EQ(ChainingTestResource::x0, "path/to/resource/bag.xml");
  EXPECT_EQ(ChainingTestResource::x1, "path/to/resource/local_path.xml");
}

}  // namespace tlg_lib
