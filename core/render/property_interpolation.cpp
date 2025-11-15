#include "property_interpolation.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace lightui {

// ============================================================================
// 公共接口
// ============================================================================

std::optional<std::string> PropertyInterpolation::Interpolate(
    const std::string& property,
    const std::string& from,
    const std::string& to,
    float factor) {
    
    // 限制 factor 在 [0, 1] 范围内
    factor = std::clamp(factor, 0.0f, 1.0f);
    
    // 根据属性类型选择插值方法
    if (IsColorProperty(property)) {
        return InterpolateColor(from, to, factor);
    } else if (IsTransformProperty(property)) {
        return InterpolateTransform(from, to, factor);
    } else if (IsNumberProperty(property)) {
        return InterpolateNumber(from, to, factor);
    }
    
    // 不支持插值的属性，使用阶跃函数
    return factor < 0.5f ? from : to;
}

std::map<std::string, std::string> PropertyInterpolation::InterpolateProperties(
    const std::map<std::string, std::string>& from,
    const std::map<std::string, std::string>& to,
    float factor) {
    
    std::map<std::string, std::string> result;
    
    // 遍历目标属性
    for (const auto& [property, to_value] : to) {
        auto from_it = from.find(property);
        if (from_it != from.end()) {
            // 两边都有该属性，进行插值
            auto interpolated = Interpolate(property, from_it->second, to_value, factor);
            if (interpolated) {
                result[property] = *interpolated;
            } else {
                result[property] = to_value;
            }
        } else {
            // 只有目标有该属性
            result[property] = to_value;
        }
    }
    
    // 添加只在起始有的属性
    for (const auto& [property, from_value] : from) {
        if (to.find(property) == to.end()) {
            result[property] = from_value;
        }
    }
    
    return result;
}

// ============================================================================
// 数值插值
// ============================================================================

std::optional<std::string> PropertyInterpolation::InterpolateNumber(
    const std::string& from,
    const std::string& to,
    float factor) {
    
    auto [from_value, from_unit] = ParseNumberWithUnit(from);
    auto [to_value, to_unit] = ParseNumberWithUnit(to);
    
    // 单位必须相同
    if (from_unit != to_unit) {
        return std::nullopt;
    }
    
    // 线性插值
    float result_value = from_value + (to_value - from_value) * factor;
    
    // 格式化结果
    std::ostringstream oss;
    oss << result_value << from_unit;
    return oss.str();
}

// ============================================================================
// 颜色插值
// ============================================================================

std::optional<std::string> PropertyInterpolation::InterpolateColor(
    const std::string& from,
    const std::string& to,
    float factor) {
    
    auto from_color = ParseColor(from);
    auto to_color = ParseColor(to);
    
    if (!from_color || !to_color) {
        return std::nullopt;
    }
    
    SkColor result = InterpolateColorValue(*from_color, *to_color, factor);
    return ColorToString(result);
}

SkColor PropertyInterpolation::InterpolateColorValue(SkColor from, SkColor to, float factor) {
    uint8_t r = static_cast<uint8_t>(SkColorGetR(from) + (SkColorGetR(to) - SkColorGetR(from)) * factor);
    uint8_t g = static_cast<uint8_t>(SkColorGetG(from) + (SkColorGetG(to) - SkColorGetG(from)) * factor);
    uint8_t b = static_cast<uint8_t>(SkColorGetB(from) + (SkColorGetB(to) - SkColorGetB(from)) * factor);
    uint8_t a = static_cast<uint8_t>(SkColorGetA(from) + (SkColorGetA(to) - SkColorGetA(from)) * factor);
    return SkColorSetARGB(a, r, g, b);
}

// ============================================================================
// Transform 插值
// ============================================================================

std::optional<std::string> PropertyInterpolation::InterpolateTransform(
    const std::string& from,
    const std::string& to,
    float factor) {
    
    // 简单实现：使用阶跃函数
    // TODO: 实现完整的 Transform 插值
    return factor < 0.5f ? from : to;
}

// ============================================================================
// 属性类型判断
// ============================================================================

