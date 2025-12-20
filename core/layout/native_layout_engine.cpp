/**
 * @file native_layout_engine.cpp
 * @brief Native layout engine implementation
 */

#include "native_layout_engine.h"
#include "content_version.h"
#include "ifc/ifc_layout.h"
#include "ifc/line_breaker.h"
#include "ifc/vertical_aligner.h"
#include "../dom/element.h"
#include "../render/render_object.h"
#include "../render/render_inline_block.h"
#include "../render/render_svg.h"
#include "../render/text_renderer.h"
#include "../render/text/font_manager.h"
#include "util/resolve.h"
#include "util/math.h"
#include "block_layout.h"
#include "flex_layout.h"
#include "grid/grid.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace lightui {

//------------------------------------------------------------------------------
// Helper Functions (must be before CreateNode)
//------------------------------------------------------------------------------

// Helper to get browser-style line-height: normal
// This matches the lookup table in ifc_layout.cpp
// 使用 Arial 字体的 line-height: normal 值（比率约 1.156）
static float GetBrowserNormalLineHeight(float font_size, const std::string& font_family) {
    // 检查是否是等宽字体（monospace）
    bool is_monospace = (font_family == "Courier New" || font_family == "Consolas" ||
                         font_family == "monospace" || font_family == "Courier" ||
                         font_family == "Monaco" || font_family == "Menlo");

    int font_size_int = static_cast<int>(font_size + 0.5f);

    if (is_monospace) {
        // Courier New 等宽字体的 line-height: normal 查找表
        // 比率约为 1.156（与 Arial 相同）
        switch (font_size_int) {
            case 13: return 15.0f;  // 13px 字体
            case 16: return 18.5f;  // 16px 字体 (从浏览器测量)
            default:
                float result = font_size * 1.156f;
                return std::round(result * 2.0f) / 2.0f;
        }
    }

    // Arial 字体的 line-height: normal 查找表
    switch (font_size_int) {
        case 10: return 11.5f;   // ~1.15
        case 11: return 13.0f;   // ~1.18
        case 12: return 14.0f;   // ~1.17
        case 13: return 15.0f;   // ~1.15
        case 14: return 16.0f;   // ~1.14
        case 15: return 17.5f;   // ~1.17
        case 16: return 18.5f;   // ~1.156 (从浏览器测量)
        case 17: return 19.5f;   // ~1.15
        case 18: return 21.0f;   // ~1.17
        case 19: return 22.0f;   // ~1.16
        case 20: return 23.0f;   // ~1.15
        case 22: return 25.5f;   // ~1.16
        case 24: return 28.0f;   // ~1.17
        case 32: return 37.0f;   // h1 (32px -> 37px)
        default:
            float result = font_size * 1.156f;
            return std::round(result * 2.0f) / 2.0f;
    }
}

// Helper to convert CSSLength to LengthPercentage
static LengthPercentage ConvertLength(const CSSLength& css_length) {
    switch (css_length.unit) {
        case CSSUnit::PX:
            return LengthPercentage::Length(css_length.value);
        case CSSUnit::PERCENT:
            return LengthPercentage::Percent(css_length.value / 100.0f);
        case CSSUnit::EM:
            return LengthPercentage::Length(css_length.value * 16.0f);
        case CSSUnit::REM:
            return LengthPercentage::Length(css_length.value * 16.0f);
        case CSSUnit::VW: {
            // 视口宽度单位
            float vw = ViewportSize::GetWidth();
            return LengthPercentage::Length(css_length.value * vw / 100.0f);
        }
        case CSSUnit::VH: {
            // 视口高度单位
            float vh = ViewportSize::GetHeight();
            return LengthPercentage::Length(css_length.value * vh / 100.0f);
        }
        case CSSUnit::VMIN: {
            // 视口最小尺寸单位
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            float vmin = std::min(vw, vh);
            return LengthPercentage::Length(css_length.value * vmin / 100.0f);
        }
        case CSSUnit::VMAX: {
            // 视口最大尺寸单位
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            float vmax = std::max(vw, vh);
            return LengthPercentage::Length(css_length.value * vmax / 100.0f);
        }
        case CSSUnit::AUTO:
        case CSSUnit::NONE:
        default:
            return LengthPercentage::Zero();
    }
}

// Forward declaration for recursive call
static std::vector<TrackSizingFunction> ParseGridTemplate(const std::string& template_str);

// Helper to parse a single grid track value
static NonRepeatedTrackSizingFunction ParseGridTrackValue(const std::string& value) {
    std::string trimmed = value;
    size_t start = trimmed.find_first_not_of(" \t");
    size_t end = trimmed.find_last_not_of(" \t");
    if (start == std::string::npos) {
        return NonRepeatedTrackSizingFunction::Auto();
    }
    trimmed = trimmed.substr(start, end - start + 1);

    if (trimmed == "auto") {
        return NonRepeatedTrackSizingFunction::Auto();
    }

    if (trimmed.size() > 2 && trimmed.substr(trimmed.size() - 2) == "fr") {
        try {
            float fr = std::stof(trimmed.substr(0, trimmed.size() - 2));
            return NonRepeatedTrackSizingFunction::Flex(fr);
        } catch (...) {
            return NonRepeatedTrackSizingFunction::Auto();
        }
    }

    if (trimmed.size() > 2 && trimmed.substr(trimmed.size() - 2) == "px") {
        try {
            float px = std::stof(trimmed.substr(0, trimmed.size() - 2));
            return NonRepeatedTrackSizingFunction::Fixed(px);
        } catch (...) {
            return NonRepeatedTrackSizingFunction::Auto();
        }
    }

    if (trimmed.size() > 1 && trimmed.back() == '%') {
        try {
            float pct = std::stof(trimmed.substr(0, trimmed.size() - 1));
            return NonRepeatedTrackSizingFunction{
                MinTrackSizingFunction::Percent(pct / 100.0f),
                MaxTrackSizingFunction::Percent(pct / 100.0f)
            };
        } catch (...) {
            return NonRepeatedTrackSizingFunction::Auto();
        }
    }

    if (trimmed.size() > 7 && trimmed.substr(0, 7) == "minmax(") {
        size_t close = trimmed.rfind(')');
        if (close != std::string::npos) {
            std::string inner = trimmed.substr(7, close - 7);
            size_t comma = inner.find(',');
            if (comma != std::string::npos) {
                std::string min_str = inner.substr(0, comma);
                std::string max_str = inner.substr(comma + 1);
                auto min_func = ParseGridTrackValue(min_str);
                auto max_func = ParseGridTrackValue(max_str);
                return NonRepeatedTrackSizingFunction{min_func.min, max_func.max};
            }
        }
    }

    try {
        float px = std::stof(trimmed);
        return NonRepeatedTrackSizingFunction::Fixed(px);
    } catch (...) {}

    return NonRepeatedTrackSizingFunction::Auto();
}

// Helper to parse grid-template-columns/rows string
static std::vector<TrackSizingFunction> ParseGridTemplate(const std::string& template_str) {
    std::vector<TrackSizingFunction> result;
    if (template_str.empty()) return result;

    std::string str = template_str;
    size_t pos = 0;

    while (pos < str.size()) {
        while (pos < str.size() && (str[pos] == ' ' || str[pos] == '\t')) pos++;
        if (pos >= str.size()) break;

        if (str.substr(pos, 7) == "repeat(") {
            size_t start = pos + 7;
            int paren_count = 1;
            size_t end = start;
            while (end < str.size() && paren_count > 0) {
                if (str[end] == '(') paren_count++;
                else if (str[end] == ')') paren_count--;
                end++;
            }

            std::string repeat_content = str.substr(start, end - start - 1);
            size_t comma = repeat_content.find(',');
            if (comma != std::string::npos) {
                std::string count_str = repeat_content.substr(0, comma);
                std::string tracks_str = repeat_content.substr(comma + 1);

                size_t cs = count_str.find_first_not_of(" \t");
                size_t ce = count_str.find_last_not_of(" \t");
                if (cs != std::string::npos) count_str = count_str.substr(cs, ce - cs + 1);

                uint16_t repeat_count = 1;
                if (count_str == "auto-fill") repeat_count = 0;
                else if (count_str == "auto-fit") repeat_count = UINT16_MAX;
                else {
                    try { repeat_count = static_cast<uint16_t>(std::stoi(count_str)); } catch (...) {}
                }

                std::vector<NonRepeatedTrackSizingFunction> repeat_tracks;
                auto inner_tracks = ParseGridTemplate(tracks_str);
                for (const auto& t : inner_tracks) {
                    if (t.type == TrackSizingFunction::Type::Single) {
                        repeat_tracks.push_back(t.single);
                    }
                }

                if (!repeat_tracks.empty()) {
                    result.push_back(TrackSizingFunction::Repeat(repeat_count, repeat_tracks));
                }
            }
            pos = end;
        } else {
            size_t end = pos;
            int paren_count = 0;
            while (end < str.size()) {
                if (str[end] == '(') paren_count++;
                else if (str[end] == ')') paren_count--;
                else if ((str[end] == ' ' || str[end] == '\t') && paren_count == 0) break;
                end++;
            }

            std::string track_value = str.substr(pos, end - pos);
            if (!track_value.empty()) {
                auto track = ParseGridTrackValue(track_value);
                result.push_back(TrackSizingFunction::Single(track));
            }
            pos = end;
        }
    }

    return result;
}

// Helper to parse grid-auto-rows/grid-auto-columns string
// Can contain multiple track sizes separated by spaces
static std::vector<NonRepeatedTrackSizingFunction> ParseGridAutoTracks(const std::string& auto_str) {
    std::vector<NonRepeatedTrackSizingFunction> result;
    if (auto_str.empty()) return result;

    std::string str = auto_str;
    size_t pos = 0;

    while (pos < str.size()) {
        // Skip whitespace
        while (pos < str.size() && (str[pos] == ' ' || str[pos] == '\t')) pos++;
        if (pos >= str.size()) break;

        // Find end of this track value (handle parentheses for minmax())
        size_t end = pos;
        int paren_count = 0;
        while (end < str.size()) {
            if (str[end] == '(') paren_count++;
            else if (str[end] == ')') paren_count--;
            else if ((str[end] == ' ' || str[end] == '\t') && paren_count == 0) break;
            end++;
        }

        std::string track_value = str.substr(pos, end - pos);
        if (!track_value.empty()) {
            result.push_back(ParseGridTrackValue(track_value));
        }
        pos = end;
    }

    return result;
}

// Helper to parse grid-column/row placement
static GridPlacement ParseGridPlacement(const std::string& value) {
    if (value.empty() || value == "auto") return GridPlacement::Auto();

    if (value.size() > 5 && value.substr(0, 5) == "span ") {
        try {
            uint16_t span = static_cast<uint16_t>(std::stoi(value.substr(5)));
            return GridPlacement::Span(span);
        } catch (...) {
            return GridPlacement::Auto();
        }
    }

    try {
        int16_t line = static_cast<int16_t>(std::stoi(value));
        return GridPlacement::Line(line);
    } catch (...) {}

    return GridPlacement::Auto();
}

// Helper to parse grid-column/row shorthand
// CSS规范: 当只有一个值且是 span 时, 表示 grid-*-end: span N
// 例如: grid-column: span 2 => grid-column-start: auto; grid-column-end: span 2
static std::pair<GridPlacement, GridPlacement> ParseGridLine(const std::string& value) {
    if (value.empty()) return {GridPlacement::Auto(), GridPlacement::Auto()};

    size_t slash = value.find('/');
    if (slash != std::string::npos) {
        std::string start_str = value.substr(0, slash);
        std::string end_str = value.substr(slash + 1);

        size_t s1 = start_str.find_first_not_of(" \t");
        size_t e1 = start_str.find_last_not_of(" \t");
        if (s1 != std::string::npos) start_str = start_str.substr(s1, e1 - s1 + 1);

        size_t s2 = end_str.find_first_not_of(" \t");
        size_t e2 = end_str.find_last_not_of(" \t");
        if (s2 != std::string::npos) end_str = end_str.substr(s2, e2 - s2 + 1);

        return {ParseGridPlacement(start_str), ParseGridPlacement(end_str)};
    }

    // 单值: 如果是 span, 放到 end 位置; 否则放到 start 位置
    auto placement = ParseGridPlacement(value);
    if (placement.IsSpan()) {
        // span 2 => start=auto, end=span 2
        return {GridPlacement::Auto(), placement};
    } else {
        // 1 或 auto => start=value, end=auto
        return {placement, GridPlacement::Auto()};
    }
}

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

    // Helper function to recursively clear is_laid_out flags
    std::function<void(RenderObject*)> clearLayoutFlags = [&](RenderObject* obj) {
        if (!obj) return;
        obj->GetLayoutInfo().is_laid_out = false;
        for (auto& child : obj->GetChildren()) {
            clearLayoutFlags(child.get());
        }
    };

    // Check if we can reuse the existing tree
    auto cached = cached_root_.lock();
    
    if (root_node_ != 0 && cached && cached.get() == root.get()) {
        if (root->NeedsLayout()) {
            // Need to rebuild - clear all layout flags first
            clearLayoutFlags(root.get());
            Clear();
            cached_root_ = root;
            BuildSubtree(root.get(), 0);
            return;
        }
        // Same tree, just update styles
        // TODO: UpdateStylesRecursive
        return;
    }

    // New tree - clear all layout flags before building
    clearLayoutFlags(root.get());
    Clear();
    cached_root_ = root;
    BuildSubtree(root.get(), 0);
}

