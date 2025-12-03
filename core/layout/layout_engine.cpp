/**
 * @file layout_engine.cpp
 * @brief Layout engine implementation using Taffy
 */

#include "layout_engine.h"
#include "dom/element.h"
#include "dom/svg_element.h"
#include "render/render_object.h"
#include "render/render_inline_block.h"
#include "render/render_svg.h"
#include "render/text_renderer.h"
#include "render/text/font_manager.h"
#include <cmath>
#include <iostream>

// InlineBlock measurement callback for Taffy
// This function is called by Taffy during layout to measure inline-block elements
static TaffySize InlineBlockMeasureFunction(
    TaffyMeasureMode width_measure_mode,
    float width,
    TaffyMeasureMode height_measure_mode,
    float height,
    void* context)
{
    TaffySize size = {0.0f, 0.0f};

    if (!context) {
        return size;
    }

    auto* inline_block = static_cast<lightui::RenderInlineBlock*>(context);

    // Determine available width
    float available_width = 0.0f;
    switch (width_measure_mode) {
        case TAFFY_MEASURE_MODE_EXACT:
        case TAFFY_MEASURE_MODE_FIT_CONTENT:
            available_width = width;
            break;
        case TAFFY_MEASURE_MODE_MIN_CONTENT:
            available_width = 0;
            break;
        case TAFFY_MEASURE_MODE_MAX_CONTENT:
            available_width = 10000.0f; // 足够大的值
            break;
    }

    // 使用 inline-block 元素的固有尺寸计算
    auto [measured_width, measured_height] = inline_block->MeasureIntrinsicSize(available_width);

    size.width = measured_width;
    size.height = measured_height;

    return size;
}

// Inline element measurement callback for Taffy
// This function is called by Taffy during layout to measure inline elements (like buttons)
static TaffySize InlineMeasureFunction(
    TaffyMeasureMode width_measure_mode,
    float width,
    TaffyMeasureMode height_measure_mode,
    float height,
    void* context)
{
    TaffySize size = {0.0f, 0.0f};

    if (!context) {
        return size;
    }

    auto* inline_elem = static_cast<lightui::RenderInline*>(context);

    // Determine available width
    float available_width = 0.0f;
    switch (width_measure_mode) {
        case TAFFY_MEASURE_MODE_EXACT:
        case TAFFY_MEASURE_MODE_FIT_CONTENT:
            available_width = width;
            break;
        case TAFFY_MEASURE_MODE_MIN_CONTENT:
            available_width = 0;
            break;
        case TAFFY_MEASURE_MODE_MAX_CONTENT:
            available_width = 10000.0f;
            break;
    }

    // 使用 inline 元素的固有尺寸计算
    auto [measured_width, measured_height] = inline_elem->MeasureIntrinsicSize(available_width);

    size.width = measured_width;
    size.height = measured_height;

    return size;
}

// Text measurement callback for Taffy
// This function is called by Taffy during layout to measure text nodes
static TaffySize TextMeasureFunction(
    TaffyMeasureMode width_measure_mode,
    float width,
    TaffyMeasureMode height_measure_mode,
    float height,
    void* context)
{
    TaffySize size = {0.0f, 0.0f};

    if (!context) {
        return size;
    }

    auto* text_obj = static_cast<lightui::RenderText*>(context);
    const std::string& text = text_obj->GetText();

    if (text.empty()) {
        return size;
    }

    const auto& style = text_obj->GetComputedStyle();

    // Create font
    lightui::FontDescriptor desc;
    desc.family = style.font_family;
    desc.size = style.font_size;
    desc.weight = (style.font_weight == "bold") ? lightui::FontWeight::BOLD : lightui::FontWeight::NORMAL;
    desc.style = (style.font_style == "italic") ? lightui::FontStyle::ITALIC : lightui::FontStyle::NORMAL;

    SkFont font = lightui::FontManager::GetInstance().LoadFont(desc);
    lightui::TextRenderer text_renderer(nullptr);

    // Determine available width for text wrapping
    float available_width = 0.0f;
    bool should_wrap = false;

    switch (width_measure_mode) {
        case TAFFY_MEASURE_MODE_EXACT:
            // Exact width constraint - wrap text to this width
            available_width = width;
            should_wrap = true;
            break;
        case TAFFY_MEASURE_MODE_FIT_CONTENT:
            // Fit content with max width constraint
            available_width = width;
            should_wrap = (width > 0);
            break;
        case TAFFY_MEASURE_MODE_MIN_CONTENT:
            // Minimum content width - wrap at every opportunity
            available_width = 0;
            should_wrap = true;
            break;
        case TAFFY_MEASURE_MODE_MAX_CONTENT:
            // Maximum content width - no wrapping
            available_width = 0;
            should_wrap = false;
            break;
    }

    if (should_wrap && available_width > 0) {
        // Wrap text and calculate size
        std::vector<std::string> lines = text_renderer.WrapText(text, available_width, font);

        // Store wrapped lines in the RenderText object for later rendering
        text_obj->SetWrappedLines(lines);

        float max_line_width = 0.0f;
        for (size_t i = 0; i < lines.size(); ++i) {
            float line_width = text_renderer.MeasureTextWidthWithEmoji(lines[i], font);
            max_line_width = std::max(max_line_width, line_width);
        }

        float line_height = style.line_height * style.font_size;
        size.width = max_line_width;
        size.height = lines.size() * line_height;

        // Store actual text width for text-align calculation
        text_obj->SetActualTextWidth(max_line_width);
    } else {
        // Single line measurement - clear any previous wrapped lines
        text_obj->SetWrappedLines({});

        // Use MeasureTextWidthWithEmoji for consistent width measurement with rendering
        float text_width = text_renderer.MeasureTextWidthWithEmoji(text, font);
        // 单行文本也应该使用 line_height 来计算高度，与浏览器行为一致
        // 浏览器的 line-height: normal 会应用到所有文本，包括单行文本
        float line_height = style.line_height * style.font_size;

        size.width = text_width;
        size.height = line_height;

        // Store actual text width for text-align calculation
        text_obj->SetActualTextWidth(text_width);
    }

    return size;
}

// Table measurement callback for Taffy
// This function is called by Taffy during layout to measure table elements
static TaffySize TableMeasureFunction(
    TaffyMeasureMode width_measure_mode,
    float width,
    TaffyMeasureMode height_measure_mode,
    float height,
    void* context)
{
    TaffySize size = {0.0f, 0.0f};

    if (!context) {
        return size;
    }

    auto* table = static_cast<lightui::RenderTable*>(context);

    // Determine available width
    float available_width = 0.0f;
    switch (width_measure_mode) {
        case TAFFY_MEASURE_MODE_EXACT:
        case TAFFY_MEASURE_MODE_FIT_CONTENT:
            available_width = width;
            break;
        case TAFFY_MEASURE_MODE_MIN_CONTENT:
            available_width = 0;
            break;
        case TAFFY_MEASURE_MODE_MAX_CONTENT:
            available_width = 10000.0f;
            break;
    }

    // Layout the table with the available width
    table->Layout(available_width, 0);

    // Return the calculated size
    const auto& layout = table->GetLayoutInfo();
    size.width = layout.width;
    size.height = layout.height;

    return size;
}

