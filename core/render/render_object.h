/**
 * @file render_object.h
 * @brief 渲染对象 - DOM 到渲染树的桥梁
 *
 * 功能：
 * - 表示渲染树中的节点
 * - 存储计算后的样式
 * - 存储布局信息（统一布局树和渲染树）
 * - 管理渲染子树
 */

#pragma once

#include "css_value.h"
#include "css_variables.h"
#include "css_filters.h"
#include "css_clip_path.h"
#include "transition.h"
#include "transform.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "include/core/SkRect.h"

// 布局类型（从 layout 模块引入）
#include "../layout/types/style.h"
#include "../layout/types/layout.h"
#include "../layout/types/cache.h"
#include "../layout/types/traits.h"

// 布局样式类型（Block、Flex、Grid）
#include "../layout/block_layout.h"
#include "../layout/flex_layout.h"
#include "../layout/grid/grid.h"

// 前向声明 Skia 类
class SkCanvas;

namespace lightui {

// 前向声明
class Node;
class Element;
class Text;
class AnimationTimeline;

/**
 * @brief 渲染对象类型
 */
enum class RenderObjectType {
    BLOCK,      // 块级元素（div, p, h1等）
    INLINE,     // 内联元素（span, a等）
    TEXT,       // 文本节点
    INLINE_BLOCK, // 内联块（img, button等）
    FLEX,       // Flex 容器
    GRID,       // Grid 容器
    TABLE,      // 表格（display: table）
    TABLE_ROW_GROUP,  // 表格行组（display: table-row-group, 如 tbody）
    TABLE_HEADER_GROUP, // 表格头组（display: table-header-group, 如 thead）
    TABLE_FOOTER_GROUP, // 表格尾组（display: table-footer-group, 如 tfoot）
    TABLE_ROW,  // 表格行（display: table-row, 如 tr）
    TABLE_CELL, // 表格单元格（display: table-cell, 如 td, th）
    TABLE_CAPTION, // 表格标题（display: table-caption, 如 caption）
    NONE        // 不渲染（display: none）
};

/**
 * @brief 计算后的样式
 */
struct ComputedStyle {
    // 显示和定位
    RenderObjectType display = RenderObjectType::BLOCK;

    // 尺寸
    CSSLength width;
    CSSLength height;
    CSSLength min_width;
    CSSLength max_width;
    CSSLength min_height;
    CSSLength max_height;

    // 盒模型
    std::string box_sizing = "content-box";  // content-box, border-box
    CSSEdges margin;
    CSSEdges padding;
    CSSBorder border;
    CSSBorderRadius border_radius;
    
    // 背景
    std::string background_color;
    std::string background_image;
    CSSBackgroundRepeat background_repeat = CSSBackgroundRepeat::REPEAT;
    CSSBackgroundSize background_size;
    
    // 文本
    std::string color;
    std::string font_family;
    float font_size = 16.0f;
    std::string font_weight;  // normal, bold, 100-900
    std::string font_style;   // normal, italic
    std::string text_align;   // left, center, right, justify
    std::string text_decoration; // none, underline, line-through
    float line_height = 1.2f;

    // 文本间距和缩进
    CSSLength text_indent;           // 首行缩进
    CSSLength letter_spacing;        // 字符间距
    CSSLength word_spacing;          // 单词间距
    
    // 阴影
    std::vector<CSSBoxShadow> box_shadow;
    std::vector<CSSTextShadow> text_shadow;

    // Outline（焦点指示器，不占用布局空间，不受 border 内联样式影响）
    CSSLength outline_width;
    std::string outline_style = "none";  // none, solid, dotted, dashed
    SkColor outline_color = SK_ColorBLACK;
    CSSLength outline_offset;  // outline 与边框的距离

    // 渐变
    std::optional<CSSLinearGradient> background_linear_gradient;
    std::optional<CSSRadialGradient> background_radial_gradient;

    // 透明度
    float opacity = 1.0f;

    // 过渡动画
    std::vector<CSSTransition> transitions;

    // CSS 变量
    CSSVariables css_variables;

    // CSS 滤镜
    std::optional<CSSFilterList> filter;
    std::optional<CSSFilterList> backdrop_filter;

    // 其他
    std::string overflow;    // visible, hidden, scroll, auto (简写属性)
    std::string overflow_x;  // visible, hidden, scroll, auto
    std::string overflow_y;  // visible, hidden, scroll, auto
    std::string position;  // static, relative, absolute, fixed

