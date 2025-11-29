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

# 依赖资源复制目标
add_dependencies(preact_form_demo copy_demo_resources)

