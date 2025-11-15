#include "transition.h"
#include "css_value.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace lightui {

// ============================================================================
// CubicBezier Implementation
// ============================================================================

float CubicBezier::Evaluate(float t) const {
    // 使用牛顿迭代法求解贝塞尔曲线
    // 参考: https://www.w3.org/TR/css-easing-1/#cubic-bezier-algo
    
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    
    // 对于线性情况的优化
    if (x1 == y1 && x2 == y2) {
        return t;
    }
    
    // 牛顿迭代法求解 x(t) = t_target
    float t_guess = t;
    const int max_iterations = 8;
    const float epsilon = 1e-6f;
    
    for (int i = 0; i < max_iterations; ++i) {
        // 计算 x(t_guess)
        float x = 3.0f * (1.0f - t_guess) * (1.0f - t_guess) * t_guess * x1 +
                  3.0f * (1.0f - t_guess) * t_guess * t_guess * x2 +
                  t_guess * t_guess * t_guess;
        
        // 如果足够接近，返回对应的 y 值
        if (std::abs(x - t) < epsilon) {
            break;
        }
        
        // 计算导数 dx/dt
        float dx = 3.0f * (1.0f - t_guess) * (1.0f - t_guess) * x1 +
                   6.0f * (1.0f - t_guess) * t_guess * (x2 - x1) +
                   3.0f * t_guess * t_guess * (1.0f - x2);
        
        if (std::abs(dx) < epsilon) {
            break;
        }
        
        // 牛顿迭代
        t_guess = t_guess - (x - t) / dx;
        t_guess = std::max(0.0f, std::min(1.0f, t_guess));
    }
    
    // 计算 y(t_guess)
    float y = 3.0f * (1.0f - t_guess) * (1.0f - t_guess) * t_guess * y1 +
              3.0f * (1.0f - t_guess) * t_guess * t_guess * y2 +
              t_guess * t_guess * t_guess;
    
    return y;
}

// ============================================================================
// CSSTransition Implementation
// ============================================================================

