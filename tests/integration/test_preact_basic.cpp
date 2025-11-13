/**
 * @file test_preact_basic.cpp
 * @brief Preact基础功能测试
 */

#include <gtest/gtest.h>
#include "core/quickjs/quickjs_runtime.h"
#include <fstream>
#include <sstream>

using namespace lightui;

class PreactBasicTest : public ::testing::Test {
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

TEST_F(PreactBasicTest, RuntimeCreation) {
    ASSERT_NE(runtime, nullptr);
    EXPECT_NE(runtime->GetContext(), nullptr);
}

TEST_F(PreactBasicTest, SimpleJavaScriptExecution) {
    auto result = runtime->Eval("1 + 1", "test.js");
    EXPECT_EQ(result, 2);
}

TEST_F(PreactBasicTest, CreateSimpleObject) {
    std::string code = R"(
        const obj = {
            type: 'div',
            props: { className: 'test' },
            children: ['Hello']
        };
        obj;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_TRUE(result.contains("type"));
    EXPECT_EQ(result["type"], "div");
    EXPECT_TRUE(result.contains("props"));
    EXPECT_EQ(result["props"]["className"], "test");
}

TEST_F(PreactBasicTest, CreateVNodeLikeObject) {
    std::string code = R"(
        function h(type, props, ...children) {
            return {
                type: type,
                props: props || {},
                children: children
            };
        }
        
        const vnode = h('div', { className: 'container' }, 'Hello', 'World');
        vnode;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_TRUE(result.contains("type"));
    EXPECT_EQ(result["type"], "div");
    EXPECT_TRUE(result.contains("props"));
    EXPECT_EQ(result["props"]["className"], "container");
    EXPECT_TRUE(result.contains("children"));
    EXPECT_EQ(result["children"].size(), 2);
}

TEST_F(PreactBasicTest, FunctionComponent) {
    std::string code = R"(
        function h(type, props, ...children) {
            return {
                type: type,
                props: props || {},
                children: children
            };
        }
        
        function Greeting(props) {
            return h('div', null, 'Hello, ' + props.name);
        }
        
        const vnode = Greeting({ name: 'World' });
        vnode;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_TRUE(result.contains("type"));
    EXPECT_EQ(result["type"], "div");
    EXPECT_TRUE(result.contains("children"));
    EXPECT_EQ(result["children"][0], "Hello, World");
}

TEST_F(PreactBasicTest, NestedVNodes) {
    std::string code = R"(
        function h(type, props, ...children) {
            return {
                type: type,
                props: props || {},
                children: children
            };
        }
        
        const vnode = h('div', { className: 'container' },
            h('h1', null, 'Title'),
            h('p', null, 'Paragraph')
        );
        vnode;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_TRUE(result.contains("type"));
    EXPECT_EQ(result["type"], "div");
    EXPECT_TRUE(result.contains("children"));
    EXPECT_EQ(result["children"].size(), 2);
    EXPECT_EQ(result["children"][0]["type"], "h1");
    EXPECT_EQ(result["children"][1]["type"], "p");
}

TEST_F(PreactBasicTest, ArrayFlattenInChildren) {
    std::string code = R"(
        function h(type, props, ...children) {
            const flatChildren = [];
            for (let child of children) {
                if (Array.isArray(child)) {
                    flatChildren.push(...child);
                } else if (child != null && child !== false && child !== true) {
                    flatChildren.push(child);
                }
            }
            
            return {
                type: type,
                props: props || {},
                children: flatChildren
            };
        }
        
        const items = ['Item 1', 'Item 2', 'Item 3'];
        const vnode = h('ul', null, items.map(item => h('li', null, item)));
        vnode;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    EXPECT_TRUE(result.contains("type"));
    EXPECT_EQ(result["type"], "ul");
    EXPECT_TRUE(result.contains("children"));
    EXPECT_EQ(result["children"].size(), 3);
}

TEST_F(PreactBasicTest, PerformanceVNodeCreation) {
    std::string code = R"(
        function h(type, props, ...children) {
            return {
                type: type,
                props: props || {},
                children: children
            };
        }
        
        const start = Date.now();
        for (let i = 0; i < 1000; i++) {
            h('div', { key: i }, 'Item ' + i);
        }
        const end = Date.now();
        
        end - start;
    )";
    
    auto result = runtime->Eval(code, "test.js");
    double time_ms = result.get<double>();
    
    std::cout << "Created 1000 VNodes in " << time_ms << "ms" << std::endl;
    
    // 性能要求：1000个VNode创建应该在100ms内完成
    EXPECT_LT(time_ms, 100.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

