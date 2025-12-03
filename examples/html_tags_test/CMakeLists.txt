# HTML Tags Comprehensive Test
# 测试所有已实现的 HTML 标签 (Phase 1-5)

add_executable(html_tags_test main.cpp)

target_include_directories(html_tags_test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/core/quickjs
    ${CMAKE_SOURCE_DIR}/third_party/quickjs
)

target_link_libraries(html_tags_test PRIVATE
    lightui_window
    lightui_dom
    lightui_render
    lightui_layout
    lightui_event
    lightui_quickjs
    lightui_lexbor
    lightui_utils
    qjs
    SDL3::SDL3-static
    skia
    lexbor_static
)

# 设置输出目录
set_target_properties(html_tags_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

