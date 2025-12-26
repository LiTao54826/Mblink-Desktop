/**
 * @file test_animation_playstate_properties.cpp
 * @brief Property-based tests for CSS animation play-state control
 * 
 * This file implements property-based testing for animation pause/resume behavior.
 * 
 * **Feature: css-animation-integration**
 * 
 * IMPORTANT: All tests use REAL rendering layer components, not mocks:
 * - Real AnimationController
 * - Real RenderObject
 * - Real pause/resume functionality
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "render/animation/animation_controller.h"
#include "render/render_object.h"
#include "render/animation/keyframes.h"
#include <random>
#include <cmath>

using namespace lightui;
using namespace lightui::test;

class AnimationPlayStatePropertyTest : public DOMTestBase {
protected:
    static constexpr int NUM_ITERATIONS = 100;
    std::mt19937 gen_{std::random_device{}()};
    
    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }
    
    void SetUp() override {
        DOMTestBase::SetUp();
    }
    
    KeyframesRule CreateTestKeyframes(const std::string& name) {
        KeyframesRule rule;
        rule.name = name;
        
        Keyframe from_kf;
        from_kf.offset = 0.0f;
        from_kf.properties["opacity"] = "0";
        rule.keyframes.push_back(from_kf);
        
        Keyframe to_kf;
        to_kf.offset = 1.0f;
        to_kf.properties["opacity"] = "1";
        rule.keyframes.push_back(to_kf);
        
        return rule;
    }
};

/**
 * **Feature: css-animation-integration, Property 13: Paused state preservation**
 * 
 * When an animation is paused, its progress should remain constant
 * regardless of how much time passes.
 * 
 * **Validates: Requirements 7.1**
 */
TEST_F(AnimationPlayStatePropertyTest, PausedStatePreservesProgress) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(2.0f, 5.0f);
        float pause_time = randFloat(0.2f, 0.8f) * duration;
        std::string anim_name = "pause_anim_" + std::to_string(i);
        
        KeyframesRule rule = CreateTestKeyframes(anim_name);
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = 0.0f;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::BOTH;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        // Run to pause point
        controller.Update(pause_time);
        
        // Get properties before pause
        auto props_before = controller.GetCurrentProperties(render_obj.get(), anim_name);
        ASSERT_TRUE(props_before.has_value())
            << "Should have properties before pause at iteration " << i;
        
        float opacity_before = std::stof((*props_before)["opacity"]);
        
        // Pause the animation
        controller.PauseAnimation(render_obj.get(), anim_name);
        
        // Verify state is PAUSED
        auto& running = controller.GetRunningAnimations();
        ASSERT_FALSE(running.empty());
        EXPECT_EQ(running[0].state, CSSAnimationState::PAUSED)
            << "Animation should be PAUSED at iteration " << i;
        
        // Advance time significantly
        controller.Update(pause_time + duration * 2);
        
        // Get properties after time passes
        auto props_after = controller.GetCurrentProperties(render_obj.get(), anim_name);
        ASSERT_TRUE(props_after.has_value())
            << "Should have properties after pause at iteration " << i;
        
        float opacity_after = std::stof((*props_after)["opacity"]);
        
        // Progress should be preserved
        EXPECT_NEAR(opacity_after, opacity_before, 0.01f)
            << "Paused animation progress should be preserved at iteration " << i
            << ", before: " << opacity_before << ", after: " << opacity_after;
    }
}

/**
 * **Feature: css-animation-integration, Property 14: Resume position continuity**
 * 
 * When a paused animation is resumed, it should continue from
 * exactly where it was paused.
 * 
 * **Validates: Requirements 7.2**
 */
