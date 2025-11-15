/**
 * @file transform.cpp
 * @brief CSS Transform 实现
 */

#include "transform.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace lightui {

// ========== TransformOrigin 实现 ==========

SkPoint TransformOrigin::ToPoint(const SkRect& rect) const {
    float px = x.ToPx(rect.width(), 16.0f);
    float py = y.ToPx(rect.height(), 16.0f);
    
    // 如果是百分比，相对于元素的左上角
    if (x.unit == CSSUnit::PERCENT) {
        px = rect.left() + rect.width() * (x.value / 100.0f);
    } else {
        px = rect.left() + px;
    }
    
    if (y.unit == CSSUnit::PERCENT) {
        py = rect.top() + rect.height() * (y.value / 100.0f);
    } else {
        py = rect.top() + py;
    }
    
    return SkPoint::Make(px, py);
}

// ========== CSSTransform 实现 ==========

std::optional<CSSTransform> CSSTransform::Parse(const std::string& str) {
    std::string trimmed = CSSValue::Trim(str);
    if (trimmed.empty() || trimmed == "none") {
        return CSSTransform();  // 空变换
    }
    
    CSSTransform result;
    
    // 解析多个 transform 函数
    size_t pos = 0;
    while (pos < trimmed.length()) {
        // 跳过空格
        while (pos < trimmed.length() && std::isspace(trimmed[pos])) {
            pos++;
        }
        
        if (pos >= trimmed.length()) break;
        
        // 查找函数名
        size_t func_start = pos;
        while (pos < trimmed.length() && trimmed[pos] != '(') {
            pos++;
        }
        
        if (pos >= trimmed.length()) break;
        
        std::string func_name = trimmed.substr(func_start, pos - func_start);
        func_name = CSSValue::Trim(func_name);
        
        // 查找参数（括号内的内容）
        pos++;  // 跳过 '('
        size_t args_start = pos;
        int paren_count = 1;
        while (pos < trimmed.length() && paren_count > 0) {
            if (trimmed[pos] == '(') paren_count++;
            else if (trimmed[pos] == ')') paren_count--;
            pos++;
        }
        
        if (paren_count != 0) {
            return std::nullopt;  // 括号不匹配
        }
        
        std::string args = trimmed.substr(args_start, pos - args_start - 1);
        
        // 解析函数
        auto transform = ParseFunction(func_name, args);
        if (!transform.has_value()) {
            return std::nullopt;
        }
        
        result.AddTransform(*transform);
    }
    
    return result;
}

std::optional<Transform> CSSTransform::ParseFunction(const std::string& func_name, 
                                                     const std::string& args) {
    if (func_name == "translate" || func_name == "translateX" || func_name == "translateY") {
        return ParseTranslate(args);
    } else if (func_name == "rotate") {
        return ParseRotate(args);
    } else if (func_name == "scale" || func_name == "scaleX" || func_name == "scaleY") {
        return ParseScale(args);
    } else if (func_name == "skew" || func_name == "skewX" || func_name == "skewY") {
        return ParseSkew(args);
    } else if (func_name == "matrix") {
        return ParseMatrix(args);
    }
    
    return std::nullopt;  // 未知函数
}

std::optional<Transform> CSSTransform::ParseTranslate(const std::string& args) {
    auto parts = CSSValue::Split(args, ',');
    
    if (parts.empty() || parts.size() > 2) {
        return std::nullopt;
    }
    
    Transform transform(TransformType::TRANSLATE, {});
    
    // 解析 X 值
    auto x_length = CSSValue::ParseLength(parts[0]);
    transform.values.push_back(x_length.ToPx(0.0f, 16.0f));
    
    // 解析 Y 值（如果有）
    if (parts.size() == 2) {
        auto y_length = CSSValue::ParseLength(parts[1]);
        transform.values.push_back(y_length.ToPx(0.0f, 16.0f));
    } else {
        transform.values.push_back(0.0f);  // 默认 Y = 0
    }
    
    return transform;
}

std::optional<Transform> CSSTransform::ParseRotate(const std::string& args) {
    std::string trimmed = CSSValue::Trim(args);
    
    // 解析角度
    float angle = 0.0f;
    if (trimmed.find("deg") != std::string::npos) {
        angle = CSSValue::ParseFloat(trimmed);
    } else if (trimmed.find("rad") != std::string::npos) {
        float rad = CSSValue::ParseFloat(trimmed);
        angle = rad * 180.0f / M_PI;  // 转换为度
    } else if (trimmed.find("turn") != std::string::npos) {
        float turn = CSSValue::ParseFloat(trimmed);
        angle = turn * 360.0f;  // 转换为度
    } else {
        // 默认为度
        angle = CSSValue::ParseFloat(trimmed);
    }
    
    return Transform(TransformType::ROTATE, {angle});
}

std::optional<Transform> CSSTransform::ParseScale(const std::string& args) {
    auto parts = CSSValue::Split(args, ',');
    
    if (parts.empty() || parts.size() > 2) {
        return std::nullopt;
    }
    
    Transform transform(TransformType::SCALE, {});
    
    // 解析 X 缩放
    float scale_x = CSSValue::ParseFloat(parts[0]);
    transform.values.push_back(scale_x);
    
    // 解析 Y 缩放（如果有）
    if (parts.size() == 2) {
        float scale_y = CSSValue::ParseFloat(parts[1]);
        transform.values.push_back(scale_y);
    } else {
        transform.values.push_back(scale_x);  // 默认 Y = X（等比缩放）
    }
    
    return transform;
}

