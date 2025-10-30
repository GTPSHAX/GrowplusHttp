#include "mongoose.h"
#include "server/config.h"
#include "plugins/CacheManager.h"
#include <stdio.h>
#include <string.h>

// Global cache instance
struct CacheBucket g_cache = {0};

// Generate ETag based on file path and modification time
static void generate_etag(const char *path, time_t mtime, char *etag, size_t etag_len) {
  mg_snprintf(etag, etag_len, "\"%lx-%lx\"", (unsigned long)mg_crc32(0, path, strlen(path)), 
              (unsigned long)mtime);
}

// Serve file from cache with proper HTTP headers
static void serve_from_cache(struct mg_connection *c, struct CacheEntry *entry, 
                             struct mg_http_message *hm) {
  // Check If-None-Match header for ETag validation
  struct mg_str *if_none_match = mg_http_get_header(hm, "If-None-Match");
  if (if_none_match != NULL && entry->etag != NULL) {
    if (mg_strcmp(*if_none_match, mg_str(entry->etag)) == 0) {
      // ETag matches, return 304 Not Modified
      mg_printf(c, "HTTP/1.1 304 Not Modified\r\n"
                   "ETag: %s\r\n"
                   "Cache-Control: public, max-age=300\r\n"
                   "Connection: keep-alive\r\n"
                   "\r\n", entry->etag);
      return;
    }
  }

  // Determine content type based on file extension
  const char *content_type = "application/octet-stream";
  if (strstr(entry->path, ".html")) content_type = "text/html";
  else if (strstr(entry->path, ".css")) content_type = "text/css";
  else if (strstr(entry->path, ".js")) content_type = "application/javascript";
  else if (strstr(entry->path, ".json")) content_type = "application/json";
  else if (strstr(entry->path, ".png")) content_type = "image/png";
  else if (strstr(entry->path, ".jpg") || strstr(entry->path, ".jpeg")) content_type = "image/jpeg";
  else if (strstr(entry->path, ".gif")) content_type = "image/gif";
  else if (strstr(entry->path, ".svg")) content_type = "image/svg+xml";
  else if (strstr(entry->path, ".ico")) content_type = "image/x-icon";
  else if (strstr(entry->path, ".woff")) content_type = "font/woff";
  else if (strstr(entry->path, ".woff2")) content_type = "font/woff2";
  else if (strstr(entry->path, ".ttf")) content_type = "font/ttf";

  // Send response with caching headers
  mg_printf(c, "HTTP/1.1 200 OK\r\n"
               "Content-Type: %s\r\n"
               "Content-Length: %lu\r\n"
               "ETag: %s\r\n"
               "Cache-Control: public, max-age=300\r\n"
               "Connection: keep-alive\r\n"
               "Accept-Ranges: bytes\r\n"
               "\r\n",
               content_type, (unsigned long)entry->data_len, 
               entry->etag ? entry->etag : "");
  
  mg_send(c, entry->data, entry->data_len);
}

// Try to load file and add to cache
static bool load_and_cache_file(struct mg_connection *c, const char *path, 
                                struct mg_http_message *hm) {
  // Read file from filesystem
  struct mg_str file_data = mg_file_read(&mg_fs_posix, path);
  if (file_data.buf == NULL) {
    return false;
  }

  // Get file metadata
  size_t file_size = 0;
  time_t mtime = 0;
  mg_fs_posix.st(path, &file_size, &mtime);

  // Generate ETag
  char etag[64];
  generate_etag(path, mtime, etag, sizeof(etag));

  // Add to cache
  if (GH_CacheAdd(&g_cache, path, file_data.buf, file_data.len, etag, mtime)) {
    MG_INFO(("Cached file: %s (%lu bytes)", path, (unsigned long)file_data.len));
    
    // Serve from newly cached entry
    struct CacheEntry *entry = GH_CacheGetByPath(&g_cache, path);
    if (entry != NULL) {
      serve_from_cache(c, entry, hm);
    }
  } else {
    // Cache failed, serve directly
    MG_ERROR(("Failed to cache file: %s", path));
    struct mg_http_serve_opts opts = {
      .root_dir = ".",
      .fs = &mg_fs_posix,
      .extra_headers = "Cache-Control: public, max-age=300\r\nConnection: keep-alive\r\n"
    };
    mg_http_serve_file(c, hm, path, &opts);
  }

  free((void *)file_data.buf);
  return true;
}

