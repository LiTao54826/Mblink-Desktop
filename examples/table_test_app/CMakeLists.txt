# 表格测试应用 - 测试HTML表格标签渲染

add_executable(table_test_app
    main.cpp
)

target_include_directories(table_test_app PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(table_test_app PRIVATE
    lightui_window
    lightui_dom
    lightui_quickjs
    lightui_event
    lightui_render
    lightui_layout
    lightui_utils
    lightui_lexbor
    qjs
    SDL3::SDL3-static
    skia
)

# Windows 特定设置
if(WIN32)
    # 添加必要的 Windows 库
    target_link_libraries(table_test_app PRIVATE
        dwmapi
        opengl32
    )
endif()

# 设置输出目录
set_target_properties(table_test_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
)

