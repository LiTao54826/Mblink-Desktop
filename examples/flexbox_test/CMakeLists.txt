# Flexbox Test Example

add_executable(flexbox_test main.cpp)

target_link_libraries(flexbox_test PRIVATE
    lightui_window
    lightui_event
    lightui_dom
    lightui_render
    lightui_quickjs
    lightui_lexbor
    lightui_utils
)

# Copy HTML file to output directory
add_custom_command(TARGET flexbox_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying flexbox_test files to output directory"
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:flexbox_test>/examples/window_demo
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/examples/window_demo/flexbox_test.html
        $<TARGET_FILE_DIR:flexbox_test>/examples/window_demo/flexbox_test.html
)

set_target_properties(flexbox_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

