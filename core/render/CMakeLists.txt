# Render Module CMakeLists.txt

set(RENDER_SOURCES
    # Core rendering
    renderer.cpp
    render_context.cpp
    render_object.cpp
    render_pipeline.cpp
    render_cache.cpp
    render_inline_block.cpp
    render_tree_synchronizer.cpp
    # unified_renderer.cpp  # TODO: Fix Skia API compatibility issues
    
    # Basic rendering primitives
    color.cpp
    paint.cpp
    shapes.cpp
    box_renderer.cpp
    shadow_renderer.cpp
    gradient_renderer.cpp
    list_marker.cpp
    
    # Transform and clipping
    transform.cpp
    clip_optimizer.cpp
    
    # Dirty region tracking
    dirty_region.cpp
    dirty_region_collector.cpp
    
    # Layer management
    layer.cpp
    layer_manager.cpp
    fbo_manager.cpp
    
    # Style resolution
    style_resolver.cpp
    
    # Performance monitoring
    performance_monitor.cpp
    
    # SVG support
    svg_path_parser.cpp
    render_svg.cpp
    
    # UI components
    select_dropdown.cpp
    
    # Text rendering (subdirectory)
    text/font_manager.cpp
    text_renderer.cpp
    text_transform.cpp
    
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
    
    # Filter cache
    filter_cache.cpp
)

set(RENDER_HEADERS
    # Core rendering
    renderer.h
    render_context.h
    render_object.h
    render_pipeline.h
    render_cache.h
    render_inline_block.h
    render_tree_synchronizer.h
    # unified_renderer.h  # TODO: Fix Skia API compatibility issues
    
    # Basic rendering primitives
    color.h
    paint.h
    shapes.h
    box_renderer.h
    shadow_renderer.h
    gradient_renderer.h
    list_marker.h
    
    # Transform and clipping
    transform.h
    clip_optimizer.h
    
    # Dirty region tracking
    dirty_region.h
    dirty_region_collector.h
    
    # Layer management
    layer.h
    layer_manager.h
    fbo_manager.h
    
    # Style resolution
    style_resolver.h
    
    # Performance monitoring
    performance_monitor.h
    
    # SVG support
    svg_path_parser.h
    render_svg.h
    
    # UI components
    select_dropdown.h
    
    # Utilities
    object_pool.h
    
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
    
    # Filter cache
    filter_cache.h
)

add_library(lightui_render STATIC ${RENDER_SOURCES})

target_include_directories(lightui_render PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/core/render
    ${CMAKE_SOURCE_DIR}/third_party/skia
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
