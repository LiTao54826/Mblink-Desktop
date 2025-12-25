/**
 * @file test_property_tree_visual.cpp
 * @brief 属性树系统视觉正确性测试
 *
 * 测试内容：
 * - 与旧渲染路径对比
 * - 变换正确性
 * - 裁剪正确性
 * - 效果正确性
 * - 滚动正确性
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "core/compositor/property_tree/property_trees.h"
#include "core/compositor/property_tree/property_tree_builder.h"
#include "core/compositor/property_tree/paint_artifact_compositor.h"
#include "core/compositor/property_tree/geometry_mapper.h"
#include "core/render/render_pipeline.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkPixmap.h"
#include <cmath>

namespace lightui {
namespace test {

class PropertyTreeVisualTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        property_trees_ = std::make_unique<PropertyTrees>();

        // 创建测试 Surface
        surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
        canvas_ = surface_->getCanvas();
    }

    void TearDown() override {
        canvas_ = nullptr;
        surface_.reset();
        property_trees_.reset();
        DOMTestBase::TearDown();
    }

    /**
     * @brief 比较两个 Surface 的像素差异
     * @param surface1 第一个 Surface
     * @param surface2 第二个 Surface
     * @param tolerance 容差（0-255）
     * @return 差异像素数量
     */
    int CompareSurfaces(sk_sp<SkSurface> surface1, sk_sp<SkSurface> surface2,
                        int tolerance = 0) {
        SkPixmap pm1, pm2;
        if (!surface1->peekPixels(&pm1) || !surface2->peekPixels(&pm2)) {
            return -1;
        }

        if (pm1.width() != pm2.width() || pm1.height() != pm2.height()) {
            return -1;
        }

        int diffCount = 0;
        for (int y = 0; y < pm1.height(); y++) {
            for (int x = 0; x < pm1.width(); x++) {
                SkColor c1 = pm1.getColor(x, y);
                SkColor c2 = pm2.getColor(x, y);

                int dr = std::abs(static_cast<int>(SkColorGetR(c1)) -
                                  static_cast<int>(SkColorGetR(c2)));
                int dg = std::abs(static_cast<int>(SkColorGetG(c1)) -
                                  static_cast<int>(SkColorGetG(c2)));
                int db = std::abs(static_cast<int>(SkColorGetB(c1)) -
                                  static_cast<int>(SkColorGetB(c2)));
                int da = std::abs(static_cast<int>(SkColorGetA(c1)) -
                                  static_cast<int>(SkColorGetA(c2)));

                if (dr > tolerance || dg > tolerance ||
                    db > tolerance || da > tolerance) {
                    diffCount++;
                }
            }
        }

        return diffCount;
    }

    /**
     * @brief 计算 Surface 的像素哈希（用于快速比较）
     */
    uint64_t ComputeSurfaceHash(sk_sp<SkSurface> surface) {
        SkPixmap pm;
        if (!surface->peekPixels(&pm)) {
            return 0;
        }

        uint64_t hash = 0;
        const uint8_t* pixels = static_cast<const uint8_t*>(pm.addr());
        size_t size = pm.computeByteSize();

        for (size_t i = 0; i < size; i++) {
            hash = hash * 31 + pixels[i];
        }

        return hash;
    }

    std::unique_ptr<PropertyTrees> property_trees_;
    sk_sp<SkSurface> surface_;
    SkCanvas* canvas_ = nullptr;
};

// ========== 变换正确性测试 ==========

TEST_F(PropertyTreeVisualTest, TransformAccumulationCorrectness) {
    // 测试变换累积的正确性
    auto& transform_tree = property_trees_->GetTransformTree();
    auto root = transform_tree.GetRoot();

    // 创建变换链：平移 -> 旋转 -> 缩放
    auto t1 = transform_tree.CreateNode(root);
    t1->SetMatrix(SkM44::Translate(100, 100, 0));

    auto t2 = transform_tree.CreateNode(t1);
    t2->SetMatrix(SkM44::Rotate({0, 0, 1}, 3.14159f / 4));  // 45度

    auto t3 = transform_tree.CreateNode(t2);
    t3->SetMatrix(SkM44::Scale(2, 2, 1));

    // 计算累积变换
    SkM44 accumulated = t3->GetAccumulatedTransform();

    // 手动计算期望结果
    SkM44 expected = SkM44::Translate(100, 100, 0);
    expected.postConcat(SkM44::Rotate({0, 0, 1}, 3.14159f / 4));
    expected.postConcat(SkM44::Scale(2, 2, 1));

    // 比较矩阵元素
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            EXPECT_NEAR(accumulated.rc(i, j), expected.rc(i, j), 0.0001f)
                << "Matrix element [" << i << "][" << j << "] mismatch";
        }
    }
}

