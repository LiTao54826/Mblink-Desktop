/**
 * @file paint_layer.cpp
 * @brief PaintLayer 实现
 */

#include "paint_layer.h"
#include "core/render/objects/render_object.h"
#include "core/compositor/compositor_layer.h"
#include "core/event/input/hit_testing.h"
#include "core/dom/element.h"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace lightui {

// =========================================================================
// 构造和析构
// =========================================================================

PaintLayer::PaintLayer(RenderObject* render_object)
    : render_object_(render_object)
    , promotion_reason_(LayerPromotionReason::None) {
}

PaintLayer::~PaintLayer() {
    // 从父层移除自己
    if (parent_) {
        parent_->RemoveChild(this);
    }
    // 清理子层
    RemoveAllChildren();
}

// =========================================================================
// 树操作
// =========================================================================

void PaintLayer::AddChild(PaintLayer* child) {
    if (!child || child == this) return;
    
    // 如果已经有父层，先从原父层移除
    if (child->parent_) {
        child->parent_->RemoveChild(child);
    }
    
    child->parent_ = this;
    children_.push_back(child);
    
    // 标记 z-order 列表需要更新
    DirtyZOrderLists();
    
    // 向上传播到 stacking context
    PaintLayer* sc = StackingContext();
    if (sc) {
        sc->DirtyZOrderLists();
    }
}

void PaintLayer::RemoveChild(PaintLayer* child) {
    if (!child) return;
    
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        children_.erase(it);
        
        // 标记 z-order 列表需要更新
        DirtyZOrderLists();
        
        // 向上传播到 stacking context
        PaintLayer* sc = StackingContext();
        if (sc) {
            sc->DirtyZOrderLists();
        }
    }
}

void PaintLayer::InsertBefore(PaintLayer* child, PaintLayer* before) {
    if (!child || child == this) return;
    
    // 如果已经有父层，先从原父层移除
    if (child->parent_) {
        child->parent_->RemoveChild(child);
    }
    
    child->parent_ = this;
    
    if (before) {
        auto it = std::find(children_.begin(), children_.end(), before);
        if (it != children_.end()) {
            children_.insert(it, child);
        } else {
            children_.push_back(child);
        }
    } else {
        children_.push_back(child);
    }
    
    // 标记 z-order 列表需要更新
    DirtyZOrderLists();
    
    // 向上传播到 stacking context
    PaintLayer* sc = StackingContext();
    if (sc) {
        sc->DirtyZOrderLists();
    }
}

void PaintLayer::RemoveAllChildren() {
    for (auto* child : children_) {
        child->parent_ = nullptr;
    }
    children_.clear();
    
    // 标记 z-order 列表需要更新
    DirtyZOrderLists();
}

// =========================================================================
// Stacking Context
// =========================================================================

bool PaintLayer::IsStackingContext() const {
    if (!render_object_) return false;
    
    const auto& style = render_object_->GetComputedStyle();
    
    // 根元素总是 stacking context
    if (!parent_) return true;
    
    // position: absolute/relative/fixed/sticky 且 z-index != auto
    // 注意：z-index 默认值是 0，但 auto 和 0 是不同的
    // 这里简化处理：如果有定位且 z-index != 0，则创建 stacking context
    bool has_position = (style.position == "absolute" || 
                         style.position == "relative" || 
                         style.position == "fixed" ||
                         style.position == "sticky");
    if (has_position && style.z_index != 0) {
        return true;
    }
    
    // opacity < 1
    if (style.opacity < 1.0f) {
        return true;
    }
    
    // transform != none
    if (style.transform.has_value()) {
        return true;
    }
    
    // filter != none
    if (style.filter.has_value()) {
        return true;
    }
    
    // will-change: transform/opacity
    if (!style.will_change.empty()) {
        if (style.will_change.find("transform") != std::string::npos ||
            style.will_change.find("opacity") != std::string::npos) {
            return true;
        }
    }
    
    // position: fixed 总是创建 stacking context
    if (style.position == "fixed") {
        return true;
    }
    
    return false;
}

PaintLayer* PaintLayer::StackingContext() const {
    // 使用缓存
    if (!stacking_context_dirty_ && cached_stacking_context_) {
        return cached_stacking_context_;
    }
    
    // 向上查找最近的 stacking context 祖先
    PaintLayer* current = parent_;
    while (current) {
        if (current->IsStackingContext()) {
            cached_stacking_context_ = current;
            stacking_context_dirty_ = false;
            return current;
        }
        current = current->parent_;
    }
    
    cached_stacking_context_ = nullptr;
    stacking_context_dirty_ = false;
    return nullptr;
}

int PaintLayer::ZIndex() const {
    if (!render_object_) return 0;
    return render_object_->GetComputedStyle().z_index;
}

