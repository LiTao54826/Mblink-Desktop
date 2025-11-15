/**
 * @file test_transform.cpp
 * @brief CSS Transform 单元测试
 */

#include <gtest/gtest.h>
#include "render/transform.h"
#include <cmath>
#include <chrono>
#include <iostream>

using namespace lightui;

// ========== TransformOrigin 测试 ==========

TEST(TransformOriginTest, DefaultOrigin) {
    TransformOrigin origin;
    
    // 默认应该是 center center (50%, 50%)
    EXPECT_EQ(origin.x.value, 50.0f);
    EXPECT_EQ(origin.x.unit, CSSUnit::PERCENT);
    EXPECT_EQ(origin.y.value, 50.0f);
    EXPECT_EQ(origin.y.unit, CSSUnit::PERCENT);
}

TEST(TransformOriginTest, ToPoint) {
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 100);
    
    // center center (50%, 50%)
    TransformOrigin origin1(
        CSSLength(50.0f, CSSUnit::PERCENT),
        CSSLength(50.0f, CSSUnit::PERCENT)
    );
    SkPoint point1 = origin1.ToPoint(rect);
    EXPECT_FLOAT_EQ(point1.x(), 200.0f);  // 100 + 200 * 0.5
    EXPECT_FLOAT_EQ(point1.y(), 150.0f);  // 100 + 100 * 0.5
    
    // top left (0%, 0%)
    TransformOrigin origin2(
        CSSLength(0.0f, CSSUnit::PERCENT),
        CSSLength(0.0f, CSSUnit::PERCENT)
    );
    SkPoint point2 = origin2.ToPoint(rect);
    EXPECT_FLOAT_EQ(point2.x(), 100.0f);
    EXPECT_FLOAT_EQ(point2.y(), 100.0f);
    
    // bottom right (100%, 100%)
    TransformOrigin origin3(
        CSSLength(100.0f, CSSUnit::PERCENT),
        CSSLength(100.0f, CSSUnit::PERCENT)
    );
    SkPoint point3 = origin3.ToPoint(rect);
    EXPECT_FLOAT_EQ(point3.x(), 300.0f);  // 100 + 200
    EXPECT_FLOAT_EQ(point3.y(), 200.0f);  // 100 + 100
}

// ========== ParseTransformOrigin 测试 ==========

TEST(ParseTransformOriginTest, Keywords) {
    // center center
    auto origin1 = ParseTransformOrigin("center center");
    ASSERT_TRUE(origin1.has_value());
    EXPECT_EQ(origin1->x.value, 50.0f);
    EXPECT_EQ(origin1->y.value, 50.0f);
    
    // left top
    auto origin2 = ParseTransformOrigin("left top");
    ASSERT_TRUE(origin2.has_value());
    EXPECT_EQ(origin2->x.value, 0.0f);
    EXPECT_EQ(origin2->y.value, 0.0f);
    
    // right bottom
    auto origin3 = ParseTransformOrigin("right bottom");
    ASSERT_TRUE(origin3.has_value());
    EXPECT_EQ(origin3->x.value, 100.0f);
    EXPECT_EQ(origin3->y.value, 100.0f);
}

TEST(ParseTransformOriginTest, Percentages) {
    auto origin = ParseTransformOrigin("25% 75%");
    ASSERT_TRUE(origin.has_value());
    EXPECT_EQ(origin->x.value, 25.0f);
    EXPECT_EQ(origin->x.unit, CSSUnit::PERCENT);
    EXPECT_EQ(origin->y.value, 75.0f);
    EXPECT_EQ(origin->y.unit, CSSUnit::PERCENT);
}

TEST(ParseTransformOriginTest, Pixels) {
    auto origin = ParseTransformOrigin("10px 20px");
    ASSERT_TRUE(origin.has_value());
    EXPECT_EQ(origin->x.value, 10.0f);
    EXPECT_EQ(origin->x.unit, CSSUnit::PX);
    EXPECT_EQ(origin->y.value, 20.0f);
    EXPECT_EQ(origin->y.unit, CSSUnit::PX);
}

// ========== CSSTransform 解析测试 ==========

TEST(CSSTransformTest, ParseTranslate) {
    auto transform = CSSTransform::Parse("translate(10px, 20px)");
    ASSERT_TRUE(transform.has_value());
    ASSERT_EQ(transform->transforms.size(), 1);
    EXPECT_EQ(transform->transforms[0].type, TransformType::TRANSLATE);
    EXPECT_EQ(transform->transforms[0].values.size(), 2);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[0], 10.0f);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[1], 20.0f);
}

TEST(CSSTransformTest, ParseRotate) {
    auto transform = CSSTransform::Parse("rotate(45deg)");
    ASSERT_TRUE(transform.has_value());
    ASSERT_EQ(transform->transforms.size(), 1);
    EXPECT_EQ(transform->transforms[0].type, TransformType::ROTATE);
    EXPECT_EQ(transform->transforms[0].values.size(), 1);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[0], 45.0f);
}

TEST(CSSTransformTest, ParseScale) {
    auto transform = CSSTransform::Parse("scale(1.5, 2.0)");
    ASSERT_TRUE(transform.has_value());
    ASSERT_EQ(transform->transforms.size(), 1);
    EXPECT_EQ(transform->transforms[0].type, TransformType::SCALE);
    EXPECT_EQ(transform->transforms[0].values.size(), 2);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[0], 1.5f);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[1], 2.0f);
}

