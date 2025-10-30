#pragma once
#include <server/config.h>

#include <stdint.h>
#include <stddef.h>
#include <mongoose.h>

#ifndef CACHE_TTL_MS
#define CACHE_TTL_MS (5 * 60 * 1000)  //!< Default cache TTL: 5 minutes
#endif
#ifndef CACHE_MAX_SIZE_MB
#define CACHE_MAX_SIZE_MB 50          //!< Default max cache size: 50 MB
#endif

//! Struct for cache entry
struct CacheEntry {
    char *path; //!< File path
    char *data; //!< Cached data (owned by this entry)
    size_t data_len; //!< Length of cached data
    uint64_t timestamp; //!< Timestamp when the entry was cached (in milliseconds)
    char *etag; //!< ETag for this entry
    time_t mtime; //!< File modification time
    struct CacheEntry *next; //!< Pointer to the next cache entry
};

//! Struct for cache bucket
struct CacheBucket {
    struct CacheEntry *entries; //!< Pointer to the first cache entry
    size_t size; //!< Total size of the cache
    size_t entry_count; //!< Number of entries in the cache
};

// ==============================================================================
// Cache management functions
// ==============================================================================

/**
 * @brief Initialize the cache
 * 
 * @param cache Pointer to the cache bucket to initialize
 */
void GH_CacheInit(struct CacheBucket *cache);
/**
 * @brief Clean up the cache and free resources
 * 
 * @param cache Pointer to the cache bucket to clean up
 */
void GH_CacheCleanup(struct CacheBucket *cache);
/**
 * @brief Check if a cache entry exists for the given file path
 * 
 * @param cache Pointer to the cache bucket
 * @param file_path File path to check
 * @return bool true if exists, false otherwise
 */
bool GH_CacheExists(struct CacheBucket *cache, const char *file_path);
/**
 * @brief Retrieve a cache entry by file path
 * 
 * @param cache Pointer to the cache bucket
 * @param file_path File path to retrieve
 * @return Pointer to the cache entry if found, NULL otherwise
 */
struct CacheEntry* GH_CacheGetByPath(struct CacheBucket *cache, const char *file_path);
/**
 * @brief Add a new cache entry
 * 
 * @param cache Pointer to the cache bucket
 * @param path File path
 * @param data File data to cache
 * @param data_len Length of data
 * @param etag ETag string
 * @param mtime File modification time
 * @return bool true on success, false on failure
 */
bool GH_CacheAdd(struct CacheBucket *cache, const char *path, const char *data, 
                 size_t data_len, const char *etag, time_t mtime);

/**
 * @brief Remove a cache entry by path
 * 
 * @param cache Pointer to the cache bucket
 * @param file_path File path to remove
 * @return bool true if removed, false if not found
 */
bool GH_CacheRemove(struct CacheBucket *cache, const char *file_path);

/**
 * @brief Evict expired entries based on TTL
 * 
 * @param cache Pointer to the cache bucket
 * @param current_time Current timestamp in milliseconds
 */
void GH_CacheEvictExpired(struct CacheBucket *cache, uint64_t current_time);

/**
 * @brief Evict oldest entries to fit within size limit
 * 
 * @param cache Pointer to the cache bucket
 * @param max_size Maximum cache size in bytes
 */
void GH_CacheEvictOldest(struct CacheBucket *cache, size_t max_size);

// global
extern struct CacheBucket g_cache; //!< Global cache bucket instance