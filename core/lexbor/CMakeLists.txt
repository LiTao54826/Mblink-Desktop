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
add_library(mbink_lexbor STATIC
    ${LEXBOR_WRAPPER_SOURCES}
    ${LEXBOR_WRAPPER_HEADERS}
)

# 设置包含目录
target_include_directories(mbink_lexbor PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${THIRD_PARTY_DIR}/lexbor/source
)

# 链接 Lexbor 静态库
target_link_libraries(mbink_lexbor PUBLIC
    lexbor_static
)

# 链接Skia（如果启用）- 需要SkRect.h
if(TARGET skia)
    target_link_libraries(mbink_lexbor PUBLIC skia)
endif()

# 设置编译选项
target_compile_features(mbink_lexbor PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(mbink_lexbor PRIVATE /W4)
else()
    target_compile_options(mbink_lexbor PRIVATE -Wall -Wextra)
endif()

# 安装
install(TARGETS mbink_lexbor
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${LEXBOR_WRAPPER_HEADERS}
    DESTINATION include/mbink/lexbor
)

