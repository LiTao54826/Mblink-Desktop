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

# Default to Release build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "Setting build type to 'Release' as none was specified.")
endif()

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
option(LIGHTUI_ENABLE_LTO "Enable Link Time Optimization (slower build, smaller binary)" OFF)

# Link Time Optimization (LTO)
# LTO allows the compiler to optimize across translation units, removing unused code
if(LIGHTUI_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_ERROR)
    if(LTO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "LTO enabled - this may reduce binary size")
    else()
        message(WARNING "LTO not supported: ${LTO_ERROR}")
    endif()
endif()

# Compiler Settings
# Disable warnings and configure runtime library to match Skia
if(MSVC)
    add_compile_options(/W0 /utf-8 /EHsc)
    # Use static runtime /MT to match prebuilt Skia library
    add_compile_options(/MT)
    add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "MSVC runtime library" FORCE)
    
    # Release 优化：移除调试符号，启用最大优化
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES)
        add_compile_options(
            $<$<CONFIG:Release>:/O2>      # 最大速度优化
            $<$<CONFIG:Release>:/Ob2>     # 内联扩展
            $<$<CONFIG:Release>:/Oi>      # 启用内部函数
            $<$<CONFIG:Release>:/Gy>      # 函数级链接
            $<$<CONFIG:Release>:/GS->     # 禁用安全检查
        )
        add_link_options(
            $<$<CONFIG:Release>:/OPT:REF>   # 移除未引用函数
            $<$<CONFIG:Release>:/OPT:ICF>   # 相同 COMDAT 折叠
            $<$<CONFIG:Release>:/DEBUG:NONE> # 不生成调试信息
            $<$<CONFIG:Release>:/INCREMENTAL:NO>  # 禁用增量链接（与 /OPT:ICF 兼容）
        )
    endif()
    # Debug 配置保留调试符号
    add_compile_options($<$<CONFIG:Debug>:/Zi>)
    add_link_options($<$<CONFIG:Debug>:/DEBUG>)
else()
    add_compile_options(-w)  # Disable all warnings
    # Release 优化
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -fvisibility=hidden -ffunction-sections -fdata-sections)
        add_link_options(-Wl,--gc-sections -s)
    endif()
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
    add_subdirectory(bindings/python)
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
