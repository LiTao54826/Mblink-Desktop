/**
 * @file window_dom_observer.cpp
 * @brief Window 的 DOM 观察者实现
 *
 * 从 window.cpp 提取的 DOM 观察者类实现。
 * 监听 DOM 变化并触发窗口重绘。
 */

#include "window_dom_observer.h"
#include "window.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/dom/text.h"
#include "core/render/objects/render_object.h"
#include "core/render/css/style_resolver.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/layout/layout_engine.h"
#include "core/layout/layout_boundary_detector.h"
#include "core/layout/incremental_layout_manager.h"
#include "core/lexbor/style_manager.h"
#include "core/compositor/layer_tree_manager.h"
#include <vector>
#include <cstdlib>
#include <sstream>
#include <iostream>

namespace mbink {

namespace {

void AddDirtyRectForRenderObject(Window* window, Element* owner, RenderObject* render_obj) {
    if (!window || !render_obj) {
        return;
    }

    SkRect bounds = render_obj->GetViewportBoundingRect();
    if (bounds.isEmpty()) {
        bounds = render_obj->GetBoundingRect();
    }
    if (!bounds.isEmpty()) {
        if (owner) {
            owner->SetDirtyRect(bounds);
        }
        window->AddDirtyRect(bounds);
    }
}

bool HasBuiltinHoverStyle(const std::string& tag_name) {
    return tag_name == "button" || tag_name == "a";
}

bool LayoutSensitiveStyleChanged(const ComputedStyle& old_style,
                                 const ComputedStyle& new_style) {
    return old_style.display != new_style.display ||
           old_style.width != new_style.width ||
           old_style.height != new_style.height ||
           old_style.min_width != new_style.min_width ||
           old_style.min_height != new_style.min_height ||
           old_style.max_width != new_style.max_width ||
           old_style.max_height != new_style.max_height ||
           old_style.margin_left != new_style.margin_left ||
           old_style.margin_right != new_style.margin_right ||
           old_style.margin_top != new_style.margin_top ||
           old_style.margin_bottom != new_style.margin_bottom ||
           old_style.padding_left != new_style.padding_left ||
           old_style.padding_right != new_style.padding_right ||
           old_style.padding_top != new_style.padding_top ||
           old_style.padding_bottom != new_style.padding_bottom ||
           old_style.border_left_width != new_style.border_left_width ||
           old_style.border_right_width != new_style.border_right_width ||
           old_style.border_top_width != new_style.border_top_width ||
           old_style.border_bottom_width != new_style.border_bottom_width ||
           old_style.text_align != new_style.text_align ||
           old_style.justify_content != new_style.justify_content ||
           old_style.align_items != new_style.align_items ||
           old_style.align_self != new_style.align_self ||
           old_style.flex_direction != new_style.flex_direction ||
           old_style.flex_wrap != new_style.flex_wrap ||
           old_style.flex_grow != new_style.flex_grow ||
           old_style.flex_shrink != new_style.flex_shrink ||
           old_style.flex_basis != new_style.flex_basis ||
           old_style.position != new_style.position ||
           old_style.left != new_style.left ||
           old_style.right != new_style.right ||
           old_style.top != new_style.top ||
           old_style.bottom != new_style.bottom ||
           old_style.overflow_x != new_style.overflow_x ||
           old_style.overflow_y != new_style.overflow_y ||
           old_style.white_space != new_style.white_space ||
           old_style.font_size != new_style.font_size ||
           old_style.font_weight != new_style.font_weight ||
           old_style.font_family != new_style.font_family ||
           old_style.line_height != new_style.line_height;
}

void SyncLayoutStyle(Window* window,
                     RenderObject* render_obj,
                     const ComputedStyle& new_style,
                     bool needs_layout_sync) {
    if (!window || !render_obj || !window->GetLayoutEngine()) {
        return;
    }

    auto* layout_engine = window->GetLayoutEngine();
    if (!needs_layout_sync) {
        return;
    }

    if (layout_engine->HasElement(render_obj)) {
        layout_engine->UpdateStyle(render_obj, new_style);
        layout_engine->MarkNeedsLayout(render_obj);
        return;
    }

    auto ancestor = render_obj->GetParent();
    while (ancestor) {
        if (layout_engine->HasElement(ancestor.get())) {
            layout_engine->MarkNeedsLayout(ancestor.get());
            break;
        }
        ancestor = ancestor->GetParent();
    }
}

void ApplyInheritedTextStyle(Window* window,
                             Element* owner,
                             RenderObject* text_render,
                             const ComputedStyle* parent_style) {
    if (!window || !text_render || !parent_style) {
        return;
    }

    const auto old_style = text_render->GetComputedStyle();
    ComputedStyle text_style = text_render->GetComputedStyle();
    text_style.color = parent_style->color;
    text_style.font_family = parent_style->font_family;
    text_style.font_size = parent_style->font_size;
    text_style.font_weight = parent_style->font_weight;
    text_style.font_style = parent_style->font_style;
    text_style.line_height = parent_style->line_height;
    text_style.text_align = parent_style->text_align;
    text_style.text_decoration = parent_style->text_decoration;
    text_style.text_shadow = parent_style->text_shadow;

    text_render->SetComputedStyle(text_style);
    const bool needs_layout_sync = LayoutSensitiveStyleChanged(old_style, text_style);
    if (needs_layout_sync) {
        text_render->MarkNeedsLayout();
    }
    text_render->MarkNeedsPaint();
    SyncLayoutStyle(window, text_render, text_style, needs_layout_sync);
    text_render->InvalidatePaintCache();
    AddDirtyRectForRenderObject(window, owner, text_render);
}

void RestyleInteractionPseudoClassSubtree(Window* window,
                                          StyleResolver& resolver,
                                          const std::shared_ptr<Element>& element,
                                          const ComputedStyle* parent_style) {
    if (!window || !element) {
        return;
    }

    auto render_obj = element->GetRenderObject();
    ComputedStyle new_style;
    ComputedStyle non_render_style;
    const ComputedStyle* child_parent_style = parent_style;

    if (render_obj) {
        const auto old_style = render_obj->GetComputedStyle();
        AddDirtyRectForRenderObject(window, element.get(), render_obj.get());

        new_style = resolver.ResolveStyle(element, parent_style);
        render_obj->SetComputedStyle(new_style);

        const bool needs_layout_sync = LayoutSensitiveStyleChanged(old_style, new_style);
        if (needs_layout_sync) {
            render_obj->MarkNeedsLayout();
            render_obj->MarkNeedsPaint();
        } else {
            render_obj->MarkNeedsPaint();
        }

        SyncLayoutStyle(window, render_obj.get(), new_style, needs_layout_sync);
        render_obj->InvalidatePaintCache();
        AddDirtyRectForRenderObject(window, element.get(), render_obj.get());
        child_parent_style = &render_obj->GetComputedStyle();
    } else {
        non_render_style = resolver.ResolveStyle(element, parent_style);
        child_parent_style = &non_render_style;
    }

    for (const auto& child : element->GetChildNodes()) {
        if (!child) {
            continue;
        }

        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto child_element = std::static_pointer_cast<Element>(child);
            RestyleInteractionPseudoClassSubtree(window, resolver, child_element, child_parent_style);
        } else if (child->GetNodeType() == NodeType::TEXT_NODE) {
            ApplyInheritedTextStyle(window, element.get(), child->GetRenderObject().get(), child_parent_style);
        }
    }
}


}  // namespace

