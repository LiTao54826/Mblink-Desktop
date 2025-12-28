/**
 * @file dom_token_list.cpp
 * @brief DOMTokenList类实现
 * 
 * 参考：
 * - W3C DOM Standard - DOMTokenList
 * - MDN Web Docs - Element.classList
 * - RmlUi/Source/Core/Element.cpp (SetClass, IsClassSet)
 */

#include "dom_token_list.h"
#include "core/dom/element.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace lightui {

DOMTokenList::DOMTokenList(std::weak_ptr<Element> element, const std::string& attr_name)
    : element_(element)
    , attr_name_(attr_name) {
}

void DOMTokenList::Add(const std::vector<std::string>& tokens) {
    auto elem = element_.lock();
    if (!elem) {
        return;
    }
    
    // 验证所有token
    for (const auto& token : tokens) {
        // 空 token 静默忽略
        if (token.empty()) {
            continue;
        }
        
        if (!ValidateToken(token)) {
            // 符合W3C规范：包含空格的token会抛出异常
            throw std::invalid_argument("DOMTokenList: Invalid token '" + token + "'");
        }
    }
    
    // 获取当前token列表
    std::vector<std::string> current_tokens = ParseTokens(Value());
    
    // 添加新token（去重）
    for (const auto& token : tokens) {
        // 再次跳过空 token
        if (token.empty()) {
            continue;
        }
        
        auto it = std::find(current_tokens.begin(), current_tokens.end(), token);
        if (it == current_tokens.end()) {
            current_tokens.push_back(token);
        }
    }
    
    // 更新属性
    elem->SetAttribute(attr_name_, SerializeTokens(current_tokens));
}

void DOMTokenList::Add(const std::string& token) {
    Add(std::vector<std::string>{token});
}

void DOMTokenList::Remove(const std::vector<std::string>& tokens) {
    auto elem = element_.lock();
    if (!elem) {
        return;
    }
    
    // 验证所有token
    for (const auto& token : tokens) {
        if (!ValidateToken(token)) {
            throw std::invalid_argument("DOMTokenList: Invalid token '" + token + "'");
        }
    }
    
    // 获取当前token列表
    std::vector<std::string> current_tokens = ParseTokens(Value());
    
    // 移除指定token
    for (const auto& token : tokens) {
        current_tokens.erase(
            std::remove(current_tokens.begin(), current_tokens.end(), token),
            current_tokens.end()
        );
    }
    
    // 更新属性
    elem->SetAttribute(attr_name_, SerializeTokens(current_tokens));
}

void DOMTokenList::Remove(const std::string& token) {
    Remove(std::vector<std::string>{token});
}

bool DOMTokenList::Toggle(const std::string& token) {
    if (!ValidateToken(token)) {
        throw std::invalid_argument("DOMTokenList: Invalid token '" + token + "'");
    }
    
    if (Contains(token)) {
        Remove(token);
        return false;
    } else {
        Add(token);
        return true;
    }
}

bool DOMTokenList::Toggle(const std::string& token, bool force) {
    if (!ValidateToken(token)) {
        throw std::invalid_argument("DOMTokenList: Invalid token '" + token + "'");
    }
    
    if (force) {
        if (!Contains(token)) {
            Add(token);
        }
        return true;
    } else {
        if (Contains(token)) {
            Remove(token);
        }
        return false;
    }
}

bool DOMTokenList::Contains(const std::string& token) const {
    if (!ValidateToken(token)) {
        return false;
    }
    
    std::vector<std::string> tokens = ParseTokens(Value());
    return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
}

std::string DOMTokenList::Item(size_t index) const {
    std::vector<std::string> tokens = ParseTokens(Value());
    if (index < tokens.size()) {
        return tokens[index];
    }
    return "";
}

size_t DOMTokenList::Length() const {
    return ParseTokens(Value()).size();
}

std::string DOMTokenList::Value() const {
    auto elem = element_.lock();
    if (!elem) {
        return "";
    }
    return elem->GetAttribute(attr_name_);
}

void DOMTokenList::SetValue(const std::string& value) {
    auto elem = element_.lock();
    if (!elem) {
        return;
    }
    elem->SetAttribute(attr_name_, value);
}

bool DOMTokenList::Replace(const std::string& old_token, const std::string& new_token) {
    if (!ValidateToken(old_token) || !ValidateToken(new_token)) {
        throw std::invalid_argument("DOMTokenList: Invalid token");
    }
    
    auto elem = element_.lock();
    if (!elem) {
        return false;
    }
    
    std::vector<std::string> tokens = ParseTokens(Value());
    
    // 查找旧token
    auto it = std::find(tokens.begin(), tokens.end(), old_token);
    if (it == tokens.end()) {
        return false;
    }
    
    // 检查新token是否已存在
    auto new_it = std::find(tokens.begin(), tokens.end(), new_token);
    if (new_it != tokens.end()) {
        // 新token已存在，只移除旧token
        tokens.erase(it);
    } else {
        // 替换
        *it = new_token;
    }
    
    elem->SetAttribute(attr_name_, SerializeTokens(tokens));
    return true;
}

bool DOMTokenList::Supports(const std::string& token) const {
    // 对于classList，所有非空token都支持
    return !token.empty();
}

std::vector<std::string> DOMTokenList::ParseTokens(const std::string& value) const {
    std::vector<std::string> tokens;
    
    if (value.empty()) {
        return tokens;
    }
    
    // 使用istringstream分割空格
    std::istringstream iss(value);
    std::string token;
    
    while (iss >> token) {
        // 去重
        if (std::find(tokens.begin(), tokens.end(), token) == tokens.end()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::string DOMTokenList::SerializeTokens(const std::vector<std::string>& tokens) const {
    if (tokens.empty()) {
        return "";
    }
    
    std::string result;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) {
            result += " ";
        }
        result += tokens[i];
    }
    
    return result;
}

bool DOMTokenList::ValidateToken(const std::string& token) const {
    // token不能为空
    if (token.empty()) {
        return false;
    }
    
    // token不能包含空格
    if (token.find(' ') != std::string::npos ||
        token.find('\t') != std::string::npos ||
        token.find('\n') != std::string::npos ||
        token.find('\r') != std::string::npos) {
        return false;
    }
    
    return true;
}

void DOMTokenList::UpdateAttribute() {
    // 这个方法在当前实现中不需要，因为我们直接通过SetAttribute更新
    // 保留这个方法以备将来需要
}

} // namespace lightui

