/**
 * @file test_keyframes.cpp
 * @brief CSS @keyframes 测试
 * @author MBink Development Team
 * @date 2025-11-14
 */

#include "core/render/keyframes.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace lightui;

// ============================================================================
// Keyframe 基础测试
// ============================================================================

TEST(KeyframeTest, DefaultConstructor) {
    Keyframe kf;
    EXPECT_FLOAT_EQ(kf.offset, 0.0f);
    EXPECT_TRUE(kf.properties.empty());
}

TEST(KeyframeTest, ConstructorWithOffset) {
    Keyframe kf(0.5f);
    EXPECT_FLOAT_EQ(kf.offset, 0.5f);
    EXPECT_TRUE(kf.properties.empty());
}

TEST(KeyframeTest, AddProperties) {
    Keyframe kf(0.5f);
    kf.properties["opacity"] = "0.5";
    kf.properties["transform"] = "translateX(100px)";
    
    EXPECT_EQ(kf.properties.size(), 2);
    EXPECT_EQ(kf.properties["opacity"], "0.5");
    EXPECT_EQ(kf.properties["transform"], "translateX(100px)");
}

TEST(KeyframeTest, Comparison) {
    Keyframe kf1(0.3f);
    Keyframe kf2(0.7f);
    
    EXPECT_TRUE(kf1 < kf2);
    EXPECT_FALSE(kf2 < kf1);
}

// ============================================================================
// KeyframesRule 解析测试
// ============================================================================

