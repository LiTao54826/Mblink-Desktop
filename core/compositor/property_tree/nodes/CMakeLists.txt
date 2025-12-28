# property_tree/nodes 子模块
# 属性树节点类型定义

set(PROPERTY_TREE_NODES_SOURCES
    transform_tree_node.cpp
    clip_tree_node.cpp
    effect_tree_node.cpp
    scroll_tree_node.cpp
)

set(PROPERTY_TREE_NODES_HEADERS
    property_tree_node.h
    transform_tree_node.h
    clip_tree_node.h
    effect_tree_node.h
    scroll_tree_node.h
)

set(PROPERTY_TREE_NODES_SOURCES ${PROPERTY_TREE_NODES_SOURCES} PARENT_SCOPE)
set(PROPERTY_TREE_NODES_HEADERS ${PROPERTY_TREE_NODES_HEADERS} PARENT_SCOPE)
