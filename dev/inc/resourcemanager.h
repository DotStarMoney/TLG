#ifndef RESOURCEMANAGER_H_
#define RESOURCEMANAGER_H_

#include <array>
#include <functional>
#include <string_view>
#include <string>
#include <type_traits>
#include <shared_mutex>

#include "external\flat_hash_map.h"
#include "deleterptr.h"
#include "static_type_assert.h"
#include "status.h"
#include "status_macros.h"
#include "statusor.h"
#include "assert.h"
#include "noncopyable.h"

class Resource {
 public:
  virtual ~Resource() {};
  virtual int64_t resource_uid() const = 0;
  virtual int64_t GetUsageBytes() const = 0;
};

class ResourceManager : public util::NonCopyable {
 public:
  typedef uint64_t MapID;
  typedef uint64_t PoolID;
  static constexpr PoolID kDefaultPoolMembership = -1;
  
  template <class T>
  using ResourcePtr = util::deleter_ptr<const T>;

  // A function that deserializes a stream into a resource.
  typedef std::function<util::StatusOr<std::unique_ptr<Resource>>(
      std::istream*)> DeserializerFunction;

  ResourceManager();
  // Will cause an abort if outstanding resource references exist.
  ~ResourceManager();

  // Get a pointer to a stored resource
  template <class T>
  util::StatusOr<ResourcePtr<T>> Get(std::string_view id) const {
    return Get(StringToMapID(id));
  }
  template <class T>
  util::StatusOr<ResourcePtr<T>> Get(MapID id) const {
    static_assert(std::is_base_of<Resource, T>::value,
        "Type is not a Resource.");
    std::shared_lock<std::shared_mutex> s_lock(shared_lock);

    ASSIGN_OR_RETURN(auto resource_entry, InternalGet(uri));
    if (resource_entry->resource->resource_uid() != T::kResourceUID) {
      return util::InvalidArgumentError(
          "Requested type does not match resource type.");
    }
    ++(resource->references);
    // Its safe to capture the resource pointer as its location won't change
    // over the lifetime of its mapping in the resources_ table.
    return ResourcePtr<T>(resource_entry->resource.get(),
        [resource_entry](const T*){ --(resource_entry->references); });
  }

  // Maps an id to a uri, with an optional pool membership for the id
  void MapIDToURI(std::string_view id, std::string_view uri,
      PoolID pool_id = kDefaultPoolMembership) {
    MapIDToURI(StringToMapID(id), uri, pool_id);
  }
  void MapIDToURI(MapID id, std::string_view uri,
      PoolID pool_id = kDefaultPoolMembership);

  // When loading a uri with the provided uri_extension, use the provided
  // deserialization function. Extensions are contextual, and could be
  // anything from a physical file extension, to values retrieved from the
  // actual byte stream that holds the resource.
  //
  // If the uri_extension is less than 3 bytes long, it is an error.
  util::Status RegisterDeserializer(const std::string& uri_extension,
      DeserializerFunction func);

  // Loads the resource corresponding to the given id
  util::Status Load(std::string_view id) { return Load(StringToMapID(id)); }
  util::Status Load(MapID id);

  // Unloads the resource corresponding to the given id
  util::Status Unload(std::string_view id) { 
    return Unload(StringToMapID(id));
  }
  util::Status Unload(MapID id);

  util::StatusOr<int64_t> GetPoolUsageBytes(PoolID id) const;
  // Registers a resource pool. Resource pools help limit the number of bytes
  // resources sharing some membership id can occupy. Pools can re-register
  // with a different number of bytes as long as it is not less than the
  // current total bytes in use by the pool.
  util::Status RegisterPool(PoolID id, int64_t size_bytes);

  int64_t GetTotalResourceBytes() const;
 private:
  // Used to convert string ids to proper MapID type ids. 
  static MapID StringToMapID(std::string_view id);

  struct ResourceEntry {
    ResourceEntry(std::string_view uri, PoolID pool = kDefaultPoolMembership)
        : references(0), uri(uri), pool(pool) {}

    std::unique_ptr<Resource> resource;
    // Note: we modify this value in Get() although it is const. This is okay,
    // because we're modifying the data in the pointer to this ResourceEntry,
    // not the pointer itself!
    std::atomic_uint32_t references;
    std::string uri;
    PoolID pool;
  };
  // We store pointers to resources so that their location in memory is
  // consistent and won't change with the map.
  external::util::flat_hash_map<MapID, std::unique_ptr<ResourceEntry>> 
      resources_;
  
  struct PoolInfo {
    int64_t size_bytes;
    int64_t capacity_bytes;
  };
  external::util::flat_hash_map<PoolID, PoolInfo> resource_pools_;
  
  // Maps URI extensions to deserialization functions.
  external::util::flat_hash_map<std::string, DeserializerFunction>
      deserializers_;

  // Get the resource corresponding to the given id
  util::StatusOr<ResourceEntry*> InternalGet(MapID id) const;

  // Given a uri extension, gets an associated deserializer. Returns an error
  // if not found.
  //
  // Not synchronized.
  util::StatusOr<DeserializerFunction> GetDeserializer(
      const std::string& extension) const;

  // Add an amount (can be negative) of bytes to a resource pool.
  //
  // Not synchronized.
  util::Status AddToPool(PoolID id, int64_t bytes);

  // Get a resource entry, or fail if it doesn't exist.
  // 
  // Not synchronized.
  util::StatusOr<ResourceEntry*> GetResourceEntry(MapID id);

  // The total amount of bytes consumed by all bytes in all resource pools 
  // (including the default pool)
  int64_t total_resource_bytes;

  // Ensures that loading/unloading and other state manipulation doesn't clash
  // with resource gettin' which can happen concurrently.
  mutable std::shared_mutex shared_lock;
};

#endif // RESOURCEMANAGER_H_
