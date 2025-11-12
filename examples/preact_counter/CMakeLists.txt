# Preact Counter示例

add_executable(preact_counter
    main.cpp
)

target_link_libraries(preact_counter
    lightui_dom
    lightui_quickjs
)

# 复制JavaScript文件到构建目录
add_custom_command(TARGET preact_counter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying example files..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/examples/preact_counter/app.js
        ${CMAKE_BINARY_DIR}/examples/preact_counter/app.js
    COMMENT "Copying Preact Counter example files"
)