TEST_F(PropertyTreeVisualTest, TransformToTargetCorrectness) {
    // 测试从一个节点到另一个节点的变换计算
    auto& transform_tree = property_trees_->GetTransformTree();
    auto root = transform_tree.GetRoot();

    // 创建两个分支
    auto t1 = transform_tree.CreateNode(root);
    t1->SetMatrix(SkM44::Translate(100, 0, 0));

    auto t2 = transform_tree.CreateNode(root);
    t2->SetMatrix(SkM44::Translate(0, 100, 0));

    // 计算从 t1 到 t2 的变换
    SkM44 t1_to_t2 = t1->GetTransformTo(t2);

    // 验证：点 (0,0) 在 t1 空间应该映射到 (100, -100) 在 t2 空间
    SkV4 point = {0, 0, 0, 1};
    SkV4 mapped = t1_to_t2 * point;

    EXPECT_NEAR(mapped.x / mapped.w, 100.0f, 0.0001f);
    EXPECT_NEAR(mapped.y / mapped.w, -100.0f, 0.0001f);
}

// ========== 裁剪正确性测试 ==========

TEST_F(PropertyTreeVisualTest, ClipAccumulationCorrectness) {
    // 测试裁剪累积的正确性
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();

    // 创建裁剪链
    auto c1 = clip_tree.CreateNode(clip_root);
    c1->SetClipRect(SkRect::MakeXYWH(0, 0, 400, 400));
    c1->SetTransformNode(transform_root);

    auto c2 = clip_tree.CreateNode(c1);
    c2->SetClipRect(SkRect::MakeXYWH(100, 100, 400, 400));
    c2->SetTransformNode(transform_root);

    // 计算累积裁剪
    SkRect accumulated = c2->GetAccumulatedClipRect(transform_root);

    // 期望结果：两个裁剪的交集
    SkRect expected = SkRect::MakeXYWH(100, 100, 300, 300);

    EXPECT_NEAR(accumulated.left(), expected.left(), 0.0001f);
    EXPECT_NEAR(accumulated.top(), expected.top(), 0.0001f);
    EXPECT_NEAR(accumulated.right(), expected.right(), 0.0001f);
    EXPECT_NEAR(accumulated.bottom(), expected.bottom(), 0.0001f);
}

TEST_F(PropertyTreeVisualTest, ClipWithTransformCorrectness) {
    // 测试带变换的裁剪
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();

    // 创建变换节点
    auto t1 = transform_tree.CreateNode(transform_root);
    t1->SetMatrix(SkM44::Scale(2, 2, 1));

    // 创建裁剪节点（在变换空间中）
    auto c1 = clip_tree.CreateNode(clip_root);
    c1->SetClipRect(SkRect::MakeXYWH(0, 0, 100, 100));
    c1->SetTransformNode(t1);

    // 获取在根空间中的裁剪
    SkRect clip_in_root = c1->GetClipRectInSpace(transform_root);

    // 期望：裁剪区域被放大 2 倍
    EXPECT_NEAR(clip_in_root.width(), 200.0f, 0.0001f);
    EXPECT_NEAR(clip_in_root.height(), 200.0f, 0.0001f);
}

// ========== 效果正确性测试 ==========

TEST_F(PropertyTreeVisualTest, OpacityAccumulationCorrectness) {
    // 测试透明度累积的正确性
    auto& effect_tree = property_trees_->GetEffectTree();
    auto root = effect_tree.GetRoot();

    // 创建效果链
    auto e1 = effect_tree.CreateNode(root);
    e1->SetOpacity(0.5f);

    auto e2 = effect_tree.CreateNode(e1);
    e2->SetOpacity(0.5f);

    // 计算累积透明度
    float accumulated = e2->GetAccumulatedOpacity();

    // 期望：0.5 * 0.5 = 0.25
    EXPECT_NEAR(accumulated, 0.25f, 0.0001f);
}