    // 双向文本属性
    std::string unicode_bidi = "normal";  // normal, embed, isolate, bidi-override, isolate-override, plaintext
    std::string direction = "ltr";        // ltr, rtl

    // Flexbox 属性
    std::string flex_direction = "row";  // row, row-reverse, column, column-reverse
    std::string flex_wrap = "nowrap";    // nowrap, wrap, wrap-reverse
    std::string justify_content = "flex-start";  // flex-start, flex-end, center, space-between, space-around, space-evenly
    std::string align_items = "normal";  // normal, flex-start, flex-end, center, baseline, stretch (CSS 规范默认值是 normal)
    std::string align_content = "normal";  // normal, flex-start, flex-end, center, space-between, space-around, stretch (CSS 规范默认值是 normal)
    std::string align_self = "auto";  // auto, flex-start, flex-end, center, baseline, stretch
    float flex_grow = 0.0f;
    float flex_shrink = 1.0f;
    CSSLength flex_basis;  // auto, length, percentage

    // Grid 属性 (基础支持)
    std::string grid_template_columns;  // e.g., "1fr 1fr", "100px auto"
    std::string grid_template_rows;     // e.g., "auto 1fr"
    std::string grid_auto_columns;      // e.g., "100px", "1fr", "minmax(100px, auto)"
    std::string grid_auto_rows;         // e.g., "60px", "auto", "minmax(50px, 1fr)"
    std::string grid_auto_flow = "row"; // row, column, row dense, column dense
    CSSLength grid_column_gap;
    CSSLength grid_row_gap;
    std::string grid_column;  // e.g., "1 / 3", "span 2"
    std::string grid_row;     // e.g., "1 / 2"
    std::string justify_items = "stretch";  // stretch, start, end, center (Grid 默认)
    std::string justify_self = "auto";      // auto, stretch, start, end, center

    // Gap (用于 Flexbox 和 Grid)
    CSSLength gap;  // 简写属性
    CSSLength column_gap;
    CSSLength row_gap;

    // 定位属性
    CSSLength top;
    CSSLength right;
    CSSLength bottom;
    CSSLength left;
    int z_index = 0;

    // Flexbox order
    int order = 0;

    // 其他布局属性
    std::string visibility = "visible";  // visible, hidden, collapse

    // 盒模型单独字段（用于兼容性）
    CSSLength margin_top;
    CSSLength margin_right;
    CSSLength margin_bottom;
    CSSLength margin_left;
    CSSLength padding_top;
    CSSLength padding_right;
    CSSLength padding_bottom;
    CSSLength padding_left;
    float border_top_width = 0.0f;
    float border_right_width = 0.0f;
    float border_bottom_width = 0.0f;
    float border_left_width = 0.0f;
    CSSBorderStyle border_top_style = CSSBorderStyle::NONE;
    CSSBorderStyle border_right_style = CSSBorderStyle::NONE;
    CSSBorderStyle border_bottom_style = CSSBorderStyle::NONE;
    CSSBorderStyle border_left_style = CSSBorderStyle::NONE;
    SkColor border_top_color = SK_ColorBLACK;
    SkColor border_right_color = SK_ColorBLACK;
    SkColor border_bottom_color = SK_ColorBLACK;
    SkColor border_left_color = SK_ColorBLACK;

    // 文本布局属性
    std::string white_space = "normal";  // normal, nowrap, pre, pre-wrap, pre-line
    std::string word_wrap = "normal";    // normal, break-word
    std::string text_overflow = "clip";  // clip, ellipsis
    std::string vertical_align = "baseline";  // baseline, top, middle, bottom
    std::string cursor = "default";  // default, pointer, text, etc.

    // CSS Basic Interaction Properties (Phase 1)
    std::string text_transform = "none";     // none, uppercase, lowercase, capitalize
    std::string pointer_events = "auto";     // auto, none
    std::string user_select = "auto";        // auto, none, text, all
    std::string word_break = "normal";       // normal, break-all, keep-all, break-word

    // 表格相关属性
    std::string border_collapse = "separate";  // collapse, separate (CSS 默认值是 separate)
    CSSLength border_spacing;  // 当 border-collapse: separate 时，单元格之间的间距

    // 伪元素 ::before 和 ::after 的 content 属性
    std::string content_before;  // ::before 伪元素的内容
    std::string content_after;   // ::after 伪元素的内容
    bool has_before = false;     // 是否有 ::before 伪元素
    bool has_after = false;      // 是否有 ::after 伪元素

