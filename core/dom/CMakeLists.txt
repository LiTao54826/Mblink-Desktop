# DOM模块 CMakeLists.txt

# 包含子目录
add_subdirectory(elements)
add_subdirectory(bindings)
add_subdirectory(observers)
add_subdirectory(selection)
add_subdirectory(style)
add_subdirectory(utils)

# DOM 核心源文件（保留在根目录）
set(DOM_CORE_SOURCES
    node.cpp
    element.cpp
    text.cpp
    comment.cpp
    document.cpp
    document_fragment.cpp
    event.cpp
    drag_event.cpp
    file_list.cpp
    file_selection_policy.cpp
)

set(DOM_CORE_HEADERS
    node.h
    element.h
    text.h
    comment.h
    document.h
    event.h
    drag_event.h
    file_list.h
    file_selection_policy.h
)

# 子目录源文件
set(DOM_BINDINGS_SOURCES
    bindings/dom_bindings.cpp
    bindings/canvas_bindings.cpp
    bindings/native_data_binding.cpp
    bindings/terminal_bindings.cpp
)

set(DOM_OBSERVERS_SOURCES
    observers/dom_observer.cpp
    observers/mutation_observer.cpp
    observers/dirty_node_tracker.cpp
)

set(DOM_SELECTION_SOURCES
    selection/selection.cpp
    selection/range.cpp
    selection/selector_engine.cpp
)

set(DOM_STYLE_SOURCES
    style/css_style_declaration.cpp
    style/incremental_style_recalc.cpp
)

set(DOM_UTILS_SOURCES
    utils/dom_token_list.cpp
    utils/dom_string_map.cpp
)

# HTML 元素源文件
set(DOM_ELEMENTS_SOURCES
    elements/html_input_element.cpp
    elements/html_textarea_element.cpp
    elements/html_button_element.cpp
    elements/html_form_element.cpp
    elements/html_select_element.cpp
    elements/html_option_element.cpp
    elements/html_anchor_element.cpp
    elements/html_audio_element.cpp
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
    elements/html_template_element.cpp
)

# 合并所有源文件
set(DOM_SOURCES
    ${DOM_CORE_SOURCES}
    ${DOM_BINDINGS_SOURCES}
    ${DOM_OBSERVERS_SOURCES}
    ${DOM_SELECTION_SOURCES}
    ${DOM_STYLE_SOURCES}
    ${DOM_UTILS_SOURCES}
    ${DOM_ELEMENTS_SOURCES}
)

add_library(mblink_dom STATIC ${DOM_SOURCES})

target_include_directories(mblink_dom PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/lexbor/source
    ${CMAKE_SOURCE_DIR}/third_party/quickjs
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include
)

target_link_libraries(mblink_dom PUBLIC
    mblink_utils
    mblink_event
    mblink_lexbor
    mblink_render
    lexbor_static
    virtual_text
    terminal_element
    logview_element
)

if(TARGET SDL3::SDL3)
    target_link_libraries(mblink_dom PUBLIC SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(mblink_dom PUBLIC SDL3-static)
endif()

# 链接Skia（如果启用）- 需要SkRect.h
if(TARGET skia)
    target_link_libraries(mblink_dom PUBLIC skia)
endif()

# 匹配 Skia 的运行时库设置
if(MSVC)
    target_compile_options(mblink_dom PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(mblink_dom PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()
