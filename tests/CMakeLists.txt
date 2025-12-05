# GoogleTest
cmake_policy(SET CMP0135 NEW)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

FetchContent_MakeAvailable(googletest)

# Unit test executable
add_executable(ad_tun_tests
    test_main.cpp
    test_config.cpp
    test_state.cpp
    test_io.cpp
)

target_include_directories(ad_tun_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ad_tun_tests
    ad_tun
    gtest
    gtest_main
    pthread
)

add_test(NAME ad_tun_tests COMMAND ad_tun_tests)