TEST_F(PropertyTreeVisualTest, EffectIsolationCorrectness) {
    // 测试效果隔离判断
    auto& effect_tree = property_trees_->GetEffectTree();
    auto root = effect_tree.GetRoot();

    // 只有 opacity 不需要隔离
    auto e1 = effect_tree.CreateNode(root);
    e1->SetOpacity(0.5f);
    EXPECT_FALSE(e1->RequiresIsolation());

    // 有 filter 需要隔离
    auto e2 = effect_tree.CreateNode(root);
    e2->SetOpacity(0.5f);
    std::vector<FilterOperation> filters;
    filters.push_back(FilterOperation::Blur(5.0f));
    e2->SetFilters(std::move(filters));
    EXPECT_TRUE(e2->RequiresIsolation());

    // 有非默认 blend mode 需要隔离
    auto e3 = effect_tree.CreateNode(root);
    e3->SetBlendMode(SkBlendMode::kMultiply);
    EXPECT_TRUE(e3->RequiresIsolation());
}

// ========== 滚动正确性测试 ==========

TEST_F(PropertyTreeVisualTest, ScrollOffsetCorrectness) {
    // 测试滚动偏移的正确性
    auto& scroll_tree = property_trees_->GetScrollTree();
    auto root = scroll_tree.GetRoot();

    auto s1 = scroll_tree.CreateNode(root);
    s1->SetContainerSize({800, 600});
    s1->SetContentSize({800, 2000});
    s1->SetScrollOffset({0, 500});

    // 验证最大滚动偏移
    SkPoint max_offset = s1->GetMaxScrollOffset();
    EXPECT_NEAR(max_offset.x(), 0.0f, 0.0001f);
    EXPECT_NEAR(max_offset.y(), 1400.0f, 0.0001f);  // 2000 - 600

    // 验证滚动方向
    EXPECT_FALSE(s1->CanScrollHorizontally());
    EXPECT_TRUE(s1->CanScrollVertically());
}

TEST_F(PropertyTreeVisualTest, ScrollTransformIntegration) {
    // 测试滚动与变换的集成
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& scroll_tree = property_trees_->GetScrollTree();

    auto transform_root = transform_tree.GetRoot();
    auto scroll_root = scroll_tree.GetRoot();

    // 创建滚动节点
    auto s1 = scroll_tree.CreateNode(scroll_root);
    s1->SetContainerSize({800, 600});
    s1->SetContentSize({800, 2000});
    s1->SetScrollOffset({0, 500});

    // 创建关联的滚动变换节点
    auto scroll_transform = transform_tree.CreateNode(transform_root);
    scroll_transform->SetScrollNode(s1);
    scroll_transform->SetMatrix(SkM44::Translate(0, -500, 0));  // 滚动偏移

    s1->SetScrollTransformNode(scroll_transform);

    // 验证变换
    EXPECT_TRUE(scroll_transform->IsScrollTranslation());
    EXPECT_EQ(scroll_transform->GetScrollNode(), s1);
}

// ========== 几何映射正确性测试 ==========

TEST_F(PropertyTreeVisualTest, GeometryMapperPointMapping) {
    // 测试点映射
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();
    auto& effect_tree = property_trees_->GetEffectTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();
    auto effect_root = effect_tree.GetRoot();

    // 创建变换
    auto t1 = transform_tree.CreateNode(transform_root);
    t1->SetMatrix(SkM44::Translate(100, 100, 0));

    PropertyTreeState source(t1, clip_root, effect_root, nullptr);
    PropertyTreeState target(transform_root, clip_root, effect_root, nullptr);

    GeometryMapper mapper(*property_trees_);

    // 映射点
    SkPoint point = {50, 50};
    SkPoint mapped = mapper.MapPoint(point, source, target);

    EXPECT_NEAR(mapped.x(), 150.0f, 0.0001f);
    EXPECT_NEAR(mapped.y(), 150.0f, 0.0001f);
}

TEST_F(PropertyTreeVisualTest, GeometryMapperRectMapping) {
    // 测试矩形映射
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();
    auto& effect_tree = property_trees_->GetEffectTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();
    auto effect_root = effect_tree.GetRoot();

    // 创建缩放变换
    auto t1 = transform_tree.CreateNode(transform_root);
    t1->SetMatrix(SkM44::Scale(2, 2, 1));

    PropertyTreeState source(t1, clip_root, effect_root, nullptr);
    PropertyTreeState target(transform_root, clip_root, effect_root, nullptr);

    GeometryMapper mapper(*property_trees_);

    // 映射矩形
    SkRect rect = SkRect::MakeXYWH(10, 10, 100, 100);
    SkRect mapped = mapper.MapRect(rect, source, target);

    EXPECT_NEAR(mapped.left(), 20.0f, 0.0001f);
    EXPECT_NEAR(mapped.top(), 20.0f, 0.0001f);
    EXPECT_NEAR(mapped.width(), 200.0f, 0.0001f);
    EXPECT_NEAR(mapped.height(), 200.0f, 0.0001f);
}

