/**
 * @file window_dom_observer.cpp
 * @brief Window 的 DOM 观察者实现
 * 
 * 从 window.cpp 提取的 DOM 观察者类实现。
 * 监听 DOM 变化并触发窗口重绘。
 */

// 性能优化：默认关闭调试日志
#ifdef LIGHTUI_DEBUG_RENDERING
    #define DEBUG_LOG(msg) std::cout << msg << std::endl
#else
    #define DEBUG_LOG(msg) ((void)0)
#endif

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
#include <iostream>
#include <cstdlib>
#include <vector>

namespace lightui {

WindowDOMObserver::WindowDOMObserver(Window* window) : window_(window) {}

void WindowDOMObserver::OnNodeAdded(Node* node, Node* parent) {
    DEBUG_LOG("[WindowDOMObserver::OnNodeAdded] node=" << node
              << ", parent=" << parent
              << ", IsInBatch=" << (node ? IsInBatch(node) : false));
    
    // 调试日志
    static bool debug_select = std::getenv("LIGHTUI_DEBUG_SELECT") != nullptr;
    if (debug_select && node) {
        std::string tag = "unknown";
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node->shared_from_this());
            if (elem) tag = elem->GetTagName();
        } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
            tag = "text";
        }
        std::cout << "[OnNodeAdded] tag=" << tag << " IsInBatch=" << IsInBatch(node) << std::endl;
    }
    
    // 处理 <style> 元素的添加：触发样式解析
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto elem = std::dynamic_pointer_cast<Element>(node->shared_from_this());
        if (elem && elem->GetTagName() == "style") {
            // 获取 Document 的 StyleManager 并解析样式
            if (auto doc = node->GetOwnerDocument()) {
                if (auto style_manager = doc->GetStyleManager()) {
                    style_manager->ParseStyleElement(elem.get());
                }
            }
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
                    // 脱离文档流元素：使用增量布局管理器处理
                    if (auto* manager = window_->GetIncrementalLayoutManager()) {
                        if (manager->AddOutOfFlowElement(elem.get(), parent)) {
                            // 关键修复：不在这里请求层创建！
                            // 层创建应该在布局完成后由 DoLayerTreeBuild 自动处理
                            // 这样可以确保层边界使用正确的布局信息
                            //
                            // 之前的问题：
                            // 1. AddOutOfFlowElement 只标记需要布局，不执行布局
                            // 2. RequestAddLayer 在布局前就被调用
                            // 3. 层创建时使用的是旧的/空的布局信息
                            // 4. 导致第一帧元素出现在错误位置闪烁
                            //
                            // 修复后：
                            // 1. 这里只添加到渲染树并标记需要布局
                            // 2. 布局在 Window::EnsureRenderTree 中执行
                            // 3. DoLayerTreeBuild 会检测需要层的元素并创建
                            // 4. 层边界使用正确的布局信息

                            window_->SetNeedsRepaint();
                            return;
                        }
                    }
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
                    
                    window_->SetNeedsRepaint();
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
        window_->SetNeedsRepaint();
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
                    // 脱离文档流元素：使用增量布局管理器处理
                    if (auto* manager = window_->GetIncrementalLayoutManager()) {
                        if (manager->RemoveOutOfFlowElement(elem.get())) {
                            // 成功处理布局，现在通知层树管理器移除层
                            if (auto* pipeline = window_->GetRenderPipeline()) {
                                if (auto* layer_manager = pipeline->GetLayerTreeManager()) {
                                    // 请求增量移除层
                                    if (style.position == "fixed") {
                                        layer_manager->RequestRemoveLayer(elem->GetRenderObject().get());
                                    }
                                }
                            }
                            window_->SetNeedsRepaint();
                            return;
                        }
                    }
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
                    
                    window_->SetNeedsRepaint();
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
                        std::cout << "[OnNodeRemoved] Parent is fixed, triggering render tree rebuild" << std::endl;
                        window_->InvalidateRenderTree();
                        window_->SetNeedsRepaint();
                        return;
                    }
                }
            }
        }

        // 2. 增量更新：标记需要重绘
        // DirtyNodeTracker 已经在 Node::RemoveChild 中记录了变化
        // RenderTreeSynchronizer 会在渲染时根据变化区域大小决定是增量更新还是全量重建
        window_->SetNeedsRepaint();
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
            SkRect bounds = render_obj->GetBoundingRect();
            if (!bounds.isEmpty()) {
                element->SetDirtyRect(bounds);
                window_->AddDirtyRect(bounds);
            }
            
            // 关键修复：当 style 或 class 属性变化时，需要重新解析样式
            // style: 确保 transform 等属性的动态更新能正确生效
            // class: 确保 CSS 类选择器匹配的样式能正确应用（如 .cm-activeLine）
            if (name == "style" || name == "class") {
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
                
                render_obj->SetComputedStyle(new_style);
                render_obj->InvalidatePaintCache();
                
                // 关键修复：同步更新布局引擎中的样式
                // UpdateStyle 内部会检查布局相关属性是否变化
                // 只有布局属性变化时才会标记 needs_layout
                if (window_->GetLayoutEngine()) {
                    window_->GetLayoutEngine()->UpdateStyle(render_obj.get(), new_style);
                }
            }
        }
        window_->SetNeedsRepaint();
        // 注意：属性变化不调用 InvalidateRenderTree()，保持渲染树结构
    }
}

