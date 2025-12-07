# DOM 元素渲染比较测试
# 用于验证 MBink 渲染结果与浏览器一致

add_executable(dom_render_test
    dom_render_test.cpp
)

target_include_directories(dom_render_test PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/core
    ${PROJECT_SOURCE_DIR}/third_party/skia
    ${PROJECT_SOURCE_DIR}/third_party/taffy-cpp/include
)

target_link_libraries(dom_render_test PRIVATE
    lightui_window
    lightui_dom
    lightui_render
    lightui_layout
    lightui_lexbor
    lightui_quickjs
    lightui_event
    lightui_utils
)

# Link Skia if available
if(TARGET skia)
    target_link_libraries(dom_render_test PRIVATE skia)

    if(MSVC)
        target_compile_options(dom_render_test PRIVATE
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(dom_render_test PRIVATE
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )

        # Link D3D12 libraries for Skia
        target_link_libraries(dom_render_test PRIVATE
            d3d12.lib
            dxgi.lib
            d3dcompiler.lib
        )
    endif()
endif()

# Link SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(dom_render_test PRIVATE SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(dom_render_test PRIVATE SDL3-static)
endif()

# 设置输出目录
set_target_properties(dom_render_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

# 复制测试资源文件到输出目录
add_custom_command(TARGET dom_render_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/test_cases
        $<TARGET_FILE_DIR:dom_render_test>/dom_render_test_cases
    COMMENT "Copying DOM render test cases..."
)

# 复制 Preact 库到输出目录
add_custom_command(TARGET dom_render_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/js/preact
        $<TARGET_FILE_DIR:dom_render_test>/js/preact
    COMMENT "Copying Preact library..."
)

# 复制 Python 比较脚本
add_custom_command(TARGET dom_render_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/compare_render.py
        $<TARGET_FILE_DIR:dom_render_test>/compare_render.py
    COMMENT "Copying compare script..."
)

