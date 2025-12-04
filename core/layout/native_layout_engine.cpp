/**
 * @file native_layout_engine.cpp
 * @brief Native layout engine implementation
 */

#include "native_layout_engine.h"
#include "ifc_layout.h"
#include "dom/element.h"
#include "render/render_object.h"
#include "render/render_inline_block.h"
#include "render/render_svg.h"
#include "render/text_renderer.h"
#include "render/text/font_manager.h"
#include "taffy/util/resolve.h"
#include "taffy/util/math.h"
#include "taffy/compute/block.h"
#include "taffy/compute/flexbox.h"
#include "taffy/compute/grid/grid.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace lightui {

//------------------------------------------------------------------------------
// Constructor / Destructor
//------------------------------------------------------------------------------

NativeLayoutEngine::NativeLayoutEngine() {
}

NativeLayoutEngine::~NativeLayoutEngine() {
    Clear();
}

//------------------------------------------------------------------------------
// Public Interface
//------------------------------------------------------------------------------

void NativeLayoutEngine::BuildLayoutTree(std::shared_ptr<RenderObject> root) {
    if (!root) {
        return;
    }

    // Check if we can reuse the existing tree
    auto cached = cached_root_.lock();
    if (root_node_ != 0 && cached && cached.get() == root.get()) {
        if (root->NeedsLayout()) {
            // Need to rebuild
            Clear();
            cached_root_ = root;
            BuildSubtree(root.get(), 0);
            return;
        }
        // Same tree, just update styles
        // TODO: UpdateStylesRecursive
        return;
    }

    Clear();
    cached_root_ = root;
    BuildSubtree(root.get(), 0);
}

void NativeLayoutEngine::ComputeLayout(float available_width, float available_height) {
    if (root_node_ == 0) {
        return;
    }

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

    ComputeNodeLayout(root_node_, inputs);
    PositionChildren(root_node_);
}

void NativeLayoutEngine::GetLayoutInfo(std::shared_ptr<RenderObject> root) {
    if (!root) {
        return;
    }
    ReadLayoutResults(root.get());
}

void NativeLayoutEngine::UpdateStyle(RenderObject* render_obj, const ComputedStyle& style) {
    auto it = render_to_node_.find(render_obj);
    if (it != render_to_node_.end()) {
        LayoutNode* node = GetNode(it->second);
        if (node) {
            node->style = ConvertStyle(style);
            node->needs_layout = true;
        }
    }
}

void NativeLayoutEngine::AddElement(RenderObject* render_obj, RenderObject* parent) {
    if (!render_obj || HasElement(render_obj)) {
        return;
    }

    NodeId parent_id = 0;
    if (parent) {
        auto it = render_to_node_.find(parent);
        if (it != render_to_node_.end()) {
            parent_id = it->second;
        }
    }

    NodeId node_id = CreateNode(render_obj);

    if (parent_id != 0) {
        LayoutNode* parent_node = GetNode(parent_id);
        if (parent_node) {
            parent_node->children.push_back(node_id);
        }
        LayoutNode* node = GetNode(node_id);
        if (node) {
            node->parent = parent_id;
        }
    }
}

void NativeLayoutEngine::RemoveElement(RenderObject* render_obj) {
    auto it = render_to_node_.find(render_obj);
    if (it == render_to_node_.end()) {
        return;
    }

    NodeId node_id = it->second;
    LayoutNode* node = GetNode(node_id);

    if (node && node->parent != 0) {
        LayoutNode* parent = GetNode(node->parent);
        if (parent) {
            auto& children = parent->children;
            children.erase(
                std::remove(children.begin(), children.end(), node_id),
                children.end()
            );
        }
    }

    // Remove from maps
    nodes_.erase(node_id);
    render_to_node_.erase(it);

    if (node_id == root_node_) {
        root_node_ = 0;
    }
}

void NativeLayoutEngine::Clear() {
    nodes_.clear();
    render_to_node_.clear();
    root_node_ = 0;
    next_node_id_ = 1;
}

bool NativeLayoutEngine::HasElement(RenderObject* render_obj) const {
    return render_to_node_.find(render_obj) != render_to_node_.end();
}

//------------------------------------------------------------------------------
// Private: Node Management
//------------------------------------------------------------------------------

