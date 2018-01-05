#ifndef STORAGE_RESOURCEMANAGER_H_
#define STORAGE_RESOURCEMANAGER_H_

#include <array>
#include <functional>
#include <string_view>
#include <string>
#include <type_traits>
#include <shared_mutex>

#include "external\flat_hash_map.h"
#include "base/static_type_assert.h"
#include "util/deleterptr.h"
#include "util/status.h"
#include "util/status_macros.h"
#include "util/statusor.h"
#include "util/assert.h"
#include "util/noncopyable.h"

// ResourceManager:
// A thread-safe class to manage static resources such as images, sounds, 
// etc...
//
// ResourceManager is designed for parallelized loading of resources and
// lock-free acquisition of resources while not loading and unloading.
//
// The usage pattern is to register de-serialization functions for a given
// "extension," register mappings from resource IDs to URIs, then load any
// resources via their id. Acquisition of resources is accomplished through
// "Get()" methods, and Get() will not pin the acquired resources in
// ResourceManager. Unloading a resource with non-destructed references is an
// error. (TLDR, ResourceManager is NOT a cache)
// 
// An "extension" is just an identifier string computed from a URI or from data
// acquired from a URI that tells ResourceManager what type of deserializer to
// use.
//
// Resource ID parmeters, or mappings, or MapIDs, take the form of a 64bit
// value: either a 64bit int or 8 byte string.

// Resource:
// An interface required by classes that wish to be "resources." They also must
// have:
// 
// public:
//  static constexpr int64_t kResourceUID = ...
//
// -such that resource_uid() returns kResourceUID
//
namespace storage {

class Resource;
class ResourceManager : public util::NonCopyable {
  friend class Resource;
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

  // Make a resource handle strongly referenced, i.e., it will block unloading
  // of itself until the strong reference is released.
  //
  // **IMPORTANT** Resources should only be made strongly referenced it they
  // are going to continue existing for an extremely short amount of time
  //
  // **IMPORTANT** Resources can only be made strongly referenced when they are
  // not already being unloaded! In other words, resource unloading and making
  // strong references must be externally synchronized.
  //
  template <class T>
  static ResourcePtr<T> MakeStrongReference(ResourcePtr<T> resource) {
    
    ResourceEntry* resource_entry = 
        reinterpret_cast<Resource*>(resource.get())->parent_entry_;

    // We increment the strong reference counter before decrementing the weak
    // reference counter so that a failure to externally synchronize this
    // method with resource unloading manifests as unloading a resource with
    // outstanding weak references only, and never the occasional 
	// (though bypassed) spin lock.
    ++(resource_entry->strong_references);
    --(resource_entry->weak_references);

    return ResourcePtr<T>(resource.release(),
        [resource_entry](const T*) { --(resource_entry->strong_references); });
  }

  // Used to convert string ids to proper MapID type ids. 
  static MapID StringToMapID(std::string_view id);
 private:

  struct ResourceEntry {
    ResourceEntry(std::string_view uri, PoolID pool = kDefaultPoolMembership)
        : weak_references(0), strong_references(0), uri(uri), pool(pool) {}

    std::unique_ptr<Resource> resource;
    // # of references that must be released before the resource is unloaded
    //
    // Note: we modify this value in Get() although it is const. This is okay,
    // because we're modifying the data in the pointer to this ResourceEntry,
    // not the pointer itself!
    std::atomic_uint32_t weak_references;
    // # of references that will block a call to unload this resource until
    // released.
    std::atomic_uint32_t strong_references;
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
  std::shared_mutex shared_lock;
};

class Resource : public util::NonCopyable {
  friend class ResourceManager;
public:
  virtual ~Resource() {};
  virtual int64_t resource_uid() const = 0;
  virtual int64_t GetUsageBytes() const = 0;
private:
  ResourceManager::ResourceEntry* parent_entry_;
};
} // namespace storage

#endif // STORAGE_RESOURCEMANAGER_H_