void NativeLayoutEngine::ComputeLayout(float available_width, float available_height) {
    if (root_node_ == 0) {
        return;
    }

    // Clear all caches and needs_layout flags before full layout computation
    // This ensures a complete recalculation of the entire tree
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 6.1**
    for (auto& pair : nodes_) {
        pair.second.cache.Clear();
        pair.second.needs_layout = false;  // Clear dirty flag after full layout
    }
    
    // Reset cached scrollbar state for full layout
    // This forces re-detection of scrollbar need
    last_needs_v_scrollbar_ = false;
    last_effective_width_ = 0.0f;

    // Delegate to internal method for actual layout computation
    ComputeLayoutInternal(available_width, available_height);
}

void NativeLayoutEngine::ComputeLayoutInternal(float available_width, float available_height) {
    // Internal layout computation - does NOT clear caches
    // This allows incremental layout to selectively clear only dirty node caches
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 2.2**
    
    if (root_node_ == 0) {
        return;
    }

    // Check if root node has overflow: auto or scroll and might need scrollbar
    LayoutNode* root = GetNode(root_node_);
    float effective_width = available_width;
    bool needs_v_scrollbar = false;

    if (root && root->render_obj) {
        const auto& style = root->render_obj->GetComputedStyle();
        std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

        // For overflow: auto or scroll on root, we need to account for potential vertical scrollbar
        if (overflow_y == "auto" || overflow_y == "scroll") {
            // Optimization for incremental layout: use cached scrollbar state
            // This avoids the expensive two-pass layout on every incremental update
            // **Feature: incremental-layout-optimization**
            // **Validates: Requirements 2.5**
            if (last_effective_width_ > 0 && last_needs_v_scrollbar_) {
                // Use cached scrollbar state - skip the first pass
                effective_width = available_width - RenderObject::GetScrollbarWidth();
                needs_v_scrollbar = true;
            } else {
                // First pass: compute layout with full width, accounting for root margin
                Rect<float> first_pass_margin = ResolveOrZero(root->style.margin, std::optional<float>(available_width));
                float first_pass_width = available_width - first_pass_margin.left - first_pass_margin.right;
                
                LayoutInput inputs;
                inputs.run_mode = RunMode::PerformLayout;
                inputs.sizing_mode = SizingMode::InherentSize;
                inputs.known_dimensions = Size<std::optional<float>>{
                    std::optional<float>(first_pass_width),
                    std::nullopt
                };
                inputs.parent_size = Size<std::optional<float>>{
                    std::optional<float>(available_width),
                    std::optional<float>(available_height)
                };
                inputs.available_space = Size<AvailableSpace>{
                    AvailableSpace::Definite(first_pass_width),
                    AvailableSpace::Definite(available_height)
                };
                // Enable vertical margin collapsing
                inputs.vertical_margins_are_collapsible = Line<bool>{true, true};

                LayoutOutput first_pass = ComputeNodeLayout(root_node_, inputs);

                // Check if content height exceeds available height (needs vertical scrollbar)
                // or if overflow-y is scroll (always show scrollbar)
                // Use size.height instead of content_size.height for the actual rendered height
                // Add root margin to get total height
                float total_height = first_pass.size.height + first_pass_margin.top + first_pass_margin.bottom;
                needs_v_scrollbar = (total_height > available_height) ||
                                          (overflow_y == "scroll");

                if (needs_v_scrollbar) {
                    // Reduce available width by scrollbar width
                    effective_width = available_width - RenderObject::GetScrollbarWidth();

                    // Optimization: Only clear caches for nodes affected by width change
                    // Nodes with fixed width (explicit pixel values) that don't depend on
                    // the available width can keep their cache results from the first pass.
                    // This significantly improves performance for layouts with many fixed-size elements.
                    // **Feature: incremental-layout-optimization**
                    // **Validates: Requirements 2.5**
                    ClearWidthDependentCaches(root_node_);
                }
            }
            
            // Cache the scrollbar state for future incremental layouts
            last_needs_v_scrollbar_ = needs_v_scrollbar;
            last_effective_width_ = effective_width;
        }
    }

    // For root element (body), we need to handle margin specially:
    // - Root element's margin offsets it from the viewport edge
    // - Root element's width = effective_width - margin_left - margin_right
    // - Scrollbar is in viewport, outside of body's margin
    LayoutNode* root_node = GetNode(root_node_);
    Rect<float> root_margin = {0.0f, 0.0f, 0.0f, 0.0f};
    float root_width = effective_width;
    
    if (root_node) {
        // Resolve root element's margin based on available_width (viewport width)
        root_margin = ResolveOrZero(root_node->style.margin, std::optional<float>(available_width));
        // Root element's width should be reduced by its horizontal margins
        root_width = effective_width - root_margin.left - root_margin.right;
    }

    LayoutInput inputs;
    inputs.run_mode = RunMode::PerformLayout;
    inputs.sizing_mode = SizingMode::InherentSize;
    // Set known width for root element to account for its margin
    inputs.known_dimensions = Size<std::optional<float>>{
        std::optional<float>(root_width),
        std::nullopt
    };
    inputs.parent_size = Size<std::optional<float>>{
        std::optional<float>(available_width),
        std::optional<float>(available_height)
    };
    inputs.available_space = Size<AvailableSpace>{
        AvailableSpace::Definite(root_width),
        AvailableSpace::Definite(available_height)
    };
    // Enable vertical margin collapsing for proper CSS margin collapse behavior
    // This allows margins of child elements to collapse with their container
    inputs.vertical_margins_are_collapsible = Line<bool>{true, true};

    ComputeNodeLayout(root_node_, inputs);
    
    // Set root element position based on its margin
    // In CSS, the root element's margin offsets it from the viewport edge
    if (root_node) {
        root_node->x = root_margin.left;
        root_node->y = root_margin.top;
        root_node->layout.location = Point<float>{root_margin.left, root_margin.top};
    }
    
    PositionChildren(root_node_);
}

void NativeLayoutEngine::GetLayoutInfo(std::shared_ptr<RenderObject> root) {
    if (!root) {
        return;
    }
    ReadLayoutResults(root.get());
}

bool NativeLayoutEngine::ComputeIncrementalLayout(float available_width, float available_height) {
    if (root_node_ == 0) {
        return false;
    }

    // 收集需要布局的节点
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 2.1, 2.2**
    std::vector<NodeId> dirty_nodes;
    std::function<void(NodeId)> collectDirty = [&](NodeId node_id) {
        LayoutNode* node = GetNode(node_id);
        if (!node) return;

        if (node->needs_layout) {
            dirty_nodes.push_back(node_id);
        }

        for (NodeId child_id : node->children) {
            collectDirty(child_id);
        }
    };
    collectDirty(root_node_);

    if (dirty_nodes.empty()) {
        return false;
    }

    // 关键修复：当根节点需要重新布局时，重置滚动条状态缓存
    // 这确保在页面切换等大规模 DOM 变化时，滚动条状态会被重新检测
    // 避免使用旧的滚动条状态导致布局错误
    LayoutNode* root = GetNode(root_node_);
    if (root && root->needs_layout) {
        last_needs_v_scrollbar_ = false;
        last_effective_width_ = 0.0f;
    }

    // 关键优化：只清除脏节点的缓存，而不是所有节点
    // 这样干净的节点可以保留其缓存结果，实现真正的增量布局
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 2.1, 2.4**
    for (NodeId node_id : dirty_nodes) {
        LayoutNode* node = GetNode(node_id);
        if (node) {
            node->cache.Clear();
            node->needs_layout = false;
        }
    }

    // 直接调用内部布局方法，跳过 ComputeLayout() 中的全局缓存清除
    // ComputeLayoutInternal() 不会清除缓存，因此干净节点的缓存会被保留
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 2.2**
    ComputeLayoutInternal(available_width, available_height);
    
    return true;
}

void NativeLayoutEngine::MarkNeedsLayout(RenderObject* render_obj) {
    auto it = render_to_node_.find(render_obj);
    if (it != render_to_node_.end()) {
        LayoutNode* node = GetNode(it->second);
        if (node) {
            // Determine the layout scope for this node
            // This decides how dirty marks should propagate through the tree
            // **Feature: incremental-layout-optimization**
            // **Validates: Requirements 4.1, 4.2, 4.3, 4.4**
            LayoutScope scope = DetermineLayoutScope(node);

            // Use intelligent dirty mark propagation based on layout scope
            // - Fixed-size containers don't propagate to ancestors
            // - Auto-size containers propagate to ancestors
            // - Flex/grid children notify parent for sibling recalculation
            PropagateLayoutDirty(it->second, scope);
        }
    }
}

void NativeLayoutEngine::UpdateStyle(RenderObject* render_obj, const ComputedStyle& style) {
    auto it = render_to_node_.find(render_obj);
    if (it != render_to_node_.end()) {
        LayoutNode* node = GetNode(it->second);
        if (node) {
            // 转换新样式
            Style new_style = ConvertStyle(style);

            // 关键优化：只有布局相关属性变化时才标记需要重新布局
            // 纯视觉属性（如 color, background-color）的变化不应触发布局
            bool layout_changed = false;

            // 检查影响布局的属性是否变化
            const Style& old_style = node->style;

            // Display 和 Position 变化会影响布局
            if (old_style.display != new_style.display ||
                old_style.position != new_style.position ||
                old_style.box_sizing != new_style.box_sizing) {
                layout_changed = true;
            }

            // 尺寸属性变化会影响布局
            if (old_style.size.width != new_style.size.width ||
                old_style.size.height != new_style.size.height ||
                old_style.min_size.width != new_style.min_size.width ||
                old_style.min_size.height != new_style.min_size.height ||
                old_style.max_size.width != new_style.max_size.width ||
                old_style.max_size.height != new_style.max_size.height) {
                layout_changed = true;
            }

            // 盒模型属性变化会影响布局
            if (old_style.padding != new_style.padding ||
                old_style.margin != new_style.margin ||
                old_style.border != new_style.border ||
                old_style.inset != new_style.inset) {
                layout_changed = true;
            }

            // Flexbox 属性变化会影响布局
            if (old_style.flex_direction != new_style.flex_direction ||
                old_style.flex_wrap != new_style.flex_wrap ||
                old_style.flex_grow != new_style.flex_grow ||
                old_style.flex_shrink != new_style.flex_shrink ||
                old_style.flex_basis != new_style.flex_basis ||
                old_style.justify_content != new_style.justify_content ||
                old_style.align_items != new_style.align_items ||
                old_style.align_content != new_style.align_content ||
                old_style.gap != new_style.gap ||
                old_style.order != new_style.order) {
                layout_changed = true;
            }

            // Grid 属性变化会影响布局
            if (old_style.grid_auto_flow != new_style.grid_auto_flow) {
                layout_changed = true;
            }

            // Overflow 变化可能影响布局（滚动条）
            if (old_style.overflow.x != new_style.overflow.x ||
                old_style.overflow.y != new_style.overflow.y ||
                old_style.scrollbar_width != new_style.scrollbar_width) {
                layout_changed = true;
            }

            // 更新样式
            node->style = new_style;

            // Also update all the specialized style structs
            node->block_container_style.display = node->style.display;
            node->block_container_style.box_sizing = node->style.box_sizing;
            node->block_container_style.position = node->style.position;
            node->block_container_style.overflow = node->style.overflow;
            node->block_container_style.scrollbar_width = node->style.scrollbar_width;
            node->block_container_style.size = node->style.size;
            node->block_container_style.min_size = node->style.min_size;
            node->block_container_style.max_size = node->style.max_size;
            node->block_container_style.padding = node->style.padding;
            node->block_container_style.border = node->style.border;
            node->block_container_style.margin = node->style.margin;
            node->block_container_style.inset = node->style.inset;

            node->block_item_style.display = node->style.display;
            node->block_item_style.box_sizing = node->style.box_sizing;
            node->block_item_style.position = node->style.position;
            node->block_item_style.overflow = node->style.overflow;
            node->block_item_style.scrollbar_width = node->style.scrollbar_width;
            node->block_item_style.size = node->style.size;
            node->block_item_style.min_size = node->style.min_size;
            node->block_item_style.max_size = node->style.max_size;
            node->block_item_style.padding = node->style.padding;
            node->block_item_style.border = node->style.border;
            node->block_item_style.margin = node->style.margin;
            node->block_item_style.inset = node->style.inset;

            node->grid_item_style.display = node->style.display;
            node->grid_item_style.box_sizing = node->style.box_sizing;
            node->grid_item_style.position = node->style.position;
            node->grid_item_style.overflow = node->style.overflow;
            node->grid_item_style.scrollbar_width = node->style.scrollbar_width;
            node->grid_item_style.size = node->style.size;
            node->grid_item_style.min_size = node->style.min_size;
            node->grid_item_style.max_size = node->style.max_size;
            node->grid_item_style.padding = node->style.padding;
            node->grid_item_style.border = node->style.border;
            node->grid_item_style.margin = node->style.margin;
            node->grid_item_style.inset = node->style.inset;

            // 只有布局相关属性变化时才标记需要重新布局和更新版本号
            // **Feature: incremental-layout-optimization**
            // **Validates: Requirements 1.3**
            if (layout_changed) {
                node->needs_layout = true;
                
                // Update content version for layout-affecting style changes
                // Pure paint styles (color, background-color, etc.) don't update version
                uint64_t new_version = ContentVersionManager::GetInstance().GenerateVersion();
                node->content_version = new_version;
            }
        }
    } else {
        // 如果找不到对应的 LayoutNode，可能是之前是 display: none
        // 现在如果变成可见的，需要添加到 LayoutTree
        // 但是！如果元素已经被标记为 is_laid_out，说明它可能由匿名块盒管理
        // （匿名块盒管理的内联元素不在 render_to_node_ 映射中）
        // 不应该重新添加，否则会破坏匿名块盒的布局
        bool already_laid_out = render_obj->GetLayoutInfo().is_laid_out;
        if (style.display != RenderObjectType::NONE && !already_laid_out) {
            AddElement(render_obj, render_obj->GetParent().get());
            
            // 找到新添加的节点并标记需要布局
            auto new_it = render_to_node_.find(render_obj);
            if (new_it != render_to_node_.end()) {
                LayoutNode* node = GetNode(new_it->second);
                if (node) {
                    node->needs_layout = true;
                    if (node->parent != 0) {
                        if (LayoutNode* parent = GetNode(node->parent)) {
                            parent->needs_layout = true;
                        }
                    }
                }
            }
        }
    }
}

