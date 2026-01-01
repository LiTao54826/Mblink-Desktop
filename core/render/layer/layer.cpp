/**
 * @file layer.cpp
 * @brief Layer 类实现
 */

#include "layer.h"
#include "core/event/input/hit_testing.h"
#include "core/dom/element.h"
#include <algorithm>
#include <iostream>

namespace lightui {

Layer::Layer(int z_min, int z_max)
    : z_index_min_(z_min)
    , z_index_max_(z_max) {
}

bool Layer::ContainsZIndex(int z_index) const {
    if (z_index < z_index_min_) return false;
    if (z_index_max_ >= 0 && z_index > z_index_max_) return false;
    return true;
}

void Layer::AddItem(std::shared_ptr<RenderObject> obj, const SkMatrix& transform, int z_index) {
    if (!obj) return;
    
    LayerItem item;
    item.render_obj = obj;
    item.transform = transform;
    item.z_index = z_index;
    
    const auto& layout = obj->GetLayoutInfo();
    const auto& style = obj->GetComputedStyle();
    
    // 检查是否是 position: fixed 元素
    bool is_fixed = (style.position == "fixed");
    
    // 计算绝对逻辑坐标
    // 对于 position: fixed 元素，layout.x/y 已经是视口绝对坐标（由布局引擎计算）
    // 对于其他元素，需要从 transform 中提取父元素偏移并加上 layout.x/y
    
    if (is_fixed) {
        // position: fixed 元素：layout.x/y 已经是视口绝对坐标
        item.abs_x = layout.x;
        item.abs_y = layout.y;
    } else {
        // 其他元素：需要计算父元素偏移
        // transform 包含: scale(dpi_scale) * translate(parent_logical_x * dpi_scale, parent_logical_y * dpi_scale)
        // 
        // 实际上 canvas 的变换顺序是：
        // 1. canvas->scale(dpi_scale, dpi_scale)  - 应用 DPI 缩放
        // 2. 绘制时 translate 到元素位置
        // 
        // 所以 transform 矩阵是: scale(dpi) * translate(parent_x, parent_y)
        // getTranslateX() = parent_x * dpi_scale
        // getScaleX() = dpi_scale
        //
        // 要得到逻辑坐标: parent_x = getTranslateX() / getScaleX()
        // 然后加上当前元素的相对位置 layout.x
        
        float scale_x = transform.getScaleX();
        float scale_y = transform.getScaleY();
        if (scale_x == 0) scale_x = 1;
        if (scale_y == 0) scale_y = 1;
        
        // 父元素的逻辑绝对坐标
        float parent_abs_x = transform.getTranslateX() / scale_x;
        float parent_abs_y = transform.getTranslateY() / scale_y;
        
        // 当前元素的逻辑绝对坐标 = 父元素绝对坐标 + 当前元素相对坐标
        item.abs_x = parent_abs_x + layout.x;
        item.abs_y = parent_abs_y + layout.y;
    }
    
    item.width = layout.width;
    item.height = layout.height;
    
    // 调试输出
    static bool debug = std::getenv("LIGHTUI_DEBUG_LAYERS") != nullptr;
    if (debug) {
        std::cout << "[Layer::AddItem] z=" << z_index 
                  << " position=" << style.position
                  << " layout=(" << layout.x << "," << layout.y << "," << layout.width << "," << layout.height << ")"
                  << " -> abs=(" << item.abs_x << "," << item.abs_y << ")"
                  << std::endl;
    }
    
    items_.push_back(item);
    needs_sort_ = true;
    
    // Debug: 检查是否添加了 Toast 容器
    auto node = obj->GetNode();
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto elem = std::dynamic_pointer_cast<Element>(node);
        if (elem && elem->GetAttribute("id") == "lightui-toast-container") {
            std::cout << "[Layer::AddItem] Added toast container to layer, z_index=" << z_index 
                      << " total_items=" << items_.size() << std::endl;
        }
    }
}

void Layer::Clear() {
    items_.clear();
    needs_sort_ = false;
}

void Layer::EnsureSorted() {
    if (!needs_sort_) return;
    
    // 按 z-index 升序排序（低的先绘制）
    std::sort(items_.begin(), items_.end(),
        [](const LayerItem& a, const LayerItem& b) {
            return a.z_index < b.z_index;
        });
    
    needs_sort_ = false;
}

void Layer::Paint(SkCanvas* canvas) {
    if (items_.empty() || !canvas) return;
    
    EnsureSorted();
    
    // 按 z-index 顺序绘制
    for (const auto& item : items_) {
        // Debug: 检查是否是 Toast 容器
        auto node = item.render_obj->GetNode();
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem && elem->GetAttribute("id") == "lightui-toast-container") {
                std::cout << "[Layer::Paint] Painting toast container from layer, z_index=" << item.z_index << std::endl;
            }
        }
        
        canvas->save();
        canvas->setMatrix(item.transform);
        item.render_obj->Paint(canvas);
        canvas->restore();
    }
}

