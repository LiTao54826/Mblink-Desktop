/**
 * @file test_animation_fillmode_properties.cpp
 * @brief Property-based tests for CSS animation fill-mode
 * 
 * This file implements property-based testing for animation fill-mode behavior.
 * 
 * **Feature: css-animation-integration**
 * 
 * IMPORTANT: All tests use REAL rendering layer components, not mocks:
 * - Real AnimationController
 * - Real RenderObject
 * - Real fill-mode handling
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "render/animation_controller.h"
#include "render/render_object.h"
#include "render/keyframes.h"
#include <random>
#include <cmath>

using namespace lightui;
using namespace lightui::test;

class AnimationFillModePropertyTest : public DOMTestBase {
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
    
    KeyframesRule CreateOpacityKeyframes(const std::string& name, 
                                          const std::string& from_val,
                                          const std::string& to_val) {
        KeyframesRule rule;
        rule.name = name;
        
        Keyframe from_kf;
        from_kf.offset = 0.0f;
        from_kf.properties["opacity"] = from_val;
        rule.keyframes.push_back(from_kf);
        
        Keyframe to_kf;
        to_kf.offset = 1.0f;
        to_kf.properties["opacity"] = to_val;
        rule.keyframes.push_back(to_kf);
        
        return rule;
    }
};

/**
 * **Feature: css-animation-integration, Property 11: Fill-mode forwards behavior**
 * 
 * With fill-mode: forwards or both, after animation completes,
 * the element should retain the final keyframe values.
 * 
 * **Validates: Requirements 6.1, 6.3**
 */
TEST_F(AnimationFillModePropertyTest, ForwardsFillModeRetainsLastFrame) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(0.5f, 2.0f);
        float final_opacity = randFloat(0.5f, 1.0f);
        std::string anim_name = "forwards_anim_" + std::to_string(i);
        
        // Create keyframes with specific final value
        std::ostringstream to_val;
        to_val << final_opacity;
        KeyframesRule rule = CreateOpacityKeyframes(anim_name, "0", to_val.str());
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = 0.0f;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::FORWARDS;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        // Complete the animation
        controller.Update(duration + 0.5);
        
        // Get properties after animation ends
        auto props = controller.GetCurrentProperties(render_obj.get(), anim_name);
        
        ASSERT_TRUE(props.has_value())
            << "fill-mode: forwards should return properties after completion at iteration " << i;
        
        auto it = props->find("opacity");
        ASSERT_NE(it, props->end())
            << "opacity should be in properties at iteration " << i;
        
        float result_opacity = std::stof(it->second);
        EXPECT_NEAR(result_opacity, final_opacity, 0.05f)
            << "fill-mode: forwards should retain final opacity at iteration " << i
            << ", expected: " << final_opacity << ", got: " << result_opacity;
    }
}

/**
 * @brief Test fill-mode: both retains last frame after completion
 */
TEST_F(AnimationFillModePropertyTest, BothFillModeRetainsLastFrame) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(0.5f, 2.0f);
        float final_opacity = randFloat(0.5f, 1.0f);
        std::string anim_name = "both_anim_" + std::to_string(i);
        
        std::ostringstream to_val;
        to_val << final_opacity;
        KeyframesRule rule = CreateOpacityKeyframes(anim_name, "0", to_val.str());
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = 0.0f;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::BOTH;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        controller.Update(duration + 0.5);
        
        auto props = controller.GetCurrentProperties(render_obj.get(), anim_name);
        
        ASSERT_TRUE(props.has_value())
            << "fill-mode: both should return properties after completion at iteration " << i;
        
        auto it = props->find("opacity");
        ASSERT_NE(it, props->end());
        
        float result_opacity = std::stof(it->second);
        EXPECT_NEAR(result_opacity, final_opacity, 0.05f)
            << "fill-mode: both should retain final opacity at iteration " << i;
    }
}

/**
 * **Feature: css-animation-integration, Property 12: Fill-mode backwards behavior**
 * 
 * With fill-mode: backwards or both, during the delay period,
 * the element should have the first keyframe values applied.
 * 
 * **Validates: Requirements 6.2, 6.3**
 */
