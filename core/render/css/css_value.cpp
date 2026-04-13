/**
 * @file css_value.cpp
 * @brief CSS 值解析和转换实现
 */

#include "css_value.h"
#include "core/render/utils/color.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdlib>

namespace mbink {

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

    if (function_type != CSSLength::FunctionType::NONE) {
        auto eval = [&](const std::shared_ptr<CSSLength>& expr) -> float {
            return expr ? expr->ToPx(base_value, font_size, root_font_size) : 0.0f;
        };

        if (function_type == CSSLength::FunctionType::MIN) {
            return std::min(eval(func_a), eval(func_b));
        }
        if (function_type == CSSLength::FunctionType::MAX) {
            return std::max(eval(func_a), eval(func_b));
        }
        if (function_type == CSSLength::FunctionType::CLAMP) {
            float min_value = eval(func_a);
            float preferred_value = eval(func_b);
            float max_value = eval(func_c);
            return std::max(min_value, std::min(preferred_value, max_value));
        }
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

std::vector<std::string> CSSValue::SplitTopLevel(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;
    int paren_depth = 0;

    for (char c : str) {
        if (c == '(') {
            paren_depth++;
            current += c;
        } else if (c == ')') {
            if (paren_depth > 0) {
                paren_depth--;
            }
            current += c;
        } else if (c == delimiter && paren_depth == 0) {
            std::string trimmed = Trim(current);
            if (!trimmed.empty()) {
                tokens.push_back(trimmed);
            }
            current.clear();
        } else {
            current += c;
        }
    }

    std::string trimmed = Trim(current);
    if (!trimmed.empty()) {
        tokens.push_back(trimmed);
    }

    return tokens;
}

CSSLength CSSValue::ParseLength(const std::string& str) {
    std::string trimmed = Trim(str);

    if (trimmed.empty()) {
        return CSSLength(0.0f, CSSUnit::PX);
    }

    // 检查是否为 auto / none
    std::string lower = trimmed;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "auto") {
        return CSSLength(0.0f, CSSUnit::AUTO);
    }
    if (lower == "none") {
        return CSSLength(0.0f, CSSUnit::NONE);
    }

    // 检查是否为 calc() 表达式
    if (lower.find("calc(") == 0 && lower.back() == ')') {
        return ParseCalc(trimmed);
    }

    auto parse_binary_function = [&](const std::string& prefix, bool is_min) -> std::optional<CSSLength> {
        if (lower.find(prefix) != 0 || lower.back() != ')') {
            return std::nullopt;
        }
        size_t start = trimmed.find('(');
        size_t end = trimmed.rfind(')');
        if (start == std::string::npos || end == std::string::npos || end <= start) {
            return CSSLength(0.0f, CSSUnit::PX);
        }
        auto args = SplitTopLevel(trimmed.substr(start + 1, end - start - 1), ',');
        if (args.size() != 2) {
            return CSSLength(0.0f, CSSUnit::PX);
        }
        CSSLength first = ParseLength(args[0]);
        CSSLength second = ParseLength(args[1]);
        return is_min ? CSSLength::Min(first, second) : CSSLength::Max(first, second);
    };

    if (auto min_value = parse_binary_function("min(", true)) {
        return *min_value;
    }
    if (auto max_value = parse_binary_function("max(", false)) {
        return *max_value;
    }
    if (lower.find("clamp(") == 0 && lower.back() == ')') {
        size_t start = trimmed.find('(');
        size_t end = trimmed.rfind(')');
        if (start == std::string::npos || end == std::string::npos || end <= start) {
            return CSSLength(0.0f, CSSUnit::PX);
        }
        auto args = SplitTopLevel(trimmed.substr(start + 1, end - start - 1), ',');
        if (args.size() != 3) {
            return CSSLength(0.0f, CSSUnit::PX);
        }
        return CSSLength::Clamp(ParseLength(args[0]), ParseLength(args[1]), ParseLength(args[2]));
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
    // 支持格式: calc(100% - 40px), calc(50% + 20px), calc(100% - 2em), calc(var(--gap) * 2)

    // 提取括号内的内容
    size_t start = str.find('(');
    size_t end = str.rfind(')');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return CSSLength(0.0f, CSSUnit::PX);
    }

    std::string expr = Trim(str.substr(start + 1, end - start - 1));

    auto is_scalar = [](const std::string& token, float& out) -> bool {
        std::string trimmed = CSSValue::Trim(token);
        if (trimmed.empty()) {
            return false;
        }
        for (char c : trimmed) {
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '%') {
                return false;
            }
        }
        char* parse_end = nullptr;
        out = std::strtof(trimmed.c_str(), &parse_end);
        return parse_end != trimmed.c_str() && parse_end && *parse_end == '\0';
    };

