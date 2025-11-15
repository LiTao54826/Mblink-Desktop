#include <gtest/gtest.h>
#include "core/render/transition.h"
#include "core/render/easing_functions.h"
#include <cmath>
#include <chrono>

using namespace lightui;

// ============================================================================
// CubicBezier Tests
// ============================================================================

TEST(CubicBezierTest, LinearBezier) {
    CubicBezier bezier(0, 0, 1, 1);
    
    EXPECT_NEAR(bezier.Evaluate(0.0f), 0.0f, 0.01f);
    EXPECT_NEAR(bezier.Evaluate(0.5f), 0.5f, 0.01f);
    EXPECT_NEAR(bezier.Evaluate(1.0f), 1.0f, 0.01f);
}

TEST(CubicBezierTest, EaseBezier) {
    // ease = cubic-bezier(0.25, 0.1, 0.25, 1.0)
    CubicBezier bezier(0.25f, 0.1f, 0.25f, 1.0f);

    EXPECT_NEAR(bezier.Evaluate(0.0f), 0.0f, 0.01f);
    EXPECT_NEAR(bezier.Evaluate(1.0f), 1.0f, 0.01f);

    // 中间值应该有缓动效果
    float mid = bezier.Evaluate(0.5f);
    EXPECT_GT(mid, 0.4f);
    EXPECT_LT(mid, 1.0f);
}

TEST(CubicBezierTest, CustomBezier) {
    CubicBezier bezier(0.4f, 0.0f, 0.2f, 1.0f);
    
    EXPECT_NEAR(bezier.Evaluate(0.0f), 0.0f, 0.01f);
    EXPECT_NEAR(bezier.Evaluate(1.0f), 1.0f, 0.01f);
}

// ============================================================================
// CSSTransition Parse Tests
// ============================================================================

TEST(CSSTransitionTest, ParseSimpleTransition) {
    auto transitions = CSSTransition::Parse("all 0.3s ease");
    
    ASSERT_EQ(transitions.size(), 1);
    EXPECT_EQ(transitions[0].property, "all");
    EXPECT_NEAR(transitions[0].duration, 0.3f, 0.001f);
    EXPECT_EQ(transitions[0].timing_function, TimingFunction::EASE);
    EXPECT_NEAR(transitions[0].delay, 0.0f, 0.001f);
}

TEST(CSSTransitionTest, ParseTransitionWithDelay) {
    auto transitions = CSSTransition::Parse("opacity 0.5s ease-in-out 0.2s");
    
    ASSERT_EQ(transitions.size(), 1);
    EXPECT_EQ(transitions[0].property, "opacity");
    EXPECT_NEAR(transitions[0].duration, 0.5f, 0.001f);
    EXPECT_EQ(transitions[0].timing_function, TimingFunction::EASE_IN_OUT);
    EXPECT_NEAR(transitions[0].delay, 0.2f, 0.001f);
}

TEST(CSSTransitionTest, ParseMultipleTransitions) {
    auto transitions = CSSTransition::Parse("width 0.3s, height 0.5s");
    
    ASSERT_EQ(transitions.size(), 2);
    EXPECT_EQ(transitions[0].property, "width");
    EXPECT_NEAR(transitions[0].duration, 0.3f, 0.001f);
    EXPECT_EQ(transitions[1].property, "height");
    EXPECT_NEAR(transitions[1].duration, 0.5f, 0.001f);
}

TEST(CSSTransitionTest, ParseCubicBezier) {
    auto transitions = CSSTransition::Parse("transform 0.3s cubic-bezier(0.4, 0, 0.2, 1)");
    
    ASSERT_EQ(transitions.size(), 1);
    EXPECT_EQ(transitions[0].property, "transform");
    EXPECT_NEAR(transitions[0].duration, 0.3f, 0.001f);
    EXPECT_EQ(transitions[0].timing_function, TimingFunction::CUBIC_BEZIER);
    EXPECT_NEAR(transitions[0].bezier.x1, 0.4f, 0.001f);
    EXPECT_NEAR(transitions[0].bezier.y1, 0.0f, 0.001f);
    EXPECT_NEAR(transitions[0].bezier.x2, 0.2f, 0.001f);
    EXPECT_NEAR(transitions[0].bezier.y2, 1.0f, 0.001f);
}

TEST(CSSTransitionTest, ParseMilliseconds) {
    auto transitions = CSSTransition::Parse("all 300ms");
    
    ASSERT_EQ(transitions.size(), 1);
    EXPECT_NEAR(transitions[0].duration, 0.3f, 0.001f);
}

TEST(CSSTransitionTest, ParseEmpty) {
    auto transitions = CSSTransition::Parse("");
    EXPECT_EQ(transitions.size(), 0);
}

TEST(CSSTransitionTest, ParseNone) {
    auto transitions = CSSTransition::Parse("none");
    EXPECT_EQ(transitions.size(), 0);
}

// ============================================================================
// CSSTransition Property Parse Tests
// ============================================================================

TEST(CSSTransitionTest, ParseProperty) {
    auto props = CSSTransition::ParseProperty("width, height, opacity");
    
    ASSERT_EQ(props.size(), 3);
    EXPECT_EQ(props[0], "width");
    EXPECT_EQ(props[1], "height");
    EXPECT_EQ(props[2], "opacity");
}

TEST(CSSTransitionTest, ParsePropertyAll) {
    auto props = CSSTransition::ParseProperty("all");
    
    ASSERT_EQ(props.size(), 1);
    EXPECT_EQ(props[0], "all");
}

