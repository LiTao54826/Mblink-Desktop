/**
 * @file test_animation_bounds_properties.cpp
 * @brief 动画边界计算器属性测试
 *
 * 测试属性：
 * - Property 1: Rotation bounds contain all rotated corners
 * - Property 2: Translation bounds include all positions
 * - Property 6: Scale bounds accommodate maximum scale
 * - Property 3: Combined transform bounds are correct
 *
 * **Feature: animation-clipping-fix**
 */

#include <gtest/gtest.h>
#include <random>
#include <cmath>
#include "core/compositor/animation/animation_bounds_calculator.h"
#include "include/core/SkMatrix.h"

namespace mbink {
namespace testing {

// =========================================================================
// 测试辅助函数
// =========================================================================

/**
 * @brief 随机数生成器
 */
class RandomGenerator {
public:
    RandomGenerator(unsigned int seed = 42) : gen_(seed) {}

    float Float(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }

    int Int(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }

private:
    std::mt19937 gen_;
};

/**
 * @brief 检查点是否在边界内（考虑偏移）
 */
bool IsPointInBounds(const SkPoint& point, const AnimationBounds& bounds) {
    // 将点转换到边界坐标系（减去偏移）
    float local_x = point.fX - bounds.offset.fX;
    float local_y = point.fY - bounds.offset.fY;
    
    // 检查是否在边界内（允许小误差）
    const float epsilon = 0.01f;
    return local_x >= -epsilon && 
           local_x <= bounds.bounds.width() + epsilon &&
           local_y >= -epsilon && 
           local_y <= bounds.bounds.height() + epsilon;
}

/**
 * @brief 旋转点
 */
SkPoint RotatePoint(const SkPoint& point, float angle_deg, const SkPoint& origin) {
    float rad = angle_deg * 3.14159265358979f / 180.0f;
    float cos_a = std::cos(rad);
    float sin_a = std::sin(rad);
    
    float dx = point.fX - origin.fX;
    float dy = point.fY - origin.fY;
    
    return SkPoint::Make(
        origin.fX + dx * cos_a - dy * sin_a,
        origin.fY + dx * sin_a + dy * cos_a
    );
}

// =========================================================================
// Property 1: Rotation bounds contain all rotated corners
// **Feature: animation-clipping-fix, Property 1: Rotation bounds contain all rotated corners**
// **Validates: Requirements 1.2, 3.1, 3.2**
// =========================================================================

class RotationBoundsPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_{12345};
};

TEST_F(RotationBoundsPropertyTest, AllCornersContainedForRandomAngles) {
    // 运行 100 次随机测试
    for (int i = 0; i < 100; ++i) {
        // 生成随机尺寸
        float width = rng_.Float(10.0f, 500.0f);
        float height = rng_.Float(10.0f, 500.0f);
        SkSize size = SkSize::Make(width, height);
        
        // 生成随机旋转角度
        float min_angle = rng_.Float(-360.0f, 0.0f);
        float max_angle = rng_.Float(0.0f, 360.0f);
        
        // 生成随机原点（在元素内部）
        SkPoint origin = SkPoint::Make(
            rng_.Float(0.0f, width),
            rng_.Float(0.0f, height)
        );
        
        // 计算旋转边界
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateRotationBounds(
            size, min_angle, max_angle, origin);
        
        // 验证所有旋转角度下的角点都在边界内
        SkPoint corners[4] = {
            {0, 0}, {width, 0}, {width, height}, {0, height}
        };
        
        // 测试多个角度
        for (float angle = min_angle; angle <= max_angle; angle += 15.0f) {
            for (const auto& corner : corners) {
                SkPoint rotated = RotatePoint(corner, angle, origin);
                
                EXPECT_TRUE(IsPointInBounds(rotated, bounds))
                    << "Corner (" << corner.fX << ", " << corner.fY << ") "
                    << "rotated " << angle << " degrees "
                    << "to (" << rotated.fX << ", " << rotated.fY << ") "
                    << "is outside bounds. "
                    << "Size: " << width << "x" << height << ", "
                    << "Origin: (" << origin.fX << ", " << origin.fY << ")";
            }
        }
    }
}

TEST_F(RotationBoundsPropertyTest, BoundsContainOriginalRect) {
    // 边界应该始终包含原始矩形
    for (int i = 0; i < 50; ++i) {
        float width = rng_.Float(10.0f, 500.0f);
        float height = rng_.Float(10.0f, 500.0f);
        SkSize size = SkSize::Make(width, height);
        
        float angle = rng_.Float(-180.0f, 180.0f);
        SkPoint origin = SkPoint::Make(width / 2, height / 2);
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateRotationBounds(
            size, 0, angle, origin);
        
        // 原始四个角点应该在边界内
        SkPoint corners[4] = {
            {0, 0}, {width, 0}, {width, height}, {0, height}
        };
        
        for (const auto& corner : corners) {
            EXPECT_TRUE(IsPointInBounds(corner, bounds))
                << "Original corner (" << corner.fX << ", " << corner.fY << ") "
                << "is outside bounds";
        }
    }
}

TEST_F(RotationBoundsPropertyTest, NoExpansionForZeroRotation) {
    // 零旋转不需要扩展
    SkSize size = SkSize::Make(100, 100);
    SkPoint origin = SkPoint::Make(50, 50);
    
    AnimationBounds bounds = AnimationBoundsCalculator::CalculateRotationBounds(
        size, 0, 0, origin);
    
    EXPECT_FALSE(bounds.needs_expansion);
}

TEST_F(RotationBoundsPropertyTest, FullRotationUsesCircumscribedCircle) {
    // 完整旋转（360度）应该使用外接圆
    for (int i = 0; i < 50; ++i) {
        float width = rng_.Float(10.0f, 500.0f);
        float height = rng_.Float(10.0f, 500.0f);
        SkSize size = SkSize::Make(width, height);
        
        // 中心原点
        SkPoint origin = SkPoint::Make(width / 2, height / 2);
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateRotationBounds(
            size, 0, 360, origin);
        
        // 对角线长度
        float diagonal = std::sqrt(width * width + height * height);
        
        // 边界应该至少是对角线长度
        EXPECT_GE(bounds.bounds.width(), diagonal - 0.1f)
            << "Bounds width should be at least diagonal length";
        EXPECT_GE(bounds.bounds.height(), diagonal - 0.1f)
            << "Bounds height should be at least diagonal length";
    }
}

// =========================================================================
// Property 2: Translation bounds include all positions
// **Feature: animation-clipping-fix, Property 2: Translation bounds include all positions**
// **Validates: Requirements 1.3, 4.1, 4.2, 4.3**
// =========================================================================

class TranslationBoundsPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_{54321};
};

