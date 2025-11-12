# Render模块 CMakeLists.txt

set(RENDER_SOURCES
    renderer.cpp
    render_context.cpp
    color.cpp
    paint.cpp
    shapes.cpp
    text_renderer.cpp
    text/font_manager.cpp
    image/image_loader.cpp
    image/image_cache.cpp
    image/image_renderer.cpp
    css_value.cpp
    box_renderer.cpp
    render_object.cpp
    render_inline_block.cpp
    style_resolver.cpp
    dirty_region.cpp
    performance_monitor.cpp
    layer.cpp
    render_cache.cpp
    clip_optimizer.cpp
)

set(RENDER_HEADERS
    renderer.h
    render_context.h
    color.h
    paint.h
    shapes.h
    text_renderer.h
    text/font_manager.h
    image/image_loader.h
    image/image_cache.h
    image/image_renderer.h
    css_value.h
    box_renderer.h
    render_object.h
    render_inline_block.h
    style_resolver.h
    dirty_region.h
    performance_monitor.h
    layer.h
    render_cache.h
    clip_optimizer.h
)

add_library(lightui_render STATIC ${RENDER_SOURCES})

target_include_directories(lightui_render PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/skia
)

target_link_libraries(lightui_render PUBLIC
    lightui_utils
    lightui_dom
    lightui_quickjs
)

# 链接Skia（如果启用）
if(TARGET skia)
    target_link_libraries(lightui_render PUBLIC skia)
    message(STATUS "Render module: Linking Skia")

    # Skia 预编译库使用静态 Release 运行时 (MT)
    # 我们需要匹配这个设置
    if(MSVC)
        # 使用静态 Release 运行时以匹配 Skia
        target_compile_options(lightui_render PUBLIC
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        # 禁用迭代器调试以匹配 Skia
        target_compile_definitions(lightui_render PUBLIC
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )
    endif()
else()
    message(WARNING "Render module: Skia target not found")
endif()
