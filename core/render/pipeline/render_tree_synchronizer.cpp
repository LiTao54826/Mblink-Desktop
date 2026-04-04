/**
 * @file render_tree_synchronizer.cpp
 * @brief 渲染树同步器实现
 */

#include "render_tree_synchronizer.h"
#include "core/render/objects/render_object.h"
#include "core/render/css/style_resolver.h"  // RenderTreeBuilder 在这里定义
#include "core/dom/observers/dirty_node_tracker.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/document.h"
#include "core/layout/layout_engine.h"
#include "core/quickjs/dom_binding_map.h"
#include <algorithm>

namespace mbink {
namespace {

bool IsNodeAttachedToDocument(Node* node) {
    while (node) {
        if (node->GetNodeType() == NodeType::DOCUMENT_NODE) {
            return true;
        }
        auto parent = node->GetParentNode();
        node = parent.get();
    }
    return false;
}

void RemoveBindingsForSubtree(const std::shared_ptr<Node>& node) {
    if (!node) {
        return;
    }

    DOMBindingMap::GetInstance().Remove(node.get());
    for (const auto& child : node->GetChildNodes()) {
        RemoveBindingsForSubtree(child);
    }
}

} // namespace

RenderTreeSynchronizer::RenderTreeSynchronizer() = default;
RenderTreeSynchronizer::~RenderTreeSynchronizer() = default;

void RenderTreeSynchronizer::SetDocument(std::shared_ptr<Document> doc) {
    document_ = doc;
}

void RenderTreeSynchronizer::SetLayoutEngine(std::shared_ptr<LayoutEngine> engine) {
    layout_engine_ = engine;
}

void RenderTreeSynchronizer::SetRenderTreeBuilder(std::shared_ptr<RenderTreeBuilder> builder) {
    render_tree_builder_ = builder;
}

bool RenderTreeSynchronizer::Synchronize(DirtyNodeTracker& tracker,
                                          std::shared_ptr<RenderObject> render_tree) {
    if (!tracker.HasPendingChanges()) {
        return false;
    }

    render_tree_ = render_tree;

    // 优化变化列表（合并冗余操作）
    tracker.Optimize();
    CleanupDetachedDOMBindings(tracker);

    // 判断是否需要子树重建
    if (NeedsSubtreeRebuild(tracker)) {
        // 收集受影响的根节点
        std::unordered_set<Node*> affected_roots;

        for (const auto& change : tracker.GetStructuralChanges()) {
            if (auto parent = change.parent.lock()) {
                affected_roots.insert(parent.get());
            }
        }

        // 重建受影响的子树
        for (Node* root : affected_roots) {
            RebuildSubtree(root);
        }
    } else {
        // 增量更新
        ProcessStructuralChanges(tracker);
    }

    // 处理样式和文本变化
    ProcessStyleChanges(tracker);
    ProcessTextChanges(tracker);

    // 清除追踪器
    tracker.Clear();

    render_tree_ = nullptr;

    return true;
}


void RenderTreeSynchronizer::CleanupDetachedDOMBindings(const DirtyNodeTracker& tracker) {
    for (const auto& change : tracker.GetStructuralChanges()) {
        std::shared_ptr<Node> root;

        switch (change.type) {
            case DirtyNodeTracker::StructuralChangeType::Removed: {
                root = change.node.lock();
                break;
            }
            case DirtyNodeTracker::StructuralChangeType::Replaced: {
                root = change.old_node.lock();
                break;
            }
            default:
                break;
        }

        if (!root || IsNodeAttachedToDocument(root.get())) {
            continue;
        }

        RemoveBindingsForSubtree(root);
    }
}

bool RenderTreeSynchronizer::NeedsSubtreeRebuild(const DirtyNodeTracker& tracker) const {
    const auto& changes = tracker.GetStructuralChanges();

    // 规则 1：计算变化区域的总面积
    // 如果变化区域超过视口面积的 50%，使用全量重建更高效
    float total_change_area = 0.0f;
    float viewport_width = 800.0f;  // 默认值
    float viewport_height = 600.0f;

    // 尝试从第一个有 RenderObject 的节点获取视口大小
    for (const auto& change : changes) {
        if (auto node = change.node.lock()) {
            if (auto render_obj = node->GetRenderObject()) {
                viewport_width = render_obj->GetViewportWidth();
                viewport_height = render_obj->GetViewportHeight();
                if (viewport_width > 0 && viewport_height > 0) break;
            }
        }
    }

    float viewport_area = viewport_width * viewport_height;

    for (const auto& change : changes) {
        // 尝试获取变化节点的渲染对象来计算面积
        if (auto node = change.node.lock()) {
            if (auto render_obj = node->GetRenderObject()) {
                const auto& layout = render_obj->GetLayoutInfo();
                total_change_area += layout.width * layout.height;
            } else {
                // 没有渲染对象，估算一个默认大小
                total_change_area += 100.0f * 50.0f;
            }
        }
        // 对于被移除的旧节点
        if (auto old_node = change.old_node.lock()) {
            if (auto render_obj = old_node->GetRenderObject()) {
                const auto& layout = render_obj->GetLayoutInfo();
                total_change_area += layout.width * layout.height;
            }
        }
    }

    // 如果变化区域超过视口的 50%，使用全量重建
    if (viewport_area > 0 && total_change_area > viewport_area * 0.5f) {
        return true;
    }

    // 规则 2：有 Replaced 操作且涉及复杂子树
    for (const auto& change : changes) {
        if (change.type == DirtyNodeTracker::StructuralChangeType::Replaced) {
            if (auto old_node = change.old_node.lock()) {
                if (old_node->GetChildNodes().size() > replaced_children_threshold_) {
                    return true;
                }
            }
        }
    }

    // 规则 3：同一父节点下有多个变化
    std::unordered_map<Node*, size_t> parent_change_count;
    for (const auto& change : changes) {
        if (auto parent = change.parent.lock()) {
            if (++parent_change_count[parent.get()] > parent_changes_threshold_) {
                return true;
            }
        }
    }

    return false;
}

void RenderTreeSynchronizer::ProcessStructuralChanges(DirtyNodeTracker& tracker) {
    for (const auto& change : tracker.GetStructuralChanges()) {
        switch (change.type) {
            case DirtyNodeTracker::StructuralChangeType::Added: {
                auto node = change.node.lock();
                auto parent = change.parent.lock();
                if (node && parent) {
                    InsertRenderObject(node.get(), parent.get(), change.index);
                }
                break;
            }

            case DirtyNodeTracker::StructuralChangeType::Removed: {
                auto node = change.node.lock();
                if (node) {
                    RemoveRenderObject(node.get());
                }
                break;
            }

            case DirtyNodeTracker::StructuralChangeType::Replaced: {
                auto old_node = change.old_node.lock();
                auto new_node = change.new_node.lock();
                auto parent = change.parent.lock();
                if (old_node && new_node && parent) {
                    ReplaceRenderObject(old_node.get(), new_node.get(), parent.get(), change.index);
                }
                break;
            }

            case DirtyNodeTracker::StructuralChangeType::Moved: {
                auto node = change.node.lock();
                auto old_parent = change.old_parent.lock();
                auto new_parent = change.parent.lock();
                if (node && new_parent) {
                    // 移动渲染对象（不清除关联）
                    MoveRenderObject(node.get(), old_parent ? old_parent.get() : nullptr,
                                    new_parent.get(), change.index);
                }
                break;
            }
        }
    }
}

void RenderTreeSynchronizer::ProcessStyleChanges(DirtyNodeTracker& tracker) {
    auto doc = document_.lock();
    if (!doc) return;

    StyleResolver resolver;
    if (doc->GetStyleManager()) {
        resolver.SetStyleManager(doc->GetStyleManager());
    }

    for (const auto& change : tracker.GetStyleChanges()) {
        auto element = change.element.lock();
        if (!element) continue;

        auto render_obj = element->GetRenderObject();
        if (!render_obj) continue;

        // 获取父元素样式用于继承
        const ComputedStyle* parent_style = nullptr;
        ComputedStyle root_parent_style;
        if (auto parent_node = element->GetParentNode()) {
            if (parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto parent_elem = std::static_pointer_cast<Element>(parent_node);
                if (auto parent_render = parent_elem->GetRenderObject()) {
                    parent_style = &parent_render->GetComputedStyle();
                }
            } else if (parent_node->GetNodeType() == NodeType::DOCUMENT_NODE &&
                       element->GetTagName() == "body") {
                auto document_element = doc->GetDocumentElement();
                if (document_element) {
                    root_parent_style = resolver.ResolveStyle(document_element, nullptr);
                    parent_style = &root_parent_style;
                }
            }
        }

        // 重新解析样式
        ComputedStyle new_style = resolver.ResolveStyle(element, parent_style);
        render_obj->SetComputedStyle(new_style);

        // 与 OnStyleChanged 保持一致：同步推进到 LayoutEngine 的样式更新路径
        // 避免仅更新 RenderObject 样式而 LayoutNode 未被正确标脏，导致增量布局漏算。
        if (auto engine = layout_engine_.lock()) {
            if (engine->HasElement(render_obj.get())) {
                engine->UpdateStyle(render_obj.get(), new_style);
            } else {
                // 当前对象不在布局树中时，回退标记最近在布局树中的祖先
                auto ancestor = render_obj->GetParent();
                while (ancestor) {
                    if (engine->HasElement(ancestor.get())) {
                        engine->MarkNeedsLayout(ancestor.get());
                        break;
                    }
                    ancestor = ancestor->GetParent();
                }
            }
        }

        // 标记需要重新布局和绘制，并向上传播到祖先
        InvalidateAncestorLayout(render_obj.get());
        render_obj->MarkNeedsPaint();
        render_obj->InvalidatePaintCache();

        // style/class/id 变化可能影响整棵后代子树的变量继承与选择器匹配
        if (change.property == "style" || change.property == "class" || change.property == "id") {
            for (const auto& child : element->GetChildNodes()) {
                if (child && child->GetNodeType() == NodeType::ELEMENT_NODE) {
                    RefreshElementSubtreeStyles(resolver, std::static_pointer_cast<Element>(child));
                }
            }
        }
    }
}

void RenderTreeSynchronizer::RefreshElementSubtreeStyles(StyleResolver& resolver,
                                                        std::shared_ptr<Element> element) {
    if (!element) {
        return;
    }

    auto render_obj = element->GetRenderObject();
    if (render_obj) {
        const ComputedStyle* parent_style = nullptr;
        ComputedStyle root_parent_style;
        if (auto parent_node = element->GetParentNode()) {
            if (parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto parent_elem = std::static_pointer_cast<Element>(parent_node);
                if (auto parent_render = parent_elem->GetRenderObject()) {
                    parent_style = &parent_render->GetComputedStyle();
                }
            } else if (parent_node->GetNodeType() == NodeType::DOCUMENT_NODE &&
                       element->GetTagName() == "body") {
                if (auto doc = document_.lock()) {
                    auto document_element = doc->GetDocumentElement();
                    if (document_element) {
                        root_parent_style = resolver.ResolveStyle(document_element, nullptr);
                        parent_style = &root_parent_style;
                    }
                }
            }
        }

        ComputedStyle new_style = resolver.ResolveStyle(element, parent_style);
        render_obj->SetComputedStyle(new_style);

        if (auto engine = layout_engine_.lock()) {
            if (engine->HasElement(render_obj.get())) {
                engine->UpdateStyle(render_obj.get(), new_style);
            }
        }

        InvalidateAncestorLayout(render_obj.get());
        render_obj->MarkNeedsPaint();
        render_obj->InvalidatePaintCache();
    }

    for (const auto& child : element->GetChildNodes()) {
        if (child && child->GetNodeType() == NodeType::ELEMENT_NODE) {
            RefreshElementSubtreeStyles(resolver, std::static_pointer_cast<Element>(child));
        }
    }
}


void RenderTreeSynchronizer::ProcessTextChanges(DirtyNodeTracker& tracker) {
    for (const auto& change : tracker.GetTextChanges()) {
        auto node = change.node.lock();
        if (!node) continue;

        // 使用增量更新系统的脏标记
        node->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
        node->SetNeedsLayout();

        auto render_obj = node->GetRenderObject();
        if (!render_obj) {
            // 文本节点无渲染对象时，回退到父布局对象，确保增量布局链路完整
            if (auto parent = node->GetParentNode()) {
                parent->SetNeedsLayout();
                render_obj = parent->GetRenderObject();
            }
        }
        if (!render_obj) continue;

        // 更新文本内容
        if (render_obj->GetType() == RenderObjectType::TEXT) {
            auto render_text = std::dynamic_pointer_cast<RenderText>(render_obj);
            if (render_text) {
                render_text->SetText(change.new_text);
            }
        }

        // 内容变化版本由同步器统一推进，避免 Observer/Synchronizer 双路径重复更新
        if (auto engine = layout_engine_.lock()) {
            engine->UpdateContentVersion(render_obj.get());
        }

        // 标记需要重新布局和绘制，并向上传播到祖先
        // 这确保滚动容器等祖先节点的 content_height_ 缓存被清除
        InvalidateAncestorLayout(render_obj.get());
        render_obj->MarkNeedsPaint();
    }
}

// 辅助函数：查找布局父级（跳过 display: contents 元素）
// 参考 Blink 的 LayoutTreeBuilderTraversal::LayoutParent()
static Node* FindLayoutParent(Node* node) {
    if (!node) return nullptr;

    Node* parent = node->GetParentNode().get();
    while (parent) {
        // 如果父节点有渲染对象，它就是布局父级
        if (parent->GetRenderObject()) {
            return parent;
        }
        // 否则继续向上查找
        parent = parent->GetParentNode().get();
    }
    return nullptr;
}

void RenderTreeSynchronizer::RebuildSubtree(Node* root) {
    if (!root) return;

    auto render_obj = root->GetRenderObject();

    // 关键修复：如果 root 没有渲染对象（可能是 display: contents 元素），
    // 向上查找布局父级，然后重建布局父级的子树
    if (!render_obj) {
        Node* layout_parent = FindLayoutParent(root);
        if (layout_parent && layout_parent != root) {
            RebuildSubtree(layout_parent);
        }
        return;
    }

    // 移除所有子渲染对象
    render_obj->RemoveAllChildren();

    // 重新创建子树
    for (const auto& child : root->GetChildNodes()) {
        CreateRenderSubtree(child.get(), render_obj.get());
    }

    // 标记需要重新布局，并向上传播到所有祖先
    // 这确保滚动容器等祖先节点的 content_height_ 缓存被清除
    InvalidateAncestorLayout(render_obj.get());
}

void RenderTreeSynchronizer::UpdateNode(Node* node) {
    if (!node) return;

    auto render_obj = node->GetRenderObject();
    if (render_obj) {
        render_obj->MarkNeedsLayout();
        render_obj->MarkNeedsPaint();
    }
}

// 辅助函数：检查节点是否是 display: contents（没有渲染对象但是元素节点）
static bool HasDisplayContentsStyle(Node* node) {
    if (!node) return false;
    if (node->GetNodeType() != NodeType::ELEMENT_NODE) return false;
    // 如果是元素节点但没有渲染对象，认为是 display: contents
    // （display: none 的元素也没有渲染对象，但在这个上下文中处理方式相同）
    return node->GetRenderObject() == nullptr;
}

// 辅助函数：获取节点的最后一个有渲染对象的后代
// 参考 Blink 的 LastChild 逻辑
static RenderObject* GetLastLayoutDescendant(Node* node) {
    if (!node) return nullptr;

    const auto& children = node->GetChildNodes();
    // 从后向前遍历子节点
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        Node* child = it->get();
        if (auto ro = child->GetRenderObject()) {
            return ro.get();
        }
        // 如果子节点是 display: contents，递归查找
        if (HasDisplayContentsStyle(child)) {
            if (auto last = GetLastLayoutDescendant(child)) {
                return last;
            }
        }
    }
    return nullptr;
}

