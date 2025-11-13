/**
 * @file benchmark_timer_performance.cpp
 * @brief Timer性能基准测试
 * 
 * 测试Timer队列优化前后的性能对比：
 * - 优化前：std::priority_queue（无法高效删除）
 * - 优化后：std::multimap（支持O(1)删除）
 */

#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include "quickjs/quickjs_runtime.h"
#include "dom/dom_bindings.h"

using namespace lightui;

class TimerBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        runtime = std::make_unique<QuickJSRuntime>();
        DOMBindings::Init(runtime->GetContext());
    }

    void TearDown() override {
        DOMBindings::Cleanup(runtime->GetContext());
        runtime.reset();
    }

    // 测量执行时间（微秒）
    template<typename Func>
    int64_t MeasureTime(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    // 获取QuickJS内存使用
    size_t GetMemoryUsage() {
        JSMemoryUsage stats;
        JS_ComputeMemoryUsage(JS_GetRuntime(runtime->GetContext()), &stats);
        return stats.malloc_size;
    }

    std::unique_ptr<QuickJSRuntime> runtime;
};

// 基准测试1: 创建大量Timer然后立即清除
TEST_F(TimerBenchmark, CreateAndClearManyTimers) {
    const int num_timers = 10000;
    
    std::cout << "\n=== Benchmark: Create and Clear " << num_timers << " Timers ===" << std::endl;
    
    size_t initial_memory = GetMemoryUsage();
    
    // 测量创建时间
    int64_t create_time = MeasureTime([&]() {
        const char* code = R"(
            let timers = [];
            for (let i = 0; i < 10000; i++) {
                let timer = setTimeout(function() {}, 1000);
                timers.push(timer);
            }
        )";
        runtime->Eval(code, "benchmark.js");
    });
    
    size_t after_create_memory = GetMemoryUsage();
    
    // 测量清除时间
    int64_t clear_time = MeasureTime([&]() {
        const char* code = R"(
            for (let timer of timers) {
                clearTimeout(timer);
            }
        )";
        runtime->Eval(code, "benchmark.js");
    });
    
    // 清除JavaScript中的timers数组引用
    runtime->Eval("timers = null;", "benchmark.js");
    runtime->RunGC();
    size_t after_clear_memory = GetMemoryUsage();

    std::cout << "  Create time: " << create_time << " μs ("
              << std::fixed << std::setprecision(2)
              << (create_time / 1000.0) << " ms)" << std::endl;
    std::cout << "  Clear time:  " << clear_time << " μs ("
              << (clear_time / 1000.0) << " ms)" << std::endl;
    std::cout << "  Total time:  " << (create_time + clear_time) << " μs ("
              << ((create_time + clear_time) / 1000.0) << " ms)" << std::endl;
    std::cout << "  Memory after create: " << (after_create_memory - initial_memory) << " bytes" << std::endl;
    std::cout << "  Memory after clear:  " << (after_clear_memory - initial_memory) << " bytes" << std::endl;

    // 性能要求：清除10000个timer应该在100ms内完成
    EXPECT_LT(clear_time, 100000) << "Clear operation too slow";

    // 内存要求：清除后内存增长应该小于500KB（允许一些QuickJS内部开销）
    EXPECT_LT(after_clear_memory - initial_memory, 500 * 1024) << "Memory not properly freed";
}

// 基准测试2: 混合创建和清除
TEST_F(TimerBenchmark, MixedCreateAndClear) {
    const int iterations = 1000;
    const int timers_per_iteration = 10;
    
    std::cout << "\n=== Benchmark: Mixed Create/Clear (" << iterations << " iterations) ===" << std::endl;
    
    size_t initial_memory = GetMemoryUsage();
    
    int64_t total_time = MeasureTime([&]() {
        for (int i = 0; i < iterations; i++) {
            const char* code = R"(
                (function() {
                    let timers = [];
                    for (let j = 0; j < 10; j++) {
                        timers.push(setTimeout(function() {}, 100));
                    }
                    for (let timer of timers) {
                        clearTimeout(timer);
                    }
                })();
            )";
            runtime->Eval(code, "benchmark.js");
            
            if (i % 100 == 0) {
                runtime->RunGC();
            }
        }
    });
    
    runtime->RunGC();
    size_t final_memory = GetMemoryUsage();
    
    std::cout << "  Total time: " << total_time << " μs (" 
              << std::fixed << std::setprecision(2) 
              << (total_time / 1000.0) << " ms)" << std::endl;
    std::cout << "  Avg per iteration: " << (total_time / iterations) << " μs" << std::endl;
    std::cout << "  Memory delta: " << (final_memory - initial_memory) << " bytes" << std::endl;
    
    // 性能要求：平均每次迭代应该在1ms内完成
    EXPECT_LT(total_time / iterations, 1000) << "Average iteration too slow";
    
    // 内存要求：最终内存增长应该小于50KB
    EXPECT_LT(final_memory - initial_memory, 50 * 1024) << "Memory leak detected";
}