namespace lightui {

LayoutEngine::LayoutEngine()
    : taffy_tree_(nullptr)
    , has_root_(false) {
    // Initialize Taffy tree
    taffy_tree_ = TaffyTree_New();
}

LayoutEngine::~LayoutEngine() {
    Clear();
    // Free Taffy tree
    if (taffy_tree_) {
        TaffyTree_Free(taffy_tree_);
        taffy_tree_ = nullptr;
    }
}

void LayoutEngine::BuildLayoutTree(std::shared_ptr<RenderObject> root) {
    if (!root || !taffy_tree_) {
        return;
    }

    // 优化：如果布局树已经存在且是同一个渲染树对象，只需要更新样式而不是重建
    // 这避免了在窗口 resize 时重新构建整个布局树
    auto cached = cached_root_.lock();
    if (has_root_ && cached && cached.get() == root.get()) {
        // 检查是否有任何节点需要布局（内容改变等）
        // 如果有，需要重建 Taffy 树以便重新测量
        if (root->NeedsLayout()) {
            // 有节点需要重新布局，清除并重建 Taffy 树
            Clear();
            cached_root_ = root;
            TaffyNodeId invalid_parent;
            invalid_parent._0 = 0;
            BuildSubtree(root.get(), invalid_parent);
            return;
        }
        // 同一个渲染树对象，没有布局需求，只需重新应用样式
        UpdateStylesRecursive(root.get());
        return;
    }
    Clear();
    cached_root_ = root;  // 缓存当前根节点

    // Build tree from root
    TaffyNodeId invalid_parent;
    invalid_parent._0 = 0;
    BuildSubtree(root.get(), invalid_parent);
}

void LayoutEngine::ComputeLayout(float available_width, float available_height) {
    if (!has_root_ || !taffy_tree_) {
        return;
    }

    // Call Taffy layout computation
    TaffyTree_ComputeLayout(taffy_tree_, root_node_, available_width, available_height);
}

void LayoutEngine::GetLayoutInfo(std::shared_ptr<RenderObject> root) {
    if (!root || !taffy_tree_) {
        return;
    }

    // Read layout results recursively
    ReadLayoutResults(root.get());
}

void LayoutEngine::UpdateStyle(RenderObject* render_obj, const ComputedStyle& style) {
    auto it = element_to_node_.find(render_obj);
    if (it != element_to_node_.end()) {
        ApplyStyle(it->second, style);
    }
}

void LayoutEngine::AddElement(RenderObject* render_obj, RenderObject* parent) {
    if (!render_obj || HasElement(render_obj) || !taffy_tree_) {
        return;
    }

    TaffyNodeId parent_node;
    parent_node._0 = 0;

    if (parent) {
        auto it = element_to_node_.find(parent);
        if (it != element_to_node_.end()) {
            parent_node = it->second;
        }
    }

    TaffyNodeId node = CreateNode(render_obj);
    element_to_node_[render_obj] = node;
    node_to_element_[node._0] = render_obj;

    // Add to parent in Taffy tree
    if (parent_node._0 != 0) {
        TaffyTree_AppendChild(taffy_tree_, parent_node, node);
    }

    if (!has_root_ && !parent) {
        root_node_ = node;
        has_root_ = true;
    }
}

void LayoutEngine::RemoveElement(RenderObject* render_obj) {
    auto it = element_to_node_.find(render_obj);
    if (it == element_to_node_.end() || !taffy_tree_) {
        return;
    }

    TaffyNodeId node = it->second;

    // Remove from Taffy tree
    TaffyTree_RemoveNode(taffy_tree_, node);

    node_to_element_.erase(node._0);
    element_to_node_.erase(it);

    if (has_root_ && root_node_._0 == node._0) {
        has_root_ = false;
    }
}

void LayoutEngine::Clear() {
    element_to_node_.clear();
    node_to_element_.clear();
    ifc_containers_.clear();
    has_root_ = false;

    // TODO: Clear Taffy tree
    // if (taffy_tree_) {
    //     TaffyTree_Clear(taffy_tree_);
    // }
}

bool LayoutEngine::HasElement(RenderObject* render_obj) const {
    return element_to_node_.find(render_obj) != element_to_node_.end();
}

void LayoutEngine::UpdateStylesRecursive(RenderObject* render_obj) {
    if (!render_obj) return;

    // 更新当前节点的样式
    auto it = element_to_node_.find(render_obj);
    if (it != element_to_node_.end()) {
        TaffyNodeId node = it->second;
        ApplyStyle(node, render_obj->GetComputedStyle());

        RenderObjectType type = render_obj->GetType();
        const auto& computed_style = render_obj->GetComputedStyle();

        // Skip if element already has explicit flex/grid display (e.g., form with display: flex)
        bool is_already_flex_or_grid = (computed_style.display == RenderObjectType::FLEX ||
                                        computed_style.display == RenderObjectType::GRID);

        // Skip if parent is a flex/grid container - flex items should not be auto-converted
        // because their size is determined by the flex algorithm, not by their internal layout
        bool parent_is_flex_or_grid = false;
        if (auto parent = render_obj->GetParent()) {
            auto parent_display = parent->GetComputedStyle().display;
            parent_is_flex_or_grid = (parent_display == RenderObjectType::FLEX ||
                                      parent_display == RenderObjectType::GRID);
        }

        // 对于 BLOCK 元素，检查是否需要设置为 FLEX（与 CreateNode 中的逻辑相同）
        // 这确保了在窗口 resize 时不会丢失 FLEX 设置
        // 但是：如果父元素是 flex/grid 容器，不要进行自动转换，以保持正确的 flex 子项行为
        if (type == RenderObjectType::BLOCK && !is_already_flex_or_grid && !parent_is_flex_or_grid) {
            auto& children = render_obj->GetChildren();
            bool has_inline_content = false;
            bool has_block_content = false;

            for (auto& child : children) {
                RenderObjectType child_type = child->GetType();
                if (child_type == RenderObjectType::TEXT ||
                    child_type == RenderObjectType::INLINE_BLOCK ||
                    child_type == RenderObjectType::INLINE) {
                    has_inline_content = true;
                } else if (child_type == RenderObjectType::BLOCK ||
                           child_type == RenderObjectType::FLEX ||
                           child_type == RenderObjectType::GRID) {
                    has_block_content = true;
                }
            }

            if (has_inline_content) {
                TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
                if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                    TaffyStyleMutRef taffy_style = style_result.value;
                    TaffyStyle_SetDisplay(taffy_style, TAFFY_DISPLAY_FLEX);

                    if (has_block_content) {
                        // Mixed content: use column layout (like block stacking)
                        // INLINE elements won't stretch because of align-items: flex-start
                        TaffyStyle_SetFlexDirection(taffy_style, TAFFY_FLEX_DIRECTION_COLUMN);
                        TaffyStyle_SetAlignItems(taffy_style, TAFFY_ALIGN_ITEMS_FLEX_START);
                    } else {
                        // Pure inline content: use row layout (horizontal flow)
                        // Known issue: flex-wrap causes inline-block elements to wrap to
                        // new line instead of flowing with text (Taffy doesn't support IFC)
                        TaffyStyle_SetFlexDirection(taffy_style, TAFFY_FLEX_DIRECTION_ROW);
                        TaffyStyle_SetFlexWrap(taffy_style, TAFFY_FLEX_WRAP_WRAP);
                        TaffyStyle_SetAlignItems(taffy_style, TAFFY_ALIGN_ITEMS_BASELINE);
                    }
                }
            }
        }
        // For INLINE elements, clear padding/border in Taffy because measure function already includes them
        else if (type == RenderObjectType::INLINE) {
            TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
            if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                TaffyStyleMutRef taffy_style = style_result.value;

                // Use border-box
                TaffyStyle_SetBoxSizing(taffy_style, TAFFY_BOX_SIZING_BORDER_BOX);

                // Width and height determined by measure function
                TaffyStyle_SetWidth(taffy_style, 0.0f, TAFFY_UNIT_AUTO);
                TaffyStyle_SetHeight(taffy_style, 0.0f, TAFFY_UNIT_AUTO);

                // Clear padding - measure function already includes it
                TaffyStyle_SetPaddingTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetPaddingRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetPaddingBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetPaddingLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                // Clear border - measure function already includes it
                TaffyStyle_SetBorderTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetBorderRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetBorderBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetBorderLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                // DON'T set align-self - let flex container control stretch behavior
                TaffyStyle_SetFlexGrow(taffy_style, 0.0f);
                TaffyStyle_SetFlexShrink(taffy_style, 0.0f);
            }
        }
        // For INLINE_BLOCK elements, always use measure function settings
        else if (type == RenderObjectType::INLINE_BLOCK) {
            TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
            if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                TaffyStyleMutRef taffy_style = style_result.value;

                // Use border-box
                TaffyStyle_SetBoxSizing(taffy_style, TAFFY_BOX_SIZING_BORDER_BOX);
                TaffyStyle_SetWidth(taffy_style, 0.0f, TAFFY_UNIT_AUTO);
                TaffyStyle_SetHeight(taffy_style, 0.0f, TAFFY_UNIT_AUTO);

                // Clear padding/border - measure function already includes them
                TaffyStyle_SetPaddingTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetPaddingRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetPaddingBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetPaddingLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                TaffyStyle_SetBorderTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetBorderRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetBorderBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                TaffyStyle_SetBorderLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                // Prevent stretch in cross-axis
                TaffyStyle_SetAlignSelf(taffy_style, TAFFY_ALIGN_ITEMS_FLEX_START);
            }
        }
    }

    // 递归更新子节点
    auto& children = render_obj->GetChildren();
    for (auto& child : children) {
        UpdateStylesRecursive(child.get());
    }
}