// 辅助函数：查找前一个布局兄弟节点的渲染对象
// 参考 Blink 的 PreviousLayoutSibling
static RenderObject* FindPreviousLayoutSiblingRenderObject(Node* node) {
    if (!node) return nullptr;

    auto parent = node->GetParentNode();
    if (!parent) return nullptr;

    const auto& siblings = parent->GetChildNodes();

    // 找到当前节点在兄弟节点中的位置
    size_t node_index = siblings.size();
    for (size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i].get() == node) {
            node_index = i;
            break;
        }
    }

    // 从当前节点向前查找有渲染对象的兄弟节点
    for (size_t i = node_index; i > 0; --i) {
        Node* sibling = siblings[i - 1].get();

        // 如果兄弟节点有渲染对象，返回它
        if (auto ro = sibling->GetRenderObject()) {
            return ro.get();
        }

        // 如果兄弟节点是 display: contents，查找它的最后一个有渲染对象的后代
        if (HasDisplayContentsStyle(sibling)) {
            if (auto last = GetLastLayoutDescendant(sibling)) {
                return last;
            }
        }
    }

    // 如果在当前父节点下没找到，检查父节点是否是 display: contents
    // 如果是，继续向上查找
    if (HasDisplayContentsStyle(parent.get())) {
        return FindPreviousLayoutSiblingRenderObject(parent.get());
    }

    return nullptr;
}