NodeId NativeLayoutEngine::CreateNode(RenderObject* render_obj) {
    NodeId id = next_node_id_++;

    LayoutNode node;
    node.id = id;
    node.render_obj = render_obj;

    const auto& computed = render_obj->GetComputedStyle();
    node.style = ConvertStyle(computed);
    node.is_ifc_container = ShouldUseIFC(render_obj);

    // Also fill in BlockContainerStyle and BlockItemStyle for the interfaces
    node.block_container_style.display = node.style.display;
    node.block_container_style.position = node.style.position;
    node.block_container_style.overflow = node.style.overflow;
    node.block_container_style.scrollbar_width = node.style.scrollbar_width;
    node.block_container_style.size = node.style.size;
    node.block_container_style.min_size = node.style.min_size;
    node.block_container_style.max_size = node.style.max_size;
    node.block_container_style.padding = node.style.padding;
    node.block_container_style.border = node.style.border;
    node.block_container_style.margin = node.style.margin;
    node.block_container_style.inset = node.style.inset;

    node.block_item_style.display = node.style.display;
    node.block_item_style.position = node.style.position;
    node.block_item_style.overflow = node.style.overflow;
    node.block_item_style.scrollbar_width = node.style.scrollbar_width;
    node.block_item_style.size = node.style.size;
    node.block_item_style.min_size = node.style.min_size;
    node.block_item_style.max_size = node.style.max_size;
    node.block_item_style.padding = node.style.padding;
    node.block_item_style.border = node.style.border;
    node.block_item_style.margin = node.style.margin;
    node.block_item_style.inset = node.style.inset;

    // Fill in FlexboxContainerStyle and FlexboxItemStyle
    node.flexbox_container_style.display = node.style.display;
    node.flexbox_container_style.position = node.style.position;
    node.flexbox_container_style.overflow = node.style.overflow;
    node.flexbox_container_style.scrollbar_width = node.style.scrollbar_width;
    node.flexbox_container_style.size = node.style.size;
    node.flexbox_container_style.min_size = node.style.min_size;
    node.flexbox_container_style.max_size = node.style.max_size;
    node.flexbox_container_style.padding = node.style.padding;
    node.flexbox_container_style.border = node.style.border;
    node.flexbox_container_style.margin = node.style.margin;
    node.flexbox_container_style.inset = node.style.inset;
    node.flexbox_container_style.flex_direction = node.style.flex_direction;
    node.flexbox_container_style.flex_wrap = node.style.flex_wrap;
    node.flexbox_container_style.align_items = node.style.align_items.value_or(AlignItems::Stretch);
    node.flexbox_container_style.align_content = node.style.align_content.value_or(AlignContent::Stretch);
    node.flexbox_container_style.justify_content = node.style.justify_content;
    node.flexbox_container_style.gap = node.style.gap;

    node.flexbox_item_style.display = node.style.display;
    node.flexbox_item_style.position = node.style.position;
    node.flexbox_item_style.overflow = node.style.overflow;
    node.flexbox_item_style.scrollbar_width = node.style.scrollbar_width;
    node.flexbox_item_style.size = node.style.size;
    node.flexbox_item_style.min_size = node.style.min_size;
    node.flexbox_item_style.max_size = node.style.max_size;
    node.flexbox_item_style.padding = node.style.padding;
    node.flexbox_item_style.border = node.style.border;
    node.flexbox_item_style.margin = node.style.margin;
    node.flexbox_item_style.inset = node.style.inset;
    node.flexbox_item_style.align_self = node.style.align_self;
    node.flexbox_item_style.flex_grow = node.style.flex_grow;
    node.flexbox_item_style.flex_shrink = node.style.flex_shrink;
    node.flexbox_item_style.flex_basis = node.style.flex_basis;

    // Fill in GridContainerStyle and GridItemStyle
    node.grid_container_style.display = node.style.display;
    node.grid_container_style.position = node.style.position;
    node.grid_container_style.overflow = node.style.overflow;
    node.grid_container_style.scrollbar_width = node.style.scrollbar_width;
    node.grid_container_style.size = node.style.size;
    node.grid_container_style.min_size = node.style.min_size;
    node.grid_container_style.max_size = node.style.max_size;
    node.grid_container_style.padding = node.style.padding;
    node.grid_container_style.border = node.style.border;
    node.grid_container_style.margin = node.style.margin;
    node.grid_container_style.inset = node.style.inset;
    // TODO: Parse grid-template-rows, grid-template-columns, etc. from computed style

    node.grid_item_style.display = node.style.display;
    node.grid_item_style.position = node.style.position;
    node.grid_item_style.overflow = node.style.overflow;
    node.grid_item_style.scrollbar_width = node.style.scrollbar_width;
    node.grid_item_style.size = node.style.size;
    node.grid_item_style.min_size = node.style.min_size;
    node.grid_item_style.max_size = node.style.max_size;
    node.grid_item_style.padding = node.style.padding;
    node.grid_item_style.border = node.style.border;
    node.grid_item_style.margin = node.style.margin;
    node.grid_item_style.inset = node.style.inset;
    // TODO: Parse grid-row-start, grid-row-end, grid-column-start, grid-column-end

    nodes_[id] = std::move(node);
    render_to_node_[render_obj] = id;

    return id;
}

NativeLayoutEngine::LayoutNode* NativeLayoutEngine::GetNode(NodeId id) {
    auto it = nodes_.find(id);
    return it != nodes_.end() ? &it->second : nullptr;
}

