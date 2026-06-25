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

namespace mblink {

// ========== TransformOrigin 实现 ==========

SkPoint TransformOrigin::ToPoint(const SkRect& rect) const {
    float px = x.ToPx(rect.width(), 16.0f);
    float py = y.ToPx(rect.height(), 16.0f);
    
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
        return CSSTransform();
    }
    
    CSSTransform result;
    size_t pos = 0;
    
    while (pos < trimmed.length()) {
        while (pos < trimmed.length() && std::isspace(trimmed[pos])) pos++;
        if (pos >= trimmed.length()) break;
        
        size_t func_start = pos;
        while (pos < trimmed.length() && trimmed[pos] != '(') pos++;
        if (pos >= trimmed.length()) break;
        
        std::string func_name = trimmed.substr(func_start, pos - func_start);
        func_name = CSSValue::Trim(func_name);
        
        pos++;
        size_t args_start = pos;
        int paren_count = 1;
        while (pos < trimmed.length() && paren_count > 0) {
            if (trimmed[pos] == '(') paren_count++;
            else if (trimmed[pos] == ')') paren_count--;
            pos++;
        }
        
        if (paren_count != 0) return std::nullopt;
        
        std::string args = trimmed.substr(args_start, pos - args_start - 1);
        auto transform = ParseFunction(func_name, args);
        if (!transform.has_value()) return std::nullopt;
        
        result.AddTransform(*transform);
    }
    
    return result;
}

std::optional<Transform> CSSTransform::ParseFunction(const std::string& func_name, 
                                                     const std::string& args) {
    if (func_name == "translate") return ParseTranslate(args);
    else if (func_name == "translateX") return ParseTranslateX(args);
    else if (func_name == "translateY") return ParseTranslateY(args);
    else if (func_name == "rotate") return ParseRotate(args);
    else if (func_name == "scale" || func_name == "scaleX" || func_name == "scaleY")
        return ParseScale(args);
    else if (func_name == "skew" || func_name == "skewX" || func_name == "skewY")
        return ParseSkew(args);
    else if (func_name == "matrix") return ParseMatrix(args);
    return std::nullopt;
}

std::optional<Transform> CSSTransform::ParseTranslateX(const std::string& args) {
    Transform transform(TransformType::TRANSLATE);
    transform.lengths.push_back(CSSValue::ParseLength(args));
    transform.lengths.push_back(CSSLength(0.0f, CSSUnit::PX));
    return transform;
}

std::optional<Transform> CSSTransform::ParseTranslateY(const std::string& args) {
    Transform transform(TransformType::TRANSLATE);
    transform.lengths.push_back(CSSLength(0.0f, CSSUnit::PX));
    transform.lengths.push_back(CSSValue::ParseLength(args));
    return transform;
}

std::optional<Transform> CSSTransform::ParseTranslate(const std::string& args) {
    auto parts = CSSValue::Split(args, ',');
    if (parts.empty() || parts.size() > 2) return std::nullopt;

    Transform transform(TransformType::TRANSLATE);
    transform.lengths.push_back(CSSValue::ParseLength(parts[0]));
    if (parts.size() == 2) {
        transform.lengths.push_back(CSSValue::ParseLength(parts[1]));
    } else {
        transform.lengths.push_back(CSSLength(0.0f, CSSUnit::PX));
    }
    return transform;
}

std::optional<Transform> CSSTransform::ParseRotate(const std::string& args) {
    std::string trimmed = CSSValue::Trim(args);
    float angle = 0.0f;
    if (trimmed.find("deg") != std::string::npos) {
        angle = CSSValue::ParseFloat(trimmed);
    } else if (trimmed.find("rad") != std::string::npos) {
        angle = CSSValue::ParseFloat(trimmed) * 180.0f / M_PI;
    } else if (trimmed.find("turn") != std::string::npos) {
        angle = CSSValue::ParseFloat(trimmed) * 360.0f;
    } else {
        angle = CSSValue::ParseFloat(trimmed);
    }
    return Transform(TransformType::ROTATE, std::vector<float>{angle});
}

