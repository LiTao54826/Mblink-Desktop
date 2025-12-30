# DOM Bindings 子模块 CMakeLists.txt

set(DOM_BINDINGS_SOURCES
    dom_bindings.cpp
    canvas_bindings.cpp
    terminal_bindings.cpp
)

set(DOM_BINDINGS_HEADERS
    dom_bindings.h
    canvas_bindings.h
    terminal_bindings.h
)

# 将源文件列表传递给父目录
set(DOM_BINDINGS_SOURCES ${DOM_BINDINGS_SOURCES} PARENT_SCOPE)
set(DOM_BINDINGS_HEADERS ${DOM_BINDINGS_HEADERS} PARENT_SCOPE)