WindowDOMObserver::WindowDOMObserver(Window* window) : window_(window) {}

void WindowDOMObserver::OnNodeAdded(Node* node, Node* parent) {
    // 处理 <style> 元素的添加：触发样式解析和渲染树重建
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto elem = std::dynamic_pointer_cast<Element>(node->shared_from_this());
        if (elem && elem->GetTagName() == "style") {
            // 获取 Document 的 StyleManager 并解析样式
            if (auto doc = node->GetOwnerDocument()) {
                if (auto style_manager = doc->GetStyleManager()) {
                    style_manager->ParseStyleElement(elem.get());

                    // 关键修复：样式表变化后需要重建渲染树
                    // 否则新样式不会应用到已有的 DOM 元素
                    if (window_) {
                        window_->InvalidateRenderTree();
                        window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                    }
                }
            }
            return;  // <style> 元素本身不需要渲染
        }
    }

    if (window_ && !IsInBatch(node)) {
        // =========================================================================
        // 增量布局边界优化
        // =========================================================================

        // 检查是否为元素节点
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node->shared_from_this());
            if (elem && elem->GetRenderObject()) {
                const auto& style = elem->GetRenderObject()->GetComputedStyle();

                // 1. 检查是否为脱离文档流的元素 (position: fixed/absolute)
                if (LayoutBoundaryDetector::IsOutOfFlow(style)) {
                    // 关键修复：禁止在 DOMObserver 增量路径里直接改挂接 RenderObject。
                    // 这会和 RenderTreeSynchronizer 的插入/重建路径竞争，导致
                    // 1) fixed/absolute 元素出现双实例（文档流坐标 + 视口坐标）
                    // 2) 动画可能更新到“不可见那一份”对象
                    // 3) 关闭后残留对象持续触发脏标记日志刷屏
                    //
                    // 对 out-of-flow 统一走渲染树重建，确保 DOM->RenderTree 单一真源。
                    window_->InvalidateRenderTree();
                    window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                    return;
                }

                // 2. 查找最近的布局边界祖先
                Element* boundary = LayoutBoundaryDetector::FindNearestLayoutBoundary(parent);
                if (boundary) {
                    // 有布局边界：只标记边界需要重新布局
                    if (auto* manager = window_->GetIncrementalLayoutManager()) {
                        manager->MarkBoundaryNeedsLayout(boundary);

                        // 如果是滚动容器，更新滚动尺寸
                        if (auto boundary_ro = boundary->GetRenderObject()) {
                            if (LayoutBoundaryDetector::IsScrollContainer(boundary_ro->GetComputedStyle())) {
                                manager->UpdateScrollContainerSize(boundary);
                            }
                        }
                    }

                    // 标记节点需要样式重算和布局
                    node->SetNeedsStyleRecalc(StyleChangeType::kSubtreeStyleChange);
                    node->SetNeedsLayout();

                    window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                    return;
                }
            }
        }

        // =========================================================================
        // 回退到原有逻辑（无布局边界时）
        // =========================================================================

        // 1. 标记节点需要样式重算
        node->SetNeedsStyleRecalc(StyleChangeType::kSubtreeStyleChange);

        // 2. 标记节点需要布局
        node->SetNeedsLayout();

        // 3. 标记父节点需要布局（子节点变化影响父节点布局）
        // 同时标记父节点的 RenderObject，清除 content_height_ 缓存
        if (parent) {
            parent->SetNeedsLayout();
            // 关键：向上传播到所有祖先的 RenderObject
            // 这确保滚动容器等祖先节点的 content_height_ 缓存被清除
            if (auto parent_ro = parent->GetRenderObject()) {
                parent_ro->MarkNeedsLayout(true);
            }
        }

        // 4. 增量更新：标记需要重绘
        // DirtyNodeTracker 已经在 Node::AppendChild 中记录了变化
        // RenderTreeSynchronizer 会在渲染时根据变化区域大小决定是增量更新还是全量重建
        window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
    }
}

