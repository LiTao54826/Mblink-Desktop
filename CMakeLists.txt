# LightUI CMake配置文件
#
# 功能：
# - 配置项目基本信息
# - 管理第三方依赖
# - 构建核心库
# - 构建语言绑定
# - 构建示例和测试
#
# TODO:
# - [ ] 配置第三方库（QuickJS, Skia, SDL3, Yoga）
# - [ ] 添加核心模块构建
# - [ ] 添加Python绑定构建
# - [ ] 添加测试构建
# - [ ] 添加安装目标

cmake_minimum_required(VERSION 3.15)
project(LightUI VERSION 0.1.0 LANGUAGES CXX C)

# C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建选项
option(LIGHTUI_BUILD_TESTS "Build tests" ON)
option(LIGHTUI_BUILD_EXAMPLES "Build examples" ON)
option(LIGHTUI_BUILD_PYTHON_BINDING "Build Python binding" ON)
option(LIGHTUI_BUILD_RUST_BINDING "Build Rust binding" OFF)
option(LIGHTUI_BUILD_GO_BINDING "Build Go binding" OFF)
option(LIGHTUI_USE_SKIA "Use Skia for rendering (requires Skia)" OFF)

# 编译器警告（已禁用以减少输出）
if(MSVC)
    add_compile_options(/W0 /utf-8)
else()
    add_compile_options(-w)  # -w 禁用所有警告
endif()

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/core)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# 第三方库
add_subdirectory(third_party)

# 核心库
add_subdirectory(core)

# 语言绑定
if(LIGHTUI_BUILD_PYTHON_BINDING)
    # Python绑定通过setup.py构建，这里不需要CMake
    message(STATUS "Python binding will be built via setup.py")
endif()

# 示例
if(LIGHTUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 测试
if(LIGHTUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装
install(TARGETS lightui
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY core/ DESTINATION include/lightui
    FILES_MATCHING PATTERN "*.h"
)

# 打印配置信息
message(STATUS "========================================")
message(STATUS "  LightUI Configuration")
message(STATUS "========================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${LIGHTUI_BUILD_TESTS}")
message(STATUS "  Build examples: ${LIGHTUI_BUILD_EXAMPLES}")
message(STATUS "  Use Skia: ${LIGHTUI_USE_SKIA}")
message(STATUS "========================================")

