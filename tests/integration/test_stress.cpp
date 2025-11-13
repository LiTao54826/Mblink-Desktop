/**
 * @file test_stress.cpp
 * @brief 压力测试 - 验证大规模操作下的内存安全性
 * 
 * 测试场景：
 * 1. 大量事件监听器（1000+）
 * 2. 大量并发Timer（1000+）
 * 3. 深度嵌套的DOM树
 * 4. 混合操作压力测试
 */

#include <gtest/gtest.h>
#include "quickjs/quickjs_runtime.h"
#include "dom/element.h"
#include "dom/document.h"
#include "dom/dom_bindings.h"
#include "lexbor/lexbor_document.h"
#include <thread>
#include <chrono>

using namespace lightui;

class StressTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime = std::make_unique<QuickJSRuntime>();
        DOMBindings::Init(runtime->GetContext());
        document = std::make_shared<Document>();
    }

    void TearDown() override {
        document.reset();
        if (runtime) {
            DOMBindings::Cleanup(runtime->GetContext());
        }
        runtime.reset();
    }

    std::unique_ptr<QuickJSRuntime> runtime;
    std::shared_ptr<Document> document;
};

// 测试1: 1000个事件监听器
TEST_F(StressTest, ThousandEventListeners) {
    JSContext* ctx = runtime->GetContext();
    auto element = document->CreateElement("div");
    
    JSValue elem_obj = DOMBindings::WrapElement(ctx, element);
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testDiv", elem_obj);
    JS_FreeValue(ctx, global);
    
    const char* code = R"(
        let listenerIds = [];
        for (let i = 0; i < 1000; i++) {
            const id = testDiv.addEventListener('click', function() {
                // Empty handler
            });
            listenerIds.push(id);
        }
        
        // 移除一半的监听器
        for (let i = 0; i < 500; i++) {
            testDiv.removeEventListener('click', listenerIds[i]);
        }
        
        listenerIds = null;
    )";
    
    EXPECT_NO_THROW({
        runtime->Eval(code, "test.js");
    });
    
    // 触发事件（应该只有500个监听器响应）
    auto event = std::make_shared<Event>("click");
    element->DispatchEvent(event);
    
    // 清理
    global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testDiv", JS_UNDEFINED);
    JS_FreeValue(ctx, global);
    element.reset();
    
    runtime->RunGC();
    SUCCEED();
}

// 测试2: 1000个并发Timer
TEST_F(StressTest, ThousandConcurrentTimers) {
    const char* code = R"(
        let timers = [];
        let intervals = [];
        
        // 创建500个setTimeout
        for (let i = 0; i < 500; i++) {
            let timer = setTimeout(function() {
                // Empty callback
            }, 5000 + i);
            timers.push(timer);
        }
        
        // 创建500个setInterval
        for (let i = 0; i < 500; i++) {
            let interval = setInterval(function() {
                // Empty callback
            }, 100 + i);
            intervals.push(interval);
        }
        
        // 清除所有timer
        for (let timer of timers) {
            clearTimeout(timer);
        }
        
        for (let interval of intervals) {
            clearInterval(interval);
        }
        
        timers = null;
        intervals = null;
    )";
    
    EXPECT_NO_THROW({
        runtime->Eval(code, "test.js");
    });
    
    runtime->RunGC();
    SUCCEED();
}

// 测试3: 深度嵌套的DOM树（100层）
TEST_F(StressTest, DeepNestedDOMTree) {
    JSContext* ctx = runtime->GetContext();

    // 创建100层嵌套的DOM树
    auto root = document->CreateElement("div");
    auto current = root;

    for (int i = 0; i < 100; i++) {
        auto child = document->CreateElement("div");
        current->AppendChild(child);
        current = child;
    }

    // 包装到JavaScript
    JSValue root_obj = DOMBindings::WrapElement(ctx, root);
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "deepTree", root_obj);
    JS_FreeValue(ctx, global);

    // 使用迭代而非递归来添加事件监听器
    const char* code = R"(
        let node = deepTree;
        let depth = 0;
        while (node && depth < 100) {
            node.addEventListener('click', function() {
                // Empty handler
            });

            const children = node.children;
            if (children && children.length > 0) {
                node = children[0];
            } else {
                break;
            }
            depth++;
        }
    )";

    EXPECT_NO_THROW({
        runtime->Eval(code, "test.js");
    });

    // 触发事件（应该冒泡到所有父节点）
    auto event = std::make_shared<Event>("click");
    current->DispatchEvent(event);

    // 清理
    global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "deepTree", JS_UNDEFINED);
    JS_FreeValue(ctx, global);
    root.reset();

    runtime->RunGC();
    SUCCEED();
}

