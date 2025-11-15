/**
 * @file test_animation.cpp
 * @brief CSS animation 测试
 * @author MBink Development Team
 * @date 2025-11-14
 */

#include "core/render/animation.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace lightui;

// ============================================================================
// CSSAnimation 基础测试
// ============================================================================

TEST(CSSAnimationTest, DefaultConstructor) {
    CSSAnimation anim;
    EXPECT_EQ(anim.name, "");
    EXPECT_FLOAT_EQ(anim.duration, 0.0f);
    EXPECT_EQ(anim.timing_function, TimingFunction::EASE);
    EXPECT_FLOAT_EQ(anim.delay, 0.0f);
    EXPECT_EQ(anim.iteration_count, 1);
    EXPECT_EQ(anim.direction, AnimationDirection::ANIM_NORMAL);
    EXPECT_EQ(anim.fill_mode, AnimationFillMode::NONE);
    EXPECT_FALSE(anim.paused);
    EXPECT_FALSE(anim.IsValid());
}

TEST(CSSAnimationTest, IsValid) {
    CSSAnimation anim;
    EXPECT_FALSE(anim.IsValid());
    
    anim.name = "test";
    EXPECT_FALSE(anim.IsValid());  // duration 还是 0
    
    anim.duration = 1.0f;
    EXPECT_TRUE(anim.IsValid());
}

// ============================================================================
// 简写属性解析测试
// ============================================================================

TEST(CSSAnimationTest, ParseSimple) {
    std::string css = "slide-in 0.5s";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "slide-in");
    EXPECT_FLOAT_EQ(anims[0].duration, 0.5f);
    EXPECT_EQ(anims[0].timing_function, TimingFunction::EASE);
    EXPECT_FLOAT_EQ(anims[0].delay, 0.0f);
    EXPECT_EQ(anims[0].iteration_count, 1);
}

TEST(CSSAnimationTest, ParseWithTimingFunction) {
    std::string css = "fade-in 1s ease-in-out";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "fade-in");
    EXPECT_FLOAT_EQ(anims[0].duration, 1.0f);
    EXPECT_EQ(anims[0].timing_function, TimingFunction::EASE_IN_OUT);
}

TEST(CSSAnimationTest, ParseWithDelay) {
    std::string css = "bounce 0.3s ease 0.1s";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "bounce");
    EXPECT_FLOAT_EQ(anims[0].duration, 0.3f);
    EXPECT_EQ(anims[0].timing_function, TimingFunction::EASE);
    EXPECT_FLOAT_EQ(anims[0].delay, 0.1f);
}

TEST(CSSAnimationTest, ParseWithIterationCount) {
    std::string css = "spin 1s linear infinite";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "spin");
    EXPECT_FLOAT_EQ(anims[0].duration, 1.0f);
    EXPECT_EQ(anims[0].timing_function, TimingFunction::LINEAR);
    EXPECT_EQ(anims[0].iteration_count, -1);  // infinite
}

TEST(CSSAnimationTest, ParseWithFiniteIterationCount) {
    std::string css = "pulse 0.5s ease 3";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "pulse");
    EXPECT_FLOAT_EQ(anims[0].duration, 0.5f);
    EXPECT_EQ(anims[0].iteration_count, 3);
}

TEST(CSSAnimationTest, ParseWithDirection) {
    std::string css = "wave 1s ease infinite alternate";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);

    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "wave");
    EXPECT_EQ(anims[0].direction, AnimationDirection::ANIM_ALTERNATE);
}

TEST(CSSAnimationTest, ParseWithFillMode) {
    std::string css = "slide-out 0.5s ease forwards";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "slide-out");
    EXPECT_EQ(anims[0].fill_mode, AnimationFillMode::FORWARDS);
}

TEST(CSSAnimationTest, ParseComplex) {
    std::string css = "complex 1s ease-in-out 0.5s infinite alternate forwards";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "complex");
    EXPECT_FLOAT_EQ(anims[0].duration, 1.0f);
    EXPECT_EQ(anims[0].timing_function, TimingFunction::EASE_IN_OUT);
    EXPECT_FLOAT_EQ(anims[0].delay, 0.5f);
    EXPECT_EQ(anims[0].iteration_count, -1);
    EXPECT_EQ(anims[0].direction, AnimationDirection::ANIM_ALTERNATE);
    EXPECT_EQ(anims[0].fill_mode, AnimationFillMode::FORWARDS);
}

TEST(CSSAnimationTest, ParseMultiple) {
    std::string css = "anim1 1s, anim2 2s ease-in";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 2);
    EXPECT_EQ(anims[0].name, "anim1");
    EXPECT_FLOAT_EQ(anims[0].duration, 1.0f);
    EXPECT_EQ(anims[1].name, "anim2");
    EXPECT_FLOAT_EQ(anims[1].duration, 2.0f);
    EXPECT_EQ(anims[1].timing_function, TimingFunction::EASE_IN);
}

