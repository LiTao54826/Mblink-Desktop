#ifndef LIGHTUI_LAYOUT_ENGINE_H
#define LIGHTUI_LAYOUT_ENGINE_H

#include <unordered_map>
#include <memory>

// Forward declarations
namespace lightui {
class Element;
struct ComputedStyle;
struct LayoutInfo;
}

// Taffy C API forward declarations
// Will be included in .cpp file
struct TaffyTree;
typedef struct TaffyNodeId {
    uint64_t _0;
} TaffyNodeId;

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
     * @brief Build layout tree from DOM tree
     * @param root Root element of the DOM tree
     */
    void BuildLayoutTree(Element* root);

    /**
     * @brief Compute layout for the entire tree
     * @param available_width Available width for layout
     * @param available_height Available height for layout
     */
    void ComputeLayout(float available_width, float available_height);

    /**
     * @brief Get layout information for an element
     * @param element The element to query
     * @return Layout information (position, size, etc.)
     */
    LayoutInfo GetLayoutInfo(Element* element) const;

    /**
     * @brief Update style for an element
     * @param element The element to update
     * @param style The new computed style
     */
    void UpdateStyle(Element* element, const ComputedStyle& style);

    /**
     * @brief Add a new element to the layout tree
     * @param element The element to add
     * @param parent Parent element (nullptr for root)
     */
    void AddElement(Element* element, Element* parent);

    /**
     * @brief Remove an element from the layout tree
     * @param element The element to remove
     */
    void RemoveElement(Element* element);

    /**
     * @brief Clear the entire layout tree
     */
    void Clear();

    /**
     * @brief Check if an element is in the layout tree
     * @param element The element to check
     * @return true if element is in the tree
     */
    bool HasElement(Element* element) const;

private:
    // Taffy tree instance
    TaffyTree* taffy_tree_;

    // Bidirectional mapping between DOM elements and Taffy nodes
    std::unordered_map<Element*, TaffyNodeId> element_to_node_;
    std::unordered_map<uint64_t, Element*> node_to_element_;

    // Root node
    TaffyNodeId root_node_;
    bool has_root_;

    /**
     * @brief Create a Taffy node for an element
     * @param element The element
     * @return Taffy node ID
     */
    TaffyNodeId CreateNode(Element* element);

    /**
     * @brief Apply computed style to a Taffy node
     * @param node Taffy node ID
     * @param style Computed style
     */
    void ApplyStyle(TaffyNodeId node, const ComputedStyle& style);

    /**
     * @brief Synchronize children between DOM and Taffy tree
     * @param element Parent element
     * @param node Parent Taffy node
     */
    void SyncChildren(Element* element, TaffyNodeId node);

    /**
     * @brief Recursively build layout tree from DOM subtree
     * @param element Current element
     * @param parent_node Parent Taffy node (invalid for root)
     */
    void BuildSubtree(Element* element, TaffyNodeId parent_node);

    /**
     * @brief Read layout results from Taffy and update elements
     * @param element Current element
     */
    void ReadLayoutResults(Element* element);
};

} // namespace lightui

#endif // LIGHTUI_LAYOUT_ENGINE_H
