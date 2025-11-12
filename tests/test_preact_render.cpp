/**
 * @file test_preact_render.cpp
 * @brief Preact渲染集成测试
 */

#include <gtest/gtest.h>
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/preact_renderer.h"
#include "core/quickjs/preact_bindings.h"
#include "core/dom/dom_bindings.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/lexbor/lexbor_document.h"
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
        
        renderer = std::make_shared<PreactRenderer>(runtime.get(), document);
        
        // 初始化DOM绑定
        DOMBindings::Init(runtime->GetContext());
        
        // 初始化Preact绑定
        PreactBindings::Init(runtime->GetContext(), renderer);
        
        // 将document暴露给JavaScript
        JSValue global = JS_GetGlobalObject(runtime->GetContext());
        JSValue doc_obj = DOMBindings::WrapDocument(runtime->GetContext(), document);
        JS_SetPropertyStr(runtime->GetContext(), global, "document", doc_obj);
        JS_FreeValue(runtime->GetContext(), global);
    }
    
    void TearDown() override {
        PreactBindings::Cleanup(runtime->GetContext());
        DOMBindings::Cleanup(runtime->GetContext());
        renderer.reset();
        document.reset();
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
    std::shared_ptr<Document> document;
    std::shared_ptr<PreactRenderer> renderer;
};

// ========== 基础测试 ==========

TEST_F(PreactRenderTest, SetupComplete) {
    ASSERT_NE(runtime, nullptr);
    ASSERT_NE(document, nullptr);
    ASSERT_NE(renderer, nullptr);
}

TEST_F(PreactRenderTest, DocumentAvailable) {
    std::string code = "typeof document";
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "object");
}

TEST_F(PreactRenderTest, PreactInternalAvailable) {
    std::string code = "typeof __preact_internal";
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "object");
}

TEST_F(PreactRenderTest, PreactRenderFunctionAvailable) {
    std::string code = "typeof __preact_internal.render";
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

// ========== VNode渲染测试 ==========

TEST_F(PreactRenderTest, RenderSimpleVNode) {
    std::string code = R"(
        // 创建简单的VNode
        const vnode = {
            type: 'div',
            props: {},
            children: ['Hello World']
        };
        
        // 创建容器
        const container = document.createElement('div');
        document.body.appendChild(container);
        
        // 渲染
        __preact_internal.render(vnode, container);
        
        // 检查结果
        container.children.length;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 1);
}

TEST_F(PreactRenderTest, RenderVNodeWithProps) {
    std::string code = R"(
        const vnode = {
            type: 'div',
            props: {
                className: 'container',
                id: 'main'
            },
            children: []
        };
        
        const container = document.createElement('div');
        document.body.appendChild(container);
        
        __preact_internal.render(vnode, container);
        
        const rendered = container.children[0];
        rendered.className;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "container");
}

TEST_F(PreactRenderTest, RenderNestedVNodes) {
    std::string code = R"(
        const vnode = {
            type: 'div',
            props: {},
            children: [
                {
                    type: 'h1',
                    props: {},
                    children: ['Title']
                },
                {
                    type: 'p',
                    props: {},
                    children: ['Paragraph']
                }
            ]
        };
        
        const container = document.createElement('div');
        document.body.appendChild(container);
        
        __preact_internal.render(vnode, container);
        
        container.children[0].children.length;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 2);
}

// ========== Preact h()函数测试 ==========

TEST_F(PreactRenderTest, RenderWithHFunction) {
    std::string code = R"(
        function h(type, props, ...children) {
            return {
                type: type,
                props: props || {},
                children: children
            };
        }
        
        const vnode = h('div', { className: 'test' },
            h('h1', null, 'Hello'),
            h('p', null, 'World')
        );
        
        const container = document.createElement('div');
        document.body.appendChild(container);
        
        __preact_internal.render(vnode, container);
        
        container.children[0].children.length;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 2);
}

// ========== 性能测试 ==========

TEST_F(PreactRenderTest, PerformanceRenderManyElements) {
    std::string code = R"(
        function h(type, props, ...children) {
            return {
                type: type,
                props: props || {},
                children: children
            };
        }
        
        const items = [];
        for (let i = 0; i < 100; i++) {
            items.push(h('li', { key: i }, 'Item ' + i));
        }
        
        const vnode = h('ul', null, ...items);
        
        const container = document.createElement('div');
        document.body.appendChild(container);
        
        const start = Date.now();
        __preact_internal.render(vnode, container);
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

