/**
 * @file test_animation_keyframes_properties.cpp
 * @brief Property-based tests for CSS @keyframes parsing and registration
 * 
 * This file implements property-based testing for @keyframes functionality.
 * Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: css-animation-integration**
 * 
 * IMPORTANT: All tests use REAL rendering layer components, not mocks:
 * - Real Document and StyleManager
 * - Real AnimationController
 * - Real KeyframesRule parsing
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "lexbor/style_manager.h"
#include "render/animation/animation_controller.h"
#include "render/animation/keyframes.h"
#include "dom/document.h"
#include <random>
#include <vector>
#include <string>
#include <sstream>

using namespace lightui;
using namespace lightui::test;

// Random number generator for property tests
class KeyframesPropertyTestRng {
public:
    KeyframesPropertyTestRng() : gen_(std::random_device{}()) {}
    
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
    
    std::string randCSSProperty() {
        static const std::vector<std::string> properties = {
            "opacity", "transform", "background-color", "color", 
            "width", "height", "margin", "padding"
        };
        return properties[randInt(0, properties.size() - 1)];
    }
    
    std::string randCSSValue(const std::string& property) {
        if (property == "opacity") {
            return std::to_string(randFloat(0.0f, 1.0f));
        } else if (property == "transform") {
            return "translateX(" + std::to_string(randInt(-100, 100)) + "px)";
        } else if (property == "background-color" || property == "color") {
            return "rgb(" + std::to_string(randInt(0, 255)) + "," +
                   std::to_string(randInt(0, 255)) + "," +
                   std::to_string(randInt(0, 255)) + ")";
        } else {
            return std::to_string(randInt(0, 200)) + "px";
        }
    }
    
private:
    std::mt19937 gen_;
};


/**
 * @brief Test fixture for @keyframes property tests
 * 
 * Uses REAL StyleManager from Document, not mocks.
 */
class KeyframesPropertyTest : public DOMTestBase {
protected:
    KeyframesPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    
    void SetUp() override {
        DOMTestBase::SetUp();
    }
    
    /**
     * @brief Generate a valid @keyframes CSS rule
     */
    std::string GenerateKeyframesCSS(const std::string& name, int num_keyframes) {
        std::ostringstream css;
        css << "@keyframes " << name << " {\n";
        
        // Generate keyframes at different percentages
        std::vector<int> percentages;
        percentages.push_back(0);   // Always include 0%
        percentages.push_back(100); // Always include 100%
        
        // Add random intermediate keyframes
        for (int i = 0; i < num_keyframes - 2 && i < 8; ++i) {
            int pct = rng_.randInt(1, 99);
            // Avoid duplicates
            bool exists = false;
            for (int p : percentages) {
                if (p == pct) { exists = true; break; }
            }
            if (!exists) {
                percentages.push_back(pct);
            }
        }
        
        // Sort percentages
        std::sort(percentages.begin(), percentages.end());
        
        // Generate each keyframe
        for (int pct : percentages) {
            if (pct == 0) {
                css << "    from {\n";
            } else if (pct == 100) {
                css << "    to {\n";
            } else {
                css << "    " << pct << "% {\n";
            }
            
            // Add 1-3 properties
            int num_props = rng_.randInt(1, 3);
            for (int j = 0; j < num_props; ++j) {
                std::string prop = rng_.randCSSProperty();
                std::string value = rng_.randCSSValue(prop);
                css << "        " << prop << ": " << value << ";\n";
            }
            
            css << "    }\n";
        }
        
        css << "}\n";
        return css.str();
    }
};

/**
 * **Feature: css-animation-integration, Property 1: @keyframes registration consistency**
 * 
 * For any valid CSS text containing @keyframes rules, parsing through StyleManager 
 * should result in all @keyframes being registered in the AnimationController 
 * with correct names and keyframe data.
 * 
 * **Validates: Requirements 1.1, 1.3, 1.4**
 */
TEST_F(KeyframesPropertyTest, KeyframesRegistrationConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a fresh document for each iteration
        auto doc = CreateDocument();
        auto& style_manager = *doc->GetStyleManager();
        
        // Generate random animation name
        std::string anim_name = rng_.randAnimationName();
        int num_keyframes = rng_.randInt(2, 5);
        
        // Generate @keyframes CSS
        std::string css = GenerateKeyframesCSS(anim_name, num_keyframes);
        
        // Parse CSS through REAL StyleManager
        bool parse_result = style_manager.ParseCSSString(css);
        EXPECT_TRUE(parse_result) 
            << "Failed to parse CSS at iteration " << i << ": " << css;
        
        // Verify @keyframes was registered in AnimationController
        auto& controller = style_manager.GetAnimationController();
        const KeyframesRule* rule = controller.GetKeyframes(anim_name);
        
        ASSERT_NE(rule, nullptr) 
            << "Keyframes '" << anim_name << "' not registered at iteration " << i;
        
        // Verify name matches
        EXPECT_EQ(rule->name, anim_name)
            << "Keyframes name mismatch at iteration " << i;
        
        // Verify keyframes were parsed (at least from and to)
        EXPECT_GE(rule->keyframes.size(), 2u)
            << "Expected at least 2 keyframes at iteration " << i;
        
        // Verify first keyframe is at 0%
        EXPECT_FLOAT_EQ(rule->keyframes.front().offset, 0.0f)
            << "First keyframe should be at 0% at iteration " << i;
        
        // Verify last keyframe is at 100%
        EXPECT_FLOAT_EQ(rule->keyframes.back().offset, 1.0f)
            << "Last keyframe should be at 100% at iteration " << i;
        
        // Verify each keyframe has properties
        for (const auto& kf : rule->keyframes) {
            EXPECT_FALSE(kf.properties.empty())
                << "Keyframe at " << (kf.offset * 100) << "% has no properties at iteration " << i;
        }
    }
}


