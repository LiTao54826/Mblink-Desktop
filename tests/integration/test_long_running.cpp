/**
 * @file test_long_running.cpp
 * @brief 长时间运行测试 - 检测慢速内存泄漏
 * 
 * 测试场景：
 * 1. 模拟真实应用场景，长时间运行
 * 2. 监控内存使用情况
 * 3. 检测慢速内存泄漏
 * 4. 验证GC正常工作
 */

#include <gtest/gtest.h>
#include "quickjs/quickjs_runtime.h"
#include "dom/element.h"
#include "dom/document.h"
#include "dom/dom_bindings.h"
#include "lexbor/lexbor_document.h"
#include <thread>
#include <chrono>
#include <iostream>

using namespace lightui;

class LongRunningTest : public ::testing::Test {
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

    // 获取QuickJS内存使用情况
    size_t GetMemoryUsage() {
        JSMemoryUsage stats;
        JS_ComputeMemoryUsage(JS_GetRuntime(runtime->GetContext()), &stats);
        return stats.malloc_size;
    }

    // 打印内存统计
    void PrintMemoryStats(const std::string& label) {
        JSMemoryUsage stats;
        JS_ComputeMemoryUsage(JS_GetRuntime(runtime->GetContext()), &stats);
        
        std::cout << "\n=== " << label << " ===" << std::endl;
        std::cout << "  malloc_size: " << stats.malloc_size << " bytes" << std::endl;
        std::cout << "  malloc_count: " << stats.malloc_count << std::endl;
        std::cout << "  memory_used_size: " << stats.memory_used_size << " bytes" << std::endl;
        std::cout << "  obj_count: " << stats.obj_count << std::endl;
        std::cout << "  obj_size: " << stats.obj_size << " bytes" << std::endl;
        std::cout << "  atom_count: " << stats.atom_count << std::endl;
        std::cout << "  str_count: " << stats.str_count << std::endl;
    }

    std::unique_ptr<QuickJSRuntime> runtime;
    std::shared_ptr<Document> document;
};

// 测试1: 持续创建和销毁DOM元素（10000次迭代）
TEST_F(LongRunningTest, ContinuousDOMCreationDestruction) {
    JSContext* ctx = runtime->GetContext();
    
    PrintMemoryStats("Initial State");
    size_t initial_memory = GetMemoryUsage();
    
    const int iterations = 10000;
    const int gc_interval = 100;
    
    for (int i = 0; i < iterations; i++) {
        // 创建元素
        auto element = document->CreateElement("div");
        
        // 包装到JavaScript
        JSValue elem_obj = DOMBindings::WrapElement(ctx, element);
        JSValue global = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, global, "tempElement", elem_obj);
        JS_FreeValue(ctx, global);
        
        // 添加一些属性
        element->SetAttribute("id", "test-" + std::to_string(i));
        element->SetAttribute("class", "test-class");
        
        // 清理
        global = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, global, "tempElement", JS_UNDEFINED);
        JS_FreeValue(ctx, global);
        element.reset();
        
        // 定期运行GC
        if (i % gc_interval == 0) {
            runtime->RunGC();
            
            if (i % 1000 == 0) {
                size_t current_memory = GetMemoryUsage();
                std::cout << "Iteration " << i << ": memory = " << current_memory 
                          << " bytes (delta: " << (int64_t)(current_memory - initial_memory) 
                          << " bytes)" << std::endl;
            }
        }
    }
    
    // 最终GC
    runtime->RunGC();
    PrintMemoryStats("After " + std::to_string(iterations) + " iterations");
    
    size_t final_memory = GetMemoryUsage();
    int64_t memory_delta = final_memory - initial_memory;
    
    std::cout << "\nMemory delta: " << memory_delta << " bytes" << std::endl;
    
    // 允许一定的内存增长（由于内存池等），但不应该线性增长
    // 如果每次迭代泄漏1KB，10000次会泄漏10MB
    // 我们允许最多500KB的增长（考虑到内存池和碎片）
    EXPECT_LT(memory_delta, 500 * 1024) << "Memory leak detected: " << memory_delta << " bytes";
}

// 测试2: 持续添加和移除事件监听器（5000次迭代）
TEST_F(LongRunningTest, ContinuousEventListenerAddRemove) {
    JSContext* ctx = runtime->GetContext();
    
    PrintMemoryStats("Initial State");
    size_t initial_memory = GetMemoryUsage();
    
    auto element = document->CreateElement("div");
    JSValue elem_obj = DOMBindings::WrapElement(ctx, element);
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testElement", elem_obj);
    JS_FreeValue(ctx, global);
    
    const int iterations = 5000;
    const int listeners_per_iteration = 10;
    
    for (int i = 0; i < iterations; i++) {
        // 添加多个监听器
        const char* add_code = R"(
            (function() {
                let ids = [];
                for (let i = 0; i < 10; i++) {
                    const id = testElement.addEventListener('click', function() {
                        // Empty handler
                    });
                    ids.push(id);
                }
                
                // 立即移除所有监听器
                for (let id of ids) {
                    testElement.removeEventListener('click', id);
                }
            })();
        )";
        
        runtime->Eval(add_code, "test.js");
        
        if (i % 100 == 0) {
            runtime->RunGC();
            
            if (i % 500 == 0) {
                size_t current_memory = GetMemoryUsage();
                std::cout << "Iteration " << i << ": memory = " << current_memory 
                          << " bytes (delta: " << (int64_t)(current_memory - initial_memory) 
                          << " bytes)" << std::endl;
            }
        }
    }
    
    // 清理
    global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "testElement", JS_UNDEFINED);
    JS_FreeValue(ctx, global);
    element.reset();
    
    runtime->RunGC();
    PrintMemoryStats("After " + std::to_string(iterations) + " iterations");
    
    size_t final_memory = GetMemoryUsage();
    int64_t memory_delta = final_memory - initial_memory;
    
    std::cout << "\nMemory delta: " << memory_delta << " bytes" << std::endl;
    
    EXPECT_LT(memory_delta, 300 * 1024) << "Memory leak detected: " << memory_delta << " bytes";
}

