/**
 * @file test_optimization.cpp
 * @brief 渲染优化测试
 */

#include "core/render/dirty_region.h"
#include "core/render/performance_monitor.h"
#include <iostream>
#include <cassert>
#include <thread>

using namespace lightui;

/**
 * @brief 测试脏区域检测
 */
void TestDirtyRegion() {
    std::cout << "Testing Dirty Region..." << std::endl;
    
    DirtyRegion dirty;
    
    // 测试空状态
    assert(!dirty.IsDirty());
    assert(!dirty.GetBoundingRect().has_value());
    
    // 添加脏区域
    dirty.AddRect(10, 10, 100, 100);
    assert(dirty.IsDirty());
    assert(dirty.GetRegions().size() == 1);
    
    auto bounds = dirty.GetBoundingRect();
    assert(bounds.has_value());
    assert(bounds->fLeft == 10.0f);
    assert(bounds->fTop == 10.0f);
    assert(bounds->width() == 100.0f);
    assert(bounds->height() == 100.0f);
    
    std::cout << "✓ Basic dirty region operations correct!" << std::endl;
    
    // 测试多个脏区域
    dirty.AddRect(200, 200, 50, 50);
    assert(dirty.GetRegions().size() == 2);
    
    bounds = dirty.GetBoundingRect();
    assert(bounds.has_value());
    assert(bounds->fLeft == 10.0f);
    assert(bounds->fTop == 10.0f);
    assert(bounds->fRight == 250.0f);
    assert(bounds->fBottom == 250.0f);
    
    std::cout << "✓ Multiple dirty regions correct!" << std::endl;
    
    // 测试相交检测
    assert(dirty.Intersects(SkRect::MakeXYWH(50, 50, 50, 50)));
    assert(dirty.Intersects(SkRect::MakeXYWH(220, 220, 10, 10)));
    assert(!dirty.Intersects(SkRect::MakeXYWH(500, 500, 10, 10)));
    
    std::cout << "✓ Intersection detection correct!" << std::endl;
    
    // 测试优化（合并相邻区域）
    dirty.Clear();
    dirty.AddRect(0, 0, 100, 100);
    dirty.AddRect(105, 0, 100, 100);  // 距离 5px，应该合并
    
    size_t before_optimize = dirty.GetRegions().size();
    dirty.Optimize();
    size_t after_optimize = dirty.GetRegions().size();
    
    assert(after_optimize < before_optimize);
    std::cout << "✓ Region optimization correct! (Before: " << before_optimize 
              << ", After: " << after_optimize << ")" << std::endl;
    
    // 测试清空
    dirty.Clear();
    assert(!dirty.IsDirty());
    assert(dirty.GetRegions().empty());
    
    std::cout << "✓ Clear operation correct!" << std::endl;
    
    // 测试标记全部
    dirty.MarkAll(800, 600);
    assert(dirty.IsDirty());
    assert(dirty.GetRegions().size() == 1);
    
    bounds = dirty.GetBoundingRect();
    assert(bounds.has_value());
    assert(bounds->width() == 800.0f);
    assert(bounds->height() == 600.0f);
    
    std::cout << "✓ Mark all operation correct!" << std::endl;
}

/**
 * @brief 测试性能监控
 */
void TestPerformanceMonitor() {
    std::cout << "\nTesting Performance Monitor..." << std::endl;
    
    PerformanceMonitor monitor(10);  // 10 个样本
    
    // 模拟几帧
    for (int i = 0; i < 5; ++i) {
        monitor.BeginFrame();
        
        // 模拟布局
        monitor.BeginLayout();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        monitor.EndLayout();
        
        // 模拟绘制
        monitor.BeginPaint();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        monitor.EndPaint();
        
        monitor.EndFrame();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 获取统计数据
    auto stats = monitor.GetStats();
    
    assert(stats.total_frames == 5);
    assert(stats.total_layouts == 5);
    assert(stats.total_paints == 5);
    assert(stats.avg_frame_time > 0.0f);
    assert(stats.avg_layout_time > 0.0f);
    assert(stats.avg_paint_time > 0.0f);
    
    std::cout << "✓ Performance monitoring correct!" << std::endl;
    std::cout << "  Total frames: " << stats.total_frames << std::endl;
    std::cout << "  Avg frame time: " << stats.avg_frame_time << " ms" << std::endl;
    std::cout << "  Avg layout time: " << stats.avg_layout_time << " ms" << std::endl;
    std::cout << "  Avg paint time: " << stats.avg_paint_time << " ms" << std::endl;
    
    // 测试重置
    monitor.Reset();
    stats = monitor.GetStats();
    assert(stats.total_frames == 0);
    assert(stats.total_layouts == 0);
    assert(stats.total_paints == 0);
    
    std::cout << "✓ Reset operation correct!" << std::endl;
    
    // 测试启用/禁用
    monitor.SetEnabled(false);
    assert(!monitor.IsEnabled());
    
    monitor.BeginFrame();
    monitor.EndFrame();
    
    stats = monitor.GetStats();
    assert(stats.total_frames == 0);  // 禁用时不应该计数
    
    monitor.SetEnabled(true);
    assert(monitor.IsEnabled());
    
    std::cout << "✓ Enable/disable correct!" << std::endl;
    
    // 测试内存记录
    monitor.RecordMemoryUsage(1024 * 1024 * 10);  // 10 MB
    stats = monitor.GetStats();
    assert(stats.memory_used == 1024 * 1024 * 10);
    
    std::cout << "✓ Memory recording correct!" << std::endl;
    
    // 输出完整统计
    std::cout << "\n" << stats.ToString() << std::endl;
}

/**
 * @brief 测试性能计时器
 */
void TestPerformanceTimer() {
    std::cout << "\nTesting Performance Timer..." << std::endl;
    
    {
        PerformanceTimer timer("Test Operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        float elapsed = timer.GetElapsed();
        assert(elapsed >= 50.0f);
        assert(elapsed < 100.0f);
    }
    
    std::cout << "✓ Performance timer correct!" << std::endl;
}

/**
 * @brief 主函数
 */
int main() {
    std::cout << "=== Optimization Tests ===" << std::endl;
    std::cout << std::endl;
    
    TestDirtyRegion();
    TestPerformanceMonitor();
    TestPerformanceTimer();
    
    std::cout << std::endl;
    std::cout << "=== All Optimization Tests Passed! ===" << std::endl;
    
    return 0;
}