    // CSS Transform
    std::string transform_str;  // 原始 transform 字符串
    std::optional<CSSTransform> transform;  // 解析后的 transform
    TransformOrigin transform_origin;  // transform-origin

    // CSS Media Properties (Phase 2) - object-fit and object-position
    std::string object_fit = "fill";           // fill, contain, cover, none, scale-down
    std::string object_position = "50% 50%";   // position value (default: centered)

    // CSS Layout Properties (Phase 2) - aspect-ratio
    struct AspectRatio {
        bool is_auto = true;
        float ratio = 0.0f;  // width / height, 0 means no ratio
        
        bool HasRatio() const { return ratio > 0.0f; }
    };
    AspectRatio aspect_ratio;

    // CSS List Style Properties (Phase 2)
    std::string list_style_type = "disc";        // disc, circle, square, decimal, etc.
    std::string list_style_position = "outside"; // inside, outside
    std::string list_style_image;                // URL or empty

    // CSS clip-path Property (Phase 3)
    std::optional<CSSClipPath> clip_path;        // 裁剪路径

    ComputedStyle() {
        width = CSSLength(0, CSSUnit::AUTO);
        height = CSSLength(0, CSSUnit::AUTO);
        min_width = CSSLength(0, CSSUnit::AUTO);   // CSS初始值是auto，不是0px
        max_width = CSSLength(0, CSSUnit::NONE);
        min_height = CSSLength(0, CSSUnit::AUTO);  // CSS初始值是auto，不是0px
        max_height = CSSLength(0, CSSUnit::NONE);
        flex_basis = CSSLength(0, CSSUnit::AUTO);
        gap = CSSLength(0, CSSUnit::PX);
        column_gap = CSSLength(0, CSSUnit::PX);
        row_gap = CSSLength(0, CSSUnit::PX);
        grid_column_gap = CSSLength(0, CSSUnit::PX);
        grid_row_gap = CSSLength(0, CSSUnit::PX);
        top = CSSLength(0, CSSUnit::AUTO);
        right = CSSLength(0, CSSUnit::AUTO);
        bottom = CSSLength(0, CSSUnit::AUTO);
        left = CSSLength(0, CSSUnit::AUTO);
        margin_top = CSSLength(0, CSSUnit::PX);
        margin_right = CSSLength(0, CSSUnit::PX);
        margin_bottom = CSSLength(0, CSSUnit::PX);
        margin_left = CSSLength(0, CSSUnit::PX);
        padding_top = CSSLength(0, CSSUnit::PX);
        padding_right = CSSLength(0, CSSUnit::PX);
        padding_bottom = CSSLength(0, CSSUnit::PX);
        padding_left = CSSLength(0, CSSUnit::PX);
    }
};

/**
 * @brief 布局信息
 */
struct LayoutInfo {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    
    // 内容区域（不包括 padding 和 border）
    SkRect content_rect;
    
    // Padding 区域（包括 padding）
    SkRect padding_rect;
    
    // Border 区域（包括 border）
    SkRect border_rect;
    
    // Margin 区域（包括 margin）
    SkRect margin_rect;
    
    bool is_laid_out = false;
};

/**
 * @brief 绘制缓存 - P1优化：样式预计算缓存
 * 
 * 缓存从 ComputedStyle 计算得到的绘制相关值，避免每帧重复计算。
 * 当样式变化时调用 InvalidatePaintCache() 使缓存失效。
 */
struct PaintCache {
    bool valid = false;
    
    // 预计算的盒模型值（像素）
    float padding_left = 0.0f;
    float padding_right = 0.0f;
    float padding_top = 0.0f;
    float padding_bottom = 0.0f;
    
    float border_left_width = 0.0f;
    float border_right_width = 0.0f;
    float border_top_width = 0.0f;
    float border_bottom_width = 0.0f;
    
    // 预解析的颜色
    SkColor background_color = SK_ColorTRANSPARENT;
    SkColor border_top_color = SK_ColorBLACK;
    SkColor border_right_color = SK_ColorBLACK;
    SkColor border_bottom_color = SK_ColorBLACK;
    SkColor border_left_color = SK_ColorBLACK;
    
    // 预计算的圆角（像素）
    float border_radius_tl = 0.0f;
    float border_radius_tr = 0.0f;
    float border_radius_bl = 0.0f;
    float border_radius_br = 0.0f;
    
    // 内容区域偏移（border + padding）
    float content_x = 0.0f;
    float content_y = 0.0f;
    
