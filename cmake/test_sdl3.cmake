cmake_minimum_required(VERSION 3.15)
project(TestSDL3 C)

if(NOT EXISTS "/home/ericsonwillians/workspace/TuxArena/third_party/SDL/CMakeLists.txt")
    message(FATAL_ERROR "SDL3 CMakeLists.txt not found")
endif()

# Test if we can configure SDL3 without errors
try_compile(SDL3_CONFIGURE_TEST
    "${CMAKE_BINARY_DIR}/test_configure"
    "/home/ericsonwillians/workspace/TuxArena/third_party/SDL"
    TestSDL3Config
    CMAKE_FLAGS
        "-DSDL_SHARED=OFF"
        "-DSDL_STATIC=ON"
        "-DSDL_TEST=OFF"
        "-DSDL_TESTS=OFF"
    OUTPUT_VARIABLE SDL3_CONFIG_OUTPUT
)

if(NOT SDL3_CONFIGURE_TEST)
    message(FATAL_ERROR "SDL3 configuration test failed: ${SDL3_CONFIG_OUTPUT}")
endif()
