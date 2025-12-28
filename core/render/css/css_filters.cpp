/**
 * @file css_filters.cpp
 * @brief CSS 滤镜实现
 */

#include "css_filters.h"
#include "core/render/utils/color.h"
#include "include/effects/SkImageFilters.h"
#include "include/effects/SkColorMatrix.h"
#include "include/core/SkColorFilter.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace lightui {

// ========== CSSFilter 静态工厂方法 ==========

CSSFilter CSSFilter::Blur(float radius) {
    CSSFilter filter(CSSFilterType::Blur);
    filter.value = radius;
    return filter;
}

CSSFilter CSSFilter::Brightness(float amount) {
    CSSFilter filter(CSSFilterType::Brightness);
    filter.amount = amount;
    return filter;
}

CSSFilter CSSFilter::Contrast(float amount) {
    CSSFilter filter(CSSFilterType::Contrast);
    filter.amount = amount;
    return filter;
}

CSSFilter CSSFilter::Grayscale(float amount) {
    CSSFilter filter(CSSFilterType::Grayscale);
    filter.amount = std::clamp(amount, 0.0f, 1.0f);
    return filter;
}

CSSFilter CSSFilter::Sepia(float amount) {
    CSSFilter filter(CSSFilterType::Sepia);
    filter.amount = std::clamp(amount, 0.0f, 1.0f);
    return filter;
}

CSSFilter CSSFilter::Saturate(float amount) {
    CSSFilter filter(CSSFilterType::Saturate);
    filter.amount = amount;
    return filter;
}

CSSFilter CSSFilter::HueRotate(float angle) {
    CSSFilter filter(CSSFilterType::HueRotate);
    filter.angle = angle;
    return filter;
}

CSSFilter CSSFilter::Invert(float amount) {
    CSSFilter filter(CSSFilterType::Invert);
    filter.amount = std::clamp(amount, 0.0f, 1.0f);
    return filter;
}

CSSFilter CSSFilter::Opacity(float amount) {
    CSSFilter filter(CSSFilterType::Opacity);
    filter.amount = std::clamp(amount, 0.0f, 1.0f);
    return filter;
}

CSSFilter CSSFilter::DropShadow(float offset_x, float offset_y, float blur_radius, uint32_t color) {
    CSSFilter filter(CSSFilterType::DropShadow);
    filter.offset_x = offset_x;
    filter.offset_y = offset_y;
    filter.blur_radius = blur_radius;
    filter.color = color;
    return filter;
}

// ========== CSSFilterList 实现 ==========

void CSSFilterList::AddFilter(const CSSFilter& filter) {
    filters_.push_back(filter);
}

void CSSFilterList::Clear() {
    filters_.clear();
}

sk_sp<SkImageFilter> CSSFilterList::CreateSkiaFilter() const {
    if (filters_.empty()) {
        return nullptr;
    }
    
    // 从后向前构建滤镜链
    sk_sp<SkImageFilter> result = nullptr;
    for (auto it = filters_.rbegin(); it != filters_.rend(); ++it) {
        result = CSSFilterRenderer::CreateSkiaFilter(*it, result);
    }
    
    return result;
}

void CSSFilterList::ApplyToPaint(SkPaint& paint) const {
    sk_sp<SkImageFilter> filter = CreateSkiaFilter();
    if (filter) {
        paint.setImageFilter(filter);
    }
}

// ========== CSSFilterRenderer 实现 ==========

