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

// ========== ViewportSize 静态成员初始化 ==========
// Default fallback values (800x600) used when viewport size is not set
// This prevents viewport units from resolving to 0 if layout happens before initialization
float ViewportSize::width_ = 800.0f;
float ViewportSize::height_ = 600.0f;

void ViewportSize::Set(float width, float height) {
    // Use fallback values if invalid dimensions are provided
    width_ = (width > 0.0f) ? width : 800.0f;
    height_ = (height > 0.0f) ? height : 600.0f;
}

// ========== CSSLength 实现 ==========

float CSSLength::ToPx(float base_value, float font_size, float root_font_size) const {
    // 处理 calc() 表达式
    if (is_calc) {
        float percent_value = base_value * (calc_percent / 100.0f);
        return percent_value + calc_px;
    }

    switch (unit) {
        case CSSUnit::PX:
            return value;
        case CSSUnit::PERCENT:
            return base_value * (value / 100.0f);
        case CSSUnit::EM:
            return value * font_size;
        case CSSUnit::REM:
            return value * root_font_size;
        case CSSUnit::VW: {
            // 从 RenderObject 获取视口宽度
            float vw = GetViewportWidth();
            return value * vw / 100.0f;
        }
        case CSSUnit::VH: {
            // 从 RenderObject 获取视口高度
            float vh = GetViewportHeight();
            return value * vh / 100.0f;
        }
        case CSSUnit::VMIN: {
            // 视口最小尺寸
            float vw = GetViewportWidth();
            float vh = GetViewportHeight();
            float vmin = std::min(vw, vh);
            return value * vmin / 100.0f;
        }
        case CSSUnit::VMAX: {
            // 视口最大尺寸
            float vw = GetViewportWidth();
            float vh = GetViewportHeight();
            float vmax = std::max(vw, vh);
            return value * vmax / 100.0f;
        }
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

    // 检查是否为 calc() 表达式
    if (lower.find("calc(") == 0 && lower.back() == ')') {
        return ParseCalc(trimmed);
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
    } else if (unit_str == "vw") {
        return CSSLength(value, CSSUnit::VW);
    } else if (unit_str == "vh") {
        return CSSLength(value, CSSUnit::VH);
    } else if (unit_str == "vmin") {
        return CSSLength(value, CSSUnit::VMIN);
    } else if (unit_str == "vmax") {
        return CSSLength(value, CSSUnit::VMAX);
    } else {
        // 未知单位，默认为像素
        return CSSLength(value, CSSUnit::PX);
    }
}

CSSLength CSSValue::ParseCalc(const std::string& str) {
    // 解析 calc() 表达式
    // 支持格式: calc(100% - 40px), calc(50% + 20px), calc(100% - 2em)

    // 提取括号内的内容
    size_t start = str.find('(');
    size_t end = str.rfind(')');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return CSSLength(0.0f, CSSUnit::PX);
    }

    std::string expr = Trim(str.substr(start + 1, end - start - 1));

    // 查找运算符 (+ 或 -)
    float percent_value = 0.0f;
    float px_value = 0.0f;

    // 简单解析: 查找 + 或 - 运算符
    size_t op_pos = std::string::npos;
    char op = '+';

    // 跳过开头的负号
    size_t search_start = 0;
    if (!expr.empty() && expr[0] == '-') {
        search_start = 1;
    }

    // 查找运算符
    for (size_t i = search_start; i < expr.length(); ++i) {
        if (expr[i] == '+' || expr[i] == '-') {
            // 确保不是数字的一部分 (如 1e-5)
            if (i > 0 && (expr[i-1] == 'e' || expr[i-1] == 'E')) {
                continue;
            }
            op_pos = i;
            op = expr[i];
            break;
        }
    }

    if (op_pos == std::string::npos) {
        // 没有运算符，只有一个值
        CSSLength single = ParseLength(expr);
        if (single.unit == CSSUnit::PERCENT) {
            // 转换百分比为 0-1 范围
            return CSSLength::Calc(single.value / 100.0f, 0.0f);
        } else {
            return CSSLength::Calc(0.0f, single.ToPx());
        }
    }

    // 解析两个操作数
    std::string left = Trim(expr.substr(0, op_pos));
    std::string right = Trim(expr.substr(op_pos + 1));

    CSSLength left_len = ParseLength(left);
    CSSLength right_len = ParseLength(right);

    // 根据单位类型分配到 percent 或 px
    // 注意：百分比值需要转换为 0-1 范围（100% = 1.0）
    if (left_len.unit == CSSUnit::PERCENT) {
        percent_value = left_len.value / 100.0f;
    } else {
        px_value = left_len.ToPx();
    }

    float right_px = 0.0f;
    if (right_len.unit == CSSUnit::PERCENT) {
        float right_percent = right_len.value / 100.0f;
        if (op == '+') {
            percent_value += right_percent;
        } else {
            percent_value -= right_percent;
        }
    } else {
        right_px = right_len.ToPx();
        if (op == '+') {
            px_value += right_px;
        } else {
            px_value -= right_px;
        }
    }

    return CSSLength::Calc(percent_value, px_value);
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

    // 支持多个阴影，用逗号分隔
    // 格式：offset-x offset-y blur-radius spread-radius color [inset]
    // 例如：2px 2px 4px 0px rgba(0,0,0,0.5), 5px 5px 10px black

    // 分割多个阴影（注意不要分割括号内的逗号，如 rgba(0,0,0,0.5)）
    std::vector<std::string> shadow_strings;
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
            if (!current.empty()) {
                shadow_strings.push_back(Trim(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        shadow_strings.push_back(Trim(current));
    }

    // 解析每个阴影
    for (const auto& shadow_str : shadow_strings) {
        CSSBoxShadow shadow;
        std::vector<std::string> parts = Split(shadow_str, ' ');

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
    }
    
    return shadows;
}

std::vector<CSSTextShadow> CSSValue::ParseTextShadow(const std::string& str) {
    std::vector<CSSTextShadow> shadows;
    std::string trimmed = Trim(str);

    if (trimmed.empty() || trimmed == "none") {
        return shadows;
    }

    // 支持多个阴影，用逗号分隔
    // 格式：offset-x offset-y blur-radius color
    // 例如：2px 2px 4px rgba(0,0,0,0.5), 1px 1px white

    // 分割多个阴影（注意不要分割括号内的逗号，如 rgba(0,0,0,0.5)）
    std::vector<std::string> shadow_strings;
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
            if (!current.empty()) {
                shadow_strings.push_back(Trim(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        shadow_strings.push_back(Trim(current));
    }

    // 解析每个阴影
    for (const auto& shadow_str : shadow_strings) {
        CSSTextShadow shadow;
        std::vector<std::string> parts = Split(shadow_str, ' ');

        size_t idx = 0;

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
            shadow.color = ParseColor(parts[idx++]);
        }

        shadows.push_back(shadow);
    }
    
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

