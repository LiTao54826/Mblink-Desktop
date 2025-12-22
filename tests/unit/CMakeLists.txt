# 单元测试 CMake 配置

# DOM 测试
set(DOM_TEST_SOURCES
    dom/test_node.cpp
    dom/test_element.cpp
    dom/test_document.cpp
    dom/test_text.cpp
    dom/test_selector_engine.cpp
    dom/test_dom_token_list.cpp
    dom/test_css_style_declaration.cpp
)

# 事件测试
set(EVENT_TEST_SOURCES
    event/test_event.cpp
    event/test_event_loop.cpp
    event/test_task_scheduler.cpp
    event/test_input_handler.cpp
)

# 布局测试
set(LAYOUT_TEST_SOURCES
    layout/test_native_layout_engine.cpp
    layout/test_flex_layout.cpp
    layout/test_block_layout.cpp
    layout/test_grid_layout.cpp
)

# 渲染测试
set(RENDER_TEST_SOURCES
    render/test_css_value.cpp
    render/test_css_variables.cpp
    render/test_color.cpp
    render/test_transform.cpp
)

# QuickJS 测试
set(QUICKJS_TEST_SOURCES
    quickjs/test_quickjs_runtime.cpp
    quickjs/test_dom_bindings.cpp
)

# Lexbor 测试
set(LEXBOR_TEST_SOURCES
    lexbor/test_lexbor_document.cpp
    lexbor/test_style_manager.cpp
)

# 工具测试
set(UTILS_TEST_SOURCES
    utils/test_json.cpp
    utils/test_encoding.cpp
)

# 创建单元测试可执行文件
add_executable(lightui_unit_tests
    main.cpp
    ${DOM_TEST_SOURCES}
    ${EVENT_TEST_SOURCES}
    ${LAYOUT_TEST_SOURCES}
    ${RENDER_TEST_SOURCES}
    ${QUICKJS_TEST_SOURCES}
    ${LEXBOR_TEST_SOURCES}
    ${UTILS_TEST_SOURCES}
)

target_link_libraries(lightui_unit_tests
    PRIVATE
    lightui
    test_utils
    GTest::gtest
    GTest::gtest_main
)

# 添加到 CTest
include(GoogleTest)
gtest_discover_tests(lightui_unit_tests)
