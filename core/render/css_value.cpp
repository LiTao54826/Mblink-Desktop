/**
 * @file css_value.cpp
 * @brief CSS 值解析和转换实现
 */

#include "css_value.h"
#include "color.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdlib>

namespace lightui {

// ========== CSSLength 实现 ==========

float CSSLength::ToPx(float base_value, float font_size, float root_font_size) const {
    switch (unit) {
        case CSSUnit::PX:
            return value;
        case CSSUnit::PERCENT:
            return base_value * (value / 100.0f);
        case CSSUnit::EM:
            return value * font_size;
        case CSSUnit::REM:
            return value * root_font_size;
        case CSSUnit::AUTO:
        case CSSUnit::NONE:
        default:
            return 0.0f;
    }
}

// ========== CSSValue 实现 ==========

std::string CSSValue::Trim(const std::string& str) {
    if (str.empty()) {
        return str;
    }
    
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> CSSValue::Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        std::string trimmed = Trim(token);
        if (!trimmed.empty()) {
            tokens.push_back(trimmed);
        }
    }
    
    return tokens;
}

CSSLength CSSValue::ParseLength(const std::string& str) {
    std::string trimmed = Trim(str);
    
    if (trimmed.empty()) {
        return CSSLength(0.0f, CSSUnit::PX);
    }
    
    // 检查是否为 auto
    std::string lower = trimmed;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "auto") {
        return CSSLength(0.0f, CSSUnit::AUTO);
    }
    
    // 查找单位
    size_t i = 0;
    while (i < trimmed.length() && (std::isdigit(trimmed[i]) || trimmed[i] == '.' || trimmed[i] == '-')) {
        i++;
    }
    
    if (i == 0) {
        return CSSLength(0.0f, CSSUnit::PX);
    }
    
    // 解析数值
    std::string value_str = trimmed.substr(0, i);
    float value = ParseFloat(value_str, 0.0f);
    
    // 解析单位
    if (i >= trimmed.length()) {
        // 无单位，默认为像素
        return CSSLength(value, CSSUnit::PX);
    }
    
    std::string unit_str = trimmed.substr(i);
    std::transform(unit_str.begin(), unit_str.end(), unit_str.begin(), ::tolower);
    
    if (unit_str == "px") {
        return CSSLength(value, CSSUnit::PX);
    } else if (unit_str == "%") {
        return CSSLength(value, CSSUnit::PERCENT);
    } else if (unit_str == "em") {
        return CSSLength(value, CSSUnit::EM);
    } else if (unit_str == "rem") {
        return CSSLength(value, CSSUnit::REM);
    } else {
        // 未知单位，默认为像素
        return CSSLength(value, CSSUnit::PX);
    }
}

SkColor CSSValue::ParseColor(const std::string& str) {
    return Color::Parse(str);
}

CSSEdges CSSValue::ParseEdges(const std::string& str) {
    std::string trimmed = Trim(str);
    
    if (trimmed.empty()) {
        return CSSEdges();
    }
    
    // 分割值（空格分隔）
    std::vector<std::string> tokens;
    std::stringstream ss(trimmed);
    std::string token;
    
    while (ss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        return CSSEdges();
    }
    
    // 根据值的数量解析
    if (tokens.size() == 1) {
        // 一个值：所有边
        CSSLength all = ParseLength(tokens[0]);
        return CSSEdges(all);
    } else if (tokens.size() == 2) {
        // 两个值：上下 左右
        CSSLength vertical = ParseLength(tokens[0]);
        CSSLength horizontal = ParseLength(tokens[1]);
        return CSSEdges(vertical, horizontal, vertical, horizontal);
    } else if (tokens.size() == 3) {
        // 三个值：上 左右 下
        CSSLength top = ParseLength(tokens[0]);
        CSSLength horizontal = ParseLength(tokens[1]);
        CSSLength bottom = ParseLength(tokens[2]);
        return CSSEdges(top, horizontal, bottom, horizontal);
    } else {
        // 四个值：上 右 下 左
        CSSLength top = ParseLength(tokens[0]);
        CSSLength right = ParseLength(tokens[1]);
        CSSLength bottom = ParseLength(tokens[2]);
        CSSLength left = ParseLength(tokens[3]);
        return CSSEdges(top, right, bottom, left);
    }
}

CSSBorderStyle CSSValue::ParseBorderStyle(const std::string& str) {
    std::string trimmed = Trim(str);
    std::string lower = trimmed;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "none") {
        return CSSBorderStyle::NONE;
    } else if (lower == "solid") {
        return CSSBorderStyle::SOLID;
    } else if (lower == "dashed") {
        return CSSBorderStyle::DASHED;
    } else if (lower == "dotted") {
        return CSSBorderStyle::DOTTED;
    } else if (lower == "double") {
        return CSSBorderStyle::DOUBLE;
    } else {
        return CSSBorderStyle::NONE;
    }
}

