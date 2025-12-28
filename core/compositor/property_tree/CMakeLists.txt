# Property Tree 模块 CMakeLists.txt
# 属性树系统 - 参考 Chromium Blink 设计

set(PROPERTY_TREE_SOURCES
    transform_tree_node.cpp
    clip_tree_node.cpp
    effect_tree_node.cpp
    scroll_tree_node.cpp
    property_tree_state.cpp
    property_trees.cpp
    property_tree_builder.cpp
    geometry_mapper.cpp
    display_item.cpp
    paint_chunk.cpp
    paint_artifact.cpp
    pending_layer.cpp
    layerizer.cpp
    raster_invalidator.cpp
    paint_artifact_compositor.cpp
    property_tree_error.cpp
)

set(PROPERTY_TREE_HEADERS
    property_tree_node.h
    transform_tree_node.h
    clip_tree_node.h
    effect_tree_node.h
    scroll_tree_node.h
    property_tree_state.h
    property_trees.h
    property_tree_builder.h
    geometry_mapper.h
    display_item.h
    paint_chunk.h
    paint_artifact.h
    compositing_reasons.h
    pending_layer.h
    layerizer.h
    raster_invalidator.h
    paint_artifact_compositor.h
    property_tree_error.h
)

add_library(lightui_property_tree STATIC ${PROPERTY_TREE_SOURCES})

target_include_directories(lightui_property_tree PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/third_party/skia/Debug>
    $<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/third_party/skia/Release>
)

target_link_libraries(lightui_property_tree PUBLIC
    lightui_render
)

# 链接 Skia
if(TARGET skia)
    target_link_libraries(lightui_property_tree PUBLIC skia)
endif()

# MSVC 运行时设置（匹配 Skia）
if(MSVC)
    target_compile_options(lightui_property_tree PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(lightui_property_tree PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()
