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

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/core)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# 第三方库
# TODO: 添加QuickJS, Skia, SDL3, Yoga
# add_subdirectory(third_party)

# 核心库
# TODO: 添加核心模块
# add_subdirectory(core)

# 语言绑定
# TODO: 添加语言绑定
# if(LIGHTUI_BUILD_PYTHON_BINDING)
#     add_subdirectory(bindings/python)
# endif()

# 示例
# TODO: 添加示例
# if(LIGHTUI_BUILD_EXAMPLES)
#     add_subdirectory(examples)
# endif()

# 测试
# TODO: 添加测试
# if(LIGHTUI_BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()

# 安装
# TODO: 添加安装目标
# install(TARGETS lightui
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     RUNTIME DESTINATION bin
# )
# install(DIRECTORY core/ DESTINATION include/lightui
#     FILES_MATCHING PATTERN "*.h"
# )