bool Layer::HitTest(float x, float y, HitTestResult& result) {
    if (items_.empty()) return false;
    
    EnsureSorted();
    
    // 从高 z-index 到低 z-index 测试（后绘制的在上面）
    for (auto it = items_.rbegin(); it != items_.rend(); ++it) {
        const auto& item = *it;
        
        // 快速边界检查（使用逻辑坐标）
        if (x < item.abs_x || x >= item.abs_x + item.width ||
            y < item.abs_y || y >= item.abs_y + item.height) {
            continue;
        }
        
        // 对于 Layer 中的元素，item.abs_x/abs_y 是元素的绝对位置
        // 递归测试时，需要传入当前元素的绝对位置作为子元素的偏移基准
        // 注意：这里直接传入 item.abs_x/abs_y，因为 HitTestRenderObject 会处理 fixed 元素
        if (HitTestRenderObject(item.render_obj, x, y, item.abs_x, item.abs_y, result, true)) {
            return true;
        }
    }
    
    return false;
}

bool Layer::HitTestRenderObject(
    std::shared_ptr<RenderObject> render_obj,
    float x, float y,
    float offset_x, float offset_y,
    HitTestResult& result,
    bool is_root) {
    
    if (!render_obj) return false;
    
    const auto& layout = render_obj->GetLayoutInfo();
    if (!layout.is_laid_out) return false;
    
    // 检查当前元素的 position 属性
    const auto& style = render_obj->GetComputedStyle();
    bool is_fixed = (style.position == "fixed");
    
    // 计算当前元素的绝对位置
    // 对于根元素（Layer 中的元素），offset_x/y 已经是元素的绝对位置
    // 对于子元素，需要加上相对于父元素的偏移
    float current_x, current_y;
    if (is_root) {
        // 根元素：offset_x/y 就是绝对位置
        current_x = offset_x;
        current_y = offset_y;
    } else if (is_fixed) {
        // 非根的 fixed 元素：layout.x/y 是视口绝对坐标
        current_x = layout.x;
        current_y = layout.y;
    } else {
        // 普通子元素：相对于父元素
        current_x = offset_x + layout.x;
        current_y = offset_y + layout.y;
    }
    
    // 边界检查
    if (x < current_x || x >= current_x + layout.width ||
        y < current_y || y >= current_y + layout.height) {
        return false;
    }
    
    // 检查 pointer-events
    bool pointer_events_none = (style.pointer_events == "none");
    
    // 处理滚动偏移
    float scroll_x = render_obj->GetScrollX();
    float scroll_y = render_obj->GetScrollY();
    float child_test_x = x + scroll_x;
    float child_test_y = y + scroll_y;
    
    // 从后向前遍历子元素
    const auto& children = render_obj->GetChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (HitTestRenderObject(*it, child_test_x, child_test_y, current_x, current_y, result, false)) {
            return true;
        }
    }
    
    // 如果 pointer-events: none，不命中当前元素
    if (pointer_events_none) {
        return false;
    }
    
    // 当前元素命中
    auto node = render_obj->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element) {
        result.element = element;
        result.render_object = render_obj;
        result.local_x = x - current_x;
        result.local_y = y - current_y;
        return true;
    }
    
    // 如果是文本节点，向上查找 Element
    auto parent_ro = render_obj->GetParent();
    while (parent_ro) {
        auto parent_node = parent_ro->GetNode();
        auto parent_element = std::dynamic_pointer_cast<Element>(parent_node);
        if (parent_element) {
            const auto& parent_style = parent_ro->GetComputedStyle();
            if (parent_style.pointer_events == "none") {
                return false;
            }
            result.element = parent_element;
            result.render_object = parent_ro;
            result.local_x = x - current_x;
            result.local_y = y - current_y;
            return true;
        }
        parent_ro = parent_ro->GetParent();
    }
    
    return false;
}

bool Layer::HandleWheel(float x, float y, float delta_x, float delta_y) {
    if (items_.empty()) return false;
    
    EnsureSorted();
    
    // 从高 z-index 到低 z-index 查找可滚动元素
    for (auto it = items_.rbegin(); it != items_.rend(); ++it) {
        const auto& item = *it;
        
        // 边界检查
        if (x < item.abs_x || x >= item.abs_x + item.width ||
            y < item.abs_y || y >= item.abs_y + item.height) {
            continue;
        }
        
        // 检查元素是否可滚动
        const auto& style = item.render_obj->GetComputedStyle();
        std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
        bool allow_scroll = (overflow_y == "scroll" || overflow_y == "auto");
        
        if (allow_scroll) {
            float max_scroll_y = item.render_obj->GetMaxScrollY();
            if (max_scroll_y > 0) {
                // 应用滚动
                item.render_obj->ScrollBy(delta_x, delta_y);
                return true;  // 事件被处理
            }
        }
    }
    
    return false;  // 事件未被处理，穿透到下层
}

} // namespace lightui
