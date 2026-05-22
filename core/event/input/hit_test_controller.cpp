/**
 * @file hit_test_controller.cpp
 * @brief 统一命中测试控制器实现
 */

#include "hit_test_controller.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/render/objects/render_object.h"
#include "core/render/layer/paint_layer.h"
#include "core/compositor/compositor_layer.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cstdlib>


namespace {

bool IsOutOfFlowForHitTest(const mbink::RenderObject* render_object) {
    if (!render_object) {
        return false;
    }

    const auto& style = render_object->GetComputedStyle();
    return style.position == "fixed" || style.position == "absolute";
}

void CollectOutOfFlowDescendantsInDomOrder(
    mbink::RenderObject* root,
    std::vector<mbink::RenderObject*>& out_of_flow_descendants) {
    if (!root) {
        return;
    }

    for (const auto& child : root->GetChildren()) {
        if (!child) {
            continue;
        }

        if (IsOutOfFlowForHitTest(child.get())) {
            out_of_flow_descendants.push_back(child.get());
        }

        CollectOutOfFlowDescendantsInDomOrder(child.get(), out_of_flow_descendants);
    }
}

void SortByZIndexAndDomOrder(std::vector<mbink::RenderObject*>& render_objects) {
    std::stable_sort(render_objects.begin(), render_objects.end(),
                     [](mbink::RenderObject* a, mbink::RenderObject* b) {
                         return a->GetComputedStyle().z_index < b->GetComputedStyle().z_index;
                     });
}

bool IsStickyTableCellForHitTest(mbink::RenderObject* render_object) {
    return render_object &&
           render_object->GetType() == mbink::RenderObjectType::TABLE_CELL &&
           render_object->GetComputedStyle().position == "sticky";
}

bool IsRenderObjectDescendantOf(
    const std::shared_ptr<mbink::RenderObject>& object,
    const mbink::RenderObject* ancestor) {
    auto current = object;
    while (current) {
        if (current.get() == ancestor) {
            return true;
        }
        current = current->GetParent();
    }
    return false;
}

int StickyTableCellAxisPriority(mbink::RenderObject* render_object) {
    if (!IsStickyTableCellForHitTest(render_object)) {
        return 0;
    }

    const auto& style = render_object->GetComputedStyle();
    bool sticks_vertically = style.top.unit != mbink::CSSUnit::AUTO ||
                             style.bottom.unit != mbink::CSSUnit::AUTO;
    bool sticks_horizontally = style.left.unit != mbink::CSSUnit::AUTO ||
                               style.right.unit != mbink::CSSUnit::AUTO;

    if (sticks_vertically && sticks_horizontally) {
        return 3;
    }
    if (sticks_horizontally) {
        return 2;
    }
    if (sticks_vertically) {
        return 1;
    }
    return 0;
}

struct StickyTableHitTestEntry {
    mbink::RenderObject* cell = nullptr;
    int z_index = 0;
    int axis_priority = 0;
    size_t dom_order = 0;
};

void CollectStickyTableCellsInDomOrder(
    mbink::RenderObject* root,
    std::vector<StickyTableHitTestEntry>& sticky_cells,
    size_t& next_dom_order) {
    if (!root) {
        return;
    }

    for (const auto& child : root->GetChildren()) {
        if (!child) {
            continue;
        }

        if (IsStickyTableCellForHitTest(child.get())) {
            StickyTableHitTestEntry entry;
            entry.cell = child.get();
            entry.z_index = child->GetComputedStyle().z_index;
            entry.axis_priority = StickyTableCellAxisPriority(child.get());
            entry.dom_order = next_dom_order++;
            sticky_cells.push_back(entry);
            continue;
        }

        CollectStickyTableCellsInDomOrder(child.get(), sticky_cells, next_dom_order);
    }
}

void SortStickyTableCellsByPaintOrder(std::vector<StickyTableHitTestEntry>& sticky_cells) {
    std::stable_sort(sticky_cells.begin(), sticky_cells.end(),
                     [](const StickyTableHitTestEntry& a, const StickyTableHitTestEntry& b) {
                         if (a.z_index != b.z_index) {
                             return a.z_index < b.z_index;
                         }
                         if (a.axis_priority != b.axis_priority) {
                             return a.axis_priority < b.axis_priority;
                         }
                         return a.dom_order < b.dom_order;
                     });
}

}  // namespace