void WindowDOMObserver::OnStyleChanged(Element* element,
                                       const std::string& property,
                                       const std::string& old_value,
                                       const std::string& new_value) {
    if (window_ && !IsInBatch(element)) {
        // 调试日志
        if (property == "line-height") {
            std::cout << "[OnStyleChanged] property=" << property 
                      << " value=" << new_value 
                      << " hasRenderObj=" << (element->GetRenderObject() != nullptr)
                      << std::endl;
        }
        
        // 特殊处理: display 属性变化影响元素的 RenderObject 存在性
        // display: none 的元素没有 RenderObject，变为 block/flex 等需要创建
        // 反之亦然，需要删除 RenderObject
        if (property == "display") {
            bool was_none = (old_value == "none" || old_value.empty());
            bool is_none = (new_value == "none");
            if (was_none != is_none) {
                // 可见性发生变化，需要重建渲染树
                window_->InvalidateRenderTree();
                window_->SetNeedsRepaint();
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
            }
            
            render_obj->InvalidatePaintCache();

            // 记录脏矩形
            SkRect bounds = render_obj->GetBoundingRect();
            if (!bounds.isEmpty()) {
                element->SetDirtyRect(bounds);
                window_->AddDirtyRect(bounds);
            }
        }
        window_->SetNeedsRepaint();
    }
}

void WindowDOMObserver::OnTextChanged(Node* node,
                                      const std::string& old_text,
                                      const std::string& new_text) {
    if (window_ && !IsInBatch(node)) {
        // Phase 4: 文本内容变化的增量更新优化
        // 使用增量更新系统的脏标记，避免全量重建渲染树
        
        // 1. 标记节点需要样式重算（文本变化可能影响样式）
        node->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
        
        // 2. 标记节点需要布局（文本尺寸可能改变）
        node->SetNeedsLayout();
        
        // 3. 先尝试获取节点自身的 RenderObject
        auto render_obj = node->GetRenderObject();

        // 如果节点没有 RenderObject，尝试获取父节点的
        if (!render_obj) {
            if (auto parent = node->GetParentNode()) {
                render_obj = parent->GetRenderObject();
                // 也标记父节点需要布局
                parent->SetNeedsLayout();
            }
        }

        if (render_obj) {
            // 如果是 RenderText，直接更新文本内容
            if (render_obj->GetType() == RenderObjectType::TEXT) {
                auto render_text = static_cast<RenderText*>(render_obj.get());
                // 直接设置新文本（SyncRenderTree 会处理规范化）
                render_text->SetText(new_text);
            }

            // 文本内容变化需要重新布局（尺寸可能改变）
            render_obj->MarkNeedsLayout();
            render_obj->MarkNeedsPaint();

            // Update content version for incremental layout optimization
            if (auto* engine = window_->GetLayoutEngine()) {
                engine->UpdateContentVersion(render_obj.get());
            }

            // 4. 记录脏矩形区域（只重绘文本节点的边界）
            SkRect bounds = render_obj->GetBoundingRect();
            if (!bounds.isEmpty()) {
                node->SetDirtyRect(bounds);
                window_->AddDirtyRect(bounds);
            }
        }
        
        // 5. 标记需要重绘，但不调用 InvalidateRenderTree()
        // 这样可以保持渲染树结构，只进行增量更新
        window_->SetNeedsRepaint();
    }
}