TEST(CSSTransformTest, ParseSkew) {
    auto transform = CSSTransform::Parse("skew(10deg, 20deg)");
    ASSERT_TRUE(transform.has_value());
    ASSERT_EQ(transform->transforms.size(), 1);
    EXPECT_EQ(transform->transforms[0].type, TransformType::SKEW);
    EXPECT_EQ(transform->transforms[0].values.size(), 2);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[0], 10.0f);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[1], 20.0f);
}

TEST(CSSTransformTest, ParseMatrix) {
    auto transform = CSSTransform::Parse("matrix(1, 0, 0, 1, 10, 20)");
    ASSERT_TRUE(transform.has_value());
    ASSERT_EQ(transform->transforms.size(), 1);
    EXPECT_EQ(transform->transforms[0].type, TransformType::MATRIX);
    EXPECT_EQ(transform->transforms[0].values.size(), 6);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[0], 1.0f);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[4], 10.0f);
    EXPECT_FLOAT_EQ(transform->transforms[0].values[5], 20.0f);
}

TEST(CSSTransformTest, ParseMultiple) {
    auto transform = CSSTransform::Parse("translate(10px, 20px) rotate(45deg) scale(1.5)");
    ASSERT_TRUE(transform.has_value());
    ASSERT_EQ(transform->transforms.size(), 3);
    
    EXPECT_EQ(transform->transforms[0].type, TransformType::TRANSLATE);
    EXPECT_EQ(transform->transforms[1].type, TransformType::ROTATE);
    EXPECT_EQ(transform->transforms[2].type, TransformType::SCALE);
}

TEST(CSSTransformTest, ParseNone) {
    auto transform = CSSTransform::Parse("none");
    ASSERT_TRUE(transform.has_value());
    EXPECT_TRUE(transform->IsEmpty());
}

TEST(CSSTransformTest, ParseEmpty) {
    auto transform = CSSTransform::Parse("");
    ASSERT_TRUE(transform.has_value());
    EXPECT_TRUE(transform->IsEmpty());
}

// ========== ToSkMatrix 测试 ==========

TEST(CSSTransformTest, ToSkMatrixTranslate) {
    auto transform = CSSTransform::Parse("translate(10px, 20px)");
    ASSERT_TRUE(transform.has_value());
    
    SkRect rect = SkRect::MakeXYWH(0, 0, 100, 100);
    TransformOrigin origin;  // center center
    
    SkMatrix matrix = transform->ToSkMatrix(rect, origin);
    
    // 验证平移
    SkPoint point = SkPoint::Make(0, 0);
    matrix.mapPoints(&point, 1);
    EXPECT_FLOAT_EQ(point.x(), 10.0f);
    EXPECT_FLOAT_EQ(point.y(), 20.0f);
}

TEST(CSSTransformTest, ToSkMatrixRotate) {
    auto transform = CSSTransform::Parse("rotate(90deg)");
    ASSERT_TRUE(transform.has_value());
    
    SkRect rect = SkRect::MakeXYWH(0, 0, 100, 100);
    TransformOrigin origin;  // center center (50, 50)
    
    SkMatrix matrix = transform->ToSkMatrix(rect, origin);
    
    // 验证旋转（绕中心点旋转 90 度）
    // 右边中点 (100, 50) 绕中心 (50, 50) 旋转 90 度 -> (50, 100)
    SkPoint point = SkPoint::Make(100, 50);  // 右边中点
    matrix.mapPoints(&point, 1);
    EXPECT_NEAR(point.x(), 50.0f, 0.1f);
    EXPECT_NEAR(point.y(), 100.0f, 0.1f);  // 应该是 100，不是 0
}

TEST(CSSTransformTest, ToSkMatrixScale) {
    auto transform = CSSTransform::Parse("scale(2.0, 2.0)");
    ASSERT_TRUE(transform.has_value());
    
    SkRect rect = SkRect::MakeXYWH(0, 0, 100, 100);
    TransformOrigin origin;  // center center
    
    SkMatrix matrix = transform->ToSkMatrix(rect, origin);
    
    // 验证缩放（绕中心点缩放 2 倍）
    SkPoint point = SkPoint::Make(100, 100);  // 右下角
    matrix.mapPoints(&point, 1);
    EXPECT_NEAR(point.x(), 150.0f, 0.1f);  // (100-50)*2 + 50
    EXPECT_NEAR(point.y(), 150.0f, 0.1f);
}

TEST(CSSTransformTest, ToSkMatrixCombined) {
    auto transform = CSSTransform::Parse("translate(10px, 20px) rotate(45deg) scale(1.5)");
    ASSERT_TRUE(transform.has_value());
    
    SkRect rect = SkRect::MakeXYWH(0, 0, 100, 100);
    TransformOrigin origin;
    
    SkMatrix matrix = transform->ToSkMatrix(rect, origin);
    
    // 验证组合变换（只检查矩阵不是单位矩阵）
    EXPECT_FALSE(matrix.isIdentity());
}

// ========== 性能测试 ==========

TEST(CSSTransformTest, PerformanceParsing) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        auto transform = CSSTransform::Parse("translate(10px, 20px) rotate(45deg) scale(1.5)");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "解析 1000 个 transform: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 100);  // 应该小于 100ms
}

TEST(CSSTransformTest, PerformanceMatrixConversion) {
    auto transform = CSSTransform::Parse("translate(10px, 20px) rotate(45deg) scale(1.5)");
    ASSERT_TRUE(transform.has_value());
    
    SkRect rect = SkRect::MakeXYWH(0, 0, 100, 100);
    TransformOrigin origin;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; i++) {
        SkMatrix matrix = transform->ToSkMatrix(rect, origin);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "转换 10000 个 matrix: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 100);  // 应该小于 100ms
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