namespace mbink {

HitTestResultEx HitTestController::HitTest(
    std::shared_ptr<RenderObject> root_render,
    float viewport_x,
    float viewport_y,
    const HitTestRequest& request) {

    HitTestResultEx result;
    result.viewport_x = viewport_x;
    result.viewport_y = viewport_y;

    if (!root_render) {
        result.miss_reason = "No render tree";
        return result;
    }

    // 确保视口坐标缓存是最新的
    // 注意：实际使用中，缓存应该在布局后统一更新
    // 这里作为后备方案

    // 1. 先测试 PaintLayer（如果有）
    PaintLayer* paint_layer = root_render->GetPaintLayer();
    if (paint_layer) {
        if (HitTestLayer(paint_layer, viewport_x, viewport_y, request, result)) {
            return result;
        }
    }

    // 2. 回退到直接测试渲染对象
    if (HitTestRenderObject(root_render.get(), viewport_x, viewport_y, request, result)) {
        return result;
    }

    result.miss_reason = "No element at coordinates";
    return result;
}

bool HitTestController::HitTestLayer(
    PaintLayer* layer,
    float viewport_x,
    float viewport_y,
    const HitTestRequest& request,
    HitTestResultEx& result) {

    if (!layer) return false;

    RenderObject* render_obj = layer->GetRenderObject();
    if (!render_obj) return false;

    // 按 z-order 测试（从高到低）
    // 1. 正 z-index 子层
    const auto& pos_list = layer->PosZOrderList();
    for (auto it = pos_list.rbegin(); it != pos_list.rend(); ++it) {
        if (*it && HitTestLayer(*it, viewport_x, viewport_y, request, result)) {
            return true;
        }
    }

    // 2. 测试自身
    if (HitTestRenderObject(render_obj, viewport_x, viewport_y, request, result)) {
        return true;
    }

    // 3. 负 z-index 子层
    const auto& neg_list = layer->NegZOrderList();
    for (auto it = neg_list.rbegin(); it != neg_list.rend(); ++it) {
        if (*it && HitTestLayer(*it, viewport_x, viewport_y, request, result)) {
            return true;
        }
    }

    return false;
}

bool HitTestController::HitTestRenderObject(
    RenderObject* render_obj,
    float viewport_x,
    float viewport_y,
    const HitTestRequest& request,
    HitTestResultEx& result,
    bool test_out_of_flow_descendants) {

    if (!render_obj) return false;

    const auto& layout = render_obj->GetLayoutInfo();
    if (!layout.is_laid_out) return false;

    const auto& style = render_obj->GetComputedStyle();

    auto node = render_obj->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);

    // 检查 visibility
    if (request.test_visibility && style.visibility == "hidden") {
        return false;
    }

    // 检查 opacity
    if (request.test_opacity && style.opacity <= 0.0f) {
        return false;
    }

    // 使用 ViewportBounds 缓存进行边界检查
    const auto& bounds = render_obj->GetViewportBounds();

    // 如果缓存无效，尝试更新
    if (!bounds.valid) {
        render_obj->UpdateViewportBounds();
    }

    // 按 z-index + DOM 顺序测试所有后代中的 out-of-flow 元素
    if (test_out_of_flow_descendants) {
        std::vector<RenderObject*> out_of_flow_descendants;
        CollectOutOfFlowDescendantsInDomOrder(render_obj, out_of_flow_descendants);
        SortByZIndexAndDomOrder(out_of_flow_descendants);
        for (auto it = out_of_flow_descendants.rbegin(); it != out_of_flow_descendants.rend(); ++it) {
            if (HitTestRenderObject(*it, viewport_x, viewport_y, request, result, false)) {
                return true;
            }
        }
    }

    const auto& children = render_obj->GetChildren();

    // 边界检查（仅对当前元素和普通流子元素）
    bool in_bounds = render_obj->ContainsViewportPoint(viewport_x, viewport_y);

    // 如果点击位置不在当前元素边界内，不测试普通流子元素
    if (!in_bounds) {
        return false;
    }

    // 检查裁剪（仅当在边界内时）
    if (IsClipped(render_obj, viewport_x, viewport_y)) {
        return false;
    }

