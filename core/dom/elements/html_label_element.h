/**
 * @file html_label_element.h
 * @brief HTML Label元素类
 * 
 * 功能：
 * - 实现<label>元素
 * - 支持for属性关联表单控件
 * - 点击label聚焦关联控件
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>

namespace mbink {

// 前向声明
class HTMLFormElement;

/**
 * @brief HTML Label元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/forms.html#the-label-element
 * 
 * IDL定义：
 * interface HTMLLabelElement : HTMLElement {
 *   readonly attribute HTMLFormElement? form;
 *   attribute DOMString htmlFor;
 *   readonly attribute HTMLElement? control;
 * };
 */
class HTMLLabelElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLLabelElement();

    /**
     * @brief 析构函数
     */
    ~HTMLLabelElement() override = default;

    // ========== IDL属性 ==========

    /**
     * @brief 获取关联的表单
     * @return 表单元素，如果没有关联表单则返回nullptr
     */
    std::shared_ptr<HTMLFormElement> GetForm() const;

    /**
     * @brief 获取for属性（关联控件的ID）
     * @return for属性值
     */
    std::string GetHtmlFor() const { return html_for_; }

    /**
     * @brief 设置for属性
     * @param html_for for属性值（控件ID）
     */
    void SetHtmlFor(const std::string& html_for);

    /**
     * @brief 获取关联的控件
     * @return 关联的控件元素，如果没有关联控件则返回nullptr
     * 
     * 查找逻辑：
     * 1. 如果有for属性，通过ID查找控件
     * 2. 如果没有for属性，查找第一个可标签化的后代元素
     */
    std::shared_ptr<Element> GetControl() const;

    // ========== 重写方法 ==========

    /**
     * @brief 设置属性
     * @param name 属性名
     * @param value 属性值
     */
    void SetAttribute(const std::string& name, const std::string& value) override;

    /**
     * @brief 移除属性
     * @param name 属性名
     */
    void RemoveAttribute(const std::string& name) override;

    /**
     * @brief 处理点击事件
     * 
     * 点击label时：
     * 1. 触发click事件
     * 2. 如果有关联控件，聚焦控件
     * 3. 如果控件是checkbox/radio，切换选中状态
     */
    void HandleClick();

private:
    /**
     * @brief 查找关联的表单
     * @return 表单元素，如果没有找到则返回nullptr
     */
    std::shared_ptr<HTMLFormElement> FindForm() const;

    /**
     * @brief 查找关联的控件（通过for属性）
     * @return 控件元素，如果没有找到则返回nullptr
     */
    std::shared_ptr<Element> FindControlById() const;

    /**
     * @brief 查找第一个可标签化的后代元素
     * @return 控件元素，如果没有找到则返回nullptr
     * 
     * 可标签化的元素包括：
     * - <button>
     * - <input> (除了type="hidden")
     * - <meter>
     * - <output>
     * - <progress>
     * - <select>
     * - <textarea>
     */
    std::shared_ptr<Element> FindLabelableDescendant() const;

    /**
     * @brief 检查元素是否可标签化
     * @param element 要检查的元素
     * @return true表示可标签化
     */
    bool IsLabelable(std::shared_ptr<Element> element) const;

    /**
     * @brief 聚焦关联的控件
     */
    void FocusControl();

    /**
     * @brief 激活关联的控件（checkbox/radio切换选中状态）
     */
    void ActivateControl();

private:
    std::string html_for_;  ///< for属性（关联控件的ID）
};

} // namespace mbink

