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

/**
 * @brief 渲染对象类型
 */
enum class RenderObjectType {
    BLOCK,      // 块级元素（div, p, h1等）
    INLINE,     // 内联元素（span, a等）
    TEXT,       // 文本节点
    INLINE_BLOCK, // 内联块（img, button等）
    FLEX,       // Flex 容器
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
    
    // 透明度
    float opacity = 1.0f;
    
    // 其他
    std::string overflow;  // visible, hidden, scroll, auto
    std::string position;  // static, relative, absolute, fixed
    
    ComputedStyle() {
        width = CSSLength(0, CSSUnit::AUTO);
        height = CSSLength(0, CSSUnit::AUTO);
        min_width = CSSLength(0, CSSUnit::PX);
        max_width = CSSLength(0, CSSUnit::NONE);
        min_height = CSSLength(0, CSSUnit::PX);
        max_height = CSSLength(0, CSSUnit::NONE);
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

protected:
    RenderObjectType type_;
    std::weak_ptr<Node> node_;
    std::weak_ptr<RenderObject> parent_;
    std::vector<std::shared_ptr<RenderObject>> children_;
    
    ComputedStyle computed_style_;
    LayoutInfo layout_info_;
    
    bool needs_layout_ = true;
    bool needs_paint_ = true;
};

/**
 * @brief 块级渲染对象
 */
class RenderBlock : public RenderObject {
public:
    RenderBlock() : RenderObject(RenderObjectType::BLOCK) {}
    
    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;
};

/**
 * @brief 内联渲染对象
 */
class RenderInline : public RenderObject {
public:
    RenderInline() : RenderObject(RenderObjectType::INLINE) {}
    
    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;
};

/**
 * @brief 文本渲染对象
 */
class RenderText : public RenderObject {
public:
    RenderText() : RenderObject(RenderObjectType::TEXT) {}
    RenderText(const std::string& text) : RenderObject(RenderObjectType::TEXT), text_(text) {}

    void SetText(const std::string& text) { text_ = text; }
    std::string GetText() const { return text_; }
    
    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

private:
    std::string text_;
};

} // namespace lightui

