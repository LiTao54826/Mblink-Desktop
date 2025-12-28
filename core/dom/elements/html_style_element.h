/**
 * @file html_style_element.h
 * @brief HTMLStyleElement 类 - <style> 元素
 */

#pragma once

#include "../element.h"

namespace lightui {

// 前向声明
class StyleManager;

/**
 * @brief HTMLStyleElement 类
 * 
 * 表示 HTML <style> 元素，包含内联 CSS 样式
 * 
 * 支持的属性：
 * - type: 样式类型（默认 "text/css"）
 * - media: 媒体查询
 * - disabled: 是否禁用
 */
class HTMLStyleElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLStyleElement();

    /**
     * @brief 析构函数
     */
    ~HTMLStyleElement() override = default;

    // ========== 特有属性 ==========

    /**
     * @brief 获取样式类型
     * @return 样式类型（默认 "text/css"）
     */
    std::string GetType() const;

    /**
     * @brief 设置样式类型
     * @param type 样式类型
     */
    void SetType(const std::string& type);

    /**
     * @brief 获取媒体查询
     * @return 媒体查询字符串
     */
    std::string GetMedia() const;

    /**
     * @brief 设置媒体查询
     * @param media 媒体查询字符串
     */
    void SetMedia(const std::string& media);

    /**
     * @brief 检查是否禁用
     * @return true 表示禁用
     */
    bool IsDisabled() const { return disabled_; }

    /**
     * @brief 设置禁用状态
     * @param disabled 是否禁用
     */
    void SetDisabled(bool disabled);

    // ========== CSS 内容操作 ==========

    /**
     * @brief 获取 CSS 文本内容
     * @return CSS 文本
     */
    std::string GetCSSText() const;

    /**
     * @brief 设置 CSS 文本内容
     * @param css CSS 文本
     * 
     * 设置后会自动触发样式重新解析
     */
    void SetCSSText(const std::string& css);

    /**
     * @brief 设置文本内容（重写）
     * @param content 文本内容
     * 
     * 重写以在内容改变时触发样式更新
     */
    void SetTextContent(const std::string& content) override;

private:
    /**
     * @brief 通知样式管理器更新
     */
    void NotifyStyleUpdate();

private:
    bool disabled_ = false;  // 是否禁用
};

} // namespace lightui
