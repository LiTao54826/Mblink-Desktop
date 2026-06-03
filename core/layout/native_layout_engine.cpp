/**
 * @file native_layout_engine.cpp
 * @brief Native layout engine implementation
 *
 * @note 大文件说明 (3790 行)
 * 本文件包含原生布局引擎的完整实现。
 * 文件较大的原因：
 * 1. 实现完整的 CSS 布局算法（块级、行内、Flex、Grid）
 * 2. 包含复杂的尺寸计算逻辑
 * 3. 包含定位（relative、absolute、fixed）处理
 * 4. 包含浮动和清除浮动逻辑
 * 5. 包含表格布局支持
 * 6. 需要处理各种 CSS 属性的交互
 *
 * 计划重构：
 * - 已有部分拆分（flex_layout.cpp、grid/）
 * - 可进一步提取块级布局和行内布局
 */

#include "native_layout_engine.h"
#include "content_version.h"
#include "ifc/ifc_layout.h"
#include "ifc/line_breaker.h"
#include "ifc/vertical_aligner.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/render/objects/render_object.h"
#include "core/render/objects/render_inline_block.h"
#include "core/render/objects/render_inline_flex.h"
#include "core/render/objects/render_svg.h"
#include "core/render/text/text_renderer.h"
#include "core/render/text/font_manager.h"
#include "util/resolve.h"
#include "util/math.h"
#include "block_layout.h"
#include "flex_layout.h"
#include "grid/grid.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <limits>
#include <sstream>
#include <atomic>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")
#endif

namespace mbink {

namespace {
std::atomic<size_t> g_layout_node_live_count{0};
std::atomic<size_t> g_layout_engine_live_count{0};
}
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
    if (css_length.is_calc) {
        return LengthPercentage::Calc(css_length.calc_percent / 100.0f, css_length.calc_px);
    }
    switch (css_length.function_type) {
        case CSSLength::FunctionType::MIN:
            return LengthPercentage::Min(ConvertLength(*css_length.func_a), ConvertLength(*css_length.func_b));
        case CSSLength::FunctionType::MAX:
            return LengthPercentage::Max(ConvertLength(*css_length.func_a), ConvertLength(*css_length.func_b));
        case CSSLength::FunctionType::CLAMP:
            return LengthPercentage::Clamp(ConvertLength(*css_length.func_a), ConvertLength(*css_length.func_b), ConvertLength(*css_length.func_c));
        case CSSLength::FunctionType::NONE:
            break;
    }
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
            float vw = ViewportSize::GetWidth();
            return LengthPercentage::Length(css_length.value * vw / 100.0f);
        }
        case CSSUnit::VH: {
            float vh = ViewportSize::GetHeight();
            return LengthPercentage::Length(css_length.value * vh / 100.0f);
        }
        case CSSUnit::VMIN: {
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            return LengthPercentage::Length(css_length.value * std::min(vw, vh) / 100.0f);
        }
        case CSSUnit::VMAX: {
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            return LengthPercentage::Length(css_length.value * std::max(vw, vh) / 100.0f);
        }
        case CSSUnit::AUTO:
        case CSSUnit::NONE:
        default:
            return LengthPercentage::Zero();
    }
}

// Forward declaration for recursive call
static std::vector<TrackSizingFunction> ParseGridTemplate(const std::string& template_str);

static bool SameTrackMin(const MinTrackSizingFunction& a, const MinTrackSizingFunction& b) {
    return a.type == b.type && a.value == b.value && a.is_percent == b.is_percent;
}

static bool SameTrackMax(const MaxTrackSizingFunction& a, const MaxTrackSizingFunction& b) {
    return a.type == b.type && a.value == b.value && a.is_percent == b.is_percent;
}

static bool SameNonRepeatedTrack(const NonRepeatedTrackSizingFunction& a,
                                 const NonRepeatedTrackSizingFunction& b) {
    return SameTrackMin(a.min, b.min) && SameTrackMax(a.max, b.max);
}

static bool SameRepeatedTracks(const std::vector<NonRepeatedTrackSizingFunction>& a,
                               const std::vector<NonRepeatedTrackSizingFunction>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (!SameNonRepeatedTrack(a[i], b[i])) {
            return false;
        }
    }
    return true;
}

static bool SameTrackTemplate(const std::vector<TrackSizingFunction>& a,
                              const std::vector<TrackSizingFunction>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].type != b[i].type ||
            !SameNonRepeatedTrack(a[i].single, b[i].single) ||
            a[i].repeat_count != b[i].repeat_count ||
            !SameRepeatedTracks(a[i].repeat_tracks, b[i].repeat_tracks)) {
            return false;
        }
    }
    return true;
}

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
    if (trimmed == "min-content") {
        return NonRepeatedTrackSizingFunction{
            MinTrackSizingFunction::MinContent(),
            MaxTrackSizingFunction::MinContent()
        };
    }
    if (trimmed == "max-content") {
        return NonRepeatedTrackSizingFunction{
            MinTrackSizingFunction::MaxContent(),
            MaxTrackSizingFunction::MaxContent()
        };
    }
    if (trimmed.size() > 12 && trimmed.substr(0, 12) == "fit-content" && trimmed[12] == '(' && trimmed.back() == ')') {
        std::string inner = trimmed.substr(13, trimmed.size() - 14);
        size_t inner_start = inner.find_first_not_of(" \t");
        size_t inner_end = inner.find_last_not_of(" \t");
        if (inner_start != std::string::npos) {
            inner = inner.substr(inner_start, inner_end - inner_start + 1);
            try {
                if (!inner.empty() && inner.back() == '%') {
                    float pct = std::stof(inner.substr(0, inner.size() - 1));
                    return NonRepeatedTrackSizingFunction{
                        MinTrackSizingFunction::Auto(),
                        MaxTrackSizingFunction::FitContentPercent(pct / 100.0f)
                    };
                }

                float px = 0.0f;
                if (inner.size() > 2 && inner.substr(inner.size() - 2) == "px") {
                    px = std::stof(inner.substr(0, inner.size() - 2));
                } else {
                    px = std::stof(inner);
                }
                return NonRepeatedTrackSizingFunction{
                    MinTrackSizingFunction::Auto(),
                    MaxTrackSizingFunction::FitContentPx(px)
                };
            } catch (...) {
                return NonRepeatedTrackSizingFunction::Auto();
            }
        }
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
    g_layout_engine_live_count.fetch_add(1, std::memory_order_relaxed);
}

NativeLayoutEngine::~NativeLayoutEngine() {
    Clear();
    g_layout_engine_live_count.fetch_sub(1, std::memory_order_relaxed);
}

size_t NativeLayoutEngine::GetLiveNodeCount() {
    return g_layout_node_live_count.load(std::memory_order_relaxed);
}

size_t NativeLayoutEngine::GetLiveEngineCount() {
    return g_layout_engine_live_count.load(std::memory_order_relaxed);
}

//------------------------------------------------------------------------------
// Public Interface
//------------------------------------------------------------------------------