TEST_F(TranslationBoundsPropertyTest, AllTranslationsContained) {
    // 运行 100 次随机测试
    for (int i = 0; i < 100; ++i) {
        float width = rng_.Float(10.0f, 200.0f);
        float height = rng_.Float(10.0f, 200.0f);
        SkSize size = SkSize::Make(width, height);
        
        // 生成随机位移列表
        int num_translations = rng_.Int(2, 10);
        std::vector<SkPoint> translations;
        for (int j = 0; j < num_translations; ++j) {
            translations.push_back(SkPoint::Make(
                rng_.Float(-200.0f, 200.0f),
                rng_.Float(-200.0f, 200.0f)
            ));
        }
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateTranslationBounds(
            size, translations);
        
        // 验证每个位移位置的元素都在边界内
        for (const auto& trans : translations) {
            // 位移后的四个角点
            SkPoint corners[4] = {
                {trans.fX, trans.fY},
                {trans.fX + width, trans.fY},
                {trans.fX + width, trans.fY + height},
                {trans.fX, trans.fY + height}
            };
            
            for (const auto& corner : corners) {
                EXPECT_TRUE(IsPointInBounds(corner, bounds))
                    << "Translated corner (" << corner.fX << ", " << corner.fY << ") "
                    << "is outside bounds";
            }
        }
    }
}