    // 第三遍：测试普通流子元素（static/relative）
    if (render_obj->GetType() == RenderObjectType::TABLE) {
        std::vector<StickyTableHitTestEntry> sticky_cells;
        size_t next_dom_order = 0;
        CollectStickyTableCellsInDomOrder(render_obj, sticky_cells, next_dom_order);
        SortStickyTableCellsByPaintOrder(sticky_cells);
        for (auto it = sticky_cells.rbegin(); it != sticky_cells.rend(); ++it) {
            if (HitTestStickyTableCell(it->cell, viewport_x, viewport_y, request, result)) {
                return true;
            }
        }
    }

    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child_style = (*it)->GetComputedStyle();
        if (child_style.position != "fixed" && child_style.position != "absolute") {
            if (HitTestRenderObject(it->get(), viewport_x, viewport_y, request, result, false)) {
                return true;
            }
        }
    }

    // 检查 pointer-events
    if (style.pointer_events == "none" && !request.ignore_pointer_events) {
        return false;
    }

    // 当前元素命中（复用之前获取的 element）
    if (element) {
        result.element = element;
        result.render_object = render_obj->shared_from_this();

        // 计算局部坐标
        SkPoint local = bounds.ToLocalCoordinates(viewport_x, viewport_y);
        result.local_x = local.x();
        result.local_y = local.y();

        // DevTools 信息
        if (request.for_devtools) {
            FillDevToolsInfo(result, render_obj, nullptr);
        }

        return true;
    }

    return false;
}

bool HitTestController::HitTestRenderObjectWithViewportOffset(
    RenderObject* render_obj,
    RenderObject* offset_root,
    float viewport_x,
    float viewport_y,
    const HitTestRequest& request,
    HitTestResultEx& result,
    float offset_x,
    float offset_y,
    bool test_out_of_flow_descendants) {

    if (!render_obj) return false;

    const auto& layout = render_obj->GetLayoutInfo();
    if (!layout.is_laid_out) return false;

    const auto& style = render_obj->GetComputedStyle();

    auto node = render_obj->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);

    if (request.test_visibility && style.visibility == "hidden") {
        return false;
    }

    if (request.test_opacity && style.opacity <= 0.0f) {
        return false;
    }

    if (!render_obj->GetViewportBounds().valid) {
        render_obj->UpdateViewportBounds();
    }
    const auto& bounds = render_obj->GetViewportBounds();
    if (!bounds.valid) {
        return false;
    }

    if (test_out_of_flow_descendants) {
        std::vector<RenderObject*> out_of_flow_descendants;
        CollectOutOfFlowDescendantsInDomOrder(render_obj, out_of_flow_descendants);
        SortByZIndexAndDomOrder(out_of_flow_descendants);
        for (auto it = out_of_flow_descendants.rbegin(); it != out_of_flow_descendants.rend(); ++it) {
            if (HitTestRenderObjectWithViewportOffset(
                    *it, offset_root, viewport_x, viewport_y, request, result, offset_x, offset_y, false)) {
                return true;
            }
        }
    }

    const bool is_offset_root = render_obj == offset_root;
    const float effective_offset_x = is_offset_root ? 0.0f : offset_x;
    const float effective_offset_y = is_offset_root ? 0.0f : offset_y;
    SkRect adjusted_bounds = SkRect::MakeXYWH(
        bounds.x + effective_offset_x,
        bounds.y + effective_offset_y,
        bounds.width,
        bounds.height);

    if (!adjusted_bounds.contains(viewport_x, viewport_y)) {
        return false;
    }

    if (IsClippedWithViewportOffset(
            render_obj, offset_root, viewport_x, viewport_y, offset_x, offset_y)) {
        return false;
    }

    const auto& children = render_obj->GetChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child_style = (*it)->GetComputedStyle();
        if (child_style.position != "fixed" && child_style.position != "absolute") {
            if (HitTestRenderObjectWithViewportOffset(
                    it->get(), offset_root, viewport_x, viewport_y, request, result, offset_x, offset_y, false)) {
                return true;
            }
        }
    }

    if (style.pointer_events == "none" && !request.ignore_pointer_events) {
        return false;
    }

    if (element) {
        result.element = element;
        result.render_object = render_obj->shared_from_this();
        result.local_x = viewport_x - adjusted_bounds.x();
        result.local_y = viewport_y - adjusted_bounds.y();

        if (request.for_devtools) {
            FillDevToolsInfo(result, render_obj, nullptr);
        }

        return true;
    }

    return false;
}

