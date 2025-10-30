cmake_minimum_required(VERSION 3.10)
project(GrowplusHttp C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "-O3 -pipe -flto -funroll-loops -fomit-frame-pointer -march=armv8-a -mtune=cortex-a75 -pthread -DMG_ENABLE_FSEEKO=1 -DMG_ENABLE_THREADS=1")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

include_directories(
  src/
  externals/mongoose
)

add_executable(GrowplusHttp
  src/main.c
  src/plugins/CacheManager.c

  externals/mongoose/mongoose.c
)

target_precompile_headers(GrowplusHttp PRIVATE
  src/server/config.h
)

# target_compile_definitions(GrowplusHttp PRIVATE
  # MG_IO_SIZE=1000
  # )

find_package(OpenSSL REQUIRED)
target_link_libraries(GrowplusHttp PRIVATE OpenSSL::SSL OpenSSL::Crypto)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(GrowplusHttp PRIVATE
        -Wall
        -Wextra
        -O2
        -D_FORTIFY_SOURCE=2
    )
  target_link_options(GrowplusHttp PRIVATE -Wl,-z,now,-z,relro)
endif()