/**
 * **Feature: css-animation-integration, Property 2: @keyframes cascade override**
 * 
 * For any CSS text containing multiple @keyframes rules with the same name, 
 * only the last defined rule should be retained in the AnimationController.
 * 
 * **Validates: Requirements 1.2**
 */
TEST_F(KeyframesPropertyTest, KeyframesCascadeOverride) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a fresh document for each iteration
        auto doc = CreateDocument();
        auto& style_manager = *doc->GetStyleManager();
        
        // Use the same animation name for both rules
        std::string anim_name = rng_.randAnimationName();
        
        // Generate first @keyframes rule with specific property
        std::ostringstream css1;
        css1 << "@keyframes " << anim_name << " {\n"
             << "    from { opacity: 0; }\n"
             << "    to { opacity: 0.5; }\n"  // First rule: opacity goes to 0.5
             << "}\n";
        
        // Generate second @keyframes rule with different property value
        std::ostringstream css2;
        css2 << "@keyframes " << anim_name << " {\n"
             << "    from { opacity: 0; }\n"
             << "    to { opacity: 1; }\n"  // Second rule: opacity goes to 1
             << "}\n";
        
        // Combine both rules (second should override first)
        std::string combined_css = css1.str() + "\n" + css2.str();
        
        // Parse combined CSS
        bool parse_result = style_manager.ParseCSSString(combined_css);
        EXPECT_TRUE(parse_result) 
            << "Failed to parse CSS at iteration " << i;
        
        // Verify only one rule exists with the name
        auto& controller = style_manager.GetAnimationController();
        const KeyframesRule* rule = controller.GetKeyframes(anim_name);
        
        ASSERT_NE(rule, nullptr) 
            << "Keyframes '" << anim_name << "' not registered at iteration " << i;
        
        // Verify the last rule's values are used (opacity: 1 at 100%)
        ASSERT_GE(rule->keyframes.size(), 2u);
        
        const auto& last_keyframe = rule->keyframes.back();
        EXPECT_FLOAT_EQ(last_keyframe.offset, 1.0f);
        
        auto it = last_keyframe.properties.find("opacity");
        ASSERT_NE(it, last_keyframe.properties.end())
            << "opacity property not found in last keyframe at iteration " << i;
        
        // The value should be "1" from the second rule, not "0.5" from the first
        EXPECT_EQ(it->second, "1")
            << "Expected opacity: 1 from second rule, got " << it->second 
            << " at iteration " << i;
    }
}

/**
 * @brief Test that multiple different @keyframes rules can coexist
 * 
 * This is a supplementary test to ensure the cascade override only affects
 * rules with the same name.
 */
TEST_F(KeyframesPropertyTest, MultipleDistinctKeyframesCoexist) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a fresh document for each iteration
        auto doc = CreateDocument();
        auto& style_manager = *doc->GetStyleManager();
        
        // Generate 2-5 distinct animation names
        int num_animations = rng_.randInt(2, 5);
        std::vector<std::string> anim_names;
        
        std::ostringstream css;
        for (int j = 0; j < num_animations; ++j) {
            std::string name = rng_.randAnimationName() + "_" + std::to_string(j);
            anim_names.push_back(name);
            css << GenerateKeyframesCSS(name, rng_.randInt(2, 4));
            css << "\n";
        }
        
        // Parse all @keyframes rules
        bool parse_result = style_manager.ParseCSSString(css.str());
        EXPECT_TRUE(parse_result) 
            << "Failed to parse CSS at iteration " << i;
        
        // Verify all distinct rules are registered
        auto& controller = style_manager.GetAnimationController();
        for (const auto& name : anim_names) {
            const KeyframesRule* rule = controller.GetKeyframes(name);
            EXPECT_NE(rule, nullptr) 
                << "Keyframes '" << name << "' not registered at iteration " << i;
            
            if (rule) {
                EXPECT_EQ(rule->name, name)
                    << "Keyframes name mismatch for '" << name << "' at iteration " << i;
            }
        }
    }
}