TaffyNodeId LayoutEngine::CreateNode(RenderObject* render_obj) {
    TaffyNodeId node;
    node._0 = 0;

    if (!taffy_tree_) {
        return node;
    }

    // Create Taffy node
    TaffyNodeIdResult result = TaffyTree_NewNode(taffy_tree_);
    if (result.return_code == TAFFY_RETURN_CODE_OK) {
        node = result.value;

        // Apply render object's computed style
        if (render_obj) {
            ApplyStyle(node, render_obj->GetComputedStyle());

            // For BLOCK elements, we need to handle inline content properly.
            // Taffy doesn't have true INLINE display, so we use FLEX to simulate it.
            //
            // Rules:
            // 1. If element already has display: flex/grid set via CSS, don't override
            // 2. If ALL children are inline (TEXT, INLINE, INLINE_BLOCK) -> FLEX ROW (horizontal flow)
            // 3. If mixed BLOCK and INLINE children -> FLEX COLUMN (vertical stack, but INLINE won't stretch)
            // 4. If ALL children are BLOCK -> keep as BLOCK (default)
            RenderObjectType type = render_obj->GetType();
            const auto& computed_style = render_obj->GetComputedStyle();

            // Skip if element already has explicit flex/grid display (e.g., form with display: flex)
            bool is_already_flex_or_grid = (computed_style.display == RenderObjectType::FLEX ||
                                            computed_style.display == RenderObjectType::GRID);

            // Skip if parent is a flex/grid container - flex items should not be auto-converted
            // because their size is determined by the flex algorithm, not by their internal layout
            bool parent_is_flex_or_grid = false;
            if (auto parent = render_obj->GetParent()) {
                auto parent_display = parent->GetComputedStyle().display;
                parent_is_flex_or_grid = (parent_display == RenderObjectType::FLEX ||
                                          parent_display == RenderObjectType::GRID);
            }

            if (type == RenderObjectType::BLOCK && !is_already_flex_or_grid && !parent_is_flex_or_grid) {
                auto& children = render_obj->GetChildren();
                bool has_inline_content = false;
                bool has_block_content = false;

                // Check computed display style (not RenderObjectType) to determine layout
                // This respects CSS display property set via inline styles or stylesheets
                for (auto& child : children) {
                    RenderObjectType child_type = child->GetType();
                    // Use computed style display for accurate CSS behavior
                    RenderObjectType child_display = child->GetComputedStyle().display;

                    // TEXT nodes are always inline
                    if (child_type == RenderObjectType::TEXT) {
                        has_inline_content = true;
                    }
                    // For other elements, check computed display style
                    else if (child_display == RenderObjectType::INLINE ||
                             child_display == RenderObjectType::INLINE_BLOCK) {
                        has_inline_content = true;
                    } else if (child_display == RenderObjectType::BLOCK ||
                               child_display == RenderObjectType::FLEX ||
                               child_display == RenderObjectType::GRID ||
                               child_display == RenderObjectType::TABLE) {
                        has_block_content = true;
                    }
                }

                if (has_inline_content) {
                    TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
                    if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                        TaffyStyleMutRef taffy_style = style_result.value;
                        TaffyStyle_SetDisplay(taffy_style, TAFFY_DISPLAY_FLEX);

                        // Always use row layout with wrap for inline content
                        // Block elements will force line breaks by having width: 100%
                        // This simulates browser's inline formatting context behavior
                        TaffyStyle_SetFlexDirection(taffy_style, TAFFY_FLEX_DIRECTION_ROW);
                        TaffyStyle_SetFlexWrap(taffy_style, TAFFY_FLEX_WRAP_WRAP);
                        TaffyStyle_SetAlignItems(taffy_style, TAFFY_ALIGN_ITEMS_BASELINE);

                        // Note: Setting block children width to 100% is done in BuildSubtree
                        // after all children are created, because at this point children
                        // haven't been added to element_to_node_ map yet.
                    }
                }
            }

            // For text nodes, set up measure function for dynamic text measurement
            if (render_obj->GetType() == RenderObjectType::TEXT) {
                auto* text_obj = dynamic_cast<RenderText*>(render_obj);
                if (text_obj) {
                    // Set the measure function with the RenderText object as context
                    // Taffy will call this function during layout to measure the text
                    TaffyTree_SetNodeContext(taffy_tree_, node, TextMeasureFunction, text_obj);

                    // Set text node style
                    TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
                    if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                        TaffyStyleMutRef taffy_style = style_result.value;

                        // Text nodes should use border-box (Taffy default)
                        TaffyStyle_SetBoxSizing(taffy_style, TAFFY_BOX_SIZING_BORDER_BOX);

                        // Clear any padding/margin/border for text nodes
                        TaffyStyle_SetPaddingLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                        TaffyStyle_SetMarginLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetMarginRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetMarginTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetMarginBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                        // Don't set explicit width/height - let Taffy call measure function
                        // Set width to auto so it can be determined by parent container
                        TaffyStyle_SetWidth(taffy_style, 0.0f, TAFFY_UNIT_AUTO);
                        TaffyStyle_SetHeight(taffy_style, 0.0f, TAFFY_UNIT_AUTO);
                    }
                }
            }
            // For inline-block elements, use measure function
            else if (render_obj->GetType() == RenderObjectType::INLINE_BLOCK) {
                auto* inline_block = dynamic_cast<RenderInlineBlock*>(render_obj);
                if (inline_block) {
                    // Always use measure function for INLINE_BLOCK
                    // This ensures proper height calculation for elements like input
                    TaffyTree_SetNodeContext(taffy_tree_, node, InlineBlockMeasureFunction, inline_block);

                    TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
                    if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                        TaffyStyleMutRef taffy_style = style_result.value;

                        // Use border-box for inline-block (measure function returns full size)
                        TaffyStyle_SetBoxSizing(taffy_style, TAFFY_BOX_SIZING_BORDER_BOX);

                        // Width and height will be determined by measure function
                        TaffyStyle_SetWidth(taffy_style, 0.0f, TAFFY_UNIT_AUTO);
                        TaffyStyle_SetHeight(taffy_style, 0.0f, TAFFY_UNIT_AUTO);

                        // IMPORTANT: Clear padding in Taffy - measure function already includes it
                        TaffyStyle_SetPaddingTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                        // Clear border too
                        TaffyStyle_SetBorderTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetBorderRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetBorderBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetBorderLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                        // Prevent stretch in cross-axis
                        TaffyStyle_SetAlignSelf(taffy_style, TAFFY_ALIGN_ITEMS_FLEX_START);
                    }
                }
            }
            // For inline elements (like buttons), set up measure function
            else if (render_obj->GetType() == RenderObjectType::INLINE) {
                auto* inline_elem = dynamic_cast<RenderInline*>(render_obj);
                if (inline_elem) {
                    // Set the measure function with the RenderInline object as context
                    TaffyTree_SetNodeContext(taffy_tree_, node, InlineMeasureFunction, inline_elem);

                    // Set inline node style - let measure function determine size
                    TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
                    if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                        TaffyStyleMutRef taffy_style = style_result.value;

                        // Use border-box for inline (measure function returns full size)
                        TaffyStyle_SetBoxSizing(taffy_style, TAFFY_BOX_SIZING_BORDER_BOX);

                        // Width and height will be determined by measure function
                        TaffyStyle_SetWidth(taffy_style, 0.0f, TAFFY_UNIT_AUTO);
                        TaffyStyle_SetHeight(taffy_style, 0.0f, TAFFY_UNIT_AUTO);

                        // IMPORTANT: Clear padding in Taffy - our measure function already includes padding!
                        // Otherwise Taffy will add padding again to the measure result.
                        TaffyStyle_SetPaddingTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                        // Clear border too - measure function includes border
                        TaffyStyle_SetBorderTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetBorderRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetBorderBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetBorderLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                        // DON'T set align-self - let flex container control stretch behavior
                        // Buttons in flex containers like form should stretch to match input height

                        // Don't grow or shrink in flex context (width doesn't change)
                        TaffyStyle_SetFlexGrow(taffy_style, 0.0f);
                        TaffyStyle_SetFlexShrink(taffy_style, 0.0f);
                    }
                }
            }
            // For table elements, use measure function for custom table layout
            else if (render_obj->GetType() == RenderObjectType::TABLE) {
                auto* table = dynamic_cast<RenderTable*>(render_obj);
                if (table) {
                    // Set the measure function with the RenderTable object as context
                    TaffyTree_SetNodeContext(taffy_tree_, node, TableMeasureFunction, table);

                    TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
                    if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                        TaffyStyleMutRef taffy_style = style_result.value;

                        // Use border-box for table (measure function returns full size)
                        TaffyStyle_SetBoxSizing(taffy_style, TAFFY_BOX_SIZING_BORDER_BOX);

                        // Width and height will be determined by measure function
                        TaffyStyle_SetWidth(taffy_style, 0.0f, TAFFY_UNIT_AUTO);
                        TaffyStyle_SetHeight(taffy_style, 0.0f, TAFFY_UNIT_AUTO);

                        // Clear padding in Taffy - measure function already includes it
                        TaffyStyle_SetPaddingTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetPaddingLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                        // Clear border too
                        TaffyStyle_SetBorderTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetBorderRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetBorderBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                        TaffyStyle_SetBorderLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                        // Table is a block-level element
                        TaffyStyle_SetDisplay(taffy_style, TAFFY_DISPLAY_BLOCK);
                    }
                }
            }
            // For SVG elements, set fixed width/height from SVG attributes
            // SVG elements have their dimensions defined by width/height attributes, not CSS
            else if (render_obj->IsSVGRenderObject()) {
                auto* svg_root = dynamic_cast<RenderSVGRoot*>(render_obj);
                if (svg_root) {
                    auto svg_element = svg_root->GetSVGSVGElement();
                    if (svg_element) {
                        // Get SVG dimensions from attributes
                        float svg_width = 300.0f;  // SVG default width
                        float svg_height = 150.0f; // SVG default height

                        std::string width_str = svg_element->GetWidth();
                        std::string height_str = svg_element->GetHeight();

                        if (!width_str.empty()) {
                            try {
                                svg_width = std::stof(width_str);
                            } catch (...) {}
                        }
                        if (!height_str.empty()) {
                            try {
                                svg_height = std::stof(height_str);
                            } catch (...) {}
                        }

                        // If still no dimensions, try viewBox
                        float view_min_x, view_min_y, view_width, view_height;
                        if (svg_element->ParseViewBox(view_min_x, view_min_y, view_width, view_height)) {
                            if (svg_width == 300.0f && width_str.empty()) svg_width = view_width;
                            if (svg_height == 150.0f && height_str.empty()) svg_height = view_height;
                        }

                        TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
                        if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                            TaffyStyleMutRef taffy_style = style_result.value;

                            // Set fixed width and height for SVG
                            TaffyStyle_SetWidth(taffy_style, svg_width, TAFFY_UNIT_LENGTH);
                            TaffyStyle_SetHeight(taffy_style, svg_height, TAFFY_UNIT_LENGTH);

                            // SVG is a replaced element, use block display in Taffy
                            TaffyStyle_SetDisplay(taffy_style, TAFFY_DISPLAY_BLOCK);
                        }
                    }
                }
            }
        }
    }

    return node;
}