const NativeLayoutEngine::LayoutNode* NativeLayoutEngine::GetNode(NodeId id) const {
    auto it = nodes_.find(id);
    return it != nodes_.end() ? &it->second : nullptr;
}

bool NativeLayoutEngine::ShouldUseIFC(RenderObject* render_obj) const {
    if (!render_obj) return false;

    const auto& style = render_obj->GetComputedStyle();
    RenderObjectType type = render_obj->GetType();

    // Only block containers can establish IFC
    if (type != RenderObjectType::BLOCK) {
        return false;
    }

    // Flex and Grid containers don't use IFC
    if (style.display == RenderObjectType::FLEX ||
        style.display == RenderObjectType::GRID) {
        return false;
    }

    // Check if all children are inline-level
    const auto& children = render_obj->GetChildren();
    if (children.empty()) {
        return false;
    }

    bool has_inline = false;
    bool has_block = false;

    for (const auto& child : children) {
        RenderObjectType child_type = child->GetType();
        RenderObjectType child_display = child->GetComputedStyle().display;

        if (child_type == RenderObjectType::TEXT) {
            has_inline = true;
        } else if (child_display == RenderObjectType::INLINE ||
                   child_display == RenderObjectType::INLINE_BLOCK) {
            has_inline = true;
        } else if (child_display == RenderObjectType::BLOCK ||
                   child_display == RenderObjectType::FLEX ||
                   child_display == RenderObjectType::GRID ||
                   child_display == RenderObjectType::TABLE) {
            has_block = true;
        }
    }

    // Use IFC only if we have inline content and no block content
    return has_inline && !has_block;
}

//------------------------------------------------------------------------------
// Private: Style Conversion
//------------------------------------------------------------------------------

namespace {

// Helper to convert CSSLength to LengthPercentage
LengthPercentage ConvertLength(const CSSLength& css_length) {
    switch (css_length.unit) {
        case CSSUnit::PX:
            return LengthPercentage::Length(css_length.value);
        case CSSUnit::PERCENT:
            return LengthPercentage::Percent(css_length.value / 100.0f);
        case CSSUnit::EM:
            return LengthPercentage::Length(css_length.value * 16.0f); // Assume 16px default
        case CSSUnit::REM:
            return LengthPercentage::Length(css_length.value * 16.0f); // Root font size
        case CSSUnit::AUTO:
        case CSSUnit::NONE:
        default:
            return LengthPercentage::Zero();
    }
}

// Helper to convert CSSLength to Dimension
Dimension ConvertDimension(const CSSLength& css_length) {
    switch (css_length.unit) {
        case CSSUnit::AUTO:
        case CSSUnit::NONE:
            return Dimension::Auto();
        case CSSUnit::PX:
            return Dimension::Length(css_length.value);
        case CSSUnit::PERCENT:
            return Dimension::Percent(css_length.value / 100.0f);
        case CSSUnit::EM:
            return Dimension::Length(css_length.value * 16.0f);
        case CSSUnit::REM:
            return Dimension::Length(css_length.value * 16.0f);
        default:
            return Dimension::Auto();
    }
}

// Helper to convert CSSLength to LengthPercentageAuto
LengthPercentageAuto ConvertLengthAuto(const CSSLength& css_length) {
    switch (css_length.unit) {
        case CSSUnit::AUTO:
        case CSSUnit::NONE:
            return LengthPercentageAuto::Auto();
        case CSSUnit::PX:
            return LengthPercentageAuto::Length(css_length.value);
        case CSSUnit::PERCENT:
            return LengthPercentageAuto::Percent(css_length.value / 100.0f);
        case CSSUnit::EM:
            return LengthPercentageAuto::Length(css_length.value * 16.0f);
        case CSSUnit::REM:
            return LengthPercentageAuto::Length(css_length.value * 16.0f);
        default:
            return LengthPercentageAuto::Auto();
    }
}

} // anonymous namespace

