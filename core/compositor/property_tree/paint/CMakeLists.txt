# property_tree/paint 子模块
# 绘制系统相关类型

set(PROPERTY_TREE_PAINT_SOURCES
    display_item.cpp
    paint_chunk.cpp
    paint_artifact.cpp
    paint_artifact_compositor.cpp
    pending_layer.cpp
)

set(PROPERTY_TREE_PAINT_HEADERS
    display_item.h
    paint_chunk.h
    paint_artifact.h
    paint_artifact_compositor.h
    pending_layer.h
)

set(PROPERTY_TREE_PAINT_SOURCES ${PROPERTY_TREE_PAINT_SOURCES} PARENT_SCOPE)
set(PROPERTY_TREE_PAINT_HEADERS ${PROPERTY_TREE_PAINT_HEADERS} PARENT_SCOPE)