sk_sp<SkImageFilter> CSSFilterRenderer::CreateSkiaFilter(const CSSFilter& filter, 
                                                         sk_sp<SkImageFilter> input) {
    switch (filter.type) {
        case CSSFilterType::Blur: {
            float sigma = filter.value / 2.0f;
            return SkImageFilters::Blur(sigma, sigma, input);
        }
        
        case CSSFilterType::Brightness: {
            // 亮度调整：使用颜色矩阵
            float matrix[20] = {
                filter.amount, 0, 0, 0, 0,
                0, filter.amount, 0, 0, 0,
                0, 0, filter.amount, 0, 0,
                0, 0, 0, 1, 0
            };
            return CreateColorMatrixFilter(matrix, input);
        }
        
        case CSSFilterType::Contrast: {
            // 对比度调整：使用颜色矩阵
            float translate = (1.0f - filter.amount) / 2.0f;
            float matrix[20] = {
                filter.amount, 0, 0, 0, translate,
                0, filter.amount, 0, 0, translate,
                0, 0, filter.amount, 0, translate,
                0, 0, 0, 1, 0
            };
            return CreateColorMatrixFilter(matrix, input);
        }
        
        case CSSFilterType::Grayscale: {
            // 灰度：使用标准的灰度转换矩阵
            float r = 0.2126f, g = 0.7152f, b = 0.0722f;
            float inv = 1.0f - filter.amount;
            float matrix[20] = {
                r * filter.amount + inv, g * filter.amount, b * filter.amount, 0, 0,
                r * filter.amount, g * filter.amount + inv, b * filter.amount, 0, 0,
                r * filter.amount, g * filter.amount, b * filter.amount + inv, 0, 0,
                0, 0, 0, 1, 0
            };
            return CreateColorMatrixFilter(matrix, input);
        }
        
        case CSSFilterType::Sepia: {
            // 棕褐色：使用标准的 sepia 矩阵
            float inv = 1.0f - filter.amount;
            float matrix[20] = {
                0.393f * filter.amount + inv, 0.769f * filter.amount, 0.189f * filter.amount, 0, 0,
                0.349f * filter.amount, 0.686f * filter.amount + inv, 0.168f * filter.amount, 0, 0,
                0.272f * filter.amount, 0.534f * filter.amount, 0.131f * filter.amount + inv, 0, 0,
                0, 0, 0, 1, 0
            };
            return CreateColorMatrixFilter(matrix, input);
        }
        
        case CSSFilterType::Saturate: {
            // 饱和度调整
            float s = filter.amount;
            float r = 0.2126f, g = 0.7152f, b = 0.0722f;
            float matrix[20] = {
                r * (1 - s) + s, g * (1 - s), b * (1 - s), 0, 0,
                r * (1 - s), g * (1 - s) + s, b * (1 - s), 0, 0,
                r * (1 - s), g * (1 - s), b * (1 - s) + s, 0, 0,
                0, 0, 0, 1, 0
            };
            return CreateColorMatrixFilter(matrix, input);
        }
        
        case CSSFilterType::HueRotate: {
            // 色相旋转：使用旋转矩阵
            float angle_rad = filter.angle * M_PI / 180.0f;
            float cos_a = std::cos(angle_rad);
            float sin_a = std::sin(angle_rad);
            
            float matrix[20] = {
                0.213f + cos_a * 0.787f - sin_a * 0.213f,
                0.715f - cos_a * 0.715f - sin_a * 0.715f,
                0.072f - cos_a * 0.072f + sin_a * 0.928f,
                0, 0,
                
                0.213f - cos_a * 0.213f + sin_a * 0.143f,
                0.715f + cos_a * 0.285f + sin_a * 0.140f,
                0.072f - cos_a * 0.072f - sin_a * 0.283f,
                0, 0,
                
                0.213f - cos_a * 0.213f - sin_a * 0.787f,
                0.715f - cos_a * 0.715f + sin_a * 0.715f,
                0.072f + cos_a * 0.928f + sin_a * 0.072f,
                0, 0,
                
                0, 0, 0, 1, 0
            };
            return CreateColorMatrixFilter(matrix, input);
        }
        
        case CSSFilterType::Invert: {
            // 反转：使用颜色矩阵
            float inv = 1.0f - 2.0f * filter.amount;
            float offset = filter.amount;
            float matrix[20] = {
                inv, 0, 0, 0, offset,
                0, inv, 0, 0, offset,
                0, 0, inv, 0, offset,
                0, 0, 0, 1, 0
            };
            return CreateColorMatrixFilter(matrix, input);
        }
        
        case CSSFilterType::Opacity: {
            // 不透明度：使用颜色矩阵
            float matrix[20] = {
                1, 0, 0, 0, 0,
                0, 1, 0, 0, 0,
                0, 0, 1, 0, 0,
                0, 0, 0, filter.amount, 0
            };
            return CreateColorMatrixFilter(matrix, input);
        }
        
        case CSSFilterType::DropShadow: {
            // 投影：使用 drop shadow 滤镜
            float sigma = filter.blur_radius / 2.0f;
            return SkImageFilters::DropShadow(
                filter.offset_x, filter.offset_y,
                sigma, sigma,
                filter.color,
                input
            );
        }
        
        default:
            return input;
    }
}

