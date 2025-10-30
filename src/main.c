#include "server/config.h"
#include <mongoose.h>
#include <string.h>
#include <time.h>

#include "plugins/CacheManager.h"

// Configuration
#ifndef APP_LISTEN_URL
#define APP_LISTEN_URL "http://0.0.0.0:8000"
#endif
#ifndef APP_POLL_TIMEOUT_MS
#define APP_POLL_TIMEOUT_MS 5000
#endif

struct CacheBucket g_cache = {0};

void serve_cached_file(struct mg_connection *c, struct mg_http_message *hm) {
  char file_path[256];
  snprintf(file_path, sizeof(file_path), ".%.*s", (int)hm->uri.len, hm->uri.buf);
  
  struct CacheEntry *entry = NULL;
  if (GH_CacheExists(&g_cache, file_path)) {
    entry = GH_CacheGetByPath(&g_cache, file_path);
    // Check TTL
    uint16_t current_time = (uint16_t)(time(NULL) / 60);
    if (current_time - entry->timestamp > (CACHE_TTL_MS / 60000)) {
      // Expired
      MG_INFO(("Cache expired for %s", file_path));
      entry = NULL;
    } else {
      // Serve from cache
      MG_INFO(("Serving from cache: %s", file_path));
      mg_http_reply(c, 200, "Content-Type: application/octet-stream\r\n", 
                    "%.*s", (int)entry->data.len, entry->data.buf);
      return;
    }
  }
  
  // Load from disk
  FILE *fp = fopen(file_path, "rb");
  if (fp == NULL) {
    mg_http_reply(c, 404, "", "File not found\n");
    return;
  }
  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  
  char *data = (char *)malloc(fsize);
  fread(data, 1, fsize, fp);
  fclose(fp);
  
  // Cache the file
  struct mg_str mg_data = mg_str_n(data, fsize);
  struct CacheEntry new_entry = {
    .path = strdup(file_path),
    .data = mg_data,
    .timestamp = (uint16_t)(time(NULL) / 60),
    .next = NULL
  };
  GH_CacheAdd(&g_cache, new_entry);
  
  // Serve the file
  MG_INFO(("Serving from disk and caching: %s", file_path));
  mg_http_reply(c, 200, "Content-Type: application/octet-stream\r\n", 
                "%.*s", (int)mg_data.len, mg_data.buf);
  
  free(data);
}

// Event handler
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = ev_data;
    serve_cached_file(c, hm);
  }
}

int main(void) {
  struct mg_mgr mgr;
  
  // Initialize event manager
  mg_mgr_init(&mgr);
  
  // Create listening connection
  struct mg_connection *ln = mg_http_listen(&mgr, APP_LISTEN_URL, ev_handler, NULL);
  if (ln == NULL) {
    MG_ERROR(("Failed to create listener on %s", APP_LISTEN_URL));
    return 1;
  }
  
  // Setup cleanup timer (every 60 seconds)
  mg_timer_add(&mgr, 60000, MG_TIMER_REPEAT, GH_CacheCleanup, NULL);

  MG_INFO(("Static file server with cache started on %s", APP_LISTEN_URL));
  MG_INFO(("Cache config: TTL=%d min, Max size=%d MB", 
           CACHE_TTL_MS/60000, CACHE_MAX_SIZE_MB));
  MG_INFO(("Press Ctrl+C to stop"));
  
  // Event loop
  for (;;) {
    mg_mgr_poll(&mgr, APP_POLL_TIMEOUT_MS);
  }
  
  // Cleanup
  GH_CacheCleanup(&g_cache);
  mg_mgr_free(&mgr);
  return 0;
}