Style NativeLayoutEngine::ConvertStyle(const ComputedStyle& computed) {
    Style style;

    // Display
    switch (computed.display) {
        case RenderObjectType::NONE:
            style.display = Display::None;
            break;
        case RenderObjectType::FLEX:
            style.display = Display::Flex;
            break;
        case RenderObjectType::GRID:
            style.display = Display::Grid;
            break;
        default:
            style.display = Display::Block;
            break;
    }

    // Position
    if (computed.position == "absolute") {
        style.position = Position::Absolute;
    } else if (computed.position == "relative") {
        style.position = Position::Relative;
    } else {
        style.position = Position::Relative;
    }

    // Size
    style.size.width = ConvertDimension(computed.width);
    style.size.height = ConvertDimension(computed.height);
    style.min_size.width = ConvertDimension(computed.min_width);
    style.min_size.height = ConvertDimension(computed.min_height);
    style.max_size.width = ConvertDimension(computed.max_width);
    style.max_size.height = ConvertDimension(computed.max_height);

    // Padding
    style.padding.left = ConvertLength(computed.padding.left);
    style.padding.right = ConvertLength(computed.padding.right);
    style.padding.top = ConvertLength(computed.padding.top);
    style.padding.bottom = ConvertLength(computed.padding.bottom);

    // Margin
    style.margin.left = ConvertLengthAuto(computed.margin.left);
    style.margin.right = ConvertLengthAuto(computed.margin.right);
    style.margin.top = ConvertLengthAuto(computed.margin.top);
    style.margin.bottom = ConvertLengthAuto(computed.margin.bottom);

    // Border widths
    style.border.left = LengthPercentage::Length(computed.border_left_width);
    style.border.right = LengthPercentage::Length(computed.border_right_width);
    style.border.top = LengthPercentage::Length(computed.border_top_width);
    style.border.bottom = LengthPercentage::Length(computed.border_bottom_width);

    // Inset (for positioned elements)
    style.inset.left = ConvertLengthAuto(computed.left);
    style.inset.right = ConvertLengthAuto(computed.right);
    style.inset.top = ConvertLengthAuto(computed.top);
    style.inset.bottom = ConvertLengthAuto(computed.bottom);

    // Flexbox properties
    style.flex_direction = FlexDirection::Row;
    if (computed.flex_direction == "column") {
        style.flex_direction = FlexDirection::Column;
    } else if (computed.flex_direction == "row-reverse") {
        style.flex_direction = FlexDirection::RowReverse;
    } else if (computed.flex_direction == "column-reverse") {
        style.flex_direction = FlexDirection::ColumnReverse;
    }

    style.flex_wrap = FlexWrap::NoWrap;
    if (computed.flex_wrap == "wrap") {
        style.flex_wrap = FlexWrap::Wrap;
    } else if (computed.flex_wrap == "wrap-reverse") {
        style.flex_wrap = FlexWrap::WrapReverse;
    }

    style.flex_grow = computed.flex_grow;
    style.flex_shrink = computed.flex_shrink;
    style.flex_basis = ConvertDimension(computed.flex_basis);

    // Alignment
    if (computed.justify_content == "flex-start" || computed.justify_content == "start") {
        style.justify_content = JustifyContent::Start;
    } else if (computed.justify_content == "flex-end" || computed.justify_content == "end") {
        style.justify_content = JustifyContent::End;
    } else if (computed.justify_content == "center") {
        style.justify_content = JustifyContent::Center;
    } else if (computed.justify_content == "space-between") {
        style.justify_content = JustifyContent::SpaceBetween;
    } else if (computed.justify_content == "space-around") {
        style.justify_content = JustifyContent::SpaceAround;
    } else if (computed.justify_content == "space-evenly") {
        style.justify_content = JustifyContent::SpaceEvenly;
    }

    if (computed.align_items == "flex-start" || computed.align_items == "start") {
        style.align_items = AlignItems::Start;
    } else if (computed.align_items == "flex-end" || computed.align_items == "end") {
        style.align_items = AlignItems::End;
    } else if (computed.align_items == "center") {
        style.align_items = AlignItems::Center;
    } else if (computed.align_items == "baseline") {
        style.align_items = AlignItems::Baseline;
    } else if (computed.align_items == "stretch") {
        style.align_items = AlignItems::Stretch;
    }

    if (computed.align_content == "flex-start" || computed.align_content == "start") {
        style.align_content = AlignContent::Start;
    } else if (computed.align_content == "flex-end" || computed.align_content == "end") {
        style.align_content = AlignContent::End;
    } else if (computed.align_content == "center") {
        style.align_content = AlignContent::Center;
    } else if (computed.align_content == "stretch") {
        style.align_content = AlignContent::Stretch;
    } else if (computed.align_content == "space-between") {
        style.align_content = AlignContent::SpaceBetween;
    } else if (computed.align_content == "space-around") {
        style.align_content = AlignContent::SpaceAround;
    }

    if (computed.align_self == "flex-start" || computed.align_self == "start") {
        style.align_self = AlignSelf::Start;
    } else if (computed.align_self == "flex-end" || computed.align_self == "end") {
        style.align_self = AlignSelf::End;
    } else if (computed.align_self == "center") {
        style.align_self = AlignSelf::Center;
    } else if (computed.align_self == "baseline") {
        style.align_self = AlignSelf::Baseline;
    } else if (computed.align_self == "stretch") {
        style.align_self = AlignSelf::Stretch;
    }

    // Gap
    style.gap.width = ConvertLength(computed.column_gap);
    style.gap.height = ConvertLength(computed.row_gap);

    return style;
}

//------------------------------------------------------------------------------
// Private: Tree Building
//------------------------------------------------------------------------------