void NativeLayoutEngine::UpdateContentVersion(RenderObject* render_obj) {
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

    // Generate a new version number
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 1.1, 1.2, 1.3**
    uint64_t new_version = ContentVersionManager::GetInstance().GenerateVersion();
    node->content_version = new_version;

    // Determine the layout scope for this node
    // This decides how dirty marks should propagate through the tree
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 4.1, 4.2, 4.3, 4.4**
    LayoutScope scope = DetermineLayoutScope(node);

    // Use intelligent dirty mark propagation based on layout scope
    // - Fixed-size containers don't propagate to ancestors
    // - Auto-size containers propagate to ancestors
    PropagateLayoutDirty(it->second, scope);
}

NativeLayoutEngine::LayoutScope NativeLayoutEngine::DetermineLayoutScope(const LayoutNode* node) const {
    // Default to SUBTREE if node is invalid
    if (!node) {
        return LayoutScope::SUBTREE;
    }

    // Check position first - absolute/fixed positioned elements only affect themselves
    // They are taken out of normal flow and don't affect siblings or ancestors
    if (node->style.position == Position::Absolute || node->style.position == Position::Fixed) {
        return LayoutScope::SELF_ONLY;
    }

    // Check if the node has fixed dimensions (both width AND height are explicit lengths)
    // Fixed-size containers isolate their children from affecting ancestors
    if (HasFixedSize(node)) {
        return LayoutScope::SELF_ONLY;
    }

    // Check if this is a flex or grid child
    // Flex/grid children may affect sibling layouts due to space distribution
    if (IsFlexOrGridChild(node)) {
        return LayoutScope::SIBLINGS;
    }

    // For auto-sized elements, changes may propagate to ancestors
    // because the parent container size may depend on child content
    return LayoutScope::ANCESTORS;
}

bool NativeLayoutEngine::HasFixedSize(const LayoutNode* node) const {
    if (!node) {
        return false;
    }

    const auto& style = node->style;
    
    // Check if width is a fixed length (not auto, not percent)
    bool width_fixed = style.size.width.IsLength() && !style.size.width.IsAuto();
    
    // Check if height is a fixed length (not auto, not percent)
    bool height_fixed = style.size.height.IsLength() && !style.size.height.IsAuto();
    
    // Both dimensions must be fixed for the container to be considered "fixed size"
    return width_fixed && height_fixed;
}

bool NativeLayoutEngine::IsFlexOrGridChild(const LayoutNode* node) const {
    if (!node || node->parent == 0) {
        return false;
    }

    const LayoutNode* parent = GetNode(node->parent);
    if (!parent) {
        return false;
    }

    // Check if parent is a flex or grid container
    return parent->style.display == Display::Flex || parent->style.display == Display::Grid;
}

bool NativeLayoutEngine::IsWidthDependent(const LayoutNode* node) const {
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 2.5**
    
    if (!node) {
        return true;  // Assume dependent if node is null
    }

    const Style& style = node->style;

    // Check if width is auto or percentage (depends on available width)
    if (style.size.width.IsAuto()) {
        return true;
    }
    if (style.size.width.IsPercent() || style.size.width.IsCalc()) {
        return true;
    }

    // Check if min-width or max-width use percentage
    if (style.min_size.width.IsPercent() || style.min_size.width.IsCalc()) {
        return true;
    }
    if (style.max_size.width.IsPercent() || style.max_size.width.IsCalc()) {
        return true;
    }

    // Check if horizontal margin uses percentage
    if (style.margin.left.IsPercent() || style.margin.left.IsCalc() ||
        style.margin.right.IsPercent() || style.margin.right.IsCalc()) {
        return true;
    }

    // Check if horizontal padding uses percentage
    if (style.padding.left.IsPercent() || style.padding.left.IsCalc() ||
        style.padding.right.IsPercent() || style.padding.right.IsCalc()) {
        return true;
    }

    // Flex/grid children may be affected by container width changes
    if (IsFlexOrGridChild(node)) {
        // Flex items with flex-grow or flex-shrink may change size
        if (style.flex_grow > 0.0f || style.flex_shrink > 0.0f) {
            return true;
        }
        // Flex items with percentage flex-basis
        if (style.flex_basis.IsPercent() || style.flex_basis.IsCalc()) {
            return true;
        }
    }

    // IFC containers may need re-layout if available width changes
    // (text wrapping depends on available width)
    if (node->is_ifc_container) {
        return true;
    }

    // Fixed width with explicit pixel value - not dependent on available width
    return false;
}

void NativeLayoutEngine::ClearWidthDependentCaches(NodeId node_id) {
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 2.5**
    
    LayoutNode* node = GetNode(node_id);
    if (!node) {
        return;
    }

    // Check if this node's layout depends on available width
    bool is_dependent = IsWidthDependent(node);

    if (is_dependent) {
        // Clear cache for this node
        node->cache.Clear();
    }

    // Recursively process children
    // If parent cache was cleared, children may also need clearing
    // because their available space may have changed
    for (NodeId child_id : node->children) {
        LayoutNode* child = GetNode(child_id);
        if (child) {
            if (is_dependent) {
                // Parent is width-dependent, so children's available space changed
                // Clear child cache regardless of child's own width dependency
                child->cache.Clear();
                // Still need to recurse to clear grandchildren
                ClearWidthDependentCachesRecursive(child_id);
            } else {
                // Parent is not width-dependent, check child independently
                ClearWidthDependentCaches(child_id);
            }
        }
    }
}

void NativeLayoutEngine::ClearWidthDependentCachesRecursive(NodeId node_id) {
    // Helper method to clear all caches in a subtree
    // Used when parent is width-dependent and all children need clearing
    LayoutNode* node = GetNode(node_id);
    if (!node) {
        return;
    }

    node->cache.Clear();

    for (NodeId child_id : node->children) {
        ClearWidthDependentCachesRecursive(child_id);
    }
}

void NativeLayoutEngine::PropagateLayoutDirty(NodeId node_id, LayoutScope scope) {
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 4.1, 4.2, 4.3, 4.4**
    
    LayoutNode* node = GetNode(node_id);
    if (!node) {
        return;
    }

    // Always mark the current node as needing layout
    node->needs_layout = true;

    // Determine propagation behavior based on scope
    switch (scope) {
        case LayoutScope::SELF_ONLY:
            // Only mark self, don't propagate to ancestors
            // This applies to absolute/fixed positioned elements and fixed-size containers
            // They are isolated from the normal flow and don't affect ancestors
            break;

        case LayoutScope::SUBTREE:
            // Mark self and subtree, but don't propagate to ancestors
            // This applies to containers with fixed dimensions
            // Changes within them don't affect parent container size
            break;

        case LayoutScope::SIBLINGS:
            // Mark self and notify parent to recalculate siblings
            // This applies to flex/grid children where space distribution may change
            // **Validates: Requirements 4.4**
            if (node->parent != 0) {
                LayoutNode* parent = GetNode(node->parent);
                if (parent && !parent->needs_layout) {
                    parent->needs_layout = true;
                    // For IFC containers, we MUST update content_version because IFC layout
                    // caches based on content_version and needs to re-collect inline content.
                    // For non-IFC containers (flex/grid), we do NOT update content_version.
                    // **Feature: incremental-layout-optimization**
                    // **Validates: Requirements 4.4**
                    if (parent->is_ifc_container) {
                        parent->content_version = ContentVersionManager::GetInstance().GenerateVersion();
                    }
                    
                    // For flex/grid containers, we need to propagate further up
                    // because the container size might change
                    LayoutScope parent_scope = DetermineLayoutScope(parent);
                    if (parent_scope == LayoutScope::ANCESTORS || parent_scope == LayoutScope::SIBLINGS) {
                        PropagateLayoutDirty(node->parent, parent_scope);
                    }
                }
            }
            break;

        case LayoutScope::ANCESTORS:
            // Mark self and propagate to all ancestors up to root
            // This applies to auto-sized elements where content changes affect parent size
            // **Validates: Requirements 4.1, 4.2**
            {
                NodeId parent_id = node->parent;
                while (parent_id != 0) {
                    LayoutNode* parent = GetNode(parent_id);
                    if (!parent) {
                        break;
                    }

                    // If parent is already marked dirty, we can stop propagation
                    // This is an optimization to avoid redundant marking
                    // **Validates: Requirements 4.2**
                    if (parent->needs_layout) {
                        break;
                    }

                    parent->needs_layout = true;
                    
                    // For IFC containers, we MUST update content_version because IFC layout
                    // caches based on content_version and needs to re-collect inline content
                    // when any child changes.
                    // For non-IFC containers, we do NOT update content_version because
                    // child layouts are computed independently and the parent's cache
                    // can remain valid (only needs_layout flag triggers re-layout).
                    // **Feature: incremental-layout-optimization**
                    // **Validates: Requirements 4.1, 4.2**
                    if (parent->is_ifc_container) {
                        parent->content_version = ContentVersionManager::GetInstance().GenerateVersion();
                    }

                    // Check if this parent is a fixed-size container
                    // If so, stop propagation here as changes won't affect its ancestors
                    LayoutScope parent_scope = DetermineLayoutScope(parent);
                    if (parent_scope == LayoutScope::SELF_ONLY) {
                        // Fixed-size container isolates changes
                        break;
                    }

                    parent_id = parent->parent;
                }
            }
            break;
    }
}

