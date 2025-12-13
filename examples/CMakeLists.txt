# LightUI Examples CMake Configuration

cmake_minimum_required(VERSION 3.15)

message(STATUS "Configuring LightUI examples...")

# ========== 复制 demo_html 和 js 到构建目录 ==========
# 创建自定义目标统一复制资源文件
add_custom_target(copy_demo_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/examples/demo_html
        ${CMAKE_BINARY_DIR}/examples/demo_html
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/js
        ${CMAKE_BINARY_DIR}/examples/demo_html/js
    # 同时复制到 build 根目录的 js 目录（html_loader 使用）
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/js
        ${CMAKE_BINARY_DIR}/js
    COMMENT "Copying demo_html and js resources to build directory"
)

# ========== C++ Examples ==========

# Integration Example
add_executable(integration_example
    cpp/integration_example.cpp
)
target_link_libraries(integration_example PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    SDL3::SDL3-static
    skia
)
if(MSVC)
    target_compile_options(integration_example PRIVATE /W3 /EHsc)
endif()
set_target_properties(integration_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# JavaScript Integration Example
add_executable(javascript_integration_example
    cpp/javascript_integration_example.cpp
)
target_link_libraries(javascript_integration_example PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
    SDL3::SDL3-static
    skia
)
if(MSVC)
    target_compile_options(javascript_integration_example PRIVATE /W3 /EHsc)
endif()
set_target_properties(javascript_integration_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# Hello World Example
add_executable(hello_world
    cpp/hello_world.cpp
)
target_link_libraries(hello_world PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    SDL3::SDL3-static
    skia
)
if(MSVC)
    target_compile_options(hello_world PRIVATE /W3 /EHsc)
endif()
set_target_properties(hello_world PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# Counter App Example
add_executable(counter_app
    cpp/counter_app.cpp
)

target_link_libraries(counter_app PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
    lightui_lexbor
    SDL3::SDL3-static
    skia
)

if(MSVC)
    target_compile_options(counter_app PRIVATE /W3 /EHsc)
endif()

set_target_properties(counter_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# Animation Demo Example
add_executable(animation_demo
    cpp/animation_demo.cpp
)
target_link_libraries(animation_demo PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
    SDL3::SDL3-static
    skia
)
if(MSVC)
    target_compile_options(animation_demo PRIVATE /W3 /EHsc)
endif()
set_target_properties(animation_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# HTML Loading Example
add_executable(html_loading_example
    cpp/html_loading_example.cpp
)
target_link_libraries(html_loading_example PRIVATE
    lightui_dom
    lightui_lexbor
)
if(MSVC)
    target_compile_options(html_loading_example PRIVATE /W3 /EHsc)
endif()
set_target_properties(html_loading_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# HTML Window Example
add_executable(html_window_example
    cpp/html_window_example.cpp
)

target_link_libraries(html_window_example PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_lexbor
    SDL3::SDL3-static
    skia
)

if(MSVC)
    target_compile_options(html_window_example PRIVATE /W3 /EHsc)
endif()

set_target_properties(html_window_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# ========== Comprehensive Test App ==========
add_subdirectory(comprehensive_test_app)

# ========== Window Demo ==========
add_subdirectory(window_demo)



# ========== Preact Counter Example ==========
add_subdirectory(preact_counter)

# ========== Preact Window Demo ==========
add_subdirectory(preact_window_demo)

# ========== Preact Todo App ==========
add_subdirectory(preact_todo_app)

# ========== Preact Form Demo ==========
add_subdirectory(preact_form_demo)

# ========== Flexbox Test ==========
add_subdirectory(flexbox_test)

# ========== SDL Test ==========
add_subdirectory(sdl_test)

# ========== Input Test App ==========
add_subdirectory(input_test_app)

# ========== Table Test App ==========
add_subdirectory(table_test_app)

# ========== HTML Tags Test ==========
add_subdirectory(html_tags_test)

# ========== Element Test ==========
add_subdirectory(element_test)

# ========== Layout Compare Test ==========
add_subdirectory(layout_compare_test)

message(STATUS "Examples configured successfully")
message(STATUS "  - integration_example")
message(STATUS "  - javascript_integration_example")
message(STATUS "  - hello_world")
message(STATUS "  - counter_app")
message(STATUS "  - animation_demo")
message(STATUS "  - html_loading_example")
message(STATUS "  - html_window_example")
message(STATUS "  - html_css_performance_demo")
message(STATUS "  - comprehensive_test_app")
message(STATUS "  - window_demo")

message(STATUS "  - preact_counter")
message(STATUS "  - preact_window_demo")
message(STATUS "  - preact_todo_app")
message(STATUS "  - preact_form_demo")
message(STATUS "  - flexbox_test")
message(STATUS "  - layout_compare_test")
message(STATUS "  - incremental_render_test")

# Incremental Render Test
add_executable(incremental_render_test
    incremental_render_test.cpp
)
target_link_libraries(incremental_render_test PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    SDL3::SDL3-static
    skia
)
if(MSVC)
    target_compile_options(incremental_render_test PRIVATE /W3 /EHsc)
endif()
set_target_properties(incremental_render_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

