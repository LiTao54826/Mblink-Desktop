/**
 * @file render_tree_updater.h
 * @brief Incremental render tree updater
 *
 * This class provides incremental updates to the render tree when DOM nodes
 * are added, removed, or moved, avoiding full render tree rebuilds.
 */

#ifndef LIGHTUI_RENDER_TREE_UPDATER_H
#define LIGHTUI_RENDER_TREE_UPDATER_H

#include <memory>
#include <functional>

namespace lightui {

// Forward declarations
class Node;
class Element;
class Text;
class RenderObject;
class Document;
class RenderTreeBuilder;
class LayoutEngine;

/**
 * @brief Incremental render tree updater
 *
 * Provides methods to incrementally update the render tree when DOM changes
 * occur, rather than rebuilding the entire tree.
 */
class RenderTreeUpdater {
public:
    RenderTreeUpdater();
    ~RenderTreeUpdater();

    /**
     * @brief Set the document for style resolution
     * @param doc The document
     */
    void SetDocument(std::shared_ptr<Document> doc);

    /**
     * @brief Set the layout engine for layout tree updates
     * @param engine The layout engine
     */
    void SetLayoutEngine(std::shared_ptr<LayoutEngine> engine);

    /**
     * @brief Set the render tree builder for creating render objects
     * @param builder The render tree builder
     */
    void SetRenderTreeBuilder(std::shared_ptr<RenderTreeBuilder> builder);

    /**
     * @brief Insert a render object for a newly added DOM node
     * @param node The new DOM node
     * @param parent The parent DOM node
     * @param reference The reference node (insert before this, or at end if nullptr)
     * @return The created render object, or nullptr if display: none
     */
    std::shared_ptr<RenderObject> InsertRenderObject(
        Node* node,
        Node* parent,
        Node* reference = nullptr
    );

    /**
     * @brief Remove a render object when a DOM node is removed
     * @param node The DOM node being removed
     */
    void RemoveRenderObject(Node* node);

    /**
     * @brief Move a render object when a DOM node is moved
     * @param node The DOM node being moved
     * @param new_parent The new parent DOM node
     * @param reference The reference node (insert before this, or at end if nullptr)
     */
    void MoveRenderObject(
        Node* node,
        Node* new_parent,
        Node* reference = nullptr
    );

    /**
     * @brief Update a render object's style when attributes/styles change
     * @param node The DOM node whose style changed
     */
    void UpdateRenderObjectStyle(Node* node);

private:
    /**
     * @brief Create a render object for a single node
     * @param node The DOM node
     * @return The created render object, or nullptr if display: none
     */
    std::shared_ptr<RenderObject> CreateRenderObjectForNode(Node* node);

    /**
     * @brief Recursively create render objects for a subtree
     * @param node Root of the subtree
     * @param parent_ro Parent render object
     */
    void CreateRenderSubtree(Node* node, RenderObject* parent_ro);

    /**
     * @brief Find the render object position in parent's children
     * @param parent_ro Parent render object
     * @param reference Reference DOM node
     * @return Index to insert at
     */
    size_t FindInsertPosition(RenderObject* parent_ro, Node* reference);

    /**
     * @brief Invalidate layout for an ancestor chain
     * @param obj The starting render object
     */
    void InvalidateAncestorLayout(RenderObject* obj);

    std::weak_ptr<Document> document_;
    std::weak_ptr<LayoutEngine> layout_engine_;
    std::weak_ptr<RenderTreeBuilder> render_tree_builder_;
};

} // namespace lightui

#endif // LIGHTUI_RENDER_TREE_UPDATER_H