TEST_F(AnimationPlayStatePropertyTest, ResumePositionContinuity) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(2.0f, 5.0f);
        float pause_time = randFloat(0.2f, 0.5f) * duration;
        float resume_delay = randFloat(1.0f, 3.0f);
        std::string anim_name = "resume_anim_" + std::to_string(i);
        
        KeyframesRule rule = CreateTestKeyframes(anim_name);
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = 0.0f;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::BOTH;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        // Run to pause point
        controller.Update(pause_time);
        
        auto props_at_pause = controller.GetCurrentProperties(render_obj.get(), anim_name);
        ASSERT_TRUE(props_at_pause.has_value());
        float opacity_at_pause = std::stof((*props_at_pause)["opacity"]);
        
        // Pause
        controller.PauseAnimation(render_obj.get(), anim_name);
        
        // Wait some time
        double current_time = pause_time + resume_delay;
        controller.Update(current_time);
        
        // Resume
        controller.ResumeAnimation(render_obj.get(), anim_name);
        
        // Verify state is RUNNING again
        auto& running = controller.GetRunningAnimations();
        ASSERT_FALSE(running.empty());
        EXPECT_EQ(running[0].state, CSSAnimationState::RUNNING)
            << "Animation should be RUNNING after resume at iteration " << i;
        
        // Get properties immediately after resume
        auto props_after_resume = controller.GetCurrentProperties(render_obj.get(), anim_name);
        ASSERT_TRUE(props_after_resume.has_value());
        float opacity_after_resume = std::stof((*props_after_resume)["opacity"]);
        
        // Should continue from pause position
        EXPECT_NEAR(opacity_after_resume, opacity_at_pause, 0.05f)
            << "Animation should resume from pause position at iteration " << i;
        
        // Advance a bit more and verify animation continues
        float additional_time = 0.2f * duration;
        controller.Update(current_time + additional_time);
        
        auto props_later = controller.GetCurrentProperties(render_obj.get(), anim_name);
        ASSERT_TRUE(props_later.has_value());
        float opacity_later = std::stof((*props_later)["opacity"]);
        
        // Should have progressed from resume point
        EXPECT_GT(opacity_later, opacity_after_resume - 0.01f)
            << "Animation should progress after resume at iteration " << i;
    }
}

/**
 * @brief Test multiple pause/resume cycles
 */
TEST_F(AnimationPlayStatePropertyTest, MultiplePauseResumeCycles) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(3.0f, 6.0f);
        std::string anim_name = "multi_pause_anim_" + std::to_string(i);
        
        KeyframesRule rule = CreateTestKeyframes(anim_name);
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = 0.0f;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::BOTH;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        double current_time = 0.0;
        float last_opacity = 0.0f;
        
        // Perform 3 pause/resume cycles
        for (int cycle = 0; cycle < 3; ++cycle) {
            // Run for a bit
            current_time += duration * 0.1;
            controller.Update(current_time);
            
            auto props = controller.GetCurrentProperties(render_obj.get(), anim_name);
            if (props.has_value()) {
                float current_opacity = std::stof((*props)["opacity"]);
                
                // Opacity should increase (animation progressing)
                EXPECT_GE(current_opacity, last_opacity - 0.01f)
                    << "Opacity should not decrease during cycle " << cycle 
                    << " at iteration " << i;
                
                last_opacity = current_opacity;
            }
            
            // Pause
            controller.PauseAnimation(render_obj.get(), anim_name);
            
            // Wait
            current_time += 0.5;
            controller.Update(current_time);
            
            // Resume
            controller.ResumeAnimation(render_obj.get(), anim_name);
        }
        
        // Animation should still be valid
        auto& running = controller.GetRunningAnimations();
        EXPECT_FALSE(running.empty())
            << "Animation should still exist after multiple cycles at iteration " << i;
    }
}

/**
 * @brief Test pause during delay period
 */
TEST_F(AnimationPlayStatePropertyTest, PauseDuringDelay) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(1.0f, 3.0f);
        float delay = randFloat(1.0f, 2.0f);
        std::string anim_name = "delay_pause_anim_" + std::to_string(i);
        
        KeyframesRule rule = CreateTestKeyframes(anim_name);
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = delay;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::BOTH;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        // Pause during delay
        controller.Update(delay * 0.5);
        controller.PauseAnimation(render_obj.get(), anim_name);
        
        auto& running = controller.GetRunningAnimations();
        ASSERT_FALSE(running.empty());
        EXPECT_EQ(running[0].state, CSSAnimationState::PAUSED)
            << "Animation should be PAUSED during delay at iteration " << i;
        
        // Resume
        controller.ResumeAnimation(render_obj.get(), anim_name);
        
        // Should eventually start running after delay completes
        controller.Update(delay + duration * 0.5);
        
        auto& running_later = controller.GetRunningAnimations();
        if (!running_later.empty()) {
            // Should be either RUNNING or FINISHED
            EXPECT_TRUE(running_later[0].state == CSSAnimationState::RUNNING ||
                       running_later[0].state == CSSAnimationState::FINISHED)
                << "Animation should be RUNNING or FINISHED after delay at iteration " << i;
        }
    }
}

/**
 * @brief Test that pausing non-existent animation is safe
 */
TEST_F(AnimationPlayStatePropertyTest, PauseNonExistentAnimationIsSafe) {
    auto doc = CreateDocument();
    AnimationController controller;
    
    auto render_obj = std::make_unique<RenderBlock>();
    
    // Should not crash
    controller.PauseAnimation(render_obj.get(), "non_existent_animation");
    controller.ResumeAnimation(render_obj.get(), "non_existent_animation");
    
    SUCCEED() << "Pausing/resuming non-existent animation should be safe";
}