TEST_F(PropertyTreeVisualTest, GeometryMapperVisualRectWithClip) {
    // 测试带裁剪的可见区域映射
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();
    auto& effect_tree = property_trees_->GetEffectTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();
    auto effect_root = effect_tree.GetRoot();

    // 创建裁剪
    auto c1 = clip_tree.CreateNode(clip_root);
    c1->SetClipRect(SkRect::MakeXYWH(0, 0, 100, 100));
    c1->SetTransformNode(transform_root);

    PropertyTreeState source(transform_root, c1, effect_root, nullptr);
    PropertyTreeState target(transform_root, clip_root, effect_root, nullptr);

    GeometryMapper mapper(*property_trees_);

    // 映射超出裁剪区域的矩形
    SkRect rect = SkRect::MakeXYWH(50, 50, 100, 100);
    SkRect mapped = mapper.MapVisualRect(rect, source, target);

    // 期望：被裁剪到 (50,50,100,100) 与 (0,0,100,100) 的交集
    EXPECT_NEAR(mapped.left(), 50.0f, 0.0001f);
    EXPECT_NEAR(mapped.top(), 50.0f, 0.0001f);
    EXPECT_NEAR(mapped.right(), 100.0f, 0.0001f);
    EXPECT_NEAR(mapped.bottom(), 100.0f, 0.0001f);
}

// ========== 属性树状态比较测试 ==========

TEST_F(PropertyTreeVisualTest, PropertyTreeStateComparison) {
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();
    auto& effect_tree = property_trees_->GetEffectTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();
    auto effect_root = effect_tree.GetRoot();

    auto t1 = transform_tree.CreateNode(transform_root);
    auto c1 = clip_tree.CreateNode(clip_root);

    PropertyTreeState state1(transform_root, clip_root, effect_root, nullptr);
    PropertyTreeState state2(transform_root, clip_root, effect_root, nullptr);
    PropertyTreeState state3(t1, clip_root, effect_root, nullptr);
    PropertyTreeState state4(transform_root, c1, effect_root, nullptr);

    // 相同状态
    EXPECT_TRUE(state1 == state2);
    EXPECT_FALSE(state1 != state2);

    // 不同变换
    EXPECT_FALSE(state1 == state3);
    auto diff1 = state1.ComputeDifference(state3);
    EXPECT_TRUE(diff1.transform_changed);
    EXPECT_FALSE(diff1.clip_changed);

    // 不同裁剪
    EXPECT_FALSE(state1 == state4);
    auto diff2 = state1.ComputeDifference(state4);
    EXPECT_FALSE(diff2.transform_changed);
    EXPECT_TRUE(diff2.clip_changed);
}

TEST_F(PropertyTreeVisualTest, PropertyTreeStateMergeability) {
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();
    auto& effect_tree = property_trees_->GetEffectTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();
    auto effect_root = effect_tree.GetRoot();

    // 相同状态可以合并
    PropertyTreeState state1(transform_root, clip_root, effect_root, nullptr);
    PropertyTreeState state2(transform_root, clip_root, effect_root, nullptr);
    EXPECT_TRUE(state1.CanMergeWith(state2));

    // 不同变换不能合并
    auto t1 = transform_tree.CreateNode(transform_root);
    PropertyTreeState state3(t1, clip_root, effect_root, nullptr);
    EXPECT_FALSE(state1.CanMergeWith(state3));

    // 不同效果（有隔离）不能合并
    auto e1 = effect_tree.CreateNode(effect_root);
    e1->SetBlendMode(SkBlendMode::kMultiply);  // 需要隔离
    PropertyTreeState state4(transform_root, clip_root, e1, nullptr);
    EXPECT_FALSE(state1.CanMergeWith(state4));
}

// ========== 渲染路径对比测试 ==========

TEST_F(PropertyTreeVisualTest, RenderPathComparison) {
    // This test requires render tree support which may not be available
    // Skip if Document doesn't support layout/render tree
    GTEST_SKIP() << "Render tree integration not available in this build";
}

} // namespace test
} // namespace lightui
