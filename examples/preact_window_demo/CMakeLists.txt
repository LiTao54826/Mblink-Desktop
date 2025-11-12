# Preact Window Demo

add_executable(preact_window_demo
    main.cpp
)

target_link_libraries(preact_window_demo
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
    lightui_lexbor
    SDL3::SDL3-static
    skia
)

# MSVC 特定设置
if(MSVC)
    target_compile_options(preact_window_demo PRIVATE
        /W3
        /EHsc
    )
endif()

# 设置输出目录
set_target_properties(preact_window_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# 复制JavaScript文件到构建目录
add_custom_command(TARGET preact_window_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying example files..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/examples/preact_window_demo/app.js
        ${CMAKE_BINARY_DIR}/examples/preact_window_demo/app.js
    COMMENT "Copying Preact Window Demo files"
)

