#include "server/config.h"
#include <mongoose.h>

// Event handler
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_HDRS) {
    struct mg_http_message *hm = ev_data;
    struct mg_http_serve_opts opts = {
      .root_dir = "." // Serve local directory
    };
    mg_http_serve_dir(c, ev_data, &opts);
  }
}

int main() {
  struct mg_mgr mgr; // Event manager
  mg_mgr_init(&mgr); // Initialise event manager

  // Setup listener
  mg_http_listen(&mgr, "http://localhost:8000", ev_handler, NULL);

  MG_INFO(("Listening on %s", "http://localhost:8000"));
  // Event loop
  for (;;) {
    mg_mgr_poll(&mgr, 5000); // Poll events every 1000ms
  }

  // Cleanup (never reached in this infinite loop)
  mg_mgr_free(&mgr);
  return 0;
}
