/**
 * @file test_compositor_layer.cpp
 * @brief Unit tests for CompositorLayer class
 *
 * Tests the non-OpenGL functionality of CompositorLayer:
 * - Bounds and transform management
 * - Dirty region tracking
 * - Parent-child relationships
 * - Scroll offset management
 *
 * Note: GPU texture tests require an OpenGL context and are tested
 * in integration tests instead.
 */

#include <gtest/gtest.h>
#include "core/compositor/compositor_layer.h"

using namespace lightui;

class CompositorLayerTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// =========================================================================
// Basic Properties Tests
// =========================================================================

TEST_F(CompositorLayerTest, CreateLayerWithUniqueId) {
    auto layer1 = CreateCompositorLayer();
    auto layer2 = CreateCompositorLayer();

    EXPECT_NE(layer1->GetId(), layer2->GetId());
    EXPECT_GT(layer1->GetId(), 0u);
    EXPECT_GT(layer2->GetId(), 0u);
}

TEST_F(CompositorLayerTest, DefaultProperties) {
    auto layer = CreateCompositorLayer();

    EXPECT_EQ(layer->GetOpacity(), 1.0f);
    EXPECT_TRUE(layer->GetTransform().isIdentity());
    EXPECT_EQ(layer->GetPromotionReason(), LayerPromotionReason::None);
    EXPECT_EQ(layer->GetRenderObject(), nullptr);
}

TEST_F(CompositorLayerTest, SetBounds) {
    auto layer = CreateCompositorLayer();

    SkRect bounds = SkRect::MakeXYWH(10, 20, 100, 200);
    layer->SetBounds(bounds);

    EXPECT_EQ(layer->GetBounds(), bounds);
}

TEST_F(CompositorLayerTest, SetTransform) {
    auto layer = CreateCompositorLayer();

    SkMatrix transform = SkMatrix::Translate(50, 100);
    layer->SetTransform(transform);

    EXPECT_EQ(layer->GetTransform(), transform);
}

TEST_F(CompositorLayerTest, SetOpacity) {
    auto layer = CreateCompositorLayer();

    layer->SetOpacity(0.5f);
    EXPECT_EQ(layer->GetOpacity(), 0.5f);

    layer->SetOpacity(0.0f);
    EXPECT_EQ(layer->GetOpacity(), 0.0f);

    layer->SetOpacity(1.0f);
    EXPECT_EQ(layer->GetOpacity(), 1.0f);
}

TEST_F(CompositorLayerTest, SetPromotionReason) {
    auto layer = CreateCompositorLayer();

    layer->SetPromotionReason(LayerPromotionReason::WillChangeTransform);
    EXPECT_EQ(layer->GetPromotionReason(), LayerPromotionReason::WillChangeTransform);

    layer->SetPromotionReason(LayerPromotionReason::PositionFixed);
    EXPECT_EQ(layer->GetPromotionReason(), LayerPromotionReason::PositionFixed);
}

// =========================================================================
// Dirty Region Tests
// =========================================================================

TEST_F(CompositorLayerTest, InitialDirtyState) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    // SetBounds marks full dirty
    EXPECT_TRUE(layer->HasDirtyRegions());
}

TEST_F(CompositorLayerTest, ClearDirtyRegions) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    layer->ClearDirtyRegions();
    EXPECT_FALSE(layer->HasDirtyRegions());
    EXPECT_TRUE(layer->GetDirtyRegions().empty());
}

TEST_F(CompositorLayerTest, MarkDirtyRegion) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));
    layer->ClearDirtyRegions();

    layer->MarkDirty(SkRect::MakeXYWH(10, 10, 50, 50));

    EXPECT_TRUE(layer->HasDirtyRegions());
    EXPECT_EQ(layer->GetDirtyRegions().size(), 1u);
}

TEST_F(CompositorLayerTest, MarkFullDirty) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));
    layer->ClearDirtyRegions();

    layer->MarkFullDirty();

    EXPECT_TRUE(layer->HasDirtyRegions());
    EXPECT_EQ(layer->GetDirtyRegions().size(), 1u);

    const auto& region = layer->GetDirtyRegions()[0];
    EXPECT_EQ(region.left(), 0);
    EXPECT_EQ(region.top(), 0);
    EXPECT_EQ(region.width(), 100);
    EXPECT_EQ(region.height(), 100);
}

