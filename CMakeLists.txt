cmake_minimum_required(VERSION 3.16)

project(ad_tun 
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source files
set(INIH_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../prebuilt/inih/src/ini.c
)

set(AD_TUN_SOURCES
    src/ad_tun.c
    ${INIH_SRC}
)

# ---- CREATE THE TARGET FIRST ----
add_library(ad_tun SHARED ${AD_TUN_SOURCES})

# ---- ADD INCLUDE DIRS ----
target_include_directories(ad_tun PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../prebuilt/inih/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ad_logger/include)

# ---- LINK LIBRARIES ----
target_link_libraries(ad_tun
    ${CMAKE_CURRENT_SOURCE_DIR}/../ad_logger/build/lib/libad_logger.so
)

enable_testing()
add_subdirectory(tests)

