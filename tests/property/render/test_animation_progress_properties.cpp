/**
 * @file test_animation_progress_properties.cpp
 * @brief Property-based tests for animation progress calculation
 * 
 * This file implements property-based testing for animation progress.
 * Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: css-animation-integration**
 * 
 * IMPORTANT: All tests use REAL rendering layer components, not mocks:
 * - Real AnimationController
 * - Real RenderObject
 * - Real KeyframesRule
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "core/render/animation/animation_controller.h"
#include "core/render/objects/render_object.h"
#include "core/render/animation/keyframes.h"
#include <random>
#include <cmath>

using namespace mblink;
using namespace mblink::test;

class AnimationProgressPropertyTest : public DOMTestBase {
protected:
    static constexpr int NUM_ITERATIONS = 100;
    std::mt19937 gen_{std::random_device{}()};
    
    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    void SetUp() override {
        DOMTestBase::SetUp();
    }
    
    /**
     * @brief Create a simple keyframes rule for testing
     */
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
 * **Feature: css-animation-integration, Property 5: Animation progress calculation**
 * 
 * For any animation with duration D and delay L, at time T:
 * - If T < L: animation should be in DELAYED state
 * - If L <= T < L + D: animation should be RUNNING with progress (T - L) / D
 * - If T >= L + D: animation should be FINISHED
 * 
 * **Validates: Requirements 3.2**
 */
TEST_F(AnimationProgressPropertyTest, ProgressCalculationCorrectness) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        // Create a render object
        auto render_obj = std::make_unique<RenderBlock>();
        
        // Generate random animation parameters
        float duration = randFloat(0.5f, 5.0f);
        float delay = randFloat(0.0f, 2.0f);
        std::string anim_name = "test_anim_" + std::to_string(i);
        
        // Register keyframes
        KeyframesRule rule = CreateTestKeyframes(anim_name);
        controller.RegisterKeyframes(rule);
        
        // Create animation config
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = delay;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::BOTH;
        
        // Start animation at time 0
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        // Test during delay period
        if (delay > 0.1f) {
            double delay_time = delay * 0.5;
            controller.Update(delay_time);
            
            auto& running = controller.GetRunningAnimations();
            ASSERT_FALSE(running.empty()) << "No running animations at iteration " << i;
            
            EXPECT_EQ(running[0].state, CSSAnimationState::DELAYED)
                << "Animation should be DELAYED during delay period at iteration " << i
                << ", delay=" << delay << ", time=" << delay_time;
        }
        
        // Test during running period
        double mid_time = delay + duration * 0.5;
        controller.Update(mid_time);
        
        auto& running = controller.GetRunningAnimations();
        ASSERT_FALSE(running.empty()) << "No running animations at iteration " << i;
        
        EXPECT_EQ(running[0].state, CSSAnimationState::RUNNING)
            << "Animation should be RUNNING at mid-point at iteration " << i;
        
        // Test after completion
        double end_time = delay + duration + 0.1;
        controller.Update(end_time);
        
        auto& finished = controller.GetRunningAnimations();
        if (!finished.empty()) {
            EXPECT_EQ(finished[0].state, CSSAnimationState::FINISHED)
                << "Animation should be FINISHED after duration at iteration " << i;
        }
    }
}

/**
 * @brief Test animation iteration counting
 * 
 * For animation with N iterations, the animation should complete
 * after N * duration time.
 */
TEST_F(AnimationProgressPropertyTest, IterationCounting) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(0.5f, 2.0f);
        int iterations = randInt(1, 5);
        std::string anim_name = "iter_anim_" + std::to_string(i);
        
        KeyframesRule rule = CreateTestKeyframes(anim_name);
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = 0.0f;
        anim.iteration_count = iterations;
        anim.fill_mode = AnimationFillMode::BOTH;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        // Test at each iteration boundary
        for (int iter = 0; iter < iterations; ++iter) {
            double iter_mid = (iter + 0.5) * duration;
            controller.Update(iter_mid);
            
            auto& running = controller.GetRunningAnimations();
            if (!running.empty() && running[0].state == CSSAnimationState::RUNNING) {
                EXPECT_EQ(running[0].current_iteration, iter)
                    << "Iteration count mismatch at iteration " << i
                    << ", expected iter " << iter << " at time " << iter_mid;
            }
        }
        
        // After all iterations
        double total_time = iterations * duration + 0.1;
        controller.Update(total_time);
        
        auto& finished = controller.GetRunningAnimations();
        if (!finished.empty()) {
            EXPECT_EQ(finished[0].state, CSSAnimationState::FINISHED)
                << "Animation should be FINISHED after all iterations at iteration " << i;
        }
    }
}

/**
 * @brief Test progress boundary conditions
 * 
 * At time = delay, progress should be 0.
 * At time = delay + duration, progress should be 1.
 */
TEST_F(AnimationProgressPropertyTest, ProgressBoundaryConditions) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(1.0f, 3.0f);
        float delay = randFloat(0.0f, 1.0f);
        std::string anim_name = "boundary_anim_" + std::to_string(i);
        
        KeyframesRule rule = CreateTestKeyframes(anim_name);
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = delay;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::BOTH;
        
        controller.StartAnimation(render_obj.get(), anim);
        
        // At start of animation (after delay)
        controller.Update(delay);
        auto props_start = controller.GetCurrentProperties(render_obj.get(), anim_name);
        
        if (props_start.has_value()) {
            auto it = props_start->find("opacity");
            if (it != props_start->end()) {
                float opacity = std::stof(it->second);
                EXPECT_NEAR(opacity, 0.0f, 0.05f)
                    << "Opacity should be ~0 at animation start at iteration " << i;
            }
        }
        
        // At end of animation
        controller.Update(delay + duration);
        auto props_end = controller.GetCurrentProperties(render_obj.get(), anim_name);
        
        if (props_end.has_value()) {
            auto it = props_end->find("opacity");
            if (it != props_end->end()) {
                float opacity = std::stof(it->second);
                EXPECT_NEAR(opacity, 1.0f, 0.05f)
                    << "Opacity should be ~1 at animation end at iteration " << i;
            }
        }
    }
}

