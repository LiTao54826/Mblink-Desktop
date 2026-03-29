/**
 * @file performance_monitor.h
 * @brief 渲染性能监控系统
 *
 * 功能：
 * - 监控 FPS (帧率)
 * - 监控渲染时间
 * - 监控内存使用
 * - 提供性能报告
 */

#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <deque>

namespace mbink {

/**
 * @brief 性能统计数据
 */
struct PerformanceStats {
    float fps = 0.0f;                    ///< 帧率 (frames per second)
    float avg_frame_time = 0.0f;         ///< 平均帧时间 (ms)
    float min_frame_time = 0.0f;         ///< 最小帧时间 (ms)
    float max_frame_time = 0.0f;         ///< 最大帧时间 (ms)
    
    float avg_layout_time = 0.0f;        ///< 平均布局时间 (ms)
    float avg_paint_time = 0.0f;         ///< 平均绘制时间 (ms)
    
    size_t total_frames = 0;             ///< 总帧数
    size_t total_layouts = 0;            ///< 总布局次数
    size_t total_paints = 0;             ///< 总绘制次数
    
    size_t memory_used = 0;              ///< 内存使用 (bytes)
    
    /**
     * @brief 转换为字符串
     * @return 性能统计字符串
     */
    std::string ToString() const;
};

/**
 * @brief 性能监控器
 *
 * 监控渲染性能，提供性能统计和报告
 */
class PerformanceMonitor {
public:
    /**
     * @brief 构造函数
     * @param sample_size 采样大小（用于计算平均值）
     */
    explicit PerformanceMonitor(size_t sample_size = 60);

    /**
     * @brief 开始帧计时
     */
    void BeginFrame();

    /**
     * @brief 结束帧计时
     */
    void EndFrame();

    /**
     * @brief 开始布局计时
     */
    void BeginLayout();

    /**
     * @brief 结束布局计时
     */
    void EndLayout();

    /**
     * @brief 开始绘制计时
     */
    void BeginPaint();

    /**
     * @brief 结束绘制计时
     */
    void EndPaint();

    /**
     * @brief 记录内存使用
     * @param bytes 内存字节数
     */
    void RecordMemoryUsage(size_t bytes);

    /**
     * @brief 获取性能统计
     * @return 性能统计数据
     */
    PerformanceStats GetStats() const;

    /**
     * @brief 重置统计数据
     */
    void Reset();

    /**
     * @brief 获取当前 FPS
     * @return FPS 值
     */
    float GetFPS() const;

    /**
     * @brief 获取平均帧时间
     * @return 平均帧时间 (ms)
     */
    float GetAverageFrameTime() const;

    /**
     * @brief 启用/禁用监控
     * @param enabled 是否启用
     */
    void SetEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief 检查是否启用
     * @return 是否启用
     */
    bool IsEnabled() const { return enabled_; }

private:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::duration<float, std::milli>;

    /**
     * @brief 计算持续时间（毫秒）
     * @param start 开始时间点
     * @param end 结束时间点
     * @return 持续时间 (ms)
     */
    float CalculateDuration(const TimePoint& start, const TimePoint& end) const;

    /**
     * @brief 计算平均值
     * @param values 值列表
     * @return 平均值
     */
    float CalculateAverage(const std::deque<float>& values) const;

    /**
     * @brief 添加样本
     * @param samples 样本队列
     * @param value 新值
     */
    void AddSample(std::deque<float>& samples, float value);

    bool enabled_ = true;                ///< 是否启用监控
    size_t sample_size_;                 ///< 采样大小

    // 帧计时
    TimePoint frame_start_;              ///< 帧开始时间
    std::deque<float> frame_times_;      ///< 帧时间样本

    // 布局计时
    TimePoint layout_start_;             ///< 布局开始时间
    std::deque<float> layout_times_;     ///< 布局时间样本

    // 绘制计时
    TimePoint paint_start_;              ///< 绘制开始时间
    std::deque<float> paint_times_;      ///< 绘制时间样本

    // 统计数据
    size_t total_frames_ = 0;            ///< 总帧数
    size_t total_layouts_ = 0;           ///< 总布局次数
    size_t total_paints_ = 0;            ///< 总绘制次数
    size_t memory_used_ = 0;             ///< 内存使用

    // FPS 计算
    TimePoint fps_start_;                ///< FPS 计算开始时间
    size_t fps_frame_count_ = 0;         ///< FPS 帧计数
    float current_fps_ = 0.0f;           ///< 当前 FPS
};

/**
 * @brief 性能计时器（RAII）
 *
 * 自动计时的辅助类
 */
class PerformanceTimer {
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;

    /**
     * @brief 构造函数
     * @param name 计时器名称
     */
    explicit PerformanceTimer(const std::string& name);

    /**
     * @brief 析构函数（自动输出计时结果）
     */
    ~PerformanceTimer();

    /**
     * @brief 获取经过的时间
     * @return 经过的时间 (ms)
     */
    float GetElapsed() const;

private:
    std::string name_;                   ///< 计时器名称
    TimePoint start_;                    ///< 开始时间
};

} // namespace mbink

