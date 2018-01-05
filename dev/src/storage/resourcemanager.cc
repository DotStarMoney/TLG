#include "storage/resourcemanager.h"

#include <fstream>
#include <string>

#include "util/make_cleanup.h"
#include "util/strcat.h"

using ::util::Status;
using ::util::StatusOr;
using ::util::StrCat;

namespace storage {
namespace {
constexpr char kResourceFilePathPrefix[] = "res/";
} // namespace

ResourceManager::ResourceManager() : total_resource_bytes(0) {}

ResourceManager::~ResourceManager() {
  // Fail if outstanding references exist.
  for (const auto& resource_entry_kv : resources_) {
    const auto& resource_entry = resource_entry_kv.second;
    ASSERT_EQ(resource_entry->weak_references.load(), 0);
    ASSERT_EQ(resource_entry->strong_references.load(), 0);
  }
}

void ResourceManager::MapIDToURI(MapID id, std::string_view uri,
    PoolID pool_id) {
  std::unique_lock<std::shared_mutex> lock(shared_lock);

  auto resource_iter = resources_.find(id);
  if (resource_iter == resources_.end()) {
    resources_.insert(std::make_pair(id, 
        std::make_unique<ResourceEntry>(uri, pool_id)));
    return;
  }
  auto resource = resource_iter->second.get();
  resource->uri = uri;
  resource->pool = pool_id;
  return;
}

Status ResourceManager::RegisterDeserializer(const std::string& uri_extension,
    DeserializerFunction func) {
  std::unique_lock<std::shared_mutex> lock(shared_lock);

  if (uri_extension.size() < 3) {
    return util::InvalidArgumentError("Extensions must be >= 3 characters.");
  }
  auto deserializer_iter = deserializers_.find(uri_extension);
  if (deserializer_iter == deserializers_.end()) {
    deserializers_.insert(std::make_pair(std::string(uri_extension), func));
    return util::OkStatus;
  }
  deserializer_iter->second = func;
  return util::OkStatus;
}

// Not synchronized
util::StatusOr<ResourceManager::ResourceEntry*>
    ResourceManager::GetResourceEntry(MapID id) {
  auto resource_iter = resources_.find(id);
  if (resource_iter == resources_.end()) {
    return util::FailedPreconditionError(
      StrCat("Resource '", id, "' does not have a mapping."));
  }
  auto resource_entry = resource_iter->second.get();
  return resource_entry;
}

Status ResourceManager::Load(MapID id) {
  // Once we have acquired these values, we can unlock the mutex to allow
  // parallel serialization
  std::string uri;
  DeserializerFunction deserializer;

  {
    std::unique_lock<std::shared_mutex> lock(shared_lock);
    ASSIGN_OR_RETURN(auto resource_entry, GetResourceEntry(id));
    // No need to reload if its already loaded
    if (resource_entry->resource) return util::OkStatus;
    // As of now, we only understand URIs that refer to file names, so the
    // extension maps directly to the file extension here (as long as its no
    // more than 3 characters)
    std::string_view extension = resource_entry->uri;
    // Get the last 3 characters of the file name.
    extension.remove_prefix(extension.size() - 3);
    ASSIGN_OR_RETURN(deserializer, GetDeserializer(std::string(extension)));
    uri = resource_entry->uri;
  }

  std::ifstream stream(StrCat(kResourceFilePathPrefix, uri), 
      std::ios::binary);
  if (!stream) {
    return util::ResourceUnobtainable("Unable to open file for stream.");
  }
  // Close the stream even if we fail during deserialization.
  util::MakeCleanup close_stream([&stream]() { stream.close(); });
  ASSIGN_OR_RETURN(auto resource, deserializer(&stream));
  close_stream.cleanup();
  
  // Lock once more and re-get the resource entry, theres a chance someone came
  // in and beat us to the deserialization punch or unloaded this while we were
  // working, so we have to check again.
  std::unique_lock<std::shared_mutex> lock(shared_lock);
  ASSIGN_OR_RETURN(auto resource_entry, GetResourceEntry(id));
  // No need to reload if its already loaded
  if (resource_entry->resource) return util::OkStatus;
  RETURN_IF_ERROR(AddToPool(resource_entry->pool, resource->GetUsageBytes()));
  resource->parent_entry_ = resource_entry;
  resource_entry->resource = std::move(resource);
  return util::OkStatus;
}

Status ResourceManager::Unload(MapID id) {
  std::unique_lock<std::shared_mutex> lock(shared_lock);

  auto resource_iter = resources_.find(id);
  if (resource_iter == resources_.end()) {
    return util::FailedPreconditionError(
      StrCat("Resource '", id, "' does not have a mapping."));
  }
  auto resource_entry = resource_iter->second.get();
  // No need to unload if its already unloaded
  if (!resource_entry->resource) return util::OkStatus;

  if (resource_entry->weak_references.load() > 0) {
    return util::FailedPreconditionError(
        StrCat("Cannot unload resource with id : ", id, " while outstanding "
        "references to it exist."));
  }

  // If there are outstanding strong references, release the mutex, and spin
  // until there are no more strong references.
  if (resource_entry->strong_references.load() > 0) {
    lock.unlock();
    while(resource_entry->strong_references.load() > 0); 
    lock.lock();
  }

  RETURN_IF_ERROR(AddToPool(resource_entry->pool,
      resource_entry->resource->GetUsageBytes()));
  resource_entry->resource.release();
  return util::OkStatus;
}

StatusOr<int64_t> ResourceManager::GetPoolUsageBytes(PoolID id) const {
  std::shared_lock<std::shared_mutex> s_lock(shared_lock);

  auto pool_iter = resource_pools_.find(id);
  if (pool_iter == resource_pools_.end()) {
    return util::FailedPreconditionError(
        StrCat("Pool '", id, "' does not exist."));
  }
  return pool_iter->second.size_bytes;
}

int64_t ResourceManager::GetTotalResourceBytes() const { 
  std::shared_lock<std::shared_mutex> s_lock(shared_lock);
  return total_resource_bytes; 
}

util::Status ResourceManager::RegisterPool(PoolID id, int64_t size_bytes) {
  std::unique_lock<std::shared_mutex> lock(shared_lock);

  auto pool_iter = resource_pools_.find(id);
  if (pool_iter == resource_pools_.end()) {
    resource_pools_.insert(std::make_pair(id, PoolInfo{0, size_bytes}));
    return util::OkStatus;
  }
  auto& pool = pool_iter->second;
  pool.capacity_bytes = size_bytes;
  if (pool.size_bytes > pool.capacity_bytes) {
    return util::FailedPreconditionError(
        StrCat("New pool size is less than its current usage. id : ", id, " (", 
            pool.size_bytes, " > ", pool.capacity_bytes, ")"));
  }
  return util::OkStatus;
}

ResourceManager::MapID ResourceManager::StringToMapID(std::string_view id) {
  ResourceManager::MapID map_id = 0;
  std::memcpy(&map_id, id.data(), 
      (id.size() < sizeof(ResourceManager::MapID)) ? id.size() : 
          sizeof(ResourceManager::MapID));
  return map_id;
}

StatusOr<ResourceManager::ResourceEntry*> ResourceManager::InternalGet(
    MapID id) const {
  auto resource_iter = resources_.find(id);
  if (resource_iter == resources_.end()) {
    return util::FailedPreconditionError(
        StrCat("Resource '", id,"' does not exist."));
  }
  auto resource_entry = resource_iter->second.get();
  if (!(resource_entry->resource)) {
    return util::FailedPreconditionError(
        StrCat("Resource '", id, "' is unloaded."));
  }
  return resource_entry;
}

// Not synchronized
StatusOr<ResourceManager::DeserializerFunction>
    ResourceManager::GetDeserializer(const std::string& extension) const {
  auto deserializer_iter = deserializers_.find(extension);
  if (deserializer_iter == deserializers_.end()) {
    return util::FailedPreconditionError(
        StrCat("Extension '", extension, "' does not exist."));
  }
  return deserializer_iter->second;
}

// Not synchronized
Status ResourceManager::AddToPool(PoolID id, int64_t bytes) {
  if (id == kDefaultPoolMembership) {
    total_resource_bytes += bytes;
    return util::OkStatus;
  }

  auto pool_iter = resource_pools_.find(id);
  if (pool_iter == resource_pools_.end()) {
    return util::FailedPreconditionError(
        StrCat("Pool '", id, "' does not exist."));
  }
  PoolInfo& pool_info = pool_iter->second;
  ASSERT_GE(pool_info.size_bytes + bytes, 0);
  if ((pool_info.size_bytes + bytes) > pool_info.capacity_bytes) {
    return util::OutOfMemoryError(StrCat("Pool : ", id, " out of memory."));
  }
  pool_info.size_bytes += bytes;
  total_resource_bytes += bytes;
  return util::OkStatus;
}
} // namespace storage