/**
 * @file test_animation_controller.cpp
 * @brief CSS 动画控制器测试
 * @author MBink Development Team
 * @date 2025-11-14
 */

#include "core/render/animation_controller.h"
#include "core/render/render_object.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace lightui;

// ============================================================================
// 测试辅助
// ============================================================================

class AnimationControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        controller = std::make_unique<AnimationController>();
        object = new RenderBlock();
        
        // 创建测试用的 @keyframes
        KeyframesRule rule;
        rule.name = "test-anim";
        
        Keyframe kf0;
        kf0.offset = 0.0f;
        kf0.properties["opacity"] = "0";
        rule.AddKeyframe(kf0);
        
        Keyframe kf1;
        kf1.offset = 1.0f;
        kf1.properties["opacity"] = "1";
        rule.AddKeyframe(kf1);
        
        controller->RegisterKeyframes(rule);
    }
    
    void TearDown() override {
        delete object;
    }
    
    std::unique_ptr<AnimationController> controller;
    RenderObject* object;
};

// ============================================================================
// 基础测试
// ============================================================================

TEST_F(AnimationControllerTest, RegisterKeyframes) {
    KeyframesRule rule;
    rule.name = "fade";
    
    Keyframe kf0;
    kf0.offset = 0.0f;
    kf0.properties["opacity"] = "0";
    rule.AddKeyframe(kf0);
    
    Keyframe kf1;
    kf1.offset = 1.0f;
    kf1.properties["opacity"] = "1";
    rule.AddKeyframe(kf1);
    
    controller->RegisterKeyframes(rule);
    
    // 验证可以启动使用该 keyframes 的动画
    CSSAnimation anim;
    anim.name = "fade";
    anim.duration = 1.0f;
    
    controller->StartAnimation(object, anim);
    EXPECT_EQ(controller->GetRunningAnimations().size(), 1);
}

TEST_F(AnimationControllerTest, StartAnimation) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;
    
    controller->StartAnimation(object, anim);
    
    EXPECT_EQ(controller->GetRunningAnimations().size(), 1);
    EXPECT_EQ(controller->GetRunningAnimations()[0].config.name, "test-anim");
    EXPECT_EQ(controller->GetRunningAnimations()[0].object, object);
}

TEST_F(AnimationControllerTest, StartAnimationWithoutKeyframes) {
    CSSAnimation anim;
    anim.name = "nonexistent";
    anim.duration = 1.0f;
    
    controller->StartAnimation(object, anim);
    
    // 应该不会启动
    EXPECT_EQ(controller->GetRunningAnimations().size(), 0);
}

TEST_F(AnimationControllerTest, StopAnimation) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;
    
    controller->StartAnimation(object, anim);
    EXPECT_EQ(controller->GetRunningAnimations().size(), 1);
    
    controller->StopAnimation(object, "test-anim");
    EXPECT_EQ(controller->GetRunningAnimations().size(), 0);
}

TEST_F(AnimationControllerTest, StopAllAnimations) {
    CSSAnimation anim1;
    anim1.name = "test-anim";
    anim1.duration = 1.0f;
    
    controller->StartAnimation(object, anim1);
    
    // 创建第二个 keyframes
    KeyframesRule rule2;
    rule2.name = "test-anim2";
    Keyframe kf0;
    kf0.offset = 0.0f;
    kf0.properties["opacity"] = "0";
    rule2.AddKeyframe(kf0);
    Keyframe kf1;
    kf1.offset = 1.0f;
    kf1.properties["opacity"] = "1";
    rule2.AddKeyframe(kf1);
    controller->RegisterKeyframes(rule2);
    
    CSSAnimation anim2;
    anim2.name = "test-anim2";
    anim2.duration = 2.0f;
    
    controller->StartAnimation(object, anim2);
    
    EXPECT_EQ(controller->GetRunningAnimations().size(), 2);
    
    controller->StopAllAnimations(object);
    EXPECT_EQ(controller->GetRunningAnimations().size(), 0);
}

TEST_F(AnimationControllerTest, PauseAndResumeAnimation) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;

    controller->StartAnimation(object, anim);
    controller->Update(0.0);

    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::RUNNING);

    controller->PauseAnimation(object, "test-anim");
    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::PAUSED);

    controller->ResumeAnimation(object, "test-anim");
    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::RUNNING);
}