void LayoutEngine::ApplyStyle(TaffyNodeId node, const ComputedStyle& style) {
    if (!taffy_tree_) {
        return;
    }

    // Get mutable style reference from Taffy
    TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
    if (style_result.return_code != TAFFY_RETURN_CODE_OK) {
        return;
    }

    TaffyStyleMutRef taffy_style = style_result.value;

    // Set box-sizing to content-box (CSS default)
    // Taffy defaults to border-box, but CSS defaults to content-box
    TaffyStyle_SetBoxSizing(taffy_style, TAFFY_BOX_SIZING_CONTENT_BOX);

    // Map display type
    TaffyDisplay display = TAFFY_DISPLAY_BLOCK;
    if (style.display == RenderObjectType::FLEX) {
        display = TAFFY_DISPLAY_FLEX;
    } else if (style.display == RenderObjectType::GRID) {
        display = TAFFY_DISPLAY_GRID;
    } else if (style.display == RenderObjectType::NONE) {
        display = TAFFY_DISPLAY_NONE;
    } else if (style.display == RenderObjectType::BLOCK) {
        display = TAFFY_DISPLAY_BLOCK;
    }
    TaffyStyle_SetDisplay(taffy_style, display);

    // Apply sizing
    auto apply_dimension = [](TaffyStyleMutRef style_ref, const CSSLength& css_len,
                              auto setter_func) {
        if (css_len.unit == CSSUnit::PX) {
            setter_func(style_ref, css_len.value, TAFFY_UNIT_LENGTH);
        } else if (css_len.unit == CSSUnit::PERCENT) {
            setter_func(style_ref, css_len.value, TAFFY_UNIT_PERCENT);
        } else if (css_len.unit == CSSUnit::AUTO) {
            setter_func(style_ref, 0.0f, TAFFY_UNIT_AUTO);
        } else if (css_len.unit == CSSUnit::EM) {
            // EM 单位转换为像素（假设基础字体大小为 16px）
            setter_func(style_ref, css_len.value * 16.0f, TAFFY_UNIT_LENGTH);
        } else if (css_len.unit == CSSUnit::REM) {
            // REM 单位转换为像素（假设根字体大小为 16px）
            setter_func(style_ref, css_len.value * 16.0f, TAFFY_UNIT_LENGTH);
        }
        // NONE 单位不设置任何值，保持 Taffy 的默认值（无限制）
    };

    apply_dimension(taffy_style, style.width, TaffyStyle_SetWidth);
    apply_dimension(taffy_style, style.height, TaffyStyle_SetHeight);
    apply_dimension(taffy_style, style.min_width, TaffyStyle_SetMinWidth);
    apply_dimension(taffy_style, style.max_width, TaffyStyle_SetMaxWidth);
    apply_dimension(taffy_style, style.min_height, TaffyStyle_SetMinHeight);
    apply_dimension(taffy_style, style.max_height, TaffyStyle_SetMaxHeight);

    // Apply margin
    apply_dimension(taffy_style, style.margin.top, TaffyStyle_SetMarginTop);
    apply_dimension(taffy_style, style.margin.right, TaffyStyle_SetMarginRight);
    apply_dimension(taffy_style, style.margin.bottom, TaffyStyle_SetMarginBottom);
    apply_dimension(taffy_style, style.margin.left, TaffyStyle_SetMarginLeft);

    // Apply padding
    apply_dimension(taffy_style, style.padding.top, TaffyStyle_SetPaddingTop);
    apply_dimension(taffy_style, style.padding.right, TaffyStyle_SetPaddingRight);
    apply_dimension(taffy_style, style.padding.bottom, TaffyStyle_SetPaddingBottom);
    apply_dimension(taffy_style, style.padding.left, TaffyStyle_SetPaddingLeft);

    // Apply border (convert to length)
    TaffyStyle_SetBorderTop(taffy_style, style.border_top_width, TAFFY_UNIT_LENGTH);
    TaffyStyle_SetBorderRight(taffy_style, style.border_right_width, TAFFY_UNIT_LENGTH);
    TaffyStyle_SetBorderBottom(taffy_style, style.border_bottom_width, TAFFY_UNIT_LENGTH);
    TaffyStyle_SetBorderLeft(taffy_style, style.border_left_width, TAFFY_UNIT_LENGTH);

    // Apply Flexbox properties
    if (style.display == RenderObjectType::FLEX) {
        // Flex direction
        TaffyFlexDirection flex_dir = TAFFY_FLEX_DIRECTION_ROW;
        if (style.flex_direction == "row") flex_dir = TAFFY_FLEX_DIRECTION_ROW;
        else if (style.flex_direction == "row-reverse") flex_dir = TAFFY_FLEX_DIRECTION_ROW_REVERSE;
        else if (style.flex_direction == "column") flex_dir = TAFFY_FLEX_DIRECTION_COLUMN;
        else if (style.flex_direction == "column-reverse") flex_dir = TAFFY_FLEX_DIRECTION_COLUMN_REVERSE;
        TaffyStyle_SetFlexDirection(taffy_style, flex_dir);

        // Flex wrap
        TaffyFlexWrap flex_wrap = TAFFY_FLEX_WRAP_NO_WRAP;
        if (style.flex_wrap == "nowrap") flex_wrap = TAFFY_FLEX_WRAP_NO_WRAP;
        else if (style.flex_wrap == "wrap") flex_wrap = TAFFY_FLEX_WRAP_WRAP;
        else if (style.flex_wrap == "wrap-reverse") flex_wrap = TAFFY_FLEX_WRAP_WRAP_REVERSE;
        TaffyStyle_SetFlexWrap(taffy_style, flex_wrap);

        // Justify content
        TaffyAlignContent justify = TAFFY_ALIGN_CONTENT_FLEX_START;
        if (style.justify_content == "flex-start") justify = TAFFY_ALIGN_CONTENT_FLEX_START;
        else if (style.justify_content == "flex-end") justify = TAFFY_ALIGN_CONTENT_FLEX_END;
        else if (style.justify_content == "center") justify = TAFFY_ALIGN_CONTENT_CENTER;
        else if (style.justify_content == "space-between") justify = TAFFY_ALIGN_CONTENT_SPACE_BETWEEN;
        else if (style.justify_content == "space-around") justify = TAFFY_ALIGN_CONTENT_SPACE_AROUND;
        else if (style.justify_content == "space-evenly") justify = TAFFY_ALIGN_CONTENT_SPACE_EVENLY;
        TaffyStyle_SetJustifyContent(taffy_style, justify);

        // Align items (CSS 规范默认值是 normal)
        TaffyAlignItems align = TAFFY_ALIGN_ITEMS_NORMAL;
        if (style.align_items == "normal") align = TAFFY_ALIGN_ITEMS_NORMAL;
        else if (style.align_items == "flex-start") align = TAFFY_ALIGN_ITEMS_FLEX_START;
        else if (style.align_items == "flex-end") align = TAFFY_ALIGN_ITEMS_FLEX_END;
        else if (style.align_items == "center") align = TAFFY_ALIGN_ITEMS_CENTER;
        else if (style.align_items == "baseline") align = TAFFY_ALIGN_ITEMS_BASELINE;
        else if (style.align_items == "stretch") align = TAFFY_ALIGN_ITEMS_STRETCH;
        else if (style.align_items == "start") align = TAFFY_ALIGN_ITEMS_START;
        else if (style.align_items == "end") align = TAFFY_ALIGN_ITEMS_END;
        TaffyStyle_SetAlignItems(taffy_style, align);

        // Align content (CSS 规范默认值是 normal)
        TaffyAlignContent align_content = TAFFY_ALIGN_CONTENT_NORMAL;
        if (style.align_content == "normal") align_content = TAFFY_ALIGN_CONTENT_NORMAL;
        else if (style.align_content == "flex-start") align_content = TAFFY_ALIGN_CONTENT_FLEX_START;
        else if (style.align_content == "flex-end") align_content = TAFFY_ALIGN_CONTENT_FLEX_END;
        else if (style.align_content == "center") align_content = TAFFY_ALIGN_CONTENT_CENTER;
        else if (style.align_content == "space-between") align_content = TAFFY_ALIGN_CONTENT_SPACE_BETWEEN;
        else if (style.align_content == "space-around") align_content = TAFFY_ALIGN_CONTENT_SPACE_AROUND;
        else if (style.align_content == "space-evenly") align_content = TAFFY_ALIGN_CONTENT_SPACE_EVENLY;
        else if (style.align_content == "stretch") align_content = TAFFY_ALIGN_CONTENT_STRETCH;
        else if (style.align_content == "start") align_content = TAFFY_ALIGN_CONTENT_START;
        else if (style.align_content == "end") align_content = TAFFY_ALIGN_CONTENT_END;
        TaffyStyle_SetAlignContent(taffy_style, align_content);

        // Gap (for flex container)
        apply_dimension(taffy_style, style.column_gap, TaffyStyle_SetColumnGap);
        apply_dimension(taffy_style, style.row_gap, TaffyStyle_SetRowGap);
    }

    // Flex item properties (apply to all elements, they become effective when parent is flex)
    // 这些属性对所有元素生效，当父元素是 flex 容器时会被使用
    TaffyStyle_SetFlexGrow(taffy_style, style.flex_grow);
    TaffyStyle_SetFlexShrink(taffy_style, style.flex_shrink);
    apply_dimension(taffy_style, style.flex_basis, TaffyStyle_SetFlexBasis);

    // Apply CSS Grid properties
    if (style.display == RenderObjectType::GRID) {
        // Grid auto flow
        TaffyGridAutoFlow grid_auto_flow = TAFFY_GRID_AUTO_FLOW_ROW;
        if (style.grid_auto_flow == "row") grid_auto_flow = TAFFY_GRID_AUTO_FLOW_ROW;
        else if (style.grid_auto_flow == "column") grid_auto_flow = TAFFY_GRID_AUTO_FLOW_COLUMN;
        else if (style.grid_auto_flow == "row dense") grid_auto_flow = TAFFY_GRID_AUTO_FLOW_ROW_DENSE;
        else if (style.grid_auto_flow == "column dense") grid_auto_flow = TAFFY_GRID_AUTO_FLOW_COLUMN_DENSE;
        TaffyStyle_SetGridAutoFlow(taffy_style, grid_auto_flow);

        // Gap (Grid uses the same gap properties as Flexbox)
        apply_dimension(taffy_style, style.column_gap, TaffyStyle_SetColumnGap);
        apply_dimension(taffy_style, style.row_gap, TaffyStyle_SetRowGap);

        // Grid template columns
        if (!style.grid_template_columns.empty()) {
            std::vector<TaffyGridTrack> columns = ParseGridTemplate(style.grid_template_columns);
            if (!columns.empty()) {
                TaffyStyle_SetGridTemplateColumns(taffy_style, columns.data(), columns.size());
            }
        }

        // Grid template rows
        if (!style.grid_template_rows.empty()) {
            std::vector<TaffyGridTrack> rows = ParseGridTemplate(style.grid_template_rows);
            if (!rows.empty()) {
                TaffyStyle_SetGridTemplateRows(taffy_style, rows.data(), rows.size());
            }
        }
    }

    // Apply Grid item placement (for children of grid containers)
    if (!style.grid_column.empty()) {
        // Parse grid-column: "span 2", "1 / 3", etc.
        TaffyGridPlacement column_placement = ParseGridPlacement(style.grid_column);
        TaffyStyle_SetGridColumn(taffy_style, column_placement);
    }

    if (!style.grid_row.empty()) {
        // Parse grid-row: "span 2", "1 / 2", etc.
        TaffyGridPlacement row_placement = ParseGridPlacement(style.grid_row);
        TaffyStyle_SetGridRow(taffy_style, row_placement);
    }

    // Apply Position property
    TaffyPosition position = TAFFY_POSITION_RELATIVE;
    if (style.position == "relative") {
        position = TAFFY_POSITION_RELATIVE;
    } else if (style.position == "absolute") {
        position = TAFFY_POSITION_ABSOLUTE;
    }
    // Note: CSS "fixed" and "sticky" are not supported by Taffy, they will be treated as absolute
    else if (style.position == "fixed" || style.position == "sticky") {
        position = TAFFY_POSITION_ABSOLUTE;
    }
    TaffyStyle_SetPosition(taffy_style, position);

    // Apply Inset properties (top, right, bottom, left)
    apply_dimension(taffy_style, style.top, TaffyStyle_SetInsetTop);
    apply_dimension(taffy_style, style.right, TaffyStyle_SetInsetRight);
    apply_dimension(taffy_style, style.bottom, TaffyStyle_SetInsetBottom);
    apply_dimension(taffy_style, style.left, TaffyStyle_SetInsetLeft);

    // Apply Overflow properties
    // 辅助函数：将 overflow 字符串转换为 TaffyOverflow
    auto parseOverflow = [](const std::string& value) -> TaffyOverflow {
        if (value == "hidden") {
            return TAFFY_OVERFLOW_HIDDEN;
        } else if (value == "scroll") {
            return TAFFY_OVERFLOW_SCROLL;
        } else if (value == "auto") {
            return TAFFY_OVERFLOW_SCROLL;  // Taffy treats auto as scroll
        } else if (value == "clip") {
            return TAFFY_OVERFLOW_HIDDEN;  // clip not supported, treat as hidden
        }
        return TAFFY_OVERFLOW_VISIBLE;
    };

    // 优先使用 overflow_x/overflow_y，如果没有设置则使用 overflow 简写属性
    std::string overflow_x_val = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y_val = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

    TaffyOverflow overflow_x = parseOverflow(overflow_x_val);
    TaffyOverflow overflow_y = parseOverflow(overflow_y_val);

    TaffyStyle_SetOverflowX(taffy_style, overflow_x);
    TaffyStyle_SetOverflowY(taffy_style, overflow_y);

    // 设置 scrollbar_width，否则 Overflow::Scroll 会表现得和 Hidden 一样
    // 滚动条宽度需要与 RenderObject::GetScrollbarWidth() 保持一致
    bool needs_scrollbar = (overflow_x_val == "scroll" || overflow_x_val == "auto" ||
                            overflow_y_val == "scroll" || overflow_y_val == "auto");
    if (needs_scrollbar) {
        TaffyStyle_SetScrollbarWidth(taffy_style, 12.0f);
    }

    // Note: z-index is not handled by Taffy layout engine
    // It should be handled by the rendering layer during paint
}