// ============================================================================
// CSSTransition Duration Parse Tests
// ============================================================================

TEST(CSSTransitionTest, ParseDuration) {
    auto durations = CSSTransition::ParseDuration("0.3s, 0.5s, 1s");
    
    ASSERT_EQ(durations.size(), 3);
    EXPECT_NEAR(durations[0], 0.3f, 0.001f);
    EXPECT_NEAR(durations[1], 0.5f, 0.001f);
    EXPECT_NEAR(durations[2], 1.0f, 0.001f);
}

TEST(CSSTransitionTest, ParseDurationMilliseconds) {
    auto durations = CSSTransition::ParseDuration("300ms, 500ms");
    
    ASSERT_EQ(durations.size(), 2);
    EXPECT_NEAR(durations[0], 0.3f, 0.001f);
    EXPECT_NEAR(durations[1], 0.5f, 0.001f);
}

// ============================================================================
// CSSTransition Timing Function Parse Tests
// ============================================================================

TEST(CSSTransitionTest, ParseTimingFunction) {
    auto funcs = CSSTransition::ParseTimingFunction("ease, linear, ease-in");
    
    ASSERT_EQ(funcs.size(), 3);
    EXPECT_EQ(funcs[0].first, TimingFunction::EASE);
    EXPECT_EQ(funcs[1].first, TimingFunction::LINEAR);
    EXPECT_EQ(funcs[2].first, TimingFunction::EASE_IN);
}

TEST(CSSTransitionTest, ParseTimingFunctionCubicBezier) {
    auto funcs = CSSTransition::ParseTimingFunction("cubic-bezier(0.4, 0, 0.2, 1)");
    
    ASSERT_EQ(funcs.size(), 1);
    EXPECT_EQ(funcs[0].first, TimingFunction::CUBIC_BEZIER);
    EXPECT_NEAR(funcs[0].second.x1, 0.4f, 0.001f);
    EXPECT_NEAR(funcs[0].second.y1, 0.0f, 0.001f);
    EXPECT_NEAR(funcs[0].second.x2, 0.2f, 0.001f);
    EXPECT_NEAR(funcs[0].second.y2, 1.0f, 0.001f);
}

// ============================================================================
// CSSTransition Delay Parse Tests
// ============================================================================

TEST(CSSTransitionTest, ParseDelay) {
    auto delays = CSSTransition::ParseDelay("0s, 0.2s, 0.5s");
    
    ASSERT_EQ(delays.size(), 3);
    EXPECT_NEAR(delays[0], 0.0f, 0.001f);
    EXPECT_NEAR(delays[1], 0.2f, 0.001f);
    EXPECT_NEAR(delays[2], 0.5f, 0.001f);
}

// ============================================================================
// EasingFunctions Tests
// ============================================================================

TEST(EasingFunctionsTest, Linear) {
    EXPECT_NEAR(EasingFunctions::Linear(0.0f), 0.0f, 0.001f);
    EXPECT_NEAR(EasingFunctions::Linear(0.5f), 0.5f, 0.001f);
    EXPECT_NEAR(EasingFunctions::Linear(1.0f), 1.0f, 0.001f);
}

TEST(EasingFunctionsTest, Ease) {
    float result = EasingFunctions::Ease(0.5f);
    EXPECT_GT(result, 0.4f);
    EXPECT_LT(result, 1.0f);
}

TEST(EasingFunctionsTest, EaseIn) {
    float result = EasingFunctions::EaseIn(0.5f);
    // ease-in 在中点应该小于 0.5 (慢启动)
    EXPECT_LT(result, 0.5f);
}

TEST(EasingFunctionsTest, EaseOut) {
    float result = EasingFunctions::EaseOut(0.5f);
    // ease-out 在中点应该大于 0.5 (快启动)
    EXPECT_GT(result, 0.5f);
}

TEST(EasingFunctionsTest, EaseInOut) {
    float result = EasingFunctions::EaseInOut(0.5f);
    EXPECT_NEAR(result, 0.5f, 0.1f);
}

TEST(EasingFunctionsTest, Apply) {
    float linear = EasingFunctions::Apply(0.5f, TimingFunction::LINEAR);
    EXPECT_NEAR(linear, 0.5f, 0.001f);

    float ease = EasingFunctions::Apply(0.5f, TimingFunction::EASE);
    EXPECT_GT(ease, 0.4f);
    EXPECT_LT(ease, 1.0f);
}

TEST(EasingFunctionsTest, ApplyCubicBezier) {
    CubicBezier bezier(0.4f, 0.0f, 0.2f, 1.0f);
    float result = EasingFunctions::Apply(0.5f, TimingFunction::CUBIC_BEZIER, bezier);
    
    EXPECT_GE(result, 0.0f);
    EXPECT_LE(result, 1.0f);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(TransitionPerformanceTest, ParsePerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        CSSTransition::Parse("all 0.3s ease 0.1s");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Parse 1000 transitions: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 100);  // 应该小于 100ms
}

TEST(TransitionPerformanceTest, EasingPerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    float sum = 0;
    for (int i = 0; i < 10000; ++i) {
        float t = i / 10000.0f;
        sum += EasingFunctions::Ease(t);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "10000 easing calculations: " << duration.count() << "ms (sum=" << sum << ")" << std::endl;
    EXPECT_LT(duration.count(), 50);  // 应该小于 50ms
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

