# 综合功能测试应用

add_executable(comprehensive_test_app
    main.cpp
)

target_link_libraries(comprehensive_test_app PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_event
    lightui_window
    lightui_render
    lightui_lexbor
    lightui_utils
    SDL3::SDL3-static
    skia
)

# 设置输出目录
set_target_properties(comprehensive_test_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# 复制 JavaScript 文件到输出目录
add_custom_command(TARGET comprehensive_test_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_FILE_DIR:comprehensive_test_app>/../../../examples/comprehensive_test_app
    COMMENT "Copying test files to output directory"
)

