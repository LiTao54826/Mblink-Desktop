/**
 * @file style_manager.h
 * @brief 样式管理器 - 管理多个样式表并计算元素样式
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include "lexbor_stylesheet.h"
#include "core/render/animation/animation_controller.h"

namespace mbink {

// 前向声明
class Element;
class Document;

/**
 * @brief 样式表条目
 */
struct StyleSheetEntry {
    std::shared_ptr<LexborStyleSheet> sheet;  // 样式表
    int priority;                              // 优先级（数值越大优先级越高）
    std::string source;                        // 来源（"user", "inline", "default"等）
    
    StyleSheetEntry(std::shared_ptr<LexborStyleSheet> s, int p = 0, const std::string& src = "user")
        : sheet(s), priority(p), source(src) {}
};

/**
 * @brief 样式管理器类
 * 
 * 管理文档中的所有样式表，包括：
 * - 外部CSS文件
 * - <style>标签中的CSS
 * - 内联样式（style属性）
 * - 默认样式
 */
class StyleManager {
public:
    /**
     * @brief 构造函数
     * @param doc 关联的文档
     */
    explicit StyleManager(Document* doc = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~StyleManager();
    
    // 禁止拷贝
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;
    
    /**
     * @brief 添加样式表
     * @param sheet 样式表
     * @param priority 优先级（默认0，数值越大优先级越高）
     * @param source 来源标识
     */
    void AddStyleSheet(std::shared_ptr<LexborStyleSheet> sheet, 
                       int priority = 0, 
                       const std::string& source = "user");
    
    /**
     * @brief 移除样式表
     * @param sheet 要移除的样式表
     * @return 是否成功移除
     */
    bool RemoveStyleSheet(std::shared_ptr<LexborStyleSheet> sheet);
    
    /**
     * @brief 清空所有样式表
     */
    void ClearStyleSheets();
    
    /**
     * @brief 获取样式表数量
     * @return 样式表数量
     */
    size_t GetStyleSheetCount() const { return stylesheets_.size(); }
    
    /**
     * @brief 解析<style>元素
     * @param style_element <style>元素
     * @return 是否成功
     */
    bool ParseStyleElement(Element* style_element);
    
    /**
     * @brief 解析内联样式
     * @param element 元素
     * @param style 样式字符串
     * @return 解析后的样式声明
     */
    std::map<std::string, std::string> ParseInlineStyle(const std::string& style) const;
    
    /**
     * @brief 加载外部CSS文件
     * @param file_path 文件路径
     * @param priority 优先级
     * @return 是否成功
     */
    bool LoadCSSFile(const std::string& file_path, int priority = 0);

    /**
     * @brief 解析CSS字符串
     * @param css_text CSS文本内容
     * @param priority 优先级
     * @param source 来源标识
     * @return 是否成功
     */
    bool ParseCSSString(const std::string& css_text, int priority = 0, const std::string& source = "external");
    
    /**
     * @brief 获取匹配元素的所有规则
     * @param element 元素
     * @return 匹配的规则列表（按优先级排序）
     */
    std::vector<const CSSRule*> GetMatchingRules(Element* element) const;
    
    /**
     * @brief 计算元素的最终样式
     * @param element 元素
     * @return 计算后的样式声明
     */
    std::map<std::string, std::string> ComputeStyle(Element* element) const;
    
    /**
     * @brief 设置关联的文档
     * @param doc 文档指针
     */
    void SetDocument(Document* doc) { document_ = doc; }
    
    /**
     * @brief 获取关联的文档
     * @return 文档指针
     */
    Document* GetDocument() const { return document_; }

    /**
     * @brief 获取动画控制器
     * @return 动画控制器引用
     */
    AnimationController& GetAnimationController() { return animation_controller_; }

    /**
     * @brief 获取动画控制器 (const 版本)
     * @return 动画控制器引用
     */
    const AnimationController& GetAnimationController() const { return animation_controller_; }

    /**
     * @brief 检查元素是否有 :hover 相关的 CSS 规则
     * @param element 元素
     * @return 是否有 hover 规则
     */
    bool HasHoverRules(Element* element) const;
    
private:
    /**
     * @brief 检查选择器是否匹配元素（支持后代选择器）
     * @param selector 选择器字符串
     * @param element 元素
     * @return 是否匹配
     */
    bool MatchesSelector(const std::string& selector, Element* element) const;

    /**
     * @brief 检查简单选择器是否匹配元素（不支持后代选择器）
     * @param selector 简单选择器字符串
     * @param element 元素
     * @return 是否匹配
     */
    bool MatchesSimpleSelector(const std::string& selector, Element* element) const;

    /**
     * @brief 解析CSS声明块
     * @param declarations CSS声明字符串（如 "color: red; font-size: 14px;"）
     * @return 解析后的声明映射
     */
    std::map<std::string, std::string> ParseDeclarations(const std::string& declarations) const;
    
    /**
     * @brief 合并样式声明（处理优先级和!important）
     * @param base 基础样式
     * @param override 覆盖样式
     * @return 合并后的样式
     */
    std::map<std::string, std::string> MergeStyles(
        const std::map<std::string, std::string>& base,
        const std::map<std::string, std::string>& override) const;

    /**
     * @brief 从 CSS 文本中提取并注册 @keyframes 规则
     * @param css_text CSS 文本内容
     */
    void ExtractAndRegisterKeyframes(const std::string& css_text);

    /**
     * @brief 查找 CSS 文本中的所有 @keyframes 块
     * @param css_text CSS 文本内容
     * @return @keyframes 规则字符串列表
     */
    std::vector<std::string> FindKeyframesBlocks(const std::string& css_text) const;
    
private:
    struct HoverRuleCacheEntry {
        std::string selector_signature;
        size_t stylesheet_version = 0;
        bool has_hover_rule = false;
    };

    void InvalidateHoverRuleCache();

private:
    Document* document_;                          // 关联的文档
    std::vector<StyleSheetEntry> stylesheets_;    // 样式表列表
    std::map<Element*, std::map<std::string, std::string>> inline_styles_; // 内联样式缓存
    AnimationController animation_controller_;    // 动画控制器
    mutable std::unordered_map<Element*, HoverRuleCacheEntry> hover_rule_cache_;
    size_t stylesheet_version_ = 0;
};

} // namespace mbink

