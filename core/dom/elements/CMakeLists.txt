# HTML Elements Module

set(ELEMENTS_SOURCES
    html_anchor_element.cpp
    html_button_element.cpp
    html_canvas_element.cpp
    html_div_element.cpp
    html_form_controls.cpp
    html_form_element.cpp
    html_head_element.cpp
    html_heading_element.cpp
    html_image_element.cpp
    html_input_element.cpp
    html_label_element.cpp
    html_li_element.cpp
    html_link_element.cpp
    html_olist_element.cpp
    html_option_element.cpp
    html_paragraph_element.cpp
    html_script_element.cpp
    html_select_element.cpp
    html_span_element.cpp
    html_style_element.cpp
    html_table_element.cpp
    html_template_element.cpp
    html_textarea_element.cpp
    html_ulist_element.cpp
    svg_element.cpp
)

set(ELEMENTS_HEADERS
    html_anchor_element.h
    html_button_element.h
    html_canvas_element.h
    html_div_element.h
    html_form_controls.h
    html_form_element.h
    html_head_element.h
    html_heading_element.h
    html_image_element.h
    html_input_element.h
    html_label_element.h
    html_li_element.h
    html_link_element.h
    html_olist_element.h
    html_option_element.h
    html_paragraph_element.h
    html_script_element.h
    html_select_element.h
    html_span_element.h
    html_style_element.h
    html_table_element.h
    html_template_element.h
    html_textarea_element.h
    html_ulist_element.h
    svg_element.h
)

# 将源文件列表导出给父目录
set(ELEMENTS_SOURCES ${ELEMENTS_SOURCES} PARENT_SCOPE)
set(ELEMENTS_HEADERS ${ELEMENTS_HEADERS} PARENT_SCOPE)

# 虚拟文本组件子目录
add_subdirectory(virtual_text)
add_subdirectory(terminal)
add_subdirectory(logview)
