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

    // Find insertion position
    size_t insert_pos = FindInsertPosition(parent_ro.get(), reference);

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

    // Update layout engine
    if (auto engine = layout_engine_.lock()) {
        engine->AddElement(new_ro.get(), parent_ro.get());
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

    // Remove from layout engine first
    if (auto engine = layout_engine_.lock()) {
        // Recursively remove children from layout engine
        // Use a stack-based approach to avoid deep recursion
        std::vector<RenderObject*> stack;
        stack.push_back(ro.get());
        
        while (!stack.empty()) {
            RenderObject* obj = stack.back();
            stack.pop_back();
            
            if (!obj) continue;
            
            // Add children to stack first (will be processed after parent)
            const auto& children = obj->GetChildren();
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                if (*it) {
                    stack.push_back(it->get());
                }
            }
            
            // Remove from layout engine
            engine->RemoveElement(obj);
        }
    }

    // Remove from parent's children
    if (parent_ro) {
        parent_ro->RemoveChild(ro);

        // Invalidate layout for ancestors
        InvalidateAncestorLayout(parent_ro.get());
    }

    // Clear bidirectional binding - use stack-based approach
    std::vector<Node*> nodeStack;
    nodeStack.push_back(node);
    
    while (!nodeStack.empty()) {
        Node* n = nodeStack.back();
        nodeStack.pop_back();
        
        if (!n) continue;
        
        n->SetRenderObject(nullptr);
        
        // Get children snapshot to avoid issues with concurrent modification
        const auto& children = n->GetChildNodes();
        for (const auto& child : children) {
            if (child) {
                nodeStack.push_back(child.get());
            }
        }
    }
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

    auto ref_ro = reference->GetRenderObject();
    if (!ref_ro) {
        return parent_ro->GetChildren().size();
    }

    const auto& children = parent_ro->GetChildren();
    for (size_t i = 0; i < children.size(); ++i) {
        if (children[i].get() == ref_ro.get()) {
            return i;
        }
    }

    return children.size();
}

void RenderTreeUpdater::InvalidateAncestorLayout(RenderObject* obj) {
    while (obj) {
        obj->MarkNeedsLayout();
        obj = obj->GetParent().get();
    }
}

} // namespace lightui

