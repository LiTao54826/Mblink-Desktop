/**
 * @file animation.cpp
 * @brief CSS animation 属性实现
 * @author MBink Development Team
 * @date 2025-11-14
 */

#include "animation.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace mbink {

// ============================================================================
// 辅助函数
// ============================================================================

std::string CSSAnimation::Trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    
    return str.substr(start, end - start);
}

float CSSAnimation::ParseTime(const std::string& str) {
    std::string trimmed = Trim(str);
    
    if (trimmed.empty()) {
        return 0.0f;
    }
    
    // 检查单位
    if (trimmed.size() >= 2 && trimmed.substr(trimmed.size() - 2) == "ms") {
        // 毫秒
        try {
            float value = std::stof(trimmed.substr(0, trimmed.size() - 2));
            return value / 1000.0f;  // 转换为秒
        } catch (...) {
            return 0.0f;
        }
    } else if (trimmed.size() >= 1 && trimmed.back() == 's') {
        // 秒
        try {
            return std::stof(trimmed.substr(0, trimmed.size() - 1));
        } catch (...) {
            return 0.0f;
        }
    } else {
        // 无单位，假设为秒
        try {
            return std::stof(trimmed);
        } catch (...) {
            return 0.0f;
        }
    }
}

std::pair<TimingFunction, CubicBezier> CSSAnimation::ParseSingleTimingFunction(const std::string& str) {
    std::string trimmed = Trim(str);
    
    if (trimmed == "linear") {
        return {TimingFunction::LINEAR, CubicBezier()};
    } else if (trimmed == "ease") {
        return {TimingFunction::EASE, CubicBezier()};
    } else if (trimmed == "ease-in") {
        return {TimingFunction::EASE_IN, CubicBezier()};
    } else if (trimmed == "ease-out") {
        return {TimingFunction::EASE_OUT, CubicBezier()};
    } else if (trimmed == "ease-in-out") {
        return {TimingFunction::EASE_IN_OUT, CubicBezier()};
    } else if (trimmed.find("cubic-bezier") == 0) {
        // 解析 cubic-bezier(x1, y1, x2, y2)
        size_t start = trimmed.find('(');
        size_t end = trimmed.find(')');
        
        if (start != std::string::npos && end != std::string::npos && start < end) {
            std::string params = trimmed.substr(start + 1, end - start - 1);
            std::stringstream ss(params);
            std::string token;
            std::vector<float> values;
            
            while (std::getline(ss, token, ',')) {
                try {
                    values.push_back(std::stof(Trim(token)));
                } catch (...) {
                    // 解析失败
                }
            }
            
            if (values.size() == 4) {
                return {TimingFunction::CUBIC_BEZIER, CubicBezier(values[0], values[1], values[2], values[3])};
            }
        }
    }
    
    // 默认返回 ease
    return {TimingFunction::EASE, CubicBezier()};
}

AnimationDirection CSSAnimation::ParseSingleDirection(const std::string& str) {
    std::string trimmed = Trim(str);

    if (trimmed == "normal") {
        return AnimationDirection::ANIM_NORMAL;
    } else if (trimmed == "reverse") {
        return AnimationDirection::ANIM_REVERSE;
    } else if (trimmed == "alternate") {
        return AnimationDirection::ANIM_ALTERNATE;
    } else if (trimmed == "alternate-reverse") {
        return AnimationDirection::ANIM_ALTERNATE_REVERSE;
    }

    return AnimationDirection::ANIM_NORMAL;
}

AnimationFillMode CSSAnimation::ParseSingleFillMode(const std::string& str) {
    std::string trimmed = Trim(str);
    
    if (trimmed == "none") {
        return AnimationFillMode::NONE;
    } else if (trimmed == "forwards") {
        return AnimationFillMode::FORWARDS;
    } else if (trimmed == "backwards") {
        return AnimationFillMode::BACKWARDS;
    } else if (trimmed == "both") {
        return AnimationFillMode::BOTH;
    }
    
    return AnimationFillMode::NONE;
}

// ============================================================================
// 分解属性解析
// ============================================================================

std::vector<std::string> CSSAnimation::ParseName(const std::string& str) {
    std::vector<std::string> names;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        std::string trimmed = Trim(token);
        if (!trimmed.empty() && trimmed != "none") {
            names.push_back(trimmed);
        }
    }
    
    return names;
}

std::vector<float> CSSAnimation::ParseDuration(const std::string& str) {
    std::vector<float> durations;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        durations.push_back(ParseTime(token));
    }
    
    return durations;
}

std::vector<std::pair<TimingFunction, CubicBezier>> CSSAnimation::ParseTimingFunction(const std::string& str) {
    std::vector<std::pair<TimingFunction, CubicBezier>> functions;
    
    // 需要处理 cubic-bezier(...) 中的逗号
    std::string trimmed = Trim(str);
    size_t pos = 0;
    int paren_depth = 0;
    size_t start = 0;
    
    for (size_t i = 0; i < trimmed.length(); ++i) {
        if (trimmed[i] == '(') {
            ++paren_depth;
        } else if (trimmed[i] == ')') {
            --paren_depth;
        } else if (trimmed[i] == ',' && paren_depth == 0) {
            // 找到顶层逗号
            std::string token = trimmed.substr(start, i - start);
            functions.push_back(ParseSingleTimingFunction(token));
            start = i + 1;
        }
    }
    
    // 处理最后一个
    if (start < trimmed.length()) {
        std::string token = trimmed.substr(start);
        functions.push_back(ParseSingleTimingFunction(token));
    }
    
    return functions;
}

std::vector<float> CSSAnimation::ParseDelay(const std::string& str) {
    return ParseDuration(str);  // 和 duration 解析方式相同
}