std::optional<Transform> CSSTransform::ParseSkew(const std::string& args) {
    auto parts = CSSValue::Split(args, ',');
    
    if (parts.empty() || parts.size() > 2) {
        return std::nullopt;
    }
    
    Transform transform(TransformType::SKEW, {});
    
    // 解析 X 倾斜角度
    float skew_x = CSSValue::ParseFloat(parts[0]);
    transform.values.push_back(skew_x);
    
    // 解析 Y 倾斜角度（如果有）
    if (parts.size() == 2) {
        float skew_y = CSSValue::ParseFloat(parts[1]);
        transform.values.push_back(skew_y);
    } else {
        transform.values.push_back(0.0f);  // 默认 Y = 0
    }
    
    return transform;
}

std::optional<Transform> CSSTransform::ParseMatrix(const std::string& args) {
    auto parts = CSSValue::Split(args, ',');
    
    if (parts.size() != 6) {
        return std::nullopt;  // matrix 需要 6 个参数
    }
    
    Transform transform(TransformType::MATRIX, {});
    
    for (const auto& part : parts) {
        float value = CSSValue::ParseFloat(part);
        transform.values.push_back(value);
    }
    
    return transform;
}

SkMatrix CSSTransform::ToSkMatrix(const SkRect& rect, const TransformOrigin& origin) const {
    SkMatrix result;
    result.reset();  // 单位矩阵

    if (transforms.empty()) {
        return result;
    }

    // 获取变换原点
    SkPoint origin_point = origin.ToPoint(rect);

    // 1. 平移到原点
    SkMatrix translate_to_origin;
    translate_to_origin.setTranslate(-origin_point.x(), -origin_point.y());
    result.postConcat(translate_to_origin);

    // 2. 应用所有变换（按顺序）
    for (const auto& transform : transforms) {
        SkMatrix t;
        t.reset();

        switch (transform.type) {
            case TransformType::TRANSLATE:
                if (transform.values.size() >= 2) {
                    t.setTranslate(transform.values[0], transform.values[1]);
                }
                break;

            case TransformType::ROTATE:
                if (!transform.values.empty()) {
                    t.setRotate(transform.values[0]);  // Skia 使用度数
                }
                break;

            case TransformType::SCALE:
                if (transform.values.size() >= 2) {
                    t.setScale(transform.values[0], transform.values[1]);
                }
                break;

            case TransformType::SKEW:
                if (transform.values.size() >= 2) {
                    float skew_x_rad = transform.values[0] * M_PI / 180.0f;
                    float skew_y_rad = transform.values[1] * M_PI / 180.0f;
                    t.setSkew(std::tan(skew_x_rad), std::tan(skew_y_rad));
                }
                break;

            case TransformType::MATRIX:
                if (transform.values.size() >= 6) {
                    t.setAll(
                        transform.values[0], transform.values[2], transform.values[4],
                        transform.values[1], transform.values[3], transform.values[5],
                        0, 0, 1
                    );
                }
                break;
        }

        result.postConcat(t);
    }

    // 3. 平移回原点
    SkMatrix translate_back;
    translate_back.setTranslate(origin_point.x(), origin_point.y());
    result.postConcat(translate_back);

    return result;
}

// ========== ParseTransformOrigin 实现 ==========

std::optional<TransformOrigin> ParseTransformOrigin(const std::string& str) {
    std::string trimmed = CSSValue::Trim(str);
    if (trimmed.empty()) {
        return TransformOrigin();  // 默认 center center
    }
    
    auto parts = CSSValue::Split(trimmed, ' ');
    
    if (parts.empty() || parts.size() > 2) {
        return std::nullopt;
    }
    
    TransformOrigin origin;
    
    // 解析 X 值
    std::string x_str = parts[0];
    if (x_str == "left") {
        origin.x = CSSLength(0.0f, CSSUnit::PERCENT);
    } else if (x_str == "center") {
        origin.x = CSSLength(50.0f, CSSUnit::PERCENT);
    } else if (x_str == "right") {
        origin.x = CSSLength(100.0f, CSSUnit::PERCENT);
    } else {
        origin.x = CSSValue::ParseLength(x_str);
    }
    
    // 解析 Y 值
    if (parts.size() == 2) {
        std::string y_str = parts[1];
        if (y_str == "top") {
            origin.y = CSSLength(0.0f, CSSUnit::PERCENT);
        } else if (y_str == "center") {
            origin.y = CSSLength(50.0f, CSSUnit::PERCENT);
        } else if (y_str == "bottom") {
            origin.y = CSSLength(100.0f, CSSUnit::PERCENT);
        } else {
            origin.y = CSSValue::ParseLength(y_str);
        }
    } else {
        origin.y = CSSLength(50.0f, CSSUnit::PERCENT);  // 默认 center
    }
    
    return origin;
}

} // namespace lightui