void LayoutEngine::SyncChildren(RenderObject* render_obj, TaffyNodeId node) {
    if (!render_obj || !taffy_tree_) {
        return;
    }

    // Get children from render object
    auto& children = render_obj->GetChildren();
    for (auto& child : children) {
        // Create or get Taffy node for child
        TaffyNodeId child_taffy_node;
        auto it = element_to_node_.find(child.get());
        if (it == element_to_node_.end()) {
            child_taffy_node = CreateNode(child.get());
            element_to_node_[child.get()] = child_taffy_node;
            node_to_element_[child_taffy_node._0] = child.get();
        } else {
            child_taffy_node = it->second;
        }

        // Add as child to parent node
        TaffyTree_AppendChild(taffy_tree_, node, child_taffy_node);

        // Recursively sync
        SyncChildren(child.get(), child_taffy_node);
    }
}

void LayoutEngine::BuildSubtree(RenderObject* render_obj, TaffyNodeId parent_node) {
    if (!render_obj || !taffy_tree_) {
        return;
    }

    // Create node for this render object
    TaffyNodeId node = CreateNode(render_obj);
    element_to_node_[render_obj] = node;
    node_to_element_[node._0] = render_obj;

    // Set as root if no parent
    if (parent_node._0 == 0 && !has_root_) {
        root_node_ = node;
        has_root_ = true;
    }

    // Add to parent in Taffy tree
    if (parent_node._0 != 0) {
        TaffyTree_AppendChild(taffy_tree_, parent_node, node);
    }

    // For INLINE_BLOCK and INLINE elements, we use measure functions to determine their size.
    // Taffy only calls measure functions for leaf nodes (nodes without children in Taffy tree).
    // So we don't add children to Taffy for these elements - they will layout their own children.
    RenderObjectType type = render_obj->GetType();
    if (type == RenderObjectType::INLINE_BLOCK || type == RenderObjectType::INLINE) {
        // Don't add children to Taffy tree - these elements manage their own children layout
        return;
    }

    // For TABLE elements, they manage their own layout (rows, cells, etc.)
    // Don't add children to Taffy tree - table elements use custom layout algorithm
    if (type == RenderObjectType::TABLE ||
        type == RenderObjectType::TABLE_ROW_GROUP ||
        type == RenderObjectType::TABLE_HEADER_GROUP ||
        type == RenderObjectType::TABLE_FOOTER_GROUP ||
        type == RenderObjectType::TABLE_ROW ||
        type == RenderObjectType::TABLE_CELL ||
        type == RenderObjectType::TABLE_CAPTION) {
        // Table elements manage their own children layout
        return;
    }

    // For SVG elements, they manage their own layout using SVG coordinate system
    // Don't add children to Taffy tree - SVG elements use custom layout
    if (render_obj->IsSVGRenderObject()) {
        // SVG elements manage their own children layout
        return;
    }

    // Check if this BLOCK element has only inline content (use IFC)
    auto& children = render_obj->GetChildren();
    const auto& computed_style = render_obj->GetComputedStyle();
    bool is_already_flex_or_grid = (computed_style.display == RenderObjectType::FLEX ||
                                    computed_style.display == RenderObjectType::GRID);

    if (type == RenderObjectType::BLOCK && !is_already_flex_or_grid && !children.empty()) {
        bool has_inline_content = false;
        bool has_block_content = false;

        // Check computed display style to determine layout
        for (auto& child : children) {
            RenderObjectType child_type = child->GetType();
            RenderObjectType child_display = child->GetComputedStyle().display;

            if (child_type == RenderObjectType::TEXT) {
                has_inline_content = true;
            } else if (child_display == RenderObjectType::INLINE ||
                       child_display == RenderObjectType::INLINE_BLOCK) {
                has_inline_content = true;
            } else if (child_display == RenderObjectType::BLOCK ||
                       child_display == RenderObjectType::FLEX ||
                       child_display == RenderObjectType::GRID ||
                       child_display == RenderObjectType::TABLE) {
                has_block_content = true;
            }
        }

        // If we have ONLY inline content, use IFC for layout
        // Don't add children to Taffy tree - IFC will handle them
        if (has_inline_content && !has_block_content) {
            ifc_containers_.insert(render_obj);
            // Don't add children to Taffy - IFC will layout them
            return;
        }

        // If we have mixed content (inline + block), still use Taffy
        // but set block children to 100% width after building subtree
    }

    // Recursively build children
    for (auto& child : children) {
        BuildSubtree(child.get(), node);
    }

    // After all children are created, set block-level children width to 100%
    // This must be done here because children are now in element_to_node_ map
    if (type == RenderObjectType::BLOCK && !is_already_flex_or_grid) {
        bool has_inline_content = false;
        bool has_block_content = false;

        // Check computed display style to determine layout
        for (auto& child : children) {
            RenderObjectType child_type = child->GetType();
            RenderObjectType child_display = child->GetComputedStyle().display;

            if (child_type == RenderObjectType::TEXT) {
                has_inline_content = true;
            } else if (child_display == RenderObjectType::INLINE ||
                       child_display == RenderObjectType::INLINE_BLOCK) {
                has_inline_content = true;
            } else if (child_display == RenderObjectType::BLOCK ||
                       child_display == RenderObjectType::FLEX ||
                       child_display == RenderObjectType::GRID ||
                       child_display == RenderObjectType::TABLE) {
                has_block_content = true;
            }
        }

        // If we have mixed content (inline + block), set block children to 100% width
        if (has_inline_content && has_block_content) {
            for (auto& child : children) {
                RenderObjectType child_display = child->GetComputedStyle().display;
                if (child_display == RenderObjectType::BLOCK ||
                    child_display == RenderObjectType::FLEX ||
                    child_display == RenderObjectType::GRID ||
                    child_display == RenderObjectType::TABLE) {
                    auto child_it = element_to_node_.find(child.get());
                    if (child_it != element_to_node_.end()) {
                        TaffyNodeId child_node = child_it->second;
                        TaffyStyleMutRefResult child_style_result = TaffyTree_GetStyleMut(taffy_tree_, child_node);
                        if (child_style_result.return_code == TAFFY_RETURN_CODE_OK) {
                            TaffyStyleMutRef child_taffy_style = child_style_result.value;
                            // Set width to 100% to force line break (like normal block behavior)
                            TaffyStyle_SetWidth(child_taffy_style, 100.0f, TAFFY_UNIT_PERCENT);
                        }
                    }
                }
            }
        }
    }
}

