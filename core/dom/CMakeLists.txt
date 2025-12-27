# DOM模块 CMakeLists.txt

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
    html_input_element.cpp
    html_textarea_element.cpp
    html_button_element.cpp
    html_form_element.cpp
    html_select_element.cpp
    html_option_element.cpp
    html_anchor_element.cpp
    html_label_element.cpp
    html_image_element.cpp
    html_div_element.cpp
    html_span_element.cpp
    html_paragraph_element.cpp
    html_heading_element.cpp
    html_ulist_element.cpp
    html_olist_element.cpp
    html_li_element.cpp
    html_table_element.cpp
    html_form_controls.cpp
    html_canvas_element.cpp
    canvas_bindings.cpp
    svg_element.cpp
    html_head_element.cpp
    html_style_element.cpp
    html_script_element.cpp
    html_link_element.cpp
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
    html_input_element.h
    html_textarea_element.h
    html_button_element.h
    html_form_element.h
    html_select_element.h
    html_option_element.h
    html_anchor_element.h
    html_label_element.h
    html_image_element.h
    html_div_element.h
    html_span_element.h
    html_paragraph_element.h
    html_heading_element.h
    html_ulist_element.h
    html_olist_element.h
    html_li_element.h
    html_table_element.h
    html_form_controls.h
    html_canvas_element.h
    canvas_bindings.h
    svg_element.h
    html_head_element.h
    html_style_element.h
    html_script_element.h
    html_link_element.h
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