void NativeLayoutEngine::BuildSubtree(RenderObject* render_obj, NodeId parent_id) {
    if (!render_obj) {
        return;
    }

    NodeId node_id = CreateNode(render_obj);

    // Set as root if no parent
    if (parent_id == 0) {
        root_node_ = node_id;
    } else {
        LayoutNode* parent = GetNode(parent_id);
        if (parent) {
            parent->children.push_back(node_id);
        }
        LayoutNode* node = GetNode(node_id);
        if (node) {
            node->parent = parent_id;
        }
    }

    // Check element type
    RenderObjectType type = render_obj->GetType();

    // For INLINE_BLOCK and INLINE elements, they manage their own children
    if (type == RenderObjectType::INLINE_BLOCK || type == RenderObjectType::INLINE) {
        return;
    }

    // For TABLE elements, they manage their own layout
    if (type == RenderObjectType::TABLE ||
        type == RenderObjectType::TABLE_ROW_GROUP ||
        type == RenderObjectType::TABLE_HEADER_GROUP ||
        type == RenderObjectType::TABLE_FOOTER_GROUP ||
        type == RenderObjectType::TABLE_ROW ||
        type == RenderObjectType::TABLE_CELL ||
        type == RenderObjectType::TABLE_CAPTION) {
        return;
    }

    // For SVG elements, they manage their own layout
    if (render_obj->IsSVGRenderObject()) {
        return;
    }

    // For IFC containers, don't add children to layout tree
    LayoutNode* node = GetNode(node_id);
    if (node && node->is_ifc_container) {
        return;
    }

    // Recursively build children
    for (auto& child : render_obj->GetChildren()) {
        BuildSubtree(child.get(), node_id);
    }
}

//------------------------------------------------------------------------------
// Private: Layout Computation
//------------------------------------------------------------------------------

LayoutOutput NativeLayoutEngine::ComputeNodeLayout(NodeId node_id, const LayoutInput& inputs) {
    LayoutNode* node = GetNode(node_id);
    if (!node) {
        return LayoutOutput{};
    }

    // Check cache
    auto cached = node->cache.Get(
        inputs.known_dimensions,
        inputs.available_space,
        inputs.run_mode
    );
    if (cached.has_value()) {
        return *cached;
    }

    LayoutOutput output;

    // Dispatch based on display type
    switch (node->style.display) {
        case Display::None:
            output = LayoutOutput{};
            break;

        case Display::Block:
            if (node->is_ifc_container) {
                output = ComputeIFCLayout(node_id, inputs);
            } else {
                output = ComputeBlockLayout(node_id, inputs);
            }
            break;

        case Display::Flex:
            output = ComputeFlexLayout(node_id, inputs);
            break;

        case Display::Grid:
            output = ComputeGridLayout(node_id, inputs);
            break;
    }

    // Store in cache
    node->cache.Store(
        inputs.known_dimensions,
        inputs.available_space,
        inputs.run_mode,
        output
    );

    node->output = output;
    return output;
}

LayoutOutput NativeLayoutEngine::ComputeBlockLayout(NodeId node_id, const LayoutInput& inputs) {
    // Use Taffy's block layout algorithm
    return lightui::ComputeBlockLayout(*this, node_id, inputs);
}


//------------------------------------------------------------------------------
// Flexbox Layout Adapter
//------------------------------------------------------------------------------

// Forward declaration
class FlexboxAdapter;

/**
 * @brief Adapter class that implements LayoutFlexboxContainer interface
 * for NativeLayoutEngine
 */
class FlexboxAdapter : public LayoutFlexboxContainer {
public:
    FlexboxAdapter(NativeLayoutEngine& engine) : engine_(engine) {}

    // LayoutTree interface
    size_t ChildCount(NodeId node) const override {
        return engine_.ChildCount(node);
    }

    NodeId GetChildId(NodeId node, size_t index) const override {
        return engine_.GetChildId(node, index);
    }

    Cache& GetCache(NodeId node) override {
        return engine_.GetCache(node);
    }

    void SetUnroundedLayout(NodeId node, const Layout& layout) override {
        engine_.SetUnroundedLayout(node, layout);
    }

    const Layout& GetLayout(NodeId node) const override {
        return engine_.GetLayout(node);
    }

    LayoutOutput PerformChildLayout(
        NodeId node,
        Size<std::optional<float>> known_dimensions,
        Size<std::optional<float>> parent_size,
        Size<AvailableSpace> available_space,
        SizingMode sizing_mode,
        Line<bool> vertical_margins_are_collapsible
    ) override {
        return engine_.PerformChildLayout(node, known_dimensions, parent_size,
                                          available_space, sizing_mode,
                                          vertical_margins_are_collapsible);
    }

    Size<float> MeasureChildSize(
        NodeId node,
        Size<std::optional<float>> known_dimensions,
        Size<std::optional<float>> parent_size,
        Size<AvailableSpace> available_space,
        SizingMode sizing_mode
    ) override {
        return engine_.MeasureChildSize(node, known_dimensions, parent_size,
                                        available_space, sizing_mode);
    }

