/**
 * @file html_script_element.h
 * @brief HTMLScriptElement 类 - <script> 元素
 */

#pragma once

#include "../element.h"

namespace mbink {

/**
 * @brief HTMLScriptElement 类
 * 
 * 表示 HTML <script> 元素，包含内联 JavaScript 代码或外部脚本引用
 * 
 * 支持的属性：
 * - type: 脚本类型（默认 "text/javascript"，也支持 "module"）
 * - src: 外部脚本 URL（暂不支持加载）
 * - async: 异步加载（暂不支持）
 * - defer: 延迟执行（暂不支持）
 */
class HTMLScriptElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLScriptElement();

    /**
     * @brief 析构函数
     */
    ~HTMLScriptElement() override = default;

    // ========== 特有属性 ==========

    /**
     * @brief 获取脚本类型
     * @return 脚本类型（默认 "text/javascript"）
     */
    std::string GetType() const;

    /**
     * @brief 设置脚本类型
     * @param type 脚本类型
     */
    void SetType(const std::string& type);

    /**
     * @brief 获取外部脚本 URL
     * @return 脚本 URL
     */
    std::string GetSrc() const;

    /**
     * @brief 设置外部脚本 URL
     * @param src 脚本 URL
     */
    void SetSrc(const std::string& src);

    /**
     * @brief 检查是否为异步脚本
     * @return true 表示异步
     */
    bool IsAsync() const;

    /**
     * @brief 设置异步属性
     * @param async 是否异步
     */
    void SetAsync(bool async);

    /**
     * @brief 检查是否为延迟脚本
     * @return true 表示延迟
     */
    bool IsDefer() const;

    /**
     * @brief 设置延迟属性
     * @param defer 是否延迟
     */
    void SetDefer(bool defer);

    // ========== 脚本内容操作 ==========

    /**
     * @brief 获取脚本文本内容
     * @return 脚本文本
     */
    std::string GetScriptText() const;

    /**
     * @brief 设置脚本文本内容
     * @param text 脚本文本
     */
    void SetScriptText(const std::string& text);

    // ========== 执行状态 ==========

    /**
     * @brief 检查脚本是否已执行
     * @return true 表示已执行
     */
    bool IsExecuted() const { return executed_; }

    /**
     * @brief 标记脚本为已执行
     */
    void MarkExecuted() { executed_ = true; }

    /**
     * @brief 重置执行状态
     */
    void ResetExecuted() { executed_ = false; }

    /**
     * @brief 检查是否为外部脚本
     * @return true 表示有 src 属性
     */
    bool IsExternal() const { return HasAttribute("src"); }

    /**
     * @brief 检查是否为模块脚本
     * @return true 表示 type="module"
     */
    bool IsModule() const;

private:
    bool executed_ = false;  // 是否已执行
};

} // namespace mbink