    // 是否有边框
    bool has_border = false;
    // 是否有圆角
    bool has_border_radius = false;
    // 是否有阴影
    bool has_box_shadow = false;
    // 是否有渐变背景
    bool has_gradient = false;
};

/**
 * @brief 渲染对象基类
 */
class RenderObject : public std::enable_shared_from_this<RenderObject> {
public:
    /**
     * @brief 构造函数
     * @param type 渲染对象类型
     */
    explicit RenderObject(RenderObjectType type);
    
    /**
     * @brief 虚析构函数
     */
    virtual ~RenderObject() = default;
    
    /**
     * @brief 获取渲染对象类型
     */
    RenderObjectType GetType() const { return type_; }

    /**
     * @brief 检查是否是SVG渲染对象
     * @return 默认返回false，SVG渲染对象重写返回true
     */
    virtual bool IsSVGRenderObject() const { return false; }

    /**
     * @brief 获取关联的 DOM 节点
     */
    std::shared_ptr<Node> GetNode() const { return node_.lock(); }
    
    /**
     * @brief 设置关联的 DOM 节点
     */
    void SetNode(std::shared_ptr<Node> node) { node_ = node; }
    
    /**
     * @brief 获取父渲染对象
     */
    std::shared_ptr<RenderObject> GetParent() const { return parent_.lock(); }
    
    /**
     * @brief 设置父渲染对象
     */
    void SetParent(std::shared_ptr<RenderObject> parent) { parent_ = parent; }
    
    /**
     * @brief 获取子渲染对象列表
     */
    const std::vector<std::shared_ptr<RenderObject>>& GetChildren() const { return children_; }

    /**
     * @brief 获取子渲染对象列表（可修改版本，用于增量更新）
     */
    std::vector<std::shared_ptr<RenderObject>>& GetChildrenMutable() { return children_; }

    /**
     * @brief 添加子渲染对象
     */
    void AppendChild(std::shared_ptr<RenderObject> child);
    
    /**
     * @brief 移除子渲染对象
     */
    void RemoveChild(std::shared_ptr<RenderObject> child);
    
    /**
     * @brief 移除所有子渲染对象
     */
    void RemoveAllChildren();
    
    /**
     * @brief 获取计算后的样式
     */
    ComputedStyle& GetComputedStyle() { return computed_style_; }
    const ComputedStyle& GetComputedStyle() const { return computed_style_; }
    
    /**
     * @brief 设置计算后的样式
     * 自动使绘制缓存失效
     */
    void SetComputedStyle(const ComputedStyle& style) { 
        computed_style_ = style; 
        paint_cache_.valid = false;  // P1优化：样式变化时使缓存失效
    }
    
    /**
     * @brief 获取布局信息
     */
    LayoutInfo& GetLayoutInfo() { return layout_info_; }
    const LayoutInfo& GetLayoutInfo() const { return layout_info_; }
    
    /**
     * @brief 标记需要重新布局
     * @param propagate_to_parent 是否向上传播到父节点（默认true）
     */
    void MarkNeedsLayout(bool propagate_to_parent = true) {
        needs_layout_ = true;
        // 清除内容尺寸缓存，因为布局改变后需要重新计算
        content_width_ = 0.0f;
        content_height_ = 0.0f;
        // 向上传播到父节点，因为父节点的大小可能依赖于子节点
        if (propagate_to_parent) {
            auto parent = parent_.lock();
            if (parent) {
                parent->MarkNeedsLayout(true);
            }
        }
    }
    
    /**
     * @brief 检查是否需要重新布局
     */
    bool NeedsLayout() const { return needs_layout_; }
    
    /**
     * @brief 清除布局标记
     */
    void ClearNeedsLayout() { needs_layout_ = false; }
    
    /**
     * @brief 标记需要重新绘制
     */
    void MarkNeedsPaint() { needs_paint_ = true; }
    
    /**
     * @brief 检查是否需要重新绘制
     */
    bool NeedsPaint() const { return needs_paint_; }
    
    /**
     * @brief 清除绘制标记
     */
    void ClearNeedsPaint() { needs_paint_ = false; }

    /**
     * @brief 清除所有脏标记（布局和绘制）
     */
    void ClearDirtyFlags() {
        needs_layout_ = false;
        needs_paint_ = false;
    }

    // =========================================================================
    // P1优化：样式预计算缓存
    // =========================================================================

    /**
     * @brief 获取绘制缓存
     */
    PaintCache& GetPaintCache() { return paint_cache_; }
    const PaintCache& GetPaintCache() const { return paint_cache_; }

