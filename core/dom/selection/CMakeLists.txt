# DOM Selection 子模块 CMakeLists.txt

set(DOM_SELECTION_SOURCES
    selection.cpp
    range.cpp
    selector_engine.cpp
)

set(DOM_SELECTION_HEADERS
    selection.h
    range.h
    selector_engine.h
)

# 将源文件列表传递给父目录
set(DOM_SELECTION_SOURCES ${DOM_SELECTION_SOURCES} PARENT_SCOPE)
set(DOM_SELECTION_HEADERS ${DOM_SELECTION_HEADERS} PARENT_SCOPE)
