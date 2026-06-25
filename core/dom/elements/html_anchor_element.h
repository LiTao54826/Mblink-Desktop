/**
 * @file html_anchor_element.h
 * @brief HTML Anchor元素 (<a>)
 * 
 * 实现WHATWG HTML标准中的HTMLAnchorElement接口
 * 参考: https://html.spec.whatwg.org/#the-a-element
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>

namespace mblink {

/**
 * @brief HTML Anchor元素类
 * 
 * 实现超链接功能，包括：
 * - href: 链接地址
 * - target: 打开方式 (_blank, _self, _parent, _top)
 * - download: 下载文件名
 * - rel: 关系类型 (nofollow, noopener, noreferrer等)
 * - 伪类状态: :link, :visited, :hover, :active
 * 
 * 默认样式:
 * - color: #0000EE (蓝色)
 * - text-decoration: underline
 * - cursor: pointer
 * - :visited { color: #551A8B }
 * - :active { color: #FF0000 }
 */
class HTMLAnchorElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLAnchorElement();
    
    /**
     * @brief 析构函数
     */
    ~HTMLAnchorElement() override = default;
    
    /**
     * @brief 重写SetAttribute以处理特殊属性
     */
    void SetAttribute(const std::string& name, const std::string& value) override;
    
    /**
     * @brief 重写RemoveAttribute以处理特殊属性
     */
    void RemoveAttribute(const std::string& name) override;
    
    // ========== IDL属性 ==========
    
    /**
     * @brief 获取href属性（链接地址）
     * @return href值
     */
    std::string GetHref() const;
    
    /**
     * @brief 设置href属性
     * @param href 链接地址
     */
    void SetHref(const std::string& href);
    
    /**
     * @brief 获取target属性（打开方式）
     * @return target值 (_blank, _self, _parent, _top)
     */
    std::string GetTarget() const;
    
    /**
     * @brief 设置target属性
     * @param target 打开方式
     */
    void SetTarget(const std::string& target);
    
    /**
     * @brief 获取download属性（下载文件名）
     * @return download值
     */
    std::string GetDownload() const;
    
    /**
     * @brief 设置download属性
     * @param download 下载文件名
     */
    void SetDownload(const std::string& download);
    
    /**
     * @brief 获取rel属性（关系类型）
     * @return rel值
     */
    std::string GetRel() const;
    
    /**
     * @brief 设置rel属性
     * @param rel 关系类型
     */
    void SetRel(const std::string& rel);
    
    /**
     * @brief 获取text内容（链接文本）
     * @return 文本内容
     */
    std::string GetText() const;
    
    /**
     * @brief 设置text内容
     * @param text 文本内容
     */
    void SetText(const std::string& text);
    
    // ========== 导航方法 ==========
    
    /**
     * @brief 处理点击事件
     *
     * 根据href和target执行导航操作：
     * - 如果有download属性，触发下载
     * - 否则根据target打开链接
     * - 触发click事件
     */
    void HandleClick();
    
    /**
     * @brief 标记链接为已访问
     * 
     * 设置:visited伪类
     */
    void MarkAsVisited();
    
    /**
     * @brief 检查链接是否已访问
     * @return true表示已访问
     */
    bool IsVisited() const;
    
private:
    /**
     * @brief 更新伪类状态
     * 
     * 设置:link或:visited伪类
     */
    void UpdatePseudoClasses();
    
    /**
     * @brief 执行导航
     * 
     * 根据href和target执行实际的导航操作
     * 在MBlink中，这会触发自定义事件，由应用层处理
     */
    void Navigate();
    
    /**
     * @brief 执行下载
     * 
     * 根据href和download执行下载操作
     */
    void Download();
    
    // ========== 私有成员 ==========
    
    std::string href_;       ///< 链接地址
    std::string target_;     ///< 打开方式
    std::string download_;   ///< 下载文件名
    std::string rel_;        ///< 关系类型
    bool visited_;           ///< 是否已访问
};

} // namespace mblink

