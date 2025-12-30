# logview 日志视图元素
# 提供高性能日志显示、过滤和搜索

set(LOGVIEW_SOURCES
    html_logview_element.cpp
    log_buffer.cpp
    log_renderer.cpp
    log_filter.cpp
    log_search.cpp
)

set(LOGVIEW_HEADERS
    html_logview_element.h
    log_entry.h
    log_buffer.h
    log_renderer.h
    log_filter.h
    log_search.h
)

add_library(logview_element STATIC
    ${LOGVIEW_SOURCES}
    ${LOGVIEW_HEADERS}
)

target_include_directories(logview_element PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(logview_element PUBLIC
    virtual_text
    skia
    SDL3-static
)
