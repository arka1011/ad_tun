# ---- TEST SOURCE FILES ----
add_executable(ad_tun_tests
    test_main.cpp
    test_config.cpp
    test_state.cpp
    test_io.cpp
    # Additional test source files can be added here
)

# ---- ADD INCLUDE DIRS ----
target_include_directories(ad_tun_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../prebuilt/inih/include
    ${CMAKE_SOURCE_DIR}/../prebuilt/zlog/include
    ${CMAKE_SOURCE_DIR}/../prebuilt/googletest/googletest/include
    # Additional include directories can be added here
)

# ---- IMPORT LIBRARIES START ----

add_library(libzlog SHARED IMPORTED)
set_target_properties(libzlog PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../prebuilt/zlog/lib/libzlog.so"
)

add_library(gtest STATIC IMPORTED)
set_target_properties(gtest PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../prebuilt/googletest/build/lib/libgtest.a"
)

add_library(gtest_main STATIC IMPORTED)
set_target_properties(gtest_main PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../prebuilt/googletest/build/lib/libgtest_main.a"
)

# additional libraries can be added here

# ---- IMPORT LIBRARIES END ----

# ---- LINK LIBRARIES ----
target_link_libraries(ad_tun_tests PRIVATE
    ad_tun
    libzlog
    gtest
    pthread
)

add_test(NAME ad_tun_tests COMMAND ad_tun_tests)
