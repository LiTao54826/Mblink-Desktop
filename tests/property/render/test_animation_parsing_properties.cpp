/**
 * @file test_animation_parsing_properties.cpp
 * @brief Property-based tests for CSS animation property parsing
 * 
 * This file implements property-based testing for animation property parsing.
 * Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: css-animation-integration**
 * 
 * IMPORTANT: All tests use REAL rendering layer components, not mocks:
 * - Real Document, Element, and RenderObject
 * - Real StyleManager and StyleResolver
 * - Real ComputedStyle with animation fields
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "core/lexbor/style_manager.h"
#include "core/render/css/style_resolver.h"
#include "core/render/objects/render_object.h"
#include "core/render/animation/animation.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

using namespace mblink;
using namespace mblink::test;

// Random number generator for property tests
class AnimationParsingTestRng {
public:
    AnimationParsingTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }
    
    std::string randAnimationName() {
        static const std::vector<std::string> prefixes = {
            "slide", "fade", "bounce", "spin", "zoom", "shake", "pulse", "flip"
        };
        static const std::vector<std::string> suffixes = {
            "In", "Out", "Up", "Down", "Left", "Right", ""
        };
        return prefixes[randInt(0, prefixes.size() - 1)] + 
               suffixes[randInt(0, suffixes.size() - 1)] +
               std::to_string(randInt(1, 100));
    }
    
    std::string randTimingFunction() {
        static const std::vector<std::string> functions = {
            "linear", "ease", "ease-in", "ease-out", "ease-in-out"
        };
        return functions[randInt(0, functions.size() - 1)];
    }
    
    std::string randDirection() {
        static const std::vector<std::string> directions = {
            "normal", "reverse", "alternate", "alternate-reverse"
        };
        return directions[randInt(0, directions.size() - 1)];
    }
    
    std::string randFillMode() {
        static const std::vector<std::string> modes = {
            "none", "forwards", "backwards", "both"
        };
        return modes[randInt(0, modes.size() - 1)];
    }
    
    std::string randDuration() {
        int ms = randInt(100, 5000);
        if (randInt(0, 1) == 0) {
            return std::to_string(ms) + "ms";
        } else {
            return std::to_string(ms / 1000.0f) + "s";
        }
    }
    
private:
    std::mt19937 gen_;
};


/**
 * @brief Test fixture for animation parsing property tests
 * 
 * Uses REAL Document, StyleManager, and RenderObject, not mocks.
 */
class AnimationParsingPropertyTest : public DOMTestBase {
protected:
    AnimationParsingTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    
    void SetUp() override {
        DOMTestBase::SetUp();
    }
    
    /**
     * @brief Build render tree for a document
     */
    std::shared_ptr<RenderObject> BuildRenderTree(std::shared_ptr<Document> doc) {
        auto body = doc->GetBody();
        if (!body) return nullptr;
        
        RenderTreeBuilder builder;
        builder.SetDocument(doc.get());
        return builder.BuildRenderTree(body, nullptr);
    }
    
    /**
     * @brief Generate a valid animation shorthand CSS value
     */
    std::string GenerateAnimationShorthand() {
        std::ostringstream css;
        css << rng_.randAnimationName() << " ";
        css << rng_.randDuration() << " ";
        css << rng_.randTimingFunction();
        
        // Optionally add delay
        if (rng_.randInt(0, 1) == 1) {
            css << " " << rng_.randDuration();
        }
        
        // Optionally add iteration count
        if (rng_.randInt(0, 1) == 1) {
            if (rng_.randInt(0, 3) == 0) {
                css << " infinite";
            } else {
                css << " " << rng_.randInt(1, 10);
            }
        }
        
        // Optionally add direction
        if (rng_.randInt(0, 1) == 1) {
            css << " " << rng_.randDirection();
        }
        
        // Optionally add fill-mode
        if (rng_.randInt(0, 1) == 1) {
            css << " " << rng_.randFillMode();
        }
        
        return css.str();
    }
    
