/**
 * @file test_frame_controller.cpp
 * @brief FrameController 单元测试
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "core/event/frame_controller.h"

using namespace lightui;

// 测试夹具
class FrameControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化测试环境
    }

    void TearDown() override {
        // 清理测试环境
    }
};

// 测试构造函数和默认值
TEST_F(FrameControllerTest, Constructor) {
    FrameController controller(60);
    
    EXPECT_EQ(controller.GetTargetFPS(), 60);
    EXPECT_EQ(controller.GetCurrentFPS(), 0.0f);
    EXPECT_EQ(controller.GetFrameTime(), 0.0f);
    EXPECT_TRUE(controller.IsFrameRateLimitEnabled());
}

// 测试设置目标帧率
TEST_F(FrameControllerTest, SetTargetFPS) {
    FrameController controller(60);
    
    controller.SetTargetFPS(30);
    EXPECT_EQ(controller.GetTargetFPS(), 30);
    
    controller.SetTargetFPS(120);
    EXPECT_EQ(controller.GetTargetFPS(), 120);
    
    // 测试无效值
    controller.SetTargetFPS(0);
    EXPECT_EQ(controller.GetTargetFPS(), 120);  // 应该保持不变
    
    controller.SetTargetFPS(1001);
    EXPECT_EQ(controller.GetTargetFPS(), 120);  // 应该保持不变
}

// 测试帧时间计算
TEST_F(FrameControllerTest, FrameTime) {
    FrameController controller(60);
    controller.SetFrameRateLimitEnabled(false);  // 禁用帧率限制以便测试
    
    controller.BeginFrame();
    
    // 模拟一些工作
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    controller.EndFrame();

    float frame_time = controller.GetFrameTime();
    EXPECT_GT(frame_time, 9.0f);   // 至少 9ms
    EXPECT_LT(frame_time, 30.0f);  // 不超过 30ms（考虑系统调度误差）
}

// 测试 Delta Time
TEST_F(FrameControllerTest, DeltaTime) {
    FrameController controller(60);
    controller.SetFrameRateLimitEnabled(false);
    
    controller.BeginFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    controller.EndFrame();
    
    float delta_time = controller.GetDeltaTime();
    EXPECT_GT(delta_time, 0.009f);   // 至少 0.009 秒
    EXPECT_LT(delta_time, 0.020f);   // 不超过 0.020 秒
}

// 测试 FPS 计算
TEST_F(FrameControllerTest, FPSCalculation) {
    FrameController controller(60);
    controller.SetFrameRateLimitEnabled(false);
    
    // 运行多帧以获得稳定的 FPS
    for (int i = 0; i < 10; i++) {
        controller.BeginFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
        controller.EndFrame();
    }
    
    float fps = controller.GetCurrentFPS();
    EXPECT_GT(fps, 40.0f);   // 应该接近 60 FPS（放宽范围，考虑系统负载）
    EXPECT_LT(fps, 80.0f);
}

// 测试帧率限制
TEST_F(FrameControllerTest, FrameRateLimit) {
    FrameController controller(60);
    controller.SetFrameRateLimitEnabled(true);
    
    auto start = std::chrono::steady_clock::now();
    
    // 运行 10 帧
    for (int i = 0; i < 10; i++) {
        controller.BeginFrame();
        // 不做任何工作，让帧率限制生效
        controller.EndFrame();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // 10 帧 @ 60 FPS = 166.67ms
    EXPECT_GT(duration, 150);  // 至少 150ms
    EXPECT_LT(duration, 200);  // 不超过 200ms
}

// 测试启用/禁用帧率限制
TEST_F(FrameControllerTest, EnableDisableFrameRateLimit) {
    FrameController controller(60);
    
    EXPECT_TRUE(controller.IsFrameRateLimitEnabled());
    
    controller.SetFrameRateLimitEnabled(false);
    EXPECT_FALSE(controller.IsFrameRateLimitEnabled());
    
    controller.SetFrameRateLimitEnabled(true);
    EXPECT_TRUE(controller.IsFrameRateLimitEnabled());
}

// 测试重置
TEST_F(FrameControllerTest, Reset) {
    FrameController controller(60);
    controller.SetFrameRateLimitEnabled(false);
    
    // 运行几帧
    for (int i = 0; i < 5; i++) {
        controller.BeginFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        controller.EndFrame();
    }
    
    EXPECT_GT(controller.GetCurrentFPS(), 0.0f);
    EXPECT_GT(controller.GetFrameTime(), 0.0f);
    
    // 重置
    controller.Reset();
    
    EXPECT_EQ(controller.GetCurrentFPS(), 0.0f);
    EXPECT_EQ(controller.GetFrameTime(), 0.0f);
}

// 测试不同目标帧率
TEST_F(FrameControllerTest, DifferentTargetFPS) {
    // 测试 30 FPS
    {
        FrameController controller(30);
        controller.SetFrameRateLimitEnabled(true);
        
        auto start = std::chrono::steady_clock::now();
        
        for (int i = 0; i < 5; i++) {
            controller.BeginFrame();
            controller.EndFrame();
        }
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        // 5 帧 @ 30 FPS = 166.67ms
        EXPECT_GT(duration, 150);
        EXPECT_LT(duration, 200);
    }
    
    // 测试 120 FPS
    {
        FrameController controller(120);
        controller.SetFrameRateLimitEnabled(true);
        
        auto start = std::chrono::steady_clock::now();
        
        for (int i = 0; i < 10; i++) {
            controller.BeginFrame();
            controller.EndFrame();
        }
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        // 10 帧 @ 120 FPS = 83.33ms
        EXPECT_GT(duration, 70);
        EXPECT_LT(duration, 100);
    }
}

// 测试 FPS 平滑
TEST_F(FrameControllerTest, FPSSmoothing) {
    FrameController controller(60);
    controller.SetFrameRateLimitEnabled(false);
    
    // 运行一些帧，帧时间有波动
    for (int i = 0; i < 20; i++) {
        controller.BeginFrame();
        
        // 交替使用不同的延迟
        if (i % 2 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(14));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(18));
        }
        
        controller.EndFrame();
    }
    
    float fps = controller.GetCurrentFPS();

    // FPS 应该被平滑，接近平均值（放宽范围）
    EXPECT_GT(fps, 45.0f);
    EXPECT_LT(fps, 75.0f);
}

