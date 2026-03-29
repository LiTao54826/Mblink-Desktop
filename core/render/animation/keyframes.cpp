/**
 * @file keyframes.cpp
 * @brief CSS @keyframes 规则实现
 * @author MBink Development Team
 * @date 2025-11-14
 */

#include "keyframes.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace mbink {

// ============================================================================
// KeyframesRule 实现
// ============================================================================

KeyframesRule KeyframesRule::Parse(const std::string& css) {
    KeyframesRule rule;
    
    // 1. 提取动画名称
    // 匹配: @keyframes name { ... }
    std::regex name_regex(R"(@keyframes\s+([a-zA-Z0-9_-]+)\s*\{)");
    std::smatch name_match;
    if (!std::regex_search(css, name_match, name_regex)) {
        return rule;  // 解析失败
    }
    rule.name = name_match[1].str();
    
    // 2. 提取规则体 (大括号内的内容)
    size_t start = css.find('{');
    size_t end = css.rfind('}');
    if (start == std::string::npos || end == std::string::npos || start >= end) {
        return rule;
    }
    std::string body = css.substr(start + 1, end - start - 1);
    
    // 3. 解析关键帧
    // 匹配: selector { properties }
    std::regex keyframe_regex(R"(([^{]+)\{([^}]+)\})");
    std::sregex_iterator iter(body.begin(), body.end(), keyframe_regex);
    std::sregex_iterator end_iter;
    
    for (; iter != end_iter; ++iter) {
        std::string selector = Trim((*iter)[1].str());
        std::string properties_str = (*iter)[2].str();
        
        // 解析选择器 (可能是多个，如 "0%, 100%")
        std::vector<float> offsets = ParseSelector(selector);
        
        // 解析属性
        std::map<std::string, std::string> properties = ParseProperties(properties_str);
        
        // 为每个 offset 创建关键帧
        for (float offset : offsets) {
            Keyframe kf(offset);
            kf.properties = properties;
            rule.AddKeyframe(kf);
        }
    }
    
    return rule;
}

std::tuple<const Keyframe*, const Keyframe*, float> 
KeyframesRule::GetKeyframesAt(float progress) const {
    if (keyframes.empty()) {
        return {nullptr, nullptr, 0.0f};
    }
    
    // 限制 progress 在 [0, 1] 范围内
    progress = std::max(0.0f, std::min(1.0f, progress));
    
    // 如果只有一个关键帧
    if (keyframes.size() == 1) {
        return {&keyframes[0], &keyframes[0], 0.0f};
    }
    
    // 如果 progress 在第一个关键帧之前
    if (progress <= keyframes[0].offset) {
        return {&keyframes[0], &keyframes[0], 0.0f};
    }
    
    // 如果 progress 在最后一个关键帧之后
    if (progress >= keyframes.back().offset) {
        return {&keyframes.back(), &keyframes.back(), 1.0f};
    }
    
    // 找到前后两个关键帧
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        const Keyframe& prev = keyframes[i];
        const Keyframe& next = keyframes[i + 1];
        
        if (progress >= prev.offset && progress <= next.offset) {
            // 计算插值因子
            float range = next.offset - prev.offset;
            float factor = (range > 0.0f) ? (progress - prev.offset) / range : 0.0f;
            return {&prev, &next, factor};
        }
    }
    
    // 不应该到达这里
    return {&keyframes.back(), &keyframes.back(), 1.0f};
}

void KeyframesRule::AddKeyframe(const Keyframe& keyframe) {
    keyframes.push_back(keyframe);
    
    // 按 offset 排序
    std::sort(keyframes.begin(), keyframes.end());
}

std::vector<float> KeyframesRule::ParseSelector(const std::string& selector) {
    std::vector<float> offsets;
    
    // 分割多个选择器 (如 "0%, 100%")
    std::stringstream ss(selector);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        token = Trim(token);
        
        if (token.empty()) {
            continue;
        }
        
        // 处理 from/to 关键字
        if (token == "from") {
            offsets.push_back(0.0f);
        } else if (token == "to") {
            offsets.push_back(1.0f);
        } else {
            // 处理百分比 (如 "50%")
            size_t percent_pos = token.find('%');
            if (percent_pos != std::string::npos) {
                try {
                    float value = std::stof(token.substr(0, percent_pos));
                    offsets.push_back(value / 100.0f);
                } catch (...) {
                    // 解析失败，忽略
                }
            }
        }
    }
    
    return offsets;
}

std::map<std::string, std::string> KeyframesRule::ParseProperties(const std::string& block) {
    std::map<std::string, std::string> properties;
    
    // 分割属性 (以分号分隔)
    std::stringstream ss(block);
    std::string line;
    
    while (std::getline(ss, line, ';')) {
        line = Trim(line);
        
        if (line.empty()) {
            continue;
        }
        
        // 分割属性名和值 (以冒号分隔)
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = Trim(line.substr(0, colon_pos));
            std::string value = Trim(line.substr(colon_pos + 1));
            
            if (!name.empty() && !value.empty()) {
                properties[name] = value;
            }
        }
    }
    
    return properties;
}

std::string KeyframesRule::Trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    // 去除前导空白
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    
    // 去除尾随空白
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    
    return str.substr(start, end - start);
}

// ============================================================================
// KeyframesManager 实现
// ============================================================================

KeyframesManager& KeyframesManager::Instance() {
    static KeyframesManager instance;
    return instance;
}

void KeyframesManager::RegisterKeyframes(const KeyframesRule& rule) {
    if (rule.IsValid()) {
        rules_[rule.name] = rule;
    }
}

const KeyframesRule* KeyframesManager::GetKeyframes(const std::string& name) const {
    auto it = rules_.find(name);
    if (it != rules_.end()) {
        return &it->second;
    }
    return nullptr;
}

void KeyframesManager::RemoveKeyframes(const std::string& name) {
    rules_.erase(name);
}

void KeyframesManager::Clear() {
    rules_.clear();
}

std::vector<std::string> KeyframesManager::GetAllNames() const {
    std::vector<std::string> names;
    names.reserve(rules_.size());
    
    for (const auto& pair : rules_) {
        names.push_back(pair.first);
    }
    
    return names;
}

} // namespace mbink

