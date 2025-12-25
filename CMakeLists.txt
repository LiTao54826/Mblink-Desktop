# LightUI CMake??????
#
# ?????# - ????????????
# - ???????????# - ????????# - ?????????
# - ??????

cmake_minimum_required(VERSION 3.15)
project(LightUI VERSION 0.1.0 LANGUAGES CXX C)

# C++??? (quickjs-ng ????C++20 ????????????)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ??????
option(LIGHTUI_BUILD_PYTHON_BINDING "Build Python binding" ON)
option(LIGHTUI_BUILD_RUST_BINDING "Build Rust binding" OFF)
option(LIGHTUI_BUILD_GO_BINDING "Build Go binding" OFF)
option(LIGHTUI_USE_SKIA "Use Skia for rendering (requires Skia)" ON)
option(LIGHTUI_BUILD_TESTS "Build tests" OFF)

# ???????????????????????
if(MSVC)
    add_compile_options(/W0 /utf-8)
    # ????????? /MT ?????????????? Skia
    # ?????kia ??Debug ??Release ????????/MT (??? /MTd)
    add_compile_options(/MT)
    add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "MSVC runtime library" FORCE)
else()
    add_compile_options(-w)  # -w ??????????
endif()

# ??????
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ??????
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/core)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# ??????
add_subdirectory(third_party)

# ?????
add_subdirectory(core)

# ??????
if(LIGHTUI_BUILD_PYTHON_BINDING)
    # Python??????setup.py?????????????Make
    message(STATUS "Python binding will be built via setup.py")
endif()

# ???
add_subdirectory(tools/app_loader)
add_subdirectory(tools/app_bundler)
add_subdirectory(tools/html_loader)
add_subdirectory(tools/esm_loader)

# ???
install(TARGETS lightui
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY core/ DESTINATION include/lightui
    FILES_MATCHING PATTERN "*.h"
)

# ???
if(LIGHTUI_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ?????????
message(STATUS "========================================")
message(STATUS "  LightUI Configuration")
message(STATUS "========================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Use Skia: ${LIGHTUI_USE_SKIA}")
message(STATUS "  Build tests: ${LIGHTUI_BUILD_TESTS}")
message(STATUS "========================================")

