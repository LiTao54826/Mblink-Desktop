/**
 * @file css_variables.h
 * @brief CSS 自定义属性（CSS Variables）支持
 * 
 * 功能：
 * - CSS 自定义属性定义（--custom-property）
 * - var() 函数解析和求值
 * - 变量继承和作用域
 * - 变量回退值支持
 * 
 * 参考：
 * - https://www.w3.org/TR/css-variables-1/
 * - https://developer.mozilla.org/en-US/docs/Web/CSS/Using_CSS_custom_properties
 */

#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

namespace lightui {

/**
 * @brief CSS 变量存储
 * 
 * 存储元素的自定义属性（--custom-property）
 * 支持继承和作用域
 */
class CSSVariables {
public:
    CSSVariables() = default;
    ~CSSVariables() = default;
    
    /**
     * @brief 设置 CSS 变量
     * @param name 变量名（包含 -- 前缀，如 "--primary-color"）
     * @param value 变量值
     * 
     * 示例：
     * SetVariable("--primary-color", "#007bff");
     * SetVariable("--spacing", "16px");
     */
    void SetVariable(const std::string& name, const std::string& value);
    
    /**
     * @brief 获取 CSS 变量值
     * @param name 变量名（包含 -- 前缀）
     * @return 变量值，如果不存在返回 nullopt
     * 
     * 示例：
     * auto value = GetVariable("--primary-color");
     * if (value.has_value()) {
     *     // 使用 value.value()
     * }
     */
    std::optional<std::string> GetVariable(const std::string& name) const;
    
    /**
     * @brief 检查变量是否存在
     * @param name 变量名
     * @return 如果变量存在返回 true
     */
    bool HasVariable(const std::string& name) const;
    
    /**
     * @brief 移除变量
     * @param name 变量名
     */
    void RemoveVariable(const std::string& name);
    
    /**
     * @brief 清空所有变量
     */
    void Clear();
    
    /**
     * @brief 获取所有变量
     * @return 变量名到值的映射
     */
    const std::unordered_map<std::string, std::string>& GetAllVariables() const {
        return variables_;
    }
    
    /**
     * @brief 从父作用域继承变量
     * @param parent 父作用域的变量
     * 
     * CSS 变量是可继承的，子元素会继承父元素的变量
     * 但子元素可以覆盖父元素的变量
     */
    void InheritFrom(const CSSVariables* parent);
    
    /**
     * @brief 合并变量（用于级联）
     * @param other 要合并的变量
     * 
     * 将 other 中的变量合并到当前变量中
     * 如果有重复的变量名，使用 other 中的值
     */
    void Merge(const CSSVariables& other);

private:
    std::unordered_map<std::string, std::string> variables_;
};

/**
 * @brief CSS var() 函数解析器
 * 
 * 解析和求值 var() 函数
 * 支持回退值和嵌套 var()
 */
class CSSVarResolver {
public:
    /**
     * @brief 解析 var() 函数
     * @param value 包含 var() 的 CSS 值字符串
     * @param variables 当前作用域的变量
     * @return 解析后的值，如果解析失败返回原始值
     * 
     * 示例：
     * ResolveVar("var(--primary-color)", variables) -> "#007bff"
     * ResolveVar("var(--unknown, red)", variables) -> "red"
     * ResolveVar("10px var(--spacing) 20px", variables) -> "10px 16px 20px"
     */
    static std::string ResolveVar(const std::string& value, const CSSVariables& variables);
    
    /**
     * @brief 检查字符串是否包含 var() 函数
     * @param value CSS 值字符串
     * @return 如果包含 var() 返回 true
     */
    static bool ContainsVar(const std::string& value);
    
    /**
     * @brief 提取 var() 函数的参数
     * @param var_func var() 函数字符串（如 "var(--color, red)"）
     * @return {变量名, 回退值}，如果解析失败返回 nullopt
     * 
     * 示例：
     * ParseVarFunction("var(--color)") -> {"--color", ""}
     * ParseVarFunction("var(--color, red)") -> {"--color", "red"}
     */
    static std::optional<std::pair<std::string, std::string>> ParseVarFunction(const std::string& var_func);

private:
    /**
     * @brief 查找匹配的括号
     * @param str 字符串
     * @param start 起始位置（左括号位置）
     * @return 匹配的右括号位置，如果找不到返回 std::string::npos
     */
    static size_t FindMatchingParen(const std::string& str, size_t start);
    
    /**
     * @brief 递归解析 var() 函数（支持嵌套）
     * @param value 包含 var() 的值
     * @param variables 变量存储
     * @param depth 递归深度（防止无限递归）
     * @return 解析后的值
     */
    static std::string ResolveVarRecursive(const std::string& value, 
                                          const CSSVariables& variables,
                                          int depth = 0);
    
    static constexpr int MAX_VAR_DEPTH = 10;  // 最大递归深度
};

/**
 * @brief 检查属性名是否为 CSS 自定义属性
 * @param property 属性名
 * @return 如果是自定义属性（以 -- 开头）返回 true
 * 
 * 示例：
 * IsCustomProperty("--primary-color") -> true
 * IsCustomProperty("color") -> false
 */
inline bool IsCustomProperty(const std::string& property) {
    return property.length() >= 2 && property[0] == '-' && property[1] == '-';
}

/**
 * @brief 验证自定义属性名是否有效
 * @param name 属性名
 * @return 如果有效返回 true
 * 
 * 有效的自定义属性名：
 * - 必须以 -- 开头
 * - 后面可以包含字母、数字、连字符、下划线
 * - 不能为空（除了 --）
 */
bool IsValidCustomPropertyName(const std::string& name);

/**
 * @brief 规范化自定义属性名
 * @param name 属性名
 * @return 规范化后的属性名（小写）
 * 
 * CSS 自定义属性名是大小写敏感的，但为了一致性，我们统一转换为小写
 */
std::string NormalizeCustomPropertyName(const std::string& name);

} // namespace lightui

