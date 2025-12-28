# DOM Style 子模块 CMakeLists.txt

set(DOM_STYLE_SOURCES
    css_style_declaration.cpp
    incremental_style_recalc.cpp
)

set(DOM_STYLE_HEADERS
    css_style_declaration.h
    incremental_style_recalc.h
)

# 将源文件列表传递给父目录
set(DOM_STYLE_SOURCES ${DOM_STYLE_SOURCES} PARENT_SCOPE)
set(DOM_STYLE_HEADERS ${DOM_STYLE_HEADERS} PARENT_SCOPE)