void LayoutEngine::ReadLayoutResults(RenderObject* render_obj) {
    if (!render_obj || !taffy_tree_) {
        return;
    }

    auto it = element_to_node_.find(render_obj);
    if (it == element_to_node_.end()) {
        return;
    }

    // Read layout from Taffy
    TaffyResult_TaffyLayout result = TaffyTree_GetLayout(taffy_tree_, it->second);
    if (result.return_code != TAFFY_RETURN_CODE_OK) {
        return;
    }

    // Update render object with layout info
    LayoutInfo& info = render_obj->GetLayoutInfo();
    info.x = result.value.x;
    info.y = result.value.y;

    const auto& style = render_obj->GetComputedStyle();
    RenderObjectType type = render_obj->GetType();



    // For TABLE elements, don't overwrite width/height from Taffy
    // because the table has already calculated its own size in TableMeasureFunction
    if (type != RenderObjectType::TABLE && !render_obj->IsSVGRenderObject()) {
        info.width = result.value.width;
        info.height = result.value.height;
    }

    // For SVG elements, call their Layout method to set correct width/height
    // They need to read width/height attributes from their DOM element
    if (render_obj->IsSVGRenderObject()) {
        render_obj->Layout(result.value.width, result.value.height);
    }

    info.is_laid_out = true;

    // Check if this is an IFC container (BLOCK with only inline content)
    if (ifc_containers_.find(render_obj) != ifc_containers_.end()) {
        // Use IFC to layout inline content
        float content_width = info.width;
        float content_height = info.height;

        // Get padding
        float padding_left = style.padding.left.ToPx(info.width, style.font_size);
        float padding_right = style.padding.right.ToPx(info.width, style.font_size);
        float padding_top = style.padding.top.ToPx(info.height, style.font_size);
        float padding_bottom = style.padding.bottom.ToPx(info.height, style.font_size);

        // Get border widths
        float border_left = style.border_left_width;
        float border_right = style.border_right_width;
        float border_top = style.border_top_width;
        float border_bottom = style.border_bottom_width;
        if (border_left == 0 && border_right == 0 && border_top == 0 && border_bottom == 0) {
            float border_width = style.border.width.ToPx(info.width, style.font_size);
            border_left = border_right = border_top = border_bottom = border_width;
        }

        // Calculate available width for IFC
        float available_width = content_width - padding_left - padding_right - border_left - border_right;

        // Perform IFC layout
        IFCLayoutResult ifc_result = ifc_layout_.Layout(render_obj, available_width);

        // Apply IFC layout results to children
        // The IFC layout positions are relative to the content area
        float offset_x = padding_left + border_left;
        float offset_y = padding_top + border_top;

        // Update child positions based on IFC results
        auto& children = render_obj->GetChildren();
        for (auto& child : children) {
            LayoutInfo& child_info = child->GetLayoutInfo();
            child_info.x += offset_x;
            child_info.y += offset_y;
            child_info.is_laid_out = true;
        }

        // Update container height if auto
        if (style.height.unit == CSSUnit::AUTO) {
            float required_height = ifc_result.total_height + padding_top + padding_bottom + border_top + border_bottom;
            if (required_height > info.height) {
                info.height = required_height;
            }
        }

        // Don't recurse into children - IFC already handled them
        return;
    }

    // For INLINE_BLOCK and INLINE elements, their children are not in Taffy tree.
    // We need to layout their children manually.
    // Note: We already have info.width and info.height from Taffy, so we just need
    // to position the children within this element.
    if (type == RenderObjectType::INLINE_BLOCK || type == RenderObjectType::INLINE) {
        // Position children within this element
        // Get padding
        float padding_left = style.padding.left.ToPx(info.width, style.font_size);
        float padding_right = style.padding.right.ToPx(info.width, style.font_size);
        float padding_top = style.padding.top.ToPx(info.height, style.font_size);
        float padding_bottom = style.padding.bottom.ToPx(info.height, style.font_size);

        // Get border widths
        float border_left = style.border_left_width;
        float border_right = style.border_right_width;
        float border_top = style.border_top_width;
        float border_bottom = style.border_bottom_width;
        if (border_left == 0 && border_right == 0 && border_top == 0 && border_bottom == 0) {
            float border_width = style.border.width.ToPx(info.width, style.font_size);
            border_left = border_right = border_top = border_bottom = border_width;
        }

        // Layout each child and position them
        // Content width/height should exclude border
        float content_width = info.width - padding_left - padding_right - border_left - border_right;
        float total_child_width = 0;
        float max_child_height = 0;

        auto& children = render_obj->GetChildren();
        for (auto& child : children) {
            child->Layout(info.width, info.height);
            auto& child_layout = child->GetLayoutInfo();
            total_child_width += child_layout.width;
            max_child_height = std::max(max_child_height, child_layout.height);
        }

        // Calculate starting x position based on text-align
        float start_x = padding_left + border_left;
        if (style.text_align == "center" && total_child_width < content_width) {
            start_x = padding_left + border_left + (content_width - total_child_width) / 2.0f;
        } else if (style.text_align == "right" && total_child_width < content_width) {
            start_x = padding_left + border_left + content_width - total_child_width;
        }

        // Position children
        float current_x = start_x;
        float content_height = info.height - padding_top - padding_bottom - border_top - border_bottom;

        for (auto& child : children) {
            auto& child_layout = child->GetLayoutInfo();
            child_layout.x = current_x;
            // Vertically center children (add border_top to offset)
            child_layout.y = padding_top + border_top + (content_height - child_layout.height) / 2.0f;
            current_x += child_layout.width;
        }

        // Don't recurse into children - we already handled them
        return;
    }

    // For SVG elements, their children are not in Taffy tree.
    // The SVG element's Layout() already handles child layout, so don't recurse.
    if (render_obj->IsSVGRenderObject()) {
        return;
    }

    // Recursively read layout for children
    auto& children = render_obj->GetChildren();
    for (auto& child : children) {
        ReadLayoutResults(child.get());
    }

    // Fix height for elements with auto height
    // Taffy sometimes doesn't correctly calculate content-based height
    // BUT: Don't modify height for elements with overflow - they should respect max-height
    bool has_overflow = (style.overflow_y == "scroll" || style.overflow_y == "auto" ||
                         style.overflow_y == "hidden" || style.overflow == "scroll" ||
                         style.overflow == "auto" || style.overflow == "hidden");

    if (style.height.unit == CSSUnit::AUTO && !children.empty() && !has_overflow) {
        // Calculate the required height based on children
        float padding_top = style.padding.top.ToPx(info.width, style.font_size);
        float padding_bottom = style.padding.bottom.ToPx(info.width, style.font_size);

        // First, fix any children with negative y coordinates
        for (auto& child : children) {
            LayoutInfo& child_info = child->GetLayoutInfo();
            if (child_info.y < 0) {
                // Move child to padding_top position
                child_info.y = padding_top;
            }
        }

        // Now calculate max_child_bottom, using the child's LAYOUT height (not content height)
        // For children with overflow, we use their constrained layout height
        float max_child_bottom = 0;
        for (auto& child : children) {
            const LayoutInfo& child_info = child->GetLayoutInfo();
            float child_bottom = child_info.y + child_info.height;
            max_child_bottom = std::max(max_child_bottom, child_bottom);
        }

        // Calculate required height: content + padding_bottom
        float required_height = max_child_bottom + padding_bottom;

        // Only adjust if the calculated height is larger than Taffy's result
        if (required_height > info.height) {
            info.height = required_height;
        }
    }

    // Fix position for absolute children with bottom property
    // After children heights have been corrected, we need to recalculate y position
    // for absolute elements that use bottom positioning
    if (style.position == "relative" && !children.empty()) {
        for (auto& child : children) {
            const auto& child_style = child->GetComputedStyle();
            if (child_style.position == "absolute" &&
                child_style.bottom.unit == CSSUnit::PX &&
                child_style.top.unit == CSSUnit::AUTO) {

                LayoutInfo& child_info = child->GetLayoutInfo();
                float bottom_offset = child_style.bottom.value;

                // Calculate new y position: parent_height - bottom - child_height
                float new_y = info.height - bottom_offset - child_info.height;
                child_info.y = new_y;
            }
        }
    }

    // Apply text-align to children (Taffy doesn't support text-align CSS property)
    if (!children.empty() && (style.text_align == "center" || style.text_align == "right")) {
        // Calculate content width (excluding padding)
        float padding_left = style.padding.left.ToPx(info.width, style.font_size);
        float padding_right = style.padding.right.ToPx(info.width, style.font_size);
        float content_width = info.width - padding_left - padding_right;

        for (auto& child : children) {
            LayoutInfo& child_info = child->GetLayoutInfo();

            // For text nodes, use actual measured text width instead of layout width
            // (Taffy may stretch text nodes to fill container)
            float child_width = child_info.width;
            if (child->GetType() == RenderObjectType::TEXT) {
                auto* text_obj = dynamic_cast<RenderText*>(child.get());
                if (text_obj && text_obj->GetActualTextWidth() > 0) {
                    child_width = text_obj->GetActualTextWidth();
                }
            }

            float old_x = child_info.x;

            if (style.text_align == "center") {
                // Center align: move child to center
                float offset = (content_width - child_width) / 2.0f;
                child_info.x = padding_left + offset;
            } else if (style.text_align == "right") {
                // Right align: move child to right
                child_info.x = padding_left + content_width - child_width;
            }
            (void)old_x;  // suppress unused warning
        }
    }
}

