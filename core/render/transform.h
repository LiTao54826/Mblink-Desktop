/**
 * @file transform.h
 * @brief CSS Transform 数据结构和解析
 */

#ifndef LIGHTUI_RENDER_TRANSFORM_H
#define LIGHTUI_RENDER_TRANSFORM_H

#include "css_value.h"
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
 *
 * 对于 TRANSLATE 类型，使用 lengths 存储 CSSLength 以支持百分比值
 * 对于其他类型，使用 values 存储已解析的数值
 */
struct Transform {
    TransformType type;
    std::vector<float> values;  // 参数值（用于 rotate, scale, skew, matrix）
    std::vector<CSSLength> lengths;  // 长度值（用于 translate，支持百分比）

    Transform() : type(TransformType::TRANSLATE) {}
    Transform(TransformType t) : type(t) {}
    Transform(TransformType t, const std::vector<float>& v) : type(t), values(v) {}
    Transform(TransformType t, const std::vector<CSSLength>& l) : type(t), lengths(l) {}
};

/**
 * @brief Transform Origin (变换原点)
 */
struct TransformOrigin {
    CSSLength x;  // 水平位置 (left, center, right, 或长度值)
    CSSLength y;  // 垂直位置 (top, center, bottom, 或长度值)
    
    TransformOrigin() 
        : x(50.0f, CSSUnit::PERCENT), 
          y(50.0f, CSSUnit::PERCENT) {}  // 默认 center center
    
    TransformOrigin(const CSSLength& x_val, const CSSLength& y_val) 
        : x(x_val), y(y_val) {}
    
    /**
     * @brief 转换为像素坐标
     * @param rect 元素的边界框
     * @return 变换原点的像素坐标
     */
    SkPoint ToPoint(const SkRect& rect) const;
};

/**
 * @brief CSS Transform 类
 * 
 * 支持的 transform 函数：
 * - translate(x, y), translateX(x), translateY(y)
 * - rotate(angle)
 * - scale(x, y), scaleX(x), scaleY(y)
 * - skew(x, y), skewX(x), skewY(y)
 * - matrix(a, b, c, d, e, f)
 */
class CSSTransform {
public:
    std::vector<Transform> transforms;  // Transform 操作列表
    
    CSSTransform() = default;
    
    /**
     * @brief 解析 transform 字符串
     * @param str CSS transform 字符串
     * @return 解析后的 CSSTransform 对象，失败返回 nullopt
     * 
     * 示例：
     * - "translate(10px, 20px)"
     * - "rotate(45deg)"
     * - "scale(1.5) rotate(45deg)"
     */
    static std::optional<CSSTransform> Parse(const std::string& str);
    
    /**
     * @brief 转换为 Skia Matrix
     * @param rect 元素的边界框（用于计算百分比值）
     * @param origin 变换原点
     * @return Skia 变换矩阵
     */
    SkMatrix ToSkMatrix(const SkRect& rect, const TransformOrigin& origin) const;
    
    /**
     * @brief 检查是否为空（无变换）
     */
    bool IsEmpty() const { return transforms.empty(); }
    
    /**
     * @brief 添加一个 transform 操作
     */
    void AddTransform(const Transform& transform) {
        transforms.push_back(transform);
    }
    
private:
    /**
     * @brief 解析单个 transform 函数
     * @param func_name 函数名（如 "translate", "rotate"）
     * @param args 参数字符串（如 "10px, 20px"）
     * @return 解析后的 Transform 对象，失败返回 nullopt
     */
    static std::optional<Transform> ParseFunction(const std::string& func_name, 
                                                  const std::string& args);
    
    /**
     * @brief 解析 translate 函数
     */
    static std::optional<Transform> ParseTranslate(const std::string& args);
    
    /**
     * @brief 解析 translateX 函数
     */
    static std::optional<Transform> ParseTranslateX(const std::string& args);
    
    /**
     * @brief 解析 translateY 函数
     */
    static std::optional<Transform> ParseTranslateY(const std::string& args);
    
    /**
     * @brief 解析 rotate 函数
     */
    static std::optional<Transform> ParseRotate(const std::string& args);
    
    /**
     * @brief 解析 scale 函数
     */
    static std::optional<Transform> ParseScale(const std::string& args);
    
    /**
     * @brief 解析 skew 函数
     */
    static std::optional<Transform> ParseSkew(const std::string& args);
    
    /**
     * @brief 解析 matrix 函数
     */
    static std::optional<Transform> ParseMatrix(const std::string& args);
};

/**
 * @brief 解析 transform-origin 属性
 * @param str CSS transform-origin 字符串
 * @return 解析后的 TransformOrigin 对象，失败返回 nullopt
 * 
 * 示例：
 * - "center center" (默认)
 * - "top left"
 * - "50% 50%"
 * - "10px 20px"
 * - "left top"
 */
std::optional<TransformOrigin> ParseTransformOrigin(const std::string& str);

} // namespace lightui

#endif // LIGHTUI_RENDER_TRANSFORM_H

