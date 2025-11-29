# Preact Form Demo Example

add_executable(preact_form_demo
    main.cpp
)

target_link_libraries(preact_form_demo
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
add_custom_command(TARGET preact_form_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying preact_form_demo files..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/examples/preact_form_demo/app.js
        ${CMAKE_BINARY_DIR}/examples/preact_form_demo/app.js
    COMMENT "Copying Preact Form Demo files"
)

