# 集成测试 CMake 配置

set(INTEGRATION_TEST_SOURCES
    main.cpp
    test_dom_integration.cpp
    test_render_pipeline.cpp
    test_event_dispatch.cpp
    test_html_loading.cpp
    test_javascript_integration.cpp
    test_incremental_layout_integration.cpp
)

add_executable(lightui_integration_tests
    ${INTEGRATION_TEST_SOURCES}
)

target_link_libraries(lightui_integration_tests
    PRIVATE
    lightui
    test_utils
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(lightui_integration_tests)
