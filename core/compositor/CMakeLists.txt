# Compositor 模块 CMakeLists.txt
# 分层合成架构 - CPU 光栅化 + GPU 合成

set(COMPOSITOR_SOURCES
    compositor_layer.cpp
    layer_tree_builder.cpp
    rasterizer.cpp
    compositor.cpp
    animation_layer_bridge.cpp
    scroll_layer_manager.cpp
    render_pipeline_v2.cpp
    window_compositor_adapter.cpp
)

set(COMPOSITOR_HEADERS
    compositor_layer.h
    layer_tree_builder.h
    rasterizer.h
    compositor.h
    animation_layer_bridge.h
    scroll_layer_manager.h
    render_pipeline_v2.h
    window_compositor_adapter.h
)

add_library(lightui_compositor STATIC ${COMPOSITOR_SOURCES})

target_include_directories(lightui_compositor PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/skia
)

target_link_libraries(lightui_compositor PUBLIC
    lightui_utils
    lightui_render
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
