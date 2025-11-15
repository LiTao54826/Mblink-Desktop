# MBink Window Demo

add_executable(window_demo
    main.cpp
)

target_link_libraries(window_demo PRIVATE
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

# 设置 UTF-8 编码支持（MSVC 需要）
if(MSVC)
    target_compile_options(window_demo PRIVATE /utf-8)
endif()

# 设置输出目录
set_target_properties(window_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# 复制资源文件到输出目录
add_custom_command(TARGET window_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_FILE_DIR:window_demo>/../../../examples/window_demo
    COMMENT "Copying window_demo files to output directory"
)

