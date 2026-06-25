/**
 * @file cascade_engine.h
 * @brief CSS级联和继承引擎
 * 
 * 功能：
 * - CSS选择器优先级计算（Specificity）
 * - CSS级联规则（Cascade）
 * - 属性继承（Inheritance）
 * - 计算值（Computed Values）
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace mblink {

// 前向声明
class Element;
struct CSSRule;

/**
 * @brief CSS选择器优先级
 */
struct Specificity {
    int inline_style;   ///< 内联样式 (1 or 0)
    int id_count;       ///< ID选择器数量
    int class_count;    ///< 类/属性/伪类选择器数量
    int element_count;  ///< 元素/伪元素选择器数量
    
    /**
     * @brief 默认构造函数
     */
    Specificity()
        : inline_style(0)
        , id_count(0)
        , class_count(0)
        , element_count(0) {
    }
    
    /**
     * @brief 构造函数
     */
    Specificity(int inline_val, int id, int cls, int elem)
        : inline_style(inline_val)
        , id_count(id)
        , class_count(cls)
        , element_count(elem) {
    }
    
    /**
     * @brief 比较优先级
     * @param other 另一个优先级
     * @return >0 表示this优先级更高，<0 表示other优先级更高，=0 表示相同
     */
    int Compare(const Specificity& other) const {
        if (inline_style != other.inline_style) {
            return inline_style - other.inline_style;
        }
        if (id_count != other.id_count) {
            return id_count - other.id_count;
        }
        if (class_count != other.class_count) {
            return class_count - other.class_count;
        }
        return element_count - other.element_count;
    }
    
    /**
     * @brief 比较运算符
     */
    bool operator<(const Specificity& other) const {
        return Compare(other) < 0;
    }
    
    bool operator>(const Specificity& other) const {
        return Compare(other) > 0;
    }
    
    bool operator==(const Specificity& other) const {
        return Compare(other) == 0;
    }
};

/**
 * @brief CSS属性值和优先级
 */
struct PropertyValue {
    std::string value;      ///< 属性值
    Specificity specificity; ///< 优先级
    int order;              ///< 声明顺序（用于相同优先级时的排序）
    
    PropertyValue()
        : value()
        , specificity()
        , order(0) {
    }
    
    PropertyValue(const std::string& val, const Specificity& spec, int ord)
        : value(val)
        , specificity(spec)
        , order(ord) {
    }
};

/**
 * @brief CSS级联引擎
 * 
 * 负责：
 * - 计算选择器优先级
 * - 应用CSS级联规则
 * - 处理属性继承
 * - 计算最终样式值
 */
class CascadeEngine {
public:
    /**
     * @brief 构造函数
     */
    CascadeEngine();
    
    /**
     * @brief 析构函数
     */
    ~CascadeEngine();
    
    /**
     * @brief 计算选择器的优先级
     * @param selector CSS选择器字符串
     * @return 优先级
     */
    Specificity CalculateSpecificity(const std::string& selector) const;
    
    /**
     * @brief 应用级联规则计算最终样式
     * @param element 目标元素
     * @param matching_rules 匹配的CSS规则列表
     * @return 最终样式映射
     */
    std::map<std::string, std::string> ApplyCascade(
        Element* element,
        const std::vector<const CSSRule*>& matching_rules
    ) const;
    
    /**
     * @brief 应用继承规则
     * @param element 目标元素
     * @param base_style 基础样式（级联后的样式）
     * @return 应用继承后的样式
     */
    std::map<std::string, std::string> ApplyInheritance(
        Element* element,
        const std::map<std::string, std::string>& base_style
    ) const;
    
    /**
     * @brief 计算元素的最终样式
     * @param element 目标元素
     * @param matching_rules 匹配的CSS规则列表
     * @return 最终样式映射
     */
    std::map<std::string, std::string> ComputeStyle(
        Element* element,
        const std::vector<const CSSRule*>& matching_rules
    ) const;
    
    /**
     * @brief 检查属性是否可继承
     * @param property 属性名
     * @return true表示可继承
     */
    bool IsInheritableProperty(const std::string& property) const;
    
    /**
     * @brief 获取属性的初始值
     * @param property 属性名
     * @return 初始值
     */
    std::string GetInitialValue(const std::string& property) const;
    
private:
    /**
     * @brief 解析选择器并计算优先级
     * @param selector 选择器字符串
     * @return 优先级
     */
    Specificity ParseSelector(const std::string& selector) const;
    
    /**
     * @brief 合并属性值（按优先级和顺序）
     * @param property 属性名
     * @param values 属性值列表
     * @return 最终值
     */
    std::string MergePropertyValues(
        const std::string& property,
        const std::vector<PropertyValue>& values
    ) const;
    
    /**
     * @brief 从父元素继承属性
     * @param element 目标元素
     * @param property 属性名
     * @return 继承的值，如果无法继承则返回空字符串
     */
    std::string InheritFromParent(Element* element, const std::string& property) const;
};

} // namespace mblink

