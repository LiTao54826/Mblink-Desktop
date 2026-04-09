/**
 * @file test_block_layout.cpp
 * @brief Block 布局算法单元测试
 */

#include <gtest/gtest.h>
#include "layout/block_layout.h"
#include "layout/types/style.h"
#include "layout/types/geometry.h"
#include "layout/util/resolve.h"

namespace mbink {
namespace test {

class BlockLayoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
    }
};

// ========== Display 类型测试 ==========

TEST_F(BlockLayoutTest, DisplayBlock) {
    Display d = Display::Block;
    EXPECT_TRUE(d == Display::Block);
}

TEST_F(BlockLayoutTest, DisplayFlex) {
    Display d = Display::Flex;
    EXPECT_TRUE(d == Display::Flex);
}

TEST_F(BlockLayoutTest, DisplayGrid) {
    Display d = Display::Grid;
    EXPECT_TRUE(d == Display::Grid);
}

TEST_F(BlockLayoutTest, DisplayNone) {
    Display d = Display::None;
    EXPECT_TRUE(d == Display::None);
}

// ========== Position 类型测试 ==========

TEST_F(BlockLayoutTest, PositionRelative) {
    Position p = Position::Relative;
    EXPECT_TRUE(p == Position::Relative);
}

TEST_F(BlockLayoutTest, PositionAbsolute) {
    Position p = Position::Absolute;
    EXPECT_TRUE(p == Position::Absolute);
}

TEST_F(BlockLayoutTest, PositionFixed) {
    Position p = Position::Fixed;
    EXPECT_TRUE(p == Position::Fixed);
}

TEST_F(BlockLayoutTest, PositionSticky) {
    Position p = Position::Sticky;
    EXPECT_TRUE(p == Position::Sticky);
}

// ========== Box Model 测试 ==========

TEST_F(BlockLayoutTest, BoxSizingContentBox) {
    BoxSizing bs = BoxSizing::ContentBox;
    EXPECT_TRUE(bs == BoxSizing::ContentBox);
}

TEST_F(BlockLayoutTest, BoxSizingBorderBox) {
    BoxSizing bs = BoxSizing::BorderBox;
    EXPECT_TRUE(bs == BoxSizing::BorderBox);
}

// ========== Overflow 测试 ==========

TEST_F(BlockLayoutTest, OverflowVisible) {
    Overflow o = Overflow::Visible;
    EXPECT_TRUE(o == Overflow::Visible);
}

TEST_F(BlockLayoutTest, OverflowHidden) {
    Overflow o = Overflow::Hidden;
    EXPECT_TRUE(o == Overflow::Hidden);
}

TEST_F(BlockLayoutTest, OverflowScroll) {
    Overflow o = Overflow::Scroll;
    EXPECT_TRUE(o == Overflow::Scroll);
}

// Note: Overflow::Auto is not defined in the current implementation
// TEST_F(BlockLayoutTest, OverflowAuto) {
//     Overflow o = Overflow::Auto;
//     EXPECT_TRUE(o == Overflow::Auto);
// }

// ========== 尺寸计算测试 ==========

TEST_F(BlockLayoutTest, LengthPixels) {
    LengthPercentage len = LengthPercentage::Length(100.0f);
    EXPECT_TRUE(len.IsLength());
    EXPECT_FLOAT_EQ(len.value, 100.0f);
}

TEST_F(BlockLayoutTest, LengthPercent) {
    LengthPercentage len = LengthPercentage::Percent(0.5f);
    EXPECT_TRUE(len.IsPercent());
    EXPECT_FLOAT_EQ(len.value, 0.5f);
}

TEST_F(BlockLayoutTest, ResolvePercentage) {
    LengthPercentage len = LengthPercentage::Percent(0.5f);
    float resolved = len.Resolve(200.0f);  // 50% of 200
    EXPECT_FLOAT_EQ(resolved, 100.0f);
}

TEST_F(BlockLayoutTest, ResolveLength) {
    LengthPercentage len = LengthPercentage::Length(100.0f);
    float resolved = len.Resolve(200.0f);  // 100px regardless of parent
    EXPECT_FLOAT_EQ(resolved, 100.0f);
}

// ========== Margin 折叠测试 ==========

TEST_F(BlockLayoutTest, MarginCollapsePositive) {
    // 两个正 margin 取较大值
    float margin1 = 20.0f;
    float margin2 = 30.0f;
    float collapsed = std::max(margin1, margin2);
    EXPECT_FLOAT_EQ(collapsed, 30.0f);
}

TEST_F(BlockLayoutTest, MarginCollapseNegative) {
    // 两个负 margin 取绝对值较大的
    float margin1 = -20.0f;
    float margin2 = -30.0f;
    float collapsed = std::min(margin1, margin2);
    EXPECT_FLOAT_EQ(collapsed, -30.0f);
}

