# 集成测试 CMake 配置

set(INTEGRATION_TEST_SOURCES
    main.cpp
    test_dom_integration.cpp
    test_render_pipeline.cpp
    test_event_dispatch.cpp
    test_html_loading.cpp
    test_javascript_integration.cpp
    test_incremental_layout_integration.cpp
    test_incremental_update_system.cpp
    test_terminal_logview_integration.cpp
)

add_executable(mblink_integration_tests
    ${INTEGRATION_TEST_SOURCES}
)

target_link_libraries(mblink_integration_tests
    PRIVATE
    mblink
    test_utils
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(mblink_integration_tests)
