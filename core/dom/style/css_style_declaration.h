/**
 * @file css_style_declaration.h
 * @brief CSSStyleDeclaration类 - 内联样式管理
 * 
 * 功能：
 * - 实现W3C CSSStyleDeclaration接口
 * - 提供style API（setProperty, getPropertyValue, removeProperty）
 * - 支持cssText读写
 * - 符合CSSOM规范
 * 
 * 参考：
 * - W3C CSSOM - CSSStyleDeclaration
 * - MDN Web Docs - HTMLElement.style
 * - RmlUi/Source/Core/ElementStyle.h
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace mblink {

// 前向声明
class Element;

/**
 * @brief CSSStyleDeclaration类
 * 
 * 表示元素的内联样式（style属性）
 * 
 * 符合W3C CSSStyleDeclaration接口：
 * - setProperty(property, value, priority?) - 设置样式属性
 * - getPropertyValue(property) - 获取样式属性值
 * - removeProperty(property) - 移除样式属性
 * - getPropertyPriority(property) - 获取优先级（!important）
 * - cssText - 完整的样式文本
 * - length - 样式属性数量
 * - item(index) - 获取指定索引的属性名
 * 
 * 参考：
 * - https://drafts.csswg.org/cssom/#cssstyledeclaration
 * - https://developer.mozilla.org/en-US/docs/Web/API/CSSStyleDeclaration
 */
class CSSStyleDeclaration {
public:
    /**
     * @brief 构造函数
     * @param element 关联的元素
     */
    CSSStyleDeclaration(std::weak_ptr<Element> element);
    
    /**
     * @brief 设置样式属性
     * @param property 属性名（如"color", "font-size"）
     * @param value 属性值（如"red", "16px"）
     * @param priority 优先级（"important"或空字符串）
     * 
     * 示例：
     * style->SetProperty("color", "red");
     * style->SetProperty("width", "100px", "important");
     */
    void SetProperty(const std::string& property, const std::string& value, const std::string& priority = "");
    
    /**
     * @brief 获取样式属性值
     * @param property 属性名
     * @return 属性值，如果不存在返回空字符串
     * 
     * 示例：
     * std::string color = style->GetPropertyValue("color");
     */
    std::string GetPropertyValue(const std::string& property) const;
    
    /**
     * @brief 移除样式属性
     * @param property 属性名
     * @return 被移除的属性值
     * 
     * 示例：
     * std::string old_color = style->RemoveProperty("color");
     */
    std::string RemoveProperty(const std::string& property);
    
    /**
     * @brief 获取属性优先级
     * @param property 属性名
     * @return "important"或空字符串
     * 
     * 示例：
     * std::string priority = style->GetPropertyPriority("width");
     * if (priority == "important") { ... }
     */
    std::string GetPropertyPriority(const std::string& property) const;
    
    /**
     * @brief 获取样式属性数量
     * @return 属性数量
     */
    size_t Length() const;
    
    /**
     * @brief 获取指定索引的属性名
     * @param index 索引（从0开始）
     * @return 属性名，如果索引越界返回空字符串
     */
    std::string Item(size_t index) const;
    
    /**
     * @brief 获取完整的样式文本
     * @return CSS样式文本（如"color: red; font-size: 16px;"）
     */
    std::string GetCssText() const;
    
    /**
     * @brief 设置完整的样式文本
     * @param css_text CSS样式文本
     * 
     * 示例：
     * style->SetCssText("color: red; font-size: 16px;");
     */
    void SetCssText(const std::string& css_text);
    
    /**
     * @brief 获取所有样式属性
     * @return 属性名到属性值的映射
     */
    const std::unordered_map<std::string, std::string>& GetAllProperties() const;

private:
    /**
     * @brief 解析CSS样式文本
     * @param css_text CSS样式文本
     * @return 属性名到属性值的映射
     * 
     * 解析格式：
     * "color: red; font-size: 16px; width: 100px !important;"
     */
    std::unordered_map<std::string, std::string> ParseCssText(const std::string& css_text) const;
    
    /**
     * @brief 序列化样式属性为CSS文本
     * @return CSS样式文本
     */
    std::string SerializeCssText() const;
    
    /**
     * @brief 规范化属性名
     * @param property 属性名
     * @return 规范化后的属性名（小写，去除空格）
     * 
     * 示例：
     * "Font-Size" -> "font-size"
     * " color " -> "color"
     */
    std::string NormalizePropertyName(const std::string& property) const;
    
    /**
     * @brief 更新元素的style属性
     * @param needs_layout 是否需要触发布局更新
     */
    void UpdateStyleAttribute(bool needs_layout = false);

private:
    std::weak_ptr<Element> element_;  // 关联的元素（弱引用避免循环引用）
    std::unordered_map<std::string, std::string> properties_;  // 样式属性映射
    std::unordered_map<std::string, std::string> priorities_;  // 属性优先级映射
    std::vector<std::string> property_order_;  // 属性顺序（用于保持插入顺序）
};

} // namespace mblink

