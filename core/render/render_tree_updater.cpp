/**
 * @file render_tree_updater.cpp
 * @brief Incremental render tree updater implementation
 */

#include "render_tree_updater.h"
#include "render_object.h"
#include "style_resolver.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/document.h"
#include "core/layout/layout_engine.h"

namespace lightui {

RenderTreeUpdater::RenderTreeUpdater() = default;
RenderTreeUpdater::~RenderTreeUpdater() = default;

void RenderTreeUpdater::SetDocument(std::shared_ptr<Document> doc) {
    document_ = doc;
}

void RenderTreeUpdater::SetLayoutEngine(std::shared_ptr<LayoutEngine> engine) {
    layout_engine_ = engine;
}

void RenderTreeUpdater::SetRenderTreeBuilder(std::shared_ptr<RenderTreeBuilder> builder) {
    render_tree_builder_ = builder;
}

std::shared_ptr<RenderObject> RenderTreeUpdater::InsertRenderObject(
    Node* node,
    Node* parent,
    Node* reference
) {
    if (!node || !parent) {
        return nullptr;
    }

    // Get parent's render object
    auto parent_ro = parent->GetRenderObject();
    if (!parent_ro) {
        // Parent doesn't have a render object (e.g., display: none)
        // We still need to create render objects for the new subtree
        // in case the parent becomes visible later
        return nullptr;
    }

    // Create render object for the new node
    auto new_ro = CreateRenderObjectForNode(node);
    if (!new_ro) {
        return nullptr;  // display: none
    }

    // 关键修复：根据 DOM 树中的位置来确定渲染树中的插入位置
    // 当 reference 为 nullptr 时，查找 DOM 树中该节点的下一个兄弟节点
    // 这确保在 ReplaceChild 等操作后，渲染树的顺序与 DOM 树一致
    Node* effective_reference = reference;
    if (!effective_reference) {
        // 查找 DOM 树中该节点的下一个兄弟节点
        auto next_sibling = node->GetNextSibling();
        if (next_sibling) {
            effective_reference = next_sibling.get();
        }
    }

    // Find insertion position
    size_t insert_pos = FindInsertPosition(parent_ro.get(), effective_reference);

    // Insert into parent's children
    auto& children = parent_ro->GetChildrenMutable();
    if (insert_pos >= children.size()) {
        parent_ro->AppendChild(new_ro);
    } else {
        children.insert(children.begin() + insert_pos, new_ro);
        new_ro->SetParent(parent_ro);
    }

    // Establish bidirectional binding
    node->SetRenderObject(new_ro);

    // Recursively create render objects for children
    CreateRenderSubtree(node, new_ro.get());

    // Invalidate layout for ancestors
    InvalidateAncestorLayout(parent_ro.get());

    // Update layout engine - 传递正确的插入位置
    // 这确保布局树的顺序与渲染树一致
    if (auto engine = layout_engine_.lock()) {
        engine->AddElement(new_ro.get(), parent_ro.get(), insert_pos);
    }

    return new_ro;
}

void RenderTreeUpdater::RemoveRenderObject(Node* node) {
    if (!node) {
        return;
    }

    auto ro = node->GetRenderObject();
    if (!ro) {
        return;
    }

    // Get parent render object
    auto parent_ro = ro->GetParent();

    // 从布局引擎移除（布局引擎会递归移除所有子节点）
    if (auto engine = layout_engine_.lock()) {
        engine->RemoveElement(ro.get());
    }

    // Remove from parent's children
    if (parent_ro) {
        parent_ro->RemoveChild(ro);
        // Invalidate layout for ancestors
        InvalidateAncestorLayout(parent_ro.get());
    }

    // 递归清除所有子节点的 DOM-RenderObject 绑定
    // 这是关键修复：确保子节点的绑定也被清除，避免布局残留
    std::function<void(Node*)> clearBindings = [&](Node* n) {
        if (!n) return;
        n->SetRenderObject(nullptr);
        for (const auto& child : n->GetChildNodes()) {
            clearBindings(child.get());
        }
    };
    clearBindings(node);
}

void RenderTreeUpdater::MoveRenderObject(
    Node* node,
    Node* new_parent,
    Node* reference
) {
    if (!node || !new_parent) {
        return;
    }

    auto ro = node->GetRenderObject();
    if (!ro) {
        // Node doesn't have a render object, try to create one
        InsertRenderObject(node, new_parent, reference);
        return;
    }

    auto old_parent_ro = ro->GetParent();
    auto new_parent_ro = new_parent->GetRenderObject();

    if (!new_parent_ro) {
        // New parent doesn't have a render object, remove the node's RO
        RemoveRenderObject(node);
        return;
    }

    // Remove from old parent
    if (old_parent_ro) {
        old_parent_ro->RemoveChild(ro);
        InvalidateAncestorLayout(old_parent_ro.get());
    }

    // Insert into new parent
    size_t insert_pos = FindInsertPosition(new_parent_ro.get(), reference);
    auto& children = new_parent_ro->GetChildrenMutable();
    if (insert_pos >= children.size()) {
        new_parent_ro->AppendChild(ro);
    } else {
        children.insert(children.begin() + insert_pos, ro);
        ro->SetParent(new_parent_ro);
    }

    InvalidateAncestorLayout(new_parent_ro.get());

    // Update layout engine
    if (auto engine = layout_engine_.lock()) {
        if (old_parent_ro) {
            engine->RemoveElement(ro.get());
        }
        engine->AddElement(ro.get(), new_parent_ro.get());
    }
}

void RenderTreeUpdater::UpdateRenderObjectStyle(Node* node) {
    if (!node) {
        return;
    }

    auto ro = node->GetRenderObject();
    if (!ro) {
        return;
    }

    // Re-resolve style and update render object
    auto builder = render_tree_builder_.lock();
    auto doc = document_.lock();
    if (!builder || !doc) {
        return;
    }

    // Get the element (Text nodes don't have styles)
    auto element = std::dynamic_pointer_cast<Element>(node->shared_from_this());
    if (element) {
        // Re-resolve computed style
        ComputedStyle new_style = builder->GetStyleResolver().ResolveStyle(element);
        ro->SetComputedStyle(new_style);

        // Update layout engine
        if (auto engine = layout_engine_.lock()) {
            engine->UpdateStyle(ro.get(), new_style);
        }
    }
}

std::shared_ptr<RenderObject> RenderTreeUpdater::CreateRenderObjectForNode(Node* node) {
    auto builder = render_tree_builder_.lock();
    if (!builder) {
        return nullptr;
    }

    // Use the RenderTreeBuilder to create a render object
    // This handles Element vs Text nodes appropriately
    if (auto element = std::dynamic_pointer_cast<Element>(node->shared_from_this())) {
        return builder->CreateRenderObjectForElement(element.get());
    } else if (auto text = std::dynamic_pointer_cast<Text>(node->shared_from_this())) {
        return builder->CreateRenderObjectForText(text.get());
    }

    return nullptr;
}

void RenderTreeUpdater::CreateRenderSubtree(Node* node, RenderObject* parent_ro) {
    for (const auto& child : node->GetChildNodes()) {
        auto child_ro = CreateRenderObjectForNode(child.get());
        if (child_ro) {
            parent_ro->AppendChild(child_ro);
            child->SetRenderObject(child_ro);

            // Recursively create children
            CreateRenderSubtree(child.get(), child_ro.get());

            // Update layout engine
            if (auto engine = layout_engine_.lock()) {
                engine->AddElement(child_ro.get(), parent_ro);
            }
        }
    }
}

size_t RenderTreeUpdater::FindInsertPosition(RenderObject* parent_ro, Node* reference) {
    if (!reference || !parent_ro) {
        return parent_ro ? parent_ro->GetChildren().size() : 0;
    }

    // 关键修复：如果 reference 没有 RenderObject（比如 display: none），
    // 继续查找下一个有 RenderObject 的兄弟节点
    Node* current_ref = reference;
    while (current_ref) {
        auto ref_ro = current_ref->GetRenderObject();
        if (ref_ro) {
            // 找到了有 RenderObject 的节点，查找它在父节点 children 中的位置
            const auto& children = parent_ro->GetChildren();
            for (size_t i = 0; i < children.size(); ++i) {
                if (children[i].get() == ref_ro.get()) {
                    return i;
                }
            }
            // 如果在 children 中没找到，继续查找下一个兄弟
        }
        current_ref = current_ref->GetNextSibling().get();
    }

    // 没有找到有效的 reference，添加到末尾
    return parent_ro->GetChildren().size();
}

void RenderTreeUpdater::InvalidateAncestorLayout(RenderObject* obj) {
    while (obj) {
        obj->MarkNeedsLayout();
        obj = obj->GetParent().get();
    }
}

} // namespace lightui

