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
    render/test_layer_system_properties.cpp
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

# Create property tests executable
add_executable(lightui_property_tests
    main.cpp
    ${RENDER_PROPERTY_TEST_SOURCES}
    ${LAYOUT_PROPERTY_TEST_SOURCES}
    ${BUNDLER_PROPERTY_TEST_SOURCES}
)

# Add include directories
target_include_directories(lightui_property_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/tools/app_bundler
)

target_link_libraries(lightui_property_tests
    PRIVATE
    lightui
    test_utils
    GTest::gtest
)

# Optionally link RapidCheck if available
if(LIGHTUI_USE_RAPIDCHECK)
    target_link_libraries(lightui_property_tests PRIVATE rapidcheck rapidcheck_gtest)
    target_compile_definitions(lightui_property_tests PRIVATE USE_RAPIDCHECK)
endif()

# Add to CTest
include(GoogleTest)
gtest_discover_tests(lightui_property_tests)
