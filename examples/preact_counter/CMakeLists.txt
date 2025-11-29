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

# 依赖资源复制目标
add_dependencies(preact_counter copy_demo_resources)

