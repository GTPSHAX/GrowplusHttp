#pragma once

#include <stdint.h>
#include <stddef.h>
#include <mongoose.h>

struct CacheEntry {
    char *path;
    struct mg_str data;
    uint16_t timestamp;
    size_t hits;
    struct CacheEntry *next;
};

struct CacheBucket {
    struct CacheEntry *entries;
    size_t size;
    size_t entry_count;
};

// =====================
// Fungsi CacheManager
// =====================
void GH_CacheInit(struct CacheBucket *cache);
void GH_CacheCleanup(struct CacheBucket *cache);
int GH_CacheExists(struct CacheBucket *cache, const char *file_path);
struct CacheEntry* GH_CacheGetByPath(struct CacheBucket *cache, const char *file_path);
bool GH_CacheAdd(struct CacheBucket *cache, struct CacheEntry entry);

// global
extern struct CacheBucket g_Cache;