    /**
     * @brief Helper to parse duration string to seconds
     */
    float ParseDurationToSeconds(const std::string& duration) {
        if (duration.find("ms") != std::string::npos) {
            return std::stof(duration.substr(0, duration.size() - 2)) / 1000.0f;
        } else if (duration.find("s") != std::string::npos) {
            return std::stof(duration.substr(0, duration.size() - 1));
        }
        return 0.0f;
    }
};

/**
 * **Feature: css-animation-integration, Property 3: Animation shorthand parsing completeness**
 * 
 * For any valid animation shorthand CSS value, parsing through StyleResolver
 * should result in ComputedStyle.animations containing a CSSAnimation with
 * all specified properties correctly extracted.
 * 
 * **Validates: Requirements 2.1, 2.2**
 */
TEST_F(AnimationParsingPropertyTest, AnimationShorthandParsingCompleteness) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a fresh document for each iteration
        auto doc = CreateDocument();
        auto body = doc->GetBody();
        ASSERT_NE(body, nullptr);
        
        // Create a div element
        auto div = doc->CreateElement("div");
        body->AppendChild(div);
        
        // Generate random animation name and duration
        std::string anim_name = rng_.randAnimationName();
        std::string duration_str = rng_.randDuration();
        std::string timing_func = rng_.randTimingFunction();
        
        // Build animation shorthand
        std::ostringstream css;
        css << anim_name << " " << duration_str << " " << timing_func;
        std::string animation_value = css.str();
        
        // Set animation style on element
        div->SetAttribute("style", "animation: " + animation_value);
        
        // Build render tree to trigger style resolution
        auto render_tree = BuildRenderTree(doc);
        ASSERT_NE(render_tree, nullptr) << "Failed to build render tree at iteration " << i;
        
        // Get the render object for div (first child of body's render object)
        auto render_obj = div->GetRenderObject();
        ASSERT_NE(render_obj, nullptr) 
            << "RenderObject not created at iteration " << i;
        
        // Get computed style
        const auto& computed_style = render_obj->GetComputedStyle();
        
        // Verify animation was parsed
        ASSERT_GE(computed_style.animations.size(), 1u)
            << "No animations parsed at iteration " << i 
            << ", animation value: " << animation_value;
        
        const auto& anim = computed_style.animations[0];
        
        // Verify animation name
        EXPECT_EQ(anim.name, anim_name)
            << "Animation name mismatch at iteration " << i
            << ", expected: " << anim_name << ", got: " << anim.name;
        
        // Verify duration (with tolerance for float comparison)
        float expected_duration = ParseDurationToSeconds(duration_str);
        EXPECT_NEAR(anim.duration, expected_duration, 0.001f)
            << "Duration mismatch at iteration " << i
            << ", expected: " << expected_duration << ", got: " << anim.duration;
        
        // Verify timing function
        TimingFunction expected_func = TimingFunction::EASE;
        if (timing_func == "linear") expected_func = TimingFunction::LINEAR;
        else if (timing_func == "ease") expected_func = TimingFunction::EASE;
        else if (timing_func == "ease-in") expected_func = TimingFunction::EASE_IN;
        else if (timing_func == "ease-out") expected_func = TimingFunction::EASE_OUT;
        else if (timing_func == "ease-in-out") expected_func = TimingFunction::EASE_IN_OUT;
        
        EXPECT_EQ(anim.timing_function, expected_func)
            << "Timing function mismatch at iteration " << i;
    }
}


/**
 * **Feature: css-animation-integration, Property 4: Multi-animation preservation**
 * 
 * For any CSS animation value containing multiple comma-separated animations,
 * all animations should be preserved in ComputedStyle.animations with their
 * individual properties intact.
 * 
 * **Validates: Requirements 2.4**
 */
