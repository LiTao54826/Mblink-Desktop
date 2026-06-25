# 性能测试 CMake 配置

set(PERFORMANCE_TEST_SOURCES
    main.cpp
    test_dom_performance.cpp
    test_layout_performance.cpp
    test_incremental_layout_performance.cpp
    test_render_performance.cpp
    test_property_tree_performance.cpp
)

add_executable(mblink_performance_tests
    ${PERFORMANCE_TEST_SOURCES}
)

target_link_libraries(mblink_performance_tests
    PRIVATE
    mblink
    test_utils
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(mblink_performance_tests)
