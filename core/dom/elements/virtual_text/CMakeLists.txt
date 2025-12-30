# virtual_text 共享核心层
# 提供虚拟滚动、环形缓冲区、文本选择等基础设施

set(VIRTUAL_TEXT_SOURCES
    virtual_scroll_renderer.cpp
    selection_manager.cpp
)

set(VIRTUAL_TEXT_HEADERS
    virtual_buffer.h
    virtual_scroll_renderer.h
    selection_manager.h
    text_style.h
)

add_library(virtual_text STATIC
    ${VIRTUAL_TEXT_SOURCES}
    ${VIRTUAL_TEXT_HEADERS}
)

target_include_directories(virtual_text PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(virtual_text PUBLIC
    skia
)
