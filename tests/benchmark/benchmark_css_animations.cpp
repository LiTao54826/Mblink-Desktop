/**
 * @file benchmark_css_animations.cpp
 * @brief CSS 动画性能基准测试
 */

#include "core/render/animation_controller.h"
#include "core/render/render_object.h"
#include "core/render/animation.h"
#include "core/render/keyframes.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>

using namespace lightui;
using namespace std::chrono;

// ============================================================================
// 性能测试工具
// ============================================================================

class PerformanceTimer {
public:
    void Start() {
        start_time_ = high_resolution_clock::now();
    }
    
    double Stop() {
        auto end_time = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end_time - start_time_);
        return duration.count() / 1000.0; // 返回毫秒
    }
    
private:
    high_resolution_clock::time_point start_time_;
};

void PrintResult(const std::string& test_name, double time_ms, size_t operations) {
    double ops_per_sec = (operations / time_ms) * 1000.0;
    std::cout << std::left << std::setw(50) << test_name 
              << std::right << std::setw(10) << std::fixed << std::setprecision(2) << time_ms << " ms"
              << std::setw(15) << std::fixed << std::setprecision(0) << ops_per_sec << " ops/s"
              << std::endl;
}

void PrintHeader(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::left << std::setw(50) << "Test Name" 
              << std::right << std::setw(10) << "Time"
              << std::setw(15) << "Throughput"
              << std::endl;
    std::cout << std::string(75, '-') << std::endl;
}

// ============================================================================
// 测试场景
// ============================================================================

/**
 * @brief 场景 1: 单个动画性能测试
 */
void BenchmarkSingleAnimation() {
    PrintHeader("Scenario 1: Single Animation Performance");

    AnimationController controller;
    auto object = std::make_shared<RenderObject>(RenderObjectType::BLOCK);
    
    // 创建简单的动画配置
    CSSAnimation config;
    config.name = "test-animation";
    config.duration = 1.0;
    config.iteration_count = -1; // 无限循环
    config.timing_function = TimingFunction::LINEAR;

    // 创建关键帧
    KeyframesRule keyframes;
    keyframes.name = "test-animation";

    Keyframe frame0;
    frame0.offset = 0.0f;
    frame0.properties["opacity"] = "0";
    keyframes.keyframes.push_back(frame0);

    Keyframe frame100;
    frame100.offset = 1.0f;
    frame100.properties["opacity"] = "1";
    keyframes.keyframes.push_back(frame100);

    controller.RegisterKeyframes(keyframes);
    controller.StartAnimation(object.get(), config);

    // 测试 1: 无优化
    controller.SetOptimizationEnabled(false);
    PerformanceTimer timer;
    timer.Start();

    const int iterations = 10000;
    for (int i = 0; i < iterations; i++) {
        controller.Update(i * 0.016); // 模拟 60 FPS
    }

    double time_no_opt = timer.Stop();
    PrintResult("Without Optimization", time_no_opt, iterations);

    // 重置
    controller.Clear();
    controller.RegisterKeyframes(keyframes);
    controller.StartAnimation(object.get(), config);
    
    // 测试 2: 有优化
    controller.SetOptimizationEnabled(true);
    timer.Start();
    
    for (int i = 0; i < iterations; i++) {
        controller.Update(i * 0.016);
    }
    
    double time_with_opt = timer.Stop();
    PrintResult("With Optimization", time_with_opt, iterations);
    
    // 计算提升
    double speedup = time_no_opt / time_with_opt;
    std::cout << "\nSpeedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    
    // 打印统计
    auto stats = controller.GetOptimizer().GetStats();
    std::cout << "Cache Hit Rate: " << std::fixed << std::setprecision(1) 
              << (stats.cache_hit_rate * 100) << "%" << std::endl;
}

/**
 * @brief 场景 2: 多个动画并发测试
 */
