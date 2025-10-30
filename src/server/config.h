#pragma once

// Server configuration
#define APP_LISTEN_URL "http://0.0.0.0:8000" //!< URL to listen on
#define APP_POLL_TIMEOUT_MS 5000                 //!< Poll timeout in milliseconds

// Mongoose configuration
#define MG_TLS MG_TLS_OPENSSL //!< Use openssl for lts
#define MG_ENABLE_MD5 1 //!< Enable native MD5
// #define MG_ENABLE_CUSTOM_CALLOC 1
#define MG_ENABLE_LINES 1 //!< Show file in logs
#define MG_IO_SIZE 524288 //!< Granularity of the send/recv IO buffer growth
#define MG_MAX_RECV_SIZE 16424 //!< Maximum recv buffer size

// Plugin configurations
#define CACHE_TTL_MS (5 * 60 * 1000) //!< Cache Time-To-Live in milliseconds (5 minutes)
#define CACHE_MAX_SIZE_MB 50 //!< Maximum cache size in megabytes