TEST_F(TranslationBoundsPropertyTest, OriginalPositionContained) {
    // 原始位置（无位移）应该始终在边界内
    for (int i = 0; i < 50; ++i) {
        float width = rng_.Float(10.0f, 200.0f);
        float height = rng_.Float(10.0f, 200.0f);
        SkSize size = SkSize::Make(width, height);
        
        std::vector<SkPoint> translations = {
            SkPoint::Make(0, 0),  // 原始位置
            SkPoint::Make(rng_.Float(-100.0f, 100.0f), rng_.Float(-100.0f, 100.0f))
        };
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateTranslationBounds(
            size, translations);
        
        // 原始位置的角点
        SkPoint corners[4] = {
            {0, 0}, {width, 0}, {width, height}, {0, height}
        };
        
        for (const auto& corner : corners) {
            EXPECT_TRUE(IsPointInBounds(corner, bounds))
                << "Original corner should be in bounds";
        }
    }
}

TEST_F(TranslationBoundsPropertyTest, NegativeTranslationsHandled) {
    // 负位移应该正确处理
    SkSize size = SkSize::Make(100, 100);
    
    std::vector<SkPoint> translations = {
        SkPoint::Make(0, 0),
        SkPoint::Make(-50, -50),
        SkPoint::Make(-100, 0),
        SkPoint::Make(0, -100)
    };
    
    AnimationBounds bounds = AnimationBoundsCalculator::CalculateTranslationBounds(
        size, translations);
    
    // 偏移应该是负值
    EXPECT_LE(bounds.offset.fX, 0);
    EXPECT_LE(bounds.offset.fY, 0);
    
    // 验证所有位置都在边界内
    for (const auto& trans : translations) {
        SkPoint corner = SkPoint::Make(trans.fX, trans.fY);
        EXPECT_TRUE(IsPointInBounds(corner, bounds));
    }
}

// =========================================================================
// Property 6: Scale bounds accommodate maximum scale
// **Feature: animation-clipping-fix, Property 6: Scale bounds accommodate maximum scale**
// **Validates: Requirements 1.4**
// =========================================================================

class ScaleBoundsPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_{67890};
};

TEST_F(ScaleBoundsPropertyTest, MaxScaleContained) {
    // 运行 100 次随机测试
    for (int i = 0; i < 100; ++i) {
        float width = rng_.Float(10.0f, 200.0f);
        float height = rng_.Float(10.0f, 200.0f);
        SkSize size = SkSize::Make(width, height);
        
        float max_scale_x = rng_.Float(1.0f, 3.0f);
        float max_scale_y = rng_.Float(1.0f, 3.0f);
        
        // 中心原点
        SkPoint origin = SkPoint::Make(width / 2, height / 2);
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateScaleBounds(
            size, max_scale_x, max_scale_y, origin);
        
        // 计算缩放后的角点
        float scaled_width = width * max_scale_x;
        float scaled_height = height * max_scale_y;
        
        // 缩放后的角点（相对于原点缩放）
        SkPoint scaled_corners[4] = {
            {origin.fX - scaled_width / 2, origin.fY - scaled_height / 2},
            {origin.fX + scaled_width / 2, origin.fY - scaled_height / 2},
            {origin.fX + scaled_width / 2, origin.fY + scaled_height / 2},
            {origin.fX - scaled_width / 2, origin.fY + scaled_height / 2}
        };
        
        for (const auto& corner : scaled_corners) {
            EXPECT_TRUE(IsPointInBounds(corner, bounds))
                << "Scaled corner (" << corner.fX << ", " << corner.fY << ") "
                << "is outside bounds. "
                << "Scale: " << max_scale_x << "x" << max_scale_y;
        }
    }
}