// =========================================================================
// Z-Order 列表
// =========================================================================

void PaintLayer::UpdateZOrderLists() {
    if (!z_order_dirty_) return;
    
    // 清空列表
    pos_z_order_list_.clear();
    neg_z_order_list_.clear();
    
    // 只有 stacking context 需要维护 z-order 列表
    if (!IsStackingContext()) {
        z_order_dirty_ = false;
        return;
    }
    
    // 收集子层
    CollectZOrderLayers();
    
    // 排序
    SortZOrderLists();
    
    z_order_dirty_ = false;
}

void PaintLayer::CollectZOrderLayers() {
    // 递归收集所有需要参与排序的子层
    for (auto* child : children_) {
        if (!child) continue;
        
        int z = child->ZIndex();
        
        // 如果子层是 stacking context，它有自己的排序
        // 但仍然需要参与父 stacking context 的排序
        if (z >= 0) {
            pos_z_order_list_.push_back(child);
        } else {
            neg_z_order_list_.push_back(child);
        }
        
        // 如果子层不是 stacking context，递归收集其子层
        if (!child->IsStackingContext()) {
            // 子层的子层参与当前 stacking context 的排序
            for (auto* grandchild : child->children_) {
                if (!grandchild) continue;
                int gz = grandchild->ZIndex();
                if (gz >= 0) {
                    pos_z_order_list_.push_back(grandchild);
                } else {
                    neg_z_order_list_.push_back(grandchild);
                }
            }
        }
    }
}

void PaintLayer::SortZOrderLists() {
    // 按 z-index 升序排序（低的先绘制）
    // 相同 z-index 按文档顺序（在 children_ 中的顺序）
    auto compare = [](PaintLayer* a, PaintLayer* b) {
        return a->ZIndex() < b->ZIndex();
    };
    
    std::stable_sort(pos_z_order_list_.begin(), pos_z_order_list_.end(), compare);
    std::stable_sort(neg_z_order_list_.begin(), neg_z_order_list_.end(), compare);
}

// =========================================================================
// Compositing
// =========================================================================

bool PaintLayer::NeedsCompositing() const {
    if (!render_object_) return false;
    
    const auto& style = render_object_->GetComputedStyle();
    
    // will-change: transform/opacity
    if (!style.will_change.empty()) {
        if (style.will_change.find("transform") != std::string::npos) {
            promotion_reason_ = LayerPromotionReason::WillChangeTransform;
            return true;
        }
        if (style.will_change.find("opacity") != std::string::npos) {
            promotion_reason_ = LayerPromotionReason::WillChangeOpacity;
            return true;
        }
    }
    
    // position: fixed
    if (style.position == "fixed") {
        promotion_reason_ = LayerPromotionReason::PositionFixed;
        return true;
    }
    
    // 活动的 transform/opacity 动画
    // TODO: 检查动画状态
    
    // 可滚动容器
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    if (overflow_y == "scroll" || overflow_y == "auto") {
        float max_scroll = render_object_->GetMaxScrollY();
        if (max_scroll > 0) {
            promotion_reason_ = LayerPromotionReason::ScrollableContent;
            return true;
        }
    }
    
    promotion_reason_ = LayerPromotionReason::None;
    return false;
}

LayerPromotionReason PaintLayer::GetPromotionReason() const {
    // 确保 promotion_reason_ 是最新的
    NeedsCompositing();
    return promotion_reason_;
}

void PaintLayer::EnsureCompositorLayer() {
    if (compositor_layer_) return;
    
    if (NeedsCompositing()) {
        compositor_layer_ = CreateCompositorLayer();
        if (compositor_layer_) {
            compositor_layer_->SetRenderObject(render_object_);
            compositor_layer_->SetPromotionReason(promotion_reason_);
        }
    }
}

CompositorLayer* PaintLayer::GetCompositedLayer() const {
    return compositor_layer_.get();
}

// =========================================================================
// 绘制
// =========================================================================

void PaintLayer::Paint(SkCanvas* canvas) {
    if (!canvas || !render_object_) return;
    
    // 更新 z-order 列表
    if (IsStackingContext()) {
        UpdateZOrderLists();
    }
    
    // 按 CSS stacking context 规则绘制
    // 1. 绘制负 z-index 子层
    PaintNegativeZOrderChildren(canvas);
    
    // 2. 绘制自身内容
    PaintContents(canvas);
    
    // 3. 绘制正 z-index 子层
    PaintPositiveZOrderChildren(canvas);
}

void PaintLayer::PaintContents(SkCanvas* canvas) {
    if (!canvas || !render_object_) return;
    
    // 如果有 CompositorLayer，内容已经在 GPU 纹理中
    // 这里只需要绘制到 CompositorLayer 的 canvas
    if (compositor_layer_) {
        SkCanvas* layer_canvas = compositor_layer_->GetCanvas();
        if (layer_canvas) {
            render_object_->Paint(layer_canvas);
        }
    } else {
        // 直接绘制到主 canvas
        render_object_->Paint(canvas);
    }
}

