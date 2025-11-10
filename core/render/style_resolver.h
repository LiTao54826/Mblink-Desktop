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
#include "core/dom/element.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace lightui {

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

private:
    /**
     * @brief 应用默认样式
     */
    void ApplyDefaultStyle(ComputedStyle& style, const std::string& tag_name);
    
    /**
     * @brief 应用元素的内联样式
     */
    void ApplyInlineStyle(ComputedStyle& style, std::shared_ptr<Element> element);
    
    /**
     * @brief 应用样式继承
     */
    void ApplyInheritance(ComputedStyle& style, const ComputedStyle* parent_style);
    
    /**
     * @brief 解析单个样式属性
     */
    void ParseStyleProperty(ComputedStyle& style, 
                           const std::string& property, 
                           const std::string& value);
    
    /**
     * @brief 检查属性是否可继承
     */
    bool IsInheritableProperty(const std::string& property);
    
    /**
     * @brief 解析 display 属性
     */
    RenderObjectType ParseDisplay(const std::string& value);

private:
    // 可继承属性集合
    std::unordered_set<std::string> inheritable_properties_;
    
    // 默认样式缓存
    std::unordered_map<std::string, ComputedStyle> default_styles_;
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

private:
    /**
     * @brief 从元素节点创建渲染对象
     */
    std::shared_ptr<RenderObject> CreateRenderObjectForElement(
        std::shared_ptr<Element> element,
        const ComputedStyle* parent_style);
    
    /**
     * @brief 从文本节点创建渲染对象
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
};

} // namespace lightui

