/**
 * @file layout_engine.cpp
 * @brief Layout engine implementation using Taffy
 */

#include "layout_engine.h"
#include "dom/element.h"
#include "render/computed_style.h"
#include "render/layout_info.h"

// Taffy C API
// NOTE: This will be included once taffy.h is available
// #include <taffy.h>

namespace lightui {

LayoutEngine::LayoutEngine()
    : taffy_tree_(nullptr)
    , has_root_(false) {
    // TODO: Initialize Taffy tree when library is available
    // taffy_tree_ = TaffyTree_New();
}

LayoutEngine::~LayoutEngine() {
    Clear();
    // TODO: Free Taffy tree when library is available
    // if (taffy_tree_) {
    //     TaffyTree_Free(taffy_tree_);
    // }
}

void LayoutEngine::BuildLayoutTree(Element* root) {
    if (!root) {
        return;
    }

    Clear();

    // TODO: Build tree when Taffy is available
    // For now, just store the mapping
    TaffyNodeId invalid_parent;
    invalid_parent.value = 0;
    BuildSubtree(root, invalid_parent);
}

void LayoutEngine::ComputeLayout(float available_width, float available_height) {
    if (!has_root_) {
        return;
    }

    // TODO: Call Taffy layout computation
    // TaffyTree_ComputeLayout(taffy_tree_, root_node_, available_width, available_height);

    // TODO: Read layout results
    // for (auto& [element, node] : element_to_node_) {
    //     ReadLayoutResults(element);
    // }
}

LayoutInfo LayoutEngine::GetLayoutInfo(Element* element) const {
    LayoutInfo info;

    // TODO: Get layout from Taffy
    // auto it = element_to_node_.find(element);
    // if (it != element_to_node_.end()) {
    //     TaffyLayout layout = TaffyTree_GetLayout(taffy_tree_, it->second);
    //     info.x = layout.x;
    //     info.y = layout.y;
    //     info.width = layout.width;
    //     info.height = layout.height;
    // }

    return info;
}

void LayoutEngine::UpdateStyle(Element* element, const ComputedStyle& style) {
    auto it = element_to_node_.find(element);
    if (it != element_to_node_.end()) {
        ApplyStyle(it->second, style);
    }
}

void LayoutEngine::AddElement(Element* element, Element* parent) {
    if (!element || HasElement(element)) {
        return;
    }

    TaffyNodeId parent_node;
    parent_node.value = 0;

    if (parent) {
        auto it = element_to_node_.find(parent);
        if (it != element_to_node_.end()) {
            parent_node = it->second;
        }
    }

    TaffyNodeId node = CreateNode(element);
    element_to_node_[element] = node;
    node_to_element_[node.value] = element;

    // TODO: Add to parent in Taffy tree
    // if (parent_node.value != 0) {
    //     TaffyTree_AddChild(taffy_tree_, parent_node, node);
    // }

    if (!has_root_ && !parent) {
        root_node_ = node;
        has_root_ = true;
    }
}

void LayoutEngine::RemoveElement(Element* element) {
    auto it = element_to_node_.find(element);
    if (it == element_to_node_.end()) {
        return;
    }

    TaffyNodeId node = it->second;

    // TODO: Remove from Taffy tree
    // TaffyTree_RemoveNode(taffy_tree_, node);

    node_to_element_.erase(node.value);
    element_to_node_.erase(it);

    if (has_root_ && root_node_.value == node.value) {
        has_root_ = false;
    }
}

void LayoutEngine::Clear() {
    element_to_node_.clear();
    node_to_element_.clear();
    has_root_ = false;

    // TODO: Clear Taffy tree
    // if (taffy_tree_) {
    //     TaffyTree_Clear(taffy_tree_);
    // }
}

bool LayoutEngine::HasElement(Element* element) const {
    return element_to_node_.find(element) != element_to_node_.end();
}

TaffyNodeId LayoutEngine::CreateNode(Element* element) {
    TaffyNodeId node;
    node.value = 0;

    // TODO: Create Taffy node
    // TaffyStyle* style = TaffyStyle_New();
    // node = TaffyTree_NewNode(taffy_tree_, style);
    // TaffyStyle_Free(style);

    return node;
}

void LayoutEngine::ApplyStyle(TaffyNodeId node, const ComputedStyle& style) {
    // TODO: Apply style to Taffy node
    // This will map ComputedStyle properties to Taffy API calls
    // Example:
    // TaffyStyle* taffy_style = TaffyTree_GetStyle(taffy_tree_, node);
    // TaffyStyle_SetDisplay(taffy_style, MapDisplay(style.display));
    // TaffyStyle_SetFlexDirection(taffy_style, MapFlexDirection(style.flex_direction));
    // etc.
}

void LayoutEngine::SyncChildren(Element* element, TaffyNodeId node) {
    // TODO: Sync children
    // Get children from element
    // For each child:
    //   - Create or get Taffy node
    //   - Add as child to parent node
    //   - Recursively sync
}

void LayoutEngine::BuildSubtree(Element* element, TaffyNodeId parent_node) {
    if (!element) {
        return;
    }

    // Create node for this element
    TaffyNodeId node = CreateNode(element);
    element_to_node_[element] = node;
    node_to_element_[node.value] = element;

    // Set as root if no parent
    if (parent_node.value == 0 && !has_root_) {
        root_node_ = node;
        has_root_ = true;
    }

    // TODO: Add to parent in Taffy tree
    // if (parent_node.value != 0) {
    //     TaffyTree_AddChild(taffy_tree_, parent_node, node);
    // }

    // TODO: Recursively build children
    // for (auto* child : element->GetChildren()) {
    //     BuildSubtree(child, node);
    // }
}

void LayoutEngine::ReadLayoutResults(Element* element) {
    // TODO: Read layout from Taffy and update element
    // TaffyLayout layout = TaffyTree_GetLayout(taffy_tree_, node);
    // LayoutInfo info;
    // info.x = layout.x;
    // info.y = layout.y;
    // info.width = layout.width;
    // info.height = layout.height;
    // element->SetLayoutInfo(info);
}

} // namespace lightui