TaffyGridPlacement LayoutEngine::ParseGridPlacement(const std::string& value) {
    TaffyGridPlacement placement;
    placement.start = 0;
    placement.end = 0;
    placement.span = 0;

    if (value.empty()) {
        return placement;
    }

    // Parse "span N" format
    if (value.find("span") != std::string::npos) {
        size_t span_pos = value.find("span");
        std::string span_str = value.substr(span_pos + 4);

        // Trim whitespace
        size_t first = span_str.find_first_not_of(" \t");
        if (first != std::string::npos) {
            span_str = span_str.substr(first);
            size_t last = span_str.find_last_not_of(" \t");
            span_str = span_str.substr(0, last + 1);

            try {
                placement.span = static_cast<uint16_t>(std::stoi(span_str));
            } catch (...) {
                placement.span = 1;
            }
        }
        return placement;
    }

    // Parse "start / end" format
    size_t slash_pos = value.find('/');
    if (slash_pos != std::string::npos) {
        std::string start_str = value.substr(0, slash_pos);
        std::string end_str = value.substr(slash_pos + 1);

        // Trim whitespace
        size_t first = start_str.find_first_not_of(" \t");
        if (first != std::string::npos) {
            start_str = start_str.substr(first);
            size_t last = start_str.find_last_not_of(" \t");
            start_str = start_str.substr(0, last + 1);
        }

        first = end_str.find_first_not_of(" \t");
        if (first != std::string::npos) {
            end_str = end_str.substr(first);
            size_t last = end_str.find_last_not_of(" \t");
            end_str = end_str.substr(0, last + 1);
        }

        try {
            placement.start = static_cast<int16_t>(std::stoi(start_str));
            placement.end = static_cast<int16_t>(std::stoi(end_str));
        } catch (...) {
            // Invalid format, use defaults
        }
        return placement;
    }

    // Parse single number (start line)
    try {
        placement.start = static_cast<int16_t>(std::stoi(value));
    } catch (...) {
        // Invalid format, use defaults
    }

    return placement;
}

