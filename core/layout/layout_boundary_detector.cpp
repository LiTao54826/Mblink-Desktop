/**
 * @file layout_boundary_detector.cpp
 * @brief 布局边界检测器实现
 */

#include "layout_boundary_detector.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/render/objects/render_object.h"

namespace lightui {

// ============================================================================
// 边界类型检测
// ============================================================================

LayoutBoundaryDetector::BoundaryType
LayoutBoundaryDetector::DetectBoundaryType(Element* element) {
    if (!element) {
        return BoundaryType::None;
    }

    auto render_object = element->GetRenderObject();
    if (!render_object) {
        return BoundaryType::None;
    }

    return DetectBoundaryType(render_object.get());
}

LayoutBoundaryDetector::BoundaryType
LayoutBoundaryDetector::DetectBoundaryType(RenderObject* render_object) {
    if (!render_object) {
        return BoundaryType::None;
    }

    const auto& style = render_object->GetComputedStyle();

    // 1. 脱离文档流 - 最强的布局边界
    if (IsOutOfFlow(style)) {
        return BoundaryType::OutOfFlow;
    }

    // 2. CSS Containment
    if (HasLayoutContainment(style)) {
        return BoundaryType::CSSContainment;
    }

    // 3. 滚动容器 + 固定尺寸
    if (IsScrollContainer(style) && HasFixedSize(style)) {
        return BoundaryType::ScrollContainer;
    }

    // 4. 固定尺寸容器（非 auto）
    if (HasFixedSize(style)) {
        return BoundaryType::FixedSize;
    }

    // 5. Flex 固定项
    if (IsFlexFixedItem(render_object)) {
        return BoundaryType::FlexFixed;
    }

    return BoundaryType::None;
}

// ============================================================================
// 辅助检测方法
// ============================================================================

bool LayoutBoundaryDetector::IsOutOfFlow(const ComputedStyle& style) {
    return style.position == "fixed" || style.position == "absolute";
}

bool LayoutBoundaryDetector::IsScrollContainer(const ComputedStyle& style) {
    // 检查 overflow-x 或 overflow-y 是否为 scroll 或 auto
    bool has_scroll_x = (style.overflow_x == "scroll" || style.overflow_x == "auto");
    bool has_scroll_y = (style.overflow_y == "scroll" || style.overflow_y == "auto");
    
    // 也检查简写属性 overflow
    bool has_scroll = (style.overflow == "scroll" || style.overflow == "auto");
    
    return has_scroll_x || has_scroll_y || has_scroll;
}

bool LayoutBoundaryDetector::HasFixedSize(const ComputedStyle& style) {
    // 检查 width 是否是固定值（px, vw, vh 等，不是 auto 或 percent）
    bool width_fixed = false;
    if (style.width.unit == CSSUnit::PX ||
        style.width.unit == CSSUnit::VW ||
        style.width.unit == CSSUnit::VH ||
        style.width.unit == CSSUnit::VMIN ||
        style.width.unit == CSSUnit::VMAX) {
        width_fixed = true;
    }

    // 检查 height 是否是固定值
    bool height_fixed = false;
    if (style.height.unit == CSSUnit::PX ||
        style.height.unit == CSSUnit::VW ||
        style.height.unit == CSSUnit::VH ||
        style.height.unit == CSSUnit::VMIN ||
        style.height.unit == CSSUnit::VMAX) {
        height_fixed = true;
    }

    // 两者都必须是固定值
    return width_fixed && height_fixed;
}

bool LayoutBoundaryDetector::HasLayoutContainment(const ComputedStyle& style) {
    return style.HasLayoutContainment();
}

bool LayoutBoundaryDetector::IsFlexFixedItem(RenderObject* render_object) {
    if (!render_object) {
        return false;
    }

    const auto& style = render_object->GetComputedStyle();

    // 检查是否是 flex: 0 0 <size> 的项
    // flex-grow = 0, flex-shrink = 0, flex-basis 是固定值
    if (style.flex_grow != 0.0f || style.flex_shrink != 0.0f) {
        return false;
    }

    // flex-basis 必须是固定值（不是 auto）
    if (style.flex_basis.unit == CSSUnit::AUTO) {
        return false;
    }

    // 检查父元素是否是 flex 容器
    auto parent = render_object->GetParent();
    if (!parent) {
        return false;
    }

    const auto& parent_style = parent->GetComputedStyle();
    return parent_style.display == RenderObjectType::FLEX;
}

// ============================================================================
// 祖先查找
// ============================================================================

Element* LayoutBoundaryDetector::FindNearestLayoutBoundary(Node* node) {
    if (!node) {
        return nullptr;
    }

    // 从父节点开始向上遍历
    auto current = node->GetParentNode();
    
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::static_pointer_cast<Element>(current);
            BoundaryType type = DetectBoundaryType(element.get());
            
            if (type != BoundaryType::None) {
                return element.get();
            }
        }
        
        current = current->GetParentNode();
    }

    return nullptr;
}

RenderObject* LayoutBoundaryDetector::FindNearestLayoutBoundary(RenderObject* render_object) {
    if (!render_object) {
        return nullptr;
    }

    // 从父渲染对象开始向上遍历
    auto current = render_object->GetParent();
    
    while (current) {
        BoundaryType type = DetectBoundaryType(current.get());
        
        if (type != BoundaryType::None) {
            return current.get();
        }
        
        current = current->GetParent();
    }

    return nullptr;
}

// ============================================================================
// 调试辅助
// ============================================================================

const char* LayoutBoundaryDetector::BoundaryTypeToString(BoundaryType type) {
    switch (type) {
        case BoundaryType::None:
            return "None";
        case BoundaryType::OutOfFlow:
            return "OutOfFlow";
        case BoundaryType::ScrollContainer:
            return "ScrollContainer";
        case BoundaryType::FixedSize:
            return "FixedSize";
        case BoundaryType::CSSContainment:
            return "CSSContainment";
        case BoundaryType::FlexFixed:
            return "FlexFixed";
        default:
            return "Unknown";
    }
}

}  // namespace lightui
