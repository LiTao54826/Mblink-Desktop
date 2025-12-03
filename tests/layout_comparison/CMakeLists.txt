# Layout Comparison Test
# 布局比较测试 - 与浏览器渲染结果对比

add_executable(test_layout_comparison
    layout_comparison_test.cpp
)

target_include_directories(test_layout_comparison PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party/skia
    ${PROJECT_SOURCE_DIR}/third_party/taffy-cpp/include
)

target_link_libraries(test_layout_comparison PRIVATE
    gtest
    gtest_main
    lightui_window
    lightui_dom
    lightui_render
    lightui_layout
    lightui_event
    lightui_lexbor
    lightui_quickjs
    lightui_utils
)

# 复制测试 HTML 文件到构建目录
add_custom_command(TARGET test_layout_comparison POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/layout_comparison
    COMMENT "Copying layout comparison test files..."
)

# 添加到 CTest
add_test(NAME LayoutComparisonTest COMMAND test_layout_comparison)

set_target_properties(test_layout_comparison PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

