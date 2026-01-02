# Compositor 模块 CMakeLists.txt
# 分层合成架构 - CPU 光栅化 + GPU 合成

# 添加子模块
add_subdirectory(property_tree)
add_subdirectory(animation)

set(COMPOSITOR_SOURCES
    compositor_layer.cpp
    layer_tree_builder.cpp
    layer_tree_manager.cpp
    rasterizer.cpp
    compositor.cpp
    scroll_layer_manager.cpp
    animation/animation_layer_bridge.cpp
    animation/animation_bounds_calculator.cpp
)

set(COMPOSITOR_HEADERS
    compositor_layer.h
    layer_tree_builder.h
    layer_tree_manager.h
    rasterizer.h
    compositor.h
    scroll_layer_manager.h
    animation/animation_layer_bridge.h
    animation/animation_bounds_calculator.h
)

add_library(lightui_compositor STATIC ${COMPOSITOR_SOURCES})

target_include_directories(lightui_compositor PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/third_party/skia/Debug>
    $<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/third_party/skia/Release>
)

target_link_libraries(lightui_compositor PUBLIC
    lightui_utils
    lightui_render
    lightui_property_tree
)

# 链接 Skia
if(TARGET skia)
    target_link_libraries(lightui_compositor PUBLIC skia)
    message(STATUS "Compositor module: Linking Skia")
else()
    message(WARNING "Compositor module: Skia target not found")
endif()

# 链接 OpenGL
if(WIN32)
    target_link_libraries(lightui_compositor PUBLIC opengl32)
endif()

# MSVC 运行时设置（匹配 Skia）
if(MSVC)
    target_compile_options(lightui_compositor PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(lightui_compositor PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()
