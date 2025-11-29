# Preact Todo App Example

add_executable(preact_todo_app
    main.cpp
)

target_link_libraries(preact_todo_app
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
    lightui_lexbor
    SDL3::SDL3-static
    skia
)

# 输出到 bin/ 目录
set_target_properties(preact_todo_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# 依赖资源复制目标
add_dependencies(preact_todo_app copy_demo_resources)