std::vector<CSSTransition> CSSTransition::Parse(const std::string& str) {
    std::vector<CSSTransition> transitions;
    std::string trimmed = CSSValue::Trim(str);
    
    if (trimmed.empty() || trimmed == "none") {
        return transitions;
    }
    
    // 按逗号分割多个 transition，但要注意 cubic-bezier() 中的逗号
    std::vector<std::string> parts;
    std::string current;
    int paren_depth = 0;

    for (char c : trimmed) {
        if (c == '(') {
            paren_depth++;
            current += c;
        } else if (c == ')') {
            paren_depth--;
            current += c;
        } else if (c == ',' && paren_depth == 0) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    
    for (const auto& part : parts) {
        CSSTransition trans;
        std::string part_trimmed = CSSValue::Trim(part);
        
        // 分割单个 transition 的各个部分 (用空格分隔)
        std::vector<std::string> tokens;
        std::istringstream iss(part_trimmed);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        if (tokens.empty()) {
            continue;
        }
        
        // 解析各个部分
        // 格式: <property> <duration> <timing-function> <delay>
        // 只有 property 和 duration 是必需的
        
        size_t idx = 0;
        
        // 1. Property (必需)
        if (idx < tokens.size()) {
            trans.property = tokens[idx++];
        }
        
        // 2. Duration (必需)
        if (idx < tokens.size()) {
            trans.duration = ParseTime(tokens[idx++]);
        }
        
        // 3. Timing function (可选)
        if (idx < tokens.size()) {
            std::string timing_str = tokens[idx];
            
            // 检查是否为 cubic-bezier (可能包含多个 token)
            if (timing_str.find("cubic-bezier") != std::string::npos) {
                // 收集完整的 cubic-bezier 字符串
                std::string full_timing = timing_str;
                while (idx < tokens.size() - 1 && full_timing.find(')') == std::string::npos) {
                    full_timing += " " + tokens[++idx];
                }
                auto [func, bezier] = ParseSingleTimingFunction(full_timing);
                trans.timing_function = func;
                trans.bezier = bezier;
                idx++;
            } else {
                // 简单的 timing function
                auto [func, bezier] = ParseSingleTimingFunction(timing_str);
                trans.timing_function = func;
                trans.bezier = bezier;
                idx++;
            }
        }
        
        // 4. Delay (可选)
        if (idx < tokens.size()) {
            trans.delay = ParseTime(tokens[idx++]);
        }
        
        transitions.push_back(trans);
    }
    
    return transitions;
}

std::vector<std::string> CSSTransition::ParseProperty(const std::string& str) {
    std::vector<std::string> properties;
    std::string trimmed = CSSValue::Trim(str);
    
    if (trimmed.empty()) {
        return properties;
    }
    
    std::vector<std::string> parts = CSSValue::Split(trimmed, ',');
    for (const auto& part : parts) {
        std::string prop = CSSValue::Trim(part);
        if (!prop.empty()) {
            properties.push_back(prop);
        }
    }
    
    return properties;
}

std::vector<float> CSSTransition::ParseDuration(const std::string& str) {
    std::vector<float> durations;
    std::string trimmed = CSSValue::Trim(str);
    
    if (trimmed.empty()) {
        return durations;
    }
    
    std::vector<std::string> parts = CSSValue::Split(trimmed, ',');
    for (const auto& part : parts) {
        durations.push_back(ParseTime(CSSValue::Trim(part)));
    }
    
    return durations;
}

std::vector<std::pair<TimingFunction, CubicBezier>> CSSTransition::ParseTimingFunction(const std::string& str) {
    std::vector<std::pair<TimingFunction, CubicBezier>> functions;
    std::string trimmed = CSSValue::Trim(str);

    if (trimmed.empty()) {
        return functions;
    }

    // 按逗号分割，但要注意 cubic-bezier() 中的逗号
    std::vector<std::string> parts;
    std::string current;
    int paren_depth = 0;

    for (char c : trimmed) {
        if (c == '(') {
            paren_depth++;
            current += c;
        } else if (c == ')') {
            paren_depth--;
            current += c;
        } else if (c == ',' && paren_depth == 0) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }

    for (const auto& part : parts) {
        functions.push_back(ParseSingleTimingFunction(CSSValue::Trim(part)));
    }

    return functions;
}

std::vector<float> CSSTransition::ParseDelay(const std::string& str) {
    std::vector<float> delays;
    std::string trimmed = CSSValue::Trim(str);
    
    if (trimmed.empty()) {
        return delays;
    }
    
    std::vector<std::string> parts = CSSValue::Split(trimmed, ',');
    for (const auto& part : parts) {
        delays.push_back(ParseTime(CSSValue::Trim(part)));
    }
    
    return delays;
}

float CSSTransition::ParseTime(const std::string& str) {
    if (str.empty()) {
        return 0.0f;
    }
    
    std::string trimmed = CSSValue::Trim(str);
    
    // 查找单位
    size_t unit_pos = trimmed.find_first_not_of("0123456789.-");
    if (unit_pos == std::string::npos) {
        // 没有单位，默认为秒
        return std::stof(trimmed);
    }
    
    std::string value_str = trimmed.substr(0, unit_pos);
    std::string unit = trimmed.substr(unit_pos);
    
    float value = std::stof(value_str);
    
    if (unit == "ms") {
        return value / 1000.0f;  // 转换为秒
    } else if (unit == "s") {
        return value;
    }
    
    return value;
}

std::pair<TimingFunction, CubicBezier> CSSTransition::ParseSingleTimingFunction(const std::string& str) {
    std::string trimmed = CSSValue::Trim(str);
    
    if (trimmed == "linear") {
        return {TimingFunction::LINEAR, CubicBezier(0, 0, 1, 1)};
    } else if (trimmed == "ease") {
        return {TimingFunction::EASE, CubicBezier(0.25f, 0.1f, 0.25f, 1.0f)};
    } else if (trimmed == "ease-in") {
        return {TimingFunction::EASE_IN, CubicBezier(0.42f, 0, 1.0f, 1.0f)};
    } else if (trimmed == "ease-out") {
        return {TimingFunction::EASE_OUT, CubicBezier(0, 0, 0.58f, 1.0f)};
    } else if (trimmed == "ease-in-out") {
        return {TimingFunction::EASE_IN_OUT, CubicBezier(0.42f, 0, 0.58f, 1.0f)};
    } else if (trimmed.find("cubic-bezier") != std::string::npos) {
        // 解析 cubic-bezier(x1, y1, x2, y2)
        size_t start = trimmed.find('(');
        size_t end = trimmed.find(')');
        
        if (start != std::string::npos && end != std::string::npos) {
            std::string params = trimmed.substr(start + 1, end - start - 1);
            std::vector<std::string> values = CSSValue::Split(params, ',');
            
            if (values.size() == 4) {
                float x1 = std::stof(CSSValue::Trim(values[0]));
                float y1 = std::stof(CSSValue::Trim(values[1]));
                float x2 = std::stof(CSSValue::Trim(values[2]));
                float y2 = std::stof(CSSValue::Trim(values[3]));
                
                return {TimingFunction::CUBIC_BEZIER, CubicBezier(x1, y1, x2, y2)};
            }
        }
    }
    
    // 默认返回 ease
    return {TimingFunction::EASE, CubicBezier(0.25f, 0.1f, 0.25f, 1.0f)};
}

} // namespace lightui

