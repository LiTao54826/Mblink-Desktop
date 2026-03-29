/**
 * @file html_button_element.h
 * @brief HTML Button元素类
 * 
 * 标准参考:
 * - WHATWG HTML: https://html.spec.whatwg.org/multipage/form-elements.html#the-button-element
 * - MDN Web Docs: https://developer.mozilla.org/en-US/docs/Web/HTML/Element/button
 * - Web IDL: https://html.spec.whatwg.org/multipage/form-elements.html#htmlbuttonelement
 * 
 * 实现状态:
 * - [x] 基础IDL属性 (disabled, type, name, value)
 * - [x] 表单关联 (form)
 * - [ ] 验证API (checkValidity, reportValidity)
 * - [x] 默认样式
 * - [x] 伪类状态 (:hover, :active, :disabled, :enabled)
 * - [x] 事件处理
 * 
 * @author MBink Team
 * @date 2025-11-12
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>

namespace mbink {

// 前向声明
class HTMLFormElement;

/**
 * @brief HTML Button元素类
 * 
 * 实现WHATWG HTMLButtonElement接口
 * 参考: https://html.spec.whatwg.org/multipage/form-elements.html#htmlbuttonelement
 */
class HTMLButtonElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLButtonElement();
    
    /**
     * @brief 析构函数
     */
    ~HTMLButtonElement() override = default;
    
    // ========== IDL属性 (按标准顺序) ==========
    
    /**
     * @brief 获取disabled属性
     * @return true表示禁用
     * 
     * 标准: https://html.spec.whatwg.org/multipage/form-elements.html#dom-button-disabled
     */
    bool GetDisabled() const { return disabled_; }
    
    /**
     * @brief 设置disabled属性
     * @param disabled 是否禁用
     * 
     * 副作用:
     * - 更新:disabled伪类
     * - 触发属性变更事件
     * - 如果禁用，移除焦点
     */
    void SetDisabled(bool disabled);
    
    /**
     * @brief 获取关联的表单元素
     * @return 表单元素指针，如果没有关联则返回nullptr
     * 
     * 标准: https://html.spec.whatwg.org/multipage/form-elements.html#dom-fae-form
     */
    std::shared_ptr<HTMLFormElement> GetForm() const;
    
    /**
     * @brief 获取name属性
     * @return 表单提交时的名称
     */
    std::string GetName() const { return name_; }
    
    /**
     * @brief 设置name属性
     * @param name 表单提交时的名称
     */
    void SetName(const std::string& name);
    
    /**
     * @brief 获取type属性
     * @return "submit" | "reset" | "button"
     * 
     * 标准: https://html.spec.whatwg.org/multipage/form-elements.html#dom-button-type
     */
    std::string GetType() const { return type_; }
    
    /**
     * @brief 设置type属性
     * @param type "submit" | "reset" | "button"
     * 
     * 注意: 无效值会被忽略，保持当前值
     */
    void SetType(const std::string& type);
    
    /**
     * @brief 获取value属性
     * @return 表单提交时的值
     */
    std::string GetValue() const { return value_; }
    
    /**
     * @brief 设置value属性
     * @param value 表单提交时的值
     */
    void SetValue(const std::string& value);
    
    // ========== 验证API ==========
    
    /**
     * @brief 检查按钮是否有效
     * @return true表示有效
     * 
     * 注意: button元素总是有效的
     */
    bool CheckValidity() const { return true; }
    
    /**
     * @brief 报告验证结果
     * @return true表示有效
     * 
     * 注意: button元素总是有效的
     */
    bool ReportValidity() const { return true; }
    
    /**
     * @brief 设置自定义验证消息
     * @param error 错误消息
     * 
     * 注意: button元素不支持自定义验证
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

    /**
     * @brief 处理点击事件
     *
     * 根据type执行不同操作:
     * - submit: 提交关联的表单
     * - reset: 重置关联的表单
     * - button: 不执行默认操作
     */
    void HandleClick();
    
private:
    // ========== 私有成员变量 (按字母顺序) ==========
    
    bool disabled_ = false;              // 是否禁用
    std::string name_;                   // 表单提交时的名称
    std::string type_ = "submit";        // submit | reset | button
    std::string value_;                  // 表单提交时的值
    std::string custom_validity_;        // 自定义验证消息
    
    // ========== 私有辅助方法 ==========
    
    /**
     * @brief 更新伪类状态
     */
    void UpdatePseudoClasses();
    
    /**
     * @brief 查找关联的表单
     * @return 表单元素指针，如果没有找到则返回nullptr
     */
    std::shared_ptr<HTMLFormElement> FindForm() const;
};

} // namespace mbink