std::vector<TaffyGridTrack> LayoutEngine::ParseGridTemplate(const std::string& value) {
    std::vector<TaffyGridTrack> tracks;

    if (value.empty()) {
        return tracks;
    }

    // Split by whitespace to get individual track definitions
    std::istringstream iss(value);
    std::string token;

    while (iss >> token) {
        TaffyGridTrack track;
        track.unit = TAFFY_UNIT_AUTO;
        track.value = 0.0f;

        // Check for "auto"
        if (token == "auto") {
            track.unit = TAFFY_UNIT_AUTO;
            track.value = 0.0f;
        }
        // Check for "min-content"
        else if (token == "min-content") {
            track.unit = TAFFY_UNIT_MIN_CONTENT;
            track.value = 0.0f;
        }
        // Check for "max-content"
        else if (token == "max-content") {
            track.unit = TAFFY_UNIT_MAX_CONTENT;
            track.value = 0.0f;
        }
        // Check for fr units (e.g., "1fr", "2.5fr")
        else if (token.find("fr") != std::string::npos) {
            try {
                float fr_value = std::stof(token.substr(0, token.find("fr")));
                track.unit = TAFFY_UNIT_FR;
                track.value = fr_value;
            } catch (...) {
                track.unit = TAFFY_UNIT_FR;
                track.value = 1.0f;
            }
        }
        // Check for percentage (e.g., "50%", "33.33%")
        else if (token.find('%') != std::string::npos) {
            try {
                float percent_value = std::stof(token.substr(0, token.find('%')));
                track.unit = TAFFY_UNIT_PERCENT;
                track.value = percent_value / 100.0f;  // Convert to 0.0-1.0 range
            } catch (...) {
                track.unit = TAFFY_UNIT_PERCENT;
                track.value = 0.0f;
            }
        }
        // Check for pixel values (e.g., "100px", "200px")
        else if (token.find("px") != std::string::npos) {
            try {
                float px_value = std::stof(token.substr(0, token.find("px")));
                track.unit = TAFFY_UNIT_LENGTH;
                track.value = px_value;
            } catch (...) {
                track.unit = TAFFY_UNIT_LENGTH;
                track.value = 0.0f;
            }
        }
        // Check for fit-content() function
        else if (token.find("fit-content(") != std::string::npos) {
            size_t start = token.find('(') + 1;
            size_t end = token.find(')');
            if (end != std::string::npos) {
                std::string arg = token.substr(start, end - start);

                if (arg.find('%') != std::string::npos) {
                    try {
                        float percent_value = std::stof(arg.substr(0, arg.find('%')));
                        track.unit = TAFFY_UNIT_FIT_CONTENT_PERCENT;
                        track.value = percent_value / 100.0f;
                    } catch (...) {
                        track.unit = TAFFY_UNIT_AUTO;
                        track.value = 0.0f;
                    }
                } else if (arg.find("px") != std::string::npos) {
                    try {
                        float px_value = std::stof(arg.substr(0, arg.find("px")));
                        track.unit = TAFFY_UNIT_FIT_CONTENT_PX;
                        track.value = px_value;
                    } catch (...) {
                        track.unit = TAFFY_UNIT_AUTO;
                        track.value = 0.0f;
                    }
                }
            }
        }
        // If we couldn't parse it, default to auto
        else {
            track.unit = TAFFY_UNIT_AUTO;
            track.value = 0.0f;
        }

        tracks.push_back(track);
    }

    return tracks;
}

} // namespace lightui
