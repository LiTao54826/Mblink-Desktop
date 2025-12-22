/**
 * @file native_layout_engine.h
 * @brief Native layout engine using translated Taffy algorithms
 *
 * This is a pure C++ implementation of CSS layout algorithms,
 * translated from Taffy (Rust). It replaces the Taffy FFI-based
 * layout engine with native code.
 */

#ifndef LIGHTUI_NATIVE_LAYOUT_ENGINE_H
#define LIGHTUI_NATIVE_LAYOUT_ENGINE_H

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#include "types/layout.h"
#include "types/style.h"
#include "types/cache.h"
#include "types/geometry.h"
#include "types/traits.h"
#include "flex_layout.h"
#include "grid/grid.h"
#include "ifc/ifc_layout.h"

// Forward declarations
namespace lightui {
class Element;
class RenderObject;
struct ComputedStyle;
struct LayoutInfo;
}

namespace lightui {

// Forward declarations for interfaces
class LayoutFlexboxContainer;
class LayoutGridContainer;
struct GridContainerStyle;
struct GridItemStyle;

/**
 * @brief Native layout engine using translated Taffy algorithms
 *
 * This class implements CSS layout algorithms (Block, Flexbox, Grid)
 * in pure C++, translated from Taffy's Rust implementation.
 *
 * Implements LayoutTree interfaces to integrate with translated Taffy algorithms.
 */
class NativeLayoutEngine : public LayoutBlockContainer {
public:
    NativeLayoutEngine();
    ~NativeLayoutEngine();

    // Prevent copying
    NativeLayoutEngine(const NativeLayoutEngine&) = delete;
    NativeLayoutEngine& operator=(const NativeLayoutEngine&) = delete;

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
     * @brief Compute incremental layout for dirty subtrees only
     * @param available_width Available width for layout
     * @param available_height Available height for layout
     * @return true if any layout was performed
     */
    bool ComputeIncrementalLayout(float available_width, float available_height);

    /**
     * @brief Mark a render object as needing layout
     * @param render_obj The render object to mark
     */
    void MarkNeedsLayout(RenderObject* render_obj);

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
     * @brief Update content version for a render object
     * 
     * Called when content changes (text, children, or layout-affecting style).
     * Generates a new version number and propagates dirty marks to ancestors.
     * 
     * @param render_obj The render object whose content changed
     * 
     * **Feature: incremental-layout-optimization**
     * **Validates: Requirements 1.1, 1.2, 1.3**
     */
    void UpdateContentVersion(RenderObject* render_obj);

    /**
     * @brief Add a new render object to the layout tree
     * @param render_obj The render object to add
     * @param parent Parent render object
     * @param insert_index Index to insert at in parent's children list (SIZE_MAX = append to end)
     */
    void AddElement(RenderObject* render_obj, RenderObject* parent, size_t insert_index = SIZE_MAX);

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

    //--------------------------------------------------------------------------
    // LayoutTree interface implementation
    //--------------------------------------------------------------------------

    size_t ChildCount(NodeId node) const override;
    NodeId GetChildId(NodeId node, size_t index) const override;
    Cache& GetCache(NodeId node) override;
    void SetUnroundedLayout(NodeId node, const Layout& layout) override;
    const Layout& GetLayout(NodeId node) const override;

    LayoutOutput PerformChildLayout(
        NodeId node,
        Size<std::optional<float>> known_dimensions,
        Size<std::optional<float>> parent_size,
        Size<AvailableSpace> available_space,
        SizingMode sizing_mode,
        Line<bool> vertical_margins_are_collapsible
    ) override;

    Size<float> MeasureChildSize(
        NodeId node,
        Size<std::optional<float>> known_dimensions,
        Size<std::optional<float>> parent_size,
        Size<AvailableSpace> available_space,
        SizingMode sizing_mode
    ) override;

    //--------------------------------------------------------------------------
    // LayoutBlockContainer interface implementation
    //--------------------------------------------------------------------------

    /// Get the unified Style for a container node
    const Style& GetContainerStyle(NodeId node) const override;
    
    /// Get the unified Style for a child node
    const Style& GetChildStyle(NodeId node) const override;
    
    bool IsTextNode(NodeId node) const override;

    //--------------------------------------------------------------------------
    // Grid style getters (for Grid-specific data)
    //--------------------------------------------------------------------------

