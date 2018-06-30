#ifndef TLG_LIB_RESCACHE_H_
#define TLG_LIB_RESCACHE_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "glog/logging.h"
#include "util/loan.h"
#include "util/noncopyable.h"

namespace tlg_lib {

// ResCache is a simple interface to constant resources. The Load method can be
// called to retrieve an already cached Loadable, or to retrieve then cache a
// Loadable using the template parameter's Load method. Loadables must have
// three special members: Load, kTypeId, and type_id. Given a Loadable
// "MyResource":
//
//   static unique_ptr<MyResource> Load(const std::string& uri) {
//     <Code to load and create instance here.>
//   }
//
//   static constexpr uint64_t kTypeId = <Type Id>
//
//   uint64_t type_id() const { return kTypeId; }
//
// "Load" loads and creates instances of the resource and is the cache's way
// of creating and caching members it does not already have. "kTypeId" provides
// a means of checking that loaded resources match the desired template type.
// It is suggested to choose a random number for this value. "type_id" must
// always just return the value of kTypeId;
//
// This class is not thread safe in any way.

class Loadable : util::Lender {
 public:
  template <class T>
  util::Loan<T> GetResource() {
    return MakeLoan<T>();
  }

  virtual uint64_t type_id() const = 0;
  // Subclasses must also implement:
  //   static unique_ptr<MyResource> Load(const std::string& uri) { ... }
  // and
  //   static constexpr uint64_t kTypeId = ...
};

class ResCache : public util::NonCopyable {
 public:
  // Get a loan to a constant cached resource, possibly loading it if it isn't
  // already loaded.
  //
  // T = class implementing Loadable
  template <typename T>
  util::Loan<const T> Load(const std::string& uri) {
    static_assert(std::is_base_of<Loadable, T>::value,
                  "Can only load classes extending tlg_lib::Loadable.");
    auto& resource_i = resources_.find(uri);
    if (resource_i == resources_.end()) {
      resource_i =
          resources_
              .insert(std::pair<std::string, std::unique_ptr<Loadable>>(
                  uri, T::Load(uri)))
              .first;
    }
    util::Loan<const T> resource = resource_i->second->GetResource<const T>();
    CHECK_EQ(T::kTypeId, resource->type_id())
        << "Loaded resource does not match requested type.";
    return std::move(resource);
  }

 private:
  // We can store Lenders outright as unordered_map insertion doesn't
  // invalidate references.
  std::unordered_map<std::string, std::unique_ptr<Loadable>> resources_;
};

}  // namespace tlg_lib
#endif  // TLG_LIB_RESCACHE_H_