// 辅助函数：根据前一个布局兄弟节点计算插入位置
static size_t CalculateInsertPositionByPreviousSibling(
    RenderObject* prev_sibling_ro, RenderObject* layout_parent_ro) {

    if (!layout_parent_ro) return 0;

    const auto& children = layout_parent_ro->GetChildren();

    if (!prev_sibling_ro) {
        // 没有前一个兄弟节点，插入到开头
        return 0;
    }

    // 找到前一个兄弟节点在父渲染对象中的位置
    for (size_t i = 0; i < children.size(); ++i) {
        if (children[i].get() == prev_sibling_ro) {
            // 插入到它后面
            return i + 1;
        }
    }

    // 如果没找到（不应该发生），插入到末尾
    return children.size();
}

std::shared_ptr<RenderObject> RenderTreeSynchronizer::InsertRenderObject(
    Node* node, Node* parent, size_t index) {

    if (!node || !parent) return nullptr;

    // 关键修复：如果节点已经有渲染对象，不要重复创建
    if (node->GetRenderObject()) {
        return node->GetRenderObject();
    }

    // 查找布局父级（跳过 display: contents 元素）
    auto parent_ro = parent->GetRenderObject();
    Node* layout_parent = parent;
    if (!parent_ro) {
        // 父元素可能是 display: contents，向上查找真正的布局父级
        layout_parent = FindLayoutParent(parent);
        if (!layout_parent) return nullptr;
        parent_ro = layout_parent->GetRenderObject();
        if (!parent_ro) return nullptr;
    }

    // 创建渲染对象
    auto render_obj = CreateRenderObjectForNode(node);
    if (!render_obj) {
        // 如果节点是 display: contents，递归处理其子元素
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            for (const auto& child : node->GetChildNodes()) {
                CreateRenderSubtree(child.get(), parent_ro.get());
            }
        }
        return nullptr;
    }

    // 计算插入位置
    // 关键修复：当父元素是 display: contents 时，使用原始的 parent 节点来计算插入位置
    // 因为 display: contents 元素的子元素在 DOM 中的顺序应该被保留
    // layout_parent 是真正的布局父级（跳过 display: contents），但它的 DOM 子节点
    // 只包含 display: contents 包装器，而不是实际的子元素
    Node* position_parent = parent;  // 使用原始的 parent 来计算位置
    size_t insert_pos = FindInsertPosition(parent_ro.get(), index, position_parent);

    // 插入到父渲染对象
    auto& children = parent_ro->GetChildrenMutable();
    if (insert_pos >= children.size()) {
        parent_ro->AppendChild(render_obj);
    } else {
        children.insert(children.begin() + insert_pos, render_obj);
        render_obj->SetParent(parent_ro);
    }

    // 递归创建子树
    for (const auto& child : node->GetChildNodes()) {
        CreateRenderSubtree(child.get(), render_obj.get());
    }

    // 使祖先布局失效
    InvalidateAncestorLayout(render_obj.get());

    return render_obj;
}

