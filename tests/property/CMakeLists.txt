# Property-Based Tests CMake Configuration
# Uses manual property-based testing patterns with Google Test

# Render property tests
set(RENDER_PROPERTY_TEST_SOURCES
    render/test_outline_properties.cpp
    render/test_text_transform_properties.cpp
    render/test_pointer_events_properties.cpp
    render/test_user_select_properties.cpp
    render/test_word_break_properties.cpp
    render/test_object_fit_properties.cpp
    render/test_list_style_properties.cpp
    render/test_aspect_ratio_properties.cpp
    render/test_clip_path_properties.cpp
    render/test_paint_layer_properties.cpp
    render/test_animation_keyframes_properties.cpp
    render/test_animation_parsing_properties.cpp
    render/test_animation_interpolation_properties.cpp
    render/test_animation_progress_properties.cpp
    render/test_transform_interpolation_properties.cpp
    render/test_animation_events_properties.cpp
    render/test_animation_fillmode_properties.cpp
    render/test_animation_playstate_properties.cpp
)

# Layout property tests
set(LAYOUT_PROPERTY_TEST_SOURCES
    layout/test_content_version_properties.cpp
    layout/test_cache_version_properties.cpp
    layout/test_version_update_properties.cpp
    layout/test_layout_scope_properties.cpp
    layout/test_incremental_layout_properties.cpp
    layout/test_layout_equivalence_properties.cpp
    layout/test_ifc_cache_properties.cpp
    layout/test_viewport_units_properties.cpp
)

# Bundler property tests
set(BUNDLER_PROPERTY_TEST_SOURCES
    bundler/test_payload_properties.cpp
    bundler/test_module_resolver_properties.cpp
    bundler/test_bytecode_compiler_properties.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/payload.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/module_resolver.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.cpp
)

# Compositor property tests
set(COMPOSITOR_PROPERTY_TEST_SOURCES
    compositor/test_dirty_region_properties.cpp
    compositor/test_layer_promotion_properties.cpp
    compositor/test_rasterizer_properties.cpp
    compositor/test_compositor_properties.cpp
    compositor/test_animation_layer_properties.cpp
    compositor/test_scroll_layer_properties.cpp
    compositor/test_render_pipeline_properties.cpp
    compositor/test_visual_consistency_properties.cpp
    compositor/test_animation_bounds_properties.cpp
)

# Virtual text property tests
set(VIRTUAL_TEXT_PROPERTY_TEST_SOURCES
    virtual_text/test_virtual_buffer_properties.cpp
    virtual_text/test_virtual_scroll_properties.cpp
    virtual_text/test_selection_properties.cpp
    virtual_text/test_ansi_parser_properties.cpp
    virtual_text/test_terminal_buffer_properties.cpp
    virtual_text/test_logview_properties.cpp
)

# Create property tests executable
add_executable(mblink_property_tests
    main.cpp
    ${RENDER_PROPERTY_TEST_SOURCES}
    ${LAYOUT_PROPERTY_TEST_SOURCES}
    ${BUNDLER_PROPERTY_TEST_SOURCES}
    ${COMPOSITOR_PROPERTY_TEST_SOURCES}
    ${VIRTUAL_TEXT_PROPERTY_TEST_SOURCES}
)

# Add include directories
target_include_directories(mblink_property_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/tools/app_bundler
)

target_link_libraries(mblink_property_tests
    PRIVATE
    mblink
    test_utils
    GTest::gtest
)

# Optionally link RapidCheck if available
if(MBLINK_USE_RAPIDCHECK)
    target_link_libraries(mblink_property_tests PRIVATE rapidcheck rapidcheck_gtest)
    target_compile_definitions(mblink_property_tests PRIVATE USE_RAPIDCHECK)
endif()

# Add to CTest
include(GoogleTest)
gtest_discover_tests(mblink_property_tests)