    const GridContainerStyle& GetGridContainerStyle(NodeId node) const;
    const GridItemStyle& GetGridItemStyle(NodeId node) const;

    //--------------------------------------------------------------------------
    // Unified Style getters
    //--------------------------------------------------------------------------

    /// Get the unified Style for a node
    const Style& GetStyle(NodeId node) const;

private:
    /**
     * @brief Layout scope enumeration - determines how layout changes propagate
     * 
     * Used to optimize incremental layout by limiting the scope of recalculation
     * based on the container's characteristics.
     */
    enum class LayoutScope {
        SELF_ONLY,   ///< Only affects self (fixed-size containers, absolute/fixed positioned)
        SUBTREE,     ///< Affects subtree (auto-size containers)
        SIBLINGS,    ///< Affects siblings (flex/grid children)
        ANCESTORS    ///< Affects ancestors (size changes propagate upward)
    };

    /**
     * @brief Internal node structure for the layout tree
     */
    struct LayoutNode {
        NodeId id = 0;
        RenderObject* render_obj = nullptr;
        NodeId parent = 0;
        std::vector<NodeId> children;

        // Style (converted from ComputedStyle) - 唯一样式存储
        Style style;

        // Grid 特有数据（仅 Grid 容器/项目需要）
        // 这些属性不在统一的 Style 结构中，需要单独存储
        GridContainerStyle grid_container_style;
        GridItemStyle grid_item_style;

        // Layout cache
        Cache cache;

        // Layout output
        LayoutOutput output;

        // Final layout (unrounded)
        Layout layout;

        // Position (relative to parent)
        float x = 0.0f;
        float y = 0.0f;

        //----------------------------------------------------------------------
        // Content Version and Layout Scope (for incremental layout optimization)
        //----------------------------------------------------------------------

        /// Content version number - incremented when content changes (text, children, layout-affecting style)
        /// Used for automatic cache invalidation without manual cache clearing
        uint64_t content_version = 0;

        /// Layout scope - determines how layout changes propagate through the tree
        /// Used to optimize incremental layout by limiting recalculation scope
        LayoutScope layout_scope = LayoutScope::SUBTREE;

        /// Last measured width (used to detect size changes for incremental layout)
        float last_measured_width = 0.0f;

        /// Last measured height (used to detect size changes for incremental layout)
        float last_measured_height = 0.0f;

        // Flags
        bool is_ifc_container = false;
        bool is_table_container = false;  // TABLE 元素标记
        bool is_anonymous_block = false;  // 匿名块盒标记（用于包裹混合内容中的内联元素）
        bool needs_layout = true;
        
        // 匿名块盒包含的内联级子元素（render_obj指针列表）
        std::vector<RenderObject*> anonymous_inline_children;

        //----------------------------------------------------------------------
        // IFC Layout Results (for integrated IFC layout)
        //----------------------------------------------------------------------

        /// Inline boxes collected during IFC layout
        std::vector<InlineBox> ifc_inline_boxes;

        /// Line boxes generated by IFC layout
        std::vector<LineBox> ifc_line_boxes;

        /// IFC measurement result (cached for reuse)
        IFCMeasureResult ifc_measure_result;
    };

    // Default layout for error returns
    mutable Layout default_layout_;

    // Node storage
    std::unordered_map<NodeId, LayoutNode> nodes_;
    std::unordered_map<RenderObject*, NodeId> render_to_node_;
    NodeId next_node_id_ = 1;
    NodeId root_node_ = 0;
    
    // Cached root render object
    std::weak_ptr<RenderObject> cached_root_;
    
    // IFC layout instance
    IFCLayout ifc_layout_;
    
    // Cached scrollbar state for incremental layout optimization
    // This avoids the two-pass layout on every incremental update
    bool last_needs_v_scrollbar_ = false;
    float last_effective_width_ = 0.0f;

    /**
     * @brief Create a new node
     * @param render_obj The render object
     * @return Node ID
     */
    NodeId CreateNode(RenderObject* render_obj);

    /**
     * @brief Convert ComputedStyle to internal Style
     * @param computed The computed style
     * @return Internal style
     */
    Style ConvertStyle(const ComputedStyle& computed);

