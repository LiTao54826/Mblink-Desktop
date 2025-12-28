# compositor/animation 子模块
# 动画与合成层系统的桥接

set(COMPOSITOR_ANIMATION_SOURCES
    animation_bounds_calculator.cpp
    animation_layer_bridge.cpp
)

set(COMPOSITOR_ANIMATION_HEADERS
    animation_bounds_calculator.h
    animation_layer_bridge.h
)

# 将源文件添加到父目录的源文件列表
set(COMPOSITOR_ANIMATION_SOURCES ${COMPOSITOR_ANIMATION_SOURCES} PARENT_SCOPE)
set(COMPOSITOR_ANIMATION_HEADERS ${COMPOSITOR_ANIMATION_HEADERS} PARENT_SCOPE)