void RenderTreeSynchronizer::RemoveRenderObject(Node* node) {
    if (!node) return;

    auto render_obj = node->GetRenderObject();
    if (render_obj) {
        // 节点有渲染对象，正常移除
        auto parent_ro = render_obj->GetParent();
        if (parent_ro) {
            // 使祖先布局失效（在移除前）
            InvalidateAncestorLayout(render_obj.get());

            // 关键修复：通过 LayoutEngine 标记布局脏
            // RenderObject::MarkNeedsLayout 只设置 RenderObject 的标志
            // 但 ComputeIncrementalLayout 检查的是 LayoutNode 的标志
            // 必须通过 LayoutEngine::MarkNeedsLayout 来同步两者
            auto layout_engine = layout_engine_.lock();
            if (layout_engine) {
                layout_engine->MarkNeedsLayout(parent_ro.get());
            }

            // 标记父节点需要重绘，确保移除后的区域被正确清理
            parent_ro->MarkNeedsPaint();
            parent_ro->InvalidatePaintCache();

            // 从父节点移除
            parent_ro->RemoveChild(render_obj);
        }

        // 清除 DOM 节点与渲染对象的关联
        node->SetRenderObject(nullptr);
    } else {
        // 节点没有渲染对象，可能是 display: contents 元素
        // 需要递归移除其子元素的渲染对象
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            for (const auto& child : node->GetChildNodes()) {
                RemoveRenderObject(child.get());
            }
        }
    }
}

