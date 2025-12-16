# MBink HTML Loader - HTML 文件加载器

add_executable(html_loader
    main.cpp
)

target_link_libraries(html_loader
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
    lightui_lexbor
    lightui_network
    lightui_devtools
    SDL3::SDL3-static
    skia
)

# 输出到 bin/ 目录
set_target_properties(html_loader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

