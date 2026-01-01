# Render Module CMakeLists.txt

set(RENDER_SOURCES
    # Core rendering (pipeline subdirectory)
    pipeline/renderer.cpp
    pipeline/render_context.cpp
    pipeline/render_pipeline.cpp
    pipeline/render_cache.cpp
    pipeline/render_tree_synchronizer.cpp
    # pipeline/unified_renderer.cpp  # TODO: Fix Skia API compatibility issues
    
    # Render objects (objects subdirectory)
    objects/render_object.cpp
    objects/render_table.cpp
    objects/render_text.cpp
    objects/render_inline.cpp
    objects/render_block.cpp
    objects/render_inline_block.cpp
    objects/render_svg.cpp
    objects/svg_path_parser.cpp
    objects/scrollbar_controller.cpp
    objects/list_marker.cpp
    objects/select_dropdown.cpp
    
    # Basic rendering primitives (utils subdirectory)
    utils/color.cpp
    utils/paint.cpp
    utils/shapes.cpp
    painters/box_renderer.cpp
    utils/shadow_renderer.cpp
    utils/gradient_renderer.cpp
    
    # Transform and clipping (utils subdirectory)
    utils/transform.cpp
    utils/clip_optimizer.cpp
    
    # Dirty region tracking (utils subdirectory)
    utils/dirty_region.cpp
    utils/dirty_region_collector.cpp
    
    # Layer management (subdirectory)
    layer/fbo_manager.cpp
    layer/paint_layer.cpp
    
    # Style resolution (css subdirectory)
    css/style_resolver.cpp
    
    # Performance monitoring (utils subdirectory)
    utils/performance_monitor.cpp
    
    # Text rendering (subdirectory)
    text/font_manager.cpp
    text/text_renderer.cpp
    text/text_transform.cpp
    
    # Image handling (subdirectory)
    image/image_loader.cpp
    image/image_cache.cpp
    image/image_renderer.cpp
    image/image_fit.cpp
    
    # Canvas (subdirectory)
    canvas/canvas_rendering_context_2d.cpp
    canvas/canvas_gradient.cpp
    canvas/canvas_pattern.cpp
    canvas/canvas_image_data.cpp
    
    # Animation (subdirectory)
    animation/animation.cpp
    animation/animation_controller.cpp
    animation/animation_applicator.cpp
    animation/animation_optimizer.cpp
    animation/animation_timeline.cpp
    animation/keyframes.cpp
    animation/easing_functions.cpp
    animation/property_interpolation.cpp
    animation/transition.cpp
    
    # CSS (subdirectory)
    css/css_value.cpp
    css/css_variables.cpp
    css/css_filters.cpp
    css/css_clip_path.cpp
    
    # Filter cache (utils subdirectory)
    utils/filter_cache.cpp
    
    # Painters (subdirectory)
    painters/background_painter.cpp
    painters/border_painter.cpp
    painters/scrollbar_painter.cpp
    painters/form_element_painter.cpp
)

set(RENDER_HEADERS
    # Core rendering (pipeline subdirectory)
    pipeline/renderer.h
    pipeline/render_context.h
    pipeline/render_pipeline.h
    pipeline/render_cache.h
    pipeline/render_tree_synchronizer.h
    # pipeline/unified_renderer.h  # TODO: Fix Skia API compatibility issues
    
    # Render objects (objects subdirectory)
    objects/render_object.h
    objects/render_inline_block.h
    objects/render_svg.h
    objects/svg_path_parser.h
    objects/scrollbar_controller.h
    objects/list_marker.h
    objects/select_dropdown.h
    
    # Basic rendering primitives (utils subdirectory)
    utils/color.h
    utils/paint.h
    utils/shapes.h
    box_renderer.h
    utils/shadow_renderer.h
    utils/gradient_renderer.h
    
    # Transform and clipping (utils subdirectory)
    utils/transform.h
    utils/clip_optimizer.h
    
    # Dirty region tracking (utils subdirectory)
    utils/dirty_region.h
    utils/dirty_region_collector.h
    
    # Layer management
    layer/fbo_manager.h
    layer/paint_layer.h
    
    # Style resolution
    style_resolver.h
    
    # Performance monitoring (utils subdirectory)
    utils/performance_monitor.h
    
    # Utilities (utils subdirectory)
    utils/object_pool.h
    
    # Text rendering (subdirectory)
    text/font_manager.h
    text_renderer.h
    text_transform.h
    
    # Image handling (subdirectory)
    image/image_loader.h
    image/image_cache.h
    image/image_renderer.h
    image/image_fit.h
    
    # Canvas (subdirectory)
    canvas/canvas_rendering_context_2d.h
    
    # Animation (subdirectory)
    animation/animation.h
    animation/animation_controller.h
    animation/animation_applicator.h
    animation/animation_optimizer.h
    animation/animation_timeline.h
    animation/keyframes.h
    animation/easing_functions.h
    animation/property_interpolation.h
    animation/transition.h
    
    # CSS (subdirectory)
    css/css_value.h
    css/css_variables.h
    css/css_filters.h
    css/css_clip_path.h
    
    # Filter cache (utils subdirectory)
    utils/filter_cache.h
    
    # Painters (subdirectory)
    painters/background_painter.h
    painters/border_painter.h
    painters/scrollbar_painter.h
    painters/form_element_painter.h
)

add_library(lightui_render STATIC ${RENDER_SOURCES})

target_include_directories(lightui_render PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/core/render
    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/third_party/skia/Debug>
    $<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/third_party/skia/Release>
)

target_link_libraries(lightui_render PUBLIC
    lightui_utils
    lightui_dom
    lightui_quickjs
)

# Link Yoga (if available)
if(TARGET yogacore)
    target_link_libraries(lightui_render PUBLIC yogacore)
    message(STATUS "Render module: Linking Yoga")
else()
    message(WARNING "Render module: Yoga target not found")
endif()

# Link Skia (if enabled)
if(TARGET skia)
    target_link_libraries(lightui_render PUBLIC skia)
    message(STATUS "Render module: Linking Skia")

    # Skia prebuilt library uses static Release runtime (MT)
    # We need to match this setting
    if(MSVC)
        # Use static Release runtime to match Skia
        target_compile_options(lightui_render PUBLIC
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        # Disable iterator debugging to match Skia
        target_compile_definitions(lightui_render PUBLIC
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )
    endif()
else()
    message(WARNING "Render module: Skia target not found")
endif()
