# Layout模块 CMakeLists.txt

set(LAYOUT_SOURCES
    native_layout_engine.cpp
    # IFC (Inline Formatting Context) 模块
    text_run.cpp
    inline_box.cpp
    line_box.cpp
    inline_formatting_context.cpp
    line_breaker.cpp
    vertical_aligner.cpp
    ifc_layout.cpp
    # Taffy 翻译模块 (原生布局引擎)
    taffy/compute/block.cpp
    taffy/compute/flexbox.cpp
    taffy/compute/grid/grid.cpp
)

set(LAYOUT_HEADERS
    layout_engine.h
    native_layout_engine.h
    # IFC (Inline Formatting Context) 模块
    text_run.h
    inline_box.h
    line_box.h
    inline_formatting_context.h
    line_breaker.h
    vertical_aligner.h
    ifc_layout.h
    # Taffy 翻译模块 (原生布局引擎)
    taffy/geometry.h
    taffy/style.h
    taffy/layout.h
    taffy/cache.h
    taffy/util/math.h
    taffy/util/resolve.h
    taffy/tree/traits.h
    taffy/compute/block.h
    taffy/compute/flexbox.h
    taffy/compute/grid/types.h
    taffy/compute/grid/grid.h
    taffy/compute/ifc.h
    taffy/compute/mod.h
)

add_library(lightui_layout STATIC ${LAYOUT_SOURCES})

target_include_directories(lightui_layout PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(lightui_layout PUBLIC
    lightui_utils
    lightui_render
)

# 原生布局引擎已替换 Taffy FFI，不再需要链接 Taffy
# Taffy 源码仅作为翻译参考保留在 third_party/taffy

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