    /**
     * @brief 更新绘制缓存（如果无效则重新计算）
     * 在 Paint() 开头调用，确保缓存值是最新的
     */
    void UpdatePaintCache();

    /**
     * @brief 使绘制缓存失效
     * 在样式变化时调用
     */
    void InvalidatePaintCache() { paint_cache_.valid = false; }

    // =========================================================================
    // 布局树统一：布局相关公共方法
    // =========================================================================

    /**
     * @brief 获取布局样式（用于布局计算）
     */
    Style& GetLayoutStyle() { return layout_style_; }
    const Style& GetLayoutStyle() const { return layout_style_; }

    /**
     * @brief 获取 Block 容器样式
     */
    BlockContainerStyle& GetBlockContainerStyle() { return block_container_style_; }
    const BlockContainerStyle& GetBlockContainerStyle() const { return block_container_style_; }

    /**
     * @brief 获取 Block 项目样式
     */
    BlockItemStyle& GetBlockItemStyle() { return block_item_style_; }
    const BlockItemStyle& GetBlockItemStyle() const { return block_item_style_; }

    /**
     * @brief 获取 Flexbox 容器样式
     */
    FlexboxContainerStyle& GetFlexContainerStyle() { return flex_container_style_; }
    const FlexboxContainerStyle& GetFlexContainerStyle() const { return flex_container_style_; }

    /**
     * @brief 获取 Flexbox 项目样式
     */
    FlexboxItemStyle& GetFlexItemStyle() { return flex_item_style_; }
    const FlexboxItemStyle& GetFlexItemStyle() const { return flex_item_style_; }

    /**
     * @brief 获取 Grid 容器样式
     */
    GridContainerStyle& GetGridContainerStyle() { return grid_container_style_; }
    const GridContainerStyle& GetGridContainerStyle() const { return grid_container_style_; }

    /**
     * @brief 获取 Grid 项目样式
     */
    GridItemStyle& GetGridItemStyle() { return grid_item_style_; }
    const GridItemStyle& GetGridItemStyle() const { return grid_item_style_; }

    /**
     * @brief 获取布局缓存
     */
    Cache& GetLayoutCache() { return layout_cache_; }
    const Cache& GetLayoutCache() const { return layout_cache_; }

    /**
     * @brief 清除布局缓存
     */
    void ClearLayoutCache() { layout_cache_.Clear(); }

    /**
     * @brief 获取布局输出
     */
    LayoutOutput& GetLayoutOutput() { return layout_output_; }
    const LayoutOutput& GetLayoutOutput() const { return layout_output_; }

    /**
     * @brief 获取未舍入布局
     */
    struct Layout& GetUnroundedLayout() { return unrounded_layout_; }
    const struct Layout& GetUnroundedLayout() const { return unrounded_layout_; }

    /**
     * @brief 检查是否为 IFC 容器
     */
    bool IsIFCContainer() const { return is_ifc_container_; }

    /**
     * @brief 设置 IFC 容器标记
     */
    void SetIsIFCContainer(bool value) { is_ifc_container_ = value; }

    /**
     * @brief 从 ComputedStyle 更新布局样式
     * 当 ComputedStyle 改变时调用此方法同步布局样式
     */
    void UpdateLayoutStyle();

    /**
     * @brief 检查布局样式是否需要更新
     */
    bool IsLayoutStyleDirty() const { return layout_style_dirty_; }

    /**
     * @brief 标记布局样式需要更新
     */
    void MarkLayoutStyleDirty() { layout_style_dirty_ = true; }

    /**
     * @brief 获取边界框（用于脏区域计算）
     * @return 屏幕空间的边界矩形
     */
    SkRect GetBoundingRect() const;

    /**
     * @brief 获取视口坐标系的边界框（用于元素选择器高亮）
     * @return 视口空间的边界矩形（考虑滚动偏移）
     */
    SkRect GetViewportBoundingRect() const;

    /**
     * @brief 执行布局
     * @param parent_width 父元素宽度
     * @param parent_height 父元素高度
     */
    virtual void Layout(float parent_width, float parent_height);

    /**
     * @brief 执行绘制
     * @param canvas Skia 画布
     */
    virtual void Paint(SkCanvas* canvas);

    /**
     * @brief 绘制 outline（焦点指示器）
     * 
     * Outline 不占用布局空间，紧贴边框外边缘绘制（符合浏览器行为）。
     * 支持 solid、dashed、dotted 样式，以及圆角。
     * 
     * @param canvas Skia 画布
     * @note 应在边框绘制之后调用
     */
    void PaintOutline(SkCanvas* canvas);

