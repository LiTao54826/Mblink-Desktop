# LightUI CMake Configuration
#
# A lightweight, high-performance UI rendering engine
# Features:
# - Cross-platform window management
# - Hardware-accelerated rendering with Skia
# - DOM-based component model
# - JavaScript integration via QuickJS
# - CSS styling support

cmake_minimum_required(VERSION 3.15)
project(LightUI VERSION 0.1.0 LANGUAGES CXX C)

# C++ Standard (quickjs-ng requires C++20 for certain features)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Options
option(LIGHTUI_BUILD_PYTHON_BINDING "Build Python binding" ON)
option(LIGHTUI_BUILD_RUST_BINDING "Build Rust binding" OFF)
option(LIGHTUI_BUILD_GO_BINDING "Build Go binding" OFF)
option(LIGHTUI_USE_SKIA "Use Skia for rendering (requires Skia)" ON)
option(LIGHTUI_BUILD_TESTS "Build tests" OFF)

# Compiler Settings
# Disable warnings and configure runtime library to match Skia
if(MSVC)
    add_compile_options(/W0 /utf-8)
    # Use static runtime /MT to match prebuilt Skia library
    # Skia uses /MT for both Debug and Release configurations
    add_compile_options(/MT)
    add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "MSVC runtime library" FORCE)
else()
    add_compile_options(-w)  # Disable all warnings
endif()

# Output Directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include Directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/core)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Third-party Dependencies
add_subdirectory(third_party)

# Core Library
add_subdirectory(core)

# Language Bindings
if(LIGHTUI_BUILD_PYTHON_BINDING)
    # Python binding is built via setup.py, not CMake
    message(STATUS "Python binding will be built via setup.py")
endif()

# Tools
add_subdirectory(tools/app_loader)
add_subdirectory(tools/app_bundler)
add_subdirectory(tools/html_loader)
add_subdirectory(tools/esm_loader)

# Installation
install(TARGETS lightui
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY core/ DESTINATION include/lightui
    FILES_MATCHING PATTERN "*.h"
)

# Tests
if(LIGHTUI_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Configuration Summary
message(STATUS "========================================")
message(STATUS "  LightUI Configuration")
message(STATUS "========================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Use Skia: ${LIGHTUI_USE_SKIA}")
message(STATUS "  Build tests: ${LIGHTUI_BUILD_TESTS}")
message(STATUS "========================================")