    // 查找运算符 (+, -, *, /)
    size_t op_pos = std::string::npos;
    char op = '+';

    size_t search_start = 0;
    if (!expr.empty() && expr[0] == '-') {
        search_start = 1;
    }

    for (size_t i = search_start; i < expr.length(); ++i) {
        if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
            if ((expr[i] == '+' || expr[i] == '-') && i > 0 && (expr[i-1] == 'e' || expr[i-1] == 'E')) {
                continue;
            }
            op_pos = i;
            op = expr[i];
            break;
        }
    }

    if (op_pos == std::string::npos) {
        CSSLength single = ParseLength(expr);
        if (single.unit == CSSUnit::PERCENT) {
            return CSSLength::Calc(single.value, 0.0f);
        }
        return CSSLength::Calc(0.0f, single.ToPx());
    }

    std::string left = Trim(expr.substr(0, op_pos));
    std::string right = Trim(expr.substr(op_pos + 1));

    CSSLength left_len = ParseLength(left);
    CSSLength right_len = ParseLength(right);

    auto to_calc_components = [](const CSSLength& len, float& percent, float& px) {
        if (len.is_calc) {
            percent = len.calc_percent;
            px = len.calc_px;
        } else if (len.unit == CSSUnit::PERCENT) {
            percent = len.value;
            px = 0.0f;
        } else {
            percent = 0.0f;
            px = len.ToPx();
        }
    };

    if (op == '+' || op == '-') {
        float left_percent = 0.0f, left_px = 0.0f;
        float right_percent = 0.0f, right_px = 0.0f;
        to_calc_components(left_len, left_percent, left_px);
        to_calc_components(right_len, right_percent, right_px);

        if (op == '+') {
            return CSSLength::Calc(left_percent + right_percent, left_px + right_px);
        }
        return CSSLength::Calc(left_percent - right_percent, left_px - right_px);
    }

    float scalar = 0.0f;
    if (is_scalar(right, scalar)) {
        float left_percent = 0.0f, left_px = 0.0f;
        to_calc_components(left_len, left_percent, left_px);
        if (op == '*') {
            return CSSLength::Calc(left_percent * scalar, left_px * scalar);
        }
        if (scalar != 0.0f) {
            return CSSLength::Calc(left_percent / scalar, left_px / scalar);
        }
        return CSSLength(0.0f, CSSUnit::PX);
    }

    if (op == '*' && is_scalar(left, scalar)) {
        float right_percent = 0.0f, right_px = 0.0f;
        to_calc_components(right_len, right_percent, right_px);
        return CSSLength::Calc(right_percent * scalar, right_px * scalar);
    }

    return CSSLength(0.0f, CSSUnit::PX);
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

    // 支持多个阴影,用逗号分隔
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
    for (size_t i = 0; i < shadow_strings.size(); i++) {
        const auto& shadow_str = shadow_strings[i];

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

    // 智能分割：正确处理括号内的逗号（如 rgba(0,0,0,0.05) 中的逗号）
    std::vector<std::string> parts;
    std::string current;
    int paren_depth = 0;

    for (size_t i = 0; i < content.length(); i++) {
        char c = content[i];

        if (c == '(') {
            paren_depth++;
            current += c;
        } else if (c == ')') {
            paren_depth--;
            current += c;
        } else if (c == ',' && paren_depth == 0) {
            // 顶层逗号，作为分隔符
            if (!current.empty()) {
                parts.push_back(Trim(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }

    // 添加最后一个
    if (!current.empty()) {
        parts.push_back(Trim(current));
    }

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
    } else {
        // 🎯 CSS规范：如果没有指定角度或方向，默认为 "to bottom" (180deg)
        gradient.angle = 180.0f;
    }

    // 解析颜色停止点
    for (; idx < parts.size(); idx++) {
        std::string part = Trim(parts[idx]);
        std::vector<std::string> stop_parts = Split(part, ' ');

        if (stop_parts.empty()) continue;

        CSSGradientStop stop;
        stop.color = ParseColor(stop_parts[0]);

        if (stop_parts.size() > 1) {
            std::string pos_str = stop_parts[1];
            if (pos_str.find('%') != std::string::npos) {
                // 百分比
                stop.position = ParseFloat(pos_str) / 100.0f;
                stop.is_pixel = false;
            } else if (pos_str.find("px") != std::string::npos) {
                // 像素值 - 保存原始值，在渲染时根据 background-size 动态计算
                float px_value = ParseFloat(pos_str);
                stop.is_pixel = true;
                stop.pixel_value = px_value;
                stop.position = 0.0f;  // 临时值，渲染时会重新计算
            } else {
                // 无单位，尝试解析为数字（0-1）
                stop.position = ParseFloat(pos_str);
                stop.is_pixel = false;
            }
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

    // 智能分割：正确处理括号内的逗号（如 rgba(0,0,0,0.05) 中的逗号）
    std::vector<std::string> parts;
    std::string current;
    int paren_depth = 0;

    for (size_t i = 0; i < content.length(); i++) {
        char c = content[i];

        if (c == '(') {
            paren_depth++;
            current += c;
        } else if (c == ')') {
            paren_depth--;
            current += c;
        } else if (c == ',' && paren_depth == 0) {
            // 顶层逗号，作为分隔符
            if (!current.empty()) {
                parts.push_back(Trim(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }

    // 添加最后一个
    if (!current.empty()) {
        parts.push_back(Trim(current));
    }

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

std::vector<CSSLinearGradient> CSSValue::ParseMultipleLinearGradients(const std::string& str) {
    std::vector<CSSLinearGradient> gradients;
    std::string trimmed = Trim(str);

    if (trimmed.empty()) {
        return gradients;
    }

    // 分割多个渐变，需要正确处理嵌套括号内的逗号
    std::vector<std::string> gradient_strings;
    std::string current;
    int paren_depth = 0;

    for (size_t i = 0; i < trimmed.length(); i++) {
        char c = trimmed[i];

        if (c == '(') {
            paren_depth++;
            current += c;
        } else if (c == ')') {
            paren_depth--;
            current += c;
        } else if (c == ',' && paren_depth == 0) {
            // 顶层逗号，作为分隔符
            if (!current.empty()) {
                gradient_strings.push_back(Trim(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }

    // 添加最后一个
    if (!current.empty()) {
        gradient_strings.push_back(Trim(current));
    }

    // 解析每个渐变
    for (const auto& gradient_str : gradient_strings) {
        if (gradient_str.find("linear-gradient") != std::string::npos) {
            auto gradient = ParseLinearGradient(gradient_str);
            if (gradient.has_value()) {
                gradients.push_back(*gradient);
            }
        }
    }

    return gradients;
}

std::vector<CSSBackgroundSize> CSSValue::ParseMultipleBackgroundSizes(const std::string& str) {
    std::vector<CSSBackgroundSize> sizes;
    std::string trimmed = Trim(str);

    if (trimmed.empty()) {
        return sizes;
    }

    // 分割多个尺寸值（逗号分隔）
    std::vector<std::string> size_strings;
    std::string current;

    for (size_t i = 0; i < trimmed.length(); i++) {
        char c = trimmed[i];

        if (c == ',') {
            if (!current.empty()) {
                size_strings.push_back(Trim(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }

    // 添加最后一个
    if (!current.empty()) {
        size_strings.push_back(Trim(current));
    }

    // 解析每个尺寸
    for (const auto& size_str : size_strings) {
        sizes.push_back(ParseBackgroundSize(size_str));
    }

    return sizes;
}

} // namespace mbink

