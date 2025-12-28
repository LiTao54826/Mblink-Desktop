/**
 * @file html_link_element.h
 * @brief HTMLLinkElement 类定义
 * 
 * 表示 HTML <link> 元素，用于链接外部资源（样式表、图标等）
 */

#pragma once

#include "../element.h"

namespace lightui {

/**
 * @brief HTMLLinkElement 类
 * 
 * 表示 HTML <link> 元素，用于链接外部资源
 * 
 * 支持的属性：
 * - rel: 关系类型（如 "stylesheet"）
 * - href: 外部资源 URL
 * - type: MIME 类型
 * - media: 媒体查询
 * - disabled: 是否禁用（仅对样式表有效）
 */
class HTMLLinkElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLLinkElement();

    /**
     * @brief 析构函数
     */
    ~HTMLLinkElement() override = default;

    // ========== 特有属性 ==========

    /**
     * @brief 获取关系类型
     * @return rel 属性值
     */
    std::string GetRel() const;

    /**
     * @brief 设置关系类型
     * @param rel 关系类型
     */
    void SetRel(const std::string& rel);

    /**
     * @brief 获取外部资源 URL
     * @return href 属性值
     */
    std::string GetHref() const;

    /**
     * @brief 设置外部资源 URL
     * @param href 资源 URL
     */
    void SetHref(const std::string& href);

    /**
     * @brief 获取 MIME 类型
     * @return type 属性值
     */
    std::string GetType() const;

    /**
     * @brief 设置 MIME 类型
     * @param type MIME 类型
     */
    void SetType(const std::string& type);

    /**
     * @brief 获取媒体查询
     * @return media 属性值
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

    // ========== 加载状态 ==========

    /**
     * @brief 检查是否已加载
     * @return true 表示已加载
     */
    bool IsLoaded() const { return loaded_; }

    /**
     * @brief 标记为已加载
     */
    void MarkLoaded() { loaded_ = true; }

    /**
     * @brief 重置加载状态
     */
    void ResetLoaded() { loaded_ = false; }

    /**
     * @brief 检查是否是样式表链接
     * @return true 表示是样式表
     */
    bool IsStylesheet() const;

private:
    bool disabled_ = false;  ///< 是否禁用
    bool loaded_ = false;    ///< 是否已加载
};

} // namespace lightui

