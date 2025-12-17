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
struct FlexboxContainerStyle;
struct FlexboxItemStyle;
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

    const BlockContainerStyle& GetBlockContainerStyle(NodeId node) const override;
    const BlockItemStyle& GetBlockChildStyle(NodeId node) const override;
    bool IsTextNode(NodeId node) const override;

    //--------------------------------------------------------------------------
    // Flexbox and Grid style getters (for adapters)
    //--------------------------------------------------------------------------

    const FlexboxContainerStyle& GetFlexboxContainerStyle(NodeId node) const;
    const FlexboxItemStyle& GetFlexboxChildStyle(NodeId node) const;
    const GridContainerStyle& GetGridContainerStyle(NodeId node) const;
    const GridItemStyle& GetGridItemStyle(NodeId node) const;

private:
    /**
     * @brief Internal node structure for the layout tree
     */
    struct LayoutNode {
        NodeId id = 0;
        RenderObject* render_obj = nullptr;
        NodeId parent = 0;
        std::vector<NodeId> children;

        // Style (converted from ComputedStyle)
        Style style;

        // Block layout styles (for LayoutBlockContainer interface)
        BlockContainerStyle block_container_style;
        BlockItemStyle block_item_style;

        // Flexbox layout styles (for LayoutFlexboxContainer interface)
        FlexboxContainerStyle flexbox_container_style;
        FlexboxItemStyle flexbox_item_style;

        // Grid layout styles (for LayoutGridContainer interface)
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
};

} // namespace lightui

#endif // LIGHTUI_NATIVE_LAYOUT_ENGINE_H