    // LayoutFlexboxContainer interface
    const FlexboxContainerStyle& GetFlexboxContainerStyle(NodeId node) const override {
        return engine_.GetFlexboxContainerStyle(node);
    }

    const FlexboxItemStyle& GetFlexboxChildStyle(NodeId node) const override {
        return engine_.GetFlexboxChildStyle(node);
    }

private:
    NativeLayoutEngine& engine_;
};

//------------------------------------------------------------------------------
// Grid Layout Adapter
//------------------------------------------------------------------------------

/**
 * @brief Adapter class that implements LayoutGridContainer interface
 * for NativeLayoutEngine
 */
class GridAdapter : public LayoutGridContainer {
public:
    GridAdapter(NativeLayoutEngine& engine) : engine_(engine) {}

    // LayoutTree interface
    size_t ChildCount(NodeId node) const override {
        return engine_.ChildCount(node);
    }

    NodeId GetChildId(NodeId node, size_t index) const override {
        return engine_.GetChildId(node, index);
    }

    Cache& GetCache(NodeId node) override {
        return engine_.GetCache(node);
    }

    void SetUnroundedLayout(NodeId node, const Layout& layout) override {
        engine_.SetUnroundedLayout(node, layout);
    }

    const Layout& GetLayout(NodeId node) const override {
        return engine_.GetLayout(node);
    }

    LayoutOutput PerformChildLayout(
        NodeId node,
        Size<std::optional<float>> known_dimensions,
        Size<std::optional<float>> parent_size,
        Size<AvailableSpace> available_space,
        SizingMode sizing_mode,
        Line<bool> vertical_margins_are_collapsible
    ) override {
        return engine_.PerformChildLayout(node, known_dimensions, parent_size,
                                          available_space, sizing_mode,
                                          vertical_margins_are_collapsible);
    }

    Size<float> MeasureChildSize(
        NodeId node,
        Size<std::optional<float>> known_dimensions,
        Size<std::optional<float>> parent_size,
        Size<AvailableSpace> available_space,
        SizingMode sizing_mode
    ) override {
        return engine_.MeasureChildSize(node, known_dimensions, parent_size,
                                        available_space, sizing_mode);
    }

    // LayoutGridContainer interface
    const GridContainerStyle& GetGridContainerStyle(NodeId node) const override {
        return engine_.GetGridContainerStyle(node);
    }

    const GridItemStyle& GetGridItemStyle(NodeId node) const override {
        return engine_.GetGridItemStyle(node);
    }

private:
    NativeLayoutEngine& engine_;
};

//------------------------------------------------------------------------------
// Flexbox and Grid Layout Implementation
//------------------------------------------------------------------------------

LayoutOutput NativeLayoutEngine::ComputeFlexLayout(NodeId node_id, const LayoutInput& inputs) {
    // Create adapter and call translated Taffy algorithm
    FlexboxAdapter adapter(*this);
    return ComputeFlexboxLayout(adapter, node_id, inputs);
}

LayoutOutput NativeLayoutEngine::ComputeGridLayout(NodeId node_id, const LayoutInput& inputs) {
    // Create adapter and call translated Taffy algorithm
    GridAdapter adapter(*this);
    return lightui::ComputeGridLayout(adapter, node_id, inputs);
}