TEST_F(AnimationParsingPropertyTest, MultiAnimationPreservation) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a fresh document for each iteration
        auto doc = CreateDocument();
        auto body = doc->GetBody();
        ASSERT_NE(body, nullptr);
        
        // Create a div element
        auto div = doc->CreateElement("div");
        body->AppendChild(div);
        
        // Generate 2-4 random animations
        int num_animations = rng_.randInt(2, 4);
        std::vector<std::string> anim_names;
        std::vector<float> anim_durations;
        
        std::ostringstream css;
        for (int j = 0; j < num_animations; ++j) {
            if (j > 0) css << ", ";
            
            std::string name = rng_.randAnimationName() + "_" + std::to_string(j);
            std::string duration_str = rng_.randDuration();
            
            anim_names.push_back(name);
            anim_durations.push_back(ParseDurationToSeconds(duration_str));
            
            css << name << " " << duration_str << " ease";
        }
        
        std::string animation_value = css.str();
        
        // Set animation style on element
        div->SetAttribute("style", "animation: " + animation_value);
        
        // Build render tree to trigger style resolution
        auto render_tree = BuildRenderTree(doc);
        ASSERT_NE(render_tree, nullptr) << "Failed to build render tree at iteration " << i;
        
        // Get the render object
        auto render_obj = div->GetRenderObject();
        ASSERT_NE(render_obj, nullptr) 
            << "RenderObject not created at iteration " << i;
        
        // Get computed style
        const auto& computed_style = render_obj->GetComputedStyle();
        
        // Verify all animations were parsed
        ASSERT_EQ(computed_style.animations.size(), static_cast<size_t>(num_animations))
            << "Animation count mismatch at iteration " << i
            << ", expected: " << num_animations 
            << ", got: " << computed_style.animations.size()
            << ", animation value: " << animation_value;
        
        // Verify each animation's properties
        for (int j = 0; j < num_animations; ++j) {
            const auto& anim = computed_style.animations[j];
            
            EXPECT_EQ(anim.name, anim_names[j])
                << "Animation " << j << " name mismatch at iteration " << i
                << ", expected: " << anim_names[j] << ", got: " << anim.name;
            
            EXPECT_NEAR(anim.duration, anim_durations[j], 0.001f)
                << "Animation " << j << " duration mismatch at iteration " << i
                << ", expected: " << anim_durations[j] << ", got: " << anim.duration;
        }
    }
}


/**
 * @brief Test animation-play-state parsing
 * 
 * Verifies that animation-play-state property is correctly parsed
 * and applied to all animations.
 */
TEST_F(AnimationParsingPropertyTest, AnimationPlayStateParsing) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a fresh document for each iteration
        auto doc = CreateDocument();
        auto body = doc->GetBody();
        ASSERT_NE(body, nullptr);
        
        // Create a div element
        auto div = doc->CreateElement("div");
        body->AppendChild(div);
        
        // Generate animation with random play state
        std::string anim_name = rng_.randAnimationName();
        bool should_be_paused = rng_.randInt(0, 1) == 1;
        std::string play_state = should_be_paused ? "paused" : "running";
        
        // Set animation style with play-state
        std::string style_value = "animation: " + anim_name + " 1s ease; "
                                  "animation-play-state: " + play_state;
        div->SetAttribute("style", style_value);
        
        // Build render tree to trigger style resolution
        auto render_tree = BuildRenderTree(doc);
        ASSERT_NE(render_tree, nullptr) << "Failed to build render tree at iteration " << i;
        
        // Get the render object
        auto render_obj = div->GetRenderObject();
        ASSERT_NE(render_obj, nullptr) 
            << "RenderObject not created at iteration " << i;
        
        // Get computed style
        const auto& computed_style = render_obj->GetComputedStyle();
        
        // Verify play state
        EXPECT_EQ(computed_style.animation_play_state, play_state)
            << "animation_play_state mismatch at iteration " << i;
        
        // Verify animation's paused flag
        if (!computed_style.animations.empty()) {
            EXPECT_EQ(computed_style.animations[0].paused, should_be_paused)
                << "Animation paused flag mismatch at iteration " << i;
        }
    }
}

