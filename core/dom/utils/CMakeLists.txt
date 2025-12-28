# DOM Utils 子模块 CMakeLists.txt

set(DOM_UTILS_SOURCES
    dom_token_list.cpp
    dom_string_map.cpp
)

set(DOM_UTILS_HEADERS
    dom_token_list.h
    dom_string_map.h
)

# 将源文件列表传递给父目录
set(DOM_UTILS_SOURCES ${DOM_UTILS_SOURCES} PARENT_SCOPE)
set(DOM_UTILS_HEADERS ${DOM_UTILS_HEADERS} PARENT_SCOPE)
