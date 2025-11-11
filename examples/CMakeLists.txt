# LightUI Examples CMake Configuration

cmake_minimum_required(VERSION 3.15)

message(STATUS "Configuring LightUI examples...")

# ========== Integration Example ==========
add_executable(integration_example
    integration_example.cpp
)

target_link_libraries(integration_example PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    SDL3::SDL3-static
    skia
)

# MSVC 特定设置
if(MSVC)
    target_compile_options(integration_example PRIVATE
        /W3
        /EHsc
    )
endif()

# 设置输出目录
set_target_properties(integration_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# ========== JavaScript Integration Example ==========
add_executable(javascript_integration_example
    javascript_integration_example.cpp
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

# MSVC 特定设置
if(MSVC)
    target_compile_options(javascript_integration_example PRIVATE
        /W3
        /EHsc
    )
endif()

# 设置输出目录
set_target_properties(javascript_integration_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# ========== Hello World Example ==========
add_executable(hello_world
    hello_world.cpp
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

# ========== Counter App Example ==========
add_executable(counter_app
    counter_app.cpp
)

target_link_libraries(counter_app PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
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

# ========== Animation Demo Example ==========
add_executable(animation_demo
    animation_demo.cpp
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

# ========== HTML Loading Example ==========
add_executable(html_loading_example
    html_loading_example.cpp
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

# ========== HTML Window Example ==========
add_executable(html_window_example
    html_window_example.cpp
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

message(STATUS "Examples configured successfully")
message(STATUS "  - integration_example")
message(STATUS "  - javascript_integration_example")
message(STATUS "  - hello_world")
message(STATUS "  - counter_app")
message(STATUS "  - animation_demo")
message(STATUS "  - html_loading_example")
message(STATUS "  - html_window_example")

