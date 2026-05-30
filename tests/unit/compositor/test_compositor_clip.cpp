/**
 * @file test_compositor_clip.cpp
 * @brief Unit tests for compositor clipping behavior.
 */

#include <gtest/gtest.h>

#include "core/compositor/compositor.h"
#include "core/compositor/compositor_layer.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkSurface.h"

using namespace mbink;

class ClipRenderObject : public RenderObject {
public:
    explicit ClipRenderObject(bool scrollable = false) : RenderObject(RenderObjectType::BLOCK) {
        layout_info_.width = 100.0f;
        layout_info_.height = 100.0f;
        layout_info_.is_laid_out = true;
        if (scrollable) {
            computed_style_.overflow_x = "auto";
            computed_style_.overflow_y = "auto";
            SetContentSize(100.0f, 220.0f);
        }
    }
};

TEST(CompositorClipTest, ScrollLayerClipsDescendantCompositorLayers) {
    auto scroll_object = std::make_shared<ClipRenderObject>(true);

    auto root = CreateCompositorLayer();
    root->SetPromotionReason(LayerPromotionReason::RootLayer);
    root->SetBounds(SkRect::MakeWH(160.0f, 160.0f));
    root->SetAllowsBitmapBacking(false);

    auto clip_layer = CreateCompositorLayer();
    clip_layer->SetRenderObject(scroll_object.get());
    clip_layer->SetPromotionReason(LayerPromotionReason::ScrollableContent);
    clip_layer->SetBounds(SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f));
    clip_layer->SetAllowsBitmapBacking(false);
    root->AddChild(clip_layer);

    auto child = CreateCompositorLayer();
    child->SetBounds(SkRect::MakeXYWH(0.0f, 110.0f, 40.0f, 40.0f));
    ASSERT_TRUE(child->EnsureBitmap());
    child->GetCanvas()->clear(SK_ColorRED);
    clip_layer->AddChild(child);

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(160, 160));
    ASSERT_TRUE(surface);
    surface->getCanvas()->clear(SK_ColorTRANSPARENT);

    Compositor compositor;
    ASSERT_TRUE(compositor.CompositeToCanvas(root.get(), surface->getCanvas()));

    SkBitmap bitmap;
    bitmap.allocPixels(SkImageInfo::MakeN32Premul(160, 160));
    ASSERT_TRUE(surface->readPixels(bitmap, 0, 0));

    EXPECT_EQ(bitmap.getColor(10, 120), SK_ColorTRANSPARENT);
}
