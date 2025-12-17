/**
 * @file frame_controller.cpp
 * @brief 帧率控制器实现
 */

#include "frame_controller.h"
#include <algorithm>
#include <numeric>

namespace lightui {

FrameController::FrameController(int target_fps)
    : target_fps_(target_fps)
    , target_frame_time_(1000.0f / target_fps)
    , frame_rate_limit_enabled_(true)
    , use_vsync_(false)  // 默认不使用 VSync，由外部设置
    , frame_start_ticks_(0)
    , performance_frequency_(SDL_GetPerformanceFrequency())
    , current_fps_(0.0f)
    , frame_time_(0.0f)
    , fps_sample_index_(0)
    , total_frames_(0)
    , start_time_(SDL_GetPerformanceCounter())
{
    fps_samples_.resize(FPS_SAMPLE_COUNT, 0.0f);
}

void FrameController::SetTargetFPS(int fps) {
    if (fps < 1 || fps > 1000) {
        return;  // 无效的帧率
    }
    
    target_fps_ = fps;
    target_frame_time_ = 1000.0f / fps;
}

int FrameController::GetTargetFPS() const {
    return target_fps_;
}

void FrameController::BeginFrame() {
    frame_start_ticks_ = SDL_GetPerformanceCounter();
}

void FrameController::EndFrame() {
    Uint64 frame_end_ticks = SDL_GetPerformanceCounter();
    Uint64 elapsed_ticks = frame_end_ticks - frame_start_ticks_;
    
    // 计算帧时间（毫秒）
    frame_time_ = (elapsed_ticks * 1000.0f) / performance_frequency_;
    
    // 更新 FPS 统计
    UpdateFPSStats(frame_time_);
    
    // 帧率限制
    // 关键优化：如果使用 VSync，完全跳过 SDL_Delay
    // VSync 会在 SwapBuffers 时自动同步到显示器刷新率，提供最流畅的体验
    if (!use_vsync_ && frame_rate_limit_enabled_ && frame_time_ < target_frame_time_) {
        // 只在帧时间明显小于目标时间时才延迟（留出一些余量）
        float remaining_time = target_frame_time_ - frame_time_;
        if (remaining_time > 2.0f) {  // 至少剩余 2ms 才延迟
            // 延迟时间减少 1ms，避免过度延迟
            Uint32 delay_ms = static_cast<Uint32>(remaining_time - 1.0f);
            if (delay_ms > 0) {
                SDL_Delay(delay_ms);
                
                // 重新计算实际帧时间
                frame_end_ticks = SDL_GetPerformanceCounter();
                elapsed_ticks = frame_end_ticks - frame_start_ticks_;
                frame_time_ = (elapsed_ticks * 1000.0f) / performance_frequency_;
            }
        }
    }
    
    total_frames_++;
}

void FrameController::SetUseVSync(bool use_vsync) {
    use_vsync_ = use_vsync;
}

float FrameController::GetCurrentFPS() const {
    return current_fps_;
}

float FrameController::GetFrameTime() const {
    return frame_time_;
}

float FrameController::GetDeltaTime() const {
    return frame_time_ / 1000.0f;  // 转换为秒
}

void FrameController::Reset() {
    current_fps_ = 0.0f;
    frame_time_ = 0.0f;
    total_frames_ = 0;
    start_time_ = SDL_GetPerformanceCounter();
    fps_sample_index_ = 0;
    std::fill(fps_samples_.begin(), fps_samples_.end(), 0.0f);
}

void FrameController::SetFrameRateLimitEnabled(bool enabled) {
    frame_rate_limit_enabled_ = enabled;
}

bool FrameController::IsFrameRateLimitEnabled() const {
    return frame_rate_limit_enabled_;
}

void FrameController::UpdateFPSStats(float frame_time) {
    if (frame_time <= 0.0f) {
        return;
    }
    
    // 计算瞬时 FPS
    float instant_fps = 1000.0f / frame_time;
    
    // 添加到采样
    fps_samples_[fps_sample_index_] = instant_fps;
    fps_sample_index_ = (fps_sample_index_ + 1) % FPS_SAMPLE_COUNT;
    
    // 计算平均 FPS（平滑）
    float sum = std::accumulate(fps_samples_.begin(), fps_samples_.end(), 0.0f);
    int valid_samples = 0;
    for (float sample : fps_samples_) {
        if (sample > 0.0f) {
            valid_samples++;
        }
    }
    
    if (valid_samples > 0) {
        current_fps_ = sum / valid_samples;
    }
}

} // namespace lightui

