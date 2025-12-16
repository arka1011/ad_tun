cmake_minimum_required(VERSION 3.16)

project(ad_tun
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---- SOURCES ----
set(INIH_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../prebuilt/inih/src/ini.c
)

add_library(ad_tun SHARED
    src/ad_tun.c
    ${INIH_SRC}
)

# ---- INCLUDE DIRS ----
target_include_directories(ad_tun
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../prebuilt/inih/include
)

# ---- LINK LIBRARIES ----
target_link_libraries(ad_tun
    PUBLIC ad_logger
)

enable_testing()
add_subdirectory(tests)
