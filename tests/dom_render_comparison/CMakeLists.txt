# DOM 元素渲染比较测试
# 用于验证 MBink 渲染结果与浏览器一致

set(DOM_RENDER_TEST_SOURCES
    dom_render_test.cpp
)

add_executable(dom_render_test ${DOM_RENDER_TEST_SOURCES})

target_include_directories(dom_render_test PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/core
    ${PROJECT_SOURCE_DIR}/third_party/skia/Release/include
)

target_link_libraries(dom_render_test PRIVATE
    lightui_dom
    lightui_css
    lightui_render
    lightui_layout
    lightui_lexbor
)

# 复制测试资源文件
add_custom_command(TARGET dom_render_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/test_cases
        $<TARGET_FILE_DIR:dom_render_test>/test_cases
    COMMENT "Copying DOM render test cases..."
)

