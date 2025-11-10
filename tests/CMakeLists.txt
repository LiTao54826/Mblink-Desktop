# LightUI Tests CMake Configuration

cmake_minimum_required(VERSION 3.15)

# Google Test is already fetched by the root CMakeLists.txt
# We just need to add test executables here

enable_testing()

message(STATUS "Tests directory configured")

# Helper function to add Skia test
function(add_skia_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/third_party/skia
    )

    target_link_libraries(${TEST_NAME} PRIVATE
        lightui_render
    )

    if(TARGET skia)
        target_link_libraries(${TEST_NAME} PRIVATE skia)

        # Skia 预编译库使用静态 Release 运行时 (MT)
        # 即使在 Debug 模式下，我们也需要匹配这个设置
        if(MSVC)
            # 使用静态 Release 运行时以匹配 Skia
            target_compile_options(${TEST_NAME} PRIVATE
                $<$<CONFIG:Debug>:/MT>
                $<$<CONFIG:Release>:/MT>
            )
            # 禁用迭代器调试以匹配 Skia
            target_compile_definitions(${TEST_NAME} PRIVATE
                $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
            )
        endif()
    endif()

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

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

# Render Engine Test
add_executable(test_render_engine test_render_engine.cpp)

target_include_directories(test_render_engine PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/skia
)

target_link_libraries(test_render_engine PRIVATE
    lightui_render
)

# Link Skia if available
if(TARGET skia)
    target_link_libraries(test_render_engine PRIVATE skia)
endif()

if(WIN32 AND MINGW)
    target_link_options(test_render_engine PRIVATE -static-libgcc -static-libstdc++)
endif()

add_test(NAME RenderEngineTest COMMAND test_render_engine)

# CSS Rendering Test
add_skia_test(test_css_rendering test_css_rendering.cpp)

# Advanced CSS Rendering Test
add_skia_test(test_advanced_css test_advanced_css.cpp)

# Render Tree Test
add_skia_test(test_render_tree test_render_tree.cpp)
target_link_libraries(test_render_tree PRIVATE lightui_dom)

# Optimization Test
add_skia_test(test_optimization test_optimization.cpp)

# Advanced Optimization Test
add_skia_test(test_advanced_optimization test_advanced_optimization.cpp)
target_link_libraries(test_advanced_optimization PRIVATE lightui_dom)
