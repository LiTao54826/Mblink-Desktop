#ifndef LIGHTUI_LAYOUT_ENGINE_H
#define LIGHTUI_LAYOUT_ENGINE_H

#include <unordered_map>
#include <memory>
#include <cstdint>

// Taffy C API
extern "C" {
#include "taffy.h"
}

// Forward declarations
namespace lightui {
class Element;
class RenderObject;
struct ComputedStyle;
struct LayoutInfo;
}

namespace lightui {

/**
 * @brief Layout engine that uses Taffy for CSS layout computation
 *
 * This class manages the Taffy layout tree and synchronizes it with the DOM tree.
 * It supports Block, Flexbox, and CSS Grid layouts according to W3C specifications.
 */
class LayoutEngine {
public:
    LayoutEngine();
    ~LayoutEngine();

    // Prevent copying
    LayoutEngine(const LayoutEngine&) = delete;
    LayoutEngine& operator=(const LayoutEngine&) = delete;

    /**
     * @brief Build layout tree from render tree
     * @param root Root of the render tree
     */
    void BuildLayoutTree(std::shared_ptr<RenderObject> root);

    /**
     * @brief Compute layout for the entire tree
     * @param available_width Available width for layout
     * @param available_height Available height for layout
     */
    void ComputeLayout(float available_width, float available_height);

    /**
     * @brief Get layout information and update render tree
     * @param root Root of the render tree
     */
    void GetLayoutInfo(std::shared_ptr<RenderObject> root);

    /**
     * @brief Update style for a render object
     * @param render_obj The render object to update
     * @param style The new computed style
     */
    void UpdateStyle(RenderObject* render_obj, const ComputedStyle& style);

    /**
     * @brief Add a new render object to the layout tree
     * @param render_obj The render object to add
     * @param parent Parent render object
     */
    void AddElement(RenderObject* render_obj, RenderObject* parent);

    /**
     * @brief Remove a render object from the layout tree
     * @param render_obj The render object to remove
     */
    void RemoveElement(RenderObject* render_obj);

    /**
     * @brief Clear the entire layout tree
     */
    void Clear();

    /**
     * @brief Check if a render object is in the layout tree
     * @param render_obj The render object to check
     * @return true if render object is in the tree
     */
    bool HasElement(RenderObject* render_obj) const;

private:
    // Taffy tree instance
    TaffyTree* taffy_tree_;

    // Bidirectional mapping between render objects and Taffy nodes
    std::unordered_map<RenderObject*, TaffyNodeId> element_to_node_;
    std::unordered_map<uint64_t, RenderObject*> node_to_element_;

    // Root node
    TaffyNodeId root_node_;
    bool has_root_;

    /**
     * @brief Create a Taffy node for a render object
     * @param render_obj The render object
     * @return Taffy node ID
     */
    TaffyNodeId CreateNode(RenderObject* render_obj);

    /**
     * @brief Apply computed style to a Taffy node
     * @param node Taffy node ID
     * @param style Computed style
     */
    void ApplyStyle(TaffyNodeId node, const ComputedStyle& style);

    /**
     * @brief Synchronize children between render tree and Taffy tree
     * @param render_obj Parent render object
     * @param node Parent Taffy node
     */
    void SyncChildren(RenderObject* render_obj, TaffyNodeId node);

    /**
     * @brief Recursively build layout tree from render subtree
     * @param render_obj Current render object
     * @param parent_node Parent Taffy node (invalid for root)
     */
    void BuildSubtree(RenderObject* render_obj, TaffyNodeId parent_node);

    /**
     * @brief Read layout results from Taffy and update render objects
     * @param render_obj Current render object
     */
    void ReadLayoutResults(RenderObject* render_obj);
};

} // namespace lightui

#endif // LIGHTUI_LAYOUT_ENGINE_H
