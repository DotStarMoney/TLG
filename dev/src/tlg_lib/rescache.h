#ifndef TLG_LIB_RESCACHE_H_
#define TLG_LIB_RESCACHE_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "absl/strings/str_cat.h"
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
//   static unique_ptr<MyResource>
//       Load(const std::string& uri, ResCache* cache) {
//     <Code to load and create instance here, possibly loading more resources.>
//   }
//
//   static constexpr uint64_t kTypeId = <Type Id>
//
//   uint64_t type_id() const override { return kTypeId; }
//
// "Load" loads and creates instances of the resource and is the cache's way
// of creating and caching members it does not already have. "kTypeId" provides
// a means of checking that loaded resources match the desired template type.
// It is suggested to choose a random number for this value. "type_id" must
// always just return the value of kTypeId.
//
// Resources that load other resources
// ___________________________________
//
// Loadable::Load method takes a ResCache* parameter for resources that, when
// loading, will load other resources. ResCache will automatically resolve paths
// to resources loaded from other resources by assuming they are specified in
// relative paths. I.e: if Loadable A has uri "/res/dir/a.xml" and in its Load
// method calls Load of Loadable B with uri "b.xml", ResCache will try to load
// the Loadable B from "/res/dir/b.xml."
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
  //
  //   static unique_ptr<MyResource>
  //       Load(const std::string& uri, ResCache* cache) { ... }
  //
  // and
  //
  //   static constexpr uint64_t kTypeId = ...
  //
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
    std::string adjusted_uri;
    const std::string* final_uri = &uri;
    if (!relative_path_.empty()) {
      adjusted_uri = relative_path_;
      absl::StrAppend(&adjusted_uri, uri);
      final_uri = &adjusted_uri;
    }

    auto& resource_i = resources_.find(*final_uri);
    if (resource_i == resources_.end()) {
      std::string_view old_relative_path(relative_path_);
      relative_path_ = ExtractRelativePath(*final_uri);

      resource_i =
          resources_
              .insert(std::pair<std::string, std::unique_ptr<Loadable>>(
                  *final_uri, T::Load(*final_uri, this)))
              .first;

      relative_path_ = old_relative_path;
    }
    util::Loan<const T> resource = resource_i->second->GetResource<const T>();
    CHECK_EQ(T::kTypeId, resource->type_id())
        << "Loaded resource does not match requested type.";
    return std::move(resource);
  }

 private:
  static std::string_view ExtractRelativePath(const std::string& uri);

  // We can store Lenders outright as unordered_map insertion doesn't
  // invalidate references.
  std::unordered_map<std::string, std::unique_ptr<Loadable>> resources_;

  // A path pre-pended to uris to support relative paths. If a resource is
  // loaded in the context of another loading resource, the Load called by
  // the first Load will prepend this path to the resource uri.
  static thread_local std::string_view relative_path_;
};

}  // namespace tlg_lib
#endif  // TLG_LIB_RESCACHE_H_