void WindowDOMObserver::OnNodeRemoved(Node* node, Node* parent) {
    if (window_ && !IsInBatch(node)) {
        // =========================================================================
        // 增量布局边界优化
        // =========================================================================

        // 检查是否为元素节点
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node->shared_from_this());
            if (elem && elem->GetRenderObject()) {
                const auto& style = elem->GetRenderObject()->GetComputedStyle();

                // 1. 检查是否为脱离文档流的元素 (position: fixed/absolute)
                if (LayoutBoundaryDetector::IsOutOfFlow(style)) {
                    // 关键修复：禁止在 DOMObserver 增量路径里直接移除 out-of-flow RenderObject。
                    // 直接移除会与同步器/层树路径竞争，可能留下残留对象或脏状态。
                    // 统一触发重建，确保 fixed/absolute 元素生命周期一致。
                    window_->InvalidateRenderTree();
                    window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                    return;
                }

                // 2. 查找最近的布局边界祖先
                Element* boundary = LayoutBoundaryDetector::FindNearestLayoutBoundary(parent);
                if (boundary) {
                    // 有布局边界：只标记边界需要重新布局
                    if (auto* manager = window_->GetIncrementalLayoutManager()) {
                        manager->MarkBoundaryNeedsLayout(boundary);

                        // 如果是滚动容器，更新滚动尺寸
                        if (auto boundary_ro = boundary->GetRenderObject()) {
                            if (LayoutBoundaryDetector::IsScrollContainer(boundary_ro->GetComputedStyle())) {
                                manager->UpdateScrollContainerSize(boundary);
                            }
                        }
                    }

                    // 标记父节点需要布局
                    if (parent) {
                        parent->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
                        parent->SetNeedsLayout();
                    }

                    window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                    return;
                }
            }
        }

        // =========================================================================
        // 回退到原有逻辑（无布局边界时）
        // =========================================================================

        // 1. 标记父节点需要布局（子节点移除影响父节点布局）
        // 同时标记父节点的 RenderObject，清除 content_height_ 缓存
        if (parent) {
            parent->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
            parent->SetNeedsLayout();
            // 关键：向上传播到所有祖先的 RenderObject
            // 这确保滚动容器等祖先节点的 content_height_ 缓存被清除
            if (auto parent_ro = parent->GetRenderObject()) {
                parent_ro->MarkNeedsLayout(true);
            }

            // 关键修复：如果父节点是 fixed 元素，触发渲染树重建
            // 因为 fixed 元素的子元素变化会影响层的边界
            if (parent->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto parent_elem = std::dynamic_pointer_cast<Element>(parent->shared_from_this());
                if (parent_elem && parent_elem->GetRenderObject()) {
                    const auto& parent_style = parent_elem->GetRenderObject()->GetComputedStyle();
                    if (parent_style.position == "fixed") {
                        window_->InvalidateRenderTree();
                        window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                        return;
                    }
                }
            }
        }

        // 2. 增量更新：标记需要重绘
        // DirtyNodeTracker 已经在 Node::RemoveChild 中记录了变化
        // RenderTreeSynchronizer 会在渲染时根据变化区域大小决定是增量更新还是全量重建
        window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
    }
}

