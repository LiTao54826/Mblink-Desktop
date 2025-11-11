/**
 * @file html_textarea_element.h
 * @brief HTML TextArea元素类
 * 
 * 参考：
 * - W3C HTML5 - HTMLTextAreaElement
 * - MDN Web Docs - HTMLTextAreaElement
 * - RmlUi/Source/Core/Elements/ElementFormControl.h
 */

#pragma once

#include "element.h"
#include <string>
#include <memory>

namespace lightui {

/**
 * @brief HTML TextArea元素类
 * 
 * 实现W3C HTMLTextAreaElement接口的子集
 * 参考：https://html.spec.whatwg.org/multipage/form-elements.html#the-textarea-element
 */
class HTMLTextAreaElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLTextAreaElement();
    
    /**
     * @brief 析构函数
     */
    ~HTMLTextAreaElement() override = default;
    
    // ========== TextArea特有属性 ==========
    
    /**
     * @brief 获取value值
     * @return 当前值
     */
    std::string GetValue() const { return value_; }
    
    /**
     * @brief 设置value值
     * @param value 新值
     * @param trigger_events 是否触发change/input事件
     */
    void SetValue(const std::string& value, bool trigger_events = false);
    
    /**
     * @brief 获取placeholder文本
     * @return placeholder文本
     */
    std::string GetPlaceholder() const { return GetAttribute("placeholder"); }
    
    /**
     * @brief 设置placeholder文本
     * @param placeholder placeholder文本
     */
    void SetPlaceholder(const std::string& placeholder) { SetAttribute("placeholder", placeholder); }
    
    /**
     * @brief 获取maxlength限制
     * @return 最大长度，-1表示无限制
     */
    int GetMaxLength() const;
    
    /**
     * @brief 设置maxlength限制
     * @param max_length 最大长度
     */
    void SetMaxLength(int max_length);
    
    /**
     * @brief 获取rows（行数）
     * @return 行数
     */
    int GetRows() const;
    
    /**
     * @brief 设置rows（行数）
     * @param rows 行数
     */
    void SetRows(int rows);
    
    /**
     * @brief 获取cols（列数）
     * @return 列数
     */
    int GetCols() const;
    
    /**
     * @brief 设置cols（列数）
     * @param cols 列数
     */
    void SetCols(int cols);
    
    /**
     * @brief 检查是否disabled
     * @return true表示禁用
     */
    bool IsDisabled() const { return HasAttribute("disabled"); }
    
    /**
     * @brief 设置disabled状态
     * @param disabled 是否禁用
     */
    void SetDisabled(bool disabled);
    
    /**
     * @brief 检查是否readonly
     * @return true表示只读
     */
    bool IsReadOnly() const { return HasAttribute("readonly"); }
    
    /**
     * @brief 设置readonly状态
     * @param readonly 是否只读
     */
    void SetReadOnly(bool readonly);
    
    /**
     * @brief 检查是否required
     * @return true表示必填
     */
    bool IsRequired() const { return HasAttribute("required"); }
    
    /**
     * @brief 设置required状态
     * @param required 是否必填
     */
    void SetRequired(bool required);
    
    // ========== 表单验证 ==========
    
    /**
     * @brief 检查输入是否有效
     * @return true表示有效
     */
    bool CheckValidity() const;
    
    /**
     * @brief 获取验证错误消息
     * @return 错误消息，如果有效则返回空字符串
     */
    std::string GetValidationMessage() const;
    
    // ========== 焦点和选择 ==========
    
    /**
     * @brief 选中所有文本
     */
    void Select();
    
    /**
     * @brief 设置选择范围
     * @param start 起始位置
     * @param end 结束位置
     */
    void SetSelectionRange(int start, int end);
    
    // ========== 内部方法 ==========
    
    /**
     * @brief 处理文本输入（由EventLoop调用）
     * @param text 输入的文本
     */
    void HandleTextInput(const std::string& text);
    
    /**
     * @brief 处理键盘事件（由EventLoop调用）
     * @param key 按键名称
     * @param ctrl_key Ctrl键是否按下
     */
    void HandleKeyPress(const std::string& key, bool ctrl_key);

protected:
    /**
     * @brief 触发change事件
     */
    void TriggerChangeEvent();
    
    /**
     * @brief 触发input事件
     */
    void TriggerInputEvent();

private:
    std::string value_;         // 当前值
    int selection_start_;       // 选择起始位置
    int selection_end_;         // 选择结束位置
};

} // namespace lightui

