#include "server/config.h"
#include <mongoose.h>
#include <string.h>
#include <time.h>

// Configuration
#define LISTEN_URL "http://0.0.0.0:8000"
#define POLL_TIMEOUT_MS 5000
#define CACHE_TTL_MS (5 * 60 * 1000)  // 5 minutes in milliseconds
#define MAX_CACHE_ENTRIES 100
#define MAX_CACHE_SIZE (50 * 1024 * 1024)  // 50MB total cache

// Cache entry structure
struct cache_entry {
  char *path;                    // File path (key)
  struct mg_str data;            // File content (value)
  uint64_t timestamp;            // Last access time
  size_t hits;                   // Access count
  struct cache_entry *next;      // Linked list
};

// Cache bucket structure
struct file_cache {
  struct cache_entry *entries;   // Linked list of cache entries
  size_t total_size;             // Total memory used
  size_t entry_count;            // Number of entries
  struct mg_fs *fs;              // Filesystem to use
};

static struct file_cache g_cache = {NULL, 0, 0, &mg_fs_posix};

// Calculate hash for simple distribution
static unsigned int hash_string(const char *str) {
  unsigned int hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

// Find cache entry
static struct cache_entry *cache_find(const char *path) {
  struct cache_entry *entry = g_cache.entries;
  uint64_t now = mg_millis();
  
  while (entry != NULL) {
    // Check if entry is expired
    if ((now - entry->timestamp) > CACHE_TTL_MS) {
      // Mark as expired, will be cleaned up later
      entry = entry->next;
      continue;
    }
    
    if (strcmp(entry->path, path) == 0) {
      entry->timestamp = now;  // Update access time
      entry->hits++;
      MG_INFO(("Cache HIT: %s (hits: %lu)", path, (unsigned long)entry->hits));
      return entry;
    }
    entry = entry->next;
  }
  
  MG_INFO(("Cache MISS: %s", path));
  return NULL;
}

// Add entry to cache
static bool cache_add(const char *path, struct mg_str data) {
  // Check cache limits
  if (g_cache.entry_count >= MAX_CACHE_ENTRIES) {
    MG_INFO(("Cache full (entries), cannot add: %s", path));
    return false;
  }
  
  if (g_cache.total_size + data.len > MAX_CACHE_SIZE) {
    MG_INFO(("Cache full (size), cannot add: %s", path));
    return false;
  }
  
  // Create new entry
  struct cache_entry *entry = (struct cache_entry *)mg_calloc(1, sizeof(*entry));
  if (entry == NULL) {
    MG_ERROR(("Failed to allocate cache entry"));
    return false;
  }
  
  // Duplicate path
  entry->path = mg_mprintf("%s", path);
  if (entry->path == NULL) {
    mg_free(entry);
    return false;
  }
  
  // Duplicate data using mg_strdup
  entry->data = mg_strdup(data);
  if (entry->data.buf == NULL) {
    mg_free(entry->path);
    mg_free(entry);
    return false;
  }
  
  entry->timestamp = mg_millis();
  entry->hits = 1;
  
  // Add to linked list (at head)
  entry->next = g_cache.entries;
  g_cache.entries = entry;
  
  g_cache.total_size += data.len;
  g_cache.entry_count++;
  
  MG_INFO(("Cache ADD: %s (%lu bytes, total: %lu/%lu entries)", 
           path, (unsigned long)data.len, 
           (unsigned long)g_cache.entry_count, (unsigned long)MAX_CACHE_ENTRIES));
  
  return true;
}

// Clean expired entries
static void cache_cleanup(void) {
  struct cache_entry *entry = g_cache.entries;
  struct cache_entry *prev = NULL;
  uint64_t now = mg_millis();
  size_t removed = 0;
  
  while (entry != NULL) {
    if ((now - entry->timestamp) > CACHE_TTL_MS) {
      // Remove this entry
      struct cache_entry *to_remove = entry;
      
      if (prev == NULL) {
        g_cache.entries = entry->next;
      } else {
        prev->next = entry->next;
      }
      
      entry = entry->next;
      
      // Free memory
      g_cache.total_size -= to_remove->data.len;
      g_cache.entry_count--;
      
      mg_free((void *)to_remove->data.buf);
      mg_free(to_remove->path);
      mg_free(to_remove);
      removed++;
    } else {
      prev = entry;
      entry = entry->next;
    }
  }
  
  if (removed > 0) {
    MG_INFO(("Cache cleanup: removed %lu expired entries", (unsigned long)removed));
  }
}

// Timer callback for periodic cleanup
static void cleanup_timer(void *arg) {
  (void)arg;
  cache_cleanup();
}

// Serve file from cache or filesystem
static void serve_cached_file(struct mg_connection *c, struct mg_http_message *hm) {
  char path[256];
  struct mg_http_serve_opts opts = {
    .root_dir = ".",
    .fs = &mg_fs_posix
  };

  // Build full path
  mg_snprintf(path, sizeof(path), ".%.*s", (int)hm->uri.len, hm->uri.buf);

  // Sanitize path
  if (!mg_path_is_sane(mg_str(path))) {
    mg_http_reply(c, 400, "", "Bad Request\n");
    return;
  }

  // Try to find in cache
  struct cache_entry *cached = cache_find(path);
  if (cached != NULL) {
    // Serve from cache using mg_http_reply
    const char *mime = "application/octet-stream";
    const char *cache_header = "public, max-age=300\r\nX-Cache: HIT"; // Extra headers

    // Simple MIME type detection
    if (mg_match(hm->uri, mg_str("*.html"), NULL)) mime = "text/html";
    else if (mg_match(hm->uri, mg_str("*.css"), NULL)) mime = "text/css";
    else if (mg_match(hm->uri, mg_str("*.js"), NULL)) mime = "application/javascript";
    else if (mg_match(hm->uri, mg_str("*.json"), NULL)) mime = "application/json";
    else if (mg_match(hm->uri, mg_str("*.png"), NULL)) mime = "image/png";
    else if (mg_match(hm->uri, mg_str("*.jpg"), NULL)) mime = "image/jpeg";
    else if (mg_match(hm->uri, mg_str("*.gif"), NULL)) mime = "image/gif";
    else if (mg_match(hm->uri, mg_str("*.svg"), NULL)) mime = "image/svg+xml";
    else if (mg_match(hm->uri, mg_str("*.ico"), NULL)) mime = "image/x-icon";

    // Use mg_http_reply to send headers and then the body
    mg_http_reply(c, 200, cache_header, "%.*s", (int) cached->data.len, cached->data.buf);
    // mg_http_reply handles sending the body data correctly
    return; // Exit after sending cached response
  }

  // Not in cache, read from filesystem
  struct mg_str file_data = mg_file_read(opts.fs, path);

  if (file_data.buf == NULL) {
    // File not found or error, use default handler
    MG_DEBUG(("File not found or error: %s", path));
    mg_http_serve_dir(c, hm, &opts); // This function handles its own response
    return; // Exit after serving directory/file not found
  }

  // Add to cache (for files < 1MB) - only cache if read was successful
  if (file_data.len < (1024 * 1024)) {
    cache_add(path, file_data);
  }

  // Serve the file read from FS using mg_http_reply
  const char *mime_fs = "application/octet-stream";
  const char *cache_header_fs = "public, max-age=300\r\nX-Cache: MISS"; // Extra headers

  if (mg_match(hm->uri, mg_str("*.html"), NULL)) mime_fs = "text/html";
  else if (mg_match(hm->uri, mg_str("*.css"), NULL)) mime_fs = "text/css";
  else if (mg_match(hm->uri, mg_str("*.js"), NULL)) mime_fs = "application/javascript";
  else if (mg_match(hm->uri, mg_str("*.json"), NULL)) mime_fs = "application/json";
  else if (mg_match(hm->uri, mg_str("*.png"), NULL)) mime_fs = "image/png";
  else if (mg_match(hm->uri, mg_str("*.jpg"), NULL)) mime_fs = "image/jpeg";
  else if (mg_match(hm->uri, mg_str("*.gif"), NULL)) mime_fs = "image/gif";
  else if (mg_match(hm->uri, mg_str("*.svg"), NULL)) mime_fs = "image/svg+xml";
  else if (mg_match(hm->uri, mg_str("*.ico"), NULL)) mime_fs = "image/x-icon";

  // Use mg_http_reply to send headers and then the body read from FS
  mg_http_reply(c, 200, cache_header_fs, "%.*s", (int) file_data.len, file_data.buf);

  // Free the file data read from FS after sending the reply
  mg_free((void *)file_data.buf);
}

// Event handler
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = ev_data;
    serve_cached_file(c, hm);
  }
}