void WindowDOMObserver::OnAttributeChanged(Element* element,
                                           const std::string& name,
                                           const std::string& old_value,
                                           const std::string& new_value) {
    if (window_ && !IsInBatch(element)) {
        // Phase 1: 精确脏区域标记
        // 获取关联的 RenderObject，标记其需要重绘
        if (auto render_obj = element->GetRenderObject()) {
            render_obj->MarkNeedsPaint();
            // 记录脏矩形（旧位置）
            AddDirtyRectForRenderObject(window_, element, render_obj.get());

            // 关键修复：当 style 或 class 属性变化时，需要重新解析样式
            // style: 确保 transform 等属性的动态更新能正确生效
            // class: 确保 CSS 类选择器匹配的样式能正确应用（如 .cm-activeLine）
            if (name == "style" || name == "class") {
                // 通用路径：class/style 变化只作用于当前元素，布局传播由样式解析结果和布局引擎决定。
                // 避免为具体组件写死额外的祖先/后代 dirty 扩散逻辑。

                // 保存旧的 display 值，用于检测可见性变化
                const auto& old_style = render_obj->GetComputedStyle();
                RenderObjectType old_display = old_style.display;

                StyleResolver resolver;
                if (window_->GetDocument() && window_->GetDocument()->GetStyleManager()) {
                    resolver.SetStyleManager(window_->GetDocument()->GetStyleManager());
                }
                auto elem_ptr = std::static_pointer_cast<Element>(element->shared_from_this());
                // 获取父元素样式用于继承
                const ComputedStyle* parent_style = nullptr;
                if (auto parent_node = element->GetParentNode()) {
                    if (parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                        auto parent_elem = std::static_pointer_cast<Element>(parent_node);
                        if (auto parent_render = parent_elem->GetRenderObject()) {
                            parent_style = &parent_render->GetComputedStyle();
                        }
                    }
                }
                auto new_style = resolver.ResolveStyle(elem_ptr, parent_style);

                // 关键修复：检测 display 属性变化（修复 CSS 类切换不触发渲染树更新的 Bug）
                // 当 class 属性变化导致 display 从 none 变为其他值（或反之）时，需要重建渲染树
                bool was_none = (old_display == RenderObjectType::NONE);
                bool is_none = (new_style.display == RenderObjectType::NONE);
                if (was_none != is_none) {
                    // 可见性发生变化，需要重建渲染树
                    window_->InvalidateRenderTree();
                    window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                    return;
                }

                render_obj->SetComputedStyle(new_style);
                render_obj->InvalidatePaintCache();

                // 关键修复：同步更新布局引擎中的样式
                // UpdateStyle 内部会检查布局相关属性是否变化
                // 只有布局属性变化时才会标记 needs_layout
                if (window_->GetLayoutEngine()) {
                    auto* layout_engine = window_->GetLayoutEngine();

                    if (layout_engine->HasElement(render_obj.get())) {
                        // 元素在布局树中，直接更新样式
                        layout_engine->UpdateStyle(render_obj.get(), new_style);
                    } else {
                        // 关键修复：元素不在布局树中（如 INLINE_FLEX 的子孙元素）
                        // UpdateStyle 对这些元素静默失败，需要向上查找最近的
                        // 有 LayoutNode 的祖先，标记其需要重新布局
                        // 这样 Wrapper(inline-flex) 会重新触发 LayoutAsFlex()，
                        // 从而让 Track 重新 Layout()，进而让 Dot 的 left 生效
                        auto ancestor = render_obj->GetParent();
                        while (ancestor) {
                            if (layout_engine->HasElement(ancestor.get())) {
                                layout_engine->MarkNeedsLayout(ancestor.get());
                                break;
                            }
                            ancestor = ancestor->GetParent();
                        }
                    }
                }
            }
        }
        window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
        // 注意：属性变化不调用 InvalidateRenderTree()，保持渲染树结构
    }
}

