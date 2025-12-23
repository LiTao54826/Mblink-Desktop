/**
 * @file style_resolver.h
 * @brief 样式解析器 - 计算元素的最终样式
 * 
 * 功能：
 * - 样式计算（从 DOM 样式到计算样式）
 * - 样式继承（color, font-family 等）
 * - 样式级联（inline > id > class > tag）
 * - 默认值处理
 */

#pragma once

#include "render_object.h"
#include "filter_cache.h"
#include "core/dom/element.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace lightui {

// 前向声明
class StyleManager;
class Document;

/**
 * @brief 样式解析器
 */
class StyleResolver {
public:
    /**
     * @brief 构造函数
     */
    StyleResolver();
    
    /**
     * @brief 析构函数
     */
    ~StyleResolver() = default;
    
    /**
     * @brief 计算元素的最终样式
     * @param element DOM 元素
     * @param parent_style 父元素的计算样式（用于继承）
     * @return 计算后的样式
     */
    ComputedStyle ResolveStyle(std::shared_ptr<Element> element,
                              const ComputedStyle* parent_style = nullptr);
    
    /**
     * @brief 获取默认样式
     * @param tag_name 标签名
     * @return 默认样式
     */
    ComputedStyle GetDefaultStyle(const std::string& tag_name);

    /**
     * @brief 获取渲染优化器
     * @return 渲染优化器引用
     */
    RenderOptimizer& GetOptimizer() { return optimizer_; }

    /**
     * @brief 获取渲染优化器 (const 版本)
     * @return 渲染优化器引用
     */
    const RenderOptimizer& GetOptimizer() const { return optimizer_; }

    /**
     * @brief 启用/禁用性能优化
     * @param enabled 是否启用
     */
    void SetOptimizationEnabled(bool enabled) { optimization_enabled_ = enabled; }

    /**
     * @brief 检查性能优化是否启用
     * @return 是否启用
     */
    bool IsOptimizationEnabled() const { return optimization_enabled_; }

    /**
     * @brief 设置样式管理器
     * @param manager 样式管理器指针
     */
    void SetStyleManager(StyleManager* manager) { style_manager_ = manager; }

private:
    /**
     * @brief 应用默认样式
     * @param style 要应用样式的对象
     * @param tag_name 标签名
     * @param is_root 是否为根元素（没有父元素）
     */
    void ApplyDefaultStyle(ComputedStyle& style, const std::string& tag_name, bool is_root);

    /**
     * @brief 应用元素特定的样式（如 h1 的 font-size）
     * @param style 要应用样式的对象
     * @param tag_name 标签名
     * @param element 元素指针（可选，用于获取属性）
     */
    void ApplyElementSpecificStyle(ComputedStyle& style, const std::string& tag_name, std::shared_ptr<Element> element = nullptr);

    /**
     * @brief 应用元素的内联样式
     */
    void ApplyInlineStyle(ComputedStyle& style, std::shared_ptr<Element> element);

    /**
     * @brief 应用样式继承
     */
    void ApplyInheritance(ComputedStyle& style, const ComputedStyle* parent_style);

    /**
     * @brief 应用伪类样式（如 :hover, :active, :focus）
     * @param style 要应用样式的对象
     * @param element 元素指针
     */
    void ApplyPseudoClassStyles(ComputedStyle& style, std::shared_ptr<Element> element);

    /**
     * @brief 应用伪元素样式（::before, ::after）
     * @param style 要应用样式的对象
     * @param element 元素指针
     */
    void ApplyPseudoElementStyles(ComputedStyle& style, std::shared_ptr<Element> element);

    /**
     * @brief 应用 CSS 规则（从 StyleManager）
     * @param style 要应用样式的对象
     * @param element 元素指针
     */
    void ApplyCSSRules(ComputedStyle& style, std::shared_ptr<Element> element);

    /**
     * @brief 解析单个样式属性
     */
    void ParseStyleProperty(ComputedStyle& style,
                           const std::string& property,
                           const std::string& value);

    /**
     * @brief 解析布局相关属性 (display, width, height, margin, padding)
     * @return true if property was handled, false otherwise
     */
    bool ParseLayoutProperty(ComputedStyle& style,
                            const std::string& property,
                            const std::string& resolved_value);

    /**
     * @brief 解析边框相关属性 (border-*)
     * @return true if property was handled, false otherwise
     */
    bool ParseBorderProperty(ComputedStyle& style,
                            const std::string& property,
                            const std::string& resolved_value);

    /**
     * @brief 解析文本相关属性 (font-*, text-*, color)
     * @return true if property was handled, false otherwise
     */
    bool ParseTextProperty(ComputedStyle& style,
                          const std::string& property,
                          const std::string& resolved_value);

    /**
     * @brief 解析背景相关属性 (background-*)
     * @return true if property was handled, false otherwise
     */
    bool ParseBackgroundProperty(ComputedStyle& style,
                                const std::string& property,
                                const std::string& resolved_value);