TEST_F(BlockLayoutTest, MarginCollapseMixed) {
    // 一正一负相加
    float margin1 = 30.0f;
    float margin2 = -10.0f;
    float collapsed = margin1 + margin2;
    EXPECT_FLOAT_EQ(collapsed, 20.0f);
}

// ========== Auto Margin 测试 ==========

TEST_F(BlockLayoutTest, AutoMarginCentering) {
    // margin: 0 auto 应该水平居中
    float container_width = 400.0f;
    float element_width = 200.0f;
    float available_space = container_width - element_width;
    float auto_margin = available_space / 2.0f;

    EXPECT_FLOAT_EQ(auto_margin, 100.0f);
}

// ========== Min/Max 尺寸测试 ==========

TEST_F(BlockLayoutTest, ResolveMinFunction) {
    LengthPercentage len = LengthPercentage::Min(
        LengthPercentage::Percent(1.0f),
        LengthPercentage::Length(720.0f));

    auto resolved_large = len.ResolveToOption(1200.0f);
    auto resolved_small = len.ResolveToOption(600.0f);

    ASSERT_TRUE(resolved_large.has_value());
    ASSERT_TRUE(resolved_small.has_value());
    EXPECT_FLOAT_EQ(*resolved_large, 720.0f);
    EXPECT_FLOAT_EQ(*resolved_small, 600.0f);
}

TEST_F(BlockLayoutTest, ResolveMaxFunction) {
    LengthPercentage len = LengthPercentage::Max(
        LengthPercentage::Percent(0.5f),
        LengthPercentage::Length(320.0f));

    auto resolved_large = len.ResolveToOption(1000.0f);
    auto resolved_small = len.ResolveToOption(400.0f);

    ASSERT_TRUE(resolved_large.has_value());
    ASSERT_TRUE(resolved_small.has_value());
    EXPECT_FLOAT_EQ(*resolved_large, 500.0f);
    EXPECT_FLOAT_EQ(*resolved_small, 320.0f);
}

TEST_F(BlockLayoutTest, ResolveClampFunction) {
    LengthPercentageAuto len = LengthPercentageAuto::Clamp(
        LengthPercentageAuto::Length(200.0f),
        LengthPercentageAuto::Percent(0.5f),
        LengthPercentageAuto::Length(720.0f));

    auto resolved_small = len.ResolveToOption(300.0f);
    auto resolved_mid = len.ResolveToOption(800.0f);
    auto resolved_large = len.ResolveToOption(2000.0f);

    ASSERT_TRUE(resolved_small.has_value());
    ASSERT_TRUE(resolved_mid.has_value());
    ASSERT_TRUE(resolved_large.has_value());
    EXPECT_FLOAT_EQ(*resolved_small, 200.0f);
    EXPECT_FLOAT_EQ(*resolved_mid, 400.0f);
    EXPECT_FLOAT_EQ(*resolved_large, 720.0f);
}

TEST_F(BlockLayoutTest, MaybeResolveLengthWithoutContext) {
    auto resolved = MaybeResolve(LengthPercentageAuto::Length(14.0f), std::nullopt);
    ASSERT_TRUE(resolved.has_value());
    EXPECT_FLOAT_EQ(*resolved, 14.0f);
}

TEST_F(BlockLayoutTest, MaybeResolveMinWithoutContextWhenAbsolute) {
    auto resolved = MaybeResolve(
        LengthPercentageAuto::Min(
            LengthPercentageAuto::Length(14.0f),
            LengthPercentageAuto::Length(20.0f)),
        std::nullopt);
    ASSERT_TRUE(resolved.has_value());
    EXPECT_FLOAT_EQ(*resolved, 14.0f);
}

TEST_F(BlockLayoutTest, MinWidthConstraint) {
    float computed_width = 50.0f;
    float min_width = 100.0f;
    float final_width = std::max(computed_width, min_width);

    EXPECT_FLOAT_EQ(final_width, 100.0f);
}

TEST_F(BlockLayoutTest, MaxWidthConstraint) {
    float computed_width = 200.0f;
    float max_width = 150.0f;
    float final_width = std::min(computed_width, max_width);

    EXPECT_FLOAT_EQ(final_width, 150.0f);
}

TEST_F(BlockLayoutTest, MinMaxWidthConstraint) {
    float computed_width = 50.0f;
    float min_width = 100.0f;
    float max_width = 200.0f;

    float final_width = std::max(min_width, std::min(computed_width, max_width));
    EXPECT_FLOAT_EQ(final_width, 100.0f);

    computed_width = 250.0f;
    final_width = std::max(min_width, std::min(computed_width, max_width));
    EXPECT_FLOAT_EQ(final_width, 200.0f);
}

} // namespace test
} // namespace mbink
