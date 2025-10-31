#pragma once

// ============================================================================
// Server configuration
// ============================================================================
#define APP_LISTEN_URL "http://0.0.0.0:8000"  //!< URL to listen on
#define APP_POLL_TIMEOUT_MS 60000                //!< Poll timeout in milliseconds (reduced for better responsiveness)

// ============================================================================
// Mongoose configuration - Optimized for high RPS
// ============================================================================

// TLS Configuration
#define MG_TLS MG_TLS_OPENSSL                 //!< Use openssl for TLS

// Enable required features
#define MG_ENABLE_MD5 1                       //!< Enable native MD5 (for ETags)
#define MG_ENABLE_LINES 1                     //!< Show file in logs

// Buffer sizes - Optimized for static file serving
// Larger IO size reduces system calls and improves throughput
#define MG_IO_SIZE 65536                      //!< IO buffer growth granularity (64KB, optimal for most files)
#define MG_MAX_RECV_SIZE 8192                 //!< Maximum recv buffer size (8KB, sufficient for HTTP headers)

// HTTP Configuration
#define MG_MAX_HTTP_HEADERS 20                //!< Maximum number of HTTP headers
#define MG_HTTP_DIRLIST_TIME_FMT "%Y-%m-%d %H:%M:%S"  //!< Directory listing time format

// Enable performance features
#define MG_ENABLE_PACKED_FS 0                 //!< Disable packed FS (using regular FS)
#define MG_ENABLE_SSI 0                       //!< Disable SSI for better performance

// ============================================================================
// Cache configuration - Optimized for high RPS static file serving
// ============================================================================
#define CACHE_TTL_MS (5 * 60 * 1000)          //!< Cache Time-To-Live: 5 minutes
#define CACHE_MAX_SIZE_MB 100                 //!< Maximum cache size: 100 MB (increased for better hit rate)

// ============================================================================
// Performance tuning notes:
// ============================================================================
// 1. Keep-Alive: Enabled in code to reuse TCP connections
// 2. ETag Support: Implemented for conditional requests (304 responses)
// 3. Cache-Control: Set to public, max-age=300 for client-side caching
// 4. Buffer Sizes: Optimized for typical static file sizes
// 5. Poll Timeout: Reduced to 50ms for better responsiveness
// 6. Memory Caching: Implemented with TTL and size-based eviction
// 7. Connection Reuse: Connection: keep-alive header added
// ============================================================================
