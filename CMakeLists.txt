cmake_minimum_required(VERSION 3.20)

# ============================================================================
# PROJECT METADATA AND CONFIGURATION
# ============================================================================

project(TuxArena
    VERSION 1.0.0
    DESCRIPTION "Advanced 2D Arena Combat Game with Networking Support"
    HOMEPAGE_URL "https://github.com/ericsonwillians/TuxArena"
    LANGUAGES CXX C
)

# Set policies for modern CMake behavior
cmake_policy(SET CMP0074 NEW) # Use <Package>_ROOT variables
cmake_policy(SET CMP0077 NEW) # option() honors normal variables
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW) # ExternalProject download timestamp
endif()
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW) # find_package uses upper-case <PACKAGENAME>_ROOT variables
endif()

# ============================================================================
# SYSTEM AND COMPILER DETECTION
# ============================================================================

# Detect system architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SYSTEM_ARCH "x64")
    set(IS_64BIT TRUE)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(SYSTEM_ARCH "x86")
    set(IS_64BIT FALSE)
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

# Enhanced compiler detection
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMPILER_IS_GCC TRUE)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
        message(FATAL_ERROR "GCC version 9.0 or higher required, found ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(COMPILER_IS_CLANG TRUE)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
        message(FATAL_ERROR "Clang version 10.0 or higher required, found ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Platform detection
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
    message(STATUS "Building for Linux platform (${SYSTEM_ARCH})")
else()
    message(FATAL_ERROR "Only Linux platforms are supported")
endif()

# ============================================================================
# BUILD CONFIGURATION AND OPTIONS
# ============================================================================

# Set default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options with better defaults
option(TUXARENA_FORCE_SDL3 "Force usage of SDL3 (fail if not available)" OFF)
option(TUXARENA_PREFER_SYSTEM_LIBS "Prefer system libraries over bundled ones" ON)
option(TUXARENA_ENABLE_NETWORKING "Enable multiplayer networking support" ON)
option(TUXARENA_ENABLE_AUDIO "Enable audio system" ON)
option(TUXARENA_STATIC_LINKING "Enable static linking for better portability" OFF)
option(TUXARENA_SKIP_BUNDLED_SDL "Skip bundled SDL completely (system only)" OFF)

# ============================================================================
# ENHANCED MODULE PATH AND SYSTEM DETECTION
# ============================================================================

# Enhanced module path setup
list(APPEND CMAKE_MODULE_PATH 
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

# Find essential packages
find_package(Threads REQUIRED)
find_package(PkgConfig QUIET)

# ============================================================================
# PROFESSIONAL OPENGL DETECTION WITH MULTIPLE LOADERS
# ============================================================================

message(STATUS "==================== OpenGL Detection ====================")

# Find standard OpenGL
find_package(OpenGL REQUIRED)
if(OpenGL_FOUND)
    message(STATUS "Found standard OpenGL: ${OPENGL_LIBRARIES}")
    if(OPENGL_INCLUDE_DIR)
        message(STATUS "Found OpenGL headers at: ${OPENGL_INCLUDE_DIR}")
    endif()
endif()

# Function to detect OpenGL implementation and loader
function(detect_opengl_implementation)
    set(OPENGL_LOADER_FOUND FALSE)
    set(OPENGL_LOADER_TYPE "NONE")
    set(OPENGL_LOADER_LIBRARY "")
    set(OPENGL_LOADER_INCLUDE "")
    
    message(STATUS "Searching for OpenGL loaders...")
    
    # Method 1: Try to find GLAD via find_package
    find_package(glad QUIET)
    if(glad_FOUND)
        set(OPENGL_LOADER_FOUND TRUE)
        set(OPENGL_LOADER_TYPE "GLAD")
        set(OPENGL_LOADER_LIBRARY "glad::glad")
        message(STATUS "Found GLAD via find_package")
    endif()
    
    if(NOT OPENGL_LOADER_FOUND)
        # Method 2: Try to find GLAD headers in system
        find_path(GLAD_INCLUDE_DIR
            NAMES glad/glad.h
            PATHS
                /usr/include
                /usr/local/include
                /opt/include
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include
        )
        
        if(GLAD_INCLUDE_DIR)
            find_library(GLAD_LIBRARY
                NAMES glad libglad
                PATHS
                    /usr/lib
                    /usr/local/lib
                    /opt/lib
                    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/lib
            )
            
            set(OPENGL_LOADER_FOUND TRUE)
            set(OPENGL_LOADER_TYPE "GLAD")
            set(OPENGL_LOADER_INCLUDE "${GLAD_INCLUDE_DIR}")
            if(GLAD_LIBRARY)
                set(OPENGL_LOADER_LIBRARY "${GLAD_LIBRARY}")
                message(STATUS "Found GLAD library: ${GLAD_LIBRARY}")
            else()
                message(STATUS "Found GLAD headers (header-only): ${GLAD_INCLUDE_DIR}")
            endif()
        endif()
    endif()
    
    if(NOT OPENGL_LOADER_FOUND)
        message(STATUS "GLAD not found, searching for GLEW...")
        
        # Method 3: Try GLEW via find_package
        find_package(GLEW QUIET)
        if(GLEW_FOUND)
            set(OPENGL_LOADER_FOUND TRUE)
            set(OPENGL_LOADER_TYPE "GLEW")
            if(TARGET GLEW::GLEW)
                set(OPENGL_LOADER_LIBRARY "GLEW::GLEW")
            else()
                set(OPENGL_LOADER_LIBRARY "${GLEW_LIBRARIES}")
                set(OPENGL_LOADER_INCLUDE "${GLEW_INCLUDE_DIRS}")
            endif()
            message(STATUS "Found GLEW as OpenGL loader: ${GLEW_VERSION}")
        endif()
    endif()
    
    if(NOT OPENGL_LOADER_FOUND)
        # Method 4: Try GLEW via pkg-config
        if(PkgConfig_FOUND)
            pkg_check_modules(GLEW_PC glew)
            if(GLEW_PC_FOUND)
                set(OPENGL_LOADER_FOUND TRUE)
                set(OPENGL_LOADER_TYPE "GLEW")
                set(OPENGL_LOADER_LIBRARY "${GLEW_PC_LIBRARIES}")
                set(OPENGL_LOADER_INCLUDE "${GLEW_PC_INCLUDE_DIRS}")
                message(STATUS "Found GLEW via pkg-config: ${GLEW_PC_VERSION}")
            endif()
        endif()
    endif()
    
    # Export results to parent scope
    set(OPENGL_LOADER_FOUND ${OPENGL_LOADER_FOUND} PARENT_SCOPE)
    set(OPENGL_LOADER_TYPE ${OPENGL_LOADER_TYPE} PARENT_SCOPE)
    set(OPENGL_LOADER_LIBRARY ${OPENGL_LOADER_LIBRARY} PARENT_SCOPE)
    set(OPENGL_LOADER_INCLUDE ${OPENGL_LOADER_INCLUDE} PARENT_SCOPE)
    
    if(NOT OPENGL_LOADER_FOUND)
        message(WARNING "No OpenGL loader (GLAD/GLEW) found - will use basic OpenGL headers")
        message(WARNING "This may cause compilation issues with modern OpenGL code")
    else()
        message(STATUS "Selected OpenGL loader: ${OPENGL_LOADER_TYPE}")
    endif()
endfunction()

# Detect OpenGL implementation
detect_opengl_implementation()

# ============================================================================
# X11 AND DISPLAY SYSTEM DETECTION
# ============================================================================

if(LINUX)
    find_package(X11)
    
    if(PkgConfig_FOUND)
        pkg_check_modules(WAYLAND wayland-client wayland-cursor wayland-egl)
    endif()
    
    if(X11_FOUND)
        message(STATUS "Found X11 support")
    endif()
    
    if(WAYLAND_FOUND)
        message(STATUS "Found Wayland support:")
        foreach(lib ${WAYLAND_LIBRARIES})
            message(STATUS "  - ${lib}")
        endforeach()
    endif()
    
    if(NOT X11_FOUND AND NOT WAYLAND_FOUND)
        message(WARNING "Neither X11 nor Wayland found - display may not work")
    endif()
endif()

# ============================================================================
# INTELLIGENT SDL DETECTION (SDL2 FIRST, THEN SDL3)
# ============================================================================

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(SDL_FOUND FALSE)
set(SDL_VERSION_MAJOR 0)
set(SDL_TARGET_NAME "")
set(SDL_IS_SDL3 FALSE)

# Function to find SDL2 using multiple methods
function(find_sdl2_robust)
    message(STATUS "Attempting to find SDL2...")
    
    # Method 1: Modern find_package with SDL2 target
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        message(STATUS "Found SDL2 via find_package: ${SDL2_VERSION}")
        
        if(TARGET SDL2::SDL2)
            set(SDL_TARGET_NAME "SDL2::SDL2" PARENT_SCOPE)
            set(SDL_FOUND TRUE PARENT_SCOPE)
            message(STATUS "Using SDL2::SDL2 target")
            return()
        elseif(TARGET SDL2::SDL2-static)
            set(SDL_TARGET_NAME "SDL2::SDL2-static" PARENT_SCOPE)
            set(SDL_FOUND TRUE PARENT_SCOPE)
            message(STATUS "Using SDL2::SDL2-static target")
            return()
        endif()
    endif()
    
    # Method 2: pkg-config
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_PC sdl2)
        if(SDL2_PC_FOUND)
            message(STATUS "Found SDL2 via pkg-config: ${SDL2_PC_VERSION}")
            
            # Create imported target for pkg-config SDL2
            add_library(SDL2_PKG INTERFACE IMPORTED)
            set_target_properties(SDL2_PKG PROPERTIES
                INTERFACE_LINK_LIBRARIES "${SDL2_PC_LIBRARIES}"
                INTERFACE_INCLUDE_DIRECTORIES "${SDL2_PC_INCLUDE_DIRS}"
                INTERFACE_COMPILE_OPTIONS "${SDL2_PC_CFLAGS_OTHER}"
            )
            
            # Add library directories
            if(SDL2_PC_LIBRARY_DIRS)
                set_target_properties(SDL2_PKG PROPERTIES
                    INTERFACE_LINK_DIRECTORIES "${SDL2_PC_LIBRARY_DIRS}"
                )
            endif()
            
            set(SDL_TARGET_NAME "SDL2_PKG" PARENT_SCOPE)
            set(SDL_FOUND TRUE PARENT_SCOPE)
            return()
        endif()
    endif()
    
    # Method 3: sdl2-config script
    find_program(SDL2_CONFIG_EXECUTABLE sdl2-config)
    if(SDL2_CONFIG_EXECUTABLE)
        message(STATUS "Found sdl2-config: ${SDL2_CONFIG_EXECUTABLE}")
        
        execute_process(
            COMMAND ${SDL2_CONFIG_EXECUTABLE} --version
            OUTPUT_VARIABLE SDL2_CONFIG_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        
        execute_process(
            COMMAND ${SDL2_CONFIG_EXECUTABLE} --cflags
            OUTPUT_VARIABLE SDL2_CONFIG_CFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        
        execute_process(
            COMMAND ${SDL2_CONFIG_EXECUTABLE} --libs
            OUTPUT_VARIABLE SDL2_CONFIG_LIBS
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        
        if(SDL2_CONFIG_VERSION AND SDL2_CONFIG_LIBS)
            message(STATUS "SDL2 version from sdl2-config: ${SDL2_CONFIG_VERSION}")
            
            # Parse flags and libraries
            string(REGEX REPLACE "^-I" "" SDL2_CONFIG_INCLUDE_DIRS "${SDL2_CONFIG_CFLAGS}")
            string(REGEX REPLACE " -I" ";" SDL2_CONFIG_INCLUDE_DIRS "${SDL2_CONFIG_INCLUDE_DIRS}")
            string(REGEX REPLACE " +" ";" SDL2_CONFIG_LIBS_LIST "${SDL2_CONFIG_LIBS}")
            
            # Create imported target
            add_library(SDL2_CONFIG INTERFACE IMPORTED)
            set_target_properties(SDL2_CONFIG PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${SDL2_CONFIG_INCLUDE_DIRS}"
            )
            
            # Parse libraries properly
            foreach(lib ${SDL2_CONFIG_LIBS_LIST})
                if(lib MATCHES "^-l(.+)")
                    set_property(TARGET SDL2_CONFIG APPEND PROPERTY
                        INTERFACE_LINK_LIBRARIES "${CMAKE_MATCH_1}")
                elseif(lib MATCHES "^-L(.+)")
                    set_property(TARGET SDL2_CONFIG APPEND PROPERTY
                        INTERFACE_LINK_DIRECTORIES "${CMAKE_MATCH_1}")
                elseif(lib MATCHES "^/.*")
                    # Absolute path to library
                    set_property(TARGET SDL2_CONFIG APPEND PROPERTY
                        INTERFACE_LINK_LIBRARIES "${lib}")
                endif()
            endforeach()
            
            set(SDL_TARGET_NAME "SDL2_CONFIG" PARENT_SCOPE)
            set(SDL_FOUND TRUE PARENT_SCOPE)
            return()
        endif()
    endif()
    
    # Method 4: Manual search
    find_path(SDL2_MANUAL_INCLUDE_DIR SDL.h
        PATHS
            /usr/include/SDL2
            /usr/local/include/SDL2
            /opt/include/SDL2
            /usr/include
            /usr/local/include
    )
    
    find_library(SDL2_MANUAL_LIBRARY
        NAMES SDL2 libSDL2
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/lib
            /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
            /usr/lib64
    )
    
    if(SDL2_MANUAL_INCLUDE_DIR AND SDL2_MANUAL_LIBRARY)
        message(STATUS "Found SDL2 manually:")
        message(STATUS "  Include: ${SDL2_MANUAL_INCLUDE_DIR}")
        message(STATUS "  Library: ${SDL2_MANUAL_LIBRARY}")
        
        # Create imported target
        add_library(SDL2_MANUAL INTERFACE IMPORTED)
        set_target_properties(SDL2_MANUAL PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MANUAL_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${SDL2_MANUAL_LIBRARY}"
        )
        
        # Add common SDL2 dependencies
        find_library(PTHREAD_LIB pthread)
        find_library(DL_LIB dl)
        find_library(M_LIB m)
        
        if(PTHREAD_LIB)
            set_property(TARGET SDL2_MANUAL APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${PTHREAD_LIB}")
        endif()
        if(DL_LIB)
            set_property(TARGET SDL2_MANUAL APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${DL_LIB}")
        endif()
        if(M_LIB)
            set_property(TARGET SDL2_MANUAL APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${M_LIB}")
        endif()
        
        set(SDL_TARGET_NAME "SDL2_MANUAL" PARENT_SCOPE)
        set(SDL_FOUND TRUE PARENT_SCOPE)
        return()
    endif()
    
    message(STATUS "SDL2 not found via any method")
    set(SDL_FOUND FALSE PARENT_SCOPE)
endfunction()

# Function to configure SDL
function(configure_sdl_library)
    # Strategy 1: Always try SDL2 first unless SDL3 is forced
    if(NOT TUXARENA_FORCE_SDL3)
        find_sdl2_robust()
        
        if(SDL_FOUND)
            set(SDL_VERSION_MAJOR 2 PARENT_SCOPE)
            set(SDL_IS_SDL3 FALSE PARENT_SCOPE)
            set(SDL_TARGET_NAME ${SDL_TARGET_NAME} PARENT_SCOPE)
            set(SDL_FOUND ${SDL_FOUND} PARENT_SCOPE)
            message(STATUS "Successfully configured SDL2: ${SDL_TARGET_NAME}")
            return()
        endif()
    endif()
    
    # Strategy 2: Try system SDL3 if SDL2 failed or SDL3 forced
    if(TUXARENA_PREFER_SYSTEM_LIBS OR TUXARENA_FORCE_SDL3)
        message(STATUS "Attempting to find system SDL3...")
        find_package(SDL3 QUIET)
        
        if(SDL3_FOUND)
            message(STATUS "Found system SDL3: ${SDL3_VERSION}")
            set(SDL_FOUND TRUE PARENT_SCOPE)
            set(SDL_VERSION_MAJOR 3 PARENT_SCOPE)
            set(SDL_TARGET_NAME "SDL3::SDL3" PARENT_SCOPE)
            set(SDL_IS_SDL3 TRUE PARENT_SCOPE)
            return()
        else()
            message(STATUS "System SDL3 not found")
        endif()
    endif()
    
    # Strategy 3: Try bundled SDL3 only if not skipped and SDL2 failed
    if(NOT TUXARENA_SKIP_BUNDLED_SDL AND NOT SDL_FOUND AND EXISTS "${THIRD_PARTY_DIR}/SDL")
        message(STATUS "Attempting to build bundled SDL3...")
        message(WARNING "Bundled SDL3 may have compatibility issues - recommend using system SDL2")
        
        # Clean any problematic variables
        unset(SDL2_INCLUDE_DIR CACHE)
        unset(SDL2_LIBRARY CACHE)
        unset(SDL2_DIR CACHE)
        
        # SDL3 build configuration with strict isolation
        set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as shared library")
        set(SDL_STATIC ON CACHE BOOL "Build SDL3 as static library")
        set(SDL_TEST OFF CACHE BOOL "Build SDL3 tests")
        set(SDL3_DISABLE_INSTALL ON CACHE BOOL "Disable SDL3 installation")
        set(SDL_TESTS OFF CACHE BOOL "Build SDL3 tests")
        set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL3 examples")
        
        # Platform-specific SDL options
        if(LINUX)
            set(SDL_X11 ${X11_FOUND} CACHE BOOL "Enable X11 support")
            set(SDL_WAYLAND ${WAYLAND_FOUND} CACHE BOOL "Enable Wayland support")
        endif()
        
        # Try to add SDL3 subdirectory with error handling
        try_compile(SDL3_TEST_COMPILE
            "${CMAKE_BINARY_DIR}/test_sdl3"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_sdl3.cmake"
            OUTPUT_VARIABLE SDL3_COMPILE_OUTPUT
        )
        
        if(NOT SDL3_TEST_COMPILE)
            message(WARNING "SDL3 bundled build test failed:")
            message(WARNING "${SDL3_COMPILE_OUTPUT}")
            message(STATUS "Skipping bundled SDL3 due to compatibility issues")
        else()
            # Add SDL3 subdirectory
            add_subdirectory(${THIRD_PARTY_DIR}/SDL ${CMAKE_BINARY_DIR}/_sdl_build)
            
            # Find SDL target
            set(SDL_POSSIBLE_TARGETS SDL3::SDL3 SDL3 SDL3-static)
            foreach(target ${SDL_POSSIBLE_TARGETS})
                if(TARGET ${target})
                    set(SDL_TARGET_NAME ${target} PARENT_SCOPE)
                    set(SDL_FOUND TRUE PARENT_SCOPE)
                    set(SDL_VERSION_MAJOR 3 PARENT_SCOPE)
                    set(SDL_IS_SDL3 TRUE PARENT_SCOPE)
                    message(STATUS "Using bundled SDL3 target: ${target}")
                    
                    # Create SDL3::SDL3 alias if needed
                    if(NOT TARGET SDL3::SDL3 AND NOT "${target}" STREQUAL "SDL3::SDL3")
                        add_library(SDL3::SDL3 ALIAS ${target})
                    endif()
                    
                    return()
                endif()
            endforeach()
            
            message(WARNING "Bundled SDL3 build completed but no usable targets found")
        endif()
    endif()
    
    # Strategy 4: Final fallback - try SDL2 again if we haven't yet
    if(NOT SDL_FOUND AND TUXARENA_FORCE_SDL3)
        message(STATUS "SDL3 forced but not found, making final attempt at SDL2...")
        find_sdl2_robust()
        
        if(SDL_FOUND)
            message(WARNING "SDL3 was forced but only SDL2 found - using SDL2")
            set(SDL_VERSION_MAJOR 2 PARENT_SCOPE)
            set(SDL_IS_SDL3 FALSE PARENT_SCOPE)
            set(SDL_TARGET_NAME ${SDL_TARGET_NAME} PARENT_SCOPE)
            set(SDL_FOUND ${SDL_FOUND} PARENT_SCOPE)
            return()
        endif()
    endif()
    
    # Final error
    if(TUXARENA_FORCE_SDL3)
        message(FATAL_ERROR "SDL3 was forced but not found in system or third_party")
    else()
        message(FATAL_ERROR "Neither SDL3 nor SDL2 could be found or built")
    endif()
endfunction()

# Create test file for SDL3 compatibility
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_sdl3.cmake" 
"cmake_minimum_required(VERSION 3.15)
project(TestSDL3 C)

if(NOT EXISTS \"${THIRD_PARTY_DIR}/SDL/CMakeLists.txt\")
    message(FATAL_ERROR \"SDL3 CMakeLists.txt not found\")
endif()

# Test if we can configure SDL3 without errors
try_compile(SDL3_CONFIGURE_TEST
    \"\${CMAKE_BINARY_DIR}/test_configure\"
    \"${THIRD_PARTY_DIR}/SDL\"
    TestSDL3Config
    CMAKE_FLAGS
        \"-DSDL_SHARED=OFF\"
        \"-DSDL_STATIC=ON\"
        \"-DSDL_TEST=OFF\"
        \"-DSDL_TESTS=OFF\"
    OUTPUT_VARIABLE SDL3_CONFIG_OUTPUT
)

if(NOT SDL3_CONFIGURE_TEST)
    message(FATAL_ERROR \"SDL3 configuration test failed: \${SDL3_CONFIG_OUTPUT}\")
endif()
")

# Configure SDL
message(STATUS "==================== SDL Detection ====================")
configure_sdl_library()

if(NOT SDL_FOUND)
    message(FATAL_ERROR "SDL configuration failed")
endif()

message(STATUS "Using SDL${SDL_VERSION_MAJOR} target: ${SDL_TARGET_NAME}")

# ============================================================================
# SDL EXTENSION LIBRARIES (NET, IMAGE, TTF) - SDL2 FOCUS
# ============================================================================

# Function to find SDL extension libraries with SDL2 priority
function(configure_sdl_extensions)
    # Configure networking
    if(TUXARENA_ENABLE_NETWORKING)
        message(STATUS "Configuring SDL networking...")
        
        set(SDL_NET_FOUND FALSE)
        set(SDL_NET_TARGET_NAME "")
        
        if(NOT SDL_IS_SDL3)
            # SDL2_net methods
            message(STATUS "Looking for SDL2_net...")
            
            # Method 1: find_package
            find_package(SDL2_net QUIET)
            if(SDL2_net_FOUND)
                set(SDL_NET_FOUND TRUE)
                if(TARGET SDL2_net::SDL2_net)
                    set(SDL_NET_TARGET_NAME "SDL2_net::SDL2_net")
                else()
                    # Create target for legacy SDL2_net
                    add_library(SDL2_net_LEGACY INTERFACE IMPORTED)
                    set_target_properties(SDL2_net_LEGACY PROPERTIES
                        INTERFACE_LINK_LIBRARIES "${SDL2_NET_LIBRARIES}"
                        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_NET_INCLUDE_DIRS}"
                    )
                    set(SDL_NET_TARGET_NAME "SDL2_net_LEGACY")
                endif()
                message(STATUS "Found SDL2_net via find_package")
            endif()
            
            # Method 2: pkg-config
            if(NOT SDL_NET_FOUND AND PkgConfig_FOUND)
                pkg_check_modules(SDL2_NET_PC SDL2_net)
                if(SDL2_NET_PC_FOUND)
                    add_library(SDL2_net_PKG INTERFACE IMPORTED)
                    set_target_properties(SDL2_net_PKG PROPERTIES
                        INTERFACE_LINK_LIBRARIES "${SDL2_NET_PC_LIBRARIES}"
                        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_NET_PC_INCLUDE_DIRS}"
                        INTERFACE_COMPILE_OPTIONS "${SDL2_NET_PC_CFLAGS_OTHER}"
                    )
                    if(SDL2_NET_PC_LIBRARY_DIRS)
                        set_target_properties(SDL2_net_PKG PROPERTIES
                            INTERFACE_LINK_DIRECTORIES "${SDL2_NET_PC_LIBRARY_DIRS}"
                        )
                    endif()
                    set(SDL_NET_TARGET_NAME "SDL2_net_PKG")
                    set(SDL_NET_FOUND TRUE)
                    message(STATUS "Found SDL2_net via pkg-config")
                endif()
            endif()
            
            # Method 3: Manual search
            if(NOT SDL_NET_FOUND)
                find_library(SDL2_NET_LIBRARY
                    NAMES SDL2_net libSDL2_net
                    PATHS /usr/lib /usr/local/lib /opt/lib /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                )
                find_path(SDL2_NET_INCLUDE_DIR SDL_net.h
                    PATHS /usr/include/SDL2 /usr/local/include/SDL2 /opt/include/SDL2
                )
                
                if(SDL2_NET_LIBRARY AND SDL2_NET_INCLUDE_DIR)
                    add_library(SDL2_net_MANUAL INTERFACE IMPORTED)
                    set_target_properties(SDL2_net_MANUAL PROPERTIES
                        INTERFACE_LINK_LIBRARIES "${SDL2_NET_LIBRARY}"
                        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_NET_INCLUDE_DIR}"
                    )
                    set(SDL_NET_TARGET_NAME "SDL2_net_MANUAL")
                    set(SDL_NET_FOUND TRUE)
                    message(STATUS "Found SDL2_net manually: ${SDL2_NET_LIBRARY}")
                endif()
            endif()
        else()
            # SDL3_net - only if using SDL3 and explicitly available
            if(TUXARENA_PREFER_SYSTEM_LIBS)
                find_package(SDL3_net QUIET)
                if(SDL3_net_FOUND)
                    set(SDL_NET_FOUND TRUE)
                    set(SDL_NET_TARGET_NAME "SDL3_net::SDL3_net")
                    message(STATUS "Found system SDL3_net")
                endif()
            endif()
        endif()
        
        if(SDL_NET_FOUND)
            set(SDL_NET_TARGET_NAME ${SDL_NET_TARGET_NAME} PARENT_SCOPE)
            set(SDL_NET_FOUND ${SDL_NET_FOUND} PARENT_SCOPE)
        else()
            message(WARNING "SDL networking library not found - disabling networking")
            set(TUXARENA_ENABLE_NETWORKING OFF PARENT_SCOPE)
            set(SDL_NET_FOUND FALSE PARENT_SCOPE)
        endif()
    endif()
    
    # Configure image loading with SDL2 priority
    message(STATUS "Configuring SDL image loading...")
    set(SDL_IMAGE_FOUND FALSE)
    set(SDL_IMAGE_TARGET_NAME "")
    
    if(NOT SDL_IS_SDL3)
        # SDL2_image methods
        message(STATUS "Looking for SDL2_image...")
        
        # Method 1: find_package
        find_package(SDL2_image QUIET)
        if(SDL2_image_FOUND)
            set(SDL_IMAGE_FOUND TRUE)
            if(TARGET SDL2_image::SDL2_image)
                set(SDL_IMAGE_TARGET_NAME "SDL2_image::SDL2_image")
            else()
                add_library(SDL2_image_LEGACY INTERFACE IMPORTED)
                set_target_properties(SDL2_image_LEGACY PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${SDL2_IMAGE_LIBRARIES}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_INCLUDE_DIRS}"
                )
                set(SDL_IMAGE_TARGET_NAME "SDL2_image_LEGACY")
            endif()
            message(STATUS "Found SDL2_image via find_package")
        endif()
        
        # Method 2: pkg-config
        if(NOT SDL_IMAGE_FOUND AND PkgConfig_FOUND)
            pkg_check_modules(SDL2_IMAGE_PC SDL2_image)
            if(SDL2_IMAGE_PC_FOUND)
                add_library(SDL2_image_PKG INTERFACE IMPORTED)
                set_target_properties(SDL2_image_PKG PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${SDL2_IMAGE_PC_LIBRARIES}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_PC_INCLUDE_DIRS}"
                    INTERFACE_COMPILE_OPTIONS "${SDL2_IMAGE_PC_CFLAGS_OTHER}"
                )
                if(SDL2_IMAGE_PC_LIBRARY_DIRS)
                    set_target_properties(SDL2_image_PKG PROPERTIES
                        INTERFACE_LINK_DIRECTORIES "${SDL2_IMAGE_PC_LIBRARY_DIRS}"
                    )
                endif()
                set(SDL_IMAGE_TARGET_NAME "SDL2_image_PKG")
                set(SDL_IMAGE_FOUND TRUE)
                message(STATUS "Found SDL2_image via pkg-config")
            endif()
        endif()
        
        # Method 3: Manual search
        if(NOT SDL_IMAGE_FOUND)
            find_library(SDL2_IMAGE_LIBRARY
                NAMES SDL2_image libSDL2_image
                PATHS /usr/lib /usr/local/lib /opt/lib /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
            )
            find_path(SDL2_IMAGE_INCLUDE_DIR SDL_image.h
                PATHS /usr/include/SDL2 /usr/local/include/SDL2 /opt/include/SDL2
            )
            
            if(SDL2_IMAGE_LIBRARY AND SDL2_IMAGE_INCLUDE_DIR)
                add_library(SDL2_image_MANUAL INTERFACE IMPORTED)
                set_target_properties(SDL2_image_MANUAL PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${SDL2_IMAGE_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_INCLUDE_DIR}"
                )
                set(SDL_IMAGE_TARGET_NAME "SDL2_image_MANUAL")
                set(SDL_IMAGE_FOUND TRUE)
                message(STATUS "Found SDL2_image manually: ${SDL2_IMAGE_LIBRARY}")
            endif()
        endif()
    else()
        # SDL3_image
        if(TUXARENA_PREFER_SYSTEM_LIBS)
            find_package(SDL3_image QUIET)
            if(SDL3_image_FOUND)
                set(SDL_IMAGE_FOUND TRUE)
                set(SDL_IMAGE_TARGET_NAME "SDL3_image::SDL3_image")
                message(STATUS "Found system SDL3_image")
            endif()
        endif()
    endif()
    
    if(NOT SDL_IMAGE_FOUND)
        message(FATAL_ERROR "SDL image library not found")
    endif()
    
    # Configure TTF with SDL2 priority
    message(STATUS "Configuring SDL TTF...")
    set(SDL_TTF_FOUND FALSE)
    set(SDL_TTF_TARGET_NAME "")
    
    if(NOT SDL_IS_SDL3)
        # SDL2_ttf methods
        message(STATUS "Looking for SDL2_ttf...")
        
        # Method 1: find_package
        find_package(SDL2_ttf QUIET)
        if(SDL2_ttf_FOUND)
            set(SDL_TTF_FOUND TRUE)
            if(TARGET SDL2_ttf::SDL2_ttf)
                set(SDL_TTF_TARGET_NAME "SDL2_ttf::SDL2_ttf")
            else()
                add_library(SDL2_ttf_LEGACY INTERFACE IMPORTED)
                set_target_properties(SDL2_ttf_LEGACY PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${SDL2_TTF_LIBRARIES}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_INCLUDE_DIRS}"
                )
                set(SDL_TTF_TARGET_NAME "SDL2_ttf_LEGACY")
            endif()
            message(STATUS "Found SDL2_ttf via find_package")
        endif()
        
        # Method 2: pkg-config
        if(NOT SDL_TTF_FOUND AND PkgConfig_FOUND)
            pkg_check_modules(SDL2_TTF_PC SDL2_ttf)
            if(SDL2_TTF_PC_FOUND)
                add_library(SDL2_ttf_PKG INTERFACE IMPORTED)
                set_target_properties(SDL2_ttf_PKG PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${SDL2_TTF_PC_LIBRARIES}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_PC_INCLUDE_DIRS}"
                    INTERFACE_COMPILE_OPTIONS "${SDL2_TTF_PC_CFLAGS_OTHER}"
                )
                if(SDL2_TTF_PC_LIBRARY_DIRS)
                    set_target_properties(SDL2_ttf_PKG PROPERTIES
                        INTERFACE_LINK_DIRECTORIES "${SDL2_TTF_PC_LIBRARY_DIRS}"
                    )
                endif()
                set(SDL_TTF_TARGET_NAME "SDL2_ttf_PKG")
                set(SDL_TTF_FOUND TRUE)
                message(STATUS "Found SDL2_ttf via pkg-config")
            endif()
        endif()
        
        # Method 3: Manual search
        if(NOT SDL_TTF_FOUND)
            find_library(SDL2_TTF_LIBRARY
                NAMES SDL2_ttf libSDL2_ttf
                PATHS /usr/lib /usr/local/lib /opt/lib /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
            )
            find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h
                PATHS /usr/include/SDL2 /usr/local/include/SDL2 /opt/include/SDL2
            )
            
            if(SDL2_TTF_LIBRARY AND SDL2_TTF_INCLUDE_DIR)
                add_library(SDL2_ttf_MANUAL INTERFACE IMPORTED)
                set_target_properties(SDL2_ttf_MANUAL PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${SDL2_TTF_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_INCLUDE_DIR}"
                )
                set(SDL_TTF_TARGET_NAME "SDL2_ttf_MANUAL")
                set(SDL_TTF_FOUND TRUE)
                message(STATUS "Found SDL2_ttf manually: ${SDL2_TTF_LIBRARY}")
            endif()
        endif()
    else()
        # SDL3_ttf
        if(TUXARENA_PREFER_SYSTEM_LIBS)
            find_package(SDL3_ttf QUIET)
            if(SDL3_ttf_FOUND)
                set(SDL_TTF_FOUND TRUE)
                set(SDL_TTF_TARGET_NAME "SDL3_ttf::SDL3_ttf")
                message(STATUS "Found system SDL3_ttf")
            endif()
        endif()
    endif()
    
    if(NOT SDL_TTF_FOUND)
        message(FATAL_ERROR "SDL TTF library not found")
    endif()
    
    # Export variables to parent scope
    set(SDL_IMAGE_TARGET_NAME ${SDL_IMAGE_TARGET_NAME} PARENT_SCOPE)
    set(SDL_TTF_TARGET_NAME ${SDL_TTF_TARGET_NAME} PARENT_SCOPE)
    set(SDL_IMAGE_FOUND ${SDL_IMAGE_FOUND} PARENT_SCOPE)
    set(SDL_TTF_FOUND ${SDL_TTF_FOUND} PARENT_SCOPE)
endfunction()

configure_sdl_extensions()

# ============================================================================
# TMXLITE CONFIGURATION
# ============================================================================

message(STATUS "Configuring tmxlite...")
set(TMXLITE_FOUND FALSE)
set(TMXLITE_TARGET_NAME "")

# Try system tmxlite first
if(TUXARENA_PREFER_SYSTEM_LIBS)
    find_package(tmxlite QUIET)
    if(tmxlite_FOUND)
        set(TMXLITE_FOUND TRUE)
        set(TMXLITE_TARGET_NAME "tmxlite::tmxlite")
        message(STATUS "Found system tmxlite")
    endif()
endif()

# Try bundled tmxlite
if(NOT TMXLITE_FOUND AND EXISTS "${THIRD_PARTY_DIR}/tmxlite")
    message(STATUS "Building bundled tmxlite...")
    add_subdirectory(${THIRD_PARTY_DIR}/tmxlite/tmxlite ${CMAKE_BINARY_DIR}/_tmxlite_build)
    
    set(TMXLITE_POSSIBLE_TARGETS tmxlite::tmxlite tmxlite tmxlite-static)
    foreach(target ${TMXLITE_POSSIBLE_TARGETS})
        if(TARGET ${target})
            set(TMXLITE_TARGET_NAME ${target})
            set(TMXLITE_FOUND TRUE)
            message(STATUS "Using bundled tmxlite target: ${target}")
            break()
        endif()
    endforeach()
endif()

if(NOT TMXLITE_FOUND)
    message(FATAL_ERROR "tmxlite library not found")
endif()

# ============================================================================
# IMGUI CONFIGURATION WITH PROPER SDL2/SDL3 BACKEND
# ============================================================================

set(IMGUI_DIR "${THIRD_PARTY_DIR}/imgui")

# Collect ImGui sources
file(GLOB IMGUI_CORE_SOURCES "${IMGUI_DIR}/*.cpp")
list(REMOVE_ITEM IMGUI_CORE_SOURCES "${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp")
if(NOT IMGUI_CORE_SOURCES)
    message(FATAL_ERROR "No ImGui core sources found in ${IMGUI_DIR}")
endif()

# Backend sources
set(IMGUI_BACKEND_SOURCES)

# SDL backend - choose based on SDL version
if(SDL_IS_SDL3)
    set(SDL_BACKENDS imgui_impl_sdl3.cpp)
else()
    set(SDL_BACKENDS imgui_impl_sdl2.cpp imgui_impl_sdl.cpp)
endif()

foreach(backend ${SDL_BACKENDS})
    if(EXISTS "${IMGUI_DIR}/backends/${backend}")
        list(APPEND IMGUI_BACKEND_SOURCES "${IMGUI_DIR}/backends/${backend}")
        message(STATUS "Using ImGui SDL backend: ${backend}")
        break()
    endif()
endforeach()

if(NOT IMGUI_BACKEND_SOURCES)
    message(FATAL_ERROR "No suitable ImGui SDL backend found")
endif()

# OpenGL backend with proper loader support
if(OpenGL_FOUND)
    set(OPENGL_BACKENDS imgui_impl_opengl3.cpp imgui_impl_opengl2.cpp)
    foreach(backend ${OPENGL_BACKENDS})
        if(EXISTS "${IMGUI_DIR}/backends/${backend}")
            list(APPEND IMGUI_BACKEND_SOURCES "${IMGUI_DIR}/backends/${backend}")
            message(STATUS "Using ImGui OpenGL backend: ${backend}")
            break()
        endif()
    endforeach()
endif()

# Create ImGui library
add_library(imgui STATIC ${IMGUI_CORE_SOURCES} ${IMGUI_BACKEND_SOURCES})

target_include_directories(imgui PUBLIC 
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

# Configure ImGui with proper OpenGL loader
if(OPENGL_LOADER_FOUND)
    if(OPENGL_LOADER_TYPE STREQUAL "GLAD")
        target_compile_definitions(imgui PUBLIC
            IMGUI_IMPL_OPENGL_LOADER_GLAD=1
            IMGUI_IMPL_OPENGL_LOADER_GLEW=0
        )
        if(OPENGL_LOADER_INCLUDE)
            target_include_directories(imgui PUBLIC ${OPENGL_LOADER_INCLUDE})
        endif()
        message(STATUS "ImGui configured with GLAD loader")
    elseif(OPENGL_LOADER_TYPE STREQUAL "GLEW")
        target_compile_definitions(imgui PUBLIC
            IMGUI_IMPL_OPENGL_LOADER_GLAD=0
            IMGUI_IMPL_OPENGL_LOADER_GLEW=1
        )
        if(OPENGL_LOADER_INCLUDE)
            target_include_directories(imgui PUBLIC ${OPENGL_LOADER_INCLUDE})
        endif()
        message(STATUS "ImGui configured with GLEW loader")
    endif()
else()
    # Fallback to custom loader
    target_compile_definitions(imgui PUBLIC
        IMGUI_IMPL_OPENGL_LOADER_GLAD=0
        IMGUI_IMPL_OPENGL_LOADER_GLEW=0
        IMGUI_IMPL_OPENGL_LOADER_CUSTOM=1
    )
    message(STATUS "ImGui configured with custom OpenGL loader")
endif()

# Additional ImGui config
target_compile_definitions(imgui PUBLIC
    IMGUI_USER_CONFIG="tuxarena_imgui_config.h"
)

# Link ImGui dependencies
target_link_libraries(imgui PUBLIC ${SDL_TARGET_NAME})
if(OpenGL_FOUND)
    target_link_libraries(imgui PUBLIC OpenGL::GL)
endif()

# Link OpenGL loader if available
if(OPENGL_LOADER_FOUND AND OPENGL_LOADER_LIBRARY)
    if(TARGET ${OPENGL_LOADER_LIBRARY})
        target_link_libraries(imgui PUBLIC ${OPENGL_LOADER_LIBRARY})
    else()
        target_link_libraries(imgui PUBLIC ${OPENGL_LOADER_LIBRARY})
    endif()
endif()

# Create ImGui config header
set(IMGUI_CONFIG_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/tuxarena_imgui_config.h")
if(NOT EXISTS "${IMGUI_CONFIG_HEADER}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
    file(WRITE "${IMGUI_CONFIG_HEADER}" 
"#pragma once

#define IMGUI_ENABLE_DOCKING
#define IMGUI_ENABLE_VIEWPORTS
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#define TUXARENA_IMGUI_CONFIG_VERSION 1
")
    message(STATUS "Created ImGui configuration header")
endif()

# ============================================================================
# PROJECT SOURCE COLLECTION
# ============================================================================

file(GLOB_RECURSE TUXARENA_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UI.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/WeaponManager.cpp"
)

if(NOT TUXARENA_SOURCES)
    message(FATAL_ERROR "No source files found in src/ directory")
endif()

list(LENGTH TUXARENA_SOURCES SOURCE_COUNT)
message(STATUS "Found ${SOURCE_COUNT} source files")

# ============================================================================
# MAIN EXECUTABLE TARGET
# ============================================================================

add_executable(TuxArena ${TUXARENA_SOURCES} src/main.cpp)

set_target_properties(TuxArena PROPERTIES
    OUTPUT_NAME "tuxarena"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Create output directory
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ============================================================================
# COMPILER FLAGS
# ============================================================================

target_compile_options(TuxArena PRIVATE
    -Wall -Wextra -Wpedantic
    "$<$<CONFIG:Debug>:-g3;-O0;-DDEBUG>"
    "$<$<CONFIG:Release>:-O3;-DNDEBUG>"
)

# Suppress specific warnings for third-party code
if(COMPILER_IS_GCC)
    target_compile_options(TuxArena PRIVATE
        -Wno-restrict  # Suppress false positives in tmxlite
    )
endif()

# ============================================================================
# INCLUDE DIRECTORIES AND LINKING
# ============================================================================

target_include_directories(TuxArena PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${THIRD_PARTY_DIR}/imgui/backends"
)



# Add third-party includes if using bundled libraries
if(EXISTS "${THIRD_PARTY_DIR}/tmxlite/tmxlite/include")
    target_include_directories(TuxArena PRIVATE "${THIRD_PARTY_DIR}/tmxlite/tmxlite/include")
endif()

# Core libraries
target_link_libraries(TuxArena PRIVATE ${SDL2_LIBRARIES} ${SDL2_NET_LIBRARIES} SDL2_image SDL2_ttf tmxlite imgui -lstdc++fs)

# Networking
if(TUXARENA_ENABLE_NETWORKING AND SDL_NET_FOUND)
    target_link_libraries(TuxArena PRIVATE ${SDL_NET_TARGET_NAME})
endif()

# Graphics
if(OpenGL_FOUND)
    target_link_libraries(TuxArena PRIVATE OpenGL::GL)
    if(TARGET OpenGL::GLU)
        target_link_libraries(TuxArena PRIVATE OpenGL::GLU)
    endif()
endif()

# OpenGL loader
if(OPENGL_LOADER_FOUND AND OPENGL_LOADER_LIBRARY)
    if(TARGET ${OPENGL_LOADER_LIBRARY})
        target_link_libraries(TuxArena PRIVATE ${OPENGL_LOADER_LIBRARY})
    else()
        target_link_libraries(TuxArena PRIVATE ${OPENGL_LOADER_LIBRARY})
    endif()
endif()

# Platform libraries
if(LINUX)
    target_link_libraries(TuxArena PRIVATE dl m rt)
    
    if(X11_FOUND)
        target_link_libraries(TuxArena PRIVATE ${X11_LIBRARIES})
    endif()
    
    if(WAYLAND_FOUND)
        target_link_libraries(TuxArena PRIVATE ${WAYLAND_LIBRARIES})
    endif()
endif()

# ============================================================================
# FEATURE DEFINITIONS
# ============================================================================

target_compile_definitions(TuxArena PRIVATE
    TUXARENA_VERSION_STRING="${PROJECT_VERSION}"
)

if(LINUX)
    target_compile_definitions(TuxArena PRIVATE TUXARENA_PLATFORM_LINUX=1)
endif()

if(SDL_IS_SDL3)
    target_compile_definitions(TuxArena PRIVATE 
        TUXARENA_SDL3=1
        TUXARENA_SDL_VERSION_MAJOR=3
    )
else()
    target_compile_definitions(TuxArena PRIVATE 
        TUXARENA_SDL2=1  
        TUXARENA_SDL_VERSION_MAJOR=2
    )
endif()

if(TUXARENA_ENABLE_NETWORKING)
    target_compile_definitions(TuxArena PRIVATE TUXARENA_NETWORKING=1)
endif()

if(OPENGL_LOADER_FOUND)
    target_compile_definitions(TuxArena PRIVATE TUXARENA_OPENGL_LOADER_${OPENGL_LOADER_TYPE}=1)
endif()

# ============================================================================
# INSTALLATION RULES
# ============================================================================

install(TARGETS TuxArena
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# Install data files if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
        DESTINATION share/tuxarena
        COMPONENT Runtime
    )
endif()

# ============================================================================
# STATUS SUMMARY
# ============================================================================

message(STATUS "")
message(STATUS "==================== TuxArena Build Summary ====================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Graphics:")
message(STATUS "  OpenGL: ${OpenGL_FOUND}")
if(OPENGL_LOADER_FOUND)
    message(STATUS "  OpenGL Loader: ${OPENGL_LOADER_TYPE}")
else()
    message(STATUS "  OpenGL Loader: None (may cause issues)")
endif()
message(STATUS "")
message(STATUS "Libraries:")
message(STATUS "  SDL${SDL_VERSION_MAJOR}: ${SDL_TARGET_NAME}")
if(TUXARENA_ENABLE_NETWORKING AND SDL_NET_FOUND)
    message(STATUS "  SDL${SDL_VERSION_MAJOR}_net: ${SDL_NET_TARGET_NAME}")
endif()
message(STATUS "  SDL${SDL_VERSION_MAJOR}_image: ${SDL_IMAGE_TARGET_NAME}")
message(STATUS "  SDL${SDL_VERSION_MAJOR}_ttf: ${SDL_TTF_TARGET_NAME}")
message(STATUS "  tmxlite: ${TMXLITE_TARGET_NAME}")
message(STATUS "  ImGui: Static library")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  Networking: ${TUXARENA_ENABLE_NETWORKING}")
message(STATUS "  Audio: ${TUXARENA_ENABLE_AUDIO}")
message(STATUS "")
message(STATUS "Source files: ${SOURCE_COUNT}")
message(STATUS "==============================================================")
message(STATUS "")

# ============================================================================
# HELPFUL BUILD COMMANDS
# ============================================================================

message(STATUS "Helpful commands:")
message(STATUS "  Force SDL2 only: cmake -DTUXARENA_SKIP_BUNDLED_SDL=ON ..")
message(STATUS "  Use system libs:  cmake -DTUXARENA_PREFER_SYSTEM_LIBS=ON ..")
message(STATUS "  Debug build:      cmake -DCMAKE_BUILD_TYPE=Debug ..")