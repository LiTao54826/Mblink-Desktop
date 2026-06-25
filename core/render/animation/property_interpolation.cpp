#include "property_interpolation.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace mblink {

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

std::optional<PropertyInterpolation::DecomposedTransform> 
PropertyInterpolation::DecomposeTransform(const std::string& transform_str) {
    DecomposedTransform result;
    
    // 处理 "none" 或空字符串
    if (transform_str.empty() || transform_str == "none") {
        return result;  // 返回默认值（单位矩阵）
    }
    
    // 解析各种 transform 函数
    // 支持: translate, translateX, translateY, rotate, scale, scaleX, scaleY, skew, skewX, skewY
    
    // translate(x, y) 或 translate(x)
    std::regex translate_regex(R"(translate\s*\(\s*([-+]?[\d.]+)(px|%|em|rem)?\s*(?:,\s*([-+]?[\d.]+)(px|%|em|rem)?)?\s*\))");
    std::smatch match;
    std::string remaining = transform_str;
    
    while (std::regex_search(remaining, match, translate_regex)) {
        result.translate_x += std::stof(match[1].str());
        if (match[3].matched) {
            result.translate_y += std::stof(match[3].str());
        }
        remaining = match.suffix().str();
    }
    
    // translateX(x)
    std::regex translateX_regex(R"(translateX\s*\(\s*([-+]?[\d.]+)(px|%|em|rem)?\s*\))");
    remaining = transform_str;
    while (std::regex_search(remaining, match, translateX_regex)) {
        result.translate_x += std::stof(match[1].str());
        remaining = match.suffix().str();
    }
    
    // translateY(y)
    std::regex translateY_regex(R"(translateY\s*\(\s*([-+]?[\d.]+)(px|%|em|rem)?\s*\))");
    remaining = transform_str;
    while (std::regex_search(remaining, match, translateY_regex)) {
        result.translate_y += std::stof(match[1].str());
        remaining = match.suffix().str();
    }
    
    // rotate(angle)
    std::regex rotate_regex(R"(rotate\s*\(\s*([-+]?[\d.]+)(deg|rad|turn)?\s*\))");
    remaining = transform_str;
    while (std::regex_search(remaining, match, rotate_regex)) {
        float angle = std::stof(match[1].str());
        std::string unit = match[2].matched ? match[2].str() : "deg";
        
        if (unit == "deg") {
            angle = angle * 3.14159265358979f / 180.0f;  // 转换为弧度
        } else if (unit == "turn") {
            angle = angle * 2.0f * 3.14159265358979f;
        }
        // rad 不需要转换
        
        result.rotate += angle;
        remaining = match.suffix().str();
    }
    
    // scale(x, y) 或 scale(x)
    std::regex scale_regex(R"(scale\s*\(\s*([-+]?[\d.]+)\s*(?:,\s*([-+]?[\d.]+))?\s*\))");
    remaining = transform_str;
    while (std::regex_search(remaining, match, scale_regex)) {
        float sx = std::stof(match[1].str());
        float sy = match[2].matched ? std::stof(match[2].str()) : sx;
        result.scale_x *= sx;
        result.scale_y *= sy;
        remaining = match.suffix().str();
    }
    
    // scaleX(x)
    std::regex scaleX_regex(R"(scaleX\s*\(\s*([-+]?[\d.]+)\s*\))");
    remaining = transform_str;
    while (std::regex_search(remaining, match, scaleX_regex)) {
        result.scale_x *= std::stof(match[1].str());
        remaining = match.suffix().str();
    }
    
    // scaleY(y)
    std::regex scaleY_regex(R"(scaleY\s*\(\s*([-+]?[\d.]+)\s*\))");
    remaining = transform_str;
    while (std::regex_search(remaining, match, scaleY_regex)) {
        result.scale_y *= std::stof(match[1].str());
        remaining = match.suffix().str();
    }
    
    // skew(x, y) 或 skew(x)
    std::regex skew_regex(R"(skew\s*\(\s*([-+]?[\d.]+)(deg|rad)?\s*(?:,\s*([-+]?[\d.]+)(deg|rad)?)?\s*\))");
    remaining = transform_str;
    while (std::regex_search(remaining, match, skew_regex)) {
        float skx = std::stof(match[1].str());
        std::string unit_x = match[2].matched ? match[2].str() : "deg";
        if (unit_x == "deg") {
            skx = skx * 3.14159265358979f / 180.0f;
        }
        result.skew_x += skx;
        
        if (match[3].matched) {
            float sky = std::stof(match[3].str());
            std::string unit_y = match[4].matched ? match[4].str() : "deg";
            if (unit_y == "deg") {
                sky = sky * 3.14159265358979f / 180.0f;
            }
            result.skew_y += sky;
        }
        remaining = match.suffix().str();
    }
    
    // skewX(x)
    std::regex skewX_regex(R"(skewX\s*\(\s*([-+]?[\d.]+)(deg|rad)?\s*\))");
    remaining = transform_str;
    while (std::regex_search(remaining, match, skewX_regex)) {
        float skx = std::stof(match[1].str());
        std::string unit = match[2].matched ? match[2].str() : "deg";
        if (unit == "deg") {
            skx = skx * 3.14159265358979f / 180.0f;
        }
        result.skew_x += skx;
        remaining = match.suffix().str();
    }
    
    // skewY(y)
    std::regex skewY_regex(R"(skewY\s*\(\s*([-+]?[\d.]+)(deg|rad)?\s*\))");
    remaining = transform_str;
    while (std::regex_search(remaining, match, skewY_regex)) {
        float sky = std::stof(match[1].str());
        std::string unit = match[2].matched ? match[2].str() : "deg";
        if (unit == "deg") {
            sky = sky * 3.14159265358979f / 180.0f;
        }
        result.skew_y += sky;
        remaining = match.suffix().str();
    }
    
    return result;
}