void NativeLayoutEngine::AddElement(RenderObject* render_obj, RenderObject* parent, size_t insert_index) {
    if (!render_obj) {
        return;
    }
    
    // 如果节点已存在，不需要重复添加
    if (HasElement(render_obj)) {
        return;
    }

    NodeId parent_id = 0;
    if (parent) {
        auto it = render_to_node_.find(parent);
        if (it != render_to_node_.end()) {
            parent_id = it->second;
        }
    }

    // 关键修复：如果父节点不在布局树中，不添加子节点
    // 这避免了孤立节点的创建
    if (parent && parent_id == 0) {
        // 父节点的 RenderObject 存在但不在布局树中
        // 这可能是因为父节点是 display: none 或其他原因
        return;
    }

    // 简化处理：总是使用 BuildSubtree 添加到末尾
    // 布局树的顺序可能与渲染树不同（因为匿名块盒等原因）
    // 使用 insert_index 可能导致索引越界或插入到错误位置
    BuildSubtree(render_obj, parent_id);
    
    // 标记父节点需要重新布局
    if (parent_id != 0) {
        LayoutNode* parent_node = GetNode(parent_id);
        if (parent_node) {
            // Update parent's content version when child is added
            // **Feature: incremental-layout-optimization**
            // **Validates: Requirements 1.2**
            uint64_t new_version = ContentVersionManager::GetInstance().GenerateVersion();
            parent_node->content_version = new_version;
            parent_node->needs_layout = true;
            parent_node->cache.Clear();
            
            // 清除所有祖先的缓存
            NodeId ancestor_id = parent_node->parent;
            while (ancestor_id != 0) {
                LayoutNode* ancestor = GetNode(ancestor_id);
                if (!ancestor) break;
                
                ancestor->cache.Clear();
                ancestor->needs_layout = true;
                ancestor->content_version = ContentVersionManager::GetInstance().GenerateVersion();
                
                ancestor_id = ancestor->parent;
            }
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

    // 从父节点的 children 列表中移除，并清除祖先的布局缓存
    if (node && node->parent != 0) {
        LayoutNode* parent = GetNode(node->parent);
        if (parent) {
            auto& children = parent->children;
            children.erase(
                std::remove(children.begin(), children.end(), node_id),
                children.end()
            );
            
            // 清除父节点及所有祖先的布局缓存
            // 这是关键：确保下次布局时重新计算
            NodeId ancestor_id = node->parent;
            while (ancestor_id != 0) {
                LayoutNode* ancestor = GetNode(ancestor_id);
                if (!ancestor) break;
                
                ancestor->cache.Clear();
                ancestor->needs_layout = true;
                uint64_t new_version = ContentVersionManager::GetInstance().GenerateVersion();
                ancestor->content_version = new_version;
                
                ancestor_id = ancestor->parent;
            }
        }
    }

    // 递归移除所有子节点（使用迭代方式避免栈溢出）
    std::vector<NodeId> to_remove;
    to_remove.push_back(node_id);
    
    size_t index = 0;
    while (index < to_remove.size()) {
        NodeId current_id = to_remove[index++];
        LayoutNode* current = GetNode(current_id);
        if (current) {
            // 将所有子节点加入待移除列表
            for (NodeId child_id : current->children) {
                to_remove.push_back(child_id);
            }
        }
    }
    
    // 从 maps 中移除所有节点，并清除 RenderObject 的布局信息
    for (NodeId id : to_remove) {
        LayoutNode* n = GetNode(id);
        if (n) {
            // 清除 RenderObject 的布局信息，避免残留
            if (n->render_obj) {
                LayoutInfo& layout = n->render_obj->GetLayoutInfo();
                layout.is_laid_out = false;
                layout.x = 0.0f;
                layout.y = 0.0f;
                layout.width = 0.0f;
                layout.height = 0.0f;
                render_to_node_.erase(n->render_obj);
            }
            
            // 对于匿名块盒，清除其管理的内联子元素的布局信息
            if (n->is_anonymous_block) {
                for (RenderObject* inline_child : n->anonymous_inline_children) {
                    if (inline_child) {
                        LayoutInfo& child_layout = inline_child->GetLayoutInfo();
                        child_layout.is_laid_out = false;
                        child_layout.x = 0.0f;
                        child_layout.y = 0.0f;
                        child_layout.width = 0.0f;
                        child_layout.height = 0.0f;
                    }
                }
            }
        }
        nodes_.erase(id);
        
        if (id == root_node_) {
            root_node_ = 0;
        }
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
    
    // 为 IFC 容器初始化 content_version（非零值启用版本检查）
    if (node.is_ifc_container) {
        node.content_version = ContentVersionManager::GetInstance().GenerateVersion();
    }
    
    // 检测 TABLE 容器
    RenderObjectType type = render_obj->GetType();
    node.is_table_container = (type == RenderObjectType::TABLE);


    // Also fill in BlockContainerStyle and BlockItemStyle for the interfaces
    node.block_container_style.display = node.style.display;
    node.block_container_style.box_sizing = node.style.box_sizing;
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
    node.block_item_style.box_sizing = node.style.box_sizing;
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
    node.flexbox_container_style.box_sizing = node.style.box_sizing;
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
    node.flexbox_item_style.box_sizing = node.style.box_sizing;
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
    node.flexbox_item_style.order = node.style.order;

    // Fill in GridContainerStyle and GridItemStyle
    node.grid_container_style.display = node.style.display;
    node.grid_container_style.box_sizing = node.style.box_sizing;
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
    // Parse grid-template-rows, grid-template-columns
    node.grid_container_style.grid_template_columns = ParseGridTemplate(computed.grid_template_columns);
    node.grid_container_style.grid_template_rows = ParseGridTemplate(computed.grid_template_rows);
    // Parse grid-auto-rows, grid-auto-columns
    node.grid_container_style.grid_auto_columns = ParseGridAutoTracks(computed.grid_auto_columns);
    node.grid_container_style.grid_auto_rows = ParseGridAutoTracks(computed.grid_auto_rows);
    // Parse gap
    node.grid_container_style.column_gap = ConvertLength(computed.column_gap);
    node.grid_container_style.row_gap = ConvertLength(computed.row_gap);
    // Parse align-items and justify-items for grid
    node.grid_container_style.align_items = node.style.align_items;
    node.grid_container_style.justify_items = node.style.justify_items;

    node.grid_item_style.display = node.style.display;
    node.grid_item_style.box_sizing = node.style.box_sizing;
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
    // Parse grid-row, grid-column
    auto [col_start, col_end] = ParseGridLine(computed.grid_column);
    auto [row_start, row_end] = ParseGridLine(computed.grid_row);
    node.grid_item_style.grid_column_start = col_start;
    node.grid_item_style.grid_column_end = col_end;
    node.grid_item_style.grid_row_start = row_start;
    node.grid_item_style.grid_row_end = row_end;
    // Grid item alignment (align-self, justify-self)
    node.grid_item_style.align_self = node.style.align_self;
    node.grid_item_style.justify_self = node.style.justify_self;

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

// Helper to convert CSSLength to Dimension
Dimension ConvertDimension(const CSSLength& css_length) {
    // Handle calc() expressions
    if (css_length.is_calc) {
        return Dimension::Calc(css_length.calc_percent / 100.0f, css_length.calc_px);
    }

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
        case CSSUnit::VW: {
            // 视口宽度单位
            float vw = ViewportSize::GetWidth();
            return Dimension::Length(css_length.value * vw / 100.0f);
        }
        case CSSUnit::VH: {
            // 视口高度单位
            float vh = ViewportSize::GetHeight();
            return Dimension::Length(css_length.value * vh / 100.0f);
        }
        case CSSUnit::VMIN: {
            // 视口最小尺寸单位
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            float vmin = std::min(vw, vh);
            return Dimension::Length(css_length.value * vmin / 100.0f);
        }
        case CSSUnit::VMAX: {
            // 视口最大尺寸单位
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            float vmax = std::max(vw, vh);
            return Dimension::Length(css_length.value * vmax / 100.0f);
        }
        default:
            return Dimension::Auto();
    }
}

// Helper to convert CSSLength to LengthPercentageAuto
LengthPercentageAuto ConvertLengthAuto(const CSSLength& css_length) {
    // Handle calc() expressions
    if (css_length.is_calc) {
        return LengthPercentageAuto::Calc(css_length.calc_percent / 100.0f, css_length.calc_px);
    }

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
        case CSSUnit::VW: {
            // 视口宽度单位
            float vw = ViewportSize::GetWidth();
            return LengthPercentageAuto::Length(css_length.value * vw / 100.0f);
        }
        case CSSUnit::VH: {
            // 视口高度单位
            float vh = ViewportSize::GetHeight();
            return LengthPercentageAuto::Length(css_length.value * vh / 100.0f);
        }
        case CSSUnit::VMIN: {
            // 视口最小尺寸单位
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            float vmin = std::min(vw, vh);
            return LengthPercentageAuto::Length(css_length.value * vmin / 100.0f);
        }
        case CSSUnit::VMAX: {
            // 视口最大尺寸单位
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            float vmax = std::max(vw, vh);
            return LengthPercentageAuto::Length(css_length.value * vmax / 100.0f);
        }
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
    } else if (computed.position == "fixed") {
        // Fixed 定位在布局时当作 absolute 处理，渲染时相对视口
        style.position = Position::Fixed;
    } else if (computed.position == "sticky") {
        // Sticky 定位在布局时当作 relative 处理，滚动时特殊处理
        style.position = Position::Sticky;
    } else {
        style.position = Position::Relative;
    }

    // Box sizing
    if (computed.box_sizing == "border-box") {
        style.box_sizing = BoxSizing::BorderBox;
    } else {
        style.box_sizing = BoxSizing::ContentBox;
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

    // Border widths - 如果独立属性是 0，使用 border.width 作为回退值
    float fallback_border = computed.border.width.ToPx(0, computed.font_size);
    style.border.left = LengthPercentage::Length(computed.border_left_width > 0 ? computed.border_left_width : fallback_border);
    style.border.right = LengthPercentage::Length(computed.border_right_width > 0 ? computed.border_right_width : fallback_border);
    style.border.top = LengthPercentage::Length(computed.border_top_width > 0 ? computed.border_top_width : fallback_border);
    style.border.bottom = LengthPercentage::Length(computed.border_bottom_width > 0 ? computed.border_bottom_width : fallback_border);

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
    style.order = computed.order;

    // Alignment - Note: flex-start/flex-end are different from start/end in reverse layouts
    if (computed.justify_content == "flex-start") {
        style.justify_content = JustifyContent::FlexStart;
    } else if (computed.justify_content == "start") {
        style.justify_content = JustifyContent::Start;
    } else if (computed.justify_content == "flex-end") {
        style.justify_content = JustifyContent::FlexEnd;
    } else if (computed.justify_content == "end") {
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

    if (computed.align_items == "flex-start") {
        style.align_items = AlignItems::FlexStart;
    } else if (computed.align_items == "start") {
        style.align_items = AlignItems::Start;
    } else if (computed.align_items == "flex-end") {
        style.align_items = AlignItems::FlexEnd;
    } else if (computed.align_items == "end") {
        style.align_items = AlignItems::End;
    } else if (computed.align_items == "center") {
        style.align_items = AlignItems::Center;
    } else if (computed.align_items == "baseline") {
        style.align_items = AlignItems::Baseline;
    } else if (computed.align_items == "stretch") {
        style.align_items = AlignItems::Stretch;
    }

    if (computed.align_content == "flex-start") {
        style.align_content = AlignContent::FlexStart;
    } else if (computed.align_content == "start") {
        style.align_content = AlignContent::Start;
    } else if (computed.align_content == "flex-end") {
        style.align_content = AlignContent::FlexEnd;
    } else if (computed.align_content == "end") {
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

    // Grid alignment (justify-items, justify-self)
    if (computed.justify_items == "start") {
        style.justify_items = AlignItems::Start;
    } else if (computed.justify_items == "end") {
        style.justify_items = AlignItems::End;
    } else if (computed.justify_items == "center") {
        style.justify_items = AlignItems::Center;
    } else if (computed.justify_items == "stretch") {
        style.justify_items = AlignItems::Stretch;
    }

    if (computed.justify_self == "start") {
        style.justify_self = AlignSelf::Start;
    } else if (computed.justify_self == "end") {
        style.justify_self = AlignSelf::End;
    } else if (computed.justify_self == "center") {
        style.justify_self = AlignSelf::Center;
    } else if (computed.justify_self == "stretch") {
        style.justify_self = AlignSelf::Stretch;
    }

    // Gap
    style.gap.width = ConvertLength(computed.column_gap);
    style.gap.height = ConvertLength(computed.row_gap);

    // Overflow
    auto parseOverflow = [](const std::string& val) -> Overflow {
        if (val == "hidden") return Overflow::Hidden;
        if (val == "scroll") return Overflow::Scroll;
        if (val == "clip") return Overflow::Clip;
        // Note: "auto" is not directly supported in Taffy's Overflow enum,
        // treat it as Visible for layout purposes (scrollbar only appears when needed)
        return Overflow::Visible;
    };

    // Use overflow-x/overflow-y if set, otherwise fall back to overflow
    std::string overflow_x = !computed.overflow_x.empty() ? computed.overflow_x : computed.overflow;
    std::string overflow_y = !computed.overflow_y.empty() ? computed.overflow_y : computed.overflow;

    style.overflow.x = parseOverflow(overflow_x);
    style.overflow.y = parseOverflow(overflow_y);

    // Set scrollbar width when overflow is scroll
    // For overflow: auto, we don't reserve space in layout (scrollbar appears only when needed)
    if (style.overflow.x == Overflow::Scroll || style.overflow.y == Overflow::Scroll) {
        style.scrollbar_width = RenderObject::GetScrollbarWidth();
    }

    return style;
}

//------------------------------------------------------------------------------
// Private: Tree Building
//------------------------------------------------------------------------------

// Helper function to check if a text is whitespace-only (space, tab, newline)
static bool IsWhitespaceOnly(const std::string& text) {
    for (char c : text) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            return false;
        }
    }
    return true;
}

// Helper function to check if a render object is inline-level
// Note: Pure whitespace text nodes in block context should be ignored
static bool IsInlineLevelElement(RenderObject* render_obj) {
    if (!render_obj) return false;
    RenderObjectType type = render_obj->GetType();
    RenderObjectType display = render_obj->GetComputedStyle().display;
    
    // For text nodes, check if it's whitespace-only
    // In block formatting context, whitespace-only text between block elements
    // should be ignored (CSS white-space processing)
    if (type == RenderObjectType::TEXT) {
        auto* text_obj = static_cast<RenderText*>(render_obj);
        if (text_obj && IsWhitespaceOnly(text_obj->GetText())) {
            return false;  // Ignore whitespace-only text in mixed block/inline context
        }
        return true;
    }
    
    return display == RenderObjectType::INLINE ||
           display == RenderObjectType::INLINE_BLOCK;
}

// Helper function to check if a render object is block-level
static bool IsBlockLevelElement(RenderObject* render_obj) {
    if (!render_obj) return false;
    RenderObjectType type = render_obj->GetType();
    if (type == RenderObjectType::TEXT) return false;
    
    RenderObjectType display = render_obj->GetComputedStyle().display;
    return display == RenderObjectType::BLOCK ||
           display == RenderObjectType::FLEX ||
           display == RenderObjectType::GRID ||
           display == RenderObjectType::TABLE;
}

void NativeLayoutEngine::BuildSubtreeAtIndex(RenderObject* render_obj, NodeId parent_id, size_t insert_index) {
    if (!render_obj) {
        return;
    }

    // 检查节点是否已存在，避免重复创建
    if (HasElement(render_obj)) {
        return;
    }

    NodeId node_id = CreateNode(render_obj);

    // Set as root if no parent
    if (parent_id == 0) {
        root_node_ = node_id;
    } else {
        LayoutNode* parent = GetNode(parent_id);
        if (parent) {
            // 关键修复：在指定位置插入，而不是总是添加到末尾
            if (insert_index < parent->children.size()) {
                parent->children.insert(parent->children.begin() + insert_index, node_id);
            } else {
                parent->children.push_back(node_id);
            }
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

    // 处理子节点（与 BuildSubtree 相同的逻辑）
    const auto& children = render_obj->GetChildren();
    bool has_block = false;
    bool has_inline = false;
    
    for (const auto& child : children) {
        if (IsBlockLevelElement(child.get())) {
            has_block = true;
        }
        if (IsInlineLevelElement(child.get())) {
            has_inline = true;
        }
    }
    
    if (has_block && has_inline) {
        std::vector<RenderObject*> current_inline_run;
        
        for (const auto& child : children) {
            if (IsBlockLevelElement(child.get())) {
                if (!current_inline_run.empty()) {
                    CreateAnonymousBlockBox(node_id, current_inline_run);
                    current_inline_run.clear();
                }
                BuildSubtree(child.get(), node_id);
            } else if (IsInlineLevelElement(child.get())) {
                current_inline_run.push_back(child.get());
            } else {
                BuildSubtree(child.get(), node_id);
            }
        }
        
        if (!current_inline_run.empty()) {
            CreateAnonymousBlockBox(node_id, current_inline_run);
        }
    } else {
        for (auto& child : children) {
            BuildSubtree(child.get(), node_id);
        }
    }
}

void NativeLayoutEngine::BuildSubtree(RenderObject* render_obj, NodeId parent_id) {
    if (!render_obj) {
        return;
    }

    // 检查节点是否已存在，避免重复创建
    if (HasElement(render_obj)) {
        return;
    }

    // 调试：检查 result 类元素
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

    // ========== CSS Anonymous Block Box Implementation ==========
    // According to CSS spec, when a block container contains both block-level
    // and inline-level content, anonymous block boxes are created to wrap
    // consecutive inline-level content.
    //
    // Example:
    //   <div>
    //     <h2>Title</h2>           <!-- block -->
    //     <button>A</button>       <!-- inline-block --> 
    //     <button>B</button>       <!-- inline-block --> wrapped in anonymous block
    //     <button>C</button>       <!-- inline-block -->
    //   </div>
    //
    // The buttons should be wrapped in an anonymous block box that uses IFC layout.
    
    const auto& children = render_obj->GetChildren();
    bool has_block = false;
    bool has_inline = false;
    
    // First pass: check if we have mixed content
    for (const auto& child : children) {
        if (IsBlockLevelElement(child.get())) {
            has_block = true;
        }
        if (IsInlineLevelElement(child.get())) {
            has_inline = true;
        }
    }
    
    // If we have mixed content, we need to create anonymous block boxes
    if (has_block && has_inline) {
        // Debug output
        auto dom_node = render_obj->GetNode();
        std::string tag_name = "unknown";
        if (dom_node && dom_node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(dom_node);
            if (element) tag_name = element->GetTagName();
        }
        std::cout << "[AnonymousBlock] Creating anonymous blocks for <" << tag_name 
                  << "> with " << children.size() << " children (has_block=" << has_block 
                  << ", has_inline=" << has_inline << ")" << std::endl;
        
        std::vector<RenderObject*> current_inline_run;
        
        for (const auto& child : children) {
            if (IsBlockLevelElement(child.get())) {
                // If we have accumulated inline elements, create an anonymous block for them
                if (!current_inline_run.empty()) {
                    CreateAnonymousBlockBox(node_id, current_inline_run);
                    current_inline_run.clear();
                }
                // Process the block element normally
                BuildSubtree(child.get(), node_id);
            } else if (IsInlineLevelElement(child.get())) {
                // Accumulate inline elements
                current_inline_run.push_back(child.get());
            } else {
                // Other elements (display:none, etc.) - process normally
                BuildSubtree(child.get(), node_id);
            }
        }
        
        // Don't forget the last run of inline elements
        if (!current_inline_run.empty()) {
            CreateAnonymousBlockBox(node_id, current_inline_run);
        }
    } else {
        // No mixed content, process children normally
        for (auto& child : children) {
            BuildSubtree(child.get(), node_id);
        }
    }
}

NodeId NativeLayoutEngine::CreateAnonymousBlockBox(NodeId parent_id, const std::vector<RenderObject*>& inline_children) {
    if (inline_children.empty()) return 0;
    
    // Create a new node ID for the anonymous block
    NodeId anon_id = next_node_id_++;
    
    LayoutNode anon_node;
    anon_node.id = anon_id;
    anon_node.render_obj = nullptr;  // Anonymous blocks have no render object
    anon_node.parent = parent_id;
    anon_node.is_anonymous_block = true;
    anon_node.is_ifc_container = true;  // Anonymous blocks use IFC for their inline content
    
    // Store the inline children for later IFC layout
    for (RenderObject* child : inline_children) {
        anon_node.anonymous_inline_children.push_back(child);
    }
    
    // Set up default block style for the anonymous block
    // Note: Position::Relative is used as the default (equivalent to static in this engine)
    anon_node.style.display = Display::Block;
    anon_node.style.box_sizing = BoxSizing::ContentBox;
    anon_node.style.position = Position::Relative;  // static position (default)
    anon_node.style.size = Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
    anon_node.style.min_size = Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
    anon_node.style.max_size = Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
    anon_node.style.padding = Rect<LengthPercentage>::Zero();
    anon_node.style.border = Rect<LengthPercentage>::Zero();
    anon_node.style.margin = Rect<LengthPercentageAuto>::Zero();
    
    // Copy block container/item styles
    anon_node.block_container_style.display = Display::Block;
    anon_node.block_container_style.box_sizing = BoxSizing::ContentBox;
    anon_node.block_container_style.position = Position::Relative;  // static position (default)
    anon_node.block_container_style.size = Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
    anon_node.block_container_style.min_size = Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
    anon_node.block_container_style.max_size = Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
    anon_node.block_container_style.padding = Rect<LengthPercentage>::Zero();
    anon_node.block_container_style.border = Rect<LengthPercentage>::Zero();
    anon_node.block_container_style.margin = Rect<LengthPercentageAuto>::Zero();
    
    anon_node.block_item_style.display = Display::Block;
    anon_node.block_item_style.box_sizing = BoxSizing::ContentBox;
    anon_node.block_item_style.position = Position::Relative;  // static position (default)
    anon_node.block_item_style.size = Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
    anon_node.block_item_style.min_size = Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
    anon_node.block_item_style.max_size = Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
    anon_node.block_item_style.padding = Rect<LengthPercentage>::Zero();
    anon_node.block_item_style.border = Rect<LengthPercentage>::Zero();
    anon_node.block_item_style.margin = Rect<LengthPercentageAuto>::Zero();
    
    // Store the node
    nodes_[anon_id] = std::move(anon_node);
    
    // Add to parent's children
    LayoutNode* parent = GetNode(parent_id);
    if (parent) {
        parent->children.push_back(anon_id);
    }
    
    return anon_id;
}

//------------------------------------------------------------------------------
// Private: Layout Computation
//------------------------------------------------------------------------------

LayoutOutput NativeLayoutEngine::ComputeNodeLayout(NodeId node_id, const LayoutInput& inputs) {
    LayoutNode* node = GetNode(node_id);
    if (!node) {
        return LayoutOutput{};
    }

    // Check cache (pass content_version for incremental layout invalidation)
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 1.4, 1.5**
    auto cached = node->cache.Get(
        inputs.known_dimensions,
        inputs.available_space,
        inputs.run_mode,
        node->content_version
    );
    if (cached.has_value()) {
        return *cached;
    }

    LayoutOutput output;

    // Check if this is a leaf node (text, inline-block, etc.)
    // Leaf nodes need special measurement handling
    if (node->render_obj) {
        RenderObjectType type = node->render_obj->GetType();

        if (type == RenderObjectType::TEXT ||
            type == RenderObjectType::INLINE_BLOCK ||
            type == RenderObjectType::INLINE) {
            output = MeasureLeafNode(node_id, inputs);

            // Store in cache and return early (pass content_version for incremental layout)
            // **Feature: incremental-layout-optimization**
            // **Validates: Requirements 1.4, 1.5**
            node->cache.Store(
                inputs.known_dimensions,
                inputs.available_space,
                inputs.run_mode,
                node->content_version,
                output
            );
            node->output = output;
            return output;
        }

        // Handle TABLE elements - use ComputeTableLayout
        if (type == RenderObjectType::TABLE) {
            output = ComputeTableLayout(node_id, inputs);
            return output;
        }


        // Handle LEGEND elements - they should use fit-content width
        auto dom_node = node->render_obj->GetNode();
        if (dom_node && dom_node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(dom_node);
            if (element && element->GetTagName() == "legend") {
                // Legend should use fit-content width (intrinsic sizing)
                // First measure content with MaxContent to get intrinsic width
                LayoutInput measure_inputs = inputs;
                measure_inputs.available_space.width = AvailableSpace::MaxContent();
                measure_inputs.known_dimensions.width = std::nullopt;

                // Compute layout to get intrinsic content size
                // Use IFC layout if legend is an IFC container (contains only text)
                LayoutOutput intrinsic;
                if (node->is_ifc_container) {
                    intrinsic = ComputeIFCLayout(node_id, measure_inputs);
                } else {
                    intrinsic = ComputeBlockLayout(node_id, measure_inputs);
                }

                // Now do actual layout with the intrinsic width
                LayoutInput final_inputs = inputs;
                final_inputs.known_dimensions.width = intrinsic.size.width;
                if (node->is_ifc_container) {
                    output = ComputeIFCLayout(node_id, final_inputs);
                } else {
                    output = ComputeBlockLayout(node_id, final_inputs);
                }

                // Store in cache and return (pass content_version for incremental layout)
                // **Feature: incremental-layout-optimization**
                // **Validates: Requirements 1.4, 1.5**
                node->cache.Store(
                    inputs.known_dimensions,
                    inputs.available_space,
                    inputs.run_mode,
                    node->content_version,
                    output
                );
                node->output = output;
                return output;
            }
        }
    }

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

    // Store in cache (pass content_version for incremental layout invalidation)
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 1.4, 1.5**
    node->cache.Store(
        inputs.known_dimensions,
        inputs.available_space,
        inputs.run_mode,
        node->content_version,
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

LayoutOutput NativeLayoutEngine::ComputeTableLayout(NodeId node_id, const LayoutInput& inputs) {
    LayoutNode* node = GetNode(node_id);
    if (!node || !node->render_obj) {
        return LayoutOutput{};
    }

    LayoutOutput output;

    // TABLE 布局委托给 RenderTable::Layout
    auto* table = static_cast<RenderTable*>(node->render_obj);

    // 确定可用宽度
    float available_width = 0.0f;
    if (inputs.known_dimensions.width.has_value()) {
        available_width = *inputs.known_dimensions.width;
    } else if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
        available_width = inputs.available_space.width.value;
    } else if (inputs.available_space.width.type == AvailableSpace::Type::MaxContent) {
        available_width = 10000.0f;  // 大值表示无限宽度
    } else {
        // MinContent 模式下，表格使用最小内容宽度
        available_width = 0.0f;
    }

    // 调用表格自身的布局方法
    table->Layout(available_width, 0);

    // 从表格的布局信息获取计算后的尺寸
    const auto& table_layout = table->GetLayoutInfo();
    output.size.width = table_layout.width;
    output.size.height = table_layout.height;
    output.content_size = output.size;

    // 存储到缓存 (pass content_version for incremental layout invalidation)
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 1.4, 1.5**
    node->cache.Store(
        inputs.known_dimensions,
        inputs.available_space,
        inputs.run_mode,
        node->content_version,
        output
    );
    node->output = output;

    return output;
}

LayoutOutput NativeLayoutEngine::ComputeIFCLayout(NodeId node_id, const LayoutInput& inputs) {
    LayoutNode* node = GetNode(node_id);
    if (!node) {
        return LayoutOutput{};
    }
    
    // Handle anonymous block boxes (they have no render_obj but contain inline children)
    if (node->is_anonymous_block) {
        return ComputeAnonymousBlockIFCLayout(node_id, inputs);
    }
    
    if (!node->render_obj) {
        return LayoutOutput{};
    }

    const auto& style = node->render_obj->GetComputedStyle();



    // Resolve container width - prefer known_dimensions (fixed width) over available_space
    // This is critical for text-align: center to work correctly
    float container_width = 0.0f;
    if (inputs.known_dimensions.width.has_value()) {
        // Container has a fixed width (e.g., width: 200px)
        container_width = *inputs.known_dimensions.width;
    } else {
        // For InherentSize mode, check CSS width property FIRST
        // This is important for Grid children with explicit width/height
        if (inputs.sizing_mode == SizingMode::InherentSize) {
            if (style.width.unit == CSSUnit::PX && style.width.value > 0) {
                container_width = style.width.value;
            } else if (style.width.unit == CSSUnit::PERCENT && inputs.parent_size.width.has_value()) {
                container_width = (style.width.value / 100.0f) * (*inputs.parent_size.width);
            }
        }

        // If no CSS width, fall back to available_space
        if (container_width == 0.0f) {
            if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
                container_width = inputs.available_space.width.value;
            } else if (inputs.available_space.width.type == AvailableSpace::Type::MaxContent) {
                container_width = 10000.0f;
            } else if (inputs.available_space.width.type == AvailableSpace::Type::MinContent) {
                // For MinContent, we need to calculate the minimum width needed
                // This is the width of the longest word in the text
                // We'll use a very small width to force line breaking at every opportunity
                container_width = 0.0f;  // Will be handled specially below
            }
        }
    }

    // Handle MinContent mode specially
    bool is_min_content = (inputs.available_space.width.type == AvailableSpace::Type::MinContent) &&
                          !inputs.known_dimensions.width.has_value();

    // Resolve padding and border
    // Note: CSS padding percentages are always relative to the containing block's WIDTH (not height)
    // So we use container_width for all padding values
    float padding_left = style.padding.left.ToPx(container_width, style.font_size);
    float padding_right = style.padding.right.ToPx(container_width, style.font_size);
    float padding_top = style.padding.top.ToPx(container_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(container_width, style.font_size);

    float border_left = style.border_left_width;
    float border_right = style.border_right_width;
    float border_top = style.border_top_width;
    float border_bottom = style.border_bottom_width;
    if (border_left == 0 && border_right == 0 && border_top == 0 && border_bottom == 0) {
        float border_width = style.border.width.ToPx(container_width, style.font_size);
        border_left = border_right = border_top = border_bottom = border_width;
    }

    // Calculate scrollbar gutter for overflow: scroll or auto
    float scrollbar_gutter_right = 0.0f;
    float scrollbar_gutter_bottom = 0.0f;
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    if (overflow_y == "scroll") {
        scrollbar_gutter_right = RenderObject::GetScrollbarWidth();
    }
    if (overflow_x == "scroll") {
        scrollbar_gutter_bottom = RenderObject::GetScrollbarWidth();
    }

    // Calculate content area width (container width minus padding, border, and scrollbar)
    float content_width = container_width - padding_left - padding_right - border_left - border_right - scrollbar_gutter_right;
    if (content_width < 0) content_width = 0;

    float total_width = 0.0f;
    float total_height = 0.0f;
    IFCLayoutResult result;

    // Only apply layout results (update render object positions) in PerformLayout mode
    // In ComputeSize mode, we only need to measure the size, not update positions
    bool apply_results = (inputs.run_mode == RunMode::PerformLayout);

    if (is_min_content) {
        // For MinContent, calculate the minimum width needed to display the content
        // This is the width of the longest word in the text
        float min_content_width = ifc_layout_.MeasureMinContentWidth(node->render_obj);

        // Now layout with this minimum width to get the height
        // Pass content_version from LayoutNode for incremental layout cache validation
        result = ifc_layout_.Layout(node->render_obj, min_content_width, apply_results, node->content_version);

        total_width = min_content_width + padding_left + padding_right + border_left + border_right;
        total_height = result.total_height + padding_top + padding_bottom + border_top + border_bottom;
    } else {
        // Use IFC to compute content layout with the correct content width
        // Pass content_version from LayoutNode for incremental layout cache validation
        result = ifc_layout_.Layout(node->render_obj, content_width, apply_results, node->content_version);

        // Calculate total size including padding and border
        total_width = result.max_width + padding_left + padding_right + border_left + border_right;
        total_height = result.total_height + padding_top + padding_bottom + border_top + border_bottom;
    }

    // Apply known dimensions if provided (override calculated size)
    float fixed_width = 0.0f;
    float fixed_height = 0.0f;
    if (inputs.known_dimensions.width.has_value()) {
        fixed_width = *inputs.known_dimensions.width;
        total_width = fixed_width;
    }
    if (inputs.known_dimensions.height.has_value()) {
        fixed_height = *inputs.known_dimensions.height;
        total_height = fixed_height;
    }

    // For InherentSize mode, also check the node's own CSS width/height properties
    // This is important for Grid children with explicit width/height
    // Note: We use render_obj->GetComputedStyle() directly to get the latest style,
    // because the node->style may be stale if the DOM was rebuilt
    // IMPORTANT: Only apply CSS width/height if known_dimensions is NOT set!
    // known_dimensions takes precedence (e.g., from flexbox target_size)
    if (inputs.sizing_mode == SizingMode::InherentSize) {
        const auto& computed = node->render_obj->GetComputedStyle();
        // Check CSS width property - only if known_dimensions.width is not set
        if (!inputs.known_dimensions.width.has_value()) {
            if (computed.width.unit == CSSUnit::PX && computed.width.value > 0) {
                total_width = computed.width.value;
            } else if (computed.width.unit == CSSUnit::PERCENT && inputs.parent_size.width.has_value()) {
                total_width = (computed.width.value / 100.0f) * (*inputs.parent_size.width);
            }
        }
        // Check CSS height property - only if known_dimensions.height is not set
        if (!inputs.known_dimensions.height.has_value()) {
            if (computed.height.unit == CSSUnit::PX && computed.height.value > 0) {
                total_height = computed.height.value;
            } else if (computed.height.unit == CSSUnit::PERCENT && inputs.parent_size.height.has_value()) {
                total_height = (computed.height.value / 100.0f) * (*inputs.parent_size.height);
            }
        }
    }

    // Handle overflow: auto - if content exceeds container, add scrollbar and relayout
    if (overflow_y == "auto" && fixed_height > 0 && scrollbar_gutter_right == 0.0f) {
        // Calculate content area height
        float content_area_height = fixed_height - padding_top - padding_bottom - border_top - border_bottom;
        // Check if content exceeds container height
        if (result.total_height > content_area_height) {
            // Need vertical scrollbar - reduce content width and relayout
            scrollbar_gutter_right = RenderObject::GetScrollbarWidth();
            content_width = container_width - padding_left - padding_right - border_left - border_right - scrollbar_gutter_right;
            if (content_width < 0) content_width = 0;

            // Clear IFC cache and relayout
            ifc_layout_.ClearCache();
            // Pass content_version from LayoutNode for incremental layout cache validation
            result = ifc_layout_.Layout(node->render_obj, content_width, true, node->content_version);
        }
    }

    // Handle overflow-x: auto - if content exceeds container width, add horizontal scrollbar height
    if (overflow_x == "auto" && scrollbar_gutter_bottom == 0.0f) {
        // Calculate content area width
        float content_area_width = content_width;
        // Check if content exceeds container width
        if (result.max_width > content_area_width) {
            // Need horizontal scrollbar - add scrollbar height to total height
            scrollbar_gutter_bottom = RenderObject::GetScrollbarWidth();
            total_height += scrollbar_gutter_bottom;
        }
    }

    // Apply min/max constraints
    // Note: min-height and max-height need to be applied even for IFC containers
    // CSS spec: when min > max, min wins (apply max first, then min)
    float min_height = style.min_height.ToPx(0, style.font_size);
    float max_height = style.max_height.ToPx(0, style.font_size);
    float min_width = style.min_width.ToPx(container_width, style.font_size);
    float max_width = style.max_width.ToPx(container_width, style.font_size);

    // Apply max first, then min - ensures min wins when min > max
    if (max_height > 0) {
        total_height = std::min(total_height, max_height);
    }
    if (min_height > 0) {
        total_height = std::max(total_height, min_height);
    }
    if (max_width > 0) {
        total_width = std::min(total_width, max_width);
    }
    if (min_width > 0) {
        total_width = std::max(total_width, min_width);
    }

    LayoutOutput output;
    output.size = Size<float>{total_width, total_height};
    output.content_size = Size<float>{result.max_width, result.total_height};

    return output;
}

LayoutOutput NativeLayoutEngine::ComputeAnonymousBlockIFCLayout(NodeId node_id, const LayoutInput& inputs) {
    LayoutNode* node = GetNode(node_id);
    if (!node || !node->is_anonymous_block || node->anonymous_inline_children.empty()) {
        return LayoutOutput{};
    }
    
    // Anonymous blocks inherit text-align from parent
    // Get parent node to inherit styles
    LayoutNode* parent = GetNode(node->parent);
    std::string text_align = "left";
    float font_size = 16.0f;
    float line_height = 1.2f;
    std::string font_family = "Arial";
    
    if (parent && parent->render_obj) {
        const auto& parent_style = parent->render_obj->GetComputedStyle();
        text_align = parent_style.text_align;
        font_size = parent_style.font_size;
        line_height = parent_style.line_height;
        font_family = parent_style.font_family;
    }
    
    // Resolve container width
    float container_width = 0.0f;
    if (inputs.known_dimensions.width.has_value()) {
        container_width = *inputs.known_dimensions.width;
    } else if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
        container_width = inputs.available_space.width.value;
    } else if (inputs.available_space.width.type == AvailableSpace::Type::MaxContent) {
        container_width = 10000.0f;
    }
    
    // Anonymous blocks have no padding/border/margin
    float content_width = container_width;
    
    // Use IFC to layout the inline children
    // We need to create a temporary container for IFC layout
    // The inline children are stored in anonymous_inline_children
    
    float total_width = 0.0f;
    float total_height = 0.0f;
    
    // Only apply layout results in PerformLayout mode
    bool apply_results = (inputs.run_mode == RunMode::PerformLayout);
    
    // Create inline boxes for all inline children and perform IFC layout
    // We'll use the IFCLayout class directly with the inline children
    
    // Clear previous IFC state
    node->ifc_inline_boxes.clear();
    node->ifc_line_boxes.clear();
    
    // Collect inline boxes from all inline children
    std::vector<InlineBox> all_inline_boxes;
    
    for (RenderObject* inline_child : node->anonymous_inline_children) {
        if (!inline_child) continue;
        
        RenderObjectType child_type = inline_child->GetType();
        const auto& child_style = inline_child->GetComputedStyle();
        
        if (child_type == RenderObjectType::TEXT) {
            // Text node - create text inline box
            RenderText* text_obj = static_cast<RenderText*>(inline_child);
            const std::string& text = text_obj->GetText();
            if (text.empty()) continue;
            
            // Measure text
            float letter_spacing = child_style.letter_spacing.ToPx(0, child_style.font_size);
            float word_spacing = child_style.word_spacing.ToPx(0, child_style.font_size);
            auto measurement = IFCLayout::MeasureTextStatic(
                text, child_style.font_size, child_style.font_family,
                letter_spacing, word_spacing, child_style.line_height);
            
            InlineBox box = InlineBox::CreateTextBox(inline_child);
            box.width = measurement.width;
            box.height = measurement.height;
            box.baseline = measurement.skia_ascent;
            box.skia_ascent = measurement.skia_ascent;
            box.skia_descent = measurement.skia_descent;
            box.line_height_multiplier = child_style.line_height;
            
            TextRun run;
            run.text = text;
            run.start_offset = 0;
            run.end_offset = text.size();
            run.width = measurement.width;
            run.height = measurement.height;
            run.baseline = measurement.skia_ascent;
            box.text_runs.push_back(run);
            
            all_inline_boxes.push_back(std::move(box));
        }
        else if (child_type == RenderObjectType::INLINE_BLOCK) {
            // Inline-block element (e.g., button)
            auto* inline_block = static_cast<RenderInlineBlock*>(inline_child);
            auto [w, h] = inline_block->MeasureIntrinsicSize(content_width);
            
            InlineBox box = InlineBox::CreateAtomicBox(inline_child, w, h, h);
            box.margin_left = child_style.margin.left.ToPx(w, child_style.font_size);
            box.margin_right = child_style.margin.right.ToPx(w, child_style.font_size);
            box.line_height_multiplier = child_style.line_height;
            
            all_inline_boxes.push_back(std::move(box));
        }
        else if (child_type == RenderObjectType::INLINE) {
            // Inline element (e.g., span) - recursively collect its children
            // For simplicity, we'll treat it as a container and process its children
            CollectInlineBoxesRecursive(inline_child, all_inline_boxes, content_width);
        }
    }
    
    if (all_inline_boxes.empty()) {
        return LayoutOutput{};
    }
    
    // Use LineBreaker to break into lines
    LineBreaker line_breaker;
    line_breaker.SetWhiteSpace(WhiteSpaceMode::NORMAL);
    line_breaker.SetOverflowWrap(OverflowWrapMode::NORMAL);
    
    // Configure word-break from parent style
    if (parent && parent->render_obj) {
        const auto& parent_style = parent->render_obj->GetComputedStyle();
        if (parent_style.word_break == "break-all") {
            line_breaker.SetWordBreak(WordBreakMode::BREAK_ALL);
        } else if (parent_style.word_break == "keep-all") {
            line_breaker.SetWordBreak(WordBreakMode::KEEP_ALL);
        } else if (parent_style.word_break == "break-word") {
            // word-break: break-word 等同于 overflow-wrap: break-word
            line_breaker.SetWordBreak(WordBreakMode::NORMAL);
            line_breaker.SetOverflowWrap(OverflowWrapMode::BREAK_WORD);
        } else {
            line_breaker.SetWordBreak(WordBreakMode::NORMAL);
        }
    } else {
        line_breaker.SetWordBreak(WordBreakMode::NORMAL);
    }
    
    std::vector<LineBox> line_boxes = line_breaker.BreakIntoLines(all_inline_boxes, content_width);
    
    // Calculate line heights and positions
    VerticalAligner vertical_aligner;
    float current_y = 0.0f;
    float max_line_width = 0.0f;
    
    // Calculate container line-height
    float container_line_height;
    if (std::abs(line_height - 1.2f) < 0.001f) {
        container_line_height = GetBrowserNormalLineHeight(font_size, font_family);
    } else {
        container_line_height = line_height * font_size;
    }
    
    for (auto& line : line_boxes) {
        std::vector<InlineBox*> box_ptrs;
        std::vector<VerticalAlignInfo> aligns;
        
        for (auto* box : line.boxes) {
            box_ptrs.push_back(box);
            VerticalAlignInfo align_info{VerticalAlignType::BASELINE, 0.0f};
            if (box->render_object) {
                const auto& box_style = box->render_object->GetComputedStyle();
                align_info = vertical_aligner.ParseVerticalAlign(
                    box_style.vertical_align, box_style.font_size);
            }
            aligns.push_back(align_info);
        }
        
        auto line_metrics = vertical_aligner.CalculateLineMetrics(box_ptrs, aligns, container_line_height);
        line.height = line_metrics.line_height;
        line.baseline = line_metrics.baseline;
        line.y = current_y;
        
        // Set horizontal positions
        float current_x = line.x;
        for (auto* box : line.boxes) {
            if (!box) continue;
            // 盒子的 x 位置是内容区域的起始位置（在 margin_left 之后）
            // current_x 指向当前可用空间的起始位置
            current_x += box->margin_left;  // 先跳过左边距
            box->x = current_x;             // 内容区域从这里开始
            current_x += box->width + box->margin_right;  // 移动到下一个盒子的起始位置
        }
        
        // Apply vertical alignment
        vertical_aligner.AlignBoxes(box_ptrs, aligns, current_y, container_line_height);
        
        // Apply text-align
        line.ApplyTextAlign(text_align);
        
        current_y += line_metrics.line_height;
        max_line_width = std::max(max_line_width, line.content_width);
    }
    
    total_height = current_y;
    total_width = max_line_width;
    
    // Store inline boxes first (line_boxes contains pointers to all_inline_boxes)
    node->ifc_inline_boxes = std::move(all_inline_boxes);
    
    // Now update line_boxes to point to the new location of inline boxes
    // and store them
    for (auto& line : line_boxes) {
        for (size_t i = 0; i < line.boxes.size(); ++i) {
            // Find the corresponding box in node->ifc_inline_boxes
            // The boxes are in the same order, so we can use index mapping
            // But since line.boxes contains pointers, we need to find by render_object
            InlineBox* old_ptr = line.boxes[i];
            if (old_ptr) {
                for (auto& new_box : node->ifc_inline_boxes) {
                    if (new_box.render_object == old_ptr->render_object &&
                        new_box.type == old_ptr->type) {
                        line.boxes[i] = &new_box;
                        break;
                    }
                }
            }
        }
    }
    node->ifc_line_boxes = std::move(line_boxes);
    
    // Apply layout results to render objects if in PerformLayout mode
    if (apply_results) {
        ApplyAnonymousBlockLayoutResults(node);
    }
    
    LayoutOutput output;
    output.size = Size<float>{total_width, total_height};
    output.content_size = output.size;
    
    return output;
}

// Helper function to recursively collect inline boxes from an inline element
void NativeLayoutEngine::CollectInlineBoxesRecursive(
    RenderObject* render_obj,
    std::vector<InlineBox>& inline_boxes,
    float available_width
) {
    if (!render_obj) return;
    
    const auto& style = render_obj->GetComputedStyle();
    
    // Add INLINE_START marker
    InlineBox start = InlineBox::CreateInlineStart(render_obj);
    inline_boxes.push_back(std::move(start));
    
    // Process children
    for (const auto& child : render_obj->GetChildren()) {
        RenderObjectType child_type = child->GetType();
        const auto& child_style = child->GetComputedStyle();
        
        if (child_type == RenderObjectType::TEXT) {
            RenderText* text_obj = static_cast<RenderText*>(child.get());
            const std::string& text = text_obj->GetText();
            if (text.empty()) continue;
            
            float letter_spacing = child_style.letter_spacing.ToPx(0, child_style.font_size);
            float word_spacing = child_style.word_spacing.ToPx(0, child_style.font_size);
            auto measurement = IFCLayout::MeasureTextStatic(
                text, child_style.font_size, child_style.font_family,
                letter_spacing, word_spacing, child_style.line_height);
            
            InlineBox box = InlineBox::CreateTextBox(child.get());
            box.width = measurement.width;
            box.height = measurement.height;
            box.baseline = measurement.skia_ascent;
            box.skia_ascent = measurement.skia_ascent;
            box.skia_descent = measurement.skia_descent;
            box.line_height_multiplier = child_style.line_height;
            
            TextRun run;
            run.text = text;
            run.start_offset = 0;
            run.end_offset = text.size();
            run.width = measurement.width;
            run.height = measurement.height;
            run.baseline = measurement.skia_ascent;
            box.text_runs.push_back(run);
            
            inline_boxes.push_back(std::move(box));
        }
        else if (child_type == RenderObjectType::INLINE_BLOCK) {
            auto* inline_block = static_cast<RenderInlineBlock*>(child.get());
            auto [w, h] = inline_block->MeasureIntrinsicSize(available_width);
            
            InlineBox box = InlineBox::CreateAtomicBox(child.get(), w, h, h);
            box.margin_left = child_style.margin.left.ToPx(w, child_style.font_size);
            box.margin_right = child_style.margin.right.ToPx(w, child_style.font_size);
            box.line_height_multiplier = child_style.line_height;
            
            inline_boxes.push_back(std::move(box));
        }
        else if (child_type == RenderObjectType::INLINE) {
            CollectInlineBoxesRecursive(child.get(), inline_boxes, available_width);
        }
    }
    
    // Add INLINE_END marker
    InlineBox end = InlineBox::CreateInlineEnd(render_obj);
    inline_boxes.push_back(std::move(end));
}

// Apply layout results from anonymous block to render objects
void NativeLayoutEngine::ApplyAnonymousBlockLayoutResults(LayoutNode* node) {
    if (!node || !node->is_anonymous_block) return;
    
    // Get parent's position for offset calculation
    LayoutNode* parent = GetNode(node->parent);
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    
    float padding_left = 0, padding_top = 0, border_left = 0, border_top = 0;
    if (parent && parent->render_obj) {
        const auto& parent_style = parent->render_obj->GetComputedStyle();
        // Anonymous block is positioned relative to parent's content area
        // Add parent's padding and border
        float parent_width = parent->layout.size.width;
        padding_left = parent_style.padding.left.ToPx(parent_width, parent_style.font_size);
        border_left = parent_style.border_left_width > 0 ? parent_style.border_left_width :
                    parent_style.border.width.ToPx(parent_width, parent_style.font_size);
        padding_top = parent_style.padding.top.ToPx(parent_width, parent_style.font_size);
        border_top = parent_style.border_top_width > 0 ? parent_style.border_top_width :
                    parent_style.border.width.ToPx(parent_width, parent_style.font_size);
        offset_x = padding_left + border_left;
        offset_y = padding_top + border_top;
    }
    
    // Add the anonymous block's own position
    // Note: node->layout.location is the position of the anonymous block
    // relative to parent's content area (after padding+border)
    // So we should NOT add padding/border again here
    std::cout << "[ApplyAnonymousBlock] node->layout.location=(" << node->layout.location.x 
              << "," << node->layout.location.y << ") padding=(" << padding_left << "," << padding_top
              << ") border=(" << border_left << "," << border_top << ")" << std::endl;
    
    // 问题分析：node->layout.location 可能已经是相对于内容区域的位置
    // 不需要再加 padding/border
    // 暂时只使用 node->layout.location
    offset_x = node->layout.location.x;
    offset_y = node->layout.location.y;
    
    // Apply positions to all inline boxes
    for (const auto& box : node->ifc_inline_boxes) {
        if (!box.render_object) continue;
        
        if (box.type == InlineBoxType::TEXT || box.type == InlineBoxType::ATOMIC) {
            LayoutInfo& layout = box.render_object->GetLayoutInfo();
            // box.x 已经是内容区域的起始位置（margin_left 已经在布局时处理过了）
            layout.x = box.x + offset_x;
            layout.y = box.y + offset_y;
            layout.width = box.width;
            layout.height = box.height;
            layout.is_laid_out = true;  // 标记为已布局，防止 ReadLayoutResults 覆盖
            
            // For inline-block elements, call Layout to position children
            if (box.render_object->GetType() == RenderObjectType::INLINE_BLOCK) {
                auto* inline_block = static_cast<RenderInlineBlock*>(box.render_object);
                inline_block->Layout(box.width, box.height);
            }
        }
    }
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
        bool is_min_content = false;

        // Check white-space property - nowrap and pre disable wrapping
        const std::string& white_space = style.white_space;
        bool wrap_allowed = (white_space != "nowrap" && white_space != "pre");

        if (wrap_allowed) {
            if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
                available_width = inputs.available_space.width.value;
                should_wrap = true;
            } else if (inputs.available_space.width.type == AvailableSpace::Type::MinContent) {
                is_min_content = true;
                should_wrap = true;
            }
        }

        LayoutOutput output;

        // 计算 line-height
        // 如果 style.line_height 是默认值 1.2，使用浏览器风格的 line-height: normal
        // 否则使用用户指定的 line-height 倍数
        float line_height;
        if (std::abs(style.line_height - 1.2f) < 0.001f) {
            // 使用浏览器风格的 line-height: normal
            line_height = GetBrowserNormalLineHeight(style.font_size, style.font_family);
        } else {
            // 用户指定了具体的 line-height
            line_height = style.line_height * style.font_size;
        }

        if (is_min_content) {
            // For MinContent, return the width of the longest word
            // This is the minimum width needed to display the text without overflow
            float max_word_width = text_renderer.MeasureMinContentWidth(text, font);
            output.size.width = max_word_width;
            output.size.height = line_height;
            text_obj->SetActualTextWidth(max_word_width);
        } else if (should_wrap && available_width > 0) {
            std::vector<std::string> lines = text_renderer.WrapText(text, available_width, font);
            text_obj->SetWrappedLines(lines);

            float max_line_width = 0.0f;
            for (const auto& line : lines) {
                float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
                max_line_width = f32_max(max_line_width, line_width);
            }

            output.size.width = max_line_width;
            output.size.height = lines.size() * line_height;
            text_obj->SetActualTextWidth(max_line_width);
        } else {
            text_obj->SetWrappedLines({});
            float text_width = text_renderer.MeasureTextWidthWithEmoji(text, font);
            output.size.width = text_width;
            output.size.height = line_height;
            text_obj->SetActualTextWidth(text_width);
        }

        output.content_size = output.size;
        return output;
    }

    // ✅ 修复：Handle inline elements (e.g., span in mixed content containers)
    // Mixed content containers (containing both inline and block elements) don't use IFC,
    // so inline elements need to be measured here.
    if (type == RenderObjectType::INLINE) {
        float available_width = 0.0f;
        if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
            available_width = inputs.available_space.width.value;
        } else if (inputs.available_space.width.type == AvailableSpace::Type::MaxContent) {
            available_width = 10000.0f;
        }

        auto* inline_obj = static_cast<RenderInline*>(render_obj);
        auto [width, height] = inline_obj->MeasureIntrinsicSize(available_width);

        // Update layout_info_ so DevTools can display correct size
        LayoutInfo& layout = inline_obj->GetLayoutInfo();
        layout.width = width;
        layout.height = height;
       layout.is_laid_out = true;

        LayoutOutput output;
        output.size = Size<float>{width, height};
        output.content_size = output.size;
        return output;
    }

    // Handle inline-block elements
    if (type == RenderObjectType::INLINE_BLOCK) {
        float available_width = 0.0f;
        if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
            available_width = inputs.available_space.width.value;
        } else if (inputs.available_space.width.type == AvailableSpace::Type::MaxContent) {
            available_width = 10000.0f;
        }

        // Check if this is an SVG element
        auto* svg_root = dynamic_cast<RenderSVGRoot*>(render_obj);
        if (svg_root) {
            auto [width, height] = svg_root->MeasureIntrinsicSize(available_width);
            LayoutOutput output;
            output.size = Size<float>{width, height};
            output.content_size = output.size;
            return output;
        }

        // Regular inline-block element
        auto* inline_block = static_cast<RenderInlineBlock*>(render_obj);
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
        // 没有映射，可能是以下几种情况：
        // 1. IFC 容器的子元素
        // 2. 匿名块盒管理的内联元素（它们的布局已经由 ApplyAnonymousBlockLayoutResults 设置）
        
        LayoutInfo& info = render_obj->GetLayoutInfo();
        
        // 检查该元素是否已经被布局（由匿名块盒处理）
        // 如果 is_laid_out 为 true，说明已经由匿名块盒或 IFC 布局过，不需要再递归处理
        if (info.is_laid_out) {
            return;
        }
        
        // 否则，递归处理子元素（用于 IFC 容器等情况）
        for (auto& child : render_obj->GetChildren()) {
            ReadLayoutResults(child.get());
        }
        return;
    }

    LayoutNode* node = GetNode(it->second);
    if (!node) {
        return;
    }

    // Update render object with layout info
    LayoutInfo& info = render_obj->GetLayoutInfo();

    // For TABLE internal elements (ROW_GROUP, ROW, CELL, etc.), their positions
    // are managed by RenderTable::Layout, not NativeLayoutEngine.
    // We should NOT overwrite their x/y values, only update width/height.
    RenderObjectType type = render_obj->GetType();
    bool is_table_internal = (type == RenderObjectType::TABLE_ROW_GROUP ||
                              type == RenderObjectType::TABLE_HEADER_GROUP ||
                              type == RenderObjectType::TABLE_FOOTER_GROUP ||
                              type == RenderObjectType::TABLE_ROW ||
                              type == RenderObjectType::TABLE_CELL ||
                              type == RenderObjectType::TABLE_CAPTION);

    if (!is_table_internal) {
        // Normal elements: update all layout info from NativeLayoutEngine
        // Use layout.location which contains the position set by the block layout algorithm
        info.x = node->layout.location.x;
        info.y = node->layout.location.y;
        info.width = node->output.size.width;
        info.height = node->output.size.height;
    }
    // For TABLE internal elements, their layout is fully managed by RenderTable::Layout
    // We only mark them as laid out, but preserve their positions and dimensions
    info.is_laid_out = true;
    
    // 清除 RenderObject 的 needs_layout_ 标志
    // 这对于 Paint 中的内容尺寸缓存优化很重要
    render_obj->ClearNeedsLayout();

    // For inline elements (like span), we need to position their children.
    // MeasureIntrinsicSize() only calculates dimensions, not child positions.
    // Flex layout positions the span element, but not its text children.
    if (type == RenderObjectType::INLINE) {
        auto* inline_obj = static_cast<RenderInline*>(render_obj);
        // Position children without recalculating size
        inline_obj->PositionChildrenOnly();
    }

    // For inline-block elements (like button), we need to call Layout() to properly
    // position their children (e.g., apply text-align: center for button text).
    // This is necessary because:
    // 1. MeasureIntrinsicSize() only calculates dimensions, not child positions
    // 2. Flex layout may stretch the element's height (align-items: stretch is default)
    // 3. Without calling Layout(), child positions remain unset or outdated
    // type was already declared above
    if (type == RenderObjectType::INLINE_BLOCK) {
        // Check if this is an SVG element (RenderSVGRoot also uses INLINE_BLOCK type)
        auto dom_node = render_obj->GetNode();
        bool is_svg = false;
        if (dom_node && dom_node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(dom_node);
            if (element && element->GetTagName() == "svg") {
                is_svg = true;
                // Call RenderSVGRoot::Layout directly
                auto* svg_root = static_cast<RenderSVGRoot*>(render_obj);
                svg_root->Layout(info.width, info.height);
            }
        }

        if (!is_svg) {
            auto* inline_block = static_cast<RenderInlineBlock*>(render_obj);
            // Call Layout with the final dimensions to properly position children
            inline_block->Layout(info.width, info.height);
        }
    }

    // Check if this is a fieldset element - need special layout handling
    bool is_fieldset = false;
    float legend_height = 0.0f;
    auto dom_node = render_obj->GetNode();
    if (dom_node && dom_node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::dynamic_pointer_cast<Element>(dom_node);
        if (element && element->GetTagName() == "fieldset") {
            is_fieldset = true;
            // Find legend to get its height
            for (auto& child : render_obj->GetChildren()) {
                auto child_node = child->GetNode();
                if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto child_elem = std::dynamic_pointer_cast<Element>(child_node);
                    if (child_elem && child_elem->GetTagName() == "legend") {
                        legend_height = child->GetLayoutInfo().height;
                        break;
                    }
                }
            }
        }
    }

    // Handle IFC containers
    // IFC layout already applies padding/border offset in ApplyLayoutResults,
    // so we just mark children as laid out without modifying positions
    if (node->is_ifc_container && !node->is_anonymous_block) {
        for (auto& child : render_obj->GetChildren()) {
            LayoutInfo& child_info = child->GetLayoutInfo();
            child_info.is_laid_out = true;
        }
        return;
    }
    
    // Handle anonymous block boxes in children
    // Anonymous blocks don't have render_obj, so we need to process their inline children
    for (NodeId child_id : node->children) {
        LayoutNode* child_node = GetNode(child_id);
        if (child_node && child_node->is_anonymous_block) {
            // Apply layout results for anonymous block's inline children
            ApplyAnonymousBlockLayoutResults(child_node);
        }
    }

    // For fieldset, calculate the offset adjustment for non-legend children
    // Browser behavior: content starts at max(min_offset, padding-top)
    // where min_offset ≈ legend.height + border-top + small gap ≈ 24px
    // Current layout puts content at: border + padding + legend
    // We need to adjust to the correct position
    float fieldset_content_offset = 0.0f;
    if (is_fieldset && legend_height > 0) {
        const auto& style = render_obj->GetComputedStyle();
        float border_top = style.border_top_width > 0 ? style.border_top_width :
                           style.border.width.ToPx(0, style.font_size);
        float padding_top = style.padding.top.ToPx(0, style.font_size);

        // Minimum offset for content area (legend height + border + small gap)
        float min_content_offset = legend_height + border_top + 3.5f;  // ≈ 24px for typical legend

        // Browser uses: max(min_offset, padding_top)
        float target_content_y = std::max(min_content_offset, padding_top);

        // Current layout puts content at: border + padding + legend
        float current_content_y = border_top + padding_top + legend_height;

        // The offset to subtract from non-legend children
        fieldset_content_offset = current_content_y - target_content_y;
    }

    // Recursively read children
    // CRITICAL FIX: Iterate over LAYOUT TREE children, not RENDER TREE children!
    // For containers with anonymous blocks, some inline elements are NOT in the layout tree
    // but are managed by anonymous blocks (stored in anonymous_inline_children).
    // We should only process children that are in the layout tree.
    for (NodeId child_id : node->children) {
        LayoutNode* child_node = GetNode(child_id);
        if (!child_node) continue;
        
        // Skip anonymous blocks - they were already processed above (lines 2646-2652)
        if (child_node->is_anonymous_block) {
            continue;
        }
        
        // Process normal layout tree children
        if (child_node->render_obj) {
            ReadLayoutResults(child_node->render_obj);

            // For fieldset, adjust children positions according to browser behavior
            if (is_fieldset) {
                auto child_dom = child_node->render_obj->GetNode();
                if (child_dom && child_dom->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto child_elem = std::dynamic_pointer_cast<Element>(child_dom);
                    LayoutInfo& child_info = child_node->render_obj->GetLayoutInfo();

                    if (child_elem && child_elem->GetTagName() == "legend") {
                        // Legend's y coordinate should be 0 relative to fieldset's border-box
                        child_info.y = 0;
                    } else if (fieldset_content_offset > 0) {
                        // Non-legend children: adjust y to start from legend.height + padding
                        // instead of border + padding + legend
                        child_info.y -= fieldset_content_offset;
                    }
                }
            }
        }
    }

    // Adjust fieldset height: we moved content up by fieldset_content_offset,
    // so the total height should be reduced by the same amount
    if (is_fieldset && fieldset_content_offset > 0) {
        info.height -= fieldset_content_offset;
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
        // Also update output.size for ReadLayoutResults
        it->second.output.size = layout.size;
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

bool NativeLayoutEngine::IsTextNode(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        return false;
    }
    if (!it->second.render_obj) {
        return false;
    }
    return it->second.render_obj->GetType() == RenderObjectType::TEXT;
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