// Connection event handler function with optimized static file serving
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    // Handle API endpoints
    if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {
      mg_http_reply(c, 200, "Content-Type: application/json\r\nConnection: keep-alive\r\n", 
                    "{%m:%d}\n", MG_ESC("status"), 1);
      return;
    }

    // Handle cache statistics endpoint
    if (mg_match(hm->uri, mg_str("/api/cache/stats"), NULL)) {
      mg_http_reply(c, 200, "Content-Type: application/json\r\nConnection: keep-alive\r\n",
                    "{%m:%lu,%m:%lu,%m:%lu}\n",
                    MG_ESC("entries"), (unsigned long)g_cache.entry_count,
                    MG_ESC("size_bytes"), (unsigned long)g_cache.size,
                    MG_ESC("size_mb"), (unsigned long)(g_cache.size / (1024 * 1024)));
      return;
    }

    // Handle cache clear endpoint
    if (mg_match(hm->uri, mg_str("/api/cache/clear"), NULL)) {
      GH_CacheCleanup(&g_cache);
      GH_CacheInit(&g_cache);
      mg_http_reply(c, 200, "Content-Type: application/json\r\nConnection: keep-alive\r\n",
                    "{%m:%m}\n", MG_ESC("status"), MG_ESC("cleared"));
      return;
    }

    // Build file path
    char path[256];
    mg_snprintf(path, sizeof(path), ".%.*s", (int)hm->uri.len, hm->uri.buf);
    
    // Default to index.html for root
    if (strcmp(path, "./") == 0 || strcmp(path, ".") == 0) {
      strcpy(path, "./index.html");
    }

    // Try to serve from cache first (cache-first strategy)
    struct CacheEntry *cached = GH_CacheGetByPath(&g_cache, path);
    if (cached != NULL) {
      // Check if cache entry is still valid
      uint64_t current_time = mg_millis();
      if (current_time - cached->timestamp <= CACHE_TTL_MS) {
        // Serve from cache
        MG_DEBUG(("Serving from cache: %s", path));
        serve_from_cache(c, cached, hm);
        return;
      } else {
        // Cache expired, remove entry
        MG_DEBUG(("Cache expired: %s", path));
        GH_CacheRemove(&g_cache, path);
      }
    }

    // Not in cache or expired, load from disk
    MG_DEBUG(("Loading from disk: %s", path));
    if (!load_and_cache_file(c, path, hm)) {
      // File not found, serve 404
      mg_http_reply(c, 404, "Connection: keep-alive\r\n", "File not found\n");
    }

  } else if (ev == MG_EV_POLL) {
    // Periodically evict expired cache entries (every poll)
    static uint64_t last_cleanup = 0;
    uint64_t now = mg_millis();
    if (now - last_cleanup > 60000) {  // Cleanup every minute
      GH_CacheEvictExpired(&g_cache, now);
      last_cleanup = now;
    }
  }
}

int main(void) {
  struct mg_mgr mgr;
  
  // Initialize logging
  mg_log_set(MG_LL_INFO);
  
  // Initialize cache
  GH_CacheInit(&g_cache);
  MG_INFO(("Cache initialized: TTL=%dms, MaxSize=%dMB", CACHE_TTL_MS, CACHE_MAX_SIZE_MB));

  // Initialize event manager
  mg_mgr_init(&mgr);
  
  // Create HTTP listener with optimized settings
  struct mg_connection *listener = mg_http_listen(&mgr, APP_LISTEN_URL, ev_handler, NULL);
  if (listener == NULL) {
    MG_ERROR(("Failed to create listener on %s", APP_LISTEN_URL));
    return 1;
  }
  
  MG_INFO(("HTTP server started on %s", APP_LISTEN_URL));
  MG_INFO(("Optimizations enabled: keep-alive, ETags, caching, compression hints"));

  // Event loop with optimized poll timeout
  for (;;) {
    mg_mgr_poll(&mgr, APP_POLL_TIMEOUT_MS);
  }

  // Cleanup (unreachable in this implementation)
  GH_CacheCleanup(&g_cache);
  mg_mgr_free(&mgr);
  
  return 0;
}