void WindowDOMObserver::OnSubtreeModified(Node* root) {
    if (window_) {
        // 检查是否是批量更新结束后的通知
        // 如果是，应该根据 DirtyNodeTracker 中的变化类型决定是否需要全量重建
        auto doc = window_->GetDocument();
        if (doc) {
            const auto& tracker = doc->GetDirtyTracker();
            
            // 调试日志
            static bool debug_render = std::getenv("LIGHTUI_DEBUG_RENDER") != nullptr;
            if (debug_render) {
                std::cout << "[OnSubtreeModified] text_changes=" << tracker.GetTextChangeCount()
                          << ", structural_changes=" << tracker.GetStructuralChangeCount()
                          << ", style_changes=" << tracker.GetStyleChangeCount() << std::endl;
            }
            
            // 如果只有文本变化，不需要全量重建
            // 文本变化已经通过 DirtyNodeTracker 记录，会在渲染时处理
            if (tracker.GetTextChangeCount() > 0 && 
                tracker.GetStructuralChangeCount() == 0) {
                // 只有文本变化，走增量更新路径
                if (debug_render) {
                    std::cout << "[OnSubtreeModified] Text-only changes, using incremental update" << std::endl;
                }
                // 处理 DirtyNodeTracker 中记录的文本变化
                for (const auto& change : tracker.GetTextChanges()) {
                    auto node = change.node.lock();
                    if (!node) continue;
                    
                    // 标记节点需要样式重算和布局
                    node->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
                    node->SetNeedsLayout();
                    
                    // 获取 RenderObject 并更新
                    auto render_obj = node->GetRenderObject();
                    if (!render_obj) {
                        if (auto parent = node->GetParentNode()) {
                            render_obj = parent->GetRenderObject();
                            parent->SetNeedsLayout();
                        }
                    }
                    
                    if (render_obj) {
                        if (render_obj->GetType() == RenderObjectType::TEXT) {
                            auto render_text = static_cast<RenderText*>(render_obj.get());
                            render_text->SetText(change.new_text);
                        }
                        render_obj->MarkNeedsLayout();
                        render_obj->MarkNeedsPaint();
                        
                        // 记录脏矩形
                        SkRect bounds = render_obj->GetBoundingRect();
                        if (!bounds.isEmpty()) {
                            node->SetDirtyRect(bounds);
                            window_->AddDirtyRect(bounds);
                        }
                    }
                }
                
                window_->SetNeedsRepaint();
                // 不调用 InvalidateRenderTree()
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
                    auto parent = change.parent.lock();
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
                    if (debug_render) {
                        std::cout << "[OnSubtreeModified] Change area " << total_change_area 
                                  << " < 50% viewport " << viewport_area 
                                  << ", using incremental update" << std::endl;
                    }
                    // 标记受影响的节点需要重新布局
                    for (const auto& change : tracker.GetStructuralChanges()) {
                        auto parent = change.parent.lock();
                        if (parent) {
                            if (auto parent_ro = parent->GetRenderObject()) {
                                parent_ro->MarkNeedsLayout(true);
                                parent_ro->MarkNeedsPaint();
                            }
                        }
                    }
                    window_->SetNeedsRepaint();
                    return;
                }
                
                if (debug_render) {
                    std::cout << "[OnSubtreeModified] Change area " << total_change_area 
                              << " >= 50% viewport " << viewport_area 
                              << ", falling back to full rebuild" << std::endl;
                }
            }
        }
        
        // 大量变化或无法确定时，回退到全量重建
        window_->SetNeedsRepaint();
        window_->InvalidateRenderTree();
    }
}

void WindowDOMObserver::OnPseudoClassChanged(std::shared_ptr<Element> element,
                                             const std::string& pseudo_class,
                                             bool activate) {
    if (window_ && !IsInBatch(element.get())) {
        // hover 伪类变化需要重新解析样式（可能有 :hover 选择器定义的动画）
        if (pseudo_class == "hover") {
            // 性能优化：只有当元素有 :hover 相关的 CSS 规则时才重新解析样式
            auto style_manager = window_->GetDocument() ? window_->GetDocument()->GetStyleManager() : nullptr;
            if (!style_manager || !style_manager->HasHoverRules(element.get())) {
                // 没有 hover 规则，不需要重新解析样式
                return;
            }
            
            if (auto render_obj = element->GetRenderObject()) {
                // 重新解析样式以获取 :hover 伪类的样式（包括动画）
                StyleResolver resolver;
                resolver.SetStyleManager(style_manager);
                
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
                
                auto new_style = resolver.ResolveStyle(element, parent_style);
                
                render_obj->SetComputedStyle(new_style);
                render_obj->MarkNeedsPaint();
                render_obj->InvalidatePaintCache();

                // 记录脏矩形
                SkRect bounds = render_obj->GetViewportBoundingRect();
                if (!bounds.isEmpty()) {
                    element->SetDirtyRect(bounds);
                    window_->AddDirtyRect(bounds);
                }
            }
            window_->SetNeedsRepaint();
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

                // 记录脏矩形
                SkRect bounds = render_obj->GetBoundingRect();
                if (!bounds.isEmpty()) {
                    element->SetDirtyRect(bounds);
                    window_->AddDirtyRect(bounds);
                }
            }
            window_->SetNeedsRepaint();
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

}  // namespace lightui