PropertyInterpolation::DecomposedTransform 
PropertyInterpolation::InterpolateDecomposed(
    const DecomposedTransform& from,
    const DecomposedTransform& to,
    float factor) {
    
    DecomposedTransform result;
    
    // 线性插值各个组件
    result.translate_x = from.translate_x + (to.translate_x - from.translate_x) * factor;
    result.translate_y = from.translate_y + (to.translate_y - from.translate_y) * factor;
    result.scale_x = from.scale_x + (to.scale_x - from.scale_x) * factor;
    result.scale_y = from.scale_y + (to.scale_y - from.scale_y) * factor;
    result.skew_x = from.skew_x + (to.skew_x - from.skew_x) * factor;
    result.skew_y = from.skew_y + (to.skew_y - from.skew_y) * factor;
    
    // 角度插值：直接线性插值，不做最短路径优化
    // 这样 0deg -> 360deg 会正确地旋转一整圈
    // 如果需要最短路径，应该在 CSS 中使用 0deg -> 0deg 或其他方式
    result.rotate = from.rotate + (to.rotate - from.rotate) * factor;
    
    return result;
}

std::string PropertyInterpolation::ComposeTransform(const DecomposedTransform& decomposed) {
    std::ostringstream oss;
    bool has_transform = false;
    
    // 按照标准顺序输出: translate -> rotate -> scale -> skew
    // 注意：对于动画，我们需要输出所有非默认值，即使很小
    // 使用更小的阈值来避免浮点误差
    const float EPSILON = 0.0001f;
    
    // translate
    if (std::abs(decomposed.translate_x) > EPSILON || std::abs(decomposed.translate_y) > EPSILON) {
        if (has_transform) oss << " ";
        oss << "translate(" << decomposed.translate_x << "px, " << decomposed.translate_y << "px)";
        has_transform = true;
    }
    
    // rotate - 对于旋转动画，即使是 0 度也需要输出（因为动画需要从 0 开始）
    // 只有当 rotate 完全为 0 且没有其他变换时才返回 none
    if (std::abs(decomposed.rotate) > EPSILON) {
        if (has_transform) oss << " ";
        // 转换为度数输出
        float degrees = decomposed.rotate * 180.0f / 3.14159265358979f;
        oss << "rotate(" << degrees << "deg)";
        has_transform = true;
    }
    
    // scale
    if (std::abs(decomposed.scale_x - 1.0f) > EPSILON || std::abs(decomposed.scale_y - 1.0f) > EPSILON) {
        if (has_transform) oss << " ";
        if (std::abs(decomposed.scale_x - decomposed.scale_y) < EPSILON) {
            oss << "scale(" << decomposed.scale_x << ")";
        } else {
            oss << "scale(" << decomposed.scale_x << ", " << decomposed.scale_y << ")";
        }
        has_transform = true;
    }
    
    // skew
    if (std::abs(decomposed.skew_x) > EPSILON || std::abs(decomposed.skew_y) > EPSILON) {
        if (has_transform) oss << " ";
        float skew_x_deg = decomposed.skew_x * 180.0f / 3.14159265358979f;
        float skew_y_deg = decomposed.skew_y * 180.0f / 3.14159265358979f;
        if (std::abs(decomposed.skew_y) < EPSILON) {
            oss << "skewX(" << skew_x_deg << "deg)";
        } else if (std::abs(decomposed.skew_x) < EPSILON) {
            oss << "skewY(" << skew_y_deg << "deg)";
        } else {
            oss << "skew(" << skew_x_deg << "deg, " << skew_y_deg << "deg)";
        }
        has_transform = true;
    }
    
    // 如果没有任何变换，返回 "none"
    if (!has_transform) {
        return "none";
    }
    
    return oss.str();
}

std::optional<std::string> PropertyInterpolation::InterpolateTransform(
    const std::string& from,
    const std::string& to,
    float factor) {
    
    // 分解两个 transform
    auto from_decomposed = DecomposeTransform(from);
    auto to_decomposed = DecomposeTransform(to);
    
    // 如果任一分解失败，使用阶跃函数
    if (!from_decomposed || !to_decomposed) {
        return factor < 0.5f ? from : to;
    }
    
    // 插值
    auto interpolated = InterpolateDecomposed(*from_decomposed, *to_decomposed, factor);
    
    // 重组
    return ComposeTransform(interpolated);
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

} // namespace mblink