sk_sp<SkImageFilter> CSSFilterRenderer::CreateColorMatrixFilter(const float matrix[20],
                                                                sk_sp<SkImageFilter> input) {
    // 创建 SkColorMatrix 对象
    SkColorMatrix color_matrix;
    color_matrix.setRowMajor(matrix);

    // 创建颜色滤镜
    sk_sp<SkColorFilter> color_filter = SkColorFilters::Matrix(color_matrix);

    // 将颜色滤镜转换为图像滤镜
    return SkImageFilters::ColorFilter(color_filter, input);
}

// ========== CSSFilterParser 实现 ==========

std::optional<CSSFilterList> CSSFilterParser::Parse(const std::string& value) {
    if (value.empty() || value == "none") {
        return std::nullopt;
    }
    
    CSSFilterList filter_list;
    
    // 分割多个滤镜函数
    std::vector<std::string> filter_funcs = SplitFilters(value);
    
    for (const auto& func : filter_funcs) {
        auto filter = ParseSingleFilter(func);
        if (filter.has_value()) {
            filter_list.AddFilter(filter.value());
        }
    }
    
    if (filter_list.IsEmpty()) {
        return std::nullopt;
    }
    
    return filter_list;
}

std::optional<CSSFilter> CSSFilterParser::ParseSingleFilter(const std::string& filter_func) {
    auto func_and_args = ExtractFunctionAndArgs(filter_func);
    if (!func_and_args.has_value()) {
        return std::nullopt;
    }
    
    std::string func_name = func_and_args->first;
    std::string args = func_and_args->second;
    
    // 转换为小写
    std::transform(func_name.begin(), func_name.end(), func_name.begin(), ::tolower);
    
    if (func_name == "blur") {
        float radius = ParseLength(args);
        return CSSFilter::Blur(radius);
    }
    else if (func_name == "brightness") {
        float amount = ParseNumberOrPercentage(args);
        return CSSFilter::Brightness(amount);
    }
    else if (func_name == "contrast") {
        float amount = ParseNumberOrPercentage(args);
        return CSSFilter::Contrast(amount);
    }
    else if (func_name == "grayscale") {
        float amount = ParseNumberOrPercentage(args);
        return CSSFilter::Grayscale(amount);
    }
    else if (func_name == "sepia") {
        float amount = ParseNumberOrPercentage(args);
        return CSSFilter::Sepia(amount);
    }
    else if (func_name == "saturate") {
        float amount = ParseNumberOrPercentage(args);
        return CSSFilter::Saturate(amount);
    }
    else if (func_name == "hue-rotate") {
        float angle = ParseAngle(args);
        return CSSFilter::HueRotate(angle);
    }
    else if (func_name == "invert") {
        float amount = ParseNumberOrPercentage(args);
        return CSSFilter::Invert(amount);
    }
    else if (func_name == "opacity") {
        float amount = ParseNumberOrPercentage(args);
        return CSSFilter::Opacity(amount);
    }
    
    return std::nullopt;
}

