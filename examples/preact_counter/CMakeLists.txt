# Preact Counter示例

add_executable(preact_counter
    main.cpp
)

target_link_libraries(preact_counter
    lightui_dom
    lightui_quickjs
    lightui_lexbor
)

# Preact Counter Interactive Test
add_executable(preact_counter_test
    test_interactive.cpp
)

target_link_libraries(preact_counter_test
    lightui_dom
    lightui_quickjs
    lightui_lexbor
    lightui_event
)

# 复制JavaScript文件到构建目录
add_custom_command(TARGET preact_counter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying example files..."

    # 创建目录
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/js/preact
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/preact_counter

    # 复制 Preact 库文件
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/js/preact/preact.js
        ${CMAKE_BINARY_DIR}/js/preact/preact.js
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/js/preact/hooks.js
        ${CMAKE_BINARY_DIR}/js/preact/hooks.js

    # 复制应用文件
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/examples/preact_counter/app.js
        ${CMAKE_BINARY_DIR}/examples/preact_counter/app.js

    COMMENT "Copying Preact Counter example files"
)

