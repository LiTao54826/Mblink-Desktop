/**
 * @file css_variables.cpp
 * @brief CSS 自定义属性（CSS Variables）实现
 */

#include "css_variables.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace mblink {

// ========== CSSVariables 实现 ==========

void CSSVariables::SetVariable(const std::string& name, const std::string& value) {
    if (!IsValidCustomPropertyName(name)) {
        return;
    }
    
    std::string normalized_name = NormalizeCustomPropertyName(name);
    variables_[normalized_name] = value;
}

std::optional<std::string> CSSVariables::GetVariable(const std::string& name) const {
    std::string normalized_name = NormalizeCustomPropertyName(name);
    auto it = variables_.find(normalized_name);
    if (it != variables_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool CSSVariables::HasVariable(const std::string& name) const {
    std::string normalized_name = NormalizeCustomPropertyName(name);
    return variables_.find(normalized_name) != variables_.end();
}

void CSSVariables::RemoveVariable(const std::string& name) {
    std::string normalized_name = NormalizeCustomPropertyName(name);
    variables_.erase(normalized_name);
}

void CSSVariables::Clear() {
    variables_.clear();
}

void CSSVariables::InheritFrom(const CSSVariables* parent) {
    if (!parent) {
        return;
    }
    
    // CSS 变量是可继承的
    // 将父元素的所有变量复制到当前元素
    // 但不覆盖当前元素已有的变量
    for (const auto& [name, value] : parent->variables_) {
        if (variables_.find(name) == variables_.end()) {
            variables_[name] = value;
        }
    }
}

void CSSVariables::Merge(const CSSVariables& other) {
    // 合并变量，other 中的变量会覆盖当前变量
    for (const auto& [name, value] : other.variables_) {
        variables_[name] = value;
    }
}

// ========== CSSVarResolver 实现 ==========

std::string CSSVarResolver::ResolveVar(const std::string& value, const CSSVariables& variables) {
    if (!ContainsVar(value)) {
        return value;
    }
    
    return ResolveVarRecursive(value, variables, 0);
}

bool CSSVarResolver::ContainsVar(const std::string& value) {
    return value.find("var(") != std::string::npos;
}

std::optional<std::pair<std::string, std::string>> CSSVarResolver::ParseVarFunction(const std::string& var_func) {
    // 检查是否以 "var(" 开头
    if (var_func.find("var(") != 0) {
        return std::nullopt;
    }

    // 查找匹配的右括号（从 "var(" 的左括号位置开始，即位置 3）
    size_t end_paren = FindMatchingParen(var_func, 3);  // 3 = position of '(' in "var("
    if (end_paren == std::string::npos) {
        return std::nullopt;
    }
    
    // 提取参数部分
    std::string args = var_func.substr(4, end_paren - 4);
    
    // 查找逗号分隔符（用于回退值）
    size_t comma_pos = std::string::npos;
    int paren_depth = 0;
    
    for (size_t i = 0; i < args.length(); ++i) {
        if (args[i] == '(') {
            paren_depth++;
        } else if (args[i] == ')') {
            paren_depth--;
        } else if (args[i] == ',' && paren_depth == 0) {
            comma_pos = i;
            break;
        }
    }
    
    std::string var_name;
    std::string fallback;
    
    if (comma_pos != std::string::npos) {
        // 有回退值
        var_name = args.substr(0, comma_pos);
        fallback = args.substr(comma_pos + 1);
        
        // 去除首尾空格
        var_name.erase(0, var_name.find_first_not_of(" \t\n\r"));
        var_name.erase(var_name.find_last_not_of(" \t\n\r") + 1);
        fallback.erase(0, fallback.find_first_not_of(" \t\n\r"));
        fallback.erase(fallback.find_last_not_of(" \t\n\r") + 1);
    } else {
        // 没有回退值
        var_name = args;
        var_name.erase(0, var_name.find_first_not_of(" \t\n\r"));
        var_name.erase(var_name.find_last_not_of(" \t\n\r") + 1);
    }
    
    return std::make_pair(var_name, fallback);
}

size_t CSSVarResolver::FindMatchingParen(const std::string& str, size_t start) {
    if (start >= str.length() || str[start] != '(') {
        return std::string::npos;
    }
    
    int depth = 1;
    for (size_t i = start + 1; i < str.length(); ++i) {
        if (str[i] == '(') {
            depth++;
        } else if (str[i] == ')') {
            depth--;
            if (depth == 0) {
                return i;
            }
        }
    }
    
    return std::string::npos;
}

std::string CSSVarResolver::ResolveVarRecursive(const std::string& value, 
                                               const CSSVariables& variables,
                                               int depth) {
    // 防止无限递归
    if (depth >= MAX_VAR_DEPTH) {
        return value;
    }
    
    std::string result = value;
    size_t pos = 0;
    
    while ((pos = result.find("var(", pos)) != std::string::npos) {
        // 查找匹配的右括号
        size_t end_paren = FindMatchingParen(result, pos + 3);  // 3 = "var".length()
        if (end_paren == std::string::npos) {
            // 找不到匹配的括号，跳过
            pos += 4;
            continue;
        }
        
        // 提取完整的 var() 函数
        std::string var_func = result.substr(pos, end_paren - pos + 1);
        
        // 解析 var() 函数
        auto parsed = ParseVarFunction(var_func);
        if (!parsed.has_value()) {
            pos += 4;
            continue;
        }
        
        auto [var_name, fallback] = parsed.value();
        
        // 查找变量值
        auto var_value = variables.GetVariable(var_name);
        
        std::string replacement;
        if (var_value.has_value()) {
            // 找到变量，使用变量值
            replacement = var_value.value();
        } else if (!fallback.empty()) {
            // 找不到变量，使用回退值
            replacement = fallback;
        } else {
            // 找不到变量且没有回退值，保持原样
            pos += 4;
            continue;
        }
        
        // 递归解析替换值（可能包含嵌套的 var()）
        if (ContainsVar(replacement)) {
            replacement = ResolveVarRecursive(replacement, variables, depth + 1);
        }
        
        // 替换 var() 函数
        result.replace(pos, var_func.length(), replacement);
        
        // 继续查找下一个 var()
        pos += replacement.length();
    }
    
    return result;
}

// ========== 辅助函数实现 ==========

bool IsValidCustomPropertyName(const std::string& name) {
    // 必须以 -- 开头
    if (name.length() < 3 || name[0] != '-' || name[1] != '-') {
        return false;
    }
    
    // 检查后面的字符是否有效
    for (size_t i = 2; i < name.length(); ++i) {
        char c = name[i];
        if (!std::isalnum(c) && c != '-' && c != '_') {
            return false;
        }
    }
    
    return true;
}

std::string NormalizeCustomPropertyName(const std::string& name) {
    std::string result = name;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace mblink

