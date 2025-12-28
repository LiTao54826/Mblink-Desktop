/**
 * @file transform.h
 * @brief CSS Transform 数据结构和解析
 */

#ifndef LIGHTUI_RENDER_UTILS_TRANSFORM_H
#define LIGHTUI_RENDER_UTILS_TRANSFORM_H

#include "css/css_value.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkRect.h"
#include "include/core/SkPoint.h"
#include <vector>
#include <string>
#include <optional>

namespace lightui {

/**
 * @brief Transform 类型枚举
 */
enum class TransformType {
    TRANSLATE,    // translate(x, y) 或 translateX(x) 或 translateY(y)
    ROTATE,       // rotate(angle)
    SCALE,        // scale(x, y) 或 scaleX(x) 或 scaleY(y)
    SKEW,         // skew(x, y) 或 skewX(x) 或 skewY(y)
    MATRIX        // matrix(a, b, c, d, e, f)
};

/**
 * @brief 单个 Transform 操作
 */
struct Transform {
    TransformType type;
    std::vector<float> values;
    std::vector<CSSLength> lengths;

    Transform() : type(TransformType::TRANSLATE) {}
    Transform(TransformType t) : type(t) {}
    Transform(TransformType t, const std::vector<float>& v) : type(t), values(v) {}
    Transform(TransformType t, const std::vector<CSSLength>& l) : type(t), lengths(l) {}
};

/**
 * @brief Transform Origin (变换原点)
 */
struct TransformOrigin {
    CSSLength x;
    CSSLength y;
    
    TransformOrigin() 
        : x(50.0f, CSSUnit::PERCENT), 
          y(50.0f, CSSUnit::PERCENT) {}
    
    TransformOrigin(const CSSLength& x_val, const CSSLength& y_val) 
        : x(x_val), y(y_val) {}
    
    SkPoint ToPoint(const SkRect& rect) const;
};

/**
 * @brief CSS Transform 类
 */
class CSSTransform {
public:
    std::vector<Transform> transforms;
    
    CSSTransform() = default;
    
    static std::optional<CSSTransform> Parse(const std::string& str);
    SkMatrix ToSkMatrix(const SkRect& rect, const TransformOrigin& origin) const;
    bool IsEmpty() const { return transforms.empty(); }
    void AddTransform(const Transform& transform) { transforms.push_back(transform); }
    
private:
    static std::optional<Transform> ParseFunction(const std::string& func_name, 
                                                  const std::string& args);
    static std::optional<Transform> ParseTranslate(const std::string& args);
    static std::optional<Transform> ParseTranslateX(const std::string& args);
    static std::optional<Transform> ParseTranslateY(const std::string& args);
    static std::optional<Transform> ParseRotate(const std::string& args);
    static std::optional<Transform> ParseScale(const std::string& args);
    static std::optional<Transform> ParseSkew(const std::string& args);
    static std::optional<Transform> ParseMatrix(const std::string& args);
};

std::optional<TransformOrigin> ParseTransformOrigin(const std::string& str);

} // namespace lightui

#endif // LIGHTUI_RENDER_UTILS_TRANSFORM_H