    /**
     * @brief Recursively build layout tree from render subtree
     * @param render_obj Current render object
     * @param parent_id Parent node ID (0 for root)
     */
    void BuildSubtree(RenderObject* render_obj, NodeId parent_id);

    /**
     * @brief Recursively build layout tree from render subtree at specific index
     * @param render_obj Current render object
     * @param parent_id Parent node ID (0 for root)
     * @param insert_index Index to insert at in parent's children list
     */
    void BuildSubtreeAtIndex(RenderObject* render_obj, NodeId parent_id, size_t insert_index);

    /**
     * @brief Compute layout for a node
     * @param node_id Node to layout
     * @param inputs Layout input parameters
     * @return Layout output
     */
    LayoutOutput ComputeNodeLayout(NodeId node_id, const LayoutInput& inputs);

    /**
     * @brief Compute block layout for a node
     * @param node_id Node to layout
     * @param inputs Layout input parameters
     * @return Layout output
     */
    LayoutOutput ComputeBlockLayout(NodeId node_id, const LayoutInput& inputs);

    /**
     * @brief Compute flexbox layout for a node
     * @param node_id Node to layout
     * @param inputs Layout input parameters
     * @return Layout output
     */
    LayoutOutput ComputeFlexLayout(NodeId node_id, const LayoutInput& inputs);

    /**
     * @brief Compute grid layout for a node
     * @param node_id Node to layout
     * @param inputs Layout input parameters
     * @return Layout output
     */
    LayoutOutput ComputeGridLayout(NodeId node_id, const LayoutInput& inputs);

    /**
     * @brief Compute table layout for a node
     * @param node_id Node to layout
     * @param inputs Layout input parameters
     * @return Layout output
     */
    LayoutOutput ComputeTableLayout(NodeId node_id, const LayoutInput& inputs);

    /**
     * @brief Compute IFC layout for a node
     * @param node_id Node to layout
     * @param inputs Layout input parameters
     * @return Layout output
     */
    LayoutOutput ComputeIFCLayout(NodeId node_id, const LayoutInput& inputs);
    
    /**
     * @brief Compute IFC layout for an anonymous block box
     * @param node_id Anonymous block node to layout
     * @param inputs Layout input parameters
     * @return Layout output
     * 
     * Anonymous block boxes wrap inline-level content in mixed-content containers.
     * They use IFC layout but don't have a render_obj - instead they reference
     * the inline children directly.
     */
    LayoutOutput ComputeAnonymousBlockIFCLayout(NodeId node_id, const LayoutInput& inputs);
    
    /**
     * @brief Recursively collect inline boxes from an inline element
     * @param render_obj The inline element to process
     * @param inline_boxes Output vector to collect inline boxes
     * @param available_width Available width for layout
     */
    void CollectInlineBoxesRecursive(
        RenderObject* render_obj,
        std::vector<InlineBox>& inline_boxes,
        float available_width
    );
    
    /**
     * @brief Apply layout results from anonymous block to render objects
     * @param node The anonymous block node
     */
    void ApplyAnonymousBlockLayoutResults(LayoutNode* node);

    /**
     * @brief Measure a leaf node (text, image, etc.)
     * @param node_id Node to measure
     * @param inputs Layout input parameters
     * @return Layout output with measured size
     */
    LayoutOutput MeasureLeafNode(NodeId node_id, const LayoutInput& inputs);

    /**
     * @brief Read layout results and update render objects
     * @param render_obj Current render object
     */
    void ReadLayoutResults(RenderObject* render_obj);

    /**
     * @brief Position children after layout
     * @param node_id Parent node
     */
    void PositionChildren(NodeId node_id);

    /**
     * @brief Internal layout computation (does not clear caches)
     * 
     * This method contains the core layout logic extracted from ComputeLayout().
     * It performs layout computation without clearing caches first, allowing
     * incremental layout to selectively clear only dirty node caches.
     * 
     * @param available_width Available width for layout
     * @param available_height Available height for layout
     * 
     * **Feature: incremental-layout-optimization**
     * **Validates: Requirements 2.2**
     */
    void ComputeLayoutInternal(float available_width, float available_height);

    /**
     * @brief Get node by ID
     * @param id Node ID
     * @return Pointer to node, or nullptr if not found
     */
    LayoutNode* GetNode(NodeId id);
    const LayoutNode* GetNode(NodeId id) const;

