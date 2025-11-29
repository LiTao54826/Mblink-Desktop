/**
 * @file test_preact_integration.cpp
 * @brief Preact集成测试
 *
 * 设计理念：遵循 JavaScript-first 架构
 * - C++ 层只提供标准 DOM API
 * - Preact 在 JavaScript 层运行
 * - 不使用 C++ PreactRenderer 类
 *
 * @see docs/PROJECT_STANDARDS.md - 规范5: JavaScript优先架构
 */

#include <gtest/gtest.h>
#include "core/quickjs/quickjs_runtime.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include <fstream>
#include <sstream>

using namespace lightui;

class PreactIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime = std::make_unique<QuickJSRuntime>();
    }

    void TearDown() override {
        runtime.reset();
    }

    std::string ReadFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void LoadPreactAsGlobal() {
        // 加载 Preact 为全局变量 (与示例应用一致的方式)
        std::string preact_code = ReadFile("js/preact/preact.js");
        ASSERT_FALSE(preact_code.empty()) << "Failed to read js/preact/preact.js";
        runtime->Eval(preact_code, "preact.js");

        // 加载 Hooks 为全局变量
        std::string hooks_code = ReadFile("js/preact/hooks.js");
        ASSERT_FALSE(hooks_code.empty()) << "Failed to read js/preact/hooks.js";
        runtime->Eval(hooks_code, "hooks.js");
    }

    void RegisterPreactModules() {
        // 注册 ES6 模块版本 (.mjs)
        std::string preact_code = ReadFile("js/preact/preact.mjs");
        ASSERT_FALSE(preact_code.empty()) << "Failed to read js/preact/preact.mjs";
        runtime->RegisterModule("preact", preact_code);

        std::string hooks_code = ReadFile("js/preact/hooks.mjs");
        ASSERT_FALSE(hooks_code.empty()) << "Failed to read js/preact/hooks.mjs";
        runtime->RegisterModule("preact/hooks", hooks_code);
    }

    std::unique_ptr<QuickJSRuntime> runtime;
};

// ========== 基础测试 ==========

TEST_F(PreactIntegrationTest, RuntimeCreation) {
    ASSERT_NE(runtime, nullptr);
    EXPECT_NE(runtime->GetContext(), nullptr);
}

TEST_F(PreactIntegrationTest, LoadPreactAsGlobalWorks) {
    // 测试全局变量方式加载 (与示例应用一致)
    LoadPreactAsGlobal();

    // 验证全局对象存在
    auto result = runtime->Eval("typeof Preact !== 'undefined'", "test.js");
    EXPECT_EQ(result, true);

    result = runtime->Eval("typeof PreactHooks !== 'undefined'", "test.js");
    EXPECT_EQ(result, true);
}

TEST_F(PreactIntegrationTest, ES6ModuleLoadWorks) {
    // 测试 ES6 模块方式加载
    RegisterPreactModules();

    // 加载模块不抛出异常即成功
    EXPECT_NO_THROW({
        runtime->LoadModule("preact");
    });

    EXPECT_NO_THROW({
        runtime->LoadModule("preact/hooks");
    });
}

// ========== VNode创建测试 (使用全局变量方式，与示例一致) ==========

TEST_F(PreactIntegrationTest, CreateSimpleVNode) {
    LoadPreactAsGlobal();

    std::string test_code = R"(
        var vnode = Preact.h('div', null, 'Hello World');
        vnode;
    )";

    auto result = runtime->Eval(test_code, "test.js");
    EXPECT_TRUE(result.contains("type"));
    EXPECT_EQ(result["type"], "div");
    EXPECT_TRUE(result.contains("children"));
}

TEST_F(PreactIntegrationTest, CreateVNodeWithProps) {
    LoadPreactAsGlobal();

    std::string test_code = R"(
        var vnode = Preact.h('div', { className: 'container', id: 'main' }, 'Content');
        vnode;
    )";

    auto result = runtime->Eval(test_code, "test.js");
    EXPECT_TRUE(result.contains("props"));
    EXPECT_EQ(result["props"]["className"], "container");
    EXPECT_EQ(result["props"]["id"], "main");
}

TEST_F(PreactIntegrationTest, CreateVNodeWithChildren) {
    LoadPreactAsGlobal();

    std::string test_code = R"(
        var vnode = Preact.h('div', null,
            Preact.h('h1', null, 'Title'),
            Preact.h('p', null, 'Paragraph')
        );
        vnode;
    )";

    auto result = runtime->Eval(test_code, "test.js");
    EXPECT_TRUE(result.contains("children"));
    EXPECT_EQ(result["children"].size(), 2);
}

// ========== 组件测试 ==========

TEST_F(PreactIntegrationTest, CreateFunctionComponent) {
    LoadPreactAsGlobal();

    std::string test_code = R"(
        function Greeting(props) {
            return Preact.h('div', null, 'Hello, ', props.name);
        }

        var vnode = Preact.h(Greeting, { name: 'World' });
        vnode;
    )";

    auto result = runtime->Eval(test_code, "test.js");
    EXPECT_TRUE(result.contains("type"));
    EXPECT_TRUE(result.contains("props"));
    EXPECT_EQ(result["props"]["name"], "World");
}

// ========== Hooks测试 ==========

TEST_F(PreactIntegrationTest, UseStateHook) {
    LoadPreactAsGlobal();

    std::string test_code = R"(
        function Counter() {
            var state = PreactHooks.useState(0);
            var count = state[0];
            return Preact.h('div', null, 'Count: ' + count);
        }

        var vnode = Preact.h(Counter);
        vnode;
    )";

    // 这个测试主要验证代码能够执行，不会抛出异常
    EXPECT_NO_THROW({
        auto result = runtime->Eval(test_code, "test.js");
    });
}

// ========== 性能测试 ==========

TEST_F(PreactIntegrationTest, PerformanceVNodeCreation) {
    LoadPreactAsGlobal();

    std::string test_code = R"(
        var start = Date.now();
        for (var i = 0; i < 1000; i++) {
            Preact.h('div', { key: i }, 'Item ' + i);
        }
        var end = Date.now();
        end - start;
    )";

    auto result = runtime->Eval(test_code, "test.js");
    double time_ms = result.get<double>();

    std::cout << "Created 1000 VNodes in " << time_ms << "ms" << std::endl;

    // 性能要求：1000个VNode创建应该在100ms内完成
    EXPECT_LT(time_ms, 100.0);
}

// ========== 集成测试 ==========

TEST_F(PreactIntegrationTest, SimpleVNodeCreation) {
    LoadPreactAsGlobal();

    std::string test_code = R"(
        var vnode = Preact.h('div', { className: 'test' }, 'Hello Preact!');
        vnode;
    )";

    auto vnode_result = runtime->Eval(test_code, "test.js");
    EXPECT_TRUE(vnode_result.contains("type"));
    EXPECT_EQ(vnode_result["type"], "div");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

