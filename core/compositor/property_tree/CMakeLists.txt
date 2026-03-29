# Property Tree 模块 CMakeLists.txt
# 属性树系统 - 参考 Chromium Blink 设计

# 添加子模块
add_subdirectory(nodes)
add_subdirectory(paint)

set(PROPERTY_TREE_SOURCES
    # nodes/ 子目录
    nodes/transform_tree_node.cpp
    nodes/clip_tree_node.cpp
    nodes/effect_tree_node.cpp
    nodes/scroll_tree_node.cpp
    # paint/ 子目录
    paint/display_item.cpp
    paint/paint_chunk.cpp
    paint/paint_artifact.cpp
    paint/pending_layer.cpp
    paint/paint_artifact_compositor.cpp
    # 根目录
    property_tree_state.cpp
    property_trees.cpp
    property_tree_builder.cpp
    geometry_mapper.cpp
    layerizer.cpp
    raster_invalidator.cpp
    property_tree_error.cpp
)

set(PROPERTY_TREE_HEADERS
    # nodes/ 子目录
    nodes/property_tree_node.h
    nodes/transform_tree_node.h
    nodes/clip_tree_node.h
    nodes/effect_tree_node.h
    nodes/scroll_tree_node.h
    # paint/ 子目录
    paint/display_item.h
    paint/paint_chunk.h
    paint/paint_artifact.h
    paint/pending_layer.h
    paint/paint_artifact_compositor.h
    # 根目录
    property_tree_state.h
    property_trees.h
    property_tree_builder.h
    geometry_mapper.h
    compositing_reasons.h
    layerizer.h
    raster_invalidator.h
    property_tree_error.h
)

add_library(mbink_property_tree STATIC ${PROPERTY_TREE_SOURCES})

target_include_directories(mbink_property_tree PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/third_party/skia/Debug>
    $<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/third_party/skia/Release>
)

target_link_libraries(mbink_property_tree PUBLIC
    mbink_render
)

# 链接 Skia
if(TARGET skia)
    target_link_libraries(mbink_property_tree PUBLIC skia)
endif()

# MSVC 运行时设置（匹配 Skia）
if(MSVC)
    target_compile_options(mbink_property_tree PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(mbink_property_tree PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()
