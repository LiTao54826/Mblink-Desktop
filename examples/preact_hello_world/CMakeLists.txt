# Preact Hello World Example

add_executable(preact_hello_world main.cpp)

target_include_directories(preact_hello_world PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${THIRD_PARTY_DIR}/quickjs
)

target_link_libraries(preact_hello_world PRIVATE
    lightui_quickjs
    lightui_dom
    lightui_lexbor
)

# Link QuickJS
if(TARGET quickjs)
    target_link_libraries(preact_hello_world PRIVATE quickjs)
endif()

if(WIN32 AND MINGW)
    target_link_options(preact_hello_world PRIVATE -static-libgcc -static-libstdc++)
endif()

# Copy JavaScript files to build directory
add_custom_command(TARGET preact_hello_world POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/js
    $<TARGET_FILE_DIR:preact_hello_world>/js
    COMMENT "Copying JavaScript files..."
)

add_custom_command(TARGET preact_hello_world POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/examples/preact_hello_world
    $<TARGET_FILE_DIR:preact_hello_world>/examples/preact_hello_world
    COMMENT "Copying example files..."
)