CSSBorder CSSValue::ParseBorder(const std::string& width_str, 
                                 const std::string& style_str, 
                                 const std::string& color_str) {
    CSSBorder border;
    
    if (!width_str.empty()) {
        border.width = ParseLength(width_str);
    }
    
    if (!style_str.empty()) {
        border.style = ParseBorderStyle(style_str);
    }
    
    if (!color_str.empty()) {
        border.color = ParseColor(color_str);
    }
    
    return border;
}

float CSSValue::ParseFloat(const std::string& str, float default_value) {
    std::string trimmed = Trim(str);
    
    if (trimmed.empty()) {
        return default_value;
    }
    
    try {
        return std::stof(trimmed);
    } catch (...) {
        return default_value;
    }
}

int CSSValue::ParseInt(const std::string& str, int default_value) {
    std::string trimmed = Trim(str);

    if (trimmed.empty()) {
        return default_value;
    }

    try {
        return std::stoi(trimmed);
    } catch (...) {
        return default_value;
    }
}

// ========== 高级 CSS 属性解析 ==========

CSSBorderRadius CSSValue::ParseBorderRadius(const std::string& str) {
    std::string trimmed = Trim(str);
    if (trimmed.empty()) {
        return CSSBorderRadius();
    }

    std::vector<std::string> parts = Split(trimmed, ' ');

    if (parts.empty()) {
        return CSSBorderRadius();
    }

    // 解析各个值
    std::vector<CSSLength> lengths;
    for (const auto& part : parts) {
        lengths.push_back(ParseLength(part));
    }

    // 根据值的数量设置圆角
    CSSBorderRadius radius;
    if (lengths.size() == 1) {
        // 1 个值：所有角相同
        radius = CSSBorderRadius(lengths[0]);
    } else if (lengths.size() == 2) {
        // 2 个值：top-left/bottom-right, top-right/bottom-left
        radius.top_left = lengths[0];
        radius.top_right = lengths[1];
        radius.bottom_right = lengths[0];
        radius.bottom_left = lengths[1];
    } else if (lengths.size() == 3) {
        // 3 个值：top-left, top-right/bottom-left, bottom-right
        radius.top_left = lengths[0];
        radius.top_right = lengths[1];
        radius.bottom_right = lengths[2];
        radius.bottom_left = lengths[1];
    } else {
        // 4 个值：top-left, top-right, bottom-right, bottom-left
        radius.top_left = lengths[0];
        radius.top_right = lengths[1];
        radius.bottom_right = lengths[2];
        radius.bottom_left = lengths[3];
    }

    return radius;
}

std::vector<CSSBoxShadow> CSSValue::ParseBoxShadow(const std::string& str) {
    std::vector<CSSBoxShadow> shadows;
    std::string trimmed = Trim(str);

    if (trimmed.empty() || trimmed == "none") {
        return shadows;
    }

    // 简化实现：只支持单个阴影
    // 格式：offset-x offset-y blur-radius spread-radius color [inset]
    // 例如：2px 2px 4px 0px rgba(0,0,0,0.5)

    CSSBoxShadow shadow;
    std::vector<std::string> parts = Split(trimmed, ' ');

    size_t idx = 0;

    // 检查是否有 inset
    if (!parts.empty() && parts[0] == "inset") {
        shadow.inset = true;
        idx++;
    }

    // 解析偏移和半径
    if (idx < parts.size()) {
        shadow.offset_x = ParseLength(parts[idx++]).ToPx();
    }
    if (idx < parts.size()) {
        shadow.offset_y = ParseLength(parts[idx++]).ToPx();
    }
    if (idx < parts.size()) {
        // 检查是否为颜色
        if (parts[idx].find("rgb") != std::string::npos ||
            parts[idx].find("#") != std::string::npos ||
            std::isalpha(parts[idx][0])) {
            shadow.color = ParseColor(parts[idx++]);
        } else {
            shadow.blur_radius = ParseLength(parts[idx++]).ToPx();
        }
    }
    if (idx < parts.size()) {
        if (parts[idx].find("rgb") != std::string::npos ||
            parts[idx].find("#") != std::string::npos ||
            std::isalpha(parts[idx][0])) {
            shadow.color = ParseColor(parts[idx++]);
        } else {
            shadow.spread_radius = ParseLength(parts[idx++]).ToPx();
        }
    }
    if (idx < parts.size()) {
        shadow.color = ParseColor(parts[idx++]);
    }

    shadows.push_back(shadow);
    return shadows;
}