    // ========== 滚动相关方法 ==========

    /**
     * @brief 获取滚动偏移量
     */
    float GetScrollX() const { return scroll_x_; }
    float GetScrollY() const { return scroll_y_; }

    /**
     * @brief 设置滚动偏移量
     */
    void SetScrollX(float x) { scroll_x_ = x; MarkNeedsPaint(); }
    void SetScrollY(float y) { scroll_y_ = y; MarkNeedsPaint(); }

    /**
     * @brief 滚动指定距离
     */
    void ScrollBy(float dx, float dy);

    /**
     * @brief 滚动到指定位置
     */
    void ScrollTo(float x, float y);

    /**
     * @brief 获取内容尺寸（用于滚动计算）
     */
    float GetContentWidth() const { return content_width_; }
    float GetContentHeight() const { return content_height_; }

    /**
     * @brief 设置内容尺寸
     */
    void SetContentSize(float width, float height) {
        content_width_ = width;
        content_height_ = height;
    }

    /**
     * @brief 检查是否可滚动
     */
    bool IsScrollable() const;

    /**
     * @brief 获取最大滚动范围
     */
    float GetMaxScrollX() const;
    float GetMaxScrollY() const;

    /**
     * @brief 滚动条区域类型
     */
    enum class ScrollbarHitArea {
        None,           // 不在滚动条区域
        HorizontalTrack,// 水平滚动条轨道
        HorizontalThumb,// 水平滚动条滑块
        VerticalTrack,  // 垂直滚动条轨道
        VerticalThumb   // 垂直滚动条滑块
    };

    /**
     * @brief 检测点是否在滚动条区域内
     * @param local_x 相对于元素的 X 坐标
     * @param local_y 相对于元素的 Y 坐标
     * @return 滚动条区域类型
     */
    ScrollbarHitArea HitTestScrollbar(float local_x, float local_y) const;

    /**
     * @brief 获取滚动条宽度
     */
    static constexpr float GetScrollbarWidth() { return 12.0f; }

    /**
     * @brief 开始拖动滚动条
     */
    void StartScrollbarDrag(ScrollbarHitArea area, float mouse_x, float mouse_y);

    /**
     * @brief 更新滚动条拖动
     */
    void UpdateScrollbarDrag(float mouse_x, float mouse_y);

    /**
     * @brief 结束滚动条拖动
     */
    void EndScrollbarDrag();

    /**
     * @brief 检查是否正在拖动滚动条
     */
    bool IsDraggingScrollbar() const { return dragging_scrollbar_ != ScrollbarHitArea::None; }

    /**
     * @brief 获取正在拖动的滚动条类型
     */
    ScrollbarHitArea GetDraggingScrollbar() const { return dragging_scrollbar_; }

    /**
     * @brief 递归计算子元素的实际内容高度
     * @return 所有子元素的实际内容高度（包括嵌套子元素）
     */
    float CalculateContentHeight() const;

    /**
     * @brief 递归计算子元素的实际内容宽度
     * @return 所有子元素的实际内容宽度（包括嵌套子元素）
     */
    float CalculateContentWidth() const;

    /**
     * @brief 设置视口尺寸（用于 body 元素滚动条计算）
     * @param width 视口宽度
     * @param height 视口高度
     */
    static void SetViewportSize(float width, float height);

    /**
     * @brief 获取视口宽度
     */
    static float GetViewportWidth() { return viewport_width_; }

    /**
     * @brief 获取视口高度
     */
    static float GetViewportHeight() { return viewport_height_; }

    /**
     * @brief 重置绘制统计（每帧开始时调用）
     */
    static void ResetPaintStats();

    /**
     * @brief 打印绘制统计（验证视口剔除效果）
     */
    static void PrintPaintStats();

    /**
     * @brief 打印绘制计时统计（分析性能瓶颈）
     */
    static void PrintPaintTimingStats();

    /**
     * @brief 重置绘制计时统计
     */
    static void ResetPaintTimingStats();

    /**
     * @brief 检查当前元素是否是 body 元素
     */
    bool IsBodyElement() const;

    /**
     * @brief 获取有效的可见宽度（对于 body 元素返回视口宽度）
     */
    float GetEffectiveVisibleWidth() const;

    /**
     * @brief 获取有效的可见高度（对于 body 元素返回视口高度）
     */
    float GetEffectiveVisibleHeight() const;

protected:
    // 静态成员：视口尺寸
    static float viewport_width_;
    static float viewport_height_;