// Cleanup all cache on exit
static void cache_destroy(void) {
  struct cache_entry *entry = g_cache.entries;
  while (entry != NULL) {
    struct cache_entry *next = entry->next;
    mg_free((void *)entry->data.buf);
    mg_free(entry->path);
    mg_free(entry);
    entry = next;
  }
  g_cache.entries = NULL;
  g_cache.total_size = 0;
  g_cache.entry_count = 0;
  MG_INFO(("Cache destroyed"));
}

int main(void) {
  struct mg_mgr mgr;
  
  // Initialize event manager
  mg_mgr_init(&mgr);
  
  // Create listening connection
  struct mg_connection *ln = mg_http_listen(&mgr, LISTEN_URL, ev_handler, NULL);
  if (ln == NULL) {
    MG_ERROR(("Failed to create listener on %s", LISTEN_URL));
    return 1;
  }
  
  // Setup cleanup timer (every 60 seconds)
  mg_timer_add(&mgr, 60000, MG_TIMER_REPEAT, cleanup_timer, NULL);
  
  MG_INFO(("Static file server with cache started on %s", LISTEN_URL));
  MG_INFO(("Cache config: TTL=%d min, Max entries=%d, Max size=%d MB", 
           CACHE_TTL_MS/60000, MAX_CACHE_ENTRIES, MAX_CACHE_SIZE/(1024*1024)));
  MG_INFO(("Press Ctrl+C to stop"));
  
  // Event loop
  for (;;) {
    mg_mgr_poll(&mgr, POLL_TIMEOUT_MS);
  }
  
  // Cleanup
  cache_destroy();
  mg_mgr_free(&mgr);
  return 0;
}