std::optional<CSSLinearGradient> CSSValue::ParseLinearGradient(const std::string& str) {
    std::string trimmed = Trim(str);

    // 检查是否为 linear-gradient
    if (trimmed.find("linear-gradient") == std::string::npos) {
        return std::nullopt;
    }

    // 提取括号内的内容
    size_t start = trimmed.find('(');
    size_t end = trimmed.rfind(')');
    if (start == std::string::npos || end == std::string::npos) {
        return std::nullopt;
    }

    std::string content = trimmed.substr(start + 1, end - start - 1);
    std::vector<std::string> parts = Split(content, ',');

    if (parts.empty()) {
        return std::nullopt;
    }

    CSSLinearGradient gradient;
    size_t idx = 0;

    // 检查第一个参数是否为角度
    std::string first = Trim(parts[0]);
    if (first.find("deg") != std::string::npos) {
        gradient.angle = ParseFloat(first);
        idx = 1;
    } else if (first.find("to ") == 0) {
        // 方向关键字（to top, to right, etc.）
        if (first == "to top") gradient.angle = 0.0f;
        else if (first == "to right") gradient.angle = 90.0f;
        else if (first == "to bottom") gradient.angle = 180.0f;
        else if (first == "to left") gradient.angle = 270.0f;
        idx = 1;
    }

    // 解析颜色停止点
    for (; idx < parts.size(); idx++) {
        std::string part = Trim(parts[idx]);
        std::vector<std::string> stop_parts = Split(part, ' ');

        if (stop_parts.empty()) continue;

        CSSGradientStop stop;
        stop.color = ParseColor(stop_parts[0]);

        if (stop_parts.size() > 1) {
            stop.position = ParseFloat(stop_parts[1]) / 100.0f; // 假设为百分比
        } else {
            // 自动计算位置
            if (gradient.stops.empty()) {
                stop.position = 0.0f;
            } else if (idx == parts.size() - 1) {
                stop.position = 1.0f;
            } else {
                stop.position = static_cast<float>(idx - (gradient.angle != 0.0f ? 1 : 0)) /
                               static_cast<float>(parts.size() - (gradient.angle != 0.0f ? 1 : 0) - 1);
            }
        }

        gradient.stops.push_back(stop);
    }

    return gradient;
}

std::optional<CSSRadialGradient> CSSValue::ParseRadialGradient(const std::string& str) {
    std::string trimmed = Trim(str);

    // 检查是否为 radial-gradient
    if (trimmed.find("radial-gradient") == std::string::npos) {
        return std::nullopt;
    }

    // 提取括号内的内容
    size_t start = trimmed.find('(');
    size_t end = trimmed.rfind(')');
    if (start == std::string::npos || end == std::string::npos) {
        return std::nullopt;
    }

    std::string content = trimmed.substr(start + 1, end - start - 1);
    std::vector<std::string> parts = Split(content, ',');

    if (parts.empty()) {
        return std::nullopt;
    }

    CSSRadialGradient gradient;
    size_t idx = 0;

    // 检查第一个参数是否为形状/位置
    std::string first = Trim(parts[0]);
    if (first == "circle" || first == "ellipse") {
        gradient.is_circle = (first == "circle");
        idx = 1;
    }

    // 解析颜色停止点
    for (; idx < parts.size(); idx++) {
        std::string part = Trim(parts[idx]);
        std::vector<std::string> stop_parts = Split(part, ' ');

        if (stop_parts.empty()) continue;

        CSSGradientStop stop;
        stop.color = ParseColor(stop_parts[0]);

        if (stop_parts.size() > 1) {
            stop.position = ParseFloat(stop_parts[1]) / 100.0f;
        } else {
            if (gradient.stops.empty()) {
                stop.position = 0.0f;
            } else if (idx == parts.size() - 1) {
                stop.position = 1.0f;
            } else {
                stop.position = static_cast<float>(idx - (gradient.is_circle ? 1 : 0)) /
                               static_cast<float>(parts.size() - (gradient.is_circle ? 1 : 0) - 1);
            }
        }

        gradient.stops.push_back(stop);
    }

    return gradient;
}

CSSBackgroundRepeat CSSValue::ParseBackgroundRepeat(const std::string& str) {
    std::string trimmed = Trim(str);
    std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(), ::tolower);

    if (trimmed == "no-repeat") return CSSBackgroundRepeat::NO_REPEAT;
    if (trimmed == "repeat-x") return CSSBackgroundRepeat::REPEAT_X;
    if (trimmed == "repeat-y") return CSSBackgroundRepeat::REPEAT_Y;
    return CSSBackgroundRepeat::REPEAT;
}

CSSBackgroundSize CSSValue::ParseBackgroundSize(const std::string& str) {
    std::string trimmed = Trim(str);
    std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(), ::tolower);

    CSSBackgroundSize size;

    if (trimmed == "cover") {
        size.type = CSSBackgroundSize::Type::COVER;
    } else if (trimmed == "contain") {
        size.type = CSSBackgroundSize::Type::CONTAIN;
    } else if (trimmed == "auto") {
        size.type = CSSBackgroundSize::Type::AUTO;
    } else {
        size.type = CSSBackgroundSize::Type::LENGTH;
        std::vector<std::string> parts = Split(trimmed, ' ');
        if (!parts.empty()) {
            size.width = ParseLength(parts[0]);
            if (parts.size() > 1) {
                size.height = ParseLength(parts[1]);
            } else {
                size.height = CSSLength(0, CSSUnit::AUTO);
            }
        }
    }

    return size;
}

} // namespace lightui