std::vector<int> CSSAnimation::ParseIterationCount(const std::string& str) {
    std::vector<int> counts;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        std::string trimmed = Trim(token);
        
        if (trimmed == "infinite") {
            counts.push_back(-1);
        } else {
            try {
                counts.push_back(std::stoi(trimmed));
            } catch (...) {
                counts.push_back(1);  // 默认值
            }
        }
    }
    
    return counts;
}

std::vector<AnimationDirection> CSSAnimation::ParseDirection(const std::string& str) {
    std::vector<AnimationDirection> directions;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        directions.push_back(ParseSingleDirection(token));
    }
    
    return directions;
}

std::vector<AnimationFillMode> CSSAnimation::ParseFillMode(const std::string& str) {
    std::vector<AnimationFillMode> modes;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        modes.push_back(ParseSingleFillMode(token));
    }
    
    return modes;
}

bool CSSAnimation::ParsePlayState(const std::string& str) {
    std::string trimmed = Trim(str);
    return (trimmed == "paused");
}

// ============================================================================
// 简写属性解析
// ============================================================================

std::vector<CSSAnimation> CSSAnimation::Parse(const std::string& str) {
    std::vector<CSSAnimation> animations;
    
    // 处理 "none" 值 - 返回空列表
    std::string trimmed = Trim(str);
    if (trimmed.empty() || trimmed == "none") {
        return animations;
    }
    
    // 分割多个动画 (以逗号分隔，但要注意 cubic-bezier 中的逗号)
    std::vector<std::string> animation_strs;
    size_t start = 0;
    int paren_depth = 0;
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '(') {
            ++paren_depth;
        } else if (str[i] == ')') {
            --paren_depth;
        } else if (str[i] == ',' && paren_depth == 0) {
            animation_strs.push_back(str.substr(start, i - start));
            start = i + 1;
        }
    }
    animation_strs.push_back(str.substr(start));
    
    // 解析每个动画
    for (const std::string& anim_str : animation_strs) {
        CSSAnimation anim;
        std::stringstream ss(anim_str);
        std::string token;
        std::vector<std::string> tokens;
        
        // 分割 token (以空格分隔，但要注意 cubic-bezier)
        std::string current_token;
        paren_depth = 0;
        
        for (char c : anim_str) {
            if (c == '(') {
                ++paren_depth;
                current_token += c;
            } else if (c == ')') {
                --paren_depth;
                current_token += c;
            } else if (std::isspace(c) && paren_depth == 0) {
                if (!current_token.empty()) {
                    tokens.push_back(Trim(current_token));
                    current_token.clear();
                }
            } else {
                current_token += c;
            }
        }
        if (!current_token.empty()) {
            tokens.push_back(Trim(current_token));
        }
        
        // 解析 tokens
        // 第一遍：找到动画名称（第一个非关键字 token）
        for (const std::string& t : tokens) {
            if (t.empty()) {
                continue;
            }

            // 检查是否是关键字
            bool is_keyword = (
                t.find("cubic-bezier") == 0 || t == "linear" || t == "ease" ||
                t == "ease-in" || t == "ease-out" || t == "ease-in-out" ||
                t == "infinite" ||
                t == "normal" || t == "reverse" || t == "alternate" || t == "alternate-reverse" ||
                t == "forwards" || t == "backwards" || t == "both" ||
                t == "paused" || t == "running"
            );

            // 检查是否是时间值（以数字开头且包含 s 或 ms）
            bool is_time = false;
            if (!t.empty() && t[0] >= '0' && t[0] <= '9') {
                if (t.find('s') != std::string::npos || t.find("ms") != std::string::npos) {
                    is_time = true;
                } else {
                    // 纯数字，可能是 iteration-count
                    is_keyword = true;
                }
            }

            if (!is_keyword && !is_time && anim.name.empty()) {
                anim.name = t;
                break;
            }
        }

        // 第二遍：解析其他属性
        for (const std::string& t : tokens) {
            if (t.empty() || t == anim.name) {
                continue;
            }

            // 尝试解析为不同的属性
            if (t.find("cubic-bezier") == 0 || t == "linear" || t == "ease" ||
                t == "ease-in" || t == "ease-out" || t == "ease-in-out") {
                // timing-function
                auto [func, bezier] = ParseSingleTimingFunction(t);
                anim.timing_function = func;
                anim.bezier = bezier;
            } else if (t == "infinite") {
                // iteration-count
                anim.iteration_count = -1;
            } else if (t[0] >= '0' && t[0] <= '9') {
                // 数字：可能是 duration, delay, 或 iteration-count
                if (t.find('s') != std::string::npos || t.find("ms") != std::string::npos) {
                    // duration 或 delay
                    float time = ParseTime(t);
                    if (anim.duration == 0.0f) {
                        anim.duration = time;
                    } else {
                        anim.delay = time;
                    }
                } else if (t.find('.') == std::string::npos) {
                    // 整数：iteration-count
                    try {
                        anim.iteration_count = std::stoi(t);
                    } catch (...) {}
                }
            } else if (t == "normal" || t == "reverse" || t == "alternate" || t == "alternate-reverse") {
                // direction
                anim.direction = ParseSingleDirection(t);
            } else if (t == "forwards" || t == "backwards" || t == "both") {
                // fill-mode
                anim.fill_mode = ParseSingleFillMode(t);
            } else if (t == "paused" || t == "running") {
                // play-state
                anim.paused = (t == "paused");
            }
        }
        
        if (anim.IsValid()) {
            animations.push_back(anim);
        }
    }
    
    return animations;
}

} // namespace mbink

