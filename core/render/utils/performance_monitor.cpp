/**
 * @file performance_monitor.cpp
 * @brief 渲染性能监控系统实现
 */

#include "performance_monitor.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <algorithm>

namespace mbink {

// ========== PerformanceStats ==========

std::string PerformanceStats::ToString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Performance Stats:\n";
    oss << "  FPS: " << fps << "\n";
    oss << "  Frame Time: avg=" << avg_frame_time << "ms"
        << ", min=" << min_frame_time << "ms"
        << ", max=" << max_frame_time << "ms\n";
    oss << "  Layout Time: avg=" << avg_layout_time << "ms\n";
    oss << "  Paint Time: avg=" << avg_paint_time << "ms\n";
    oss << "  Total Frames: " << total_frames << "\n";
    oss << "  Total Layouts: " << total_layouts << "\n";
    oss << "  Total Paints: " << total_paints << "\n";
    oss << "  Memory Used: " << (memory_used / 1024.0 / 1024.0) << " MB";
    return oss.str();
}

// ========== PerformanceMonitor ==========

PerformanceMonitor::PerformanceMonitor(size_t sample_size)
    : sample_size_(sample_size) {
    fps_start_ = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::BeginFrame() {
    if (!enabled_) return;
    frame_start_ = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::EndFrame() {
    if (!enabled_) return;
    
    auto end = std::chrono::high_resolution_clock::now();
    float frame_time = CalculateDuration(frame_start_, end);
    
    AddSample(frame_times_, frame_time);
    total_frames_++;
    fps_frame_count_++;
    
    // 每秒更新一次 FPS
    auto fps_duration = CalculateDuration(fps_start_, end);
    if (fps_duration >= 1000.0f) {
        current_fps_ = fps_frame_count_ / (fps_duration / 1000.0f);
        fps_frame_count_ = 0;
        fps_start_ = end;
    }
}

void PerformanceMonitor::BeginLayout() {
    if (!enabled_) return;
    layout_start_ = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::EndLayout() {
    if (!enabled_) return;
    
    auto end = std::chrono::high_resolution_clock::now();
    float layout_time = CalculateDuration(layout_start_, end);
    
    AddSample(layout_times_, layout_time);
    total_layouts_++;
}

void PerformanceMonitor::BeginPaint() {
    if (!enabled_) return;
    paint_start_ = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::EndPaint() {
    if (!enabled_) return;
    
    auto end = std::chrono::high_resolution_clock::now();
    float paint_time = CalculateDuration(paint_start_, end);
    
    AddSample(paint_times_, paint_time);
    total_paints_++;
}

void PerformanceMonitor::RecordMemoryUsage(size_t bytes) {
    if (!enabled_) return;
    memory_used_ = bytes;
}

PerformanceStats PerformanceMonitor::GetStats() const {
    PerformanceStats stats;
    
    stats.fps = current_fps_;
    stats.avg_frame_time = CalculateAverage(frame_times_);
    
    if (!frame_times_.empty()) {
        stats.min_frame_time = *std::min_element(frame_times_.begin(), frame_times_.end());
        stats.max_frame_time = *std::max_element(frame_times_.begin(), frame_times_.end());
    }
    
    stats.avg_layout_time = CalculateAverage(layout_times_);
    stats.avg_paint_time = CalculateAverage(paint_times_);
    
    stats.total_frames = total_frames_;
    stats.total_layouts = total_layouts_;
    stats.total_paints = total_paints_;
    stats.memory_used = memory_used_;
    
    return stats;
}

void PerformanceMonitor::Reset() {
    frame_times_.clear();
    layout_times_.clear();
    paint_times_.clear();
    
    total_frames_ = 0;
    total_layouts_ = 0;
    total_paints_ = 0;
    memory_used_ = 0;
    
    fps_frame_count_ = 0;
    current_fps_ = 0.0f;
    fps_start_ = std::chrono::high_resolution_clock::now();
}

float PerformanceMonitor::GetFPS() const {
    return current_fps_;
}

float PerformanceMonitor::GetAverageFrameTime() const {
    return CalculateAverage(frame_times_);
}

float PerformanceMonitor::CalculateDuration(const TimePoint& start, const TimePoint& end) const {
    return std::chrono::duration_cast<Duration>(end - start).count();
}

float PerformanceMonitor::CalculateAverage(const std::deque<float>& values) const {
    if (values.empty()) {
        return 0.0f;
    }
    
    float sum = std::accumulate(values.begin(), values.end(), 0.0f);
    return sum / values.size();
}

void PerformanceMonitor::AddSample(std::deque<float>& samples, float value) {
    samples.push_back(value);
    
    // 保持样本数量在限制内
    while (samples.size() > sample_size_) {
        samples.pop_front();
    }
}

// ========== PerformanceTimer ==========

PerformanceTimer::PerformanceTimer(const std::string& name)
    : name_(name) {
    start_ = std::chrono::high_resolution_clock::now();
}

PerformanceTimer::~PerformanceTimer() {
    float elapsed = GetElapsed();
}

float PerformanceTimer::GetElapsed() const {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(end - start_).count();
}

} // namespace mbink

