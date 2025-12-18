# LightUI 测试套件 CMake 配置
#
# 测试分类：
# - unit/       单元测试（核心功能）
# - render/     渲染测试
# - integration/ 集成测试
# - performance/ 性能测试
# - property/   属性测试（Property-Based Testing）

cmake_minimum_required(VERSION 3.15)

include(FetchContent)

# 查找 Google Test
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # 如果没有找到，使用 FetchContent 下载
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # 对于 Windows: 防止覆盖父项目的编译器/链接器设置
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# RapidCheck for Property-Based Testing (optional)
# If RapidCheck is not available, property tests will use manual iteration
option(LIGHTUI_USE_RAPIDCHECK "Use RapidCheck for property-based testing" OFF)
if(LIGHTUI_USE_RAPIDCHECK)
    FetchContent_Declare(
        rapidcheck
        GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
        GIT_TAG master
    )
    set(RC_ENABLE_GTEST ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(rapidcheck)
endif()

# 启用测试
enable_testing()

# 测试公共头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/core)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# 添加测试辅助库
add_library(test_utils STATIC
    test_utils/test_helpers.cpp
    test_utils/mock_objects.cpp
)
target_link_libraries(test_utils PUBLIC lightui GTest::gtest)

# 单元测试
add_subdirectory(unit)

# 渲染测试
add_subdirectory(render)

# 集成测试
add_subdirectory(integration)

# 性能测试（可选）
option(LIGHTUI_BUILD_PERF_TESTS "Build performance tests" OFF)
if(LIGHTUI_BUILD_PERF_TESTS)
    add_subdirectory(performance)
endif()

# 属性测试（Property-Based Testing）
add_subdirectory(property)

# 添加运行所有测试的目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS lightui_unit_tests lightui_render_tests lightui_integration_tests lightui_property_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
