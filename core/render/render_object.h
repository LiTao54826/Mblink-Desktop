/**
 * @file render_object.h
 * @brief 渲染对象 - DOM 到渲染树的桥梁
 * 
 * 功能：
 * - 表示渲染树中的节点
 * - 存储计算后的样式
 * - 存储布局信息
 * - 管理渲染子树
 */

#pragma once

#include "css_value.h"
#include "css_variables.h"
#include "css_filters.h"
#include "transition.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "include/core/SkRect.h"

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
    
    // 阴影
    std::vector<CSSBoxShadow> box_shadow;
    std::vector<CSSTextShadow> text_shadow;

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
    std::string overflow;  // visible, hidden, scroll, auto
    std::string position;  // static, relative, absolute, fixed

    // Flexbox 属性
    std::string flex_direction = "row";  // row, row-reverse, column, column-reverse
    std::string flex_wrap = "nowrap";    // nowrap, wrap, wrap-reverse
    std::string justify_content = "flex-start";  // flex-start, flex-end, center, space-between, space-around, space-evenly
    std::string align_items = "stretch";  // flex-start, flex-end, center, baseline, stretch
    std::string align_content = "stretch";  // flex-start, flex-end, center, space-between, space-around, stretch
    std::string align_self = "auto";  // auto, flex-start, flex-end, center, baseline, stretch
    float flex_grow = 0.0f;
    float flex_shrink = 1.0f;
    CSSLength flex_basis;  // auto, length, percentage

    // Grid 属性 (基础支持)
    std::string grid_template_columns;  // e.g., "1fr 1fr", "100px auto"
    std::string grid_template_rows;     // e.g., "auto 1fr"
    std::string grid_auto_flow = "row"; // row, column, row dense, column dense
    CSSLength grid_column_gap;
    CSSLength grid_row_gap;
    std::string grid_column;  // e.g., "1 / 3", "span 2"
    std::string grid_row;     // e.g., "1 / 2"

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

    // 文本布局属性
    std::string white_space = "normal";  // normal, nowrap, pre, pre-wrap, pre-line
    std::string word_wrap = "normal";    // normal, break-word
    std::string text_overflow = "clip";  // clip, ellipsis
    std::string vertical_align = "baseline";  // baseline, top, middle, bottom
    std::string cursor = "default";  // default, pointer, text, etc.

    ComputedStyle() {
        width = CSSLength(0, CSSUnit::AUTO);
        height = CSSLength(0, CSSUnit::AUTO);
        min_width = CSSLength(0, CSSUnit::PX);
        max_width = CSSLength(0, CSSUnit::NONE);
        min_height = CSSLength(0, CSSUnit::PX);
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
     */
    void SetComputedStyle(const ComputedStyle& style) { computed_style_ = style; }
    
    /**
     * @brief 获取布局信息
     */
    LayoutInfo& GetLayoutInfo() { return layout_info_; }
    const LayoutInfo& GetLayoutInfo() const { return layout_info_; }
    
    /**
     * @brief 标记需要重新布局
     */
    void MarkNeedsLayout() { needs_layout_ = true; }
    
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

protected:
    RenderObjectType type_;
    std::weak_ptr<Node> node_;
    std::weak_ptr<RenderObject> parent_;
    std::vector<std::shared_ptr<RenderObject>> children_;

    ComputedStyle computed_style_;
    LayoutInfo layout_info_;

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

} // namespace lightui

