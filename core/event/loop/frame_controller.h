/**
 * @file frame_controller.h
 * @brief 帧率控制器
 * 
 * 负责控制应用程序的帧率，计算 FPS 和帧时间
 */

#pragma once

#include <SDL3/SDL.h>
#include <vector>

namespace mblink {

/**
 * @brief 帧率控制器类
 * 
 * 功能：
 * 1. 控制目标帧率（默认 60 FPS）
 * 2. 计算当前 FPS
 * 3. 计算帧时间
 * 4. 自动延迟以达到目标帧率
 */
class FrameController {
public:
    /**
     * @brief 构造函数
     * 
     * @param target_fps 目标帧率（默认 60）
     */
    explicit FrameController(int target_fps = 60);
    
    /**
     * @brief 析构函数
     */
    ~FrameController() = default;

    /**
     * @brief 设置目标帧率
     * 
     * @param fps 目标帧率（1-1000）
     */
    void SetTargetFPS(int fps);
    
    /**
     * @brief 获取目标帧率
     * 
     * @return int 目标帧率
     */
    int GetTargetFPS() const;
    
    /**
     * @brief 开始帧
     * 
     * 记录帧开始时间，必须在每帧开始时调用
     */
    void BeginFrame();
    
    /**
     * @brief 结束帧
     * 
     * 计算帧时间和 FPS，如果需要则延迟以达到目标帧率
     * 必须在每帧结束时调用
     */
    void EndFrame();
    
    /**
     * @brief 获取当前 FPS
     * 
     * @return float 当前 FPS（平滑后的值）
     */
    float GetCurrentFPS() const;
    
    /**
     * @brief 获取帧时间
     * 
     * @return float 上一帧的时间（毫秒）
     */
    float GetFrameTime() const;
    
    /**
     * @brief 获取帧时间（秒）
     * 
     * @return float 上一帧的时间（秒）
     */
    float GetDeltaTime() const;
    
    /**
     * @brief 重置统计信息
     */
    void Reset();
    
    /**
     * @brief 启用/禁用帧率限制
     * 
     * @param enabled true 启用帧率限制
     */
    void SetFrameRateLimitEnabled(bool enabled);
    
    /**
     * @brief 检查帧率限制是否启用
     * 
     * @return true 如果启用
     */
    bool IsFrameRateLimitEnabled() const;

    /**
     * @brief 设置是否使用 VSync
     * 
     * 如果启用 VSync，将禁用 SDL_Delay 以避免额外延迟
     * 
     * @param use_vsync true 表示使用 VSync
     */
    void SetUseVSync(bool use_vsync);

private:
    /**
     * @brief 更新 FPS 统计
     * 
     * @param frame_time 当前帧时间（毫秒）
     */
    void UpdateFPSStats(float frame_time);

private:
    int target_fps_;                    // 目标帧率
    float target_frame_time_;           // 目标帧时间（毫秒）
    bool frame_rate_limit_enabled_;     // 是否启用帧率限制
    bool use_vsync_;                    // 是否使用 VSync（如果是，则不使用 SDL_Delay）
    
    Uint64 frame_start_ticks_;          // 帧开始时间（性能计数器）
    Uint64 performance_frequency_;      // 性能计数器频率
    
    float current_fps_;                 // 当前 FPS（平滑后）
    float frame_time_;                  // 上一帧时间（毫秒）
    
    // FPS 平滑
    static constexpr int FPS_SAMPLE_COUNT = 60;  // FPS 采样数量
    std::vector<float> fps_samples_;    // FPS 采样
    int fps_sample_index_;              // 当前采样索引
    
    // 统计信息
    Uint64 total_frames_;               // 总帧数
    Uint64 start_time_;                 // 开始时间
};

} // namespace mblink

