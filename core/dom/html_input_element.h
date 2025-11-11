/**
 * @file html_input_element.h
 * @brief HTML Input元素类
 * 
 * 参考：
 * - W3C HTML5 - HTMLInputElement
 * - MDN Web Docs - HTMLInputElement
 * - RmlUi/Source/Core/Elements/ElementFormControl.h
 */

#pragma once

#include "element.h"
#include <string>
#include <memory>

namespace lightui {

/**
 * @brief Input类型枚举
 */
enum class InputType {
    Text,       // 文本输入
    Password,   // 密码输入
    Checkbox,   // 复选框
    Radio,      // 单选按钮
    Button,     // 按钮
    Submit,     // 提交按钮
    Reset,      // 重置按钮
    Hidden,     // 隐藏字段
    Number,     // 数字输入
    Email,      // 邮箱输入
    Tel,        // 电话输入
    Url,        // URL输入
    Search,     // 搜索框
    Date,       // 日期选择
    Time,       // 时间选择
    Color,      // 颜色选择
    Range,      // 范围滑块
    File        // 文件上传
};

/**
 * @brief HTML Input元素类
 * 
 * 实现W3C HTMLInputElement接口的子集
 * 参考：https://html.spec.whatwg.org/multipage/input.html
 */
class HTMLInputElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLInputElement();
    
    /**
     * @brief 析构函数
     */
    ~HTMLInputElement() override = default;
    
    // ========== Input特有属性 ==========
    
    /**
     * @brief 获取input类型
     * @return Input类型
     */
    InputType GetInputType() const { return input_type_; }
    
    /**
     * @brief 设置input类型
     * @param type Input类型
     */
    void SetInputType(InputType type);
    
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
     * @brief 获取checked状态（用于checkbox和radio）
     * @return true表示选中
     */
    bool GetChecked() const { return checked_; }
    
    /**
     * @brief 设置checked状态
     * @param checked 是否选中
     * @param trigger_events 是否触发change事件
     */
    void SetChecked(bool checked, bool trigger_events = false);
    
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
     * @brief 选中所有文本（用于text类型）
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
    
    /**
     * @brief 将InputType转换为字符串
     * @param type Input类型
     * @return 类型字符串
     */
    static std::string InputTypeToString(InputType type);
    
    /**
     * @brief 将字符串转换为InputType
     * @param type_str 类型字符串
     * @return Input类型
     */
    static InputType StringToInputType(const std::string& type_str);

private:
    InputType input_type_;      // Input类型
    std::string value_;         // 当前值
    bool checked_;              // 选中状态（checkbox/radio）
    int selection_start_;       // 选择起始位置
    int selection_end_;         // 选择结束位置
};

} // namespace lightui