// 基准测试3: 选择性清除（清除一半timer）
TEST_F(TimerBenchmark, SelectiveClear) {
    const int num_timers = 5000;
    
    std::cout << "\n=== Benchmark: Selective Clear (clear half of " << num_timers << " timers) ===" << std::endl;
    
    size_t initial_memory = GetMemoryUsage();
    
    // 创建timer
    int64_t create_time = MeasureTime([&]() {
        const char* code = R"(
            let timers = [];
            for (let i = 0; i < 5000; i++) {
                timers.push(setTimeout(function() {}, 1000));
            }
        )";
        runtime->Eval(code, "benchmark.js");
    });
    
    // 清除一半
    int64_t clear_time = MeasureTime([&]() {
        const char* code = R"(
            for (let i = 0; i < timers.length; i += 2) {
                clearTimeout(timers[i]);
            }
        )";
        runtime->Eval(code, "benchmark.js");
    });
    
    runtime->RunGC();
    size_t after_clear_memory = GetMemoryUsage();
    
    std::cout << "  Create time: " << create_time << " μs" << std::endl;
    std::cout << "  Clear time:  " << clear_time << " μs" << std::endl;
    std::cout << "  Memory delta: " << (after_clear_memory - initial_memory) << " bytes" << std::endl;
    
    // 性能要求：清除2500个timer应该在50ms内完成
    EXPECT_LT(clear_time, 50000) << "Selective clear too slow";
}

// 基准测试4: 大量setInterval的创建和清除
TEST_F(TimerBenchmark, IntervalCreateAndClear) {
    const int num_intervals = 1000;
    
    std::cout << "\n=== Benchmark: Create and Clear " << num_intervals << " Intervals ===" << std::endl;
    
    size_t initial_memory = GetMemoryUsage();
    
    int64_t create_time = MeasureTime([&]() {
        const char* code = R"(
            let intervals = [];
            for (let i = 0; i < 1000; i++) {
                intervals.push(setInterval(function() {}, 100));
            }
        )";
        runtime->Eval(code, "benchmark.js");
    });
    
    int64_t clear_time = MeasureTime([&]() {
        const char* code = R"(
            for (let interval of intervals) {
                clearInterval(interval);
            }
        )";
        runtime->Eval(code, "benchmark.js");
    });
    
    runtime->RunGC();
    size_t final_memory = GetMemoryUsage();
    
    std::cout << "  Create time: " << create_time << " μs" << std::endl;
    std::cout << "  Clear time:  " << clear_time << " μs" << std::endl;
    std::cout << "  Memory delta: " << (final_memory - initial_memory) << " bytes" << std::endl;
    
    // 性能要求
    EXPECT_LT(clear_time, 50000) << "Interval clear too slow";
    EXPECT_LT(final_memory - initial_memory, 50 * 1024) << "Memory leak detected";
}

// 基准测试5: 压力测试 - 快速创建和清除
TEST_F(TimerBenchmark, RapidCreateClearCycles) {
    const int cycles = 100;
    const int timers_per_cycle = 100;
    
    std::cout << "\n=== Benchmark: Rapid Create/Clear Cycles (" << cycles << " cycles) ===" << std::endl;
    
    size_t initial_memory = GetMemoryUsage();
    
    int64_t total_time = MeasureTime([&]() {
        for (int i = 0; i < cycles; i++) {
            // 使用IIFE避免变量重复声明
            const char* code = R"(
                (function() {
                    let batch = [];
                    for (let j = 0; j < 100; j++) {
                        batch.push(setTimeout(function() {}, 50));
                    }
                    for (let timer of batch) {
                        clearTimeout(timer);
                    }
                })();
            )";
            runtime->Eval(code, "benchmark.js");
        }
    });
    
    runtime->RunGC();
    size_t final_memory = GetMemoryUsage();
    
    std::cout << "  Total time: " << total_time << " μs (" 
              << (total_time / 1000.0) << " ms)" << std::endl;
    std::cout << "  Avg per cycle: " << (total_time / cycles) << " μs" << std::endl;
    std::cout << "  Memory delta: " << (final_memory - initial_memory) << " bytes" << std::endl;
    
    // 性能要求：平均每个周期应该在3ms内完成
    EXPECT_LT(total_time / cycles, 3000) << "Cycle too slow";
    
    // 内存要求：最终内存增长应该小于20KB
    EXPECT_LT(final_memory - initial_memory, 20 * 1024) << "Memory leak detected";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

