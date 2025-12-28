/**
 * @file html_select_element.h
 * @brief HTML Select元素类
 * 
 * 实现WHATWG HTML标准的HTMLSelectElement接口
 * https://html.spec.whatwg.org/#htmlselectelement
 */

#pragma once

#include "../element.h"
#include "html_option_element.h"
#include <string>
#include <vector>
#include <memory>

namespace lightui {

// 前向声明
class HTMLFormElement;

/**
 * @brief HTML Select元素类
 * 
 * 表示<select>元素，用于创建下拉选择框或列表框
 * 
 * IDL接口:
 * interface HTMLSelectElement : HTMLElement {
 *   attribute DOMString autocomplete;
 *   attribute boolean disabled;
 *   readonly attribute HTMLFormElement? form;
 *   attribute boolean multiple;
 *   attribute DOMString name;
 *   attribute boolean required;
 *   attribute unsigned long size;
 *   readonly attribute DOMString type;
 *   readonly attribute HTMLOptionsCollection options;
 *   attribute unsigned long length;
 *   readonly attribute HTMLCollection selectedOptions;
 *   attribute long selectedIndex;
 *   attribute DOMString value;
 *   boolean checkValidity();
 *   boolean reportValidity();
 *   void setCustomValidity(DOMString error);
 * };
 */
class HTMLSelectElement : public Element {
public:
    // ========== 构造函数 ==========
    
    /**
     * @brief 构造函数
     */
    HTMLSelectElement();
    
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
     * @brief 获取关联的form元素
     * @return form元素，如果没有返回nullptr
     */
    std::shared_ptr<HTMLFormElement> GetForm() const;
    
    /**
     * @brief 获取multiple状态
     * @return 是否多选
     */
    bool GetMultiple() const { return multiple_; }
    
    /**
     * @brief 设置multiple状态
     * @param multiple 是否多选
     */
    void SetMultiple(bool multiple);
    
    /**
     * @brief 获取name属性
     * @return 名称
     */
    std::string GetName() const { return name_; }
    
    /**
     * @brief 设置name属性
     * @param name 名称
     */
    void SetName(const std::string& name);
    
    /**
     * @brief 获取required状态
     * @return 是否必填
     */
    bool GetRequired() const { return required_; }
    
    /**
     * @brief 设置required状态
     * @param required 是否必填
     */
    void SetRequired(bool required);
    
    /**
     * @brief 获取size属性
     * @return 显示的选项数量
     */
    unsigned long GetSize() const { return size_; }
    
    /**
     * @brief 设置size属性
     * @param size 显示的选项数量
     */
    void SetSize(unsigned long size);
    
    /**
     * @brief 获取type属性
     * @return "select-one" 或 "select-multiple"
     */
    std::string GetType() const;
    
    /**
     * @brief 获取所有option元素
     * @return option元素列表
     */
    std::vector<std::shared_ptr<HTMLOptionElement>> GetOptions() const;
    
    /**
     * @brief 获取options数量
     * @return 数量
     */
    unsigned long GetLength() const;
    
    /**
     * @brief 获取所有选中的option元素
     * @return 选中的option元素列表
     */
    std::vector<std::shared_ptr<HTMLOptionElement>> GetSelectedOptions() const;
    
    /**
     * @brief 获取selectedIndex
     * @return 第一个选中option的索引，如果没有选中返回-1
     */
    long GetSelectedIndex() const;
    
    /**
     * @brief 设置selectedIndex
     * @param index 要选中的索引
     */
    void SetSelectedIndex(long index);
    
    /**
     * @brief 获取value
     * @return 第一个选中option的value
     */
    std::string GetValue() const;
    
    /**
     * @brief 设置value
     * @param value 要设置的值
     */
    void SetValue(const std::string& value);
    
    // ========== 验证方法 ==========
    
    /**
     * @brief 检查有效性
     * @return 是否有效
     */
    bool CheckValidity() const;
    
    /**
     * @brief 报告有效性
     * @return 是否有效
     */
    bool ReportValidity() const;
    
    /**
     * @brief 设置自定义验证消息
     * @param error 错误消息
     */
    void SetCustomValidity(const std::string& error) {
        custom_validity_ = error;
    }
    
    // ========== 重写方法 ==========
    
    /**
     * @brief 重写SetAttribute以处理特殊属性
     */
    void SetAttribute(const std::string& name, const std::string& value) override;
    
    /**
     * @brief 重写RemoveAttribute以处理特殊属性
     */
    void RemoveAttribute(const std::string& name) override;
    
    // ========== 内部方法 ==========

    /**
     * @brief 当option的选中状态改变时调用
     * @param option 改变的option
     */
    void OnOptionSelectionChanged(std::shared_ptr<HTMLOptionElement> option);

    /**
     * @brief 处理点击事件（切换下拉菜单或选择下一个选项）
     */
    void HandleClick();

    /**
     * @brief 检查下拉菜单是否打开
     */
    bool IsDropdownOpen() const { return is_dropdown_open_; }

    /**
     * @brief 设置下拉菜单打开状态
     */
    void SetDropdownOpen(bool open) { is_dropdown_open_ = open; }

    /**
     * @brief 选择下一个选项
     */
    void SelectNextOption();

    /**
     * @brief 选择上一个选项
     */
    void SelectPreviousOption();

    /**
     * @brief 获取当前悬停的选项索引
     */
    long GetHoveredIndex() const { return hovered_index_; }

    /**
     * @brief 设置悬停的选项索引
     */
    void SetHoveredIndex(long index) { hovered_index_ = index; }

    /**
     * @brief 选择悬停的选项并关闭下拉菜单
     */
    void SelectHoveredOption();

private:
    // ========== 私有成员变量 ==========

    bool disabled_ = false;           // 是否禁用
    bool multiple_ = false;           // 是否多选
    bool required_ = false;           // 是否必填
    unsigned long size_ = 0;          // 显示的选项数量（0表示默认）
    std::string name_;                // 表单提交时的名称
    std::string custom_validity_;     // 自定义验证消息
    bool is_dropdown_open_ = false;   // 下拉菜单是否打开
    long hovered_index_ = -1;         // 当前悬停的选项索引
    
    // ========== 辅助方法 ==========
    
    /**
     * @brief 更新伪类状态
     */
    void UpdatePseudoClasses();
    
    /**
     * @brief 查找父级form元素
     * @return form元素，如果没有返回nullptr
     */
    std::shared_ptr<HTMLFormElement> FindForm() const;
    
    /**
     * @brief 触发change事件
     */
    void TriggerChangeEvent();
};

} // namespace lightui