void RenderTreeSynchronizer::ReplaceRenderObject(
    Node* old_node, Node* new_node, Node* parent, size_t index) {

    if (!old_node || !new_node || !parent) return;

    // 查找布局父级（跳过 display: contents 元素）
    auto parent_ro = parent->GetRenderObject();
    Node* layout_parent = parent;
    if (!parent_ro) {
        layout_parent = FindLayoutParent(parent);
        if (!layout_parent) return;
        parent_ro = layout_parent->GetRenderObject();
        if (!parent_ro) return;
    }

    auto old_ro = old_node->GetRenderObject();

    // 创建新的渲染对象
    auto new_ro = CreateRenderObjectForNode(new_node);

    if (old_ro) {
        // 找到旧渲染对象在父节点中的位置
        auto& children = parent_ro->GetChildrenMutable();
        auto it = std::find(children.begin(), children.end(), old_ro);

        if (it != children.end()) {
            if (new_ro) {
                // 原子替换：直接替换，不经过中间状态
                *it = new_ro;
                new_ro->SetParent(parent_ro);
            } else {
                // 新节点是 display: none 或 display: contents
                children.erase(it);
                // 如果是 display: contents，递归处理其子元素
                if (new_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    for (const auto& child : new_node->GetChildNodes()) {
                        CreateRenderSubtree(child.get(), parent_ro.get());
                    }
                }
            }
        }

        // 清除旧节点的关联
        old_node->SetRenderObject(nullptr);
    } else if (new_ro) {
        // 旧节点没有渲染对象（可能是 display: none），直接插入新节点
        size_t insert_pos = FindInsertPosition(parent_ro.get(), index, layout_parent);
        auto& children = parent_ro->GetChildrenMutable();
        if (insert_pos >= children.size()) {
            parent_ro->AppendChild(new_ro);
        } else {
            children.insert(children.begin() + insert_pos, new_ro);
            new_ro->SetParent(parent_ro);
        }
    } else {
        // 新旧节点都没有渲染对象，但新节点可能是 display: contents
        if (new_node->GetNodeType() == NodeType::ELEMENT_NODE) {
            for (const auto& child : new_node->GetChildNodes()) {
                CreateRenderSubtree(child.get(), parent_ro.get());
            }
        }
    }

    // 递归创建新节点的子树
    if (new_ro) {
        for (const auto& child : new_node->GetChildNodes()) {
            CreateRenderSubtree(child.get(), new_ro.get());
        }
    }

    // 使祖先布局失效
    if (new_ro) {
        InvalidateAncestorLayout(new_ro.get());
    } else {
        InvalidateAncestorLayout(parent_ro.get());
    }
}

