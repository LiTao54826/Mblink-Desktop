/**
 * @file test_transform.cpp
 * @brief CSS Transform 单元测试
 */

#include <gtest/gtest.h>
#include "render/transform.h"
#include <cmath>

namespace lightui {
namespace test {

class TransformTest : public ::testing::Test {};

// ========== 基本变换测试 ==========

TEST_F(TransformTest, EmptyTransform) {
    auto t = CSSTransform::Parse("none");
    ASSERT_TRUE(t.has_value());
    EXPECT_TRUE(t->IsEmpty());
}

// ========== CSS 解析测试 ==========

TEST_F(TransformTest, ParseTranslate) {
    auto t = CSSTransform::Parse("translate(100px, 50px)");
    ASSERT_TRUE(t.has_value());
    EXPECT_FALSE(t->IsEmpty());
    EXPECT_EQ(t->transforms.size(), 1);
    EXPECT_EQ(t->transforms[0].type, TransformType::TRANSLATE);
}

TEST_F(TransformTest, ParseTranslateX) {
    auto t = CSSTransform::Parse("translateX(100px)");
    ASSERT_TRUE(t.has_value());
    EXPECT_FALSE(t->IsEmpty());
}

TEST_F(TransformTest, ParseTranslateY) {
    auto t = CSSTransform::Parse("translateY(50px)");
    ASSERT_TRUE(t.has_value());
    EXPECT_FALSE(t->IsEmpty());
}

TEST_F(TransformTest, ParseScale) {
    auto t = CSSTransform::Parse("scale(2)");
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(t->transforms.size(), 1);
    EXPECT_EQ(t->transforms[0].type, TransformType::SCALE);
}

TEST_F(TransformTest, ParseScaleXY) {
    auto t = CSSTransform::Parse("scale(2, 3)");
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(t->transforms[0].type, TransformType::SCALE);
}

TEST_F(TransformTest, ParseRotate) {
    auto t = CSSTransform::Parse("rotate(90deg)");
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(t->transforms.size(), 1);
    EXPECT_EQ(t->transforms[0].type, TransformType::ROTATE);
}

TEST_F(TransformTest, ParseRotateRad) {
    auto t = CSSTransform::Parse("rotate(1.5708rad)");  // π/2
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(t->transforms[0].type, TransformType::ROTATE);
}

TEST_F(TransformTest, ParseSkew) {
    auto t = CSSTransform::Parse("skew(45deg, 0deg)");
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(t->transforms[0].type, TransformType::SKEW);
}

TEST_F(TransformTest, ParseMatrix) {
    // matrix(a, b, c, d, tx, ty)
    auto t = CSSTransform::Parse("matrix(1, 0, 0, 1, 100, 50)");
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(t->transforms[0].type, TransformType::MATRIX);
}

TEST_F(TransformTest, ParseMultiple) {
    auto t = CSSTransform::Parse("translate(100px, 100px) scale(2)");
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(t->transforms.size(), 2);
    EXPECT_EQ(t->transforms[0].type, TransformType::TRANSLATE);
    EXPECT_EQ(t->transforms[1].type, TransformType::SCALE);
}

TEST_F(TransformTest, ParseNone) {
    auto t = CSSTransform::Parse("none");
    ASSERT_TRUE(t.has_value());
    EXPECT_TRUE(t->IsEmpty());
}

// ========== 转换为 SkMatrix 测试 ==========

TEST_F(TransformTest, ToSkMatrix) {
    auto t = CSSTransform::Parse("translate(100px, 50px)");
    ASSERT_TRUE(t.has_value());
    
    SkRect rect = SkRect::MakeWH(200, 200);
    TransformOrigin origin;
    SkMatrix matrix = t->ToSkMatrix(rect, origin);

    EXPECT_FLOAT_EQ(matrix.getTranslateX(), 100);
    EXPECT_FLOAT_EQ(matrix.getTranslateY(), 50);
}

TEST_F(TransformTest, TransformOriginDefault) {
    TransformOrigin origin;
    SkRect rect = SkRect::MakeWH(200, 200);
    SkPoint point = origin.ToPoint(rect);
    
    // Default is center center (50%, 50%)
    EXPECT_FLOAT_EQ(point.x(), 100);
    EXPECT_FLOAT_EQ(point.y(), 100);
}

} // namespace test
} // namespace lightui