bool PropertyInterpolation::IsNumberProperty(const std::string& property) {
    static const std::vector<std::string> number_properties = {
        "width", "height", "min-width", "min-height", "max-width", "max-height",
        "margin", "margin-top", "margin-right", "margin-bottom", "margin-left",
        "padding", "padding-top", "padding-right", "padding-bottom", "padding-left",
        "border-width", "border-top-width", "border-right-width", "border-bottom-width", "border-left-width",
        "border-radius", "border-top-left-radius", "border-top-right-radius", 
        "border-bottom-left-radius", "border-bottom-right-radius",
        "top", "right", "bottom", "left",
        "font-size", "line-height", "letter-spacing", "word-spacing",
        "opacity", "z-index"
    };
    
    return std::find(number_properties.begin(), number_properties.end(), property) != number_properties.end();
}

bool PropertyInterpolation::IsColorProperty(const std::string& property) {
    static const std::vector<std::string> color_properties = {
        "color", "background-color", "border-color",
        "border-top-color", "border-right-color", "border-bottom-color", "border-left-color",
        "outline-color", "text-decoration-color"
    };
    
    return std::find(color_properties.begin(), color_properties.end(), property) != color_properties.end();
}

bool PropertyInterpolation::IsTransformProperty(const std::string& property) {
    return property == "transform";
}

// ============================================================================
// 解析辅助函数
// ============================================================================

std::pair<float, std::string> PropertyInterpolation::ParseNumberWithUnit(const std::string& str) {
    std::regex number_regex(R"(^([-+]?[0-9]*\.?[0-9]+)([a-z%]*)$)");
    std::smatch match;
    
    if (std::regex_match(str, match, number_regex)) {
        float value = std::stof(match[1].str());
        std::string unit = match[2].str();
        return {value, unit};
    }
    
    return {0.0f, ""};
}

std::optional<SkColor> PropertyInterpolation::ParseColor(const std::string& str) {
    // 解析 #RRGGBB 或 #RGB
    if (str[0] == '#') {
        std::string hex = str.substr(1);
        
        if (hex.length() == 3) {
            // #RGB -> #RRGGBB
            hex = std::string(2, hex[0]) + std::string(2, hex[1]) + std::string(2, hex[2]);
        }
        
        if (hex.length() == 6) {
            unsigned int r, g, b;
            std::sscanf(hex.c_str(), "%02x%02x%02x", &r, &g, &b);
            return SkColorSetRGB(r, g, b);
        }
    }
    
    // 解析 rgb(r, g, b)
    std::regex rgb_regex(R"(rgb\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\))");
    std::smatch match;
    if (std::regex_match(str, match, rgb_regex)) {
        int r = std::stoi(match[1].str());
        int g = std::stoi(match[2].str());
        int b = std::stoi(match[3].str());
        return SkColorSetRGB(r, g, b);
    }
    
    // 解析 rgba(r, g, b, a)
    std::regex rgba_regex(R"(rgba\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([\d.]+)\s*\))");
    if (std::regex_match(str, match, rgba_regex)) {
        int r = std::stoi(match[1].str());
        int g = std::stoi(match[2].str());
        int b = std::stoi(match[3].str());
        float a = std::stof(match[4].str());
        return SkColorSetARGB(static_cast<uint8_t>(a * 255), r, g, b);
    }
    
    // 解析命名颜色
    static const std::map<std::string, SkColor> named_colors = {
        {"red", SK_ColorRED},
        {"green", SK_ColorGREEN},
        {"blue", SK_ColorBLUE},
        {"white", SK_ColorWHITE},
        {"black", SK_ColorBLACK},
        {"yellow", SK_ColorYELLOW},
        {"cyan", SK_ColorCYAN},
        {"magenta", SK_ColorMAGENTA},
        {"gray", SK_ColorGRAY},
        {"transparent", SK_ColorTRANSPARENT}
    };
    
    auto it = named_colors.find(str);
    if (it != named_colors.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::string PropertyInterpolation::ColorToString(SkColor color) {
    uint8_t a = SkColorGetA(color);
    uint8_t r = SkColorGetR(color);
    uint8_t g = SkColorGetG(color);
    uint8_t b = SkColorGetB(color);
    
    if (a == 255) {
        // 不透明颜色，使用 rgb()
        std::ostringstream oss;
        oss << "rgb(" << static_cast<int>(r) << ", " 
            << static_cast<int>(g) << ", " 
            << static_cast<int>(b) << ")";
        return oss.str();
    } else {
        // 半透明颜色，使用 rgba()
        std::ostringstream oss;
        oss << "rgba(" << static_cast<int>(r) << ", " 
            << static_cast<int>(g) << ", " 
            << static_cast<int>(b) << ", " 
            << std::fixed << std::setprecision(2) << (a / 255.0f) << ")";
        return oss.str();
    }
}

} // namespace lightui

