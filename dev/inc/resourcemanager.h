#ifndef RESOURCEMANAGER_H_
#define RESOURCEMANAGER_H_

#include <array>
#include <string_view>
#include <type_traits>

#include "external\flat_hash_map.h"
#include "static_type_assert.h"
#include "status.h"
#include "statusor.h"
#include "assert.h"


class Resource {
 public:
  virtual ~Resource() = 0;
  virtual int64_t resource_uid() const = 0;
  virtual int64_t GetUsageBytes() const = 0;
};

class ResourceManager {
 public:
  typedef uint64_t MapID;

  static constexpr int64_t kNoPoolMembership = -1;

  ResourceManager();

  // Get a pointer to a stored resource, blocks until resource is loaded or
  // it wasn't able to be loaded.
  template <class T>
  util::StatusOr<const T*> Get(std::string_view id) {
    static_assert(std::is_base_of<Resource, T>::value,
        "Type is not a Resource.");
    ASSIGN_OR_RETURN(auto resource, BlockingGet(uri));

    ASSERT_EQ(resource->resource_uid(), T::resource_uid()); 
    return reinterpret_cast<const T*>(resource);
  }

  // Maps an id to a uri, with an optional pool membership.
  util::Status MapIDToURI(std::string_view id, std::string_view uri,
      int64_t membership_id = kNoPoolMembership);


  util::StatusOr<int64_t> GetPoolSize(int64_t membership_id);
  // Registers a resource pool. Resource pools help limit the number of bytes
  // resources sharing some membership id can occupy. 
  void RegisterPool(int64_t membership_id, int64_t size_bytes);
 private:
  util::StatusOr<const Resource*> BlockingGet(std::string_view uri);

  struct ResourceEntry {
    const Resource* resource;
    std::string uri;
  };
  external::util::flat_hash_map<std::string, std::string> resources_; 

  struct PoolInfo {
    int64_t size_bytes;
    int64_t capacity_bytes;
  };
  external::util::flat_hash_map<int64_t, PoolInfo> resource_pools_; 

};

#endif // RESOURCEMANAGER_H_