std::optional<Transform> CSSTransform::ParseScale(const std::string& args) {
    auto parts = CSSValue::Split(args, ',');
    if (parts.empty() || parts.size() > 2) return std::nullopt;

    Transform transform(TransformType::SCALE);
    float scale_x = CSSValue::ParseFloat(parts[0]);
    transform.values.push_back(scale_x);
    transform.values.push_back(parts.size() == 2 ? CSSValue::ParseFloat(parts[1]) : scale_x);
    return transform;
}

std::optional<Transform> CSSTransform::ParseSkew(const std::string& args) {
    auto parts = CSSValue::Split(args, ',');
    if (parts.empty() || parts.size() > 2) return std::nullopt;

    Transform transform(TransformType::SKEW);
    transform.values.push_back(CSSValue::ParseFloat(parts[0]));
    transform.values.push_back(parts.size() == 2 ? CSSValue::ParseFloat(parts[1]) : 0.0f);
    return transform;
}

std::optional<Transform> CSSTransform::ParseMatrix(const std::string& args) {
    auto parts = CSSValue::Split(args, ',');
    if (parts.size() != 6) return std::nullopt;

    Transform transform(TransformType::MATRIX);
    for (const auto& part : parts) {
        transform.values.push_back(CSSValue::ParseFloat(part));
    }
    return transform;
}

SkMatrix CSSTransform::ToSkMatrix(const SkRect& rect, const TransformOrigin& origin) const {
    SkMatrix result;
    result.reset();
    if (transforms.empty()) return result;

    SkPoint origin_point = origin.ToPoint(rect);
    SkMatrix translate_to_origin;
    translate_to_origin.setTranslate(-origin_point.x(), -origin_point.y());
    result.postConcat(translate_to_origin);

    for (const auto& transform : transforms) {
        SkMatrix t;
        t.reset();

        switch (transform.type) {
            case TransformType::TRANSLATE:
                if (transform.lengths.size() >= 2) {
                    float tx = 0.0f, ty = 0.0f;
                    const auto& x_len = transform.lengths[0];
                    const auto& y_len = transform.lengths[1];
                    tx = (x_len.unit == CSSUnit::PERCENT) ? 
                         rect.width() * (x_len.value / 100.0f) : x_len.ToPx(rect.width(), 16.0f);
                    ty = (y_len.unit == CSSUnit::PERCENT) ? 
                         rect.height() * (y_len.value / 100.0f) : y_len.ToPx(rect.height(), 16.0f);
                    t.setTranslate(tx, ty);
                }
                break;
            case TransformType::ROTATE:
                if (!transform.values.empty()) t.setRotate(transform.values[0]);
                break;
            case TransformType::SCALE:
                if (transform.values.size() >= 2)
                    t.setScale(transform.values[0], transform.values[1]);
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
                    t.setAll(transform.values[0], transform.values[2], transform.values[4],
                             transform.values[1], transform.values[3], transform.values[5], 0, 0, 1);
                }
                break;
        }
        result.postConcat(t);
    }

    SkMatrix translate_back;
    translate_back.setTranslate(origin_point.x(), origin_point.y());
    result.postConcat(translate_back);
    return result;
}

std::optional<TransformOrigin> ParseTransformOrigin(const std::string& str) {
    std::string trimmed = CSSValue::Trim(str);
    if (trimmed.empty()) return TransformOrigin();
    
    auto parts = CSSValue::Split(trimmed, ' ');
    if (parts.empty() || parts.size() > 2) return std::nullopt;
    
    TransformOrigin origin;
    std::string x_str = parts[0];
    if (x_str == "left") origin.x = CSSLength(0.0f, CSSUnit::PERCENT);
    else if (x_str == "center") origin.x = CSSLength(50.0f, CSSUnit::PERCENT);
    else if (x_str == "right") origin.x = CSSLength(100.0f, CSSUnit::PERCENT);
    else origin.x = CSSValue::ParseLength(x_str);
    
    if (parts.size() == 2) {
        std::string y_str = parts[1];
        if (y_str == "top") origin.y = CSSLength(0.0f, CSSUnit::PERCENT);
        else if (y_str == "center") origin.y = CSSLength(50.0f, CSSUnit::PERCENT);
        else if (y_str == "bottom") origin.y = CSSLength(100.0f, CSSUnit::PERCENT);
        else origin.y = CSSValue::ParseLength(y_str);
    } else {
        origin.y = CSSLength(50.0f, CSSUnit::PERCENT);
    }
    
    return origin;
}

} // namespace mblink