void PaintLayer::PaintNegativeZOrderChildren(SkCanvas* canvas) {
    for (auto* child : neg_z_order_list_) {
        if (child) {
            child->Paint(canvas);
        }
    }
}

void PaintLayer::PaintPositiveZOrderChildren(SkCanvas* canvas) {
    for (auto* child : pos_z_order_list_) {
        if (child) {
            child->Paint(canvas);
        }
    }
}

// =========================================================================
// Hit Testing
// =========================================================================

bool PaintLayer::HitTest(float x, float y, HitTestResult& result) {
    if (!render_object_) return false;
    
    // 更新 z-order 列表
    if (IsStackingContext()) {
        UpdateZOrderLists();
    }
    
    // 按 z-order 逆序测试（从高到低）
    
    // 1. 先测试正 z-index 子层（从高到低）
    for (auto it = pos_z_order_list_.rbegin(); it != pos_z_order_list_.rend(); ++it) {
        if (*it && (*it)->HitTest(x, y, result)) {
            return true;
        }
    }
    
    // 2. 在测试自身边界之前，先测试 fixed 子元素
    // fixed 元素使用视口坐标，不受父元素边界和滚动的限制
    const auto& children = render_object_->GetChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child_style = (*it)->GetComputedStyle();
        if (child_style.position == "fixed") {
            // fixed 元素使用原始视口坐标进行测试
            if (HitTestRenderObject(it->get(), x, y, 0, 0, result, false)) {
                return true;
            }
        }
    }
    
    // 3. 测试自身
    float abs_x, abs_y;
    GetAbsolutePosition(abs_x, abs_y);
    
    const auto& layout = render_object_->GetLayoutInfo();
    if (x >= abs_x && x < abs_x + layout.width &&
        y >= abs_y && y < abs_y + layout.height) {
        
        // 递归测试子元素（非 fixed）
        if (HitTestChildren(x, y, result)) {
            return true;
        }
        
        // 检查 pointer-events
        const auto& style = render_object_->GetComputedStyle();
        if (style.pointer_events != "none") {
            auto node = render_object_->GetNode();
            auto element = std::dynamic_pointer_cast<Element>(node);
            if (element) {
                result.element = element;
                result.render_object = render_object_->shared_from_this();
                result.local_x = x - abs_x;
                result.local_y = y - abs_y;
                return true;
            }
        }
    }
    
    // 4. 最后测试负 z-index 子层（从高到低）
    for (auto it = neg_z_order_list_.rbegin(); it != neg_z_order_list_.rend(); ++it) {
        if (*it && (*it)->HitTest(x, y, result)) {
            return true;
        }
    }
    
    return false;
}

bool PaintLayer::HitTestChildren(float x, float y, HitTestResult& result) {
    if (!render_object_) return false;
    
    // 获取当前元素的绝对位置
    float abs_x, abs_y;
    GetAbsolutePosition(abs_x, abs_y);
    
    // 处理滚动偏移
    float scroll_x = render_object_->GetScrollX();
    float scroll_y = render_object_->GetScrollY();
    float child_test_x = x + scroll_x;
    float child_test_y = y + scroll_y;
    
    // 从后向前遍历子元素（后绘制的在上面）
    // 先测试 fixed 元素（它们不受滚动影响，使用原始坐标）
    // 再测试普通元素（使用滚动调整后的坐标）
    const auto& children = render_object_->GetChildren();
    
    // 第一遍：测试 fixed 元素（使用原始视口坐标）
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child_style = (*it)->GetComputedStyle();
        if (child_style.position == "fixed") {
            if (HitTestRenderObject(it->get(), x, y, abs_x, abs_y, result, false)) {
                return true;
            }
        }
    }
    
    // 第二遍：测试非 fixed 元素（使用滚动调整后的坐标）
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child_style = (*it)->GetComputedStyle();
        if (child_style.position != "fixed") {
            if (HitTestRenderObject(it->get(), child_test_x, child_test_y, abs_x, abs_y, result, false)) {
                return true;
            }
        }
    }
    
    return false;
}

