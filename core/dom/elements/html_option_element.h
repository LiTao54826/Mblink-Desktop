/**
 * @file html_option_element.h
 * @brief HTML Option元素类
 * 
 * 实现WHATWG HTML标准的HTMLOptionElement接口
 * https://html.spec.whatwg.org/#htmloptionelement
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>

namespace lightui {

// 前向声明
class HTMLSelectElement;
class HTMLFormElement;

/**
 * @brief HTML Option元素类
 * 
 * 表示<option>元素，用于在<select>中提供选项
 * 
 * IDL接口:
 * interface HTMLOptionElement : HTMLElement {
 *   attribute boolean disabled;
 *   readonly attribute HTMLFormElement? form;
 *   attribute DOMString label;
 *   attribute boolean defaultSelected;
 *   attribute boolean selected;
 *   attribute DOMString value;
 *   attribute DOMString text;
 *   readonly attribute long index;
 * };
 */
class HTMLOptionElement : public Element {
public:
    // ========== 构造函数 ==========
    
    /**
     * @brief 构造函数
     */
    HTMLOptionElement();
    
    // ========== IDL属性 ==========
    
    /**
     * @brief 获取disabled状态
     * @return 是否禁用
     */
    bool GetDisabled() const { return disabled_; }
    
    /**
     * @brief 设置disabled状态
     * @param disabled 是否禁用
     */
    void SetDisabled(bool disabled);
    
    /**
     * @brief 获取label属性
     * @return label文本
     */
    std::string GetLabel() const;
    
    /**
     * @brief 设置label属性
     * @param label label文本
     */
    void SetLabel(const std::string& label);
    
    /**
     * @brief 获取defaultSelected状态
     * @return 默认是否选中
     */
    bool GetDefaultSelected() const { return default_selected_; }
    
    /**
     * @brief 设置defaultSelected状态
     * @param selected 默认是否选中
     */
    void SetDefaultSelected(bool selected);
    
    /**
     * @brief 获取selected状态
     * @return 是否选中
     */
    bool GetSelected() const { return selected_; }
    
    /**
     * @brief 设置selected状态
     * @param selected 是否选中
     */
    void SetSelected(bool selected);
    
    /**
     * @brief 获取value属性
     * @return 选项值
     */
    std::string GetValue() const;
    
    /**
     * @brief 设置value属性
     * @param value 选项值
     */
    void SetValue(const std::string& value);
    
    /**
     * @brief 获取text内容
     * @return 选项文本
     */
    std::string GetText() const;
    
    /**
     * @brief 设置text内容
     * @param text 选项文本
     */
    void SetText(const std::string& text);
    
    /**
     * @brief 获取在select中的索引
     * @return 索引，如果不在select中返回-1
     */
    int GetIndex() const;
    
    /**
     * @brief 获取关联的form元素
     * @return form元素，如果没有返回nullptr
     */
    std::shared_ptr<HTMLFormElement> GetForm() const;
    
    // ========== 重写方法 ==========
    
    /**
     * @brief 重写SetAttribute以处理特殊属性
     */
    void SetAttribute(const std::string& name, const std::string& value) override;
    
    /**
     * @brief 重写RemoveAttribute以处理特殊属性
     */
    void RemoveAttribute(const std::string& name) override;
    
private:
    // ========== 私有成员变量 ==========
    
    bool disabled_ = false;           // 是否禁用
    bool default_selected_ = false;   // 默认是否选中（selected属性）
    bool selected_ = false;           // 当前是否选中（运行时状态）
    std::string value_;               // 选项值
    
    // ========== 辅助方法 ==========
    
    /**
     * @brief 更新伪类状态
     */
    void UpdatePseudoClasses();
    
    /**
     * @brief 查找父级select元素
     * @return select元素，如果没有返回nullptr
     */
    std::shared_ptr<HTMLSelectElement> FindSelectElement() const;
};

} // namespace lightui