TEST(CSSAnimationTest, ParseWithCubicBezier) {
    std::string css = "custom 1s cubic-bezier(0.42, 0, 0.58, 1)";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "custom");
    EXPECT_EQ(anims[0].timing_function, TimingFunction::CUBIC_BEZIER);
    EXPECT_FLOAT_EQ(anims[0].bezier.x1, 0.42f);
    EXPECT_FLOAT_EQ(anims[0].bezier.y1, 0.0f);
    EXPECT_FLOAT_EQ(anims[0].bezier.x2, 0.58f);
    EXPECT_FLOAT_EQ(anims[0].bezier.y2, 1.0f);
}

TEST(CSSAnimationTest, ParseMilliseconds) {
    std::string css = "fast 500ms";
    std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    
    ASSERT_EQ(anims.size(), 1);
    EXPECT_EQ(anims[0].name, "fast");
    EXPECT_FLOAT_EQ(anims[0].duration, 0.5f);  // 转换为秒
}

// ============================================================================
// 分解属性解析测试
// ============================================================================

TEST(CSSAnimationTest, ParseName) {
    std::vector<std::string> names = CSSAnimation::ParseName("anim1, anim2, anim3");
    ASSERT_EQ(names.size(), 3);
    EXPECT_EQ(names[0], "anim1");
    EXPECT_EQ(names[1], "anim2");
    EXPECT_EQ(names[2], "anim3");
}

TEST(CSSAnimationTest, ParseDuration) {
    std::vector<float> durations = CSSAnimation::ParseDuration("1s, 500ms, 2.5s");
    ASSERT_EQ(durations.size(), 3);
    EXPECT_FLOAT_EQ(durations[0], 1.0f);
    EXPECT_FLOAT_EQ(durations[1], 0.5f);
    EXPECT_FLOAT_EQ(durations[2], 2.5f);
}

TEST(CSSAnimationTest, ParseTimingFunction) {
    auto functions = CSSAnimation::ParseTimingFunction("linear, ease, ease-in-out");
    ASSERT_EQ(functions.size(), 3);
    EXPECT_EQ(functions[0].first, TimingFunction::LINEAR);
    EXPECT_EQ(functions[1].first, TimingFunction::EASE);
    EXPECT_EQ(functions[2].first, TimingFunction::EASE_IN_OUT);
}

TEST(CSSAnimationTest, ParseIterationCount) {
    std::vector<int> counts = CSSAnimation::ParseIterationCount("1, 3, infinite");
    ASSERT_EQ(counts.size(), 3);
    EXPECT_EQ(counts[0], 1);
    EXPECT_EQ(counts[1], 3);
    EXPECT_EQ(counts[2], -1);  // infinite
}

TEST(CSSAnimationTest, ParseDirection) {
    auto directions = CSSAnimation::ParseDirection("normal, reverse, alternate, alternate-reverse");
    ASSERT_EQ(directions.size(), 4);
    EXPECT_EQ(directions[0], AnimationDirection::ANIM_NORMAL);
    EXPECT_EQ(directions[1], AnimationDirection::ANIM_REVERSE);
    EXPECT_EQ(directions[2], AnimationDirection::ANIM_ALTERNATE);
    EXPECT_EQ(directions[3], AnimationDirection::ANIM_ALTERNATE_REVERSE);
}

TEST(CSSAnimationTest, ParseFillMode) {
    auto modes = CSSAnimation::ParseFillMode("none, forwards, backwards, both");
    ASSERT_EQ(modes.size(), 4);
    EXPECT_EQ(modes[0], AnimationFillMode::NONE);
    EXPECT_EQ(modes[1], AnimationFillMode::FORWARDS);
    EXPECT_EQ(modes[2], AnimationFillMode::BACKWARDS);
    EXPECT_EQ(modes[3], AnimationFillMode::BOTH);
}

TEST(CSSAnimationTest, ParsePlayState) {
    EXPECT_TRUE(CSSAnimation::ParsePlayState("paused"));
    EXPECT_FALSE(CSSAnimation::ParsePlayState("running"));
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST(CSSAnimationTest, ParseEmpty) {
    std::vector<CSSAnimation> anims = CSSAnimation::Parse("");
    EXPECT_TRUE(anims.empty());
}

TEST(CSSAnimationTest, ParseInvalid) {
    std::vector<CSSAnimation> anims = CSSAnimation::Parse("invalid");
    EXPECT_TRUE(anims.empty());  // 没有 duration，无效
}

// ============================================================================
// 性能测试
// ============================================================================

TEST(CSSAnimationTest, PerformanceParse) {
    std::string css = "test 1s ease-in-out 0.5s infinite alternate forwards";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Parse 10000 animations: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 1000);  // 应该小于 1000ms
}

TEST(CSSAnimationTest, PerformanceParseMultiple) {
    std::string css = "anim1 1s, anim2 2s ease-in, anim3 0.5s linear infinite";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        std::vector<CSSAnimation> anims = CSSAnimation::Parse(css);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Parse 10000 multiple animations: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 1000);  // 应该小于 1000ms
}