LayoutOutput NativeLayoutEngine::ComputeIFCLayout(NodeId node_id, const LayoutInput& inputs) {
    LayoutNode* node = GetNode(node_id);
    if (!node || !node->render_obj) {
        return LayoutOutput{};
    }

    const auto& style = node->render_obj->GetComputedStyle();

    // Resolve container width - prefer known_dimensions (fixed width) over available_space
    // This is critical for text-align: center to work correctly
    float container_width = 0.0f;
    if (inputs.known_dimensions.width.has_value()) {
        // Container has a fixed width (e.g., width: 200px)
        container_width = *inputs.known_dimensions.width;
    } else if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
        container_width = inputs.available_space.width.value;
    } else if (inputs.available_space.width.type == AvailableSpace::Type::MaxContent) {
        container_width = 10000.0f;
    }

    // Resolve padding and border
    float padding_left = style.padding.left.ToPx(container_width, style.font_size);
    float padding_right = style.padding.right.ToPx(container_width, style.font_size);
    float padding_top = style.padding.top.ToPx(0, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(0, style.font_size);

    float border_left = style.border_left_width;
    float border_right = style.border_right_width;
    float border_top = style.border_top_width;
    float border_bottom = style.border_bottom_width;
    if (border_left == 0 && border_right == 0 && border_top == 0 && border_bottom == 0) {
        float border_width = style.border.width.ToPx(container_width, style.font_size);
        border_left = border_right = border_top = border_bottom = border_width;
    }

    // Calculate content area width (container width minus padding and border)
    float content_width = container_width - padding_left - padding_right - border_left - border_right;
    if (content_width < 0) content_width = 0;

    // Use IFC to compute content layout with the correct content width
    IFCLayoutResult result = ifc_layout_.Layout(node->render_obj, content_width);

    // Calculate total size including padding and border
    float total_width = result.max_width + padding_left + padding_right + border_left + border_right;
    float total_height = result.total_height + padding_top + padding_bottom + border_top + border_bottom;

    // Apply known dimensions if provided (override calculated size)
    if (inputs.known_dimensions.width.has_value()) {
        total_width = *inputs.known_dimensions.width;
    }
    if (inputs.known_dimensions.height.has_value()) {
        total_height = *inputs.known_dimensions.height;
    }

    // Apply min/max constraints
    // Note: min-height and max-height need to be applied even for IFC containers
    float min_height = style.min_height.ToPx(0, style.font_size);
    float max_height = style.max_height.ToPx(0, style.font_size);
    float min_width = style.min_width.ToPx(container_width, style.font_size);
    float max_width = style.max_width.ToPx(container_width, style.font_size);

    if (min_height > 0) {
        total_height = std::max(total_height, min_height);
    }
    if (max_height > 0) {
        total_height = std::min(total_height, max_height);
    }
    if (min_width > 0) {
        total_width = std::max(total_width, min_width);
    }
    if (max_width > 0) {
        total_width = std::min(total_width, max_width);
    }

    LayoutOutput output;
    output.size = Size<float>{total_width, total_height};
    output.content_size = Size<float>{result.max_width, result.total_height};

    return output;
}

LayoutOutput NativeLayoutEngine::MeasureLeafNode(NodeId node_id, const LayoutInput& inputs) {
    LayoutNode* node = GetNode(node_id);
    if (!node || !node->render_obj) {
        return LayoutOutput{};
    }

    RenderObject* render_obj = node->render_obj;
    RenderObjectType type = render_obj->GetType();

    // Handle text nodes
    if (type == RenderObjectType::TEXT) {
        auto* text_obj = static_cast<RenderText*>(render_obj);
        const std::string& text = text_obj->GetText();

        if (text.empty()) {
            return LayoutOutput{};
        }

        const auto& style = text_obj->GetComputedStyle();

        // Create font
        FontDescriptor desc;
        desc.family = style.font_family;
        desc.size = style.font_size;
        desc.weight = (style.font_weight == "bold") ? FontWeight::BOLD : FontWeight::NORMAL;
        desc.style = (style.font_style == "italic") ? FontStyle::ITALIC : FontStyle::NORMAL;

        SkFont font = FontManager::GetInstance().LoadFont(desc);
        TextRenderer text_renderer(nullptr);

        // Determine available width
        float available_width = 0.0f;
        bool should_wrap = false;

        if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
            available_width = inputs.available_space.width.value;
            should_wrap = true;
        } else if (inputs.available_space.width.type == AvailableSpace::Type::MinContent) {
            available_width = 0;
            should_wrap = true;
        }

        LayoutOutput output;

        if (should_wrap && available_width > 0) {
            std::vector<std::string> lines = text_renderer.WrapText(text, available_width, font);
            text_obj->SetWrappedLines(lines);

            float max_line_width = 0.0f;
            for (const auto& line : lines) {
                float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
                max_line_width = f32_max(max_line_width, line_width);
            }

            float line_height = style.line_height * style.font_size;
            output.size.width = max_line_width;
            output.size.height = lines.size() * line_height;
            text_obj->SetActualTextWidth(max_line_width);
        } else {
            text_obj->SetWrappedLines({});
            float text_width = text_renderer.MeasureTextWidthWithEmoji(text, font);
            float line_height = style.line_height * style.font_size;
            output.size.width = text_width;
            output.size.height = line_height;
            text_obj->SetActualTextWidth(text_width);
        }

        output.content_size = output.size;
        return output;
    }

    // Handle inline-block elements
    if (type == RenderObjectType::INLINE_BLOCK) {
        auto* inline_block = static_cast<RenderInlineBlock*>(render_obj);

        float available_width = 0.0f;
        if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
            available_width = inputs.available_space.width.value;
        } else if (inputs.available_space.width.type == AvailableSpace::Type::MaxContent) {
            available_width = 10000.0f;
        }

        auto [width, height] = inline_block->MeasureIntrinsicSize(available_width);

        LayoutOutput output;
        output.size = Size<float>{width, height};
        output.content_size = output.size;
        return output;
    }

    return LayoutOutput{};
}

//------------------------------------------------------------------------------
// Private: Position and Read Results
//------------------------------------------------------------------------------

void NativeLayoutEngine::PositionChildren(NodeId node_id) {
    LayoutNode* node = GetNode(node_id);
    if (!node) return;

    for (NodeId child_id : node->children) {
        PositionChildren(child_id);
    }
}

