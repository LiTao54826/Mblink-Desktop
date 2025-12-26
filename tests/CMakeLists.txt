# LightUI Test Suite CMake Configuration
#
# Test Categories:
# - unit/       Unit tests (core functionality)
# - render/     Render tests
# - integration/ Integration tests
# - performance/ Performance tests
# - property/   Property-Based Testing

cmake_minimum_required(VERSION 3.15)

include(FetchContent)

# Configure test output directory
set(TEST_RESULTS_DIR "${CMAKE_BINARY_DIR}/test_results")
file(MAKE_DIRECTORY ${TEST_RESULTS_DIR})

# Find Google Test
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # If not found, use FetchContent to download
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: prevent overriding parent project's compiler/linker settings
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

# Enable testing
enable_testing()

# Test common include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/core)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Add test utility library
add_library(test_utils STATIC
    test_utils/test_helpers.cpp
    test_utils/mock_objects.cpp
)
target_link_libraries(test_utils PUBLIC lightui GTest::gtest)

# Unit tests
add_subdirectory(unit)

# Render tests
add_subdirectory(render)

# Integration tests
add_subdirectory(integration)

# Performance tests (optional)
option(LIGHTUI_BUILD_PERF_TESTS "Build performance tests" OFF)
if(LIGHTUI_BUILD_PERF_TESTS)
    add_subdirectory(performance)
endif()

# Property-Based Testing
add_subdirectory(property)

# Add target to run all tests with results output to test_results directory
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --output-junit ${TEST_RESULTS_DIR}/all_tests.xml
    DEPENDS lightui_unit_tests lightui_render_tests lightui_integration_tests lightui_property_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
