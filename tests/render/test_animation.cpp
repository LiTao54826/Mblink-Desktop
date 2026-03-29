/**
 * @file test_animation.cpp
 * @brief 动画系统测试
 */

#include <gtest/gtest.h>
#include "render/animation/animation.h"
#include "render/animation/easing_functions.h"

namespace mbink {
namespace test {

class AnimationTest : public ::testing::Test {};

// ========== 缓动函数测试 ==========

TEST_F(AnimationTest, EaseLinear) {
    EXPECT_FLOAT_EQ(EasingFunctions::Linear(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingFunctions::Linear(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(EasingFunctions::Linear(1.0f), 1.0f);
}

TEST_F(AnimationTest, EaseIn) {
    EXPECT_FLOAT_EQ(EasingFunctions::EaseIn(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingFunctions::EaseIn(1.0f), 1.0f);
    // 缓入在中间点应该小于线性
    EXPECT_LT(EasingFunctions::EaseIn(0.5f), 0.5f);
}

TEST_F(AnimationTest, EaseOut) {
    EXPECT_FLOAT_EQ(EasingFunctions::EaseOut(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingFunctions::EaseOut(1.0f), 1.0f);
    // 缓出在中间点应该大于线性
    EXPECT_GT(EasingFunctions::EaseOut(0.5f), 0.5f);
}

TEST_F(AnimationTest, EaseInOut) {
    EXPECT_FLOAT_EQ(EasingFunctions::EaseInOut(0.0f), 0.0f);
    EXPECT_NEAR(EasingFunctions::EaseInOut(0.5f), 0.5f, 0.1f);
    EXPECT_FLOAT_EQ(EasingFunctions::EaseInOut(1.0f), 1.0f);
}

TEST_F(AnimationTest, Ease) {
    EXPECT_FLOAT_EQ(EasingFunctions::Ease(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(EasingFunctions::Ease(1.0f), 1.0f);
}

TEST_F(AnimationTest, CubicBezierEasing) {
    // ease: cubic-bezier(0.25, 0.1, 0.25, 1.0)
    float result = EasingFunctions::CubicBezierEasing(0.5f, 0.25f, 0.1f, 0.25f, 1.0f);
    EXPECT_GT(result, 0.0f);
    EXPECT_LT(result, 1.0f);
}

TEST_F(AnimationTest, ApplyTimingFunction) {
    // 测试 Apply 方法
    float linear = EasingFunctions::Apply(0.5f, TimingFunction::LINEAR);
    EXPECT_FLOAT_EQ(linear, 0.5f);
    
    float ease = EasingFunctions::Apply(0.5f, TimingFunction::EASE);
    EXPECT_GT(ease, 0.0f);
    EXPECT_LT(ease, 1.0f);
}

// ========== CSS Animation 解析测试 ==========

TEST_F(AnimationTest, ParseSimpleAnimation) {
    auto animations = CSSAnimation::Parse("slide-in 0.5s");
    ASSERT_EQ(animations.size(), 1);
    EXPECT_EQ(animations[0].name, "slide-in");
    EXPECT_FLOAT_EQ(animations[0].duration, 0.5f);
}

TEST_F(AnimationTest, ParseAnimationWithTimingFunction) {
    auto animations = CSSAnimation::Parse("fade-in 1s ease-in-out");
    ASSERT_EQ(animations.size(), 1);
    EXPECT_EQ(animations[0].name, "fade-in");
    EXPECT_FLOAT_EQ(animations[0].duration, 1.0f);
    EXPECT_EQ(animations[0].timing_function, TimingFunction::EASE_IN_OUT);
}

TEST_F(AnimationTest, ParseAnimationWithDelay) {
    auto animations = CSSAnimation::Parse("bounce 0.3s ease 0.5s");
    ASSERT_EQ(animations.size(), 1);
    EXPECT_EQ(animations[0].name, "bounce");
    EXPECT_FLOAT_EQ(animations[0].duration, 0.3f);
    EXPECT_FLOAT_EQ(animations[0].delay, 0.5f);
}

TEST_F(AnimationTest, ParseAnimationWithIterationCount) {
    auto animations = CSSAnimation::Parse("spin 1s linear 3");
    ASSERT_EQ(animations.size(), 1);
    EXPECT_EQ(animations[0].iteration_count, 3);
}

TEST_F(AnimationTest, ParseAnimationInfinite) {
    auto animations = CSSAnimation::Parse("rotate 2s linear infinite");
    ASSERT_EQ(animations.size(), 1);
    EXPECT_EQ(animations[0].iteration_count, -1);  // -1 表示 infinite
}

TEST_F(AnimationTest, ParseAnimationDirection) {
    auto directions = CSSAnimation::ParseDirection("alternate");
    ASSERT_EQ(directions.size(), 1);
    EXPECT_EQ(directions[0], AnimationDirection::ANIM_ALTERNATE);
}

TEST_F(AnimationTest, ParseAnimationFillMode) {
    auto fillModes = CSSAnimation::ParseFillMode("forwards");
    ASSERT_EQ(fillModes.size(), 1);
    EXPECT_EQ(fillModes[0], AnimationFillMode::FORWARDS);
}

TEST_F(AnimationTest, ParseAnimationPlayState) {
    EXPECT_TRUE(CSSAnimation::ParsePlayState("paused"));
    EXPECT_FALSE(CSSAnimation::ParsePlayState("running"));
}

TEST_F(AnimationTest, ParseAnimationName) {
    auto names = CSSAnimation::ParseName("slide-in, fade-out");
    ASSERT_EQ(names.size(), 2);
    EXPECT_EQ(names[0], "slide-in");
    EXPECT_EQ(names[1], "fade-out");
}

TEST_F(AnimationTest, ParseAnimationDuration) {
    auto durations = CSSAnimation::ParseDuration("0.5s, 1s, 500ms");
    ASSERT_EQ(durations.size(), 3);
    EXPECT_FLOAT_EQ(durations[0], 0.5f);
    EXPECT_FLOAT_EQ(durations[1], 1.0f);
    EXPECT_FLOAT_EQ(durations[2], 0.5f);
}

TEST_F(AnimationTest, AnimationIsValid) {
    CSSAnimation anim;
    EXPECT_FALSE(anim.IsValid());  // 默认无效
    
    anim.name = "test";
    anim.duration = 1.0f;
    EXPECT_TRUE(anim.IsValid());
}

} // namespace test
} // namespace mbink