bool HitTestController::HitTestStickyTableCell(
    RenderObject* render_obj,
    float viewport_x,
    float viewport_y,
    const HitTestRequest& request,
    HitTestResultEx& result) {

    if (!render_obj) {
        return false;
    }

    if (!render_obj->GetViewportBounds().valid) {
        render_obj->UpdateViewportBounds();
    }

    SkPoint sticky_offset = render_obj->ComputeStickyOffset();
    return HitTestRenderObjectWithViewportOffset(
        render_obj,
        render_obj,
        viewport_x,
        viewport_y,
        request,
        result,
        sticky_offset.x(),
        sticky_offset.y(),
        false);
}

bool HitTestController::IsClippedWithViewportOffset(
    RenderObject* render_obj,
    RenderObject* offset_root,
    float viewport_x,
    float viewport_y,
    float offset_x,
    float offset_y) {

    if (!render_obj) {
        return false;
    }

    const auto& style = render_obj->GetComputedStyle();
    if (style.position == "fixed") {
        return false;
    }

    constexpr int TOP_LAYER_ZINDEX_THRESHOLD = 100;
    bool is_absolute = (style.position == "absolute");
    if (is_absolute && style.z_index >= TOP_LAYER_ZINDEX_THRESHOLD) {
        return false;
    }

    auto ancestor = render_obj->GetParent();
    while (ancestor) {
        const auto& ancestor_style = ancestor->GetComputedStyle();
        bool ancestor_is_positioned = (ancestor_style.position == "absolute" ||
                                       ancestor_style.position == "fixed");
        if (ancestor_is_positioned && ancestor_style.z_index >= TOP_LAYER_ZINDEX_THRESHOLD) {
            return false;
        }
        ancestor = ancestor->GetParent();
    }

    auto parent = render_obj->GetParent();
    while (parent) {
        const auto& parent_style = parent->GetComputedStyle();

        bool has_overflow_clip = parent_style.overflow == "hidden" ||
                                 parent_style.overflow == "scroll" ||
                                 parent_style.overflow == "auto";

        if (has_overflow_clip) {
            if (!parent->GetViewportBounds().valid) {
                parent->UpdateViewportBounds();
            }

            const auto& parent_bounds = parent->GetViewportBounds();
            if (parent_bounds.valid) {
                const bool parent_needs_offset =
                    offset_root && parent.get() != offset_root &&
                    IsRenderObjectDescendantOf(parent, offset_root);
                SkRect adjusted_parent_bounds = SkRect::MakeXYWH(
                    parent_bounds.x + (parent_needs_offset ? offset_x : 0.0f),
                    parent_bounds.y + (parent_needs_offset ? offset_y : 0.0f),
                    parent_bounds.width,
                    parent_bounds.height);

                if (!adjusted_parent_bounds.contains(viewport_x, viewport_y)) {
                    return true;
                }
            }
        }

        if (is_absolute) {
            bool is_positioned = !parent_style.position.empty() &&
                                 parent_style.position != "static";
            if (is_positioned) {
                break;
            }
        }

        parent = parent->GetParent();
    }

    return false;
}

