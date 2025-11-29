/**
 * @file test_preact_components.cpp
 * @brief Preact组件和Hooks测试
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
#include <memory>
#include <fstream>
#include <sstream>

using namespace lightui;

class PreactComponentTest : public ::testing::Test {
protected:
    std::unique_ptr<QuickJSRuntime> runtime;
    std::shared_ptr<Document> document;

    void SetUp() override {
        // 创建运行时
        runtime = std::make_unique<QuickJSRuntime>();

        // 创建文档
        document = std::make_shared<Document>();
        document->Initialize();

        // 初始化DOM绑定 - 提供标准 DOM API
        DOMBindings::Init(runtime->GetContext());

        // 暴露document到JavaScript
        JSValue global = JS_GetGlobalObject(runtime->GetContext());
        JSValue doc_obj = DOMBindings::WrapDocument(runtime->GetContext(), document);
        JS_SetPropertyStr(runtime->GetContext(), global, "document", doc_obj);
        JS_FreeValue(runtime->GetContext(), global);

        // 加载Preact库 (JavaScript层)
        LoadPreactLibrary();
    }

    void TearDown() override {
        DOMBindings::Cleanup(runtime->GetContext());
        document.reset();
        runtime.reset();
    }

    void LoadPreactLibrary() {
        // 读取preact.js
        std::string preact_code = ReadFile("js/preact/preact.js");
        if (!preact_code.empty()) {
            runtime->Eval(preact_code, "preact.js");
        }

        // 读取hooks.js
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
};

// 测试1: 函数组件基础渲染
TEST_F(PreactComponentTest, FunctionComponentBasic) {
    std::string code = R"(
        // 使用 Preact 全局对象的方法
        function Greeting() {
            return Preact.h('div', null, 'Hello from component!');
        }

        Preact.render(Preact.h(Greeting), document.body);

        document.body.children.length;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 1);
}

// 测试2: 带Props的函数组件
TEST_F(PreactComponentTest, FunctionComponentWithProps) {
    std::string code = R"(
        function Greeting(props) {
            return Preact.h('div', null, 'Hello, ' + props.name + '!');
        }

        Preact.render(Preact.h(Greeting, { name: 'World' }), document.body);

        document.body.children[0].textContent;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "Hello, World!");
}

// 测试3: 嵌套组件
TEST_F(PreactComponentTest, NestedComponents) {
    std::string code = R"(
        function Title() {
            return Preact.h('h1', null, 'Title');
        }

        function Content() {
            return Preact.h('p', null, 'Content');
        }

        function App() {
            return Preact.h('div', null,
                Preact.h(Title),
                Preact.h(Content)
            );
        }

        Preact.render(Preact.h(App), document.body);

        document.body.children[0].children.length;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 2);
}

// 测试4: useState Hook基础
TEST_F(PreactComponentTest, UseStateBasic) {
    std::string code = R"(
        function Counter() {
            const [count, setCount] = PreactHooks.useState(0);
            return Preact.h('div', null, 'Count: ' + count);
        }

        Preact.render(Preact.h(Counter), document.body);

        document.body.children[0].textContent;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "Count: 0");
}

// 测试5: useState更新
TEST_F(PreactComponentTest, UseStateUpdate) {
    std::string code = R"(
        let updateCount;

        function Counter() {
            const [count, setCount] = PreactHooks.useState(0);
            updateCount = setCount;
            return Preact.h('div', null, 'Count: ' + count);
        }

        Preact.render(Preact.h(Counter), document.body);

        // 更新状态
        updateCount(5);

        document.body.children[0].textContent;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, "Count: 5");
}

// 测试6: useLayoutEffect Hook (同步执行)
TEST_F(PreactComponentTest, UseLayoutEffectBasic) {
    std::string code = R"(
        let effectRan = false;

        function EffectComponent() {
            // 使用 useLayoutEffect 而不是 useEffect，因为它是同步执行的
            PreactHooks.useLayoutEffect(() => {
                effectRan = true;
            }, []);

            return Preact.h('div', null, 'EffectComponent');
        }

        Preact.render(Preact.h(EffectComponent), document.body);

        effectRan;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, true);
}

// 测试7: 多个Hooks
TEST_F(PreactComponentTest, MultipleHooks) {
    std::string code = R"(
        let effectCount = 0;

        function MultiHooksComponent() {
            const [count, setCount] = PreactHooks.useState(0);
            const [name, setName] = PreactHooks.useState('Test');

            // 使用 useLayoutEffect 而不是 useEffect，因为它是同步执行的
            PreactHooks.useLayoutEffect(() => {
                effectCount++;
            }, [count]);

            return Preact.h('div', null, name + ': ' + count);
        }

        Preact.render(Preact.h(MultiHooksComponent), document.body);

        effectCount;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 1);
}

// 测试8: 性能测试 - 渲染多个组件
TEST_F(PreactComponentTest, PerformanceMultipleComponents) {
    std::string code = R"(
        function Item(props) {
            return Preact.h('div', null, 'Item ' + props.index);
        }

        function List() {
            const items = [];
            for (let i = 0; i < 50; i++) {
                items.push(Preact.h(Item, { index: i, key: i }));
            }
            return Preact.h('div', null, items);
        }

        const start = Date.now();
        Preact.render(Preact.h(List), document.body);
        const end = Date.now();

        console.log('Rendered 50 components in ' + (end - start) + 'ms');

        document.body.children[0].children.length;
    )";

    auto result = runtime->Eval(code, "test.js");
    EXPECT_EQ(result, 50);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

