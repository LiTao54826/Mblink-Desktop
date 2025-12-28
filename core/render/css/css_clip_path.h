/**
 * @file css_clip_path.h
 * @brief CSS clip-path 属性支持
 *
 * 支持的裁剪形状：
 * - none: 不裁剪
 * - inset(): 矩形裁剪，支持圆角
 * - circle(): 圆形裁剪
 * - ellipse(): 椭圆裁剪
 * - polygon(): 多边形裁剪
 */

#pragma once

#include "core/render/css/css_value.h"
#include <string>
#include <vector>
#include <optional>
#include "include/core/SkPath.h"
#include "include/core/SkRect.h"
#include "include/core/SkRRect.h"

namespace lightui {

/**
 * @brief 裁剪形状类型
 */
enum class ClipPathType {
    NONE,       // 不裁剪
    INSET,      // 矩形裁剪
    CIRCLE,     // 圆形裁剪
    ELLIPSE,    // 椭圆裁剪
    POLYGON     // 多边形裁剪
};

/**
 * @brief 多边形填充规则
 */
enum class ClipFillRule {
    NONZERO,    // 非零环绕规则（默认）
    EVENODD     // 奇偶规则
};

/**
 * @brief 裁剪位置（用于 circle/ellipse 的 at 参数）
 */
struct ClipPosition {
    CSSLength x;
    CSSLength y;
    
    ClipPosition() {
        // 默认居中 (50% 50%)
        x = CSSLength(50.0f, CSSUnit::PERCENT);
        y = CSSLength(50.0f, CSSUnit::PERCENT);
    }
    
    ClipPosition(const CSSLength& px, const CSSLength& py) : x(px), y(py) {}
};

/**
 * @brief 半径类型（用于 circle/ellipse）
 */
enum class RadiusType {
    LENGTH,         // 具体长度值
    CLOSEST_SIDE,   // 到最近边的距离
    FARTHEST_SIDE   // 到最远边的距离
};

/**
 * @brief Inset 裁剪参数
 * 
 * 语法: inset(top right bottom left [round radius])
 */
struct ClipInset {
    CSSLength top;
    CSSLength right;
    CSSLength bottom;
    CSSLength left;
    CSSBorderRadius border_radius;  // round 参数
    bool has_round = false;
    
    ClipInset() {
        top = CSSLength(0, CSSUnit::PX);
        right = CSSLength(0, CSSUnit::PX);
        bottom = CSSLength(0, CSSUnit::PX);
        left = CSSLength(0, CSSUnit::PX);
    }
};

/**
 * @brief Circle 裁剪参数
 * 
 * 语法: circle([radius] [at position])
 */
struct ClipCircle {
    CSSLength radius;
    RadiusType radius_type = RadiusType::CLOSEST_SIDE;
    ClipPosition position;
    
    ClipCircle() {
        radius = CSSLength(50.0f, CSSUnit::PERCENT);
    }
};

/**
 * @brief Ellipse 裁剪参数
 * 
 * 语法: ellipse([rx ry] [at position])
 */
struct ClipEllipse {
    CSSLength radius_x;
    CSSLength radius_y;
    RadiusType radius_type_x = RadiusType::CLOSEST_SIDE;
    RadiusType radius_type_y = RadiusType::CLOSEST_SIDE;
    ClipPosition position;
    
    ClipEllipse() {
        radius_x = CSSLength(50.0f, CSSUnit::PERCENT);
        radius_y = CSSLength(50.0f, CSSUnit::PERCENT);
    }
};

/**
 * @brief Polygon 裁剪参数
 * 
 * 语法: polygon([fill-rule,] x1 y1, x2 y2, ...)
 */
struct ClipPolygon {
    std::vector<std::pair<CSSLength, CSSLength>> points;
    ClipFillRule fill_rule = ClipFillRule::NONZERO;
};

/**
 * @brief CSS clip-path 属性值
 */
struct CSSClipPath {
    ClipPathType type = ClipPathType::NONE;
    
    // 根据 type 使用对应的数据
    ClipInset inset;
    ClipCircle circle;
    ClipEllipse ellipse;
    ClipPolygon polygon;
    
    /**
     * @brief 检查是否为 none
     */
    bool IsNone() const { return type == ClipPathType::NONE; }
    
    /**
     * @brief 转换为 SkPath
     * @param bounds 元素的边界框
     * @return Skia 路径对象
     */
    SkPath ToSkPath(const SkRect& bounds) const;
};

/**
 * @brief 解析 clip-path 属性值
 * @param value CSS clip-path 字符串
 * @return CSSClipPath 对象
 */
CSSClipPath ParseClipPath(const std::string& value);

/**
 * @brief 解析 inset() 函数
 * @param params 函数参数字符串
 * @return ClipInset 对象
 */
ClipInset ParseClipInset(const std::string& params);

/**
 * @brief 解析 circle() 函数
 * @param params 函数参数字符串
 * @return ClipCircle 对象
 */
ClipCircle ParseClipCircle(const std::string& params);

/**
 * @brief 解析 ellipse() 函数
 * @param params 函数参数字符串
 * @return ClipEllipse 对象
 */
ClipEllipse ParseClipEllipse(const std::string& params);

/**
 * @brief 解析 polygon() 函数
 * @param params 函数参数字符串
 * @return ClipPolygon 对象
 */
ClipPolygon ParseClipPolygon(const std::string& params);

} // namespace lightui