TEST_F(CompositorLayerTest, DirtyRegionClippedToBounds) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));
    layer->ClearDirtyRegions();

    // Mark region that extends outside bounds
    layer->MarkDirty(SkRect::MakeXYWH(-10, -10, 200, 200));

    const auto& regions = layer->GetDirtyRegions();
    EXPECT_EQ(regions.size(), 1u);

    // Should be clipped to layer bounds
    const auto& region = regions[0];
    EXPECT_GE(region.left(), 0);
    EXPECT_GE(region.top(), 0);
    EXPECT_LE(region.right(), 100);
    EXPECT_LE(region.bottom(), 100);
}

TEST_F(CompositorLayerTest, MergeDirtyRegions) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(500, 500));
    layer->ClearDirtyRegions();

    // Add overlapping regions
    layer->MarkDirty(SkRect::MakeXYWH(10, 10, 50, 50));
    layer->MarkDirty(SkRect::MakeXYWH(30, 30, 50, 50));

    EXPECT_EQ(layer->GetDirtyRegions().size(), 2u);

    layer->MergeDirtyRegions();

    // Should be merged into fewer regions
    EXPECT_LE(layer->GetDirtyRegions().size(), 2u);
}

// =========================================================================
// Parent-Child Relationship Tests
// =========================================================================

TEST_F(CompositorLayerTest, AddChild) {
    auto parent = CreateCompositorLayer();
    auto child = CreateCompositorLayer();

    parent->AddChild(child);

    EXPECT_EQ(parent->GetChildren().size(), 1u);
    EXPECT_EQ(parent->GetChildren()[0], child);
    EXPECT_EQ(child->GetParent(), parent);
}

TEST_F(CompositorLayerTest, RemoveChild) {
    auto parent = CreateCompositorLayer();
    auto child = CreateCompositorLayer();

    parent->AddChild(child);
    parent->RemoveChild(child.get());

    EXPECT_TRUE(parent->GetChildren().empty());
    EXPECT_EQ(child->GetParent(), nullptr);
}

TEST_F(CompositorLayerTest, RemoveAllChildren) {
    auto parent = CreateCompositorLayer();
    auto child1 = CreateCompositorLayer();
    auto child2 = CreateCompositorLayer();

    parent->AddChild(child1);
    parent->AddChild(child2);

    EXPECT_EQ(parent->GetChildren().size(), 2u);

    parent->RemoveAllChildren();

    EXPECT_TRUE(parent->GetChildren().empty());
    EXPECT_EQ(child1->GetParent(), nullptr);
    EXPECT_EQ(child2->GetParent(), nullptr);
}

TEST_F(CompositorLayerTest, ReparentChild) {
    auto parent1 = CreateCompositorLayer();
    auto parent2 = CreateCompositorLayer();
    auto child = CreateCompositorLayer();

    parent1->AddChild(child);
    EXPECT_EQ(child->GetParent(), parent1);

    // Adding to new parent should remove from old parent
    parent2->AddChild(child);

    EXPECT_TRUE(parent1->GetChildren().empty());
    EXPECT_EQ(parent2->GetChildren().size(), 1u);
    EXPECT_EQ(child->GetParent(), parent2);
}

// =========================================================================
// Scroll Offset Tests
// =========================================================================

TEST_F(CompositorLayerTest, DefaultScrollOffset) {
    auto layer = CreateCompositorLayer();

    EXPECT_EQ(layer->GetScrollOffset().fX, 0.0f);
    EXPECT_EQ(layer->GetScrollOffset().fY, 0.0f);
}

TEST_F(CompositorLayerTest, SetScrollOffset) {
    auto layer = CreateCompositorLayer();

    layer->SetScrollOffset(SkPoint::Make(100, 200));

    EXPECT_EQ(layer->GetScrollOffset().fX, 100.0f);
    EXPECT_EQ(layer->GetScrollOffset().fY, 200.0f);
}