void WindowDOMObserver::OnStyleChanged(Element* element,
                                       const std::string& property,
                                       const std::string& old_value,
                                       const std::string& new_value) {
    if (window_ && !IsInBatch(element)) {
        // 特殊处理: display 属性变化影响元素的 RenderObject 存在性
        // display: none 的元素没有 RenderObject，变为 block/flex 等需要创建
        // 反之亦然，需要删除 RenderObject
        if (property == "display") {
            bool was_none = (old_value == "none" || old_value.empty());
            bool is_none = (new_value == "none");
            if (was_none != is_none) {
                // 可见性发生变化，需要重建渲染树
                window_->InvalidateRenderTree();
                window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                return;
            }
        }

        // 获取渲染对象
        auto render_obj = element->GetRenderObject();
        if (render_obj) {
            // 重新解析样式以获取新的样式配置
            StyleResolver resolver;
            if (window_->GetDocument() && window_->GetDocument()->GetStyleManager()) {
                resolver.SetStyleManager(window_->GetDocument()->GetStyleManager());
            }

            // 获取父元素样式用于继承
            const ComputedStyle* parent_style = nullptr;
            if (auto parent_node = element->GetParentNode()) {
                if (parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto parent_elem = std::static_pointer_cast<Element>(parent_node);
                    if (auto parent_render = parent_elem->GetRenderObject()) {
                        parent_style = &parent_render->GetComputedStyle();
                    }
                }
            }

            auto new_style = resolver.ResolveStyle(
                std::static_pointer_cast<Element>(element->shared_from_this()),
                parent_style);

            render_obj->SetComputedStyle(new_style);

            // 某些样式属性只影响绘制，不影响布局
            static const std::vector<std::string> paint_only_props = {
                "color", "background-color", "background-image",
                "border-color", "opacity", "visibility",
                "box-shadow", "text-shadow", "outline",
                "cursor", "caret-color", "text-decoration-color"
            };

            bool is_paint_only = false;
            for (const auto& prop : paint_only_props) {
                if (property == prop) {
                    is_paint_only = true;
                    break;
                }
            }

            if (is_paint_only) {
                // Paint-only 属性：只标记需要重绘，不需要布局
                render_obj->MarkNeedsPaint();
            } else {
                // 其他属性可能影响布局
                render_obj->MarkNeedsLayout();
                render_obj->MarkNeedsPaint();

                // 关键修复：同步更新布局引擎中的样式！
                // 与 OnAttributeChanged 中 style/class 变化的处理保持一致
                // 没有这一步，LayoutNode 不会被标记为 needs_layout，
                // ComputeIncrementalLayout() 找不到脏节点，布局不会重新计算，
                // 导致 absolute 定位元素的 left/top 等属性变化不生效
                if (window_->GetLayoutEngine()) {
                    auto* layout_engine = window_->GetLayoutEngine();

                    // 首先尝试直接更新当前元素的 LayoutNode
                    if (layout_engine->HasElement(render_obj.get())) {
                        layout_engine->UpdateStyle(render_obj.get(), new_style);
                    } else {
                        // 当前元素不在布局树中（例如 INLINE_FLEX 的子孙元素）
                        // 需要向上查找最近的在布局树中的祖先，标记其 LayoutNode 为 dirty
                        // 这样 ComputeIncrementalLayout 才能重新计算布局，
                        // 进而触发 ReadLayoutResults → RenderInlineFlex::Layout() 等
                        // 重新定位 absolute 子元素
                        auto ancestor = render_obj->GetParent();
                        while (ancestor) {
                            if (layout_engine->HasElement(ancestor.get())) {
                                layout_engine->MarkNeedsLayout(ancestor.get());
                                break;
                            }
                            ancestor = ancestor->GetParent();
                        }
                    }
                }
            }

            render_obj->InvalidatePaintCache();

            // 记录脏矩形
            SkRect bounds = render_obj->GetBoundingRect();
            if (!bounds.isEmpty()) {
                element->SetDirtyRect(bounds);
                window_->AddDirtyRect(bounds);
            }
        }
        window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
    }
}

