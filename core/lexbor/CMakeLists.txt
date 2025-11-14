# Lexbor 包装层 CMakeLists.txt

set(LEXBOR_WRAPPER_SOURCES
    lexbor_document.cpp
    lexbor_stylesheet.cpp
    style_manager.cpp
    cascade_engine.cpp
    style_cache.cpp
)

set(LEXBOR_WRAPPER_HEADERS
    lexbor_document.h
    lexbor_stylesheet.h
    style_manager.h
    cascade_engine.h
    style_cache.h
)

# 创建 Lexbor 包装库
add_library(lightui_lexbor STATIC
    ${LEXBOR_WRAPPER_SOURCES}
    ${LEXBOR_WRAPPER_HEADERS}
)

# 设置包含目录
target_include_directories(lightui_lexbor PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${THIRD_PARTY_DIR}/lexbor/source
)

# 链接 Lexbor 静态库
target_link_libraries(lightui_lexbor PUBLIC
    lexbor_static
)

# 链接Skia（如果启用）- 需要SkRect.h
if(TARGET skia)
    target_link_libraries(lightui_lexbor PUBLIC skia)
endif()

# 设置编译选项
target_compile_features(lightui_lexbor PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(lightui_lexbor PRIVATE /W4)
else()
    target_compile_options(lightui_lexbor PRIVATE -Wall -Wextra)
endif()

# 安装
install(TARGETS lightui_lexbor
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${LEXBOR_WRAPPER_HEADERS}
    DESTINATION include/lightui/lexbor
)