    RenderObjectType type_;
    std::weak_ptr<Node> node_;
    std::weak_ptr<RenderObject> parent_;
    std::vector<std::shared_ptr<RenderObject>> children_;

    ComputedStyle computed_style_;
    LayoutInfo layout_info_;
    PaintCache paint_cache_;  // P1优化：样式预计算缓存

    bool needs_layout_ = true;
    bool needs_paint_ = true;

    // 滚动状态
    float scroll_x_ = 0.0f;
    float scroll_y_ = 0.0f;
    float content_width_ = 0.0f;
    float content_height_ = 0.0f;

    // 滚动条拖动状态
    ScrollbarHitArea dragging_scrollbar_ = ScrollbarHitArea::None;
    float drag_start_scroll_ = 0.0f;      // 拖动开始时的滚动位置
    float drag_start_mouse_ = 0.0f;       // 拖动开始时的鼠标位置

    // =========================================================================
    // 布局树统一：以下字段从 NativeLayoutEngine::LayoutNode 移入
    // =========================================================================

    /// 布局样式（从 ComputedStyle 转换而来，用于布局计算）
    Style layout_style_;

    /// Block 布局样式
    BlockContainerStyle block_container_style_;
    BlockItemStyle block_item_style_;

    /// Flexbox 布局样式
    FlexboxContainerStyle flex_container_style_;
    FlexboxItemStyle flex_item_style_;

    /// Grid 布局样式
    GridContainerStyle grid_container_style_;
    GridItemStyle grid_item_style_;

    /// 布局缓存（避免重复计算）
    Cache layout_cache_;

    /// 布局输出结果
    LayoutOutput layout_output_;

    /// 最终布局（未舍入）
    struct Layout unrounded_layout_;

    /// 是否为 IFC（Inline Formatting Context）容器
    bool is_ifc_container_ = false;

    /// 布局样式是否已更新
    bool layout_style_dirty_ = true;
};

// 前向声明
class HTMLInputElement;
class HTMLTextAreaElement;
struct Box;

/**
 * @brief 块级渲染对象
 */
class RenderBlock : public RenderObject {
public:
    RenderBlock() : RenderObject(RenderObjectType::BLOCK) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

private:
    /**
     * @brief 渲染input元素的特定内容
     * @param canvas Skia画布
     * @param input HTMLInputElement指针
     * @param box 盒模型
     */
    void PaintInputElement(SkCanvas* canvas, HTMLInputElement* input, const Box& box);

    /**
     * @brief 渲染textarea元素的特定内容
     * @param canvas Skia画布
     * @param textarea HTMLTextAreaElement指针
     * @param box 盒模型
     */
    void PaintTextAreaElement(SkCanvas* canvas, HTMLTextAreaElement* textarea, const Box& box);
};

/**
 * @brief 内联渲染对象
 */
class RenderInline : public RenderObject {
public:
    RenderInline() : RenderObject(RenderObjectType::INLINE) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

    /**
     * @brief 计算元素的固有尺寸（用于 Taffy measure function）
     * @param available_width 可用宽度
     * @return 包含宽度和高度的尺寸
     */
    std::pair<float, float> MeasureIntrinsicSize(float available_width);

    /**
     * @brief 只定位子元素，不重新计算尺寸
     * 用于 flex 布局后定位内部文本
     */
    void PositionChildrenOnly();

private:
    // 表单控件渲染辅助方法
    void PaintInputElement(SkCanvas* canvas, HTMLInputElement* input, const Box& box);
    void PaintTextAreaElement(SkCanvas* canvas, HTMLTextAreaElement* textarea, const Box& box);
};

/**
 * @brief 文本渲染对象
 */
class RenderText : public RenderObject {
public:
    RenderText() : RenderObject(RenderObjectType::TEXT) {}
    RenderText(const std::string& text) : RenderObject(RenderObjectType::TEXT), text_(text) {}

    void SetText(const std::string& text) { text_ = text; wrapped_lines_.clear(); }
    std::string GetText() const { return text_; }

    // Get wrapped lines (populated after layout with width constraint)
    const std::vector<std::string>& GetWrappedLines() const { return wrapped_lines_; }

    // Set wrapped lines (called by measure function)
    void SetWrappedLines(const std::vector<std::string>& lines) { wrapped_lines_ = lines; }