void WindowDOMObserver::OnTextChanged(Node* node,
                                      const std::string& old_text,
                                      const std::string& new_text) {
    if (window_ && !IsInBatch(node)) {
        (void)old_text;
        (void)new_text;

        // 文本变化统一走 DirtyNodeTracker + RenderTreeSynchronizer authoritative 路径，
        // Observer 侧仅做通用脏标记与重绘请求，避免与同步器重复打脏造成时序抖动。
        node->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
        node->SetNeedsLayout();

        // 不在这里直接 SetText / UpdateContentVersion / 逐节点 MarkNeedsLayout，
        // 统一由 ProcessTextChanges 执行，确保单一语义入口。
        window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
    }
}


void WindowDOMObserver::OnSubtreeModified(Node* root) {
    if (window_) {
        // 检查是否是批量更新结束后的通知
        // 如果是，应该根据 DirtyNodeTracker 中的变化类型决定是否需要全量重建
        auto doc = window_->GetDocument();
        if (doc) {
            const auto& tracker = doc->GetDirtyTracker();

            // 如果只有文本变化，不需要全量重建
            // 文本内容更新统一由 RenderTreeSynchronizer::ProcessTextChanges 处理，
            // 这里不再重复 SetText/MarkNeedsLayout，避免双路径重复导致时序抖动。
            if (tracker.GetTextChangeCount() > 0 &&
                tracker.GetStructuralChangeCount() == 0) {
                // 保持增量路径：触发重绘，但不触发全量渲染树失效
                window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                return;
            }

            // 基于区域大小判断是否需要全量重建
            if (tracker.GetStructuralChangeCount() > 0) {
                // 计算变化区域的总面积
                float total_change_area = 0.0f;
                int width = 800, height = 600;
                window_->GetSize(&width, &height);
                float viewport_area = static_cast<float>(width * height);

                for (const auto& change : tracker.GetStructuralChanges()) {
                    auto parent = change.parent;
                    if (parent) {
                        if (auto render_obj = parent->GetRenderObject()) {
                            const auto& layout = render_obj->GetLayoutInfo();
                            total_change_area += layout.width * layout.height;
                        } else {
                            // 没有渲染对象，估算一个默认大小
                            total_change_area += 100.0f * 50.0f;
                        }
                    }
                }

                // 如果变化区域小于视口的 50%，使用增量更新
                if (viewport_area > 0 && total_change_area < viewport_area * 0.5f) {
                    // 标记受影响的节点需要重新布局
                    for (const auto& change : tracker.GetStructuralChanges()) {
                        auto parent = change.parent;
                        if (parent) {
                            if (auto parent_ro = parent->GetRenderObject()) {
                                parent_ro->MarkNeedsLayout(true);
                                parent_ro->MarkNeedsPaint();
                            }
                        }
                    }
                    window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
                    return;
                }
            }
        }

        // 大量变化或无法确定时，回退到全量重建
        window_->SetNeedsRepaintFor(RepaintReason::DOMMutation);
        window_->InvalidateRenderTree();
    }
}