TEST_F(AnimationFillModePropertyTest, BackwardsFillModeAppliesFirstFrame) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(0.5f, 2.0f);
        float delay = randFloat(0.5f, 2.0f);
        float initial_opacity = randFloat(0.0f, 0.5f);
        std::string anim_name = "backwards_anim_" + std::to_string(i);
        
        std::ostringstream from_val;
        from_val << initial_opacity;
        KeyframesRule rule = CreateOpacityKeyframes(anim_name, from_val.str(), "1");
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = delay;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::BACKWARDS;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        // During delay period
        controller.Update(delay * 0.5);
        
        auto props = controller.GetCurrentProperties(render_obj.get(), anim_name);
        
        ASSERT_TRUE(props.has_value())
            << "fill-mode: backwards should return properties during delay at iteration " << i;
        
        auto it = props->find("opacity");
        ASSERT_NE(it, props->end())
            << "opacity should be in properties at iteration " << i;
        
        float result_opacity = std::stof(it->second);
        EXPECT_NEAR(result_opacity, initial_opacity, 0.05f)
            << "fill-mode: backwards should apply first frame during delay at iteration " << i
            << ", expected: " << initial_opacity << ", got: " << result_opacity;
    }
}

/**
 * @brief Test fill-mode: both applies first frame during delay
 */
TEST_F(AnimationFillModePropertyTest, BothFillModeAppliesFirstFrameDuringDelay) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(0.5f, 2.0f);
        float delay = randFloat(0.5f, 2.0f);
        float initial_opacity = randFloat(0.0f, 0.5f);
        std::string anim_name = "both_delay_anim_" + std::to_string(i);
        
        std::ostringstream from_val;
        from_val << initial_opacity;
        KeyframesRule rule = CreateOpacityKeyframes(anim_name, from_val.str(), "1");
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = delay;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::BOTH;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        controller.Update(delay * 0.5);
        
        auto props = controller.GetCurrentProperties(render_obj.get(), anim_name);
        
        ASSERT_TRUE(props.has_value())
            << "fill-mode: both should return properties during delay at iteration " << i;
        
        auto it = props->find("opacity");
        ASSERT_NE(it, props->end());
        
        float result_opacity = std::stof(it->second);
        EXPECT_NEAR(result_opacity, initial_opacity, 0.05f)
            << "fill-mode: both should apply first frame during delay at iteration " << i;
    }
}

/**
 * @brief Test fill-mode: none does not apply styles outside animation
 */
TEST_F(AnimationFillModePropertyTest, NoneFillModeNoStylesOutsideAnimation) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(0.5f, 2.0f);
        float delay = randFloat(0.5f, 2.0f);
        std::string anim_name = "none_anim_" + std::to_string(i);
        
        KeyframesRule rule = CreateOpacityKeyframes(anim_name, "0", "1");
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = delay;
        anim.iteration_count = 1;
        anim.fill_mode = AnimationFillMode::NONE;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        // During delay - should return nullopt
        controller.Update(delay * 0.5);
        auto props_delay = controller.GetCurrentProperties(render_obj.get(), anim_name);
        
        EXPECT_FALSE(props_delay.has_value())
            << "fill-mode: none should not return properties during delay at iteration " << i;
        
        // After completion - should return nullopt
        controller.Update(delay + duration + 0.5);
        auto props_after = controller.GetCurrentProperties(render_obj.get(), anim_name);
        
        EXPECT_FALSE(props_after.has_value())
            << "fill-mode: none should not return properties after completion at iteration " << i;
    }
}

/**
 * @brief Test fill-mode during running state (all modes should return properties)
 */
TEST_F(AnimationFillModePropertyTest, AllFillModesReturnPropertiesDuringRunning) {
    std::vector<AnimationFillMode> fill_modes = {
        AnimationFillMode::NONE,
        AnimationFillMode::FORWARDS,
        AnimationFillMode::BACKWARDS,
        AnimationFillMode::BOTH
    };
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateDocument();
        AnimationController controller;
        
        auto render_obj = std::make_unique<RenderBlock>();
        
        float duration = randFloat(1.0f, 3.0f);
        AnimationFillMode fill_mode = fill_modes[i % fill_modes.size()];
        std::string anim_name = "running_anim_" + std::to_string(i);
        
        KeyframesRule rule = CreateOpacityKeyframes(anim_name, "0", "1");
        controller.RegisterKeyframes(rule);
        
        CSSAnimation anim;
        anim.name = anim_name;
        anim.duration = duration;
        anim.delay = 0.0f;
        anim.iteration_count = 1;
        anim.fill_mode = fill_mode;
        
        controller.StartAnimation(render_obj.get(), anim);
        controller.Update(0.0);
        
        // During running state
        controller.Update(duration * 0.5);
        auto props = controller.GetCurrentProperties(render_obj.get(), anim_name);
        
        ASSERT_TRUE(props.has_value())
            << "All fill-modes should return properties during running at iteration " << i;
        
        auto it = props->find("opacity");
        ASSERT_NE(it, props->end());
        
        float result_opacity = std::stof(it->second);
        // At 50% progress, opacity should be around 0.5
        EXPECT_NEAR(result_opacity, 0.5f, 0.15f)
            << "Opacity should be ~0.5 at mid-point at iteration " << i;
    }
}