    // Get/Set actual measured text width (for text-align calculation)
    float GetActualTextWidth() const { return actual_text_width_; }
    void SetActualTextWidth(float width) { actual_text_width_ = width; }

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

private:
    std::string text_;
    std::vector<std::string> wrapped_lines_;  // Cached wrapped lines for rendering
    float actual_text_width_ = 0.0f;  // Actual measured text width (for text-align)
};

/**
 * @brief 表格渲染对象（display: table）
 *
 * CSS表格布局模型：
 * - 表格由行组（thead, tbody, tfoot）和行（tr）组成
 * - 每行由单元格（td, th）组成
 * - 表格自动计算列宽度
 */
class RenderTable : public RenderObject {
public:
    RenderTable() : RenderObject(RenderObjectType::TABLE) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

    // 获取计算后的列宽度
    const std::vector<float>& GetColumnWidths() const { return column_widths_; }

    // 获取列数
    size_t GetColumnCount() const { return column_widths_.size(); }

    // 获取列的背景色（从 colgroup/col 继承）
    std::string GetColumnBackgroundColor(size_t col_index) const {
        if (col_index < column_background_colors_.size()) {
            return column_background_colors_[col_index];
        }
        return "";
    }

private:
    // 计算表格列宽度
    void CalculateColumnWidths(float available_width);

    // 收集 colgroup/col 的样式
    void CollectColumnStyles();

    // 缓存的列宽度
    std::vector<float> column_widths_;

    // 列的背景色（从 colgroup/col 收集）
    std::vector<std::string> column_background_colors_;
};

/**
 * @brief 表格行组渲染对象（display: table-row-group/table-header-group/table-footer-group）
 *
 * 用于 thead, tbody, tfoot 元素
 */
class RenderTableRowGroup : public RenderObject {
public:
    explicit RenderTableRowGroup(RenderObjectType type = RenderObjectType::TABLE_ROW_GROUP)
        : RenderObject(type) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;
};

/**
 * @brief 表格行渲染对象（display: table-row）
 *
 * 用于 tr 元素
 */
class RenderTableRow : public RenderObject {
public:
    RenderTableRow() : RenderObject(RenderObjectType::TABLE_ROW) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

    // 设置和获取列宽度（由父表格设置）
    void SetColumnWidths(const std::vector<float>& widths) { column_widths_ = widths; }
    const std::vector<float>& GetColumnWidths() const { return column_widths_; }

    // 设置 border-spacing 和 border-collapse（由父表格设置）
    void SetBorderSpacing(float spacing) { border_spacing_ = spacing; }
    float GetBorderSpacing() const { return border_spacing_; }
    void SetBorderCollapse(bool collapse) { border_collapse_ = collapse; }
    bool GetBorderCollapse() const { return border_collapse_; }

    // Rowspan 占据的列信息（由父表格设置）
    // 记录被上方单元格的 rowspan 占据的列索引
    struct RowspanCell {
        size_t col_index;          // 列索引
        int remaining_rows;        // 还需要跨越的行数
        std::shared_ptr<RenderObject> cell;  // 原始单元格（用于高度对齐）
    };
    void SetRowspanOccupiedCols(const std::vector<RowspanCell>& cols) { rowspan_occupied_cols_ = cols; }
    const std::vector<RowspanCell>& GetRowspanOccupiedCols() const { return rowspan_occupied_cols_; }

private:
    std::vector<float> column_widths_;
    float border_spacing_ = 2.0f;  // CSS 默认值
    bool border_collapse_ = false;
    std::vector<RowspanCell> rowspan_occupied_cols_;  // 被 rowspan 占据的列
};

/**
 * @brief 表格单元格渲染对象（display: table-cell）
 *
 * 用于 td, th 元素
 */
class RenderTableCell : public RenderObject {
public:
    RenderTableCell() : RenderObject(RenderObjectType::TABLE_CELL) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

    // colspan 和 rowspan 支持
    void SetColSpan(int span) { col_span_ = span; }
    int GetColSpan() const { return col_span_; }

    void SetRowSpan(int span) { row_span_ = span; }
    int GetRowSpan() const { return row_span_; }

    // 列索引（用于应用 col 的背景色）
    void SetColumnIndex(size_t index) { column_index_ = index; }
    size_t GetColumnIndex() const { return column_index_; }

private:
    int col_span_ = 1;
    int row_span_ = 1;
    size_t column_index_ = 0;  // 单元格所在的逻辑列索引
};

/**
 * @brief 表格标题渲染对象（display: table-caption）
 *
 * 用于 caption 元素
 */
class RenderTableCaption : public RenderObject {
public:
    RenderTableCaption() : RenderObject(RenderObjectType::TABLE_CAPTION) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;
};

} // namespace lightui

