# Layout模块 CMakeLists.txt

set(LAYOUT_SOURCES
    native_layout_engine.cpp
    content_version.cpp
    layout_boundary_detector.cpp
    incremental_layout_manager.cpp
    # 布局算法
    block_layout.cpp
    flex_layout.cpp
    grid/grid.cpp
    # IFC (Inline Formatting Context) 模块
    ifc/ifc_layout.cpp
    ifc/text_run.cpp
    ifc/inline_box.cpp
    ifc/line_box.cpp
    ifc/line_breaker.cpp
    ifc/vertical_aligner.cpp
)

set(LAYOUT_HEADERS
    layout_engine.h
    native_layout_engine.h
    content_version.h
    layout_boundary_detector.h
    incremental_layout_manager.h
    # 布局算法
    block_layout.h
    flex_layout.h
    grid/grid.h
    grid/types.h
    # IFC (Inline Formatting Context) 模块
    ifc/ifc_layout.h
    ifc/ifc.h
    ifc/text_run.h
    ifc/inline_box.h
    ifc/line_box.h
    ifc/line_breaker.h
    ifc/vertical_aligner.h
    # 类型定义
    types/geometry.h
    types/style.h
    types/layout.h
    types/cache.h
    types/traits.h
    types/mod.h
    # 工具函数
    util/math.h
    util/resolve.h
)

add_library(mbink_layout STATIC ${LAYOUT_SOURCES})

target_include_directories(mbink_layout PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(mbink_layout PUBLIC
    mbink_utils
    mbink_render
)

# 原生布局引擎 (Native Layout Engine)
# 已完全替换 Taffy FFI，Taffy 源码移至 ReferenceProject/taffy 作为参考

# 链接Skia（如果启用）
if(TARGET skia)
    target_link_libraries(mbink_layout PUBLIC skia)
    target_include_directories(mbink_layout PUBLIC
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/third_party/skia/Debug>
        $<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/third_party/skia/Release>
    )

    # Match Skia's runtime library settings (MSVC)
    if(MSVC)
        target_compile_options(mbink_layout PUBLIC
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(mbink_layout PUBLIC
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )
    endif()
endif()
