# DOM模块 CMakeLists.txt

# 包含 elements 子目录
add_subdirectory(elements)

set(DOM_SOURCES
    node.cpp
    element.cpp
    text.cpp
    document.cpp
    event.cpp
    drag_event.cpp
    range.cpp
    selection.cpp
    mutation_observer.cpp
    dom_bindings.cpp
    dom_observer.cpp
    dirty_node_tracker.cpp
    incremental_style_recalc.cpp
    selector_engine.cpp
    dom_token_list.cpp
    css_style_declaration.cpp
    dom_string_map.cpp
    canvas_bindings.cpp
    # HTML 元素文件已移动到 elements/ 子目录
    elements/html_input_element.cpp
    elements/html_textarea_element.cpp
    elements/html_button_element.cpp
    elements/html_form_element.cpp
    elements/html_select_element.cpp
    elements/html_option_element.cpp
    elements/html_anchor_element.cpp
    elements/html_label_element.cpp
    elements/html_image_element.cpp
    elements/html_div_element.cpp
    elements/html_span_element.cpp
    elements/html_paragraph_element.cpp
    elements/html_heading_element.cpp
    elements/html_ulist_element.cpp
    elements/html_olist_element.cpp
    elements/html_li_element.cpp
    elements/html_table_element.cpp
    elements/html_form_controls.cpp
    elements/html_canvas_element.cpp
    elements/svg_element.cpp
    elements/html_head_element.cpp
    elements/html_style_element.cpp
    elements/html_script_element.cpp
    elements/html_link_element.cpp
)

set(DOM_HEADERS
    node.h
    element.h
    text.h
    document.h
    event.h
    drag_event.h
    range.h
    selection.h
    mutation_observer.h
    dom_bindings.h
    dom_observer.h
    dirty_node_tracker.h
    incremental_style_recalc.h
    selector_engine.h
    dom_token_list.h
    css_style_declaration.h
    dom_string_map.h
    canvas_bindings.h
    # HTML 元素头文件已移动到 elements/ 子目录
    elements/html_input_element.h
    elements/html_textarea_element.h
    elements/html_button_element.h
    elements/html_form_element.h
    elements/html_select_element.h
    elements/html_option_element.h
    elements/html_anchor_element.h
    elements/html_label_element.h
    elements/html_image_element.h
    elements/html_div_element.h
    elements/html_span_element.h
    elements/html_paragraph_element.h
    elements/html_heading_element.h
    elements/html_ulist_element.h
    elements/html_olist_element.h
    elements/html_li_element.h
    elements/html_table_element.h
    elements/html_form_controls.h
    elements/html_canvas_element.h
    elements/svg_element.h
    elements/html_head_element.h
    elements/html_style_element.h
    elements/html_script_element.h
    elements/html_link_element.h
)

add_library(lightui_dom STATIC ${DOM_SOURCES})

target_include_directories(lightui_dom PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/lexbor/source
    ${CMAKE_SOURCE_DIR}/third_party/quickjs
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include
)

target_link_libraries(lightui_dom PUBLIC
    lightui_utils
    lightui_event
    lightui_lexbor
    lightui_render
    lexbor_static
)

# 链接Skia（如果启用）- 需要SkRect.h
if(TARGET skia)
    target_link_libraries(lightui_dom PUBLIC skia)
endif()

# 匹配 Skia 的运行时库设置
if(MSVC)
    target_compile_options(lightui_dom PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(lightui_dom PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()
