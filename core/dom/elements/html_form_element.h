/**
 * @file html_form_element.h
 * @brief HTML Form元素类
 * 
 * 标准参考:
 * - WHATWG HTML: https://html.spec.whatwg.org/multipage/forms.html#the-form-element
 * - MDN Web Docs: https://developer.mozilla.org/en-US/docs/Web/HTML/Element/form
 * - Web IDL: https://html.spec.whatwg.org/multipage/forms.html#htmlformelement
 * 
 * @author MBlink Team
 * @date 2025-11-12
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>
#include <vector>

namespace mblink {

/**
 * @brief HTML Form元素类
 * 
 * 实现WHATWG HTMLFormElement接口
 * 参考: https://html.spec.whatwg.org/multipage/forms.html#htmlformelement
 */
class HTMLFormElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLFormElement();
    
    /**
     * @brief 析构函数
     */
    ~HTMLFormElement() override = default;
    
    // ========== IDL属性 ==========
    
    /**
     * @brief 获取action属性
     * @return 表单提交URL
     */
    std::string GetAction() const { return GetAttribute("action"); }
    
    /**
     * @brief 设置action属性
     * @param action 表单提交URL
     */
    void SetAction(const std::string& action) { SetAttribute("action", action); }
    
    /**
     * @brief 获取method属性
     * @return "get" | "post"
     */
    std::string GetMethod() const;
    
    /**
     * @brief 设置method属性
     * @param method "get" | "post"
     */
    void SetMethod(const std::string& method);
    
    /**
     * @brief 获取enctype属性
     * @return 编码类型
     */
    std::string GetEnctype() const;
    
    /**
     * @brief 设置enctype属性
     * @param enctype 编码类型
     */
    void SetEnctype(const std::string& enctype);
    
    /**
     * @brief 获取target属性
     * @return 目标窗口
     */
    std::string GetTarget() const { return GetAttribute("target"); }
    
    /**
     * @brief 设置target属性
     * @param target 目标窗口
     */
    void SetTarget(const std::string& target) { SetAttribute("target", target); }
    
    // ========== 表单控件集合 ==========
    
    /**
     * @brief 获取所有表单控件
     * @return 表单控件列表
     */
    std::vector<std::shared_ptr<Element>> GetElements() const;
    
    /**
     * @brief 获取表单控件数量
     * @return 控件数量
     */
    int GetLength() const { return static_cast<int>(GetElements().size()); }
    
    // ========== 表单操作 ==========
    
    /**
     * @brief 提交表单
     */
    void Submit();
    
    /**
     * @brief 重置表单
     */
    void Reset();
    
    /**
     * @brief 检查表单是否有效
     * @return true表示所有控件都有效
     */
    bool CheckValidity() const;
    
    /**
     * @brief 报告验证结果
     * @return true表示所有控件都有效
     */
    bool ReportValidity() const;

    // ========== 表单数据序列化 ==========

    /**
     * @brief 获取表单数据（URL编码格式）
     * @return URL编码的表单数据字符串
     */
    std::string GetFormDataURLEncoded() const;

    /**
     * @brief 获取表单数据（JSON格式）
     * @return JSON格式的表单数据字符串
     */
    std::string GetFormDataJSON() const;

private:
    /**
     * @brief 收集表单数据
     * @return 表单数据（键值对）
     */
    std::vector<std::pair<std::string, std::string>> CollectFormData() const;

    /**
     * @brief URL编码字符串
     * @param str 要编码的字符串
     * @return 编码后的字符串
     */
    static std::string URLEncode(const std::string& str);
};

} // namespace mblink

