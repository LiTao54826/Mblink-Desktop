/**
 * @file test_preact_components.cpp
 * @brief Preact组件和Hooks测试
 */

#include <gtest/gtest.h>
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/preact_renderer.h"
#include "core/quickjs/preact_bindings.h"
#include "core/dom/dom_bindings.h"
#include "core/dom/document.h"
#include "core/lexbor/lexbor_document.h"
#include <memory>
#include <fstream>
#include <sstream>

using namespace lightui;

class PreactComponentTest : public ::testing::Test {
protected:
    std::unique_ptr<QuickJSRuntime> runtime;
    std::shared_ptr<Document> document;
    std::shared_ptr<PreactRenderer> renderer;

    void SetUp() override {
        // 创建运行时
        runtime = std::make_unique<QuickJSRuntime>();
        
        // 创建文档
        document = std::make_shared<Document>();
        document->Initialize();
        
        // 创建渲染器
        renderer = std::make_shared<PreactRenderer>(runtime.get(), document);
        
        // 初始化DOM绑定
        DOMBindings::Init(runtime->GetContext());
        
        // 初始化Preact绑定
        PreactBindings::Init(runtime->GetContext(), renderer);
        
        // 暴露document到JavaScript
        JSValue global = JS_GetGlobalObject(runtime->GetContext());
        JSValue doc_obj = DOMBindings::WrapDocument(runtime->GetContext(), document);
        JS_SetPropertyStr(runtime->GetContext(), global, "document", doc_obj);
        JS_FreeValue(runtime->GetContext(), global);
        
        // 加载Preact库
        LoadPreactLibrary();
    }

    void TearDown() override {
        renderer.reset();
        document.reset();
        runtime.reset();
    }

    void LoadPreactLibrary() {
        // 读取preact.js
        std::ifstream preact_file("js/preact/preact.js");
        ASSERT_TRUE(preact_file.is_open()) << "Failed to open js/preact/preact.js";
        
        std::stringstream buffer;
        buffer << preact_file.rdbuf();
        std::string preact_code = buffer.str();
        
        // 执行Preact代码
        runtime->EvaluateScript(preact_code, "preact.js");
        
        // 读取hooks.js
        std::ifstream hooks_file("js/preact/hooks.js");
        ASSERT_TRUE(hooks_file.is_open()) << "Failed to open js/preact/hooks.js";
        
        buffer.str("");
        buffer.clear();
        buffer << hooks_file.rdbuf();
        std::string hooks_code = buffer.str();
        
        // 执行Hooks代码
        runtime->EvaluateScript(hooks_code, "hooks.js");
    }

    std::string GetBodyHTML() {
        auto body = document->GetBody();
        if (!body) return "";
        
        std::string html;
        auto children = body->GetChildNodes();
        for (const auto& child : children) {
            if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto elem = std::static_pointer_cast<Element>(child);
                html += "<" + elem->GetTagName() + ">";
                html += elem->GetTextContent();
                html += "</" + elem->GetTagName() + ">";
            }
        }
        return html;
    }
};

// 测试1: 函数组件基础渲染
TEST_F(PreactComponentTest, FunctionComponentBasic) {
    const char* code = R"(
        const { h, render } = Preact;
        
        function Greeting() {
            return h('div', null, 'Hello from component!');
        }
        
        render(h(Greeting), document.body);
        
        document.body.children.length;
    )";
    
    auto result = runtime->EvaluateScript(code, "test.js");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "1");
    
    auto body = document->GetBody();
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->GetChildNodes().size(), 1);
    
    auto div = std::static_pointer_cast<Element>(body->GetFirstChild());
    EXPECT_EQ(div->GetTagName(), "div");
    EXPECT_EQ(div->GetTextContent(), "Hello from component!");
}

