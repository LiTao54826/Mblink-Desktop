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
#include <sstream>
#include <iostream>
#include <cstdlib>

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
    HitTestResultEx& result) {

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

    // 关键修复：先递归测试所有后代中的 absolute/fixed 元素
    // 这些元素可以渲染在父元素边界之外，不受普通流边界限制
    const auto& children = render_obj->GetChildren();

    // 递归测试所有子元素中的 fixed/absolute 元素（包括嵌套的）
    std::function<bool(const std::vector<std::shared_ptr<RenderObject>>&)> testOutOfFlowDescendants;
    testOutOfFlowDescendants = [&](const std::vector<std::shared_ptr<RenderObject>>& nodes) -> bool {
        for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
            const auto& child = *it;
            const auto& child_style = child->GetComputedStyle();

            // 测试 fixed/absolute 元素
            if (child_style.position == "fixed" || child_style.position == "absolute") {
                if (HitTestRenderObject(child.get(), viewport_x, viewport_y, request, result)) {
                    return true;
                }
            }

            // 无条件递归测试所有子元素的后代
            // absolute/fixed 元素可以出现在任意深度，不受中间元素边界限制
            if (testOutOfFlowDescendants(child->GetChildren())) {
                return true;
            }
        }
        return false;
    };

    // 先测试所有后代中的 out-of-flow 元素
    if (testOutOfFlowDescendants(children)) {
        return true;
    }

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
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child_style = (*it)->GetComputedStyle();
        if (child_style.position != "fixed" && child_style.position != "absolute") {
            if (HitTestRenderObject(it->get(), viewport_x, viewport_y, request, result)) {
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

