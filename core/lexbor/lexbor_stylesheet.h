/**
 * @file lexbor_stylesheet.h
 * @brief Lexbor CSS StyleSheet 包装类
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <lexbor/css/css.h>

namespace mblink {

/**
 * @brief 资源提供者回调 - 用于从嵌入资源加载
 */
using CSSAssetProvider = std::function<bool(const std::string& path, std::vector<uint8_t>& out_data)>;

/**
 * @brief CSS规则结构
 */
struct CSSRule {
    std::string selector;                           // 选择器字符串
    std::map<std::string, std::string> declarations; // 属性声明
    int specificity;                                // 选择器优先级
    size_t source_order = 0;
    bool important;                                 // 是否有!important
    
    CSSRule() : specificity(0), source_order(0), important(false) {}
};

/**
 * @brief Lexbor StyleSheet 包装类
 * 
 * 管理 Lexbor CSS 样式表的生命周期，提供 C++ 风格的 API
 */
class LexborStyleSheet {
public:
    /**
     * @brief 构造函数
     */
    LexborStyleSheet();
    
    /**
     * @brief 析构函数 - 自动清理 Lexbor 资源
     */
    ~LexborStyleSheet();
    
    // 禁止拷贝
    LexborStyleSheet(const LexborStyleSheet&) = delete;
    LexborStyleSheet& operator=(const LexborStyleSheet&) = delete;
    
    // 允许移动
    LexborStyleSheet(LexborStyleSheet&& other) noexcept;
    LexborStyleSheet& operator=(LexborStyleSheet&& other) noexcept;
    
    /**
     * @brief 解析 CSS 字符串
     * @param css CSS 字符串
     * @return 是否成功
     */
    bool ParseCSS(const std::string& css);
    
    /**
     * @brief 从文件解析 CSS（优先检查嵌入资源）
     * @param file_path 文件路径
     * @return 是否成功
     */
    bool ParseCSSFile(const std::string& file_path);
    
    /**
     * @brief 设置资源提供者（用于嵌入资源）
     */
    static void SetAssetProvider(CSSAssetProvider provider);
    
    /**
     * @brief 获取资源提供者
     */
    static CSSAssetProvider GetAssetProvider();
    
    /**
     * @brief 获取规则数量
     * @return 规则数量
     */
    size_t GetRuleCount() const { return rules_.size(); }
    
    /**
     * @brief 获取指定索引的规则
     * @param index 索引
     * @return 规则指针，如果索引无效返回 nullptr
     */
    const CSSRule* GetRule(size_t index) const;
    
    /**
     * @brief 获取所有规则
     * @return 规则列表
     */
    const std::vector<std::unique_ptr<CSSRule>>& GetRules() const { return rules_; }
    
    /**
     * @brief 添加规则
     * @param selector 选择器
     * @param declarations 属性声明
     * @return 是否成功
     */
    bool AddRule(const std::string& selector, const std::map<std::string, std::string>& declarations);
    
    /**
     * @brief 移除指定索引的规则
     * @param index 索引
     * @return 是否成功
     */
    bool RemoveRule(size_t index);
    
    /**
     * @brief 清空所有规则
     */
    void ClearRules();
    
    /**
     * @brief 检查是否有错误
     * @return 是否有错误
     */
    bool HasErrors() const { return !errors_.empty(); }
    
    /**
     * @brief 获取错误列表
     * @return 错误列表
     */
    const std::vector<std::string>& GetErrors() const { return errors_; }
    
    /**
     * @brief 获取底层的 Lexbor 样式表指针
     * @return lxb_css_stylesheet_t 指针
     */
    lxb_css_stylesheet_t* GetNativeStyleSheet() { return stylesheet_; }
    
    /**
     * @brief 序列化为 CSS 字符串
     * @return CSS 字符串
     */
    std::string SerializeToCSS();
    
private:
    /**
     * @brief 从 Lexbor 样式表提取规则
     */
    void ExtractRules();
    
    /**
     * @brief 处理样式规则
     * @param rule Lexbor 规则
     */
    void ProcessStyleRule(lxb_css_rule_style_t* rule);
    
    /**
     * @brief 计算选择器优先级
     * @param selector 选择器字符串
     * @return 优先级值
     */
    int CalculateSpecificity(const std::string& selector);
    
private:
    lxb_css_parser_t* parser_;
    lxb_css_stylesheet_t* stylesheet_;
    std::vector<std::unique_ptr<CSSRule>> rules_;
    size_t next_source_order_ = 0;
    std::vector<std::string> errors_;
    
    static CSSAssetProvider asset_provider_;
};

} // namespace mblink

