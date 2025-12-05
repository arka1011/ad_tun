cmake_minimum_required(VERSION 3.16)
project(ad_tun C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source files
set(INIH_SRC
    ${CMAKE_SOURCE_DIR}/../prebuilt/inih/src/ini.c
)

set(AD_TUN_SOURCES
    src/ad_tun.c
    ${INIH_SRC}
)

# ---- CREATE THE TARGET FIRST ----
add_library(ad_tun SHARED ${AD_TUN_SOURCES})

# ---- NOW SAFE TO ADD INCLUDE DIRS ----
target_include_directories(ad_tun PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../prebuilt/inih/include
    ${CMAKE_SOURCE_DIR}/../prebuilt/zlog/include
)

# ---- LINK LIBRARIES ----
target_link_libraries(ad_tun
    ${CMAKE_SOURCE_DIR}/../prebuilt/zlog/libs/libzlog.so
)

# Optional install rules
install(TARGETS ad_tun DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