void NativeLayoutEngine::ReadLayoutResults(RenderObject* render_obj) {
    if (!render_obj) {
        return;
    }

    auto it = render_to_node_.find(render_obj);
    if (it == render_to_node_.end()) {
        return;
    }

    LayoutNode* node = GetNode(it->second);
    if (!node) {
        return;
    }

    // Update render object with layout info
    LayoutInfo& info = render_obj->GetLayoutInfo();
    info.x = node->x;
    info.y = node->y;
    info.width = node->output.size.width;
    info.height = node->output.size.height;
    info.is_laid_out = true;

    // Handle IFC containers
    // IFC layout already applies padding/border offset in ApplyLayoutResults,
    // so we just mark children as laid out without modifying positions
    if (node->is_ifc_container) {
        for (auto& child : render_obj->GetChildren()) {
            LayoutInfo& child_info = child->GetLayoutInfo();
            child_info.is_laid_out = true;
        }
        return;
    }

    // Recursively read children
    for (auto& child : render_obj->GetChildren()) {
        ReadLayoutResults(child.get());
    }
}

//------------------------------------------------------------------------------
// LayoutTree Interface Implementation
//------------------------------------------------------------------------------

size_t NativeLayoutEngine::ChildCount(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        return 0;
    }
    return it->second.children.size();
}

NodeId NativeLayoutEngine::GetChildId(NodeId node, size_t index) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end() || index >= it->second.children.size()) {
        return INVALID_NODE_ID;
    }
    return it->second.children[index];
}

Cache& NativeLayoutEngine::GetCache(NodeId node) {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static Cache empty_cache;
        return empty_cache;
    }
    return it->second.cache;
}

void NativeLayoutEngine::SetUnroundedLayout(NodeId node, const Layout& layout) {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        it->second.layout = layout;
        // Also update x, y for backward compatibility
        it->second.x = layout.location.x;
        it->second.y = layout.location.y;
    }
}

const Layout& NativeLayoutEngine::GetLayout(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        return default_layout_;
    }
    return it->second.layout;
}

LayoutOutput NativeLayoutEngine::PerformChildLayout(
    NodeId node,
    Size<std::optional<float>> known_dimensions,
    Size<std::optional<float>> parent_size,
    Size<AvailableSpace> available_space,
    SizingMode sizing_mode,
    Line<bool> vertical_margins_are_collapsible
) {
    LayoutInput inputs;
    inputs.known_dimensions = known_dimensions;
    inputs.parent_size = parent_size;
    inputs.available_space = available_space;
    inputs.sizing_mode = sizing_mode;
    inputs.vertical_margins_are_collapsible = vertical_margins_are_collapsible;
    inputs.run_mode = RunMode::PerformLayout;

    return ComputeNodeLayout(node, inputs);
}

Size<float> NativeLayoutEngine::MeasureChildSize(
    NodeId node,
    Size<std::optional<float>> known_dimensions,
    Size<std::optional<float>> parent_size,
    Size<AvailableSpace> available_space,
    SizingMode sizing_mode
) {
    LayoutInput inputs;
    inputs.known_dimensions = known_dimensions;
    inputs.parent_size = parent_size;
    inputs.available_space = available_space;
    inputs.sizing_mode = sizing_mode;
    inputs.run_mode = RunMode::ComputeSize;
    inputs.vertical_margins_are_collapsible = Line<bool>{false, false};

    LayoutOutput output = ComputeNodeLayout(node, inputs);
    return output.size;
}

//------------------------------------------------------------------------------
// LayoutBlockContainer Interface Implementation
//------------------------------------------------------------------------------

const BlockContainerStyle& NativeLayoutEngine::GetBlockContainerStyle(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static BlockContainerStyle default_style;
        return default_style;
    }
    return it->second.block_container_style;
}

const BlockItemStyle& NativeLayoutEngine::GetBlockChildStyle(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static BlockItemStyle default_style;
        return default_style;
    }
    return it->second.block_item_style;
}

//------------------------------------------------------------------------------
// Flexbox and Grid Style Getters
//------------------------------------------------------------------------------

const FlexboxContainerStyle& NativeLayoutEngine::GetFlexboxContainerStyle(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static FlexboxContainerStyle default_style;
        return default_style;
    }
    return it->second.flexbox_container_style;
}

const FlexboxItemStyle& NativeLayoutEngine::GetFlexboxChildStyle(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static FlexboxItemStyle default_style;
        return default_style;
    }
    return it->second.flexbox_item_style;
}

const GridContainerStyle& NativeLayoutEngine::GetGridContainerStyle(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static GridContainerStyle default_style;
        return default_style;
    }
    return it->second.grid_container_style;
}

const GridItemStyle& NativeLayoutEngine::GetGridItemStyle(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static GridItemStyle default_style;
        return default_style;
    }
    return it->second.grid_item_style;
}

} // namespace lightui