TEST_F(ScaleBoundsPropertyTest, NoExpansionForUnitScale) {
    // 单位缩放不需要扩展
    SkSize size = SkSize::Make(100, 100);
    SkPoint origin = SkPoint::Make(50, 50);
    
    AnimationBounds bounds = AnimationBoundsCalculator::CalculateScaleBounds(
        size, 1.0f, 1.0f, origin);
    
    EXPECT_FALSE(bounds.needs_expansion);
}

TEST_F(ScaleBoundsPropertyTest, OffCenterOriginHandled) {
    // 非中心原点应该正确处理
    for (int i = 0; i < 50; ++i) {
        float width = rng_.Float(50.0f, 200.0f);
        float height = rng_.Float(50.0f, 200.0f);
        SkSize size = SkSize::Make(width, height);
        
        float max_scale = rng_.Float(1.5f, 2.5f);
        
        // 随机原点
        SkPoint origin = SkPoint::Make(
            rng_.Float(0.0f, width),
            rng_.Float(0.0f, height)
        );
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateScaleBounds(
            size, max_scale, max_scale, origin);
        
        // 使用矩阵计算缩放后的角点
        SkMatrix scale_matrix;
        scale_matrix.setScale(max_scale, max_scale, origin.fX, origin.fY);
        
        SkPoint corners[4] = {
            {0, 0}, {width, 0}, {width, height}, {0, height}
        };
        scale_matrix.mapPoints(corners, 4);
        
        for (const auto& corner : corners) {
            EXPECT_TRUE(IsPointInBounds(corner, bounds))
                << "Scaled corner with off-center origin should be in bounds";
        }
    }
}

// =========================================================================
// 边界计算一致性测试
// =========================================================================

class BoundsConsistencyTest : public ::testing::Test {
protected:
    RandomGenerator rng_{11111};
};

TEST_F(BoundsConsistencyTest, BoundsAreNonNegative) {
    // 边界尺寸应该始终为非负
    for (int i = 0; i < 100; ++i) {
        float width = rng_.Float(1.0f, 500.0f);
        float height = rng_.Float(1.0f, 500.0f);
        SkSize size = SkSize::Make(width, height);
        
        float angle = rng_.Float(-360.0f, 360.0f);
        SkPoint origin = SkPoint::Make(width / 2, height / 2);
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateRotationBounds(
            size, 0, angle, origin);
        
        if (bounds.needs_expansion) {
            EXPECT_GE(bounds.bounds.width(), 0);
            EXPECT_GE(bounds.bounds.height(), 0);
        }
    }
}

TEST_F(BoundsConsistencyTest, ExpandedBoundsLargerThanOriginal) {
    // 扩展后的边界应该不小于原始尺寸
    for (int i = 0; i < 100; ++i) {
        float width = rng_.Float(10.0f, 200.0f);
        float height = rng_.Float(10.0f, 200.0f);
        SkSize size = SkSize::Make(width, height);
        
        float angle = rng_.Float(10.0f, 350.0f);  // 非零旋转
        SkPoint origin = SkPoint::Make(width / 2, height / 2);
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateRotationBounds(
            size, 0, angle, origin);
        
        if (bounds.needs_expansion) {
            EXPECT_GE(bounds.bounds.width(), width - 0.1f);
            EXPECT_GE(bounds.bounds.height(), height - 0.1f);
        }
    }
}

// =========================================================================
// Property 3: Combined transform bounds are correct
// **Feature: animation-clipping-fix, Property 3: Combined transform bounds are correct**
// **Validates: Requirements 1.1, 3.4, 4.4**
// =========================================================================

class CombinedTransformBoundsPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_{99999};
};

