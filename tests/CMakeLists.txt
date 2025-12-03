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
        lightui_lexbor
        GTest::gtest
        GTest::gtest_main
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

# ============================================================================
# Benchmark Tests
# ============================================================================

# CSS Animation Benchmark
add_executable(benchmark_css_animations
    benchmark/benchmark_css_animations.cpp
)

target_include_directories(benchmark_css_animations PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/skia
)

target_link_libraries(benchmark_css_animations PRIVATE
    lightui_render
    lightui_dom
    lightui_lexbor
)

if(TARGET skia)
    target_link_libraries(benchmark_css_animations PRIVATE skia)

    if(MSVC)
        target_compile_options(benchmark_css_animations PRIVATE
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(benchmark_css_animations PRIVATE
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )
    endif()
endif()

# ============================================================================
# Unit Tests
# ============================================================================

# Hello Test
add_executable(test_hello unit/test_hello.cpp)
if(WIN32 AND MINGW)
    target_link_options(test_hello PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME HelloTest COMMAND test_hello)

# QuickJS C Test
add_executable(test_quickjs_c unit/test_quickjs_c.c)
target_include_directories(test_quickjs_c PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/quickjs
)
target_link_libraries(test_quickjs_c PRIVATE quickjs)
if(WIN32)
    target_link_libraries(test_quickjs_c PRIVATE ws2_32)
endif()
add_test(NAME QuickJSCTest COMMAND test_quickjs_c)

# Simple QuickJS Test
add_executable(test_simple unit/test_simple.cpp)
target_include_directories(test_simple PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)
target_link_libraries(test_simple PRIVATE
    lightui_quickjs
)
if(WIN32 AND MINGW)
    target_link_options(test_simple PRIVATE -static-libgcc -static-libstdc++)
endif()
if(WIN32)
    target_link_libraries(test_simple PRIVATE ws2_32)
endif()
add_test(NAME SimpleTest COMMAND test_simple)

# QuickJS Runtime Test
add_executable(test_quickjs_runtime unit/test_quickjs_runtime.cpp)
target_include_directories(test_quickjs_runtime PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)
target_link_libraries(test_quickjs_runtime PRIVATE
    lightui_quickjs
    nlohmann_json
)
if(WIN32 AND MINGW)
    target_link_options(test_quickjs_runtime PRIVATE -static-libgcc -static-libstdc++)
endif()
if(WIN32)
    target_link_libraries(test_quickjs_runtime PRIVATE ws2_32)
endif()
add_test(NAME QuickJSRuntime COMMAND test_quickjs_runtime)

# DOM Node Test
add_executable(test_dom_node unit/test_dom_node.cpp)
target_include_directories(test_dom_node PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)
target_link_libraries(test_dom_node PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)
if(WIN32 AND MINGW)
    target_link_options(test_dom_node PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME DOMNodeTest COMMAND test_dom_node)

# Dirty Marking Test (Week 1 - Partial Rendering)
add_executable(test_dirty_marking unit/test_dirty_marking.cpp)
target_include_directories(test_dirty_marking PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_dirty_marking PRIVATE
    lightui_dom
    lightui_lexbor
    lightui_render
    GTest::gtest
    GTest::gtest_main
)
# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_dirty_marking PRIVATE lexbor_static)
endif()
# Link Skia for SkRect
if(TARGET skia)
    target_link_libraries(test_dirty_marking PRIVATE skia)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_dirty_marking PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME DirtyMarkingTest COMMAND test_dirty_marking)

# Incremental Rendering Test (Week 2 - Partial Rendering)
add_executable(test_incremental_rendering unit/test_incremental_rendering.cpp)
target_include_directories(test_incremental_rendering PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_incremental_rendering PRIVATE
    lightui_dom
    lightui_lexbor
    lightui_render
    lightui_window
    GTest::gtest
    GTest::gtest_main
)
# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_incremental_rendering PRIVATE lexbor_static)
endif()
# Link Skia
if(TARGET skia)
    target_link_libraries(test_incremental_rendering PRIVATE skia)
endif()
# Link SDL3
if(TARGET SDL3-static)
    target_link_libraries(test_incremental_rendering PRIVATE SDL3-static)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_incremental_rendering PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME IncrementalRenderingTest COMMAND test_incremental_rendering)

