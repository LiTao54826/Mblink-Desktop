/**
 * @file mod.h
 * @brief Layout algorithm dispatcher
 * 
 * This file provides the main entry point for layout computation,
 * dispatching to the appropriate algorithm based on display type.
 */

#pragma once

#include "../layout.h"
#include "../style.h"
#include "../tree/traits.h"
#include "block.h"
#include "flexbox.h"
#include "grid/grid.h"

namespace lightui {

//------------------------------------------------------------------------------
// Layout Algorithm Dispatcher
//------------------------------------------------------------------------------

/**
 * @brief Compute layout for a node based on its display type
 * 
 * This function dispatches to the appropriate layout algorithm:
 * - display: block  -> ComputeBlockLayout()
 * - display: flex   -> ComputeFlexboxLayout()
 * - display: grid   -> ComputeGridLayout()
 * - display: none   -> Returns zero size
 * 
 * @param tree The layout tree interface
 * @param node The node to layout
 * @param inputs Layout input parameters
 * @return LayoutOutput with computed size and positions
 */
template<typename Tree>
LayoutOutput ComputeLayout(
    Tree& tree,
    NodeId node,
    const LayoutInput& inputs
) {
    // Get node's display type
    Display display = tree.GetDisplay(node);
    
    switch (display) {
        case Display::None:
            // Hidden elements have zero size
            return LayoutOutput{};
            
        case Display::Block:
            // Block layout (may include IFC for inline content)
            if constexpr (std::is_base_of_v<LayoutBlockContainer, Tree>) {
                return ComputeBlockLayout(
                    static_cast<LayoutBlockContainer&>(tree),
                    node,
                    inputs
                );
            }
            break;
            
        case Display::Flex:
            // Flexbox layout
            if constexpr (std::is_base_of_v<LayoutFlexboxContainer, Tree>) {
                return ComputeFlexboxLayout(
                    static_cast<LayoutFlexboxContainer&>(tree),
                    node,
                    inputs
                );
            }
            break;
            
        case Display::Grid:
            // Grid layout
            if constexpr (std::is_base_of_v<LayoutGridContainer, Tree>) {
                return ComputeGridLayout(
                    static_cast<LayoutGridContainer&>(tree),
                    node,
                    inputs
                );
            }
            break;
    }
    
    // Fallback: return empty output
    return LayoutOutput{};
}

/**
 * @brief Perform layout on a subtree
 * 
 * This function recursively computes layout for a node and all its descendants.
 * It handles caching to avoid redundant computations.
 * 
 * @param tree The layout tree interface
 * @param node The root node of the subtree
 * @param inputs Layout input parameters
 * @return LayoutOutput with computed size
 */
template<typename Tree>
LayoutOutput PerformLayout(
    Tree& tree,
    NodeId node,
    const LayoutInput& inputs
) {
    // Check cache first
    Cache* cache = tree.GetCache(node);
    if (cache) {
        auto cached = cache->Get(
            inputs.known_dimensions,
            inputs.available_space,
            inputs.run_mode
        );
        if (cached.has_value()) {
            return *cached;
        }
    }
    
    // Compute layout
    LayoutOutput output = ComputeLayout(tree, node, inputs);
    
    // Store in cache
    if (cache) {
        cache->Store(
            inputs.known_dimensions,
            inputs.available_space,
            inputs.run_mode,
            output
        );
    }
    
    return output;
}

/**
 * @brief Compute layout for the entire tree
 * 
 * This is the main entry point for layout computation.
 * It sets up the root constraints and performs layout on the entire tree.
 * 
 * @param tree The layout tree interface
 * @param root The root node
 * @param available_width Available width for the root
 * @param available_height Available height for the root
 */
template<typename Tree>
void ComputeRootLayout(
    Tree& tree,
    NodeId root,
    float available_width,
    float available_height
) {
    LayoutInput inputs;
    inputs.run_mode = RunMode::PerformLayout;
    inputs.sizing_mode = SizingMode::InherentSize;
    inputs.known_dimensions = Size<std::optional<float>>{std::nullopt, std::nullopt};
    inputs.parent_size = Size<std::optional<float>>{
        std::optional<float>(available_width),
        std::optional<float>(available_height)
    };
    inputs.available_space = Size<AvailableSpace>{
        AvailableSpace::Definite(available_width),
        AvailableSpace::Definite(available_height)
    };
    
    PerformLayout(tree, root, inputs);
}

//------------------------------------------------------------------------------
// Measure Functions
//------------------------------------------------------------------------------

/**
 * @brief Measure a leaf node (text, image, etc.)
 * 
 * Leaf nodes don't have children and their size is determined by
 * their content (e.g., text measurement, image dimensions).
 * 
 * @param tree The layout tree interface
 * @param node The leaf node to measure
 * @param inputs Layout input parameters
 * @return LayoutOutput with measured size
 */
template<typename Tree>
LayoutOutput MeasureLeafNode(
    Tree& tree,
    NodeId node,
    const LayoutInput& inputs
) {
    // Get measure function from tree
    auto measure_func = tree.GetMeasureFunction(node);
    if (!measure_func) {
        return LayoutOutput{};
    }
    
    // Call measure function
    Size<float> size = measure_func(
        inputs.known_dimensions,
        inputs.available_space
    );
    
    LayoutOutput output;
    output.size = size;
    output.content_size = size;
    return output;
}

} // namespace lightui

