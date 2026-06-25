#include <gtest/gtest.h>

#include "core/dom/element.h"
#include "core/event/input/hit_test_controller.h"
#include "core/render/layer/paint_layer.h"
#include "core/render/objects/render_object.h"

namespace mblink {
namespace test {

namespace {

struct BoxNode {
    std::shared_ptr<Element> element;
    std::shared_ptr<RenderObject> render;
};

BoxNode CreateBox(const std::string& preact_id,
                  const std::string& position,
                  int z_index,
                  float x,
                  float y,
                  float width,
                  float height) {
    BoxNode box;
    box.element = std::make_shared<Element>("div");
    box.element->SetAttribute("data-preact-id", preact_id);

    box.render = std::make_shared<RenderObject>(RenderObjectType::BLOCK);
    ComputedStyle style;
    style.position = position;
    style.z_index = z_index;
    box.render->SetNode(box.element);
    box.render->SetComputedStyle(style);

    auto& layout = box.render->GetLayoutInfo();
    layout.x = x;
    layout.y = y;
    layout.width = width;
    layout.height = height;
    layout.is_laid_out = true;
    return box;
}


}  // namespace

TEST(HitTestControllerTest, OutOfFlowHitTestUsesZIndexAcrossBranches) {
    auto root = CreateBox("root", "static", 0, 0, 0, 800, 600);
    auto branch410 = CreateBox("branch-410", "static", 0, 0, 0, 800, 600);
    auto branch420 = CreateBox("branch-420", "static", 0, 0, 0, 800, 600);
    auto box410 = CreateBox("410", "fixed", 10, 100, 100, 120, 120);
    auto box420 = CreateBox("420", "fixed", 1, 100, 100, 120, 120);

    PaintLayer* root_layer = root.render->EnsurePaintLayer();
    ASSERT_NE(root_layer, nullptr);

    root.render->AppendChild(branch410.render);
    root.render->AppendChild(branch420.render);
    branch410.render->AppendChild(box410.render);
    branch420.render->AppendChild(box420.render);

    box410.render->UpdateViewportBounds();
    box420.render->UpdateViewportBounds();

    HitTestResult layer_result;
    ASSERT_TRUE(root_layer->HitTest(110.0f, 110.0f, layer_result));
    ASSERT_TRUE(layer_result.element);
    EXPECT_EQ(layer_result.element->GetAttribute("data-preact-id"), "410");
    ASSERT_TRUE(layer_result.render_object);
    EXPECT_EQ(layer_result.render_object.get(), box410.render.get());

    HitTestController controller;
    auto controller_result = controller.HitTest(root.render, 110.0f, 110.0f);
    ASSERT_TRUE(controller_result.IsValid());
    ASSERT_TRUE(controller_result.element);
    EXPECT_EQ(controller_result.element->GetAttribute("data-preact-id"), "410");
    ASSERT_TRUE(controller_result.render_object);
    EXPECT_EQ(controller_result.render_object.get(), box410.render.get());
}

TEST(HitTestControllerTest, ViewportBoundsStopsAtFixedAncestorForDescendants) {
    auto root = CreateBox("root", "static", 0, 0, 0, 800, 600);
    auto offset_branch = CreateBox("offset-branch", "static", 0, 0, 180, 800, 600);
    auto fixed_modal = CreateBox("fixed-modal", "fixed", 1000, 100, 80, 500, 320);
    auto select = CreateBox("select", "static", 0, 24, 160, 240, 36);

    root.render->AppendChild(offset_branch.render);
    offset_branch.render->AppendChild(fixed_modal.render);
    fixed_modal.render->AppendChild(select.render);

    fixed_modal.render->UpdateViewportBounds();
    select.render->UpdateViewportBounds();

    const auto rect = select.render->GetViewportBoundingRect();
    EXPECT_FLOAT_EQ(rect.x(), 124.0f);
    EXPECT_FLOAT_EQ(rect.y(), 240.0f);
    EXPECT_FLOAT_EQ(rect.width(), 240.0f);
    EXPECT_FLOAT_EQ(rect.height(), 36.0f);
}

}  // namespace test
}  // namespace mblink