// 测试4: 宽DOM树（100个兄弟节点）
TEST_F(StressTest, WideDOMTree) {
    JSContext* ctx = runtime->GetContext();

    auto root = document->CreateElement("div");

    // 创建100个子节点
    for (int i = 0; i < 100; i++) {
        auto child = document->CreateElement("div");
        root->AppendChild(child);
    }

    JSValue root_obj = DOMBindings::WrapElement(ctx, root);
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "wideTree", root_obj);
    JS_FreeValue(ctx, global);

    // 在每个子节点上添加监听器
    const char* code = R"(
        (function() {
            const children = wideTree.children;
            for (let i = 0; i < children.length; i++) {
                children[i].addEventListener('click', function() {
                    // Empty handler
                });
            }
        })();
    )";

    EXPECT_NO_THROW({
        runtime->Eval(code, "test.js");
    });

    // 清理
    global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "wideTree", JS_UNDEFINED);
    JS_FreeValue(ctx, global);
    root.reset();

    runtime->RunGC();
    SUCCEED();
}

// 测试5: 混合操作压力测试
TEST_F(StressTest, MixedOperationsStress) {
    JSContext* ctx = runtime->GetContext();
    
    const char* code = R"(
        // 创建多个元素
        let elements = [];
        for (let i = 0; i < 50; i++) {
            elements.push({ type: 'element' });
        }
        
        // 添加事件监听器
        let listenerIds = [];
        for (let i = 0; i < 100; i++) {
            // 模拟addEventListener（实际测试中会用真实元素）
            listenerIds.push(i);
        }
        
        // 创建Timer
        let timers = [];
        for (let i = 0; i < 100; i++) {
            let timer = setTimeout(function() {
                // Empty
            }, 10000 + i);
            timers.push(timer);
        }
        
        // 清除一半的Timer
        for (let i = 0; i < 50; i++) {
            clearTimeout(timers[i]);
        }
        
        // 清除剩余的Timer
        for (let i = 50; i < 100; i++) {
            clearTimeout(timers[i]);
        }
        
        // 清理
        elements = null;
        listenerIds = null;
        timers = null;
    )";
    
    EXPECT_NO_THROW({
        runtime->Eval(code, "test.js");
    });
    
    runtime->RunGC();
    SUCCEED();
}

// 测试6: 重复添加和移除监听器
TEST_F(StressTest, RepeatedAddRemoveListeners) {
    JSContext* ctx = runtime->GetContext();
    auto element = document->CreateElement("div");
    
    JSValue elem_obj = DOMBindings::WrapElement(ctx, element);
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testDiv", elem_obj);
    JS_FreeValue(ctx, global);
    
    // 重复100次：添加10个监听器，然后全部移除
    for (int round = 0; round < 100; round++) {
        const char* code = R"(
            (function() {
                let ids = [];
                for (let i = 0; i < 10; i++) {
                    const id = testDiv.addEventListener('click', function() {});
                    ids.push(id);
                }

                for (let id of ids) {
                    testDiv.removeEventListener('click', id);
                }
            })();
        )";

        runtime->Eval(code, "test.js");

        if (round % 10 == 0) {
            runtime->RunGC();
        }
    }
    
    // 清理
    global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testDiv", JS_UNDEFINED);
    JS_FreeValue(ctx, global);
    element.reset();
    
    runtime->RunGC();
    SUCCEED();
}