TEST(KeyframesRuleTest, ParseSimpleFromTo) {
    std::string css = R"(
        @keyframes slide-in {
            from {
                transform: translateX(-100%);
                opacity: 0;
            }
            to {
                transform: translateX(0);
                opacity: 1;
            }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    
    EXPECT_EQ(rule.name, "slide-in");
    EXPECT_EQ(rule.keyframes.size(), 2);
    EXPECT_TRUE(rule.IsValid());
    
    // 检查第一个关键帧 (from = 0%)
    EXPECT_FLOAT_EQ(rule.keyframes[0].offset, 0.0f);
    EXPECT_EQ(rule.keyframes[0].properties["transform"], "translateX(-100%)");
    EXPECT_EQ(rule.keyframes[0].properties["opacity"], "0");
    
    // 检查第二个关键帧 (to = 100%)
    EXPECT_FLOAT_EQ(rule.keyframes[1].offset, 1.0f);
    EXPECT_EQ(rule.keyframes[1].properties["transform"], "translateX(0)");
    EXPECT_EQ(rule.keyframes[1].properties["opacity"], "1");
}

TEST(KeyframesRuleTest, ParsePercentages) {
    std::string css = R"(
        @keyframes bounce {
            0% {
                transform: translateY(0);
            }
            50% {
                transform: translateY(-20px);
            }
            100% {
                transform: translateY(0);
            }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    
    EXPECT_EQ(rule.name, "bounce");
    EXPECT_EQ(rule.keyframes.size(), 3);
    
    EXPECT_FLOAT_EQ(rule.keyframes[0].offset, 0.0f);
    EXPECT_FLOAT_EQ(rule.keyframes[1].offset, 0.5f);
    EXPECT_FLOAT_EQ(rule.keyframes[2].offset, 1.0f);
    
    EXPECT_EQ(rule.keyframes[1].properties["transform"], "translateY(-20px)");
}

TEST(KeyframesRuleTest, ParseMultipleSelectors) {
    std::string css = R"(
        @keyframes fade {
            0%, 100% {
                opacity: 0;
            }
            50% {
                opacity: 1;
            }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    
    EXPECT_EQ(rule.name, "fade");
    EXPECT_EQ(rule.keyframes.size(), 3);
    
    // 0% 和 100% 应该有相同的属性
    EXPECT_FLOAT_EQ(rule.keyframes[0].offset, 0.0f);
    EXPECT_FLOAT_EQ(rule.keyframes[2].offset, 1.0f);
    EXPECT_EQ(rule.keyframes[0].properties["opacity"], "0");
    EXPECT_EQ(rule.keyframes[2].properties["opacity"], "0");
    
    // 50%
    EXPECT_FLOAT_EQ(rule.keyframes[1].offset, 0.5f);
    EXPECT_EQ(rule.keyframes[1].properties["opacity"], "1");
}

TEST(KeyframesRuleTest, ParseMultipleProperties) {
    std::string css = R"(
        @keyframes complex {
            0% {
                opacity: 0;
                transform: scale(0.5) rotate(0deg);
                background-color: red;
            }
            100% {
                opacity: 1;
                transform: scale(1) rotate(360deg);
                background-color: blue;
            }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    
    EXPECT_EQ(rule.name, "complex");
    EXPECT_EQ(rule.keyframes.size(), 2);
    
    // 检查第一个关键帧的所有属性
    EXPECT_EQ(rule.keyframes[0].properties.size(), 3);
    EXPECT_EQ(rule.keyframes[0].properties["opacity"], "0");
    EXPECT_EQ(rule.keyframes[0].properties["transform"], "scale(0.5) rotate(0deg)");
    EXPECT_EQ(rule.keyframes[0].properties["background-color"], "red");
}

TEST(KeyframesRuleTest, ParseWithHyphens) {
    std::string css = R"(
        @keyframes slide-in-from-left {
            from { left: -100px; }
            to { left: 0; }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    
    EXPECT_EQ(rule.name, "slide-in-from-left");
    EXPECT_EQ(rule.keyframes.size(), 2);
}

TEST(KeyframesRuleTest, ParseInvalidCSS) {
    std::string css = "invalid css";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    
    EXPECT_FALSE(rule.IsValid());
    EXPECT_TRUE(rule.name.empty());
    EXPECT_TRUE(rule.keyframes.empty());
}

// ============================================================================
// GetKeyframesAt 测试
// ============================================================================

TEST(KeyframesRuleTest, GetKeyframesAtStart) {
    std::string css = R"(
        @keyframes test {
            0% { opacity: 0; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    auto [prev, next, factor] = rule.GetKeyframesAt(0.0f);
    
    EXPECT_NE(prev, nullptr);
    EXPECT_NE(next, nullptr);
    EXPECT_FLOAT_EQ(prev->offset, 0.0f);
    EXPECT_FLOAT_EQ(next->offset, 0.0f);
    EXPECT_FLOAT_EQ(factor, 0.0f);
}

TEST(KeyframesRuleTest, GetKeyframesAtEnd) {
    std::string css = R"(
        @keyframes test {
            0% { opacity: 0; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    auto [prev, next, factor] = rule.GetKeyframesAt(1.0f);
    
    EXPECT_NE(prev, nullptr);
    EXPECT_NE(next, nullptr);
    EXPECT_FLOAT_EQ(prev->offset, 1.0f);
    EXPECT_FLOAT_EQ(next->offset, 1.0f);
    EXPECT_FLOAT_EQ(factor, 1.0f);
}

TEST(KeyframesRuleTest, GetKeyframesAtMiddle) {
    std::string css = R"(
        @keyframes test {
            0% { opacity: 0; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    auto [prev, next, factor] = rule.GetKeyframesAt(0.25f);
    
    EXPECT_NE(prev, nullptr);
    EXPECT_NE(next, nullptr);
    EXPECT_FLOAT_EQ(prev->offset, 0.0f);
    EXPECT_FLOAT_EQ(next->offset, 0.5f);
    EXPECT_FLOAT_EQ(factor, 0.5f);  // 0.25 在 [0, 0.5] 的中间
}

TEST(KeyframesRuleTest, GetKeyframesAtBetweenFrames) {
    std::string css = R"(
        @keyframes test {
            0% { opacity: 0; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    auto [prev, next, factor] = rule.GetKeyframesAt(0.75f);
    
    EXPECT_NE(prev, nullptr);
    EXPECT_NE(next, nullptr);
    EXPECT_FLOAT_EQ(prev->offset, 0.5f);
    EXPECT_FLOAT_EQ(next->offset, 1.0f);
    EXPECT_FLOAT_EQ(factor, 0.5f);  // 0.75 在 [0.5, 1.0] 的中间
}

// ============================================================================
// KeyframesManager 测试
// ============================================================================

TEST(KeyframesManagerTest, RegisterAndGet) {
    KeyframesManager& manager = KeyframesManager::Instance();
    manager.Clear();
    
    KeyframesRule rule("test-animation");
    rule.AddKeyframe(Keyframe(0.0f));
    rule.AddKeyframe(Keyframe(1.0f));
    
    manager.RegisterKeyframes(rule);
    
    const KeyframesRule* retrieved = manager.GetKeyframes("test-animation");
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "test-animation");
    EXPECT_EQ(retrieved->keyframes.size(), 2);
}

TEST(KeyframesManagerTest, GetNonExistent) {
    KeyframesManager& manager = KeyframesManager::Instance();
    manager.Clear();
    
    const KeyframesRule* retrieved = manager.GetKeyframes("non-existent");
    EXPECT_EQ(retrieved, nullptr);
}

TEST(KeyframesManagerTest, Remove) {
    KeyframesManager& manager = KeyframesManager::Instance();
    manager.Clear();
    
    KeyframesRule rule("test-animation");
    rule.AddKeyframe(Keyframe(0.0f));
    manager.RegisterKeyframes(rule);
    
    EXPECT_NE(manager.GetKeyframes("test-animation"), nullptr);
    
    manager.RemoveKeyframes("test-animation");
    
    EXPECT_EQ(manager.GetKeyframes("test-animation"), nullptr);
}

TEST(KeyframesManagerTest, GetAllNames) {
    KeyframesManager& manager = KeyframesManager::Instance();
    manager.Clear();
    
    KeyframesRule rule1("animation1");
    rule1.AddKeyframe(Keyframe(0.0f));
    manager.RegisterKeyframes(rule1);
    
    KeyframesRule rule2("animation2");
    rule2.AddKeyframe(Keyframe(0.0f));
    manager.RegisterKeyframes(rule2);
    
    std::vector<std::string> names = manager.GetAllNames();
    EXPECT_EQ(names.size(), 2);
    
    // 检查名称是否存在
    bool found1 = false, found2 = false;
    for (const auto& name : names) {
        if (name == "animation1") found1 = true;
        if (name == "animation2") found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

// ============================================================================
// 性能测试
// ============================================================================

TEST(KeyframesRuleTest, PerformanceParse) {
    std::string css = R"(
        @keyframes test {
            0% { opacity: 0; transform: translateX(0); }
            25% { opacity: 0.25; transform: translateX(25px); }
            50% { opacity: 0.5; transform: translateX(50px); }
            75% { opacity: 0.75; transform: translateX(75px); }
            100% { opacity: 1; transform: translateX(100px); }
        }
    )";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        KeyframesRule rule = KeyframesRule::Parse(css);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Parse 1000 keyframes: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 1000);  // 应该小于 1000ms (regex 在 MSVC 上较慢)
}

TEST(KeyframesRuleTest, PerformanceGetKeyframesAt) {
    std::string css = R"(
        @keyframes test {
            0% { opacity: 0; }
            25% { opacity: 0.25; }
            50% { opacity: 0.5; }
            75% { opacity: 0.75; }
            100% { opacity: 1; }
        }
    )";
    
    KeyframesRule rule = KeyframesRule::Parse(css);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        float progress = static_cast<float>(i % 100) / 100.0f;
        auto [prev, next, factor] = rule.GetKeyframesAt(progress);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "GetKeyframesAt 10000 times: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 20);  // 应该小于 20ms
}