    /**
     * @brief 解析动画相关属性 (animation-*)
     * @return true if property was handled, false otherwise
     */
    bool ParseAnimationProperty(ComputedStyle& style,
                               const std::string& property,
                               const std::string& resolved_value);

    /**
     * @brief 解析定位相关属性 (position, top, left, z-index, etc.)
     * @return true if property was handled, false otherwise
     */
    bool ParsePositionProperty(ComputedStyle& style,
                              const std::string& property,
                              const std::string& resolved_value);

    /**
     * @brief 解析Flexbox相关属性 (flex-*, align-*, justify-*)
     * @return true if property was handled, false otherwise
     */
    bool ParseFlexProperty(ComputedStyle& style,
                          const std::string& property,
                          const std::string& resolved_value);

    /**
     * @brief 解析视觉效果属性 (opacity, visibility, overflow, etc.)
     * @return true if property was handled, false otherwise
     */
    bool ParseVisualProperty(ComputedStyle& style,
                            const std::string& property,
                            const std::string& resolved_value);

    /**
     * @brief 解析交互属性 (pointer-events, user-select, cursor, etc.)
     * @return true if property was handled, false otherwise
     */
    bool ParseInteractionProperty(ComputedStyle& style,
                                 const std::string& property,
                                 const std::string& resolved_value);

    /**
     * @brief 解析媒体和布局属性 (object-fit, object-position, aspect-ratio, list-style-*)
     * @return true if property was handled, false otherwise
     */
    bool ParseMediaLayoutProperty(ComputedStyle& style,
                                 const std::string& property,
                                 const std::string& resolved_value);

    /**
     * @brief 检查属性是否可继承
     */
    bool IsInheritableProperty(const std::string& property);

    /**
     * @brief 解析 display 属性
     */
    RenderObjectType ParseDisplay(const std::string& value);

    /**
     * @brief 解析边框简写属性（如 "4px solid #4CAF50"）
     * @param value 边框简写值
     * @param font_size 用于解析相对单位的字体大小
     * @return CSSBorder 对象
     */
    CSSBorder ParseBorderShorthand(const std::string& value, float font_size);

private:
    // 可继承属性集合
    std::unordered_set<std::string> inheritable_properties_;

    // 默认样式缓存
    std::unordered_map<std::string, ComputedStyle> default_styles_;

    // 渲染优化器
    RenderOptimizer optimizer_;

    // 是否启用性能优化
    bool optimization_enabled_ = true;

    // 样式管理器指针
    StyleManager* style_manager_ = nullptr;
};

/**
 * @brief 渲染树构建器
 */
class RenderTreeBuilder {
public:
    /**
     * @brief 构造函数
     */
    RenderTreeBuilder();
    
    /**
     * @brief 析构函数
     */
    ~RenderTreeBuilder() = default;
    
    /**
     * @brief 从 DOM 树构建渲染树
     * @param node DOM 节点
     * @param parent_style 父元素的计算样式
     * @return 渲染对象
     */
    std::shared_ptr<RenderObject> BuildRenderTree(std::shared_ptr<Node> node,
                                                   const ComputedStyle* parent_style = nullptr);

    /**
     * @brief 设置 Document（用于访问 StyleManager）
     * @param doc Document 指针
     */
    void SetDocument(Document* doc) { document_ = doc; }

    /**
     * @brief 获取样式解析器
     */
    StyleResolver& GetStyleResolver() { return style_resolver_; }

    // ========== Phase 3: 增量渲染树更新接口 ==========

    /**
     * @brief 从元素节点创建渲染对象（用于增量更新）
     * @param element 元素节点
     * @return 渲染对象，如果 display: none 则返回 nullptr
     */
    std::shared_ptr<RenderObject> CreateRenderObjectForElement(Element* element);

    /**
     * @brief 从文本节点创建渲染对象（用于增量更新）
     * @param text 文本节点
     * @return 渲染对象
     */
    std::shared_ptr<RenderObject> CreateRenderObjectForText(Text* text);

private:
    /**
     * @brief 从元素节点创建渲染对象（内部使用）
     */
    std::shared_ptr<RenderObject> CreateRenderObjectForElement(
        std::shared_ptr<Element> element,
        const ComputedStyle* parent_style);

    /**
     * @brief 从文本节点创建渲染对象（内部使用）
     */
    std::shared_ptr<RenderObject> CreateRenderObjectForText(
        std::shared_ptr<Text> text,
        const ComputedStyle* parent_style);

    /**
     * @brief 根据样式创建对应类型的渲染对象
     */
    std::shared_ptr<RenderObject> CreateRenderObjectByType(RenderObjectType type);

private:
    StyleResolver style_resolver_;
    Document* document_ = nullptr;
};

} // namespace lightui

