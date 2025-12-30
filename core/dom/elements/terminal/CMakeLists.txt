# terminal 终端元素
# 提供 ANSI 终端模拟、命令执行和交互式 Shell

set(TERMINAL_SOURCES
    html_terminal_element.cpp
    ansi_parser.cpp
    terminal_buffer.cpp
    terminal_renderer.cpp
    command_executor.cpp
)

set(TERMINAL_HEADERS
    html_terminal_element.h
    ansi_parser.h
    terminal_buffer.h
    terminal_renderer.h
    pty_backend.h
    command_executor.h
)

# 平台特定 PTY 实现
if(WIN32)
    list(APPEND TERMINAL_SOURCES conpty_backend.cpp)
else()
    list(APPEND TERMINAL_SOURCES unix_pty_backend.cpp)
endif()

add_library(terminal_element STATIC
    ${TERMINAL_SOURCES}
    ${TERMINAL_HEADERS}
)

target_include_directories(terminal_element PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(terminal_element PUBLIC
    virtual_text
    skia
)

# Windows 需要链接额外的库
if(WIN32)
    target_link_libraries(terminal_element PRIVATE
        kernel32
    )
else()
    target_link_libraries(terminal_element PRIVATE
        util  # for forkpty
    )
endif()
