#include <gtest/gtest.h>
#include "core/render/animation_timeline.h"
#include "core/render/render_object.h"
#include <chrono>

using namespace lightui;

// ============================================================================
// AnimationTimeline Basic Tests
// ============================================================================

TEST(AnimationTimelineTest, CreateTimeline) {
    AnimationTimeline timeline;

    EXPECT_FALSE(timeline.HasRunningTransitions());
    EXPECT_EQ(timeline.GetRunningTransitionCount(), 0);
}

TEST(AnimationTimelineTest, StartTransition) {
    AnimationTimeline timeline;
    RenderBlock obj;

    CSSTransition trans;
    trans.property = "opacity";
    trans.duration = 0.3f;
    trans.timing_function = TimingFunction::LINEAR;
    trans.delay = 0.0f;

    timeline.StartTransition(&obj, "opacity", trans, 0.0f, 1.0f);

    EXPECT_TRUE(timeline.HasRunningTransitions());
    EXPECT_EQ(timeline.GetRunningTransitionCount(), 1);
}

TEST(AnimationTimelineTest, StopTransition) {
    AnimationTimeline timeline;
    RenderBlock obj;

    CSSTransition trans;
    trans.property = "opacity";
    trans.duration = 0.3f;

    timeline.StartTransition(&obj, "opacity", trans, 0.0f, 1.0f);
    EXPECT_TRUE(timeline.HasRunningTransitions());

    timeline.StopTransition(&obj, "opacity");
    EXPECT_FALSE(timeline.HasRunningTransitions());
}

TEST(AnimationTimelineTest, StopAllTransitions) {
    AnimationTimeline timeline;
    RenderBlock obj;

    CSSTransition trans;
    trans.duration = 0.3f;

    timeline.StartTransition(&obj, "opacity", trans, 0.0f, 1.0f);
    timeline.StartTransition(&obj, "width", trans, 100.0f, 200.0f);

    EXPECT_EQ(timeline.GetRunningTransitionCount(), 2);

    timeline.StopAllTransitions(&obj);
    EXPECT_EQ(timeline.GetRunningTransitionCount(), 0);
}

TEST(AnimationTimelineTest, MultipleObjects) {
    AnimationTimeline timeline;
    RenderBlock obj1, obj2;
    
    CSSTransition trans;
    trans.duration = 0.3f;
    
    timeline.StartTransition(&obj1, "opacity", trans, 0.0f, 1.0f);
    timeline.StartTransition(&obj2, "opacity", trans, 0.0f, 1.0f);
    
    EXPECT_EQ(timeline.GetRunningTransitionCount(), 2);
    
    timeline.StopAllTransitions(&obj1);
    EXPECT_EQ(timeline.GetRunningTransitionCount(), 1);
}

// ============================================================================
// AnimationTimeline Update Tests
// ============================================================================

TEST(AnimationTimelineTest, UpdateProgress) {
    AnimationTimeline timeline;
    RenderBlock obj;

    CSSTransition trans;
    trans.property = "opacity";
    trans.duration = 1.0f;  // 1 秒
    trans.timing_function = TimingFunction::LINEAR;
    trans.delay = 0.0f;

    timeline.StartTransition(&obj, "opacity", trans, 0.0f, 1.0f);

    // 开始时间
    timeline.Update(0.0);

    // 0.5 秒后
    timeline.Update(500.0);
    auto value = timeline.GetCurrentValue(&obj, "opacity");
    ASSERT_TRUE(value.has_value());
    EXPECT_NEAR(std::get<float>(*value), 0.5f, 0.05f);

    // 1.0 秒后 (完成)
    timeline.Update(1000.0);
    EXPECT_FALSE(timeline.HasRunningTransitions());
}

TEST(AnimationTimelineTest, UpdateWithDelay) {
    AnimationTimeline timeline;
    RenderBlock obj;
    
    CSSTransition trans;
    trans.property = "opacity";
    trans.duration = 1.0f;
    trans.timing_function = TimingFunction::LINEAR;
    trans.delay = 0.5f;  // 0.5 秒延迟
    
    timeline.StartTransition(&obj, "opacity", trans, 0.0f, 1.0f);
    
    // 开始时间
    timeline.Update(0.0);
    EXPECT_TRUE(timeline.HasRunningTransitions());
    
    // 0.3 秒后 (仍在延迟中)
    timeline.Update(300.0);
    auto value = timeline.GetCurrentValue(&obj, "opacity");
    EXPECT_FALSE(value.has_value());  // 延迟期间没有值
    
    // 0.5 秒后 (延迟结束，开始过渡)
    timeline.Update(500.0);
    EXPECT_TRUE(timeline.HasRunningTransitions());
    
    // 1.0 秒后 (过渡进行到一半)
    timeline.Update(1000.0);
    value = timeline.GetCurrentValue(&obj, "opacity");
    ASSERT_TRUE(value.has_value());
    EXPECT_NEAR(std::get<float>(*value), 0.5f, 0.05f);
}

// ============================================================================
// Interpolation Tests
// ============================================================================