TEST_F(CombinedTransformBoundsPropertyTest, AllTransformedCornersContained) {
    // 运行 100 次随机测试
    for (int i = 0; i < 100; ++i) {
        float width = rng_.Float(20.0f, 200.0f);
        float height = rng_.Float(20.0f, 200.0f);
        SkSize size = SkSize::Make(width, height);
        
        // 生成随机变换矩阵列表
        int num_transforms = rng_.Int(2, 8);
        std::vector<SkMatrix> transforms;
        
        for (int j = 0; j < num_transforms; ++j) {
            SkMatrix matrix = SkMatrix::I();
            
            // 随机选择变换类型
            int transform_type = rng_.Int(0, 2);
            
            if (transform_type == 0) {
                // 旋转
                float angle = rng_.Float(-180.0f, 180.0f);
                matrix.setRotate(angle, width / 2, height / 2);
            } else if (transform_type == 1) {
                // 位移
                float tx = rng_.Float(-100.0f, 100.0f);
                float ty = rng_.Float(-100.0f, 100.0f);
                matrix.setTranslate(tx, ty);
            } else {
                // 缩放
                float scale = rng_.Float(0.5f, 2.0f);
                matrix.setScale(scale, scale, width / 2, height / 2);
            }
            
            transforms.push_back(matrix);
        }
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateUnionBounds(
            size, transforms);
        
        // 验证每个变换后的角点都在边界内
        SkRect original_rect = SkRect::MakeWH(width, height);
        
        for (const auto& transform : transforms) {
            SkPoint corners[4] = {
                {0, 0}, {width, 0}, {width, height}, {0, height}
            };
            transform.mapPoints(corners, 4);
            
            for (const auto& corner : corners) {
                EXPECT_TRUE(IsPointInBounds(corner, bounds))
                    << "Transformed corner (" << corner.fX << ", " << corner.fY << ") "
                    << "is outside bounds";
            }
        }
    }
}

TEST_F(CombinedTransformBoundsPropertyTest, RotationPlusTranslation) {
    // 测试旋转 + 位移的组合
    for (int i = 0; i < 50; ++i) {
        float width = rng_.Float(50.0f, 150.0f);
        float height = rng_.Float(50.0f, 150.0f);
        SkSize size = SkSize::Make(width, height);
        
        std::vector<SkMatrix> transforms;
        
        // 添加旋转变换
        float angle = rng_.Float(-90.0f, 90.0f);
        SkMatrix rotate;
        rotate.setRotate(angle, width / 2, height / 2);
        transforms.push_back(rotate);
        
        // 添加位移变换
        float tx = rng_.Float(-50.0f, 50.0f);
        float ty = rng_.Float(-50.0f, 50.0f);
        SkMatrix translate;
        translate.setTranslate(tx, ty);
        transforms.push_back(translate);
        
        // 添加组合变换
        SkMatrix combined;
        combined.setRotate(angle, width / 2, height / 2);
        combined.postTranslate(tx, ty);
        transforms.push_back(combined);
        
        AnimationBounds bounds = AnimationBoundsCalculator::CalculateUnionBounds(
            size, transforms);
        
        // 验证所有变换后的角点都在边界内
        for (const auto& transform : transforms) {
            SkPoint corners[4] = {
                {0, 0}, {width, 0}, {width, height}, {0, height}
            };
            transform.mapPoints(corners, 4);
            
            for (const auto& corner : corners) {
                EXPECT_TRUE(IsPointInBounds(corner, bounds))
                    << "Combined transform corner should be in bounds";
            }
        }
    }
}

TEST_F(CombinedTransformBoundsPropertyTest, EmptyTransformsReturnNoExpansion) {
    SkSize size = SkSize::Make(100, 100);
    std::vector<SkMatrix> empty_transforms;
    
    AnimationBounds bounds = AnimationBoundsCalculator::CalculateUnionBounds(
        size, empty_transforms);
    
    EXPECT_FALSE(bounds.needs_expansion);
}

TEST_F(CombinedTransformBoundsPropertyTest, IdentityTransformsNoExpansion) {
    SkSize size = SkSize::Make(100, 100);
    std::vector<SkMatrix> identity_transforms = {
        SkMatrix::I(),
        SkMatrix::I(),
        SkMatrix::I()
    };
    
    AnimationBounds bounds = AnimationBoundsCalculator::CalculateUnionBounds(
        size, identity_transforms);
    
    EXPECT_FALSE(bounds.needs_expansion);
}

} // namespace testing
} // namespace mbink
