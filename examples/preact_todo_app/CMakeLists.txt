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

# Copy JavaScript files to build directory
add_custom_command(TARGET preact_todo_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying example files..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/examples/preact_todo_app/app.js
        ${CMAKE_BINARY_DIR}/examples/preact_todo_app/app.js
    COMMENT "Copying Preact Todo App files"
)

