/**
 * @file test_preact_render.cpp
 * @brief Preact渲染集成测试
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
#include "core/dom/dom_bindings.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace lightui;

class PreactRenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime = std::make_unique<QuickJSRuntime>();
        document = std::make_shared<Document>();
        document->Initialize();

        // 初始化DOM绑定 - 提供标准 DOM API
        DOMBindings::Init(runtime->GetContext());

        // 将document暴露给JavaScript
        JSValue global = JS_GetGlobalObject(runtime->GetContext());
        JSValue doc_obj = DOMBindings::WrapDocument(runtime->GetContext(), document);
        JS_SetPropertyStr(runtime->GetContext(), global, "document", doc_obj);
        JS_FreeValue(runtime->GetContext(), global);

        // 加载 Preact 库 (JavaScript 层)
        LoadPreactLibrary();
    }

    void TearDown() override {
        DOMBindings::Cleanup(runtime->GetContext());
        document.reset();
        runtime.reset();
    }

    void LoadPreactLibrary() {
        std::string preact_code = ReadFile("js/preact/preact.js");
        if (!preact_code.empty()) {
            runtime->Eval(preact_code, "preact.js");
        }

        std::string hooks_code = ReadFile("js/preact/hooks.js");
        if (!hooks_code.empty()) {
            runtime->Eval(hooks_code, "hooks.js");
        }
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
    std::shared_ptr<Document> document;
};

// ========== 基础测试 ==========

TEST_F(PreactRenderTest, SetupComplete) {
    ASSERT_NE(runtime, nullptr);
    ASSERT_NE(document, nullptr);
}

TEST_F(PreactRenderTest, DocumentAvailable) {
    std::string code = "typeof document";
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "object");
}

TEST_F(PreactRenderTest, PreactModuleLoaded) {
    // Preact 通过 JavaScript 加载，不需要 C++ 绑定
    std::string code = "typeof h";  // h 是 Preact 的 createElement 函数
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "function");
}

TEST_F(PreactRenderTest, PreactRenderFunctionAvailable) {
    std::string code = "typeof render";  // render 函数
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "function");
}

// ========== DOM创建测试 ==========

TEST_F(PreactRenderTest, CreateSimpleElement) {
    std::string code = R"(
        const div = document.createElement('div');
        div.tagName;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "div");
}

TEST_F(PreactRenderTest, CreateElementWithText) {
    std::string code = R"(
        const div = document.createElement('div');
        const text = document.createTextNode('Hello');
        div.appendChild(text);
        div.textContent;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "Hello");
}

TEST_F(PreactRenderTest, CreateNestedElements) {
    std::string code = R"(
        const div = document.createElement('div');
        const p = document.createElement('p');
        const text = document.createTextNode('Paragraph');
        p.appendChild(text);
        div.appendChild(p);
        div.children.length;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 1);
}

// ========== VNode渲染测试 (使用 JavaScript Preact API) ==========

TEST_F(PreactRenderTest, RenderSimpleVNode) {
    std::string code = R"(
        // 使用 h() 创建 VNode (Preact JavaScript API)
        const vnode = h('div', null, 'Hello World');

        // 创建容器
        const container = document.createElement('div');
        document.body.appendChild(container);

        // 使用 render() 渲染 (Preact JavaScript API)
        render(vnode, container);

        // 检查结果
        container.children.length;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 1);
}

TEST_F(PreactRenderTest, RenderVNodeWithProps) {
    std::string code = R"(
        const vnode = h('div', { className: 'container', id: 'main' });

        const container = document.createElement('div');
        document.body.appendChild(container);

        render(vnode, container);

        const rendered = container.children[0];
        rendered.className;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "container");
}

TEST_F(PreactRenderTest, RenderNestedVNodes) {
    std::string code = R"(
        const vnode = h('div', null,
            h('h1', null, 'Title'),
            h('p', null, 'Paragraph')
        );

        const container = document.createElement('div');
        document.body.appendChild(container);

        render(vnode, container);

        container.children[0].children.length;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 2);
}

// ========== 函数组件测试 ==========

TEST_F(PreactRenderTest, RenderFunctionComponent) {
    std::string code = R"(
        // 函数组件
        function Greeting(props) {
            return h('div', { className: 'greeting' },
                h('h1', null, 'Hello'),
                h('p', null, props.name)
            );
        }

        const container = document.createElement('div');
        document.body.appendChild(container);

        render(h(Greeting, { name: 'World' }), container);

        container.children.length;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 1);
}

// ========== 性能测试 ==========

TEST_F(PreactRenderTest, PerformanceRenderManyElements) {
    std::string code = R"(
        const items = [];
        for (let i = 0; i < 100; i++) {
            items.push(h('li', { key: i }, 'Item ' + i));
        }

        const vnode = h('ul', null, ...items);

        const container = document.createElement('div');
        document.body.appendChild(container);

        const start = Date.now();
        render(vnode, container);
        const end = Date.now();

        end - start;
    )";

    auto result = runtime->Eval(code, "test.js");
    double time_ms = result.get<double>();

    std::cout << "Rendered 100 elements in " << time_ms << "ms" << std::endl;

    // 性能要求：100个元素渲染应该在100ms内完成
    EXPECT_LT(time_ms, 100.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