// 测试2: 带Props的函数组件
TEST_F(PreactComponentTest, FunctionComponentWithProps) {
    const char* code = R"(
        const { h, render } = Preact;
        
        function Greeting(props) {
            return h('div', null, 'Hello, ' + props.name + '!');
        }
        
        render(h(Greeting, { name: 'World' }), document.body);
        
        document.body.children[0].textContent;
    )";
    
    auto result = runtime->EvaluateScript(code, "test.js");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Hello, World!");
}

// 测试3: 嵌套组件
TEST_F(PreactComponentTest, NestedComponents) {
    const char* code = R"(
        const { h, render } = Preact;
        
        function Title() {
            return h('h1', null, 'Title');
        }
        
        function Content() {
            return h('p', null, 'Content');
        }
        
        function App() {
            return h('div', null,
                h(Title),
                h(Content)
            );
        }
        
        render(h(App), document.body);
        
        document.body.children[0].children.length;
    )";
    
    auto result = runtime->EvaluateScript(code, "test.js");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "2");
}

// 测试4: useState Hook基础
TEST_F(PreactComponentTest, UseStateBasic) {
    const char* code = R"(
        const { h, render } = Preact;
        const { useState } = PreactHooks;
        
        function Counter() {
            const [count, setCount] = useState(0);
            return h('div', null, 'Count: ' + count);
        }
        
        render(h(Counter), document.body);
        
        document.body.children[0].textContent;
    )";
    
    auto result = runtime->EvaluateScript(code, "test.js");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Count: 0");
}

// 测试5: useState更新
TEST_F(PreactComponentTest, UseStateUpdate) {
    const char* code = R"(
        const { h, render } = Preact;
        const { useState } = PreactHooks;
        
        let updateCount;
        
        function Counter() {
            const [count, setCount] = useState(0);
            updateCount = setCount;
            return h('div', null, 'Count: ' + count);
        }
        
        render(h(Counter), document.body);
        
        // 更新状态
        updateCount(5);
        
        document.body.children[0].textContent;
    )";
    
    auto result = runtime->EvaluateScript(code, "test.js");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Count: 5");
}

// 测试6: useEffect Hook
TEST_F(PreactComponentTest, UseEffectBasic) {
    const char* code = R"(
        const { h, render } = Preact;
        const { useEffect } = PreactHooks;
        
        let effectRan = false;
        
        function Component() {
            useEffect(() => {
                effectRan = true;
            }, []);
            
            return h('div', null, 'Component');
        }
        
        render(h(Component), document.body);
        
        effectRan;
    )";
    
    auto result = runtime->EvaluateScript(code, "test.js");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "true");
}

// 测试7: 多个Hooks
TEST_F(PreactComponentTest, MultipleHooks) {
    const char* code = R"(
        const { h, render } = Preact;
        const { useState, useEffect } = PreactHooks;
        
        let effectCount = 0;
        
        function Component() {
            const [count, setCount] = useState(0);
            const [name, setName] = useState('Test');
            
            useEffect(() => {
                effectCount++;
            }, [count]);
            
            return h('div', null, name + ': ' + count);
        }
        
        render(h(Component), document.body);
        
        effectCount;
    )";
    
    auto result = runtime->EvaluateScript(code, "test.js");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "1");
}

// 测试8: 性能测试 - 渲染多个组件
TEST_F(PreactComponentTest, PerformanceMultipleComponents) {
    const char* code = R"(
        const { h, render } = Preact;
        
        function Item(props) {
            return h('div', null, 'Item ' + props.index);
        }
        
        function List() {
            const items = [];
            for (let i = 0; i < 50; i++) {
                items.push(h(Item, { index: i, key: i }));
            }
            return h('div', null, items);
        }
        
        const start = Date.now();
        render(h(List), document.body);
        const end = Date.now();
        
        console.log('Rendered 50 components in ' + (end - start) + 'ms');
        
        document.body.children[0].children.length;
    )";
    
    auto result = runtime->EvaluateScript(code, "test.js");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "50");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