void RenderTreeSynchronizer::MoveRenderObject(Node* node, Node* old_parent,
                                               Node* new_parent, size_t index) {
    if (!node || !new_parent) return;

    auto render_obj = node->GetRenderObject();
    if (!render_obj) {
        // 节点没有渲染对象，可能是 display: contents
        // 对于 display: contents，需要移动其子元素的渲染对象
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            // 找到新的布局父级
            auto new_parent_ro = new_parent->GetRenderObject();
            Node* layout_parent = new_parent;
            if (!new_parent_ro) {
                layout_parent = FindLayoutParent(new_parent);
                if (!layout_parent) return;
                new_parent_ro = layout_parent->GetRenderObject();
                if (!new_parent_ro) return;
            }

            // 递归移动子元素
            for (const auto& child : node->GetChildNodes()) {
                MoveRenderObject(child.get(), node, new_parent, index);
            }
        }
        return;
    }

    // 获取旧的父渲染对象
    auto old_parent_ro = render_obj->GetParent();
    if (!old_parent_ro) return;

    // 获取新的父渲染对象
    auto new_parent_ro = new_parent->GetRenderObject();
    Node* layout_parent = new_parent;
    if (!new_parent_ro) {
        layout_parent = FindLayoutParent(new_parent);
        if (!layout_parent) return;
        new_parent_ro = layout_parent->GetRenderObject();
        if (!new_parent_ro) return;
    }

    // 如果父节点没变，只需要调整位置
    if (old_parent_ro.get() == new_parent_ro.get()) {
        // 从旧位置移除
        auto& children = old_parent_ro->GetChildrenMutable();
        auto it = std::find(children.begin(), children.end(), render_obj);
        if (it != children.end()) {
            children.erase(it);
        }

        // 计算新的插入位置
        RenderObject* prev_sibling_ro = FindPreviousLayoutSiblingRenderObject(node);
        size_t insert_pos = CalculateInsertPositionByPreviousSibling(prev_sibling_ro, new_parent_ro.get());

        // 插入到新位置
        if (insert_pos >= children.size()) {
            children.push_back(render_obj);
        } else {
            children.insert(children.begin() + insert_pos, render_obj);
        }
    } else {
        // 父节点变了，需要从旧父节点移除，添加到新父节点
        // 从旧父节点移除（不清除关联！）
        auto& old_children = old_parent_ro->GetChildrenMutable();
        auto it = std::find(old_children.begin(), old_children.end(), render_obj);
        if (it != old_children.end()) {
            old_children.erase(it);
        }

        // 计算新的插入位置
        RenderObject* prev_sibling_ro = FindPreviousLayoutSiblingRenderObject(node);
        size_t insert_pos = CalculateInsertPositionByPreviousSibling(prev_sibling_ro, new_parent_ro.get());

        // 添加到新父节点
        auto& new_children = new_parent_ro->GetChildrenMutable();
        if (insert_pos >= new_children.size()) {
            new_parent_ro->AppendChild(render_obj);
        } else {
            new_children.insert(new_children.begin() + insert_pos, render_obj);
            render_obj->SetParent(new_parent_ro);
        }

        // 使旧父节点布局失效
        InvalidateAncestorLayout(old_parent_ro.get());
    }

    // 使新位置的祖先布局失效
    InvalidateAncestorLayout(render_obj.get());
}

