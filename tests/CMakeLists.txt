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

# DOM Node Test
add_executable(test_dom_node test_dom_node.cpp)

target_include_directories(test_dom_node PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_dom_node PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)

# Force use of MSVCRT instead of UCRT on Windows
if(WIN32 AND MINGW)
    target_link_options(test_dom_node PRIVATE -static-libgcc -static-libstdc++)
endif()

add_test(NAME DOMNodeTest COMMAND test_dom_node)

# DOM Event Test
add_executable(test_dom_event test_dom_event.cpp)

target_include_directories(test_dom_event PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_dom_event PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)

if(WIN32 AND MINGW)
    target_link_options(test_dom_event PRIVATE -static-libgcc -static-libstdc++)
endif()

add_test(NAME DOMEventTest COMMAND test_dom_event)

# DOM Document Test
add_executable(test_dom_document test_dom_document.cpp)

target_include_directories(test_dom_document PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_dom_document PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)

if(WIN32 AND MINGW)
    target_link_options(test_dom_document PRIVATE -static-libgcc -static-libstdc++)
endif()

add_test(NAME DOMDocumentTest COMMAND test_dom_document)

# DOM Query Test
add_executable(test_dom_query test_dom_query.cpp)

target_include_directories(test_dom_query PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_dom_query PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)

if(WIN32 AND MINGW)
    target_link_options(test_dom_query PRIVATE -static-libgcc -static-libstdc++)
endif()

add_test(NAME DOMQueryTest COMMAND test_dom_query)

# DOM Integration Test
add_executable(test_dom_integration test_dom_integration.cpp)

target_include_directories(test_dom_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_dom_integration PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)

if(WIN32 AND MINGW)
    target_link_options(test_dom_integration PRIVATE -static-libgcc -static-libstdc++)
endif()

add_test(NAME DOMIntegrationTest COMMAND test_dom_integration)

# DOM Bindings Integration Test
add_executable(test_dom_bindings_integration test_dom_bindings_integration.cpp)

target_include_directories(test_dom_bindings_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/quickjs
)

target_link_libraries(test_dom_bindings_integration PRIVATE
    lightui_dom
    lightui_quickjs
    GTest::gtest
    GTest::gtest_main
)

if(WIN32 AND MINGW)
    target_link_options(test_dom_bindings_integration PRIVATE -static-libgcc -static-libstdc++)
endif()

add_test(NAME DOMBindingsIntegrationTest COMMAND test_dom_bindings_integration)

# DOM 性能基准测试
add_executable(benchmark_dom benchmark_dom.cpp)
target_link_libraries(benchmark_dom PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)
# 注意：基准测试不添加到 CTest，因为它们不是功能测试

