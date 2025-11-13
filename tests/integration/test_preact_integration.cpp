/**
 * @file test_preact_integration.cpp
 * @brief Preact集成测试
 */

#include <gtest/gtest.h>
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/preact_renderer.h"
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

    std::unique_ptr<QuickJSRuntime> runtime;
};

// ========== 基础测试 ==========

TEST_F(PreactIntegrationTest, RuntimeCreation) {
    ASSERT_NE(runtime, nullptr);
    EXPECT_NE(runtime->GetContext(), nullptr);
}

TEST_F(PreactIntegrationTest, LoadPreactModule) {
    // 读取Preact模块
    std::string preact_code = ReadFile("js/preact/preact.js");
    ASSERT_FALSE(preact_code.empty()) << "Failed to read js/preact/preact.js";
    
    // 注册Preact模块
    runtime->RegisterModule("preact", preact_code);
    
    // 加载模块
    auto result = runtime->LoadModule("preact");
    EXPECT_TRUE(result.contains("h"));
    EXPECT_TRUE(result.contains("render"));
    EXPECT_TRUE(result.contains("createElement"));
}

TEST_F(PreactIntegrationTest, LoadHooksModule) {
    // 读取Hooks模块
    std::string hooks_code = ReadFile("js/preact/hooks.js");
    ASSERT_FALSE(hooks_code.empty()) << "Failed to read js/preact/hooks.js";
    
    // 注册Hooks模块
    runtime->RegisterModule("preact/hooks", hooks_code);
    
    // 加载模块
    auto result = runtime->LoadModule("preact/hooks");
    EXPECT_TRUE(result.contains("useState"));
    EXPECT_TRUE(result.contains("useEffect"));
    EXPECT_TRUE(result.contains("useRef"));
}

// ========== VNode创建测试 ==========

TEST_F(PreactIntegrationTest, CreateSimpleVNode) {
    std::string preact_code = ReadFile("js/preact/preact.js");
    runtime->RegisterModule("preact", preact_code);
    
    std::string test_code = R"(
        import { h } from 'preact';
        const vnode = h('div', null, 'Hello World');
        vnode;
    )";
    
    auto result = runtime->Eval(test_code, "test.js");
    EXPECT_TRUE(result.contains("type"));
    EXPECT_EQ(result["type"], "div");
    EXPECT_TRUE(result.contains("children"));
}

TEST_F(PreactIntegrationTest, CreateVNodeWithProps) {
    std::string preact_code = ReadFile("js/preact/preact.js");
    runtime->RegisterModule("preact", preact_code);
    
    std::string test_code = R"(
        import { h } from 'preact';
        const vnode = h('div', { className: 'container', id: 'main' }, 'Content');
        vnode;
    )";
    
    auto result = runtime->Eval(test_code, "test.js");
    EXPECT_TRUE(result.contains("props"));
    EXPECT_EQ(result["props"]["className"], "container");
    EXPECT_EQ(result["props"]["id"], "main");
}

TEST_F(PreactIntegrationTest, CreateVNodeWithChildren) {
    std::string preact_code = ReadFile("js/preact/preact.js");
    runtime->RegisterModule("preact", preact_code);
    
    std::string test_code = R"(
        import { h } from 'preact';
        const vnode = h('div', null,
            h('h1', null, 'Title'),
            h('p', null, 'Paragraph')
        );
        vnode;
    )";
    
    auto result = runtime->Eval(test_code, "test.js");
    EXPECT_TRUE(result.contains("children"));
    EXPECT_EQ(result["children"].size(), 2);
}

// ========== 组件测试 ==========

TEST_F(PreactIntegrationTest, CreateFunctionComponent) {
    std::string preact_code = ReadFile("js/preact/preact.js");
    runtime->RegisterModule("preact", preact_code);
    
    std::string test_code = R"(
        import { h } from 'preact';
        
        function Greeting(props) {
            return h('div', null, 'Hello, ', props.name);
        }
        
        const vnode = h(Greeting, { name: 'World' });
        vnode;
    )";
    
    auto result = runtime->Eval(test_code, "test.js");
    EXPECT_TRUE(result.contains("type"));
    EXPECT_TRUE(result.contains("props"));
    EXPECT_EQ(result["props"]["name"], "World");
}

// ========== Hooks测试 ==========

TEST_F(PreactIntegrationTest, UseStateHook) {
    std::string preact_code = ReadFile("js/preact/preact.js");
    std::string hooks_code = ReadFile("js/preact/hooks.js");
    
    runtime->RegisterModule("preact", preact_code);
    runtime->RegisterModule("preact/hooks", hooks_code);
    
    std::string test_code = R"(
        import { h } from 'preact';
        import { useState } from 'preact/hooks';
        
        function Counter() {
            const [count, setCount] = useState(0);
            return h('div', null, 'Count: ', count);
        }
        
        const vnode = h(Counter);
        vnode;
    )";
    
    // 这个测试主要验证代码能够执行，不会抛出异常
    EXPECT_NO_THROW({
        auto result = runtime->Eval(test_code, "test.js");
    });
}

// ========== 性能测试 ==========

TEST_F(PreactIntegrationTest, PerformanceVNodeCreation) {
    std::string preact_code = ReadFile("js/preact/preact.js");
    runtime->RegisterModule("preact", preact_code);
    
    std::string test_code = R"(
        import { h } from 'preact';
        
        const start = Date.now();
        for (let i = 0; i < 1000; i++) {
            h('div', { key: i }, 'Item ', i);
        }
        const end = Date.now();
        
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
    std::string preact_code = ReadFile("js/preact/preact.js");
    ASSERT_FALSE(preact_code.empty());

    runtime->RegisterModule("preact", preact_code);

    // 创建一个简单的VNode
    std::string test_code = R"(
        import { h } from 'preact';
        const vnode = h('div', { className: 'test' }, 'Hello Preact!');
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