std::shared_ptr<RenderObject> RenderTreeSynchronizer::CreateRenderObjectForNode(Node* node) {
    if (!node) return nullptr;

    auto builder = render_tree_builder_.lock();
    if (!builder) return nullptr;

    std::shared_ptr<RenderObject> render_obj;

    // 根据节点类型使用不同的创建方法
    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::dynamic_pointer_cast<Element>(node->shared_from_this());
        if (element) {
            render_obj = builder->CreateRenderObjectForElement(element.get());
        }
    } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::dynamic_pointer_cast<Text>(node->shared_from_this());
        if (text) {
            render_obj = builder->CreateRenderObjectForText(text.get());
        }
    }

    if (render_obj) {
        // 建立双向关联
        node->SetRenderObject(render_obj);
        render_obj->SetNode(node->shared_from_this());
    }

    return render_obj;
}

void RenderTreeSynchronizer::CreateRenderSubtree(Node* node, RenderObject* parent_ro) {
    if (!node || !parent_ro) return;

    // 关键修复：如果节点已经有渲染对象，不要重复创建
    // 这可能发生在 BuildRenderTree 之后 Synchronize 被调用的情况
    if (node->GetRenderObject()) {
        // 节点已经有渲染对象，只需要递归处理子节点
        auto existing_ro = node->GetRenderObject();
        for (const auto& child : node->GetChildNodes()) {
            CreateRenderSubtree(child.get(), existing_ro.get());
        }
        return;
    }

    auto render_obj = CreateRenderObjectForNode(node);

    // 处理 display: contents - 不创建渲染对象，但递归处理子元素
    if (!render_obj) {
        // 检查是否是 display: contents 元素（而不是 display: none）
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(node->shared_from_this());
            if (element) {
                // 递归处理子元素，将它们添加到当前父渲染对象
                for (const auto& child : node->GetChildNodes()) {
                    CreateRenderSubtree(child.get(), parent_ro);
                }
            }
        }
        return;
    }

    parent_ro->AppendChild(render_obj);

    // 递归处理子节点
    for (const auto& child : node->GetChildNodes()) {
        CreateRenderSubtree(child.get(), render_obj.get());
    }
}

size_t RenderTreeSynchronizer::FindInsertPosition(
    RenderObject* parent_ro, size_t dom_index, Node* parent_node) {

    if (!parent_ro || !parent_node) return 0;

    const auto& dom_children = parent_node->GetChildNodes();
    const auto& ro_children = parent_ro->GetChildren();

    // 遍历 DOM 子节点，找到在 dom_index 之前有多少个有渲染对象的节点
    size_t ro_index = 0;
    for (size_t i = 0; i < dom_index && i < dom_children.size(); ++i) {
        bool has_ro = dom_children[i]->GetRenderObject() != nullptr;
        if (has_ro) {
            ++ro_index;
        }
    }

    return std::min(ro_index, ro_children.size());
}

void RenderTreeSynchronizer::InvalidateAncestorLayout(RenderObject* obj) {
    while (obj) {
        obj->MarkNeedsLayout();
        obj = obj->GetParent().get();
    }
}

} // namespace mbink