    /**
     * @brief Check if a node should use IFC layout
     * @param render_obj The render object
     * @return true if IFC should be used
     */
    bool ShouldUseIFC(RenderObject* render_obj) const;
    
    /**
     * @brief Create an anonymous block box to wrap inline-level elements
     * @param parent_id Parent node ID
     * @param inline_children Vector of inline-level render objects to wrap
     * @return Node ID of the created anonymous block box
     * 
     * This implements CSS anonymous block box creation for mixed content.
     * When a block container has both block-level and inline-level children,
     * consecutive inline-level children are wrapped in anonymous block boxes.
     */
    NodeId CreateAnonymousBlockBox(NodeId parent_id, const std::vector<RenderObject*>& inline_children);

    /**
     * @brief Determine the layout scope for a node
     * 
     * Analyzes the node's style to determine how layout changes should propagate.
     * This is used to optimize incremental layout by limiting recalculation scope.
     * 
     * @param node The layout node to analyze
     * @return LayoutScope indicating how layout changes propagate
     * 
     * Rules:
     * - SELF_ONLY: Absolute/fixed positioned elements, or elements with fixed width AND height
     * - SUBTREE: Elements with fixed dimensions (width or height)
     * - SIBLINGS: Flex/grid children that may affect sibling layouts
     * - ANCESTORS: Auto-sized elements that may affect parent container size
     * 
     * **Feature: incremental-layout-optimization**
     * **Validates: Requirements 3.1.3, 3.1.4, 3.1.5**
     */
    LayoutScope DetermineLayoutScope(const LayoutNode* node) const;

    /**
     * @brief Propagate layout dirty marks based on LayoutScope
     * 
     * Intelligently propagates dirty marks through the layout tree based on
     * the node's LayoutScope. This optimizes incremental layout by limiting
     * the scope of recalculation.
     * 
     * @param node_id The node that triggered the dirty mark
     * @param scope The layout scope determining propagation behavior
     * 
     * Propagation rules:
     * - SELF_ONLY: Only mark the node itself, don't propagate to ancestors
     * - SUBTREE: Mark the node and its subtree, don't propagate to ancestors
     * - SIBLINGS: Mark the node and notify parent to recalculate siblings
     * - ANCESTORS: Mark the node and propagate to all ancestors up to root
     * 
     * **Feature: incremental-layout-optimization**
     * **Validates: Requirements 4.1, 4.2, 4.3, 4.4**
     */
    void PropagateLayoutDirty(NodeId node_id, LayoutScope scope);

    /**
     * @brief Check if a node has fixed dimensions (both width and height are explicit lengths)
     * @param node The layout node to check
     * @return true if both width and height are fixed lengths
     */
    bool HasFixedSize(const LayoutNode* node) const;

    /**
     * @brief Check if a node is a flex or grid child
     * @param node The layout node to check
     * @return true if the node's parent is a flex or grid container
     */
    bool IsFlexOrGridChild(const LayoutNode* node) const;

    /**
     * @brief Clear caches only for nodes affected by width changes
     * 
     * This method is used during scrollbar detection to optimize the two-pass layout.
     * Instead of clearing all caches, it only clears caches for nodes whose layout
     * depends on the available width. Nodes with fixed width (explicit pixel values)
     * can keep their cache results from the first pass.
     * 
     * @param node_id The root node to start clearing from
     * 
     * A node's cache is cleared if:
     * - Its width is auto, percentage, or depends on parent width
     * - It's a flex/grid child (may be affected by container width changes)
     * - Its parent's cache was cleared (cascading effect)
     * 
     * **Feature: incremental-layout-optimization**
     * **Validates: Requirements 2.5**
     */
    void ClearWidthDependentCaches(NodeId node_id);

    /**
     * @brief Check if a node's layout depends on available width
     * 
     * @param node The layout node to check
     * @return true if the node's layout depends on available width
     */
    bool IsWidthDependent(const LayoutNode* node) const;

    /**
     * @brief Recursively clear all caches in a subtree
     * 
     * Helper method used by ClearWidthDependentCaches when a parent is
     * width-dependent and all children need their caches cleared.
     * 
     * @param node_id The root node of the subtree to clear
     */
    void ClearWidthDependentCachesRecursive(NodeId node_id);
};

} // namespace lightui

#endif // LIGHTUI_NATIVE_LAYOUT_ENGINE_H