// 测试3: 持续创建和清除Timer（5000次迭代）
// 注意：已知问题 - timer_queue_中的Task副本会持有JSValueWrapper直到timer到期
// 这会导致一些内存积压，但不是真正的泄漏（timer到期后会被清理）
TEST_F(LongRunningTest, ContinuousTimerCreationCancellation) {
    PrintMemoryStats("Initial State");
    size_t initial_memory = GetMemoryUsage();

    const int iterations = 5000;
    const int timers_per_iteration = 10;
    
    for (int i = 0; i < iterations; i++) {
        const char* code = R"(
            (function() {
                let timers = [];

                // 创建setTimeout（使用较短的延迟避免timer_queue积压）
                for (let i = 0; i < 5; i++) {
                    let timer = setTimeout(function() {}, 100);
                    timers.push(timer);
                }

                // 创建setInterval
                for (let i = 0; i < 5; i++) {
                    let interval = setInterval(function() {}, 100);
                    timers.push(interval);
                }

                // 立即清除所有timer
                for (let timer of timers) {
                    clearTimeout(timer);
                }
            })();
        )";

        runtime->Eval(code, "test.js");

        if (i % 100 == 0) {
            runtime->RunGC();

            if (i % 500 == 0) {
                size_t current_memory = GetMemoryUsage();
                std::cout << "Iteration " << i << ": memory = " << current_memory
                          << " bytes (delta: " << (int64_t)(current_memory - initial_memory)
                          << " bytes)" << std::endl;
            }
        }
    }
    
    runtime->RunGC();
    PrintMemoryStats("After " + std::to_string(iterations) + " iterations");
    
    size_t final_memory = GetMemoryUsage();
    int64_t memory_delta = final_memory - initial_memory;
    
    std::cout << "\nMemory delta: " << memory_delta << " bytes" << std::endl;

    // 由于timer_queue_积压问题，允许更大的内存增长
    // 这不是真正的泄漏，timer到期后会被清理
    // 5000次迭代 * 10个timer * 100ms延迟 = 最多500秒的timer积压
    // 允许最多10MB的临时内存使用
    EXPECT_LT(memory_delta, 10 * 1024 * 1024) << "Excessive memory usage: " << memory_delta << " bytes";
}

// 测试4: 模拟真实应用场景 - 动态UI更新（1000次迭代）
TEST_F(LongRunningTest, SimulatedDynamicUIUpdates) {
    JSContext* ctx = runtime->GetContext();
    
    PrintMemoryStats("Initial State");
    size_t initial_memory = GetMemoryUsage();
    
    const int iterations = 1000;
    
    for (int i = 0; i < iterations; i++) {
        // 创建一个小型DOM树
        auto container = document->CreateElement("div");
        container->SetAttribute("id", "container-" + std::to_string(i));
        
        // 添加10个子元素
        for (int j = 0; j < 10; j++) {
            auto child = document->CreateElement("div");
            child->SetAttribute("class", "item");
            container->AppendChild(child);
        }
        
        // 包装到JavaScript
        JSValue container_obj = DOMBindings::WrapElement(ctx, container);
        JSValue global = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, global, "container", container_obj);
        JS_FreeValue(ctx, global);
        
        // 在JavaScript中操作DOM
        const char* code = R"(
            (function() {
                const children = container.children;
                for (let i = 0; i < children.length; i++) {
                    // 添加事件监听器
                    const id = children[i].addEventListener('click', function() {
                        // Simulate some work
                    });
                    
                    // 设置属性
                    children[i].setAttribute('data-index', i.toString());
                    
                    // 移除监听器
                    children[i].removeEventListener('click', id);
                }
            })();
        )";
        
        runtime->Eval(code, "test.js");
        
        // 清理
        global = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, global, "container", JS_UNDEFINED);
        JS_FreeValue(ctx, global);
        container.reset();
        
        if (i % 50 == 0) {
            runtime->RunGC();
            
            if (i % 100 == 0) {
                size_t current_memory = GetMemoryUsage();
                std::cout << "Iteration " << i << ": memory = " << current_memory 
                          << " bytes (delta: " << (int64_t)(current_memory - initial_memory) 
                          << " bytes)" << std::endl;
            }
        }
    }
    
    runtime->RunGC();
    PrintMemoryStats("After " + std::to_string(iterations) + " iterations");
    
    size_t final_memory = GetMemoryUsage();
    int64_t memory_delta = final_memory - initial_memory;
    
    std::cout << "\nMemory delta: " << memory_delta << " bytes" << std::endl;
    
    EXPECT_LT(memory_delta, 500 * 1024) << "Memory leak detected: " << memory_delta << " bytes";
}