bool PaintLayer::HitTestRenderObject(
    RenderObject* render_obj,
    float x, float y,
    float offset_x, float offset_y,
    HitTestResult& result,
    bool is_root) {
    
    if (!render_obj) return false;
    
    const auto& layout = render_obj->GetLayoutInfo();
    if (!layout.is_laid_out) return false;
    
    const auto& style = render_obj->GetComputedStyle();
    bool is_fixed = (style.position == "fixed");
    
    // 计算当前元素的绝对位置
    float current_x, current_y;
    if (is_root) {
        current_x = offset_x;
        current_y = offset_y;
    } else if (is_fixed) {
        current_x = layout.x;
        current_y = layout.y;
    } else {
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
    // 先测试 fixed 元素（使用原始视口坐标）
    // 再测试非 fixed 元素（使用滚动调整后的坐标）
    const auto& children = render_obj->GetChildren();
    
    // 第一遍：测试 fixed 元素
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child_style = (*it)->GetComputedStyle();
        if (child_style.position == "fixed") {
            if (HitTestRenderObject(it->get(), x, y, current_x, current_y, result, false)) {
                return true;
            }
        }
    }
    
    // 第二遍：测试非 fixed 元素
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child_style = (*it)->GetComputedStyle();
        if (child_style.position != "fixed") {
            if (HitTestRenderObject(it->get(), child_test_x, child_test_y, current_x, current_y, result, false)) {
                return true;
            }
        }
    }
    
    // 如果 pointer-events: none，不命中当前元素
    if (pointer_events_none) {
        return false;
    }
    
    // 当前元素命中
    auto hit_node = render_obj->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(hit_node);
    if (element) {
        result.element = element;
        result.render_object = render_obj->shared_from_this();
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

// =========================================================================
// 滚动支持
// =========================================================================

bool PaintLayer::HandleWheel(float x, float y, float delta_x, float delta_y) {
    if (!render_object_) return false;
    
    // 获取绝对位置
    float abs_x, abs_y;
    GetAbsolutePosition(abs_x, abs_y);
    
    const auto& layout = render_object_->GetLayoutInfo();
    
    // 边界检查
    if (x < abs_x || x >= abs_x + layout.width ||
        y < abs_y || y >= abs_y + layout.height) {
        return false;
    }
    
    // 检查元素是否可滚动
    const auto& style = render_object_->GetComputedStyle();
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    bool allow_scroll = (overflow_y == "scroll" || overflow_y == "auto");
    
    if (allow_scroll) {
        float max_scroll_y = render_object_->GetMaxScrollY();
        if (max_scroll_y > 0) {
            render_object_->ScrollBy(delta_x, delta_y);
            return true;
        }
    }
    
    // 递归测试子层
    for (auto* child : children_) {
        if (child && child->HandleWheel(x, y, delta_x, delta_y)) {
            return true;
        }
    }
    
    return false;
}

// =========================================================================
// Position: Fixed 支持
// =========================================================================

bool PaintLayer::IsFixedPositioned() const {
    if (!render_object_) return false;
    return render_object_->GetComputedStyle().position == "fixed";
}

void PaintLayer::GetAbsolutePosition(float& abs_x, float& abs_y) const {
    if (!render_object_) {
        abs_x = 0;
        abs_y = 0;
        return;
    }
    
    const auto& layout = render_object_->GetLayoutInfo();
    const auto& style = render_object_->GetComputedStyle();
    
    if (style.position == "fixed") {
        // position: fixed 元素：layout.x/y 已经是视口绝对坐标
        abs_x = layout.x;
        abs_y = layout.y;
    } else if (parent_) {
        // 其他元素：累加父元素偏移
        float parent_x, parent_y;
        parent_->GetAbsolutePosition(parent_x, parent_y);
        abs_x = parent_x + layout.x;
        abs_y = parent_y + layout.y;
    } else {
        // 根元素
        abs_x = layout.x;
        abs_y = layout.y;
    }
}

// =========================================================================
// 调试
// =========================================================================

std::string PaintLayer::ToDebugString() const {
    std::ostringstream oss;
    
    oss << "PaintLayer{";
    
    if (render_object_) {
        auto node = render_object_->GetNode();
        if (node) {
            auto element = std::dynamic_pointer_cast<Element>(node);
            if (element) {
                oss << "tag=" << element->GetTagName();
                std::string id = element->GetAttribute("id");
                if (!id.empty()) {
                    oss << "#" << id;
                }
            }
        }
    }
    
    oss << ", z=" << ZIndex();
    oss << ", sc=" << (IsStackingContext() ? "yes" : "no");
    oss << ", comp=" << (HasCompositedLayer() ? "yes" : "no");
    
    if (HasCompositedLayer()) {
        oss << ", reason=" << CompositorLayer::PromotionReasonToString(promotion_reason_);
    }
    
    oss << ", children=" << children_.size();
    oss << "}";
    
    return oss.str();
}

void PaintLayer::DumpTree(int indent) const {
    std::string prefix(indent * 2, ' ');
    std::cout << prefix << ToDebugString() << std::endl;
    
    for (auto* child : children_) {
        if (child) {
            child->DumpTree(indent + 1);
        }
    }
}

} // namespace lightui
