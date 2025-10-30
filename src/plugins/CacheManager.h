#pragma once

#include <stdint.h>
#include <stddef.h>
#include <mongoose.h>

//! Struct for cache entry
struct CacheEntry {
    char *path; //!< File path
    struct mg_str data; //!< Cached data
    uint16_t timestamp; //!< Timestamp when the entry was cached
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
 * @param entry Cache entry to add
 * @return bool true on success, false on failure
 */
bool GH_CacheAdd(struct CacheBucket *cache, struct CacheEntry entry);

// global
extern struct CacheBucket *g_pCache; //!< Global cache bucket instance