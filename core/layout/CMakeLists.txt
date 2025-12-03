# Layout模块 CMakeLists.txt

set(LAYOUT_SOURCES
    layout_engine.cpp
    # IFC (Inline Formatting Context) 模块
    text_run.cpp
    inline_box.cpp
    line_box.cpp
    inline_formatting_context.cpp
    line_breaker.cpp
    vertical_aligner.cpp
    ifc_layout.cpp
)

set(LAYOUT_HEADERS
    layout_engine.h
    # IFC (Inline Formatting Context) 模块
    text_run.h
    inline_box.h
    line_box.h
    inline_formatting_context.h
    line_breaker.h
    vertical_aligner.h
    ifc_layout.h
)

add_library(lightui_layout STATIC ${LAYOUT_SOURCES})

target_include_directories(lightui_layout PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(lightui_layout PUBLIC
    lightui_utils
)

# 链接 Taffy 布局引擎
if(TARGET taffy)
    target_link_libraries(lightui_layout PUBLIC taffy)
    message(STATUS "Linking Taffy layout engine")
else()
    message(WARNING "Taffy not found, layout module may not build correctly")
endif()

# 链接Skia（如果启用）
if(TARGET skia)
    target_link_libraries(lightui_layout PUBLIC skia)
    target_include_directories(lightui_layout PUBLIC ${CMAKE_SOURCE_DIR}/third_party/skia)

    # Match Skia's runtime library settings (MSVC)
    if(MSVC)
        target_compile_options(lightui_layout PUBLIC
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(lightui_layout PUBLIC
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )
    endif()
endif()