std::optional<std::pair<std::string, std::string>> CSSFilterParser::ExtractFunctionAndArgs(
    const std::string& filter_func) {
    size_t paren_pos = filter_func.find('(');
    if (paren_pos == std::string::npos) {
        return std::nullopt;
    }
    
    std::string func_name = Trim(filter_func.substr(0, paren_pos));
    
    size_t close_paren = filter_func.rfind(')');
    if (close_paren == std::string::npos || close_paren <= paren_pos) {
        return std::nullopt;
    }
    
    std::string args = Trim(filter_func.substr(paren_pos + 1, close_paren - paren_pos - 1));
    
    return std::make_pair(func_name, args);
}

float CSSFilterParser::ParseLength(const std::string& value) {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return 0.0f;
    }
    
    // 查找单位
    size_t unit_pos = trimmed.length();
    for (size_t i = 0; i < trimmed.length(); ++i) {
        if (!std::isdigit(trimmed[i]) && trimmed[i] != '.' && trimmed[i] != '-') {
            unit_pos = i;
            break;
        }
    }
    
    float num = std::stof(trimmed.substr(0, unit_pos));
    
    if (unit_pos < trimmed.length()) {
        std::string unit = trimmed.substr(unit_pos);
        // 目前只支持 px，其他单位暂时按 px 处理
        // TODO: 支持 em, rem, % 等单位
    }
    
    return num;
}

float CSSFilterParser::ParseAngle(const std::string& value) {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return 0.0f;
    }
    
    // 查找单位
    size_t unit_pos = trimmed.length();
    for (size_t i = 0; i < trimmed.length(); ++i) {
        if (!std::isdigit(trimmed[i]) && trimmed[i] != '.' && trimmed[i] != '-') {
            unit_pos = i;
            break;
        }
    }
    
    float num = std::stof(trimmed.substr(0, unit_pos));
    
    if (unit_pos < trimmed.length()) {
        std::string unit = trimmed.substr(unit_pos);
        std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
        
        if (unit == "rad") {
            // 弧度转角度
            num = num * 180.0f / M_PI;
        }
        else if (unit == "grad") {
            // 梯度转角度
            num = num * 360.0f / 400.0f;
        }
        else if (unit == "turn") {
            // 圈数转角度
            num = num * 360.0f;
        }
        // "deg" 或无单位默认为角度
    }
    
    return num;
}

float CSSFilterParser::ParseNumberOrPercentage(const std::string& value) {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return 1.0f; // 默认值
    }
    
    if (trimmed.back() == '%') {
        // 百分比
        float percent = std::stof(trimmed.substr(0, trimmed.length() - 1));
        return percent / 100.0f;
    }
    else {
        // 数字
        return std::stof(trimmed);
    }
}

std::string CSSFilterParser::Trim(const std::string& str) {
    size_t start = 0;
    while (start < str.length() && std::isspace(str[start])) {
        start++;
    }
    
    size_t end = str.length();
    while (end > start && std::isspace(str[end - 1])) {
        end--;
    }
    
    return str.substr(start, end - start);
}

std::vector<std::string> CSSFilterParser::SplitFilters(const std::string& value) {
    std::vector<std::string> result;
    std::string current;
    int paren_depth = 0;
    
    for (char c : value) {
        if (c == '(') {
            paren_depth++;
            current += c;
        }
        else if (c == ')') {
            paren_depth--;
            current += c;
            
            // 如果括号闭合，这是一个完整的滤镜函数
            if (paren_depth == 0 && !current.empty()) {
                result.push_back(Trim(current));
                current.clear();
            }
        }
        else if (std::isspace(c) && paren_depth == 0) {
            // 括号外的空格作为分隔符
            if (!current.empty()) {
                result.push_back(Trim(current));
                current.clear();
            }
        }
        else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        result.push_back(Trim(current));
    }
    
    return result;
}

} // namespace lightui