bool HitTestController::IsClipped(
    RenderObject* render_obj,
    float viewport_x,
    float viewport_y) {

    const auto& style = render_obj->GetComputedStyle();

    // fixed 元素不受任何祖先的 overflow 裁剪
    if (style.position == "fixed") {
        return false;
    }

    // =========================================================================
    // Top Layer 机制：高 z-index 的弹出元素不受祖先 overflow 裁剪
    // =========================================================================
    // 类似于浏览器的 Top Layer（用于 popover、dialog、fullscreen），
    // 高 z-index 的弹出元素（如下拉菜单、tooltip、modal）应该能够
    // 超出父容器的 overflow: hidden 边界显示和接收事件。
    //
    // 检查当前元素或其祖先是否是 Top Layer 元素：
    // - 当前元素本身是高 z-index 的 absolute/fixed
    // - 或者当前元素在一个高 z-index 的 absolute/fixed 容器内
    constexpr int TOP_LAYER_ZINDEX_THRESHOLD = 100;

    // 检查自身是否是 Top Layer
    bool is_absolute = (style.position == "absolute");
    if (is_absolute && style.z_index >= TOP_LAYER_ZINDEX_THRESHOLD) {
        return false;
    }

    // 检查祖先是否是 Top Layer（元素在 Top Layer 容器内）
    auto ancestor = render_obj->GetParent();
    while (ancestor) {
        const auto& ancestor_style = ancestor->GetComputedStyle();
        bool ancestor_is_positioned = (ancestor_style.position == "absolute" ||
                                       ancestor_style.position == "fixed");
        if (ancestor_is_positioned && ancestor_style.z_index >= TOP_LAYER_ZINDEX_THRESHOLD) {
            // 祖先是 Top Layer，当前元素不受 overflow 裁剪
            return false;
        }
        ancestor = ancestor->GetParent();
    }

    auto parent = render_obj->GetParent();
    while (parent) {
        const auto& parent_style = parent->GetComputedStyle();

        // 检查 overflow 裁剪
        bool has_overflow_clip = parent_style.overflow == "hidden" ||
                                 parent_style.overflow == "scroll" ||
                                 parent_style.overflow == "auto";

        if (has_overflow_clip) {
            const auto& parent_bounds = parent->GetViewportBounds();
            if (parent_bounds.valid) {
                if (!parent_bounds.Contains(viewport_x, viewport_y)) {
                    return true;  // 被裁剪
                }
            }
        }

        // 对于 absolute 元素，遇到定位祖先（包含块）后停止检查
        // absolute 元素不受包含块之外的祖先裁剪
        if (is_absolute) {
            bool is_positioned = !parent_style.position.empty() &&
                                 parent_style.position != "static";
            if (is_positioned) {
                break;  // 找到包含块，停止检查
            }
        }

        parent = parent->GetParent();
    }

    return false;
}

void HitTestController::FillDevToolsInfo(
    HitTestResultEx& result,
    RenderObject* render_obj,
    PaintLayer* layer) {

    if (!render_obj) return;

    const auto& style = render_obj->GetComputedStyle();
    result.z_index = style.z_index;

    // 检查是否在层叠上下文中
    if (layer) {
        result.in_stacking_context = layer->IsStackingContext();
    }
}

std::string HitTestController::ExplainMiss(
    std::shared_ptr<RenderObject> render_object,
    float viewport_x,
    float viewport_y) {

    if (!render_object) {
        return "Render object is null";
    }

    std::ostringstream oss;
    const auto& bounds = render_object->GetViewportBounds();
    const auto& style = render_object->GetComputedStyle();

    if (!bounds.valid) {
        oss << "ViewportBounds cache is invalid";
        return oss.str();
    }

    // 检查边界
    if (!bounds.Contains(viewport_x, viewport_y)) {
        oss << "Point (" << viewport_x << ", " << viewport_y << ") "
            << "is outside bounds [" << bounds.x << ", " << bounds.y << ", "
            << bounds.width << ", " << bounds.height << "]";
        return oss.str();
    }

    // 检查 pointer-events
    if (style.pointer_events == "none") {
        return "pointer-events: none";
    }

    // 检查 visibility
    if (style.visibility == "hidden") {
        return "visibility: hidden";
    }

    // 检查裁剪
    if (IsClipped(render_object.get(), viewport_x, viewport_y)) {
        return "Clipped by ancestor overflow";
    }

    return "Unknown reason";
}

bool HitTestController::HitTestCompositorLayers(
    const std::vector<std::shared_ptr<CompositorLayer>>& layers,
    float viewport_x,
    float viewport_y,
    const HitTestRequest& request,
    HitTestResultEx& result) {

    // 从后向前测试（后添加的层在上面）
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        auto& layer = *it;
        if (!layer) continue;

        // 获取层的渲染对象
        auto render_obj = layer->GetRenderObject();
        if (render_obj) {
            if (HitTestRenderObject(render_obj, viewport_x, viewport_y, request, result)) {
                return true;
            }
        }
    }

    return false;
}

}  // namespace mbink

