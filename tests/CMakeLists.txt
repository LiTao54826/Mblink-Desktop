# LightUI Tests CMake Configuration

cmake_minimum_required(VERSION 3.15)

# Google Test is already fetched by the root CMakeLists.txt
# We just need to add test executables here

enable_testing()

message(STATUS "Tests directory configured")

# Hello Test
add_executable(test_hello test_hello.cpp)
# Force use of MSVCRT instead of UCRT on Windows
if(WIN32 AND MINGW)
    target_link_options(test_hello PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME HelloTest COMMAND test_hello)

# QuickJS C Test
add_executable(test_quickjs_c test_quickjs_c.c)
target_include_directories(test_quickjs_c PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/quickjs
)
target_link_libraries(test_quickjs_c PRIVATE quickjs)
if(WIN32)
    target_link_libraries(test_quickjs_c PRIVATE ws2_32)
endif()
add_test(NAME QuickJSCTest COMMAND test_quickjs_c)

# Simple QuickJS Test
add_executable(test_simple test_simple.cpp)

target_include_directories(test_simple PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_simple PRIVATE
    lightui_quickjs
)

# Force use of MSVCRT instead of UCRT on Windows
if(WIN32 AND MINGW)
    target_link_options(test_simple PRIVATE -static-libgcc -static-libstdc++)
endif()

if(WIN32)
    target_link_libraries(test_simple PRIVATE ws2_32)
endif()

add_test(NAME SimpleTest COMMAND test_simple)

# QuickJS Runtime Test
add_executable(test_quickjs_runtime test_quickjs_runtime.cpp)

target_include_directories(test_quickjs_runtime PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_quickjs_runtime PRIVATE
    lightui_quickjs
    nlohmann_json
)

# Force use of MSVCRT instead of UCRT on Windows
if(WIN32 AND MINGW)
    target_link_options(test_quickjs_runtime PRIVATE -static-libgcc -static-libstdc++)
endif()

# Link Windows socket library if on Windows
if(WIN32)
    target_link_libraries(test_quickjs_runtime PRIVATE ws2_32)
endif()

add_test(NAME QuickJSRuntime COMMAND test_quickjs_runtime)