TEST(AnimationTimelineTest, InterpolateFloat) {
    AnimationTimeline timeline;
    RenderBlock obj;

    CSSTransition trans;
    trans.property = "width";
    trans.duration = 1.0f;
    trans.timing_function = TimingFunction::LINEAR;

    timeline.StartTransition(&obj, "width", trans, 100.0f, 200.0f);

    timeline.Update(0.0);
    timeline.Update(500.0);  // 0.5 秒

    auto value = timeline.GetCurrentValue(&obj, "width");
    ASSERT_TRUE(value.has_value());
    EXPECT_NEAR(std::get<float>(*value), 150.0f, 1.0f);
}

TEST(AnimationTimelineTest, InterpolateColor) {
    AnimationTimeline timeline;
    RenderBlock obj;

    CSSTransition trans;
    trans.property = "color";
    trans.duration = 1.0f;
    trans.timing_function = TimingFunction::LINEAR;

    SkColor start = SkColorSetARGB(255, 0, 0, 0);      // 黑色
    SkColor end = SkColorSetARGB(255, 255, 255, 255);  // 白色

    timeline.StartTransition(&obj, "color", trans, start, end);

    timeline.Update(0.0);
    timeline.Update(500.0);  // 0.5 秒

    auto value = timeline.GetCurrentValue(&obj, "color");
    ASSERT_TRUE(value.has_value());

    SkColor result = std::get<SkColor>(*value);
    // 应该是灰色 (127, 127, 127)
    EXPECT_NEAR(SkColorGetR(result), 127, 10);
    EXPECT_NEAR(SkColorGetG(result), 127, 10);
    EXPECT_NEAR(SkColorGetB(result), 127, 10);
}

TEST(AnimationTimelineTest, InterpolateTransform) {
    AnimationTimeline timeline;
    RenderBlock obj;
    
    CSSTransition trans;
    trans.property = "transform";
    trans.duration = 1.0f;
    trans.timing_function = TimingFunction::LINEAR;
    
    CSSTransform start, end;
    start.transforms.push_back(Transform(TransformType::TRANSLATE));
    start.transforms[0].values = {0.0f, 0.0f};
    
    end.transforms.push_back(Transform(TransformType::TRANSLATE));
    end.transforms[0].values = {100.0f, 100.0f};
    
    timeline.StartTransition(&obj, "transform", trans, start, end);
    
    timeline.Update(0.0);
    timeline.Update(500.0);  // 0.5 秒
    
    auto value = timeline.GetCurrentValue(&obj, "transform");
    ASSERT_TRUE(value.has_value());
    
    const CSSTransform& result = std::get<CSSTransform>(*value);
    ASSERT_EQ(result.transforms.size(), 1);
    EXPECT_NEAR(result.transforms[0].values[0], 50.0f, 1.0f);
    EXPECT_NEAR(result.transforms[0].values[1], 50.0f, 1.0f);
}

// ============================================================================
// Easing Tests
// ============================================================================

TEST(AnimationTimelineTest, EaseInTiming) {
    AnimationTimeline timeline;
    RenderBlock obj;

    CSSTransition trans;
    trans.property = "opacity";
    trans.duration = 1.0f;
    trans.timing_function = TimingFunction::EASE_IN;

    timeline.StartTransition(&obj, "opacity", trans, 0.0f, 1.0f);

    timeline.Update(0.0);
    timeline.Update(500.0);  // 0.5 秒

    auto value = timeline.GetCurrentValue(&obj, "opacity");
    ASSERT_TRUE(value.has_value());

    // ease-in 在中点应该小于 0.5 (慢启动)
    float result = std::get<float>(*value);
    EXPECT_LT(result, 0.5f);
    EXPECT_GT(result, 0.0f);
}

TEST(AnimationTimelineTest, EaseOutTiming) {
    AnimationTimeline timeline;
    RenderBlock obj;
    
    CSSTransition trans;
    trans.property = "opacity";
    trans.duration = 1.0f;
    trans.timing_function = TimingFunction::EASE_OUT;
    
    timeline.StartTransition(&obj, "opacity", trans, 0.0f, 1.0f);
    
    timeline.Update(0.0);
    timeline.Update(500.0);  // 0.5 秒
    
    auto value = timeline.GetCurrentValue(&obj, "opacity");
    ASSERT_TRUE(value.has_value());
    
    // ease-out 在中点应该大于 0.5 (快启动)
    float result = std::get<float>(*value);
    EXPECT_GT(result, 0.5f);
    EXPECT_LT(result, 1.0f);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(AnimationTimelinePerformanceTest, ManyTransitions) {
    AnimationTimeline timeline;
    std::vector<RenderBlock> objects(100);
    
    CSSTransition trans;
    trans.duration = 1.0f;
    trans.timing_function = TimingFunction::LINEAR;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 启动 100 个过渡
    for (int i = 0; i < 100; ++i) {
        timeline.StartTransition(&objects[i], "opacity", trans, 0.0f, 1.0f);
    }
    
    // 更新 100 次
    for (int i = 0; i < 100; ++i) {
        timeline.Update(i * 10.0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "100 transitions x 100 updates: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 100);  // 应该小于 100ms
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