void NativeLayoutEngine::BuildLayoutTree(std::shared_ptr<RenderObject> root, bool force_rebuild) {
    if (!root) {
        return;
    }

    // Helper function to recursively clear layout flags for full rebuild
    // 关键修复：除了清除 is_laid_out，还要设 needs_layout_=true。
    // 否则 INLINE_FLEX/FLEX 的 LayoutAsFlex 中 parent_layout_pass=false，
    // 子元素不会被重新 Layout，导致 is_laid_out 保持 false。
    std::function<void(RenderObject*)> clearLayoutFlags = [&](RenderObject* obj) {
        if (!obj) return;
        obj->GetLayoutInfo().is_laid_out = false;
        obj->MarkNeedsLayout(false);  // 设 needs_layout_=true，不向上传播
        for (auto& child : obj->GetChildren()) {
            clearLayoutFlags(child.get());
        }
    };

    // Check if viewport size has changed
    // When viewport size changes, styles using vh/vw units need to be recalculated
    float current_vw = ViewportSize::GetWidth();
    float current_vh = ViewportSize::GetHeight();
    bool viewport_changed = (cached_viewport_width_ != current_vw ||
                             cached_viewport_height_ != current_vh);

    // Update cached viewport size
    cached_viewport_width_ = current_vw;
    cached_viewport_height_ = current_vh;

    // Check if we can reuse the existing tree
    auto cached = cached_root_.lock();

    if (root_node_ != 0 && cached && cached.get() == root.get()) {
        // If viewport size changed, we need to rebuild to recalculate vh/vw units
        // Also rebuild if root or any child needs layout
        // force_rebuild is used when DOM structure has changed (e.g., after Synchronize)
        bool needs_layout = root->NeedsLayout();
        bool child_needs_layout = root->ChildNeedsLayout();

        if (needs_layout || child_needs_layout || viewport_changed || force_rebuild) {
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

    // Delegate to internal method for actual layout computation
    ComputeLayoutInternal(available_width, available_height);
}

void NativeLayoutEngine::ComputeLayoutInternal(float available_width, float available_height) {
    // Internal layout computation - does NOT clear caches
    // This allows incremental layout to selectively clear only dirty node caches
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 2.2**
    //
    // 统一滚动条处理：
    // 所有元素（包括 root/body）的滚动条检测都在 ComputeNodeLayout 中统一处理
    // 这里只处理 root margin 和位置
    // **Feature: unified-scrollbar-system**
    // **Validates: Requirements 1.4, 3.1, 3.2**

    if (root_node_ == 0) {
        return;
    }

    // For root element (body), we need to handle margin specially:
    // - Root element's margin offsets it from the viewport edge
    // - Root element's width = available_width - margin_left - margin_right
    // - Scrollbar handling is unified in ComputeNodeLayout
    LayoutNode* root_node = GetNode(root_node_);
    Rect<float> root_margin = {0.0f, 0.0f, 0.0f, 0.0f};
    float root_width = available_width;

    if (root_node) {
        // Resolve root element's margin based on available_width (viewport width)
        root_margin = ResolveOrZero(root_node->style.margin, std::optional<float>(available_width));
        // Root element's width should be reduced by its horizontal margins
        root_width = available_width - root_margin.left - root_margin.right;
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
    if (it == render_to_node_.end()) {
        auto ancestor = render_obj ? render_obj->GetParent() : nullptr;
        while (ancestor) {
            it = render_to_node_.find(ancestor.get());
            if (it != render_to_node_.end()) {
                break;
            }
            ancestor = ancestor->GetParent();
        }
    }

    if (it != render_to_node_.end()) {
        LayoutNode* node = GetNode(it->second);
        if (node) {
            if (node->render_obj) {
                node->render_obj->MarkNeedsLayout(false);
            }

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

            // 关键修复：检查这个 RenderObject 是否真的对应这个 LayoutNode
            // 对于 IFC 容器的子节点（包括匿名块中的 inline 元素），
            // 它们被映射到父节点/匿名块的 LayoutNode。
            // 这类节点不能直接覆盖 mapped 节点样式，但其尺寸/布局变化
            // 必须驱动 mapped 节点重新布局，否则会出现增量布局漏算。
            if (node->render_obj != render_obj) {
                // 不能更新 node->style（那是父/匿名节点的样式），
                // 但必须让 mapped 节点失效并向上传播。
                node->needs_layout = true;
                node->cache.Clear();

                if (node->render_obj) {
                    node->render_obj->MarkNeedsLayout(false);
                }

                // 对 IFC/匿名块场景，content_version 变化可确保行内内容重新收集与测量。
                node->content_version = ContentVersionManager::GetInstance().GenerateVersion();

                LayoutScope scope = DetermineLayoutScope(node);
                PropagateLayoutDirty(it->second, scope);
                return;
            }

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
            auto new_grid_template_columns = ParseGridTemplate(style.grid_template_columns);
            auto new_grid_template_rows = ParseGridTemplate(style.grid_template_rows);
            auto new_grid_auto_columns = ParseGridAutoTracks(style.grid_auto_columns);
            auto new_grid_auto_rows = ParseGridAutoTracks(style.grid_auto_rows);
            auto new_grid_column_gap = ConvertLength(style.column_gap);
            auto new_grid_row_gap = ConvertLength(style.row_gap);

            if (old_style.grid_auto_flow != new_style.grid_auto_flow ||
                !SameTrackTemplate(node->grid_container_style.grid_template_columns, new_grid_template_columns) ||
                !SameTrackTemplate(node->grid_container_style.grid_template_rows, new_grid_template_rows) ||
                !SameRepeatedTracks(node->grid_container_style.grid_auto_columns, new_grid_auto_columns) ||
                !SameRepeatedTracks(node->grid_container_style.grid_auto_rows, new_grid_auto_rows) ||
                node->grid_container_style.column_gap != new_grid_column_gap ||
                node->grid_container_style.row_gap != new_grid_row_gap) {
                layout_changed = true;
            }

            // Overflow 变化可能影响布局（滚动条）
            // 当 overflow 变化时，滚动条的出现/消失会影响子元素的可用宽度
            bool overflow_changed = (old_style.overflow.x != new_style.overflow.x ||
                                     old_style.overflow.y != new_style.overflow.y ||
                                     old_style.scrollbar_width != new_style.scrollbar_width);
            if (overflow_changed) {
                layout_changed = true;
            }

            // 更新样式 - 只需更新统一的 style 字段
            node->style = new_style;

            // 更新 Grid 特有数据（仅解析 Grid 特有属性，不再同步 CoreStyle 基类）
            // 这些属性不在统一的 Style 结构中，需要单独解析
            node->grid_container_style.grid_template_columns = std::move(new_grid_template_columns);
            node->grid_container_style.grid_template_rows = std::move(new_grid_template_rows);
            node->grid_container_style.grid_auto_columns = std::move(new_grid_auto_columns);
            node->grid_container_style.grid_auto_rows = std::move(new_grid_auto_rows);
            node->grid_container_style.column_gap = new_grid_column_gap;
            node->grid_container_style.row_gap = new_grid_row_gap;

            // 解析 Grid 项目特有属性
            auto [col_start, col_end] = ParseGridLine(style.grid_column);
            auto [row_start, row_end] = ParseGridLine(style.grid_row);
            node->grid_item_style.grid_column_start = col_start;
            node->grid_item_style.grid_column_end = col_end;
            node->grid_item_style.grid_row_start = row_start;
            node->grid_item_style.grid_row_end = row_end;

            // 只有布局相关属性变化时才标记需要重新布局和更新版本号
            // **Feature: incremental-layout-optimization**
            // **Validates: Requirements 1.3**
            if (layout_changed) {
                node->needs_layout = true;
                const bool is_flex_or_grid_container =
                    old_style.display == Display::Flex ||
                    old_style.display == Display::Grid ||
                    new_style.display == Display::Flex ||
                    new_style.display == Display::Grid;
                for (NodeId ancestor_id = node->parent; ancestor_id != 0;) {
                    LayoutNode* ancestor = GetNode(ancestor_id);
                    if (!ancestor) {
                        break;
                    }
                    ancestor->cache.Clear();
                    ancestor->needs_layout = true;
                    if (ancestor->render_obj) {
                        ancestor->render_obj->MarkNeedsLayout(false);
                    }
                    ancestor_id = ancestor->parent;
                }
                // 关键修复：清除当前节点的缓存，确保布局重新计算
                node->cache.Clear();

                // 关键修复：当 overflow 变化时，滚动条的出现/消失会影响子元素的可用宽度
                // 需要清除所有子元素的布局缓存，确保它们使用新的可用宽度重新布局
                if (is_flex_or_grid_container) {
                    for (NodeId child_id : node->children) {
                        MarkSubtreeNeedsLayout(child_id);
                    }
                }

                if (overflow_changed) {
                    // 关键修复：清除当前元素自身的 content_width_/content_height_ 缓存
                    // 当 overflow 变化时，滚动条的出现/消失会影响内容区域宽度的判断
                    // 必须重新计算 content_width_ 以正确判断是否需要水平滚动条
                    render_obj->SetContentSize(0.0f, 0.0f);

                    std::function<void(NodeId)> clearChildrenCache = [&](NodeId child_id) {
                        LayoutNode* child = GetNode(child_id);
                        if (!child) return;
                        child->cache.Clear();
                        child->needs_layout = true;
                        if (child->render_obj) {
                            child->render_obj->MarkNeedsLayout(false);
                        }
                        for (NodeId grandchild_id : child->children) {
                            clearChildrenCache(grandchild_id);
                        }
                    };
                    for (NodeId child_id : node->children) {
                        clearChildrenCache(child_id);
                    }
                }

                // Update content version for layout-affecting style changes
                // Pure paint styles (color, background-color, etc.) don't update version
                uint64_t new_version = ContentVersionManager::GetInstance().GenerateVersion();
                node->content_version = new_version;

                // 关键修复：当子元素尺寸变化时，需要通知父元素重新布局
                // 这对于flex/grid容器的居中对齐等功能至关重要
                // 因为父容器需要根据子元素的新尺寸重新计算位置
                LayoutScope scope = DetermineLayoutScope(node);
                PropagateLayoutDirty(it->second, scope);
            }
        }
    } else {
        bool already_laid_out = render_obj->GetLayoutInfo().is_laid_out;
        // 如果找不到对应的 LayoutNode，可能是之前是 display: none
        // 现在如果变成可见的，需要添加到 LayoutTree
        // 但是！如果元素已经被标记为 is_laid_out，说明它可能由匿名块盒管理
        // （匿名块盒管理的内联元素不在 render_to_node_ 映射中）
        // 不应该重新添加，否则会破坏匿名块盒的布局
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
        } else if (already_laid_out) {
            // 关键修复：inline 元素不在 render_to_node_ 映射中（由 IFC/匿名块盒/INLINE_FLEX 管理），
            // 但它们的样式变化（如 width/height）需要通知父级 LayoutNode 重新布局。
            // 注意：不能清除 is_laid_out！否则下一次 UpdateStyle 会因为 !already_laid_out
            // 而调用 AddElement，给不该有 LayoutNode 的元素创建 LayoutNode（output={0,0}），
            // 导致 ReadLayoutResults 把 0x0 写入 LayoutInfo，元素消失。
            // 只需标记 render_obj 需要重新布局，让父级的 Layout() 重新调用 child->Layout()。

            render_obj->MarkNeedsLayout(false);  // 不向上传播，下面手动传播到 LayoutNode

            auto parent_obj = render_obj->GetParent();
            if (parent_obj) {
                auto parent_it = render_to_node_.find(parent_obj.get());
                if (parent_it != render_to_node_.end()) {
                    LayoutNode* parent_node = GetNode(parent_it->second);
                    if (parent_node) {
                        parent_node->needs_layout = true;
                        parent_node->cache.Clear();
                        parent_node->content_version = ContentVersionManager::GetInstance().GenerateVersion();

                        LayoutScope scope = DetermineLayoutScope(parent_node);
                        PropagateLayoutDirty(parent_it->second, scope);
                    }
                } else {
                    // 父级也可能不在映射中（嵌套 inline），继续向上找
                    auto grandparent_obj = parent_obj->GetParent();
                    while (grandparent_obj) {
                        auto gp_it = render_to_node_.find(grandparent_obj.get());
                        if (gp_it != render_to_node_.end()) {
                            LayoutNode* gp_node = GetNode(gp_it->second);
                            if (gp_node) {
                                gp_node->needs_layout = true;
                                gp_node->cache.Clear();
                                gp_node->content_version = ContentVersionManager::GetInstance().GenerateVersion();

                                LayoutScope scope = DetermineLayoutScope(gp_node);
                                PropagateLayoutDirty(gp_it->second, scope);
                            }
                            break;
                        }
                        grandparent_obj = grandparent_obj->GetParent();
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
        auto ancestor = render_obj->GetParent();
        while (ancestor) {
            it = render_to_node_.find(ancestor.get());
            if (it != render_to_node_.end()) {
                break;
            }
            ancestor = ancestor->GetParent();
        }
    }

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

    // 关键修复：先检查是否是 flex/grid 子元素
    // 即使子元素有固定尺寸，当尺寸变化时也需要通知父元素重新计算位置
    // 因为 justify-content: center 和 align-items: center 需要根据子元素尺寸计算位置
    // Check if this is a flex or grid child BEFORE checking fixed size
    // Flex/grid children may affect sibling layouts due to space distribution
    if (IsFlexOrGridChild(node)) {
        return LayoutScope::SIBLINGS;
    }

    // Check if the node has fixed dimensions (both width AND height are explicit lengths)
    // Fixed-size containers isolate their children from affecting ancestors
    // Note: This check is AFTER flex/grid child check, because flex/grid children
    // need to notify parent even if they have fixed size
    if (HasFixedSize(node)) {
        return LayoutScope::SELF_ONLY;
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

void NativeLayoutEngine::MarkSubtreeNeedsLayout(NodeId node_id) {
    LayoutNode* node = GetNode(node_id);
    if (!node) {
        return;
    }

    node->needs_layout = true;
    node->cache.Clear();
    if (node->render_obj) {
        node->render_obj->MarkNeedsLayout(false);
    }

    for (NodeId child_id : node->children) {
        MarkSubtreeNeedsLayout(child_id);
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
                if (parent) {
                    // 关键修复：标记所有兄弟元素需要布局并清除缓存
                    // 因为 flex/grid 布局中，一个子元素尺寸变化会影响其他子元素的位置和尺寸
                    for (NodeId sibling_id : parent->children) {
                        if (sibling_id == node_id) continue;  // 跳过自己
                        LayoutNode* sibling = GetNode(sibling_id);
                        if (sibling) {
                            sibling->needs_layout = true;
                            sibling->cache.Clear();
                            if (sibling->render_obj) {
                                sibling->render_obj->MarkNeedsLayout(false);
                            }
                        }
                    }

                    if (!parent->needs_layout) {
                        parent->needs_layout = true;
                        // 关键修复：同时标记RenderObject需要布局
                        // 这确保LayoutDirtySubtree能正确检测到脏节点
                        if (parent->render_obj) {
                            parent->render_obj->MarkNeedsLayout(false);  // false = 不再向上传播
                        }
                        // 关键修复：清除父元素的缓存，确保flex/grid布局重新计算
                        // 这对于子元素尺寸变化后的居中对齐等功能至关重要
                        parent->cache.Clear();
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
                    // 关键修复：同时标记RenderObject需要布局
                    if (parent->render_obj) {
                        parent->render_obj->MarkNeedsLayout(false);
                    }
                    // 关键修复：清除父元素的缓存，确保布局重新计算
                    parent->cache.Clear();

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
    RenderObject* layout_parent = parent;

    // 关键修复：如果直接父节点不在布局树中，向上查找最近的在布局树中的祖先
    // 这处理了 IFC 容器中的 inline 元素（如 span）不在布局树中的情况
    while (layout_parent) {
        auto it = render_to_node_.find(layout_parent);
        if (it != render_to_node_.end()) {
            parent_id = it->second;
            break;
        }
        // 向上查找
        auto parent_ptr = layout_parent->GetParent();
        layout_parent = parent_ptr ? parent_ptr.get() : nullptr;
    }

    // 如果找不到任何在布局树中的祖先，不添加子节点
    if (parent && parent_id == 0) {
        return;
    }

    // 关键修复：计算正确的插入位置
    // 遍历父渲染对象的子元素，找到当前元素应该插入的位置
    size_t layout_insert_index = 0;
    if (parent_id != 0 && layout_parent) {
        LayoutNode* parent_node = GetNode(parent_id);
        if (parent_node) {
            // 遍历父渲染对象的子元素，计算在布局树中的插入位置
            const auto& render_children = layout_parent->GetChildren();
            for (size_t i = 0; i < render_children.size(); ++i) {
                if (render_children[i].get() == render_obj) {
                    // 找到当前元素在渲染树中的位置
                    break;
                }
                // 检查这个渲染子元素是否在布局树中
                if (HasElement(render_children[i].get())) {
                    ++layout_insert_index;
                }
            }
        }
    }

    // 使用 BuildSubtreeAtIndex 在正确位置插入
    BuildSubtreeAtIndex(render_obj, parent_id, layout_insert_index);

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

            // 关键修复：如果父节点是 flex/grid 容器，清除所有兄弟节点的缓存
            // 因为 flex/grid 布局中，添加新子元素会影响其他子元素的位置和尺寸
            if (parent_node->style.display == Display::Flex ||
                parent_node->style.display == Display::Grid) {
                for (NodeId sibling_id : parent_node->children) {
                    LayoutNode* sibling = GetNode(sibling_id);
                    if (sibling) {
                        sibling->needs_layout = true;
                        sibling->cache.Clear();
                        if (sibling->render_obj) {
                            sibling->render_obj->MarkNeedsLayout(false);
                        }
                    }
                }
            }

            // 清除所有祖先的缓存
            NodeId ancestor_id = parent_node->parent;
            while (ancestor_id != 0) {
                LayoutNode* ancestor = GetNode(ancestor_id);
                if (!ancestor) break;

                ancestor->cache.Clear();
                ancestor->needs_layout = true;
                ancestor->content_version = ContentVersionManager::GetInstance().GenerateVersion();

                // 关键修复：清除祖先的所有子元素的缓存
                // 因为添加新元素可能影响兄弟元素的布局（特别是 flex/grid 容器）
                // 不检查 display 类型，因为 style.display 可能没有正确反映 CSS display 属性
                for (NodeId child_id : ancestor->children) {
                    LayoutNode* child = GetNode(child_id);
                    if (child) {
                        child->needs_layout = true;
                        child->cache.Clear();

                        if (child->render_obj) {
                            child->render_obj->MarkNeedsLayout(false);
                        }
                    }
                }

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

    if (node && node->render_obj != render_obj) {
        render_to_node_.erase(render_obj);
        node->cache.Clear();
        node->needs_layout = true;
        node->content_version = ContentVersionManager::GetInstance().GenerateVersion();
        if (node->render_obj) {
            node->render_obj->MarkNeedsLayout(false);
        }
        return;
    }

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

                // 对于 IFC 容器，清除其子元素在 render_to_node_ 中的注册
                // 与 BuildSubtree 中的注册对应
                if (n->is_ifc_container && !n->is_anonymous_block) {
                    for (auto& child : n->render_obj->GetChildren()) {
                        if (child) {
                            LayoutInfo& child_layout = child->GetLayoutInfo();
                            child_layout.is_laid_out = false;
                            child_layout.x = 0.0f;
                            child_layout.y = 0.0f;
                            child_layout.width = 0.0f;
                            child_layout.height = 0.0f;
                            render_to_node_.erase(child.get());
                        }
                    }
                }
            }

            // 对于匿名块盒，清除其管理的内联子元素的布局信息
            // 并从 render_to_node_ 映射中移除它们
            if (n->is_anonymous_block) {
                for (RenderObject* inline_child : n->anonymous_inline_children) {
                    if (inline_child) {
                        LayoutInfo& child_layout = inline_child->GetLayoutInfo();
                        child_layout.is_laid_out = false;
                        child_layout.x = 0.0f;
                        child_layout.y = 0.0f;
                        child_layout.width = 0.0f;
                        child_layout.height = 0.0f;
                        // 从映射中移除，与 CreateAnonymousBlockBox 中的注册对应
                        render_to_node_.erase(inline_child);
                    }
                }
            }
        }
        if (nodes_.erase(id) > 0) {
            g_layout_node_live_count.fetch_sub(1, std::memory_order_relaxed);
        }

        if (id == root_node_) {
            root_node_ = 0;
        }
    }
}

void NativeLayoutEngine::Clear() {
    const size_t cleared_count = nodes_.size();
    if (cleared_count > 0) {
        g_layout_node_live_count.fetch_sub(cleared_count, std::memory_order_relaxed);
    }
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

    // ✅ Set margins_collapse based on element type
    // CSS spec: margins do NOT collapse for:
    // - Inline-block elements (display: inline-block)
    // - Absolutely positioned elements (position: absolute/fixed)
    // - Floated elements (float: left/right)
    // - Flex/Grid items (handled by parent container)
    // - Elements that establish new block formatting contexts
    RenderObjectType type = render_obj->GetType();
    bool is_inline_block = (type == RenderObjectType::INLINE_BLOCK);
    bool is_absolute_or_fixed = (node.style.position == Position::Absolute ||
                                   node.style.position == Position::Fixed);
    // Note: Float property is not currently in Style, so we skip it for now
    // Flex/Grid items will be handled by the parent container's layout algorithm
    node.style.margins_collapse = !is_inline_block && !is_absolute_or_fixed;

    // 为 IFC 容器初始化 content_version（非零值启用版本检查）
    if (node.is_ifc_container) {
        node.content_version = ContentVersionManager::GetInstance().GenerateVersion();
    }

    // 检测 TABLE 容器
    node.is_table_container = (type == RenderObjectType::TABLE);

    // 初始化 Grid 特有数据（仅解析 Grid 特有属性，不再同步 CoreStyle 基类）
    // 这些属性不在统一的 Style 结构中，需要单独解析
    node.grid_container_style.grid_template_columns = ParseGridTemplate(computed.grid_template_columns);
    node.grid_container_style.grid_template_rows = ParseGridTemplate(computed.grid_template_rows);
    node.grid_container_style.grid_auto_columns = ParseGridAutoTracks(computed.grid_auto_columns);
    node.grid_container_style.grid_auto_rows = ParseGridAutoTracks(computed.grid_auto_rows);
    node.grid_container_style.column_gap = ConvertLength(computed.column_gap);
    node.grid_container_style.row_gap = ConvertLength(computed.row_gap);

    // 解析 Grid 项目特有属性
    auto [col_start, col_end] = ParseGridLine(computed.grid_column);
    auto [row_start, row_end] = ParseGridLine(computed.grid_row);
    node.grid_item_style.grid_column_start = col_start;
    node.grid_item_style.grid_column_end = col_end;
    node.grid_item_style.grid_row_start = row_start;
    node.grid_item_style.grid_row_end = row_end;

    nodes_[id] = std::move(node);
    render_to_node_[render_obj] = id;
    g_layout_node_live_count.fetch_add(1, std::memory_order_relaxed);

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
        style.display == RenderObjectType::GRID ||
        style.display == RenderObjectType::INLINE_FLEX ||
        style.display == RenderObjectType::INLINE_GRID) {
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

        // ✅ FIX: Use child_type (GetType()) not child_display (GetComputedStyle().display)
        // Inline-block, inline-flex, inline-grid elements are inline-level
        if (child_type == RenderObjectType::TEXT) {
            has_inline = true;
        } else if (child_type == RenderObjectType::INLINE ||
                   child_type == RenderObjectType::INLINE_BLOCK ||
                   child_type == RenderObjectType::INLINE_FLEX ||
                   child_type == RenderObjectType::INLINE_GRID) {
            has_inline = true;
        } else if (child_type == RenderObjectType::BLOCK ||
                   child_type == RenderObjectType::FLEX ||
                   child_type == RenderObjectType::GRID ||
                   child_type == RenderObjectType::TABLE) {
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
    if (css_length.is_calc) {
        return Dimension::Calc(css_length.calc_percent / 100.0f, css_length.calc_px);
    }
    switch (css_length.function_type) {
        case CSSLength::FunctionType::MIN:
            return Dimension::Min(ConvertDimension(*css_length.func_a), ConvertDimension(*css_length.func_b));
        case CSSLength::FunctionType::MAX:
            return Dimension::Max(ConvertDimension(*css_length.func_a), ConvertDimension(*css_length.func_b));
        case CSSLength::FunctionType::CLAMP:
            return Dimension::Clamp(ConvertDimension(*css_length.func_a), ConvertDimension(*css_length.func_b), ConvertDimension(*css_length.func_c));
        case CSSLength::FunctionType::NONE:
            break;
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
            float vw = ViewportSize::GetWidth();
            return Dimension::Length(css_length.value * vw / 100.0f);
        }
        case CSSUnit::VH: {
            float vh = ViewportSize::GetHeight();
            return Dimension::Length(css_length.value * vh / 100.0f);
        }
        case CSSUnit::VMIN: {
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            return Dimension::Length(css_length.value * std::min(vw, vh) / 100.0f);
        }
        case CSSUnit::VMAX: {
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            return Dimension::Length(css_length.value * std::max(vw, vh) / 100.0f);
        }
        default:
            return Dimension::Auto();
    }
}

// Helper to convert CSSLength to LengthPercentageAuto
LengthPercentageAuto ConvertLengthAuto(const CSSLength& css_length) {
    if (css_length.is_calc) {
        return LengthPercentageAuto::Calc(css_length.calc_percent / 100.0f, css_length.calc_px);
    }
    switch (css_length.function_type) {
        case CSSLength::FunctionType::MIN:
            return LengthPercentageAuto::Min(ConvertLengthAuto(*css_length.func_a), ConvertLengthAuto(*css_length.func_b));
        case CSSLength::FunctionType::MAX:
            return LengthPercentageAuto::Max(ConvertLengthAuto(*css_length.func_a), ConvertLengthAuto(*css_length.func_b));
        case CSSLength::FunctionType::CLAMP:
            return LengthPercentageAuto::Clamp(ConvertLengthAuto(*css_length.func_a), ConvertLengthAuto(*css_length.func_b), ConvertLengthAuto(*css_length.func_c));
        case CSSLength::FunctionType::NONE:
            break;
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
            float vw = ViewportSize::GetWidth();
            return LengthPercentageAuto::Length(css_length.value * vw / 100.0f);
        }
        case CSSUnit::VH: {
            float vh = ViewportSize::GetHeight();
            return LengthPercentageAuto::Length(css_length.value * vh / 100.0f);
        }
        case CSSUnit::VMIN: {
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            return LengthPercentageAuto::Length(css_length.value * std::min(vw, vh) / 100.0f);
        }
        case CSSUnit::VMAX: {
            float vw = ViewportSize::GetWidth();
            float vh = ViewportSize::GetHeight();
            return LengthPercentageAuto::Length(css_length.value * std::max(vw, vh) / 100.0f);
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
        case RenderObjectType::INLINE_FLEX:
            style.display = Display::Flex;
            break;
        case RenderObjectType::GRID:
        case RenderObjectType::INLINE_GRID:
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
        if (val == "auto") return Overflow::Auto;
        return Overflow::Visible;
    };

    // Use overflow-x/overflow-y if set, otherwise fall back to overflow
    std::string overflow_x = !computed.overflow_x.empty() ? computed.overflow_x : computed.overflow;
    std::string overflow_y = !computed.overflow_y.empty() ? computed.overflow_y : computed.overflow;

    style.overflow.x = parseOverflow(overflow_x);
    style.overflow.y = parseOverflow(overflow_y);

    // Set scrollbar size when overflow is scroll
    // For overflow: auto, we don't reserve space in layout (scrollbar appears only when needed)
    if (style.overflow.y == Overflow::Scroll) {
        style.scrollbar_width = RenderObject::GetScrollbarWidth();
    }
    if (style.overflow.x == Overflow::Scroll) {
        style.scrollbar_height = RenderObject::GetScrollbarWidth();
    }

    style.item_is_table = (computed.display == RenderObjectType::TABLE);

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

static std::string DebugEscapeText(const std::string& text) {
    std::string out;
    out.reserve(text.size() * 2);
    for (char c : text) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

static const char* DebugRenderObjectTypeName(RenderObjectType type) {
    switch (type) {
        case RenderObjectType::TEXT: return "TEXT";
        case RenderObjectType::INLINE: return "INLINE";
        case RenderObjectType::INLINE_BLOCK: return "INLINE_BLOCK";
        case RenderObjectType::BLOCK: return "BLOCK";
        case RenderObjectType::FLEX: return "FLEX";
        case RenderObjectType::INLINE_FLEX: return "INLINE_FLEX";
        case RenderObjectType::GRID: return "GRID";
        case RenderObjectType::INLINE_GRID: return "INLINE_GRID";
        case RenderObjectType::TABLE: return "TABLE";
        case RenderObjectType::TABLE_ROW_GROUP: return "TABLE_ROW_GROUP";
        case RenderObjectType::TABLE_HEADER_GROUP: return "TABLE_HEADER_GROUP";
        case RenderObjectType::TABLE_FOOTER_GROUP: return "TABLE_FOOTER_GROUP";
        case RenderObjectType::TABLE_ROW: return "TABLE_ROW";
        case RenderObjectType::TABLE_CELL: return "TABLE_CELL";
        case RenderObjectType::TABLE_CAPTION: return "TABLE_CAPTION";
        case RenderObjectType::CONTENTS: return "CONTENTS";
        case RenderObjectType::NONE: return "NONE";
        default: return "OTHER";
    }
}



// Helper function to check if a render object is inline-level
// Inline formatting context 的文本语义必须完整保留，
// whitespace 是否最终可见应由 white-space 处理和断行阶段决定，
// 不能在 tree building / anonymous block grouping 阶段提前过滤。
static bool IsInlineLevelElement(RenderObject* render_obj) {
    static bool debug_anon_inline_ws = std::getenv("DEBUG_ANON_INLINE_WS") != nullptr;
    if (!render_obj) return false;
    RenderObjectType type = render_obj->GetType();

    if (type == RenderObjectType::TEXT) {
        auto* text_obj = static_cast<RenderText*>(render_obj);
        std::string text = text_obj ? text_obj->GetText() : std::string();
        bool is_ws_only = IsWhitespaceOnly(text);
        bool result = true;
        if (debug_anon_inline_ws) {
            std::cout << "[ANON_WS_INLINE_CHECK] type=TEXT ptr=" << render_obj
                      << " raw='" << DebugEscapeText(text) << "'"
                      << " ws_only=" << (is_ws_only ? 1 : 0)
                      << " result=" << (result ? 1 : 0)
                      << std::endl;
        }
        return result;
    }

    bool result = type == RenderObjectType::INLINE ||
                  type == RenderObjectType::INLINE_BLOCK;
    if (debug_anon_inline_ws) {
        std::cout << "[ANON_WS_INLINE_CHECK] type=" << DebugRenderObjectTypeName(type)
                  << " ptr=" << render_obj
                  << " result=" << (result ? 1 : 0)
                  << std::endl;
    }
    return result;
}

static WhiteSpaceMode ResolveWhiteSpaceMode(const ComputedStyle& style) {
    if (style.white_space == "pre") return WhiteSpaceMode::PRE;
    if (style.white_space == "pre-wrap") return WhiteSpaceMode::PRE_WRAP;
    if (style.white_space == "pre-line") return WhiteSpaceMode::PRE_LINE;
    if (style.white_space == "nowrap") return WhiteSpaceMode::NOWRAP;
    return WhiteSpaceMode::NORMAL;
}

static std::string NormalizeInlineTextForWhiteSpace(
    const std::string& text,
    const ComputedStyle& style
) {
    WhiteSpaceMode mode = ResolveWhiteSpaceMode(style);
    LineBreaker breaker;
    breaker.SetWhiteSpace(mode);
    return breaker.ProcessWhitespace(text);
}

// Helper function to check if a render object is block-level
static bool IsBlockLevelElement(RenderObject* render_obj) {
    if (!render_obj) return false;
    RenderObjectType type = render_obj->GetType();
    if (type == RenderObjectType::TEXT) return false;

    // ✅ FIX: Check computed style display for FLEX/GRID containers
    // because they use RenderBlock but should be treated as block-level
    const auto& style = render_obj->GetComputedStyle();
    RenderObjectType display = style.display;

    return type == RenderObjectType::BLOCK ||
           display == RenderObjectType::FLEX ||
           display == RenderObjectType::GRID ||
           type == RenderObjectType::TABLE;
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
    // But register them in render_to_node_ to prevent duplicate node creation
    LayoutNode* node = GetNode(node_id);
    if (node && node->is_ifc_container) {
        // Register all children's RenderObjects to prevent them from being
        // added as separate layout nodes during incremental layout
        for (auto& child : render_obj->GetChildren()) {
            if (child && !HasElement(child.get())) {
                render_to_node_[child.get()] = node_id;
            }
        }
        return;
    }

    // ✅ FIX: For FLEX/GRID containers, process all children directly
    // Same logic as BuildSubtree - flex/grid containers handle all children as flex/grid items
    // Note: INLINE_FLEX/INLINE_GRID are NOT included - they are leaf nodes (early return above)
    const auto& computed = render_obj->GetComputedStyle();
    if (computed.display == RenderObjectType::FLEX ||
        computed.display == RenderObjectType::GRID) {
        const auto& children = render_obj->GetChildren();
        for (auto& child : children) {
            BuildSubtree(child.get(), node_id);
        }
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

    // For INLINE_BLOCK, INLINE, INLINE_FLEX, and INLINE_GRID elements, they manage their own children
    // INLINE_FLEX/INLINE_GRID are leaf nodes in the layout tree - their internal layout is handled
    // by RenderInlineFlex::Layout()/LayoutAsFlex() which directly manages render tree children.
    // Adding their children to the layout tree would cause ReadLayoutResults to overwrite
    // the correct layout with 0x0 data from unused LayoutNodes.
    if (type == RenderObjectType::INLINE_BLOCK || type == RenderObjectType::INLINE ||
        type == RenderObjectType::INLINE_FLEX || type == RenderObjectType::INLINE_GRID) {
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
    // But register them in render_to_node_ to prevent duplicate node creation
    LayoutNode* node = GetNode(node_id);
    if (node && node->is_ifc_container) {
        // Register all children's RenderObjects to prevent them from being
        // added as separate layout nodes during incremental layout
        for (auto& child : render_obj->GetChildren()) {
            if (child && !HasElement(child.get())) {
                render_to_node_[child.get()] = node_id;
            }
        }
        return;
    }

    // ✅ FIX: For FLEX/GRID containers, process all children directly
    // FLEX/GRID containers don't use anonymous block boxes - they handle
    // all children (both block and inline) as flex/grid items
    // Note: INLINE_FLEX/INLINE_GRID are NOT included here - they are leaf nodes
    // whose children are managed by RenderInlineFlex::Layout() directly.
    const auto& computed = render_obj->GetComputedStyle();
    if (computed.display == RenderObjectType::FLEX ||
        computed.display == RenderObjectType::GRID) {
        const auto& children = render_obj->GetChildren();

        for (auto& child : children) {
            BuildSubtree(child.get(), node_id);
        }
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
        static bool debug_anon_inline_ws = std::getenv("DEBUG_ANON_INLINE_WS") != nullptr;
        std::vector<RenderObject*> current_inline_run;

        for (const auto& child : children) {
            if (debug_anon_inline_ws) {
                std::cout << "[ANON_WS_CHILD_SCAN] parent_node=" << node_id
                          << " child_ptr=" << child.get()
                          << " type=" << DebugRenderObjectTypeName(child->GetType());
                if (child->GetType() == RenderObjectType::TEXT) {
                    auto* text_obj = static_cast<RenderText*>(child.get());
                    std::cout << " raw='" << DebugEscapeText(text_obj ? text_obj->GetText() : std::string()) << "'";
                }
                std::cout << std::endl;
            }

            if (IsBlockLevelElement(child.get())) {
                // If we have accumulated inline elements, create an anonymous block for them
                if (!current_inline_run.empty()) {
                    if (debug_anon_inline_ws) {
                        std::cout << "[ANON_WS_FLUSH_RUN] parent_node=" << node_id
                                  << " run_size=" << current_inline_run.size() << std::endl;
                    }
                    CreateAnonymousBlockBox(node_id, current_inline_run);
                    current_inline_run.clear();
                }
                // Process the block element normally
                BuildSubtree(child.get(), node_id);
            } else if (IsInlineLevelElement(child.get())) {
                // Accumulate inline elements
                current_inline_run.push_back(child.get());
                if (debug_anon_inline_ws) {
                    std::cout << "[ANON_WS_PUSH_RUN] parent_node=" << node_id
                              << " child_ptr=" << child.get()
                              << " type=" << DebugRenderObjectTypeName(child->GetType());
                    if (child->GetType() == RenderObjectType::TEXT) {
                        auto* text_obj = static_cast<RenderText*>(child.get());
                        std::cout << " raw='" << DebugEscapeText(text_obj ? text_obj->GetText() : std::string()) << "'";
                    }
                    std::cout << std::endl;
                }
            } else {
                // Other elements (display:none, etc.) - process normally
                BuildSubtree(child.get(), node_id);
            }
        }

        // Don't forget the last run of inline elements
        if (!current_inline_run.empty()) {
            if (debug_anon_inline_ws) {
                std::cout << "[ANON_WS_FLUSH_RUN] parent_node=" << node_id
                          << " run_size=" << current_inline_run.size() << std::endl;
            }
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

    static bool debug_anon_inline_ws = std::getenv("DEBUG_ANON_INLINE_WS") != nullptr;

    // Create a new node ID for the anonymous block
    NodeId anon_id = next_node_id_++;

    LayoutNode anon_node;
    anon_node.id = anon_id;
    anon_node.render_obj = nullptr;  // Anonymous blocks have no render object
    anon_node.parent = parent_id;
    anon_node.is_anonymous_block = true;
    anon_node.is_ifc_container = true;  // Anonymous blocks use IFC for their inline content

    if (debug_anon_inline_ws) {
        std::cout << "[ANON_WS_CREATE_BLOCK] parent_node=" << parent_id
                  << " anon_node=" << anon_id
                  << " child_count=" << inline_children.size() << std::endl;
    }

    // Store the inline children for later IFC layout
    // Also register them in render_to_node_ to prevent duplicate node creation
    // during incremental layout updates (e.g., when AddElement is called)
    for (RenderObject* child : inline_children) {
        if (debug_anon_inline_ws) {
            std::cout << "[ANON_WS_BLOCK_CHILD] anon_node=" << anon_id
                      << " child_ptr=" << child
                      << " type=" << DebugRenderObjectTypeName(child ? child->GetType() : RenderObjectType::TEXT);
            if (child && child->GetType() == RenderObjectType::TEXT) {
                auto* text_obj = static_cast<RenderText*>(child);
                std::cout << " raw='" << DebugEscapeText(text_obj ? text_obj->GetText() : std::string()) << "'";
            }
            std::cout << std::endl;
        }
        anon_node.anonymous_inline_children.push_back(child);
        // Register the inline child's RenderObject to prevent it from being
        // added again as a separate layout node. Use a special marker (anon_id)
        // to indicate it belongs to this anonymous block box.
        // This fixes the bug where inline children were being duplicated
        // as flex children during incremental layout.
        render_to_node_[child] = anon_id;
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
    // NOTE:
    // IFC/匿名块在 PerformLayout 阶段除了产出 size，还会回写 RenderObject 坐标与 wrapped-lines。
    // 仅返回缓存的 LayoutOutput 会丢失这些 side effects，导致“高度是两行但绘制仍单行”等时序问题。
    // 因此在 PerformLayout + IFC 路径下禁用该层缓存读取。

    const bool is_text_node = node->render_obj &&
                              node->render_obj->GetType() == RenderObjectType::TEXT;
    const bool is_content_size_probe =
        inputs.run_mode == RunMode::PerformLayout &&
        inputs.sizing_mode == SizingMode::ContentSize;
    bool disable_cache_read = is_content_size_probe ||
                              ((inputs.run_mode == RunMode::PerformLayout) &&
                               (node->is_ifc_container || node->is_anonymous_block ||
                                is_text_node));

    if (!disable_cache_read) {
        auto cached = node->cache.Get(
            inputs.known_dimensions,
            inputs.available_space,
            inputs.run_mode,
            node->content_version
        );
        if (cached.has_value()) {
            return *cached;
        }
    }

    LayoutOutput output;

    // Check if this is a leaf node (text, inline-block, inline-flex, etc.)
    // Leaf nodes need special measurement handling
    if (node->render_obj) {
        RenderObjectType type = node->render_obj->GetType();

        if (type == RenderObjectType::TEXT ||
            type == RenderObjectType::INLINE_BLOCK ||
            type == RenderObjectType::INLINE_FLEX ||
            type == RenderObjectType::INLINE_GRID ||
            type == RenderObjectType::INLINE) {
            output = MeasureLeafNode(node_id, inputs);

            // Store in cache and return early (pass content_version for incremental layout)
            // **Feature: incremental-layout-optimization**
            // **Validates: Requirements 1.4, 1.5**
            if (!is_content_size_probe) {
                node->cache.Store(
                    inputs.known_dimensions,
                    inputs.available_space,
                    inputs.run_mode,
                    node->content_version,
                    output
                );
                node->output = output;
            }
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
                if (!is_content_size_probe) {
                    node->cache.Store(
                        inputs.known_dimensions,
                        inputs.available_space,
                        inputs.run_mode,
                        node->content_version,
                        output
                    );
                    node->output = output;
                }
                return output;
            }
        }
    }

    // Dispatch based on display type
    switch (node->style.display) {
        case Display::None:
            output = LayoutOutput{};
            break;

        case Display::Block: {
            if (node->is_ifc_container) {
                output = ComputeIFCLayout(node_id, inputs);
            } else {
                output = ComputeBlockLayout(node_id, inputs);
            }

            // Handle overflow: auto for block layout - if content exceeds container, add scrollbar and relayout
            // Skip for anonymous blocks (no render_obj)
            // 统一滚动条处理：包括 root 节点
            // **Feature: unified-scrollbar-system**
            // **Validates: Requirements 1.1, 1.4, 3.1**
            if (node->render_obj) {
            const auto& computed = node->render_obj->GetComputedStyle();
            std::string overflow_y = !computed.overflow_y.empty() ? computed.overflow_y : computed.overflow;
            std::string overflow_x = !computed.overflow_x.empty() ? computed.overflow_x : computed.overflow;

            // Only handle overflow: auto (overflow: scroll is handled in style parsing)
            if (overflow_y == "auto" || overflow_x == "auto") {
                // Check if we need to relayout with scrollbar space
                bool needs_relayout = false;
                float scrollbar_width = RenderObject::GetScrollbarWidth();

                bool had_vertical_scrollbar = node->style.scrollbar_width > 0.0f;
                bool had_horizontal_scrollbar = node->style.scrollbar_height > 0.0f;
                float checked_content_width = -1.0f;
                float checked_effective_width = -1.0f;
                float checked_content_height = -1.0f;
                float checked_effective_height = -1.0f;

                // Reset auto scrollbar state before recomputing it
                if (computed.overflow_y != "scroll") {
                    node->style.scrollbar_width = 0.0f;
                }
                if (computed.overflow_x != "scroll") {
                    node->style.scrollbar_height = 0.0f;
                }

                // For overflow-y: auto, check if content height exceeds container height
                // 对于 root 节点，使用 available_space.height 作为容器高度
                // **Feature: unified-scrollbar-system**
                // **Validates: Requirements 1.1, 1.4**
                float fallback_height = inputs.available_space.height.IsDefinite() ?
                    inputs.available_space.height.value : output.size.height;
                float container_height = inputs.known_dimensions.height.value_or(fallback_height);
                if (overflow_y == "auto" && container_height > 0) {
                    float actual_content_height = output.content_size.height;
                    float effective_container_height = container_height - node->style.scrollbar_height;
                    checked_content_height = actual_content_height;
                    checked_effective_height = effective_container_height;
                    if (effective_container_height < 0) effective_container_height = 0;
                    if (actual_content_height > effective_container_height) {
                        // Need vertical scrollbar - update style and relayout
                        node->style.scrollbar_width = scrollbar_width;
                        needs_relayout = true;
                    }
                }

                // For overflow-x: auto, check if content width exceeds container width
                float container_width = inputs.known_dimensions.width.value_or(output.size.width);
                if (overflow_x == "auto" && container_width > 0) {
                    float actual_content_width = output.content_size.width;
                    // 考虑垂直滚动条占用的宽度
                    float effective_container_width = container_width - node->style.scrollbar_width;
                    checked_content_width = actual_content_width;
                    checked_effective_width = effective_container_width;
                    if (effective_container_width < 0) effective_container_width = 0;
                    if (actual_content_width > effective_container_width) {
                        // Need horizontal scrollbar - update style and relayout
                        node->style.scrollbar_height = scrollbar_width;
                        needs_relayout = true;
                    }
                }

                // Horizontal scrollbar may reduce available height and trigger vertical overflow
                if (overflow_y == "auto" && container_height > 0 && node->style.scrollbar_height > 0.0f) {
                    float actual_content_height = output.content_size.height;
                    float effective_container_height = container_height - node->style.scrollbar_height;
                    checked_content_height = actual_content_height;
                    checked_effective_height = effective_container_height;
                    if (effective_container_height < 0) effective_container_height = 0;
                    if (actual_content_height > effective_container_height) {
                        node->style.scrollbar_width = scrollbar_width;
                        needs_relayout = true;
                    }
                }

                bool scrollbar_changed =
                    (had_vertical_scrollbar != (node->style.scrollbar_width > 0.0f)) ||
                    (had_horizontal_scrollbar != (node->style.scrollbar_height > 0.0f));
                if (needs_relayout && scrollbar_changed) {
                    // Clear cache and relayout with scrollbar space
                    node->cache.Clear();

                    // 滚动条空间由 ComputeBlockLayoutInner 中的 scrollbar_gutter 处理
                    // 不需要在这里修改 known_dimensions，否则会导致双重减少
                    // **Feature: unified-scrollbar-system**
                    // **Validates: Requirements 1.5, 2.2**

                    if (node->is_ifc_container) {
                        output = ComputeIFCLayout(node_id, inputs);
                    } else {
                        output = ComputeBlockLayout(node_id, inputs);
                    }
                }
            }
            }  // end if (node->render_obj)
            break;
        }

        case Display::Flex: {
            output = ComputeFlexLayout(node_id, inputs);

            // Handle overflow: auto - if content exceeds container, add scrollbar and relayout
            // Skip for anonymous blocks (no render_obj)
            if (node->render_obj) {
            const auto& computed = node->render_obj->GetComputedStyle();
            std::string overflow_y = !computed.overflow_y.empty() ? computed.overflow_y : computed.overflow;
            std::string overflow_x = !computed.overflow_x.empty() ? computed.overflow_x : computed.overflow;

            // Only handle overflow: auto (overflow: scroll is handled in style parsing)
            if (overflow_y == "auto" || overflow_x == "auto") {
                // Check if we need to relayout with scrollbar space
                bool needs_relayout = false;
                float scrollbar_width = RenderObject::GetScrollbarWidth();

                bool had_vertical_scrollbar = node->style.scrollbar_width > 0.0f;
                bool had_horizontal_scrollbar = node->style.scrollbar_height > 0.0f;
                float checked_content_width = -1.0f;
                float checked_effective_width = -1.0f;
                float checked_content_height = -1.0f;
                float checked_effective_height = -1.0f;

                // Reset auto scrollbar state before recomputing it
                if (computed.overflow_y != "scroll") {
                    node->style.scrollbar_width = 0.0f;
                }
                if (computed.overflow_x != "scroll") {
                    node->style.scrollbar_height = 0.0f;
                }

                // For overflow-y: auto, check if content height exceeds container height
                float fallback_height = inputs.available_space.height.IsDefinite() ?
                    inputs.available_space.height.value : output.size.height;
                float container_height = inputs.known_dimensions.height.value_or(fallback_height);
                if (overflow_y == "auto" && container_height > 0) {
                    float actual_content_height = output.content_size.height;
                    float effective_container_height = container_height - node->style.scrollbar_height;
                    checked_content_height = actual_content_height;
                    checked_effective_height = effective_container_height;
                    if (effective_container_height < 0) effective_container_height = 0;
                    if (actual_content_height > effective_container_height) {
                        // Need vertical scrollbar - update style and relayout
                        node->style.scrollbar_width = scrollbar_width;
                        needs_relayout = true;
                    }
                }

                // For overflow-x: auto, check if content width exceeds container width
                float container_width = inputs.known_dimensions.width.value_or(output.size.width);
                if (overflow_x == "auto" && container_width > 0) {
                    float actual_content_width = output.content_size.width;
                    float effective_container_width = container_width - node->style.scrollbar_width;
                    checked_content_width = actual_content_width;
                    checked_effective_width = effective_container_width;
                    if (effective_container_width < 0) effective_container_width = 0;
                    if (actual_content_width > effective_container_width) {
                        // Need horizontal scrollbar - update style and relayout
                        node->style.scrollbar_height = scrollbar_width;
                        needs_relayout = true;
                    }
                }

                if (overflow_y == "auto" && container_height > 0 && node->style.scrollbar_height > 0.0f) {
                    float actual_content_height = output.content_size.height;
                    float effective_container_height = container_height - node->style.scrollbar_height;
                    checked_content_height = actual_content_height;
                    checked_effective_height = effective_container_height;
                    if (effective_container_height < 0) effective_container_height = 0;
                    if (actual_content_height > effective_container_height) {
                        node->style.scrollbar_width = scrollbar_width;
                        needs_relayout = true;
                    }
                }

                bool scrollbar_changed =
                    (had_vertical_scrollbar != (node->style.scrollbar_width > 0.0f)) ||
                    (had_horizontal_scrollbar != (node->style.scrollbar_height > 0.0f));
                if (needs_relayout && scrollbar_changed) {
                    // Clear cache and relayout with scrollbar space
                    node->cache.Clear();
                    output = ComputeFlexLayout(node_id, inputs);
                }
            }
            }  // end if (node->render_obj)
            break;
        }

        case Display::Grid:
            output = ComputeGridLayout(node_id, inputs);
            break;
    }

    // Store in cache (pass content_version for incremental layout invalidation)
    // **Feature: incremental-layout-optimization**
    // **Validates: Requirements 1.4, 1.5**
    if (!is_content_size_probe) {
        node->cache.Store(
            inputs.known_dimensions,
            inputs.available_space,
            inputs.run_mode,
            node->content_version,
            output
        );

        node->output = output;
    }
    return output;
}

LayoutOutput NativeLayoutEngine::ComputeBlockLayout(NodeId node_id, const LayoutInput& inputs) {
    // Use Taffy's block layout algorithm
    return mbink::ComputeBlockLayout(*this, node_id, inputs);
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
    const Style& GetContainerStyle(NodeId node) const override {
        return engine_.GetStyle(node);
    }

    const Style& GetChildStyle(NodeId node) const override {
        return engine_.GetStyle(node);
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

    // LayoutGridContainer interface - unified Style methods
    const Style& GetContainerStyle(NodeId node) const override {
        return engine_.GetStyle(node);
    }

    const Style& GetChildStyle(NodeId node) const override {
        return engine_.GetStyle(node);
    }

    // LayoutGridContainer interface - Grid-specific data methods
    const GridContainerStyle& GetGridContainerStyle(NodeId node) const override {
        return engine_.GetGridContainerStyle(node);
    }

    const GridItemStyle& GetGridItemStyle(NodeId node) const override {
        return engine_.GetGridItemStyle(node);
    }

    bool IsTextNode(NodeId node) const override {
        return engine_.IsTextNode(node);
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
    return mbink::ComputeGridLayout(adapter, node_id, inputs);
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
    const bool is_content_size_probe =
        inputs.run_mode == RunMode::PerformLayout &&
        inputs.sizing_mode == SizingMode::ContentSize;
    if (!is_content_size_probe) {
        node->cache.Store(
            inputs.known_dimensions,
            inputs.available_space,
            inputs.run_mode,
            node->content_version,
            output
        );
        node->output = output;
    }

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

    // IMPORTANT:
    // min/max-width 必须在 IFC 断行前参与可用宽度计算。
    // 否则会出现：宽窗口下先按大宽度排成单行，最后 total_width 被 max-width 裁到 400，
    // 但高度仍保留单行，导致“视觉换行了但块高度没涨”。
    float pre_ifc_min_width = style.min_width.ToPx(container_width, style.font_size);
    float pre_ifc_max_width = style.max_width.ToPx(container_width, style.font_size);
    if (pre_ifc_max_width > 0) {
        container_width = std::min(container_width, pre_ifc_max_width);
    }
    if (pre_ifc_min_width > 0) {
        container_width = std::max(container_width, pre_ifc_min_width);
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

    // =============================================================================
    // CRITICAL NOTE - IFC 临时测量绝不能回写真实布局
    // -----------------------------------------------------------------------------
    // 这个坑非常隐蔽，已实际导致 CodeMirror gutter 行号闪烁 / 重影：
    //
    // 调用链：
    //   flex_layout.cpp -> CalculateChildrenBaseLines()
    //                   -> PerformChildLayout()
    //                   -> ComputeIFCLayout()
    //
    // 在这条链路里，run_mode 虽然是 PerformLayout，但 sizing_mode=ContentSize，
    // 本质上仍然只是“中间测量 / 基线探测”，传入的 known_dimensions / available_space
    // 往往是临时值（例如 gutter 被压成 known_width=20, content_width=12）。
    //
    // 如果这里继续 apply_results=true：
    //   1. IFCLayout::ApplyLayoutResults() 会把临时 x/y 回写到 RenderText/RenderObject；
    //   2. 后续真正最终布局再写一次，就会出现同一文本节点在两个 x 之间来回跳；
    //   3. 典型现象就是 text-align:right 的单字符 gutter 数字在 8.20312 / 13.7969 之间抖动。
    //
    // 规则：
    //   - 只有“最终布局”才能回写；
    //   - 任何 ContentSize / baseline / shrink-to-fit / intrinsic measurement 路径都只能测量，
    //     不能污染 render tree 的最终坐标。
    //
    // 附近类似风险点：
    //   - ComputeAnonymousBlockIFCLayout()：匿名块 IFC 也会在中间测量里拿到临时宽度；
    //   - 任何通过 PerformChildLayout() 进入、但 sizing_mode=ContentSize 的路径；
    //   - 未来若新增 intrinsic measurement / baseline probing，必须复用同一规则。
    // =============================================================================
    bool apply_results = (inputs.run_mode == RunMode::PerformLayout) &&
                         (inputs.sizing_mode != SizingMode::ContentSize);



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
    // Note: min-height and max-height 默认约束 content-box。
    // 当前 total_height / total_width 已经包含 padding + border，
    // 因此在 content-box 模式下需要把 padding/border 加回去再做外框约束。
    // CSS spec: when min > max, min wins (apply max first, then min)
    float percent_height_base = inputs.parent_size.height.value_or(0.0f);
    float min_height = style.min_height.ToPx(percent_height_base, style.font_size);
    float max_height = style.max_height.ToPx(percent_height_base, style.font_size);
    float min_width = style.min_width.ToPx(container_width, style.font_size);
    float max_width = style.max_width.ToPx(container_width, style.font_size);

    float vertical_non_content = padding_top + padding_bottom + border_top + border_bottom;
    float horizontal_non_content = padding_left + padding_right + border_left + border_right;

    if (style.box_sizing != "border-box") {
        if (min_height > 0) min_height += vertical_non_content;
        if (max_height > 0) max_height += vertical_non_content;
        if (min_width > 0) min_width += horizontal_non_content;
        if (max_width > 0) max_width += horizontal_non_content;
    }

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

    // Layout absolutely positioned children (position: absolute/fixed)
    // IFC containers can have absolutely positioned children that need to be laid out
    // relative to the container (for absolute) or viewport (for fixed)
    // Note: We handle this differently from block layout to avoid infinite recursion.
    // For IFC containers, we directly set the layout for absolute/fixed children
    // without calling MeasureChildSize/PerformChildLayout which would trigger
    // the parent's IFC layout again.
    if (inputs.run_mode == RunMode::PerformLayout) {
        LayoutAbsoluteChildrenInIFC(node_id, total_width, total_height,
                                     padding_left + border_left,
                                     padding_right + border_right,
                                     padding_top + border_top,
                                     padding_bottom + border_bottom);
    }

    LayoutOutput output;
    output.size = Size<float>{total_width, total_height};
    output.content_size = Size<float>{result.max_width, result.total_height};

    return output;
}

void NativeLayoutEngine::LayoutAbsoluteChildrenInIFC(
    NodeId node_id,
    float container_width,
    float container_height,
    float padding_border_left,
    float padding_border_right,
    float padding_border_top,
    float padding_border_bottom
) {
    LayoutNode* node = GetNode(node_id);
    if (!node || !node->render_obj) {
        return;
    }

    // Calculate content box area for absolute positioning
    float content_box_left = padding_border_left;
    float content_box_right = padding_border_right;
    float content_box_top = padding_border_top;
    float content_box_bottom = padding_border_bottom;

    float area_width = container_width - content_box_left - content_box_right;
    float area_height = container_height - content_box_top - content_box_bottom;

    // IFC containers don't have children in the layout tree (node->children)
    // Instead, we need to iterate through the render object's children
    const auto& render_children = node->render_obj->GetChildren();

    for (const auto& child_render_obj : render_children) {
        if (!child_render_obj) continue;

        // Get the child's computed style to check position
        const auto& child_computed = child_render_obj->GetComputedStyle();

        // Skip non-absolute/fixed children
        if (child_computed.position != "absolute" && child_computed.position != "fixed") {
            continue;
        }

        // For position: fixed, use viewport size instead of parent container size
        bool is_fixed = (child_computed.position == "fixed");
        Size<float> containing_block_size = is_fixed
            ? Size<float>{ViewportSize::GetWidth(), ViewportSize::GetHeight()}
            : Size<float>{area_width, area_height};
        Point<float> containing_block_offset = is_fixed
            ? Point<float>{0.0f, 0.0f}
            : Point<float>{content_box_left, content_box_top};

        // Parse inset values directly from computed style
        float left_val = 0.0f, right_val = 0.0f, top_val = 0.0f, bottom_val = 0.0f;
        bool has_left = false, has_right = false, has_top = false, has_bottom = false;

        if (child_computed.left.unit == CSSUnit::PX) {
            left_val = child_computed.left.value;
            has_left = true;
        } else if (child_computed.left.unit == CSSUnit::PERCENT) {
            left_val = (child_computed.left.value / 100.0f) * containing_block_size.width;
            has_left = true;
        }

        if (child_computed.right.unit == CSSUnit::PX) {
            right_val = child_computed.right.value;
            has_right = true;
        } else if (child_computed.right.unit == CSSUnit::PERCENT) {
            right_val = (child_computed.right.value / 100.0f) * containing_block_size.width;
            has_right = true;
        }

        if (child_computed.top.unit == CSSUnit::PX) {
            top_val = child_computed.top.value;
            has_top = true;
        } else if (child_computed.top.unit == CSSUnit::PERCENT) {
            top_val = (child_computed.top.value / 100.0f) * containing_block_size.height;
            has_top = true;
        }

        if (child_computed.bottom.unit == CSSUnit::PX) {
            bottom_val = child_computed.bottom.value;
            has_bottom = true;
        } else if (child_computed.bottom.unit == CSSUnit::PERCENT) {
            bottom_val = (child_computed.bottom.value / 100.0f) * containing_block_size.height;
            has_bottom = true;
        }

        // Parse size from computed style
        float width = 0.0f, height = 0.0f;
        bool has_width = false, has_height = false;

        if (child_computed.width.unit == CSSUnit::PX && child_computed.width.value > 0) {
            width = child_computed.width.value;
            has_width = true;
        } else if (child_computed.width.unit == CSSUnit::PERCENT) {
            width = (child_computed.width.value / 100.0f) * containing_block_size.width;
            has_width = true;
        }

        if (child_computed.height.unit == CSSUnit::PX && child_computed.height.value > 0) {
            height = child_computed.height.value;
            has_height = true;
        } else if (child_computed.height.unit == CSSUnit::PERCENT) {
            height = (child_computed.height.value / 100.0f) * containing_block_size.height;
            has_height = true;
        }

        // If no explicit size, use a default size for inline-block elements
        // (We can't call MeasureChildSize here as it would cause infinite recursion)
        if (!has_width) width = 100.0f;  // Default width
        if (!has_height) height = 40.0f; // Default height

        // Add padding and border to size
        float padding_left = child_computed.padding_left.ToPx(containing_block_size.width, child_computed.font_size);
        float padding_right = child_computed.padding_right.ToPx(containing_block_size.width, child_computed.font_size);
        float padding_top = child_computed.padding_top.ToPx(containing_block_size.width, child_computed.font_size);
        float padding_bottom = child_computed.padding_bottom.ToPx(containing_block_size.width, child_computed.font_size);

        float border_left_w = child_computed.border_left_width;
        float border_right_w = child_computed.border_right_width;
        float border_top_w = child_computed.border_top_width;
        float border_bottom_w = child_computed.border_bottom_width;

        float total_width = width + padding_left + padding_right + border_left_w + border_right_w;
        float total_height = height + padding_top + padding_bottom + border_top_w + border_bottom_w;

        // Compute location
        float loc_x = 0.0f, loc_y = 0.0f;

        // X position
        if (has_left) {
            loc_x = containing_block_offset.x + left_val;
        } else if (has_right) {
            loc_x = containing_block_offset.x + containing_block_size.width - total_width - right_val;
        } else {
            loc_x = containing_block_offset.x;
        }

        // Y position
        if (has_top) {
            loc_y = containing_block_offset.y + top_val;
        } else if (has_bottom) {
            loc_y = containing_block_offset.y + containing_block_size.height - total_height - bottom_val;
        } else {
            loc_y = containing_block_offset.y;
        }

        // Set layout directly on the render object
        auto& layout_info = child_render_obj->GetLayoutInfo();
        layout_info.x = loc_x;
        layout_info.y = loc_y;
        layout_info.width = total_width;
        layout_info.height = total_height;
    }
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

    // =============================================================================
    // NOTE - AnonymousBlock IFC 也必须遵守“ContentSize 只测量、不回写”的规则
    // -----------------------------------------------------------------------------
    // 这里和主 ComputeIFCLayout() 是同类风险点：
    // - 调用方可能只是为了 baseline / intrinsic-size / shrink-to-fit 做中间测量；
    // - 这时 known_dimensions / parent_size / available_space 可能都是临时参考值；
    // - 如果 apply_results=true，就会把匿名块 line box 的临时结果写回真实 RenderText，
    //   继而在最终布局阶段与正式坐标互相覆盖，形成抖动。
    //
    // 因此这里也统一约束：
    //   只有最终 PerformLayout 且 sizing_mode 不是 ContentSize 时，才能回写真实布局。
    // =============================================================================
    bool apply_results = (inputs.run_mode == RunMode::PerformLayout) &&
                         (inputs.sizing_mode != SizingMode::ContentSize);
    static bool debug_fab_inline = std::getenv("DEBUG_FAB_INLINE") != nullptr;

    // Resolve container width
    // Root cause fix: percentage width for inline/inline-block children inside anonymous block
    // must prefer parent content-box width passed in via inputs.parent_size.width.
    // parent->layout/output can be parent outer(border-box) width in some passes, which makes
    // width:100% (content-box) elements add padding+border again and overflow.
    float container_width = 0.0f;
    const char* container_width_source = "none";

    // Anonymous block inline formatting context must align against the current pass's
    // parent content-box width. Using parent render object's cached content_rect first
    // can pick up stale width from a previous state (e.g. old active gutter width),
    // which makes text-align:right oscillate between two reference widths.
    if (apply_results && inputs.parent_size.width.has_value() && *inputs.parent_size.width > 0.0f) {
        container_width = *inputs.parent_size.width;
        container_width_source = "inputs.parent_size";
    }

    if (container_width <= 0.0f && inputs.known_dimensions.width.has_value() &&
        *inputs.known_dimensions.width > 0.0f) {
        container_width = *inputs.known_dimensions.width;
        container_width_source = "inputs.known_dimensions";
    }

    if (container_width <= 0.0f && parent && parent->render_obj) {
        const LayoutInfo& parent_layout_info = parent->render_obj->GetLayoutInfo();
        const float parent_content_width = parent_layout_info.content_rect.width();
        if (parent_content_width > 0.0f) {
            container_width = parent_content_width;
            container_width_source = "parent.render_obj.content_rect";
        }
    }

    if (container_width <= 0.0f && !apply_results && inputs.parent_size.width.has_value() && *inputs.parent_size.width > 0.0f) {
        container_width = *inputs.parent_size.width;
        container_width_source = "inputs.parent_size";
    }

    if (container_width <= 0.0f && apply_results && parent) {
        float parent_layout_width = parent->layout.size.width;
        if (parent_layout_width > 0.0f) {
            container_width = parent_layout_width;
            container_width_source = "parent.layout";
        }
    }

    if (container_width <= 0.0f && apply_results && parent) {
        float parent_output_width = parent->output.size.width;
        if (parent_output_width > 0.0f) {
            container_width = parent_output_width;
            container_width_source = "parent.output";
        }
    }

    if (container_width <= 0.0f && parent && parent->render_obj) {
        const auto& parent_style = parent->render_obj->GetComputedStyle();
        if (parent_style.width.unit == CSSUnit::PX && parent_style.width.value > 0.0f) {
            container_width = parent_style.width.value;
            container_width_source = "parent.style.px";
        } else if (parent_style.width.unit == CSSUnit::PERCENT && inputs.parent_size.width.has_value()) {
            container_width = (parent_style.width.value / 100.0f) * (*inputs.parent_size.width);
            container_width_source = "parent.style.percent";
        }
    }

    if (container_width <= 0.0f) {
        if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
            container_width = inputs.available_space.width.value;
            container_width_source = "inputs.available.definite";
        } else if (inputs.available_space.width.type == AvailableSpace::Type::MaxContent) {
            container_width = 10000.0f;
            container_width_source = "inputs.available.max_content";
        }
    }

    if (debug_fab_inline) {
        std::cout << "[FAB_IFC_SRC] node=" << node->id
                  << " parent=" << (parent ? parent->id : 0)
                  << " apply=" << (apply_results ? 1 : 0)
                  << " run_mode=" << static_cast<int>(inputs.run_mode)
                  << " container_w=" << container_width
                  << " src=" << container_width_source
                  << " parent_layout_w=" << (parent ? parent->layout.size.width : -1.0f)
                  << " parent_output_w=" << (parent ? parent->output.size.width : -1.0f)
                  << " in_parent_w=" << (inputs.parent_size.width.has_value() ? *inputs.parent_size.width : -1.0f)
                  << " in_known_w=" << (inputs.known_dimensions.width.has_value() ? *inputs.known_dimensions.width : -1.0f)
                  << " in_avail_type=" << static_cast<int>(inputs.available_space.width.type)
                  << " in_avail_w=" << inputs.available_space.width.value
                  << std::endl;
    }



    // Anonymous blocks have no padding/border/margin
    float content_width = container_width;

    // Use IFC to layout the inline children
    // We need to create a temporary container for IFC layout
    // The inline children are stored in anonymous_inline_children

    float total_width = 0.0f;
    float total_height = 0.0f;

    // Create inline boxes for all inline children and perform IFC layout
    // We'll use the IFCLayout class directly with the inline children

    // Only PerformLayout pass can update IFC cached results used by ReadLayoutResults().
    // Measure pass should not clear/write these caches, otherwise it may overwrite
    // correct coordinates with temporary measurement coordinates.
    if (apply_results) {
        node->ifc_inline_boxes.clear();
        node->ifc_line_boxes.clear();
    }

    // Collect inline boxes from all inline children
    std::vector<InlineBox> all_inline_boxes;
    static bool debug_anon_inline_ws = std::getenv("DEBUG_ANON_INLINE_WS") != nullptr;

    for (RenderObject* inline_child : node->anonymous_inline_children) {
        if (!inline_child) continue;

        RenderObjectType child_type = inline_child->GetType();
        const auto& child_style = inline_child->GetComputedStyle();

        if (child_type == RenderObjectType::TEXT) {
            // Text node - create text inline box
            RenderText* text_obj = static_cast<RenderText*>(inline_child);
            const std::string& text = text_obj->GetText();
            if (text.empty()) continue;

            float letter_spacing = child_style.letter_spacing.ToPx(0, child_style.font_size);
            float word_spacing = child_style.word_spacing.ToPx(0, child_style.font_size);

            std::string processed_text = NormalizeInlineTextForWhiteSpace(text, child_style);
            if (debug_anon_inline_ws) {
                std::cout << "[ANON_WS_TEXT_BOX] node=" << node->id
                          << " child_ptr=" << inline_child
                          << " raw='" << DebugEscapeText(text) << "'"
                          << " processed='" << DebugEscapeText(processed_text) << "'"
                          << std::endl;
            }
            if (processed_text.empty()) continue;

            auto measurement = IFCLayout::MeasureTextStatic(
                processed_text, child_style.font_size, child_style.font_family,
                letter_spacing, word_spacing, child_style.line_height,
                child_style.font_weight, child_style.font_style);

            InlineBox box = InlineBox::CreateTextBox(inline_child);
            box.width = measurement.width;
            box.height = measurement.height;
            box.baseline = measurement.skia_ascent;
            box.skia_ascent = measurement.skia_ascent;
            box.skia_descent = measurement.skia_descent;
            box.line_height_multiplier = child_style.line_height;

            TextRun run;
            run.text = processed_text;
            run.start_offset = 0;
            run.end_offset = processed_text.size();
            run.width = measurement.width;
            run.height = measurement.height;
            run.baseline = measurement.skia_ascent;
            run.is_whitespace = run.IsOnlyWhitespace();
            box.text_runs.push_back(run);

            if (debug_anon_inline_ws) {
                std::cout << "[ANON_WS_TEXT_MEASURE] node=" << node->id
                          << " child_ptr=" << inline_child
                          << " width=" << measurement.width
                          << " height=" << measurement.height
                          << " is_whitespace=" << (run.is_whitespace ? 1 : 0)
                          << std::endl;
            }

            all_inline_boxes.push_back(std::move(box));
        }
        else if (child_type == RenderObjectType::INLINE_BLOCK) {
            // Inline-block element (e.g., button)
            auto* inline_block = static_cast<RenderInlineBlock*>(inline_child);
            auto [w, h] = inline_block->MeasureIntrinsicSize(content_width);

            InlineBox box = InlineBox::CreateAtomicBox(inline_child, w, h, h);
            box.margin_left = child_style.margin_left.ToPx(w, child_style.font_size);
            box.margin_right = child_style.margin_right.ToPx(w, child_style.font_size);
            // ✅ Add vertical margin support for inline-block elements
            box.margin_top = child_style.margin_top.ToPx(w, child_style.font_size);
            box.margin_bottom = child_style.margin_bottom.ToPx(w, child_style.font_size);
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
    line_breaker.SetOverflowWrap(OverflowWrapMode::NORMAL);

    // 对齐主 IFC 的 white-space / word-break 处理
    if (parent && parent->render_obj) {
        const auto& parent_style = parent->render_obj->GetComputedStyle();
        if (parent_style.white_space == "pre" || parent_style.white_space == "pre-wrap") {
            line_breaker.SetWhiteSpace(WhiteSpaceMode::PRE_WRAP);
        } else if (parent_style.white_space == "nowrap") {
            line_breaker.SetWhiteSpace(WhiteSpaceMode::NOWRAP);
        } else {
            line_breaker.SetWhiteSpace(WhiteSpaceMode::NORMAL);
        }

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
        line_breaker.SetWhiteSpace(WhiteSpaceMode::NORMAL);
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

        if (debug_fab_inline) {
            std::cout << "[FAB_IFC_LINE_PRE] node=" << node->id
                      << " line_y=" << line.y
                      << " line_x=" << line.x
                      << " avail_w=" << line.available_width
                      << " content_w=" << line.content_width
                      << " align=" << text_align
                      << " box_count=" << line.boxes.size()
                      << std::endl;
            for (auto* dbg_box : line.boxes) {
                if (!dbg_box) continue;
                if (dbg_box->type == InlineBoxType::TEXT) {
                    std::cout << "[FAB_IFC_LINE_PRE_BOX] type=TEXT x=" << dbg_box->x
                              << " y=" << dbg_box->y
                              << " w=" << dbg_box->width
                              << " h=" << dbg_box->height
                              << std::endl;
                }
            }
        }

        // Set horizontal positions
        float current_x = line.x;
        for (auto* box : line.boxes) {
            if (!box) continue;

            if (box->IsInlineStart()) {
                // INLINE_START：跳过左侧 margin + padding + border，推进 current_x
                // 这样后续文本盒的 x 会正确从 padding 之后开始
                current_x += box->margin_left + box->padding_left + box->border_left;
                box->x = current_x;
                // width = 0，右侧由对应 INLINE_END 处理
            } else if (box->IsInlineEnd()) {
                // INLINE_END：先记录当前位置，再跳过右侧 padding + border + margin
                box->x = current_x;
                current_x += box->padding_right + box->border_right + box->margin_right;
            } else {
                // TEXT 或 ATOMIC：正常处理 margin + width
                current_x += box->margin_left;  // 先跳过左边距
                box->x = current_x;             // 内容区域从这里开始
                current_x += box->width + box->margin_right;  // 移动到下一个盒子的起始位置
            }

            if (debug_anon_inline_ws) {
                std::cout << "[ANON_WS_BOX_POS] node=" << node->id
                          << " type="
                          << (box->IsInlineStart() ? "INLINE_START" :
                              box->IsInlineEnd() ? "INLINE_END" :
                              box->IsText() ? "TEXT" :
                              box->IsAtomic() ? "ATOMIC" : "OTHER")
                          << " render_obj=" << box->render_object
                          << " x=" << box->x
                          << " w=" << box->width;
                if (box->IsText() && !box->text_runs.empty()) {
                    std::cout << " text='" << DebugEscapeText(box->text_runs[0].text) << "'";
                }
                std::cout << std::endl;
            }
        }

        // Apply vertical alignment
        vertical_aligner.AlignBoxes(box_ptrs, aligns, current_y, container_line_height);

        // Apply text-align
        line.ApplyTextAlign(text_align);

        if (debug_fab_inline) {
            std::cout << "[FAB_IFC_LINE_POST] node=" << node->id
                      << " line_y=" << line.y
                      << " line_x=" << line.x
                      << " avail_w=" << line.available_width
                      << " content_w=" << line.content_width
                      << " align=" << text_align
                      << std::endl;
            for (auto* dbg_box : line.boxes) {
                if (!dbg_box) continue;
                if (dbg_box->type == InlineBoxType::TEXT) {
                    std::cout << "[FAB_IFC_LINE_POST_BOX] type=TEXT x=" << dbg_box->x
                              << " y=" << dbg_box->y
                              << " w=" << dbg_box->width
                              << " h=" << dbg_box->height
                              << std::endl;
                }
            }
        }

        current_y += line_metrics.line_height;
        max_line_width = std::max(max_line_width, line.content_width);
    }

    total_height = current_y;
    total_width = max_line_width;

    // Persist IFC caches only in PerformLayout pass.
    // This prevents measure-pass coordinates (e.g. container_w=388/400) from
    // polluting final ApplyAnonymousBlockLayoutResults() in ReadLayoutResults().
    if (apply_results) {
        // line_boxes stores pointers into all_inline_boxes.
        // Preserve exact box identity across move; matching only by render_object/type
        // is ambiguous when the same RenderText is split into multiple InlineBox fragments.
        std::unordered_map<const InlineBox*, size_t> old_box_index_map;
        old_box_index_map.reserve(all_inline_boxes.size());
        for (size_t i = 0; i < all_inline_boxes.size(); ++i) {
            old_box_index_map.emplace(&all_inline_boxes[i], i);
        }

        node->ifc_inline_boxes = std::move(all_inline_boxes);

        for (auto& line : line_boxes) {
            for (size_t i = 0; i < line.boxes.size(); ++i) {
                InlineBox* old_ptr = line.boxes[i];
                if (!old_ptr) continue;

                auto it = old_box_index_map.find(old_ptr);
                if (it == old_box_index_map.end()) continue;
                if (it->second >= node->ifc_inline_boxes.size()) continue;

                line.boxes[i] = &node->ifc_inline_boxes[it->second];
            }
        }
        node->ifc_line_boxes = std::move(line_boxes);
    }

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

            std::string processed_text = NormalizeInlineTextForWhiteSpace(text, child_style);
            if (processed_text.empty()) continue;

            auto measurement = IFCLayout::MeasureTextStatic(
                processed_text, child_style.font_size, child_style.font_family,
                letter_spacing, word_spacing, child_style.line_height,
                child_style.font_weight, child_style.font_style);

            InlineBox box = InlineBox::CreateTextBox(child.get());
            box.width = measurement.width;
            box.height = measurement.height;
            box.baseline = measurement.skia_ascent;
            box.skia_ascent = measurement.skia_ascent;
            box.skia_descent = measurement.skia_descent;
            box.line_height_multiplier = child_style.line_height;

            TextRun run;
            run.text = processed_text;
            run.start_offset = 0;
            run.end_offset = processed_text.size();
            run.width = measurement.width;
            run.height = measurement.height;
            run.baseline = measurement.skia_ascent;
            run.is_whitespace = run.IsOnlyWhitespace();
            box.text_runs.push_back(run);

            inline_boxes.push_back(std::move(box));
        }
        else if (child_type == RenderObjectType::INLINE_BLOCK) {
            auto* inline_block = static_cast<RenderInlineBlock*>(child.get());
            auto [w, h] = inline_block->MeasureIntrinsicSize(available_width);

            InlineBox box = InlineBox::CreateAtomicBox(child.get(), w, h, h);
            box.margin_left = child_style.margin_left.ToPx(w, child_style.font_size);
            box.margin_right = child_style.margin_right.ToPx(w, child_style.font_size);
            // ✅ Add vertical margin support for inline-block elements
            box.margin_top = child_style.margin_top.ToPx(w, child_style.font_size);
            box.margin_bottom = child_style.margin_bottom.ToPx(w, child_style.font_size);
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

    auto accumulate_inline_ancestor_offset = [](RenderObject* render_obj) {
        std::pair<float, float> offset{0.0f, 0.0f};
        auto ancestor = render_obj ? render_obj->GetParent() : nullptr;
        while (ancestor && ancestor->GetType() == RenderObjectType::INLINE) {
            const LayoutInfo& ancestor_layout = ancestor->GetLayoutInfo();
            offset.first += ancestor_layout.x;
            offset.second += ancestor_layout.y;
            ancestor = ancestor->GetParent();
        }
        return offset;
    };

    float offset_x = node->layout.location.x;
    float offset_y = node->layout.location.y;

    // 预聚合：把匿名块 IFC 的实际断行结果同步给 RenderText
    std::unordered_map<RenderObject*, std::vector<std::string>> text_wrapped_lines;
    std::unordered_map<RenderObject*, std::vector<float>> text_wrapped_line_first_x;
    std::unordered_map<RenderObject*, std::vector<float>> text_wrapped_line_first_y;
    for (const auto& line_box : node->ifc_line_boxes) {
        std::unordered_map<RenderObject*, std::string> line_fragments;
        std::unordered_map<RenderObject*, float> line_first_x;
        std::unordered_map<RenderObject*, float> line_first_y;

        for (InlineBox* line_box_item : line_box.boxes) {
            if (!line_box_item || !line_box_item->IsText() || !line_box_item->render_object) continue;
            RenderObject* text_render_obj = line_box_item->render_object;
            if (text_render_obj->GetType() != RenderObjectType::TEXT) continue;

            std::string fragment;
            for (const auto& run : line_box_item->text_runs) {
                fragment += run.text;
            }
            line_fragments[text_render_obj] += fragment;

            if (line_first_x.find(text_render_obj) == line_first_x.end()) {
                line_first_x[text_render_obj] = line_box_item->x + offset_x;
                line_first_y[text_render_obj] = line_box_item->y + offset_y;
            }
        }

        for (auto& [text_obj, line_text] : line_fragments) {
            if (line_text.empty()) continue;
            text_wrapped_lines[text_obj].push_back(line_text);
            auto it_x = line_first_x.find(text_obj);
            text_wrapped_line_first_x[text_obj].push_back(
                it_x != line_first_x.end() ? it_x->second : 0.0f);
            auto it_y = line_first_y.find(text_obj);
            text_wrapped_line_first_y[text_obj].push_back(
                it_y != line_first_y.end() ? it_y->second : 0.0f);
        }
    }

    struct Bounds {
        float min_x = std::numeric_limits<float>::max();
        float min_y = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float max_y = std::numeric_limits<float>::lowest();
        float first_x = 0.0f;
        float first_y = 0.0f;
        bool has_first = false;
        bool has_content = false;
    };

    std::unordered_map<RenderObject*, Bounds> inline_bounds;
    std::unordered_map<RenderObject*, Bounds> text_bounds;
    std::vector<RenderObject*> inline_stack;

    // 第一遍：写回原子盒；文本盒只累计边界，避免被最后一个 fragment 覆盖
    for (const auto& box : node->ifc_inline_boxes) {
        if (!box.render_object) continue;

        if (box.type == InlineBoxType::INLINE_START) {
            inline_stack.push_back(box.render_object);
            inline_bounds[box.render_object] = Bounds{};
            continue;
        }
        if (box.type == InlineBoxType::INLINE_END) {
            if (!inline_stack.empty() && inline_stack.back() == box.render_object) {
                inline_stack.pop_back();
            }
            continue;
        }

        if (box.type != InlineBoxType::TEXT && box.type != InlineBoxType::ATOMIC) continue;

        float box_left = box.x + offset_x;
        float box_top = box.y + offset_y;
        float box_right = box_left + box.width;
        float box_bottom = box_top + box.height;

        if (box.type == InlineBoxType::TEXT) {
            auto& b = text_bounds[box.render_object];
            if (!b.has_content) {
                b.min_x = box_left; b.min_y = box_top; b.max_x = box_right; b.max_y = box_bottom;
                b.has_content = true;
            } else {
                b.min_x = std::min(b.min_x, box_left);
                b.min_y = std::min(b.min_y, box_top);
                b.max_x = std::max(b.max_x, box_right);
                b.max_y = std::max(b.max_y, box_bottom);
            }
            if (!b.has_first) {
                b.first_x = box_left;
                b.first_y = box_top;
                b.has_first = true;
            }
        } else {
            LayoutInfo& layout = box.render_object->GetLayoutInfo();
            layout.x = box_left;
            layout.y = box_top;
            layout.width = box.width;
            layout.height = box.height;
            layout.is_laid_out = true;

            if (box.render_object->GetType() == RenderObjectType::INLINE_BLOCK) {
                auto* inline_block = static_cast<RenderInlineBlock*>(box.render_object);
                inline_block->Layout(box.width, box.height);
            }
        }

        for (RenderObject* inline_elem : inline_stack) {
            auto& b = inline_bounds[inline_elem];
            // [BugFix] 记录首片段位置，用于多行 inline 元素的 layout 起点
            if (!b.has_content) {
                b.first_x = box_left;
                b.first_y = box_top;
                b.has_first = true;
            }
            b.min_x = std::min(b.min_x, box_left);
            b.min_y = std::min(b.min_y, box_top);
            b.max_x = std::max(b.max_x, box_right);
            b.max_y = std::max(b.max_y, box_bottom);
            b.has_content = true;
        }
    }

    // 第二遍：内联元素边界
    // [BugFix] 计算 inline 元素的 layout 时需要包含 padding 和 border，
    // 与 ifc_layout.cpp 中 ApplyLayoutResults 的第二遍逻辑保持一致。
    // 原实现只使用 min_x/min_y 作为 layout.x/y，不包含 padding/border，
    // 导致背景绘制区域仅覆盖文本内容而非整个 padding+border 区域。
    for (auto& [render_obj, b] : inline_bounds) {
        if (!b.has_content) continue;

        const auto& iline_style = render_obj->GetComputedStyle();
        float ipl = iline_style.padding.left.ToPx(0.0f, iline_style.font_size);
        float ipr = iline_style.padding.right.ToPx(0.0f, iline_style.font_size);
        float ipt = iline_style.padding.top.ToPx(0.0f, iline_style.font_size);
        float ipb = iline_style.padding.bottom.ToPx(0.0f, iline_style.font_size);
        // 优先使用分侧 border 宽度，回退到统一 border.width
        float ibl = (iline_style.border_left_width > 0.0f)
                        ? iline_style.border_left_width
                        : iline_style.border.width.ToPx(0.0f, iline_style.font_size);
        float ibr = (iline_style.border_right_width > 0.0f)
                        ? iline_style.border_right_width
                        : iline_style.border.width.ToPx(0.0f, iline_style.font_size);
        float ibt = (iline_style.border_top_width > 0.0f)
                        ? iline_style.border_top_width
                        : iline_style.border.width.ToPx(0.0f, iline_style.font_size);
        float ibb = (iline_style.border_bottom_width > 0.0f)
                        ? iline_style.border_bottom_width
                        : iline_style.border.width.ToPx(0.0f, iline_style.font_size);

        LayoutInfo& layout = render_obj->GetLayoutInfo();
        // 用首片段位置（first_x）定位 border-box 左上角，向左/上扩展 padding+border
        float origin_x = b.has_first ? b.first_x : b.min_x;
        float origin_y = b.has_first ? b.first_y : b.min_y;
        layout.x = origin_x - ipl - ibl;
        layout.y = origin_y - ipt - ibt;
        // 宽高加上两侧 padding + border
        layout.width  = (b.max_x - b.min_x) + ipl + ipr + ibl + ibr;
        layout.height = (b.max_y - b.min_y) + ipt + ipb + ibt + ibb;
        layout.is_laid_out = true;
    }

    // 第三遍：文本节点合并边界（高度 = 多行总高度）
    for (auto& [render_obj, b] : text_bounds) {
        if (!b.has_content) continue;
        LayoutInfo& layout = render_obj->GetLayoutInfo();

        auto parent = render_obj->GetParent();
        auto inline_ancestor_offset = accumulate_inline_ancestor_offset(render_obj);
        if (parent && parent->GetType() == RenderObjectType::INLINE) {
            float new_x = (b.has_first ? b.first_x : b.min_x) - inline_ancestor_offset.first;
            layout.x = new_x;
            layout.y = (b.has_first ? b.first_y : b.min_y) - inline_ancestor_offset.second;
        } else {
            float new_x = b.has_first ? b.first_x : b.min_x;
            layout.x = new_x;
            layout.y = b.has_first ? b.first_y : b.min_y;
        }

        layout.width = b.max_x - b.min_x;
        layout.height = b.max_y - b.min_y;
        layout.is_laid_out = true;
    }

    // 第四遍：同步 IFC 实际分行与每行 x/y 偏移到 RenderText
    for (auto& [render_obj, lines] : text_wrapped_lines) {
        if (!render_obj || render_obj->GetType() != RenderObjectType::TEXT) continue;

        auto it_x = text_wrapped_line_first_x.find(render_obj);
        auto it_y = text_wrapped_line_first_y.find(render_obj);
        const std::vector<float> empty_offsets;
        const std::vector<float>& line_abs_x_list = (it_x != text_wrapped_line_first_x.end()) ? it_x->second : empty_offsets;
        const std::vector<float>& line_abs_y_list = (it_y != text_wrapped_line_first_y.end()) ? it_y->second : empty_offsets;

        const LayoutInfo& text_layout = render_obj->GetLayoutInfo();
        float abs_text_origin_x = text_layout.x;
        float abs_text_origin_y = text_layout.y;
        auto inline_ancestor_offset = accumulate_inline_ancestor_offset(render_obj);
        abs_text_origin_x += inline_ancestor_offset.first;
        abs_text_origin_y += inline_ancestor_offset.second;

        std::vector<float> local_x_offsets;
        local_x_offsets.reserve(line_abs_x_list.size());
        for (float abs_x : line_abs_x_list) {
            local_x_offsets.push_back(abs_x - abs_text_origin_x);
        }

        std::vector<float> local_y_offsets;
        local_y_offsets.reserve(line_abs_y_list.size());
        for (float abs_y : line_abs_y_list) {
            local_y_offsets.push_back(abs_y - abs_text_origin_y);
        }

        auto* text_obj = static_cast<RenderText*>(render_obj);
        if (!local_x_offsets.empty() && local_x_offsets.size() == lines.size() &&
            !local_y_offsets.empty() && local_y_offsets.size() == lines.size()) {
            text_obj->SetWrappedLinesWithOffsets(lines, local_x_offsets, local_y_offsets);
        } else if (!local_x_offsets.empty() && local_x_offsets.size() == lines.size()) {
            text_obj->SetWrappedLinesWithOffsets(lines, local_x_offsets);
        } else {
            text_obj->SetWrappedLines(lines);
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
        desc.weight = ParseCSSFontWeight(style.font_weight);
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
            text_obj->SetWrappedLinesWithAlignedOffsets(lines, available_width);

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
        } else if (inputs.available_space.width.type == AvailableSpace::Type::MinContent) {
            // For MinContent, use a large value to measure intrinsic size,
            // then the caller will use the minimum width
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
        // ✅ FIX: Respect known_dimensions from flex/grid layout
        // If external layout has calculated a target size, seed layout_info_ first
        // so RenderInlineBlock::Layout() preserves that assigned size instead of
        // recalculating shrink-to-fit dimensions.
        if (inputs.known_dimensions.width.has_value() && inputs.known_dimensions.height.has_value()) {
            auto* inline_block = static_cast<RenderInlineBlock*>(render_obj);

            inline_block->SetExternalLayoutSize(*inputs.known_dimensions.width,
                                                *inputs.known_dimensions.height);
            inline_block->Layout(*inputs.known_dimensions.width,
                                 *inputs.known_dimensions.height);

            LayoutOutput output;
            output.size = Size<float>{inline_block->GetLayoutInfo().width,
                                      inline_block->GetLayoutInfo().height};
            output.content_size = output.size;
            output.margins_can_collapse_through = false;

            return output;
        }

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
            // Inline-block elements don't participate in margin collapsing
            output.margins_can_collapse_through = false;
            return output;
        }

        // Regular inline-block element
        auto* inline_block = static_cast<RenderInlineBlock*>(render_obj);
        auto [width, height] = inline_block->MeasureIntrinsicSize(available_width);

        LayoutOutput output;
        output.size = Size<float>{width, height};
        output.content_size = output.size;
        // ✅ FIX: Inline-block elements don't participate in margin collapsing
        // CSS spec: The margins of inline-block elements do NOT collapse with
        // the vertical margins of adjacent elements or their descendants.
        // By setting margins_can_collapse_through = false, we ensure that
        // the block layout algorithm will NOT apply margin collapsing to this element.
        output.margins_can_collapse_through = false;
        return output;
    }

    // ✅ Handle inline-flex elements (similar to inline-block)
    if (type == RenderObjectType::INLINE_FLEX || type == RenderObjectType::INLINE_GRID) {
        // Respect known_dimensions from flex layout
        if (inputs.known_dimensions.width.has_value() && inputs.known_dimensions.height.has_value()) {
            auto* inline_flex = static_cast<RenderInlineFlex*>(render_obj);

            inline_flex->SetExternalLayoutSize(*inputs.known_dimensions.width,
                                               *inputs.known_dimensions.height);
            inline_flex->Layout(*inputs.known_dimensions.width, *inputs.known_dimensions.height);

            LayoutOutput output;
            output.size = Size<float>{inline_flex->GetLayoutInfo().width,
                                      inline_flex->GetLayoutInfo().height};
            output.content_size = output.size;
            output.margins_can_collapse_through = false;

            return output;
        }

        float available_width = 0.0f;
        if (inputs.available_space.width.type == AvailableSpace::Type::Definite) {
            available_width = inputs.available_space.width.value;
        } else if (inputs.available_space.width.type == AvailableSpace::Type::MaxContent) {
            available_width = 10000.0f;
        }

        // Call MeasureIntrinsicSize to get proper size
        auto* inline_flex = static_cast<RenderInlineFlex*>(render_obj);
        auto [width, height] = inline_flex->MeasureIntrinsicSize(available_width);

        LayoutOutput output;
        output.size = Size<float>{width, height};
        output.content_size = output.size;
        output.margins_can_collapse_through = false;
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

    // 统一规则：
    // - is_laid_out=true 表示元素自身尺寸/子树布局已完成
    // - 但元素在父格式化上下文中的最终位置通常在更后阶段才确定
    // 因此这里不按具体类型做特判，而是统一允许“位置同步”，仅跳过“尺寸覆盖”。
    LayoutInfo& info = render_obj->GetLayoutInfo();
    RenderObjectType type = render_obj->GetType();

    bool is_table_internal = (type == RenderObjectType::TABLE_ROW_GROUP ||
                              type == RenderObjectType::TABLE_HEADER_GROUP ||
                              type == RenderObjectType::TABLE_FOOTER_GROUP ||
                              type == RenderObjectType::TABLE_ROW ||
                              type == RenderObjectType::TABLE_CELL ||
                              type == RenderObjectType::TABLE_CAPTION);

    LayoutNode* parent_node = (node->parent != 0) ? GetNode(node->parent) : nullptr;
    bool is_text_under_anonymous_block = (type == RenderObjectType::TEXT &&
                                          info.is_laid_out &&
                                          node &&
                                          node->is_anonymous_block);

    if (info.is_laid_out) {
        // TABLE 内部盒由 RenderTable::Layout 完整管理，这里不覆盖。
        if (is_table_internal) {
            return;
        }
    }

    if (is_text_under_anonymous_block) {
        info.is_laid_out = true;
        render_obj->ClearNeedsLayout();
        return;
    }

    // Update render object with layout info

    if (!is_table_internal) {
        // Normal elements: always sync the latest position and size from the layout tree.
        // `is_laid_out` only means the node has valid layout info; it must not freeze the
        // render object on an older size after content or style changes.
        bool position_changed = (info.x != node->layout.location.x || info.y != node->layout.location.y);
        bool size_changed = (info.width != node->output.size.width || info.height != node->output.size.height);

        if (position_changed || size_changed) {
            render_obj->MarkNeedsPaint();
            render_obj->InvalidatePaintCache();
            render_obj->InvalidateViewportBounds();
            render_obj->InvalidateDescendantViewportBounds();
        }

        // Position is always determined by parent formatting context / layout tree.
        info.x = node->layout.location.x;
        info.y = node->layout.location.y;

        info.width = node->output.size.width;
        info.height = node->output.size.height;
    }
    // For TABLE internal elements, their layout is fully managed by RenderTable::Layout
    // We only mark them as laid out, but preserve their positions and dimensions
    info.is_laid_out = true;

    // 同步 layout 侧 content_size，避免 render 回退遍历 render tree 时
    // 将 block 容器下不应影响布局的空白文本节点再次算入滚动尺寸。
    render_obj->SetContentSize(node->output.content_size.width, node->output.content_size.height);

    // 清除 RenderObject 的 needs_layout_ 标志
    // 这对于 Paint 中的内容尺寸缓存优化很重要
    render_obj->ClearNeedsLayout();

    // For inline elements (like span), we only reposition children when this render object
    // owns a real inline LayoutNode. Inline descendants managed by anonymous block IFC have
    // already received final fragment positions from ApplyAnonymousBlockLayoutResults();
    // calling PositionChildrenOnly() again would re-run inline text-align and overwrite them.
    if (type == RenderObjectType::INLINE && node->render_obj == render_obj) {
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
            inline_block->SetExternalLayoutSize(info.width, info.height);
            inline_block->Layout(info.width, info.height);
        }
    }

    // For inline-flex and inline-grid elements, we need to call Layout() to properly
    // position their children. This is necessary because:
    // 1. When inline-flex/inline-grid is a child of a flex container, flex layout calls
    //    tree.PerformChildLayout() which performs Taffy layout and sets the element's size
    // 2. But PerformChildLayout() does NOT call RenderInlineFlex::Layout(), so the
    //    element's children (e.g., text nodes) are never laid out
    // 3. This causes child text nodes to have rect=(0,0,0,0), making them invisible
    // 4. Similar to how IFC handles inline-flex (calling inline_flex->Layout() in ApplyLayoutResults),
    //    we need to call Layout() here after flex layout completes
    if (type == RenderObjectType::INLINE_FLEX || type == RenderObjectType::INLINE_GRID) {
        if (type == RenderObjectType::INLINE_FLEX) {
            auto* inline_flex = static_cast<RenderInlineFlex*>(render_obj);
            inline_flex->SetExternalLayoutSize(info.width, info.height);
            inline_flex->Layout(info.width, info.height);
        }
        // inline-grid can be added here in the future

        // Mark children as laid out to prevent ReadLayoutResults from overwriting their positions
        for (auto& child : render_obj->GetChildren()) {
            LayoutInfo& child_info = child->GetLayoutInfo();
            child_info.is_laid_out = true;
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

    // Handle anonymous block boxes in children
    // Anonymous blocks don't have render_obj, so we need to process their inline children
    bool has_anonymous_block_child = false;
    for (NodeId child_id : node->children) {
        LayoutNode* child_node = GetNode(child_id);
        if (child_node && child_node->is_anonymous_block) {
            has_anonymous_block_child = true;
            // Apply layout results for anonymous block's inline children
            ApplyAnonymousBlockLayoutResults(child_node);
        }
    }

    // Handle IFC containers
    // IFC inline children are positioned by ApplyAnonymousBlockLayoutResults() above.
    // Here we only mark render-tree children as laid out, then return to avoid
    // ReadLayoutResults recursively overwriting IFC-assigned positions.
    if (node->is_ifc_container && !node->is_anonymous_block) {
        for (auto& child : render_obj->GetChildren()) {
            LayoutInfo& child_info = child->GetLayoutInfo();
            child_info.is_laid_out = true;
        }
        return;
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

const Style& NativeLayoutEngine::GetContainerStyle(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static Style default_style;
        return default_style;
    }
    return it->second.style;
}

const Style& NativeLayoutEngine::GetChildStyle(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static Style default_style;
        return default_style;
    }
    return it->second.style;
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
// Style Getters
//------------------------------------------------------------------------------

const Style& NativeLayoutEngine::GetStyle(NodeId node) const {
    auto it = nodes_.find(node);
    if (it == nodes_.end()) {
        static Style default_style;
        return default_style;
    }
    return it->second.style;
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

} // namespace mbink

