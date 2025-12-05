cmake_minimum_required(VERSION 3.16)
project(ad_tun C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include dirs
target_include_directories(ad_tun PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../prebuilt/inih/include
    ${CMAKE_SOURCE_DIR}/../prebuilt/zlog/include
)

# INIH source
set(INIH_SRC
    ${CMAKE_SOURCE_DIR}/../prebuilt/inih/src/ini.c
)

# ad_tun sources
set(AD_TUN_SOURCES
    src/ad_tun.c
    ${INIH_SRC}
)

# Create shared lib
add_library(ad_tun SHARED ${AD_TUN_SOURCES})

# Link zlog
target_link_libraries(ad_tun
    ${CMAKE_SOURCE_DIR}/../prebuilt/zlog/lib/libzlog.so
)

# Install (optional)
install(TARGETS ad_tun DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
