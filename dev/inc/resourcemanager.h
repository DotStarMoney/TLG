#ifndef RESOURCEMANAGER_H_
#define RESOURCEMANAGER_H_

#include <array>
#include <functional>
#include <string_view>
#include <string>
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
  typedef uint64_t PoolID;
  static constexpr PoolID kDefaultPoolMembership = -1;

  // A function that deserializes a stream into a resource.
  typedef std::function<util::StatusOr<std::unique_ptr<const Resource>>(
      std::istream*)> DeserializerFunction;

  ResourceManager();

  // Get a pointer to a stored resource
  template <class T>
  util::StatusOr<const T*> Get(std::string_view id) {
    return Get(StringToMapID(id));
  }
  template <class T>
  util::StatusOr<const T*> Get(MapID id) {
    static_assert(std::is_base_of<Resource, T>::value,
        "Type is not a Resource.");
    ASSIGN_OR_RETURN(auto resource, InternalGet(uri));

    ASSERT_EQ(resource->resource_uid(), T::kResourceUID);
    return reinterpret_cast<const T*>(resource);
  }

  // Maps an id to a uri, with an optional pool membership for the id
  util::Status MapIDToURI(std::string_view id, std::string_view uri,
      PoolID membership_id = kDefaultPoolMembership);
  util::Status MapIDToURI(MapID id, std::string_view uri,
      PoolID membership_id = kDefaultPoolMembership);

  // When loading a uri with the provided uri_extension, use the provided
  // deserialization function.
  void RegisterDeserializer(std::string_view uri_extension, 
      DeserializerFunction func);

  // Loads the resource corresponding to the given id
  util::Status Load(std::string_view id);
  util::Status Load(MapID id);

  // Unloads the resource corresponding to the given id
  util::Status Unload(std::string_view id);
  util::Status Unload(MapID id);

  util::StatusOr<int64_t> GetPoolSize(PoolID membership_id);
  // Registers a resource pool. Resource pools help limit the number of bytes
  // resources sharing some membership id can occupy. 
  void RegisterPool(PoolID membership_id, int64_t size_bytes);
 private:
  // Get the resource corresponding to the given id
  util::StatusOr<const Resource*> InternalGet(MapID id);

  // Used to convert string ids to proper MapID type ids. 
  MapID StringToMapID(std::string_view id);

  struct ResourceEntry {
    std::unique_ptr<const Resource> resource;
    std::atomic_uint32_t references;
    std::string uri;
  };
  external::util::flat_hash_map<MapID, ResourceEntry> resources_;
  struct PoolInfo {
    int64_t size_bytes;
    int64_t capacity_bytes;
  };
  external::util::flat_hash_map<PoolID, PoolInfo> resource_pools_;
  // Maps URI extensions to deserialization functions.
  external::util::flat_hash_map<std::string, DeserializerFunction>
      deserializers_;
};

#endif // RESOURCEMANAGER_H_