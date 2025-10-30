#include "CacheManager.h"
#include <string.h>
#include <stdlib.h>

void GH_CacheInit(struct CacheBucket *cache) {
  if (cache == NULL) {
    return;
  }

  // Initialize cache bucket
  cache->entries = NULL;
  cache->size = 0;
  cache->entry_count = 0;
}

void GH_CacheCleanup(struct CacheBucket *cache) {
  if (cache == NULL) {
    return;
  }

  struct CacheEntry *entry = cache->entries;
  while (entry != NULL) {
    // Free each cache entry
    struct CacheEntry *next = entry->next;
    free(entry->path);
    free(entry->data);
    free(entry->etag);
    free(entry);
    entry = next;
  }

  // Reset cache bucket
  cache->entries = NULL;
  cache->size = 0;
  cache->entry_count = 0;
}

bool GH_CacheExists(struct CacheBucket *cache, const char *file_path) {
  if (cache == NULL || file_path == NULL) {
    return false;
  }

  struct CacheEntry *entry = cache->entries;
  while (entry != NULL) {
    // Compare paths
    if (strcmp(entry->path, file_path) == 0) {
      return true;
    }
    entry = entry->next;
  }
  return false;
}

struct CacheEntry* GH_CacheGetByPath(struct CacheBucket *cache, const char *file_path) {
  if (cache == NULL || file_path == NULL) {
    return NULL;
  }

  struct CacheEntry *entry = cache->entries;
  while (entry != NULL) {
    if (strcmp(entry->path, file_path) == 0) {
      return entry;
    }
    entry = entry->next;
  }
  return NULL;
}

bool GH_CacheAdd(struct CacheBucket *cache, const char *path, const char *data, 
                 size_t data_len, const char *etag, time_t mtime) {
  if (cache == NULL || path == NULL || data == NULL) {
    return false;
  }

  // Check if entry already exists and remove it
  GH_CacheRemove(cache, path);

  struct CacheEntry *new_entry = malloc(sizeof(struct CacheEntry));
  if (new_entry == NULL) {
    return false;
  }

  // Allocate and copy path
  new_entry->path = strdup(path);
  if (new_entry->path == NULL) {
    free(new_entry);
    return false;
  }

  // Allocate and copy data
  new_entry->data = malloc(data_len);
  if (new_entry->data == NULL) {
    free(new_entry->path);
    free(new_entry);
    return false;
  }
  memcpy(new_entry->data, data, data_len);
  new_entry->data_len = data_len;

  // Copy ETag if provided
  if (etag != NULL) {
    new_entry->etag = strdup(etag);
    if (new_entry->etag == NULL) {
      free(new_entry->data);
      free(new_entry->path);
      free(new_entry);
      return false;
    }
  } else {
    new_entry->etag = NULL;
  }

  new_entry->timestamp = mg_millis();
  new_entry->mtime = mtime;
  new_entry->next = cache->entries;
  cache->entries = new_entry;
  cache->size += data_len;
  cache->entry_count++;

  // Evict if cache is too large
  size_t max_cache_bytes = (size_t)CACHE_MAX_SIZE_MB * 1024 * 1024;
  if (cache->size > max_cache_bytes) {
    GH_CacheEvictOldest(cache, max_cache_bytes);
  }

  return true;
}

bool GH_CacheRemove(struct CacheBucket *cache, const char *file_path) {
  if (cache == NULL || file_path == NULL) {
    return false;
  }

  struct CacheEntry **prev = &cache->entries;
  struct CacheEntry *entry = cache->entries;

  while (entry != NULL) {
    if (strcmp(entry->path, file_path) == 0) {
      *prev = entry->next;
      cache->size -= entry->data_len;
      cache->entry_count--;
      
      free(entry->path);
      free(entry->data);
      free(entry->etag);
      free(entry);
      return true;
    }
    prev = &entry->next;
    entry = entry->next;
  }
  return false;
}

void GH_CacheEvictExpired(struct CacheBucket *cache, uint64_t current_time) {
  if (cache == NULL) {
    return;
  }

  struct CacheEntry **prev = &cache->entries;
  struct CacheEntry *entry = cache->entries;

  while (entry != NULL) {
    struct CacheEntry *next = entry->next;
    
    // Check if entry has expired
    if (current_time - entry->timestamp > CACHE_TTL_MS) {
      *prev = next;
      cache->size -= entry->data_len;
      cache->entry_count--;
      
      free(entry->path);
      free(entry->data);
      free(entry->etag);
      free(entry);
      
      entry = next;
    } else {
      prev = &entry->next;
      entry = next;
    }
  }
}

void GH_CacheEvictOldest(struct CacheBucket *cache, size_t max_size) {
  if (cache == NULL) {
    return;
  }

  // Keep removing oldest (last in list) entries until under limit
  while (cache->size > max_size && cache->entries != NULL) {
    struct CacheEntry **prev = &cache->entries;
    struct CacheEntry *entry = cache->entries;
    struct CacheEntry **oldest_prev = prev;
    struct CacheEntry *oldest = entry;

    // Find the oldest entry (smallest timestamp)
    while (entry != NULL) {
      if (entry->timestamp < oldest->timestamp) {
        oldest = entry;
        oldest_prev = prev;
      }
      prev = &entry->next;
      entry = entry->next;
    }

    // Remove oldest entry
    *oldest_prev = oldest->next;
    cache->size -= oldest->data_len;
    cache->entry_count--;
    
    free(oldest->path);
    free(oldest->data);
    free(oldest->etag);
    free(oldest);
  }
}