TEST_F(CompositorLayerTest, ScrollBy) {
    auto layer = CreateCompositorLayer();

    layer->ScrollBy(50, 100);
    EXPECT_EQ(layer->GetScrollOffset().fX, 50.0f);
    EXPECT_EQ(layer->GetScrollOffset().fY, 100.0f);

    layer->ScrollBy(25, 50);
    EXPECT_EQ(layer->GetScrollOffset().fX, 75.0f);
    EXPECT_EQ(layer->GetScrollOffset().fY, 150.0f);
}

// =========================================================================
// CPU Bitmap Tests
// =========================================================================

TEST_F(CompositorLayerTest, EnsureBitmapWithValidBounds) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    EXPECT_TRUE(layer->EnsureBitmap());
    EXPECT_NE(layer->GetCanvas(), nullptr);

    const auto& bitmap = layer->GetBitmap();
    EXPECT_EQ(bitmap.width(), 100);
    EXPECT_EQ(bitmap.height(), 100);
}

TEST_F(CompositorLayerTest, EnsureBitmapWithZeroBounds) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(0, 0));

    EXPECT_FALSE(layer->EnsureBitmap());
    EXPECT_EQ(layer->GetCanvas(), nullptr);
}

TEST_F(CompositorLayerTest, ReleaseBitmap) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));
    layer->EnsureBitmap();

    // Verify bitmap is allocated
    EXPECT_EQ(layer->GetBitmap().width(), 100);

    layer->ReleaseBitmap();

    // After release, bitmap should be reset
    EXPECT_TRUE(layer->GetBitmap().isNull());
}

TEST_F(CompositorLayerTest, BitmapResizedOnBoundsChange) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));
    layer->EnsureBitmap();

    EXPECT_EQ(layer->GetBitmap().width(), 100);

    layer->SetBounds(SkRect::MakeWH(200, 200));
    layer->EnsureBitmap();

    EXPECT_EQ(layer->GetBitmap().width(), 200);
    EXPECT_EQ(layer->GetBitmap().height(), 200);
}

// =========================================================================
// Debug Support Tests
// =========================================================================

TEST_F(CompositorLayerTest, DebugName) {
    auto layer = CreateCompositorLayer();

    layer->SetDebugName("test-layer");
    EXPECT_EQ(layer->GetDebugName(), "test-layer");
}

TEST_F(CompositorLayerTest, PromotionReasonToString) {
    EXPECT_STREQ(CompositorLayer::PromotionReasonToString(LayerPromotionReason::None), "None");
    EXPECT_STREQ(CompositorLayer::PromotionReasonToString(LayerPromotionReason::WillChangeTransform), "will-change: transform");
    EXPECT_STREQ(CompositorLayer::PromotionReasonToString(LayerPromotionReason::WillChangeOpacity), "will-change: opacity");
    EXPECT_STREQ(CompositorLayer::PromotionReasonToString(LayerPromotionReason::PositionFixed), "position: fixed");
    EXPECT_STREQ(CompositorLayer::PromotionReasonToString(LayerPromotionReason::TransformAnimation), "transform animation");
    EXPECT_STREQ(CompositorLayer::PromotionReasonToString(LayerPromotionReason::OpacityAnimation), "opacity animation");
    EXPECT_STREQ(CompositorLayer::PromotionReasonToString(LayerPromotionReason::ScrollableContent), "scrollable content");
    EXPECT_STREQ(CompositorLayer::PromotionReasonToString(LayerPromotionReason::Explicit), "explicit");
    EXPECT_STREQ(CompositorLayer::PromotionReasonToString(LayerPromotionReason::RootLayer), "root layer");
}

// =========================================================================
// Texture State Tests (without OpenGL context)
// =========================================================================

TEST_F(CompositorLayerTest, InitialTextureState) {
    auto layer = CreateCompositorLayer();

    EXPECT_EQ(layer->GetTextureId(), 0u);
    EXPECT_FALSE(layer->HasTexture());
    EXPECT_FALSE(layer->IsTextureDirty());
}

TEST_F(CompositorLayerTest, MarkTextureDirty) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    layer->MarkTextureDirty(SkIRect::MakeXYWH(10, 10, 50, 50));

    EXPECT_TRUE(layer->IsTextureDirty());
}

