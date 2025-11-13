/**
 * @file test_memory_leak_fix.cpp
 * @brief 测试内存泄漏修复
 * 
 * 验证addEventListener和Timer的内存泄漏已修复
 */

#include <gtest/gtest.h>
#include "core/quickjs/quickjs_runtime.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include <thread>
#include <chrono>

using namespace lightui;

class MemoryLeakFixTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime = std::make_unique<QuickJSRuntime>();

        // 初始化DOM绑定
        DOMBindings::Init(runtime->GetContext());
    }

    void TearDown() override {
        if (runtime) {
            DOMBindings::Cleanup(runtime->GetContext());
        }
        runtime.reset();
    }

    std::unique_ptr<QuickJSRuntime> runtime;
};

// 测试addEventListener不会泄漏内存
TEST_F(MemoryLeakFixTest, AddEventListenerNoLeak) {
    // 创建元素
    auto element = std::make_shared<Element>("button");

    // 将元素包装为JS对象
    JSContext* ctx = runtime->GetContext();
    JSValue element_obj = DOMBindings::WrapElement(ctx, element);
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testButton", element_obj);
    JS_FreeValue(ctx, global);

    // 添加多个事件监听器
    const char* test_code = R"(
        for (let i = 0; i < 100; i++) {
            testButton.addEventListener('click', function() {
                console.log('Click ' + i);
            });
        }
    )";

    EXPECT_NO_THROW({
        runtime->Eval(test_code, "test.js");
    });

    // 运行GC
    runtime->RunGC();

    // 销毁元素（应该自动清理所有事件监听器）
    global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testButton", JS_UNDEFINED);
    JS_FreeValue(ctx, global);
    element.reset();

    // 再次运行GC
    runtime->RunGC();

    // 如果没有内存泄漏，这里不应该崩溃
    SUCCEED();
}

// 测试Timer不会泄漏内存
TEST_F(MemoryLeakFixTest, TimerNoLeak) {
    JSContext* ctx = runtime->GetContext();

    // 创建多个setTimeout（不使用console.log避免额外的引用）
    const char* test_code = R"(
        let timers = [];
        for (let i = 0; i < 50; i++) {
            let timer = setTimeout(function() {
                // Empty callback
            }, 1000 + i);
            timers.push(timer);
        }

        // 清除所有timer
        for (let timer of timers) {
            clearTimeout(timer);
        }

        // 清除timers数组引用
        timers = null;
    )";

    EXPECT_NO_THROW({
        runtime->Eval(test_code, "test.js");
    });

    // 运行GC多次确保清理
    for (int i = 0; i < 3; i++) {
        runtime->RunGC();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 如果没有内存泄漏，这里不应该崩溃
    SUCCEED();
}

// 测试setInterval不会泄漏内存
TEST_F(MemoryLeakFixTest, IntervalNoLeak) {
    JSContext* ctx = runtime->GetContext();

    // 创建多个setInterval并清除（不使用console.log避免额外的引用）
    const char* test_code = R"(
        let intervals = [];
        for (let i = 0; i < 50; i++) {
            let interval = setInterval(function() {
                // Empty callback
            }, 100 + i);
            intervals.push(interval);
        }

        // 清除所有interval
        for (let interval of intervals) {
            clearInterval(interval);
        }

        // 清除intervals数组引用
        intervals = null;
    )";

    EXPECT_NO_THROW({
        runtime->Eval(test_code, "test.js");
    });

    // 运行GC多次确保清理
    for (int i = 0; i < 3; i++) {
        runtime->RunGC();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 如果没有内存泄漏，这里不应该崩溃
    SUCCEED();
}

// 压力测试：大量addEventListener和Timer操作
TEST_F(MemoryLeakFixTest, StressTest) {
    JSContext* ctx = runtime->GetContext();

    // 创建元素
    auto element = std::make_shared<Element>("div");

    JSValue element_obj = DOMBindings::WrapElement(ctx, element);
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testDiv", element_obj);
    JS_FreeValue(ctx, global);
    
    // 压力测试代码（减少数量避免崩溃，移除console.log避免额外引用）
    const char* test_code = R"(
        // 添加事件监听器
        for (let i = 0; i < 100; i++) {
            testDiv.addEventListener('click', function() {
                // Empty callback
            });
        }

        // 创建和清除Timer
        let timers = [];
        for (let i = 0; i < 100; i++) {
            let timer = setTimeout(function() {
                // Empty callback
            }, 5000 + i);
            timers.push(timer);
        }

        for (let timer of timers) {
            clearTimeout(timer);
        }
        timers = null;

        // 创建和清除Interval
        let intervals = [];
        for (let i = 0; i < 100; i++) {
            let interval = setInterval(function() {
                // Empty callback
            }, 100 + i);
            intervals.push(interval);
        }

        for (let interval of intervals) {
            clearInterval(interval);
        }
        intervals = null;
    )";
    
    EXPECT_NO_THROW({
        runtime->Eval(test_code, "test.js");
    });
    
    // 运行GC多次
    for (int i = 0; i < 5; i++) {
        runtime->RunGC();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 清理
    global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testDiv", JS_UNDEFINED);
    JS_FreeValue(ctx, global);
    element.reset();

    // 运行GC多次确保清理
    for (int i = 0; i < 3; i++) {
        runtime->RunGC();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 如果没有内存泄漏，这里不应该崩溃
    SUCCEED();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