// ============================================================================
// 更新测试
// ============================================================================

TEST_F(AnimationControllerTest, UpdateBasic) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;

    controller->StartAnimation(object, anim);

    controller->Update(0.0);
    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::RUNNING);

    controller->Update(0.5);
    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::RUNNING);

    controller->Update(1.5);
    // 动画应该已完成并被移除
    EXPECT_EQ(controller->GetRunningAnimations().size(), 0);
}

TEST_F(AnimationControllerTest, UpdateWithDelay) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;
    anim.delay = 0.5f;

    controller->StartAnimation(object, anim);

    controller->Update(0.0);
    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::DELAYED);

    controller->Update(0.3);
    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::DELAYED);

    controller->Update(0.6);
    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::RUNNING);
}

TEST_F(AnimationControllerTest, UpdateWithIterationCount) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;
    anim.iteration_count = 3;
    
    controller->StartAnimation(object, anim);
    
    controller->Update(0.0);
    EXPECT_EQ(controller->GetRunningAnimations()[0].current_iteration, 0);
    
    controller->Update(1.5);
    EXPECT_EQ(controller->GetRunningAnimations()[0].current_iteration, 1);
    
    controller->Update(2.5);
    EXPECT_EQ(controller->GetRunningAnimations()[0].current_iteration, 2);
    
    controller->Update(3.5);
    // 动画应该已完成
    EXPECT_EQ(controller->GetRunningAnimations().size(), 0);
}

TEST_F(AnimationControllerTest, UpdateWithInfiniteIteration) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;
    anim.iteration_count = -1;  // infinite

    controller->StartAnimation(object, anim);

    controller->Update(0.0);
    controller->Update(1.5);
    controller->Update(10.0);

    // 动画应该一直运行
    EXPECT_EQ(controller->GetRunningAnimations().size(), 1);
    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::RUNNING);
}

TEST_F(AnimationControllerTest, UpdateWithFillModeForwards) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;
    anim.fill_mode = AnimationFillMode::FORWARDS;

    controller->StartAnimation(object, anim);

    controller->Update(0.0);
    controller->Update(1.5);

    // 动画应该保留最后一帧
    EXPECT_EQ(controller->GetRunningAnimations().size(), 1);
    EXPECT_EQ(controller->GetRunningAnimations()[0].state, CSSAnimationState::FINISHED);
}

// ============================================================================
// 属性计算测试
// ============================================================================

TEST_F(AnimationControllerTest, GetCurrentProperties) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;
    
    controller->StartAnimation(object, anim);
    controller->Update(0.0);
    
    auto props = controller->GetCurrentProperties(object, "test-anim");
    EXPECT_TRUE(props.has_value());
    EXPECT_TRUE(props->find("opacity") != props->end());
}

TEST_F(AnimationControllerTest, GetCurrentPropertiesNonexistent) {
    auto props = controller->GetCurrentProperties(object, "nonexistent");
    EXPECT_FALSE(props.has_value());
}

// ============================================================================
// 清除测试
// ============================================================================

TEST_F(AnimationControllerTest, Clear) {
    CSSAnimation anim;
    anim.name = "test-anim";
    anim.duration = 1.0f;
    
    controller->StartAnimation(object, anim);
    EXPECT_EQ(controller->GetRunningAnimations().size(), 1);
    
    controller->Clear();
    EXPECT_EQ(controller->GetRunningAnimations().size(), 0);
}

// ============================================================================
// 性能测试
// ============================================================================

TEST_F(AnimationControllerTest, PerformanceUpdate) {
    // 创建 1000 个动画
    std::vector<RenderObject*> objects;
    for (int i = 0; i < 1000; ++i) {
        RenderObject* obj = new RenderBlock();
        objects.push_back(obj);
        
        CSSAnimation anim;
        anim.name = "test-anim";
        anim.duration = 1.0f;
        anim.iteration_count = -1;  // infinite
        
        controller->StartAnimation(obj, anim);
    }
    
    EXPECT_EQ(controller->GetRunningAnimations().size(), 1000);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    controller->Update(0.5);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Update 1000 animations: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 50);  // 应该小于 50ms
    
    // 清理
    for (auto obj : objects) {
        delete obj;
    }
}