void WindowDOMObserver::OnPseudoClassChanged(std::shared_ptr<Element> element,
                                             const std::string& pseudo_class,
                                             bool activate) {
    (void)activate;

    if (window_ && !IsInBatch(element.get())) {
        if (pseudo_class == "hover" || pseudo_class == "active" ||
            pseudo_class == "focus" || pseudo_class == "focus-visible") {
            auto style_manager = window_->GetDocument() ? window_->GetDocument()->GetStyleManager() : nullptr;
            const std::string tag_name = element->GetTagName();
            const bool has_builtin_hover = pseudo_class == "hover" && HasBuiltinHoverStyle(tag_name);
            const bool has_css_hover = pseudo_class == "hover" && style_manager && style_manager->HasHoverRules(element.get());
            if (pseudo_class == "hover" && !has_builtin_hover && !has_css_hover) {
                return;
            }

            StyleResolver resolver;
            if (style_manager) {
                resolver.SetStyleManager(style_manager);
            }

            const ComputedStyle* parent_style = nullptr;
            if (auto parent_node = element->GetParentNode()) {
                if (parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto parent_elem = std::static_pointer_cast<Element>(parent_node);
                    if (auto parent_render = parent_elem->GetRenderObject()) {
                        parent_style = &parent_render->GetComputedStyle();
                    }
                }
            }

            RestyleInteractionPseudoClassSubtree(window_, resolver, element, parent_style);
            window_->SetNeedsRepaintFor(RepaintReason::PseudoClass);
            return;

        }

        // 其他伪类的处理
        bool needs_repaint = false;
        if (pseudo_class == "active" || pseudo_class == "focus" ||
            pseudo_class == "focus-visible" || pseudo_class == "checked" ||
            pseudo_class == "disabled") {
            needs_repaint = true;
        }

        if (needs_repaint) {
            // Phase 1: 精确脏区域标记
            // 伪类变化只影响绘制，不影响渲染树结构
            if (auto render_obj = element->GetRenderObject()) {
                render_obj->MarkNeedsPaint();
                AddDirtyRectForRenderObject(window_, element.get(), render_obj.get());
            }
            window_->SetNeedsRepaintFor(RepaintReason::PseudoClass);
        }
    }
}

bool WindowDOMObserver::IsInBatch(Node* node) const {
    if (!node) return false;

    auto doc = node->GetOwnerDocument();
    if (!doc) return false;

    auto document = std::dynamic_pointer_cast<Document>(doc);
    if (!document) return false;

    return document->IsInBatch();
}

}  // namespace mbink
