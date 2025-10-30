#include "CacheManager.h"

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

bool GH_CacheAdd(struct CacheBucket *cache, struct CacheEntry entry) {
  if (cache == NULL) {
    return false;
  }

  struct CacheEntry *new_entry = malloc(sizeof(struct CacheEntry));
  if (new_entry == NULL) {
    return false;
  }

  // Copy entry data
  new_entry->path = strdup(entry.path);
  new_entry->data = entry.data;
  new_entry->timestamp = entry.timestamp;
  new_entry->next = cache->entries;
  cache->entries = new_entry;
  cache->size += new_entry->data.len;
  cache->entry_count++;

  return true;
}