void BenchmarkMultipleAnimations() {
    PrintHeader("Scenario 2: Multiple Concurrent Animations");
    
    const int num_objects = 100;
    const int num_frames = 1000;
    
    AnimationController controller;
    std::vector<std::shared_ptr<RenderObject>> objects;
    
    // 创建多个对象
    for (int i = 0; i < num_objects; i++) {
        objects.push_back(std::make_shared<RenderObject>(RenderObjectType::BLOCK));
    }
    
    // 创建关键帧
    KeyframesRule keyframes;
    keyframes.name = "multi-animation";

    Keyframe frame0;
    frame0.offset = 0.0f;
    frame0.properties["opacity"] = "0";
    frame0.properties["transform"] = "translateX(0px)";
    keyframes.keyframes.push_back(frame0);

    Keyframe frame50;
    frame50.offset = 0.5f;
    frame50.properties["opacity"] = "1";
    frame50.properties["transform"] = "translateX(100px)";
    keyframes.keyframes.push_back(frame50);

    Keyframe frame100;
    frame100.offset = 1.0f;
    frame100.properties["opacity"] = "0";
    frame100.properties["transform"] = "translateX(200px)";
    keyframes.keyframes.push_back(frame100);

    controller.RegisterKeyframes(keyframes);

    // 启动所有动画
    CSSAnimation config;
    config.name = "multi-animation";
    config.duration = 2.0;
    config.iteration_count = -1;
    config.timing_function = TimingFunction::EASE_IN_OUT;

    for (auto& obj : objects) {
        controller.StartAnimation(obj.get(), config);
    }

    // 测试 1: 无优化
    controller.SetOptimizationEnabled(false);
    PerformanceTimer timer;
    timer.Start();

    for (int i = 0; i < num_frames; i++) {
        controller.Update(i * 0.016);
    }

    double time_no_opt = timer.Stop();
    PrintResult("Without Optimization", time_no_opt, num_frames * num_objects);

    // 重置
    controller.Clear();
    controller.RegisterKeyframes(keyframes);
    for (auto& obj : objects) {
        controller.StartAnimation(obj.get(), config);
    }
    
    // 测试 2: 有优化
    controller.SetOptimizationEnabled(true);
    timer.Start();
    
    for (int i = 0; i < num_frames; i++) {
        controller.Update(i * 0.016);
    }
    
    double time_with_opt = timer.Stop();
    PrintResult("With Optimization", time_with_opt, num_frames * num_objects);
    
    // 计算提升
    double speedup = time_no_opt / time_with_opt;
    std::cout << "\nSpeedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    
    // 打印统计
    auto stats = controller.GetOptimizer().GetStats();
    std::cout << "Cache Hit Rate: " << std::fixed << std::setprecision(1) 
              << (stats.cache_hit_rate * 100) << "%" << std::endl;
}

/**
 * @brief 场景 3: 复杂关键帧动画测试
 */
void BenchmarkComplexKeyframes() {
    PrintHeader("Scenario 3: Complex Keyframes Animation");

    AnimationController controller;
    auto object = std::make_shared<RenderObject>(RenderObjectType::BLOCK);
    
    // 创建复杂的关键帧（10 个关键帧）
    KeyframesRule keyframes;
    keyframes.name = "complex-animation";

    for (int i = 0; i <= 10; i++) {
        Keyframe frame;
        frame.offset = i / 10.0f;
        frame.properties["opacity"] = std::to_string(i / 10.0);
        frame.properties["transform"] = "translateX(" + std::to_string(i * 50) + "px) rotate(" + std::to_string(i * 36) + "deg)";
        frame.properties["filter"] = "blur(" + std::to_string(i) + "px) brightness(" + std::to_string(1.0 + i * 0.1) + ")";
        keyframes.keyframes.push_back(frame);
    }

    controller.RegisterKeyframes(keyframes);

    CSSAnimation config;
    config.name = "complex-animation";
    config.duration = 3.0;
    config.iteration_count = -1;
    config.timing_function = TimingFunction::EASE_IN_OUT;

    controller.StartAnimation(object.get(), config);

    // 测试 1: 无优化
    controller.SetOptimizationEnabled(false);
    PerformanceTimer timer;
    timer.Start();

    const int iterations = 5000;
    for (int i = 0; i < iterations; i++) {
        controller.Update(i * 0.016);
    }

    double time_no_opt = timer.Stop();
    PrintResult("Without Optimization", time_no_opt, iterations);

    // 重置
    controller.Clear();
    controller.RegisterKeyframes(keyframes);
    controller.StartAnimation(object.get(), config);
    
    // 测试 2: 有优化
    controller.SetOptimizationEnabled(true);
    timer.Start();
    
    for (int i = 0; i < iterations; i++) {
        controller.Update(i * 0.016);
    }
    
    double time_with_opt = timer.Stop();
    PrintResult("With Optimization", time_with_opt, iterations);
    
    // 计算提升
    double speedup = time_no_opt / time_with_opt;
    std::cout << "\nSpeedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    
    // 打印统计
    auto stats = controller.GetOptimizer().GetStats();
    std::cout << "Cache Hit Rate: " << std::fixed << std::setprecision(1) 
              << (stats.cache_hit_rate * 100) << "%" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "CSS Animation Performance Benchmarks" << std::endl;
    std::cout << "========================================" << std::endl;
    
    BenchmarkSingleAnimation();
    BenchmarkMultipleAnimations();
    BenchmarkComplexKeyframes();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "All Benchmarks Completed!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}

