#pragma once

#define MG_TLS MG_TLS_OPENSSL //!< Use openssl for lts
#define MG_ENABLE_MD5 1 //!< Enable native MD5
// #define MG_ENABLE_CUSTOM_CALLOC 1
#define MG_ENABLE_LINES 1 //!< Show file in logs
#define MG_IO_SIZE 524288 //!< Granularity of the send/recv IO buffer growth
#define MG_MAX_RECV_SIZE 16424 //!< Maximum recv buffer size