# Real World Application Test (Week 3 - Practical Testing)
add_executable(test_real_world_app integration/test_real_world_app.cpp)
target_include_directories(test_real_world_app PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_real_world_app PRIVATE
    lightui_dom
    lightui_lexbor
    lightui_render
    lightui_window
    GTest::gtest
    GTest::gtest_main
)
# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_real_world_app PRIVATE lexbor_static)
endif()
# Link Skia
if(TARGET skia)
    target_link_libraries(test_real_world_app PRIVATE skia)
endif()
# Link SDL3
if(TARGET SDL3-static)
    target_link_libraries(test_real_world_app PRIVATE SDL3-static)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_real_world_app PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME RealWorldAppTest COMMAND test_real_world_app)

# Rendering Benchmark (Performance Testing)
add_executable(benchmark_rendering benchmarks/benchmark_rendering.cpp)
target_include_directories(benchmark_rendering PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(benchmark_rendering PRIVATE
    lightui_dom
    lightui_lexbor
    lightui_render
    lightui_window
    GTest::gtest
    GTest::gtest_main
)
if(TARGET lexbor_static)
    target_link_libraries(benchmark_rendering PRIVATE lexbor_static)
endif()
if(TARGET skia)
    target_link_libraries(benchmark_rendering PRIVATE skia)
endif()
if(TARGET SDL3-static)
    target_link_libraries(benchmark_rendering PRIVATE SDL3-static)
endif()
if(WIN32 AND MINGW)
    target_link_options(benchmark_rendering PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME RenderingBenchmark COMMAND benchmark_rendering)

# DOM Event Test
add_executable(test_dom_event unit/test_dom_event.cpp)
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
add_executable(test_dom_document unit/test_dom_document.cpp)
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
add_executable(test_dom_query unit/test_dom_query.cpp)
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

# DOM HTML Content Test (innerHTML/outerHTML)
add_executable(test_dom_html_content unit/test_dom_html_content.cpp)
target_include_directories(test_dom_html_content PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)
target_link_libraries(test_dom_html_content PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)
if(WIN32 AND MINGW)
    target_link_options(test_dom_html_content PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME DOMHTMLContentTest COMMAND test_dom_html_content)

# Render Engine Test
add_executable(test_render_engine unit/test_render_engine.cpp)

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
add_skia_test(test_css_rendering unit/test_css_rendering.cpp)

# Advanced CSS Rendering Test
add_skia_test(test_advanced_css unit/test_advanced_css.cpp)

# CSS Variables Test
add_skia_test(test_css_variables unit/test_css_variables.cpp)

# CSS Filters Test
add_skia_test(test_css_filters unit/test_css_filters.cpp)

# Performance Optimization Test
add_skia_test(test_performance_optimization unit/test_performance_optimization.cpp)

# Render Tree Test
add_skia_test(test_render_tree unit/test_render_tree.cpp)
target_link_libraries(test_render_tree PRIVATE lightui_dom)

# Simple Skia Text Test
add_skia_test(test_skia_text_simple unit/test_skia_text_simple.cpp)

# Shadow Renderer Test
add_skia_test(test_shadow_renderer render/test_shadow_renderer.cpp)

# Text Shadow Test
add_skia_test(test_text_shadow render/test_text_shadow.cpp)

# Gradient Renderer Test
add_skia_test(test_gradient_renderer render/test_gradient_renderer.cpp)

# Transform Test
add_skia_test(test_transform render/test_transform.cpp)

# Transition Test
add_skia_test(test_transition render/test_transition.cpp)

# Animation Timeline Test
add_skia_test(test_animation_timeline render/test_animation_timeline.cpp)

# Keyframes Test
add_skia_test(test_keyframes render/test_keyframes.cpp)

# Animation Test
add_skia_test(test_animation render/test_animation.cpp)

# Animation Controller Test
add_skia_test(test_animation_controller render/test_animation_controller.cpp)

# Property Interpolation Test
add_skia_test(test_property_interpolation render/test_property_interpolation.cpp)

# CSS Integration Test (需要额外的库)
add_executable(test_css_integration render/test_css_integration.cpp)
target_include_directories(test_css_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/skia
)
target_link_libraries(test_css_integration PRIVATE
    lightui_render
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET skia)
    target_link_libraries(test_css_integration PRIVATE skia)
    if(MSVC)
        target_compile_options(test_css_integration PRIVATE
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(test_css_integration PRIVATE
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )
    endif()
endif()
add_test(NAME test_css_integration COMMAND test_css_integration)

# ============================================================================
# Benchmark Tests
# ============================================================================

# DOM Benchmark
add_executable(benchmark_dom benchmarks/benchmark_dom.cpp)
target_link_libraries(benchmark_dom PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)
# 注意：基准测试不添加到 CTest，因为它们不是功能测试

# Optimization Test
add_skia_test(test_optimization benchmarks/test_optimization.cpp)

# Advanced Optimization Test
add_skia_test(test_advanced_optimization benchmarks/test_advanced_optimization.cpp)
target_link_libraries(test_advanced_optimization PRIVATE lightui_dom)

# Window Test (Unit tests)
add_executable(test_window unit/test_window.cpp)

target_include_directories(test_window PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_window PRIVATE
    lightui_window
    GTest::gtest
    GTest::gtest_main
)

# Link SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(test_window PRIVATE SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(test_window PRIVATE SDL3-static)
endif()

# Link Skia if available
if(TARGET skia)
    target_link_libraries(test_window PRIVATE skia)

    # Match Skia's runtime library settings (MSVC)
    # Skia 138 is compiled with Release runtime (/MT) and _ITERATOR_DEBUG_LEVEL=0
    # We MUST match these settings even in Debug mode
    if(MSVC)
        target_compile_options(test_window PRIVATE
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(test_window PRIVATE
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )

        # Link D3D12 libraries for Skia
        target_link_libraries(test_window PRIVATE
            d3d12.lib
            dxgi.lib
            d3dcompiler.lib
        )
    endif()
endif()

if(WIN32 AND MINGW)
    target_link_options(test_window PRIVATE -static-libgcc -static-libstdc++)
endif()

add_test(NAME WindowTest COMMAND test_window)

# ============================================================================
# Event Loop Tests
# ============================================================================

# FrameController Test
add_executable(test_frame_controller unit/test_frame_controller.cpp)

target_include_directories(test_frame_controller PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_frame_controller PRIVATE
    lightui_event
    GTest::gtest
    GTest::gtest_main
)

# Link SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(test_frame_controller PRIVATE SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(test_frame_controller PRIVATE SDL3-static)
endif()

# MSVC settings
if(MSVC)
    target_compile_options(test_frame_controller PRIVATE
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(test_frame_controller PRIVATE
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()

add_test(NAME FrameControllerTest COMMAND test_frame_controller)

# TaskScheduler Test
add_executable(test_task_scheduler unit/test_task_scheduler.cpp)

target_include_directories(test_task_scheduler PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_task_scheduler PRIVATE
    lightui_event
    GTest::gtest
    GTest::gtest_main
)

# Link SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(test_task_scheduler PRIVATE SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(test_task_scheduler PRIVATE SDL3-static)
endif()

# MSVC settings
if(MSVC)
    target_compile_options(test_task_scheduler PRIVATE
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(test_task_scheduler PRIVATE
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()

add_test(NAME TaskSchedulerTest COMMAND test_task_scheduler)

# InputHandler Test
add_executable(test_input_handler unit/test_input_handler.cpp)

target_include_directories(test_input_handler PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_input_handler PRIVATE
    lightui_event
    GTest::gtest
    GTest::gtest_main
)

# Link SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(test_input_handler PRIVATE SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(test_input_handler PRIVATE SDL3-static)
endif()

# MSVC settings
if(MSVC)
    target_compile_options(test_input_handler PRIVATE
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(test_input_handler PRIVATE
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()

add_test(NAME InputHandlerTest COMMAND test_input_handler)

# EventLoop Test
add_executable(test_event_loop unit/test_event_loop.cpp)

target_include_directories(test_event_loop PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_event_loop PRIVATE
    lightui_event
    lightui_window
    GTest::gtest
    GTest::gtest_main
)

# Link SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(test_event_loop PRIVATE SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(test_event_loop PRIVATE SDL3-static)
endif()

# Link Skia if available
if(TARGET skia)
    target_link_libraries(test_event_loop PRIVATE skia)

    # MSVC settings
    if(MSVC)
        target_compile_options(test_event_loop PRIVATE
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(test_event_loop PRIVATE
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )

        # Link D3D12 libraries for Skia
        target_link_libraries(test_event_loop PRIVATE
            d3d12.lib
            dxgi.lib
            d3dcompiler.lib
        )
    endif()
endif()

add_test(NAME EventLoopTest COMMAND test_event_loop)

# ============================================================================
# Integration Tests
# ============================================================================

# Integration Test (Window + DOM + Renderer + TaskScheduler)
add_executable(test_integration unit/test_integration.cpp)

target_include_directories(test_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_integration PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
    GTest::gtest
    GTest::gtest_main
)

# Link SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(test_integration PRIVATE SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(test_integration PRIVATE SDL3-static)
endif()

# Link Skia if available
if(TARGET skia)
    target_link_libraries(test_integration PRIVATE skia)

    # MSVC settings
    if(MSVC)
        # Link D3D12 libraries for Skia
        target_link_libraries(test_integration PRIVATE
            d3d12.lib
            dxgi.lib
            d3dcompiler.lib
        )
    endif()
endif()

add_test(NAME IntegrationTest COMMAND test_integration)

# JavaScript Bindings Integration Test
add_executable(test_javascript_bindings unit/test_javascript_bindings.cpp)

target_include_directories(test_javascript_bindings PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_javascript_bindings PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
    GTest::gtest
    GTest::gtest_main
)

# Link SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(test_javascript_bindings PRIVATE SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(test_javascript_bindings PRIVATE SDL3-static)
endif()

# Link Skia if available
if(TARGET skia)
    target_link_libraries(test_javascript_bindings PRIVATE skia)

    # MSVC settings
    if(MSVC)
        # Link D3D12 libraries for Skia
        target_link_libraries(test_javascript_bindings PRIVATE
            d3d12.lib
            dxgi.lib
            d3dcompiler.lib
        )
    endif()
endif()

add_test(NAME JavaScriptBindingsTest COMMAND test_javascript_bindings)

# Lexbor Document Test
add_executable(test_lexbor_document unit/test_lexbor_document.cpp)

target_include_directories(test_lexbor_document PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_lexbor_document PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_lexbor_document PRIVATE lexbor_static)
endif()

add_test(NAME LexborDocumentTest COMMAND test_lexbor_document)

# HTML5 Parser Test
add_executable(test_html5_parser unit/test_html5_parser.cpp)

target_include_directories(test_html5_parser PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_html5_parser PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_html5_parser PRIVATE lexbor_static)
endif()

add_test(NAME HTML5ParserTest COMMAND test_html5_parser)

# HTML5 Error Handling Test
add_executable(test_html5_error_handling unit/test_html5_error_handling.cpp)

target_include_directories(test_html5_error_handling PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_html5_error_handling PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_html5_error_handling PRIVATE lexbor_static)
endif()

add_test(NAME HTML5ErrorHandlingTest COMMAND test_html5_error_handling)

# HTML5 Features Test
add_executable(test_html5_features unit/test_html5_features.cpp)

target_include_directories(test_html5_features PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_html5_features PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_html5_features PRIVATE lexbor_static)
endif()

add_test(NAME HTML5FeaturesTest COMMAND test_html5_features)

# HTML5 Performance Test
add_executable(test_html5_performance unit/test_html5_performance.cpp)

target_include_directories(test_html5_performance PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_html5_performance PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_html5_performance PRIVATE lexbor_static)
endif()

add_test(NAME HTML5PerformanceTest COMMAND test_html5_performance)

# CSS3 Selectors Test
add_executable(test_css3_selectors unit/test_css3_selectors.cpp)

target_include_directories(test_css3_selectors PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_css3_selectors PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_css3_selectors PRIVATE lexbor_static)
endif()

add_test(NAME CSS3SelectorsTest COMMAND test_css3_selectors)

# CSS3 Pseudo Classes Test
add_executable(test_css3_pseudo_classes unit/test_css3_pseudo_classes.cpp)

target_include_directories(test_css3_pseudo_classes PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_css3_pseudo_classes PRIVATE
    lightui_lexbor
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_css3_pseudo_classes PRIVATE lexbor_static)
endif()

add_test(NAME CSS3PseudoClassesTest COMMAND test_css3_pseudo_classes)

# CSS3 Pseudo Elements Test
add_executable(test_css3_pseudo_elements unit/test_css3_pseudo_elements.cpp)

target_include_directories(test_css3_pseudo_elements PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_css3_pseudo_elements PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_css3_pseudo_elements PRIVATE lexbor_static)
endif()

add_test(NAME CSS3PseudoElementsTest COMMAND test_css3_pseudo_elements)

# Form Elements Test
add_executable(test_form_elements unit/test_form_elements.cpp)

target_include_directories(test_form_elements PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_form_elements PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_form_elements PRIVATE lexbor_static)
endif()

add_test(NAME FormElementsTest COMMAND test_form_elements)

# Form Validation Test
add_executable(test_form_validation unit/test_form_validation.cpp)

target_include_directories(test_form_validation PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_form_validation PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_form_validation PRIVATE lexbor_static)
endif()

add_test(NAME FormValidationTest COMMAND test_form_validation)

# Form State Test
add_executable(test_form_state unit/test_form_state.cpp)

target_include_directories(test_form_state PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_form_state PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_form_state PRIVATE lexbor_static)
endif()

add_test(NAME FormStateTest COMMAND test_form_state)

# DOM Lexbor Integration Test
add_executable(test_dom_lexbor_integration unit/test_dom_lexbor_integration.cpp)

target_include_directories(test_dom_lexbor_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_dom_lexbor_integration PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_dom_lexbor_integration PRIVATE lexbor_static)
endif()

add_test(NAME DOMDocumentLexborIntegrationTest COMMAND test_dom_lexbor_integration)

# Lexbor StyleSheet Test
add_executable(test_lexbor_stylesheet unit/test_lexbor_stylesheet.cpp)

target_include_directories(test_lexbor_stylesheet PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_lexbor_stylesheet PRIVATE
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_lexbor_stylesheet PRIVATE lexbor_static)
endif()

add_test(NAME LexborStyleSheetTest COMMAND test_lexbor_stylesheet)

# Style Manager Test
add_executable(test_style_manager unit/test_style_manager.cpp)

target_include_directories(test_style_manager PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_style_manager PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_style_manager PRIVATE lexbor_static)
endif()

add_test(NAME StyleManagerTest COMMAND test_style_manager)

# Cascade Engine Test
add_executable(test_cascade_engine unit/test_cascade_engine.cpp)

target_include_directories(test_cascade_engine PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_cascade_engine PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_cascade_engine PRIVATE lexbor_static)
endif()

add_test(NAME CascadeEngineTest COMMAND test_cascade_engine)

# Style Cache Test
add_executable(test_style_cache unit/test_style_cache.cpp)

target_include_directories(test_style_cache PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)

target_link_libraries(test_style_cache PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)

# Link Lexbor
if(TARGET lexbor_static)
    target_link_libraries(test_style_cache PRIVATE lexbor_static)
endif()

add_test(NAME StyleCacheTest COMMAND test_style_cache)

# HTML Button Element Test
add_executable(test_html_button_element unit/test_html_button_element.cpp)
target_include_directories(test_html_button_element PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_html_button_element PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET lexbor_static)
    target_link_libraries(test_html_button_element PRIVATE lexbor_static)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_html_button_element PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME HTMLButtonElementTest COMMAND test_html_button_element)

# HTML Select Element Test
add_executable(test_html_select_element unit/test_html_select_element.cpp)
target_include_directories(test_html_select_element PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_html_select_element PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET lexbor_static)
    target_link_libraries(test_html_select_element PRIVATE lexbor_static)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_html_select_element PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME HTMLSelectElementTest COMMAND test_html_select_element)

# HTML Form Element Test
add_executable(test_html_form_element unit/test_html_form_element.cpp)
target_include_directories(test_html_form_element PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_html_form_element PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET lexbor_static)
    target_link_libraries(test_html_form_element PRIVATE lexbor_static)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_html_form_element PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME HTMLFormElementTest COMMAND test_html_form_element)

# HTML Anchor Element Test
add_executable(test_html_anchor_element unit/test_html_anchor_element.cpp)
target_include_directories(test_html_anchor_element PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_html_anchor_element PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET lexbor_static)
    target_link_libraries(test_html_anchor_element PRIVATE lexbor_static)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_html_anchor_element PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME HTMLAnchorElementTest COMMAND test_html_anchor_element)

# HTML Label Element Test
add_executable(test_html_label_element unit/test_html_label_element.cpp)
target_include_directories(test_html_label_element PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_html_label_element PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET lexbor_static)
    target_link_libraries(test_html_label_element PRIVATE lexbor_static)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_html_label_element PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME HTMLLabelElementTest COMMAND test_html_label_element)

# HTML Image Element Test
add_executable(test_html_image_element unit/test_html_image_element.cpp)
target_include_directories(test_html_image_element PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_html_image_element PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET lexbor_static)
    target_link_libraries(test_html_image_element PRIVATE lexbor_static)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_html_image_element PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME HTMLImageElementTest COMMAND test_html_image_element)

# HTML Container Elements Test (div, span, p, h1-h6)
add_executable(test_html_container_elements unit/test_html_container_elements.cpp)
target_include_directories(test_html_container_elements PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_html_container_elements PRIVATE
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET lexbor_static)
    target_link_libraries(test_html_container_elements PRIVATE lexbor_static)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_html_container_elements PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME HTMLContainerElementsTest COMMAND test_html_container_elements)

# ============================================================================
# Integration Tests
# ============================================================================

# DOM Integration Test
add_executable(test_dom_integration integration/test_dom_integration.cpp)
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
add_executable(test_dom_bindings_integration integration/test_dom_bindings_integration.cpp)
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

# innerHTML Debug Test
add_executable(test_innerHTML_debug integration/test_innerHTML_debug.cpp)
target_include_directories(test_innerHTML_debug PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)
target_link_libraries(test_innerHTML_debug PRIVATE
    lightui_dom
)

# Preact Integration Test
add_executable(test_preact_integration integration/test_preact_integration.cpp)
target_include_directories(test_preact_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
)
target_link_libraries(test_preact_integration PRIVATE
    lightui_quickjs
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_preact_integration PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_preact_integration PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME PreactIntegrationTest COMMAND test_preact_integration WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Preact Basic Test
add_executable(test_preact_basic integration/test_preact_basic.cpp)
target_include_directories(test_preact_basic PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
)
target_link_libraries(test_preact_basic PRIVATE
    lightui_quickjs
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_preact_basic PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_preact_basic PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME PreactBasicTest COMMAND test_preact_basic WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Preact Render Test
add_executable(test_preact_render integration/test_preact_render.cpp)
target_include_directories(test_preact_render PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
)
target_link_libraries(test_preact_render PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_preact_render PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_preact_render PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME PreactRenderTest COMMAND test_preact_render WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Preact Components Test
add_executable(test_preact_components integration/test_preact_components.cpp)
target_include_directories(test_preact_components PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
)
target_link_libraries(test_preact_components PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_preact_components PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_preact_components PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME PreactComponentsTest COMMAND test_preact_components WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# ============================================================================
# Memory Safety Tests
# ============================================================================

# JSValueWrapper Test
add_executable(test_js_value_wrapper test_js_value_wrapper.cpp)
target_include_directories(test_js_value_wrapper PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
)
target_link_libraries(test_js_value_wrapper PRIVATE
    lightui_quickjs
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_js_value_wrapper PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_js_value_wrapper PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME JSValueWrapperTest COMMAND test_js_value_wrapper)

# Memory Leak Fix Test
add_executable(test_memory_leak_fix unit/test_memory_leak_fix.cpp)
target_include_directories(test_memory_leak_fix PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_memory_leak_fix PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_memory_leak_fix PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_memory_leak_fix PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME MemoryLeakFixTest COMMAND test_memory_leak_fix)

# Object Cache Test
add_executable(test_object_cache unit/test_object_cache.cpp)
target_include_directories(test_object_cache PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_object_cache PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_object_cache PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_object_cache PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME ObjectCacheTest COMMAND test_object_cache)

# Remove Event Listener Test
add_executable(test_remove_event_listener unit/test_remove_event_listener.cpp)
target_include_directories(test_remove_event_listener PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_remove_event_listener PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_remove_event_listener PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_remove_event_listener PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME RemoveEventListenerTest COMMAND test_remove_event_listener)

# ========== Integration Tests ==========

# Stress Test
add_executable(test_stress integration/test_stress.cpp)
target_include_directories(test_stress PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_stress PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_stress PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_stress PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME StressTest COMMAND test_stress)

# Long Running Test
add_executable(test_long_running integration/test_long_running.cpp)
target_include_directories(test_long_running PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
    ${THIRD_PARTY_DIR}/lexbor/source
)
target_link_libraries(test_long_running PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_lexbor
    GTest::gtest
    GTest::gtest_main
)
if(TARGET quickjs)
    target_link_libraries(test_long_running PRIVATE quickjs)
endif()
if(WIN32 AND MINGW)
    target_link_options(test_long_running PRIVATE -static-libgcc -static-libstdc++)
endif()
add_test(NAME LongRunningTest COMMAND test_long_running)


# ========== Benchmark Tests ==========

# Timer Performance Benchmark
add_executable(benchmark_timer_performance
    benchmarks/benchmark_timer_performance.cpp
)

target_link_libraries(benchmark_timer_performance
    PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_lexbor
    lightui_utils
    gtest
    gtest_main
)

target_include_directories(benchmark_timer_performance
    PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

set_target_properties(benchmark_timer_performance PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

# ============================================================================
# Performance Tests
# ============================================================================

# Parsing Performance Test
add_executable(test_parsing_performance
    performance/test_parsing_performance.cpp
)

target_link_libraries(test_parsing_performance
    PRIVATE
    lightui_lexbor
    gtest
    gtest_main
)

target_include_directories(test_parsing_performance
    PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tests
)

set_target_properties(test_parsing_performance PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

add_test(NAME ParsingPerformanceTest COMMAND test_parsing_performance)

# Selector Performance Test
add_executable(test_selector_performance
    performance/test_selector_performance.cpp
)

target_link_libraries(test_selector_performance
    PRIVATE
    lightui_lexbor
    gtest
    gtest_main
)

target_include_directories(test_selector_performance
    PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tests
)

set_target_properties(test_selector_performance PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

add_test(NAME SelectorPerformanceTest COMMAND test_selector_performance)

# DOM Performance Test
add_executable(test_dom_performance
    performance/test_dom_performance.cpp
)

target_link_libraries(test_dom_performance
    PRIVATE
    lightui_lexbor
    gtest
    gtest_main
)

target_include_directories(test_dom_performance
    PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tests
)

set_target_properties(test_dom_performance PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

add_test(NAME DOMPerformanceTest COMMAND test_dom_performance)

# Form Performance Test
add_executable(test_form_performance
    performance/test_form_performance.cpp
)

target_link_libraries(test_form_performance
    PRIVATE
    lightui_lexbor
    gtest
    gtest_main
)

target_include_directories(test_form_performance
    PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tests
)

set_target_properties(test_form_performance PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

add_test(NAME FormPerformanceTest COMMAND test_form_performance)

# Memory Stress Test
add_executable(test_memory_stress
    performance/test_memory_stress.cpp
)

target_link_libraries(test_memory_stress
    PRIVATE
    lightui_lexbor
    gtest
    gtest_main
)

target_include_directories(test_memory_stress
    PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tests
)

set_target_properties(test_memory_stress PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

add_test(NAME MemoryStressTest COMMAND test_memory_stress)

# ============================================================================
# IFC (Inline Formatting Context) Tests
# ============================================================================

# IFC Unit Test
add_executable(test_ifc unit/test_ifc.cpp)

target_include_directories(test_ifc PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(test_ifc PRIVATE
    lightui_layout
    lightui_render
    GTest::gtest
    GTest::gtest_main
)

# Link Skia if available
if(TARGET skia)
    target_link_libraries(test_ifc PRIVATE skia)

    if(MSVC)
        target_compile_options(test_ifc PRIVATE
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(test_ifc PRIVATE
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )
    endif()
endif()

if(WIN32 AND MINGW)
    target_link_options(test_ifc PRIVATE -static-libgcc -static-libstdc++)
endif()

add_test(NAME IFCTest COMMAND test_ifc)

# ============================================================================
# Layout Comparison Tests
# ============================================================================

add_subdirectory(layout_comparison)
