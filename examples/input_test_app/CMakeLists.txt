# 输入框测试应用 - 测试中文输入和快捷键

add_executable(input_test_app
    main.cpp
)

target_include_directories(input_test_app PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(input_test_app PRIVATE
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
    # 设置为窗口应用程序（同时保留控制台输出用于调试）
    # set_target_properties(input_test_app PROPERTIES WIN32_EXECUTABLE TRUE)
    
    # 添加必要的 Windows 库
    target_link_libraries(input_test_app PRIVATE
        dwmapi
        opengl32
    )
endif()

# 设置输出目录
set_target_properties(input_test_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
)

