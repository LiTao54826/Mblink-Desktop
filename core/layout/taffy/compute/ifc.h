/**
 * @file ifc.h
 * @brief IFC (Inline Formatting Context) layout adapter
 * 
 * This file provides an adapter to integrate the existing IFC implementation
 * with the new native layout system.
 */

#pragma once

#include "../layout.h"
#include "../tree/traits.h"
#include "../../ifc_layout.h"

namespace lightui {

//------------------------------------------------------------------------------
// IFC Layout Adapter
//------------------------------------------------------------------------------

/**
 * @brief Compute IFC layout for a container
 * 
 * This function wraps the existing IFCLayout class to return a LayoutOutput
 * compatible with the new layout system.
 * 
 * @param container The container RenderObject with inline content
 * @param inputs Layout input parameters
 * @return LayoutOutput with computed size and content size
 */
inline LayoutOutput ComputeIFCLayout(
    RenderObject* container,
    const LayoutInput& inputs
) {
    LayoutOutput output;
    
    if (!container) {
        return output;
    }
    
    // Determine available width
    float available_width = 0.0f;
    if (inputs.available_space.width.type == AvailableSpaceType::Definite) {
        available_width = inputs.available_space.width.value;
    } else if (inputs.available_space.width.type == AvailableSpaceType::MaxContent) {
        available_width = 10000.0f; // Large value for max-content
    }
    // MinContent: available_width = 0
    
    // Get style for padding/border calculations
    const auto& style = container->GetComputedStyle();
    
    // Calculate padding and border
    float padding_left = style.padding.left.ToPx(available_width, style.font_size);
    float padding_right = style.padding.right.ToPx(available_width, style.font_size);
    float padding_top = style.padding.top.ToPx(0, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(0, style.font_size);
    
    float border_left = style.border_left_width;
    float border_right = style.border_right_width;
    float border_top = style.border_top_width;
    float border_bottom = style.border_bottom_width;
    
    // Handle shorthand border
    if (border_left == 0 && border_right == 0 && border_top == 0 && border_bottom == 0) {
        float border_width = style.border.width.ToPx(available_width, style.font_size);
        border_left = border_right = border_top = border_bottom = border_width;
    }
    
    // Calculate content area width (subtract padding and border)
    float content_width = available_width - padding_left - padding_right - border_left - border_right;
    if (content_width < 0) content_width = 0;
    
    // Use IFC to compute content layout
    IFCLayout ifc_layout;
    IFCLayoutResult result = ifc_layout.Layout(container, content_width);
    
    // Compute total size including padding and border
    // NOTE: This is the key difference from the old approach - we add padding/border
    // here ONCE, instead of having Taffy add it again.
    float total_width = result.max_width + padding_left + padding_right + border_left + border_right;
    float total_height = result.total_height + padding_top + padding_bottom + border_top + border_bottom;
    
    // If known dimensions are provided, use them
    if (inputs.known_dimensions.width.has_value()) {
        total_width = *inputs.known_dimensions.width;
    }
    if (inputs.known_dimensions.height.has_value()) {
        total_height = *inputs.known_dimensions.height;
    }
    
    output.size = Size<float>{total_width, total_height};
    output.content_size = Size<float>{result.max_width, result.total_height};
    
    return output;
}

/**
 * @brief Check if a container should use IFC layout
 * 
 * A container uses IFC layout if:
 * 1. It has display: block (not flex or grid)
 * 2. All its children are inline-level elements
 * 
 * @param container The container to check
 * @return true if IFC layout should be used
 */
inline bool ShouldUseIFCLayout(RenderObject* container) {
    if (!container) return false;
    
    const auto& style = container->GetComputedStyle();
    
    // Only block containers can establish IFC
    if (style.display != RenderObjectType::BLOCK) {
        return false;
    }
    
    // Check if container has inline content
    return IFCLayout::HasInlineContent(container);
}

} // namespace lightui

