# 渲染测试 CMake 配置

set(RENDER_TEST_SOURCES
    main.cpp
    test_animation.cpp
    test_gradient_renderer.cpp
    test_shadow_renderer.cpp
    test_text_renderer.cpp
)

add_executable(lightui_render_tests
    ${RENDER_TEST_SOURCES}
)

target_link_libraries(lightui_render_tests
    PRIVATE
    lightui
    test_utils
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(lightui_render_tests)
