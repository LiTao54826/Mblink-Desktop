# LightUI Tests CMake Configuration

cmake_minimum_required(VERSION 3.15)

# Google Test is already fetched by the root CMakeLists.txt
# We just need to add test executables here

enable_testing()

message(STATUS "Tests directory configured")

# TODO: Add test executables as modules are implemented
# Example structure:
# add_executable(window_test window/window_test.cpp)
# target_link_libraries(window_test PRIVATE lightui_core gtest gtest_main)
# add_test(NAME WindowTest COMMAND window_test)

