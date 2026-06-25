/**
 * @file svg_path_parser.h
 * @brief SVG路径解析器
 * 
 * 功能：
 * - 解析SVG path元素的d属性
 * - 将路径命令转换为Skia SkPath
 * - 支持所有标准SVG路径命令
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "include/core/SkPath.h"
#include "include/core/SkMatrix.h"

namespace mblink {

/**
 * @brief SVG路径命令类型
 */
enum class SVGPathCommand {
    MOVE_TO,           // M/m
    LINE_TO,           // L/l
    HORIZONTAL_LINE,   // H/h
    VERTICAL_LINE,     // V/v
    CUBIC_BEZIER,      // C/c
    SMOOTH_CUBIC,      // S/s
    QUADRATIC_BEZIER,  // Q/q
    SMOOTH_QUADRATIC,  // T/t
    ARC,               // A/a
    CLOSE_PATH         // Z/z
};

/**
 * @brief SVG路径段
 */
struct SVGPathSegment {
    SVGPathCommand command;
    bool is_relative;      // 小写命令为相对坐标
    std::vector<float> params;
};

/**
 * @brief SVG路径解析器
 * 
 * 将SVG path的d属性解析为Skia SkPath对象。
 * 
 * 支持的命令：
 * - M x y: moveto（移动到绝对坐标）
 * - m dx dy: moveto（移动到相对坐标）
 * - L x y: lineto（画线到绝对坐标）
 * - l dx dy: lineto（画线到相对坐标）
 * - H x: 水平线到绝对X坐标
 * - h dx: 水平线（相对）
 * - V y: 垂直线到绝对Y坐标
 * - v dy: 垂直线（相对）
 * - C x1 y1 x2 y2 x y: 三次贝塞尔曲线
 * - c: 三次贝塞尔曲线（相对）
 * - S x2 y2 x y: 平滑三次贝塞尔
 * - s: 平滑三次贝塞尔（相对）
 * - Q x1 y1 x y: 二次贝塞尔曲线
 * - q: 二次贝塞尔曲线（相对）
 * - T x y: 平滑二次贝塞尔
 * - t: 平滑二次贝塞尔（相对）
 * - A rx ry x-axis-rotation large-arc-flag sweep-flag x y: 椭圆弧
 * - a: 椭圆弧（相对）
 * - Z/z: 闭合路径
 */
class SVGPathParser {
public:
    /**
     * @brief 将SVG路径字符串解析为SkPath
     * @param d SVG path的d属性值
     * @return 解析后的SkPath
     */
    static SkPath Parse(const std::string& d);

    /**
     * @brief 解析路径为段列表
     * @param d SVG path的d属性值
     * @return 路径段列表
     */
    static std::vector<SVGPathSegment> ParseToSegments(const std::string& d);

private:
    /**
     * @brief 跳过空白字符
     */
    static size_t SkipWhitespace(const std::string& str, size_t pos);

    /**
     * @brief 跳过可选的逗号和空白
     */
    static size_t SkipCommaWhitespace(const std::string& str, size_t pos);

    /**
     * @brief 解析一个数字
     */
    static bool ParseNumber(const std::string& str, size_t& pos, float& value);

    /**
     * @brief 解析一个标志（0或1）
     */
    static bool ParseFlag(const std::string& str, size_t& pos, bool& flag);

    /**
     * @brief 将椭圆弧参数转换为贝塞尔曲线
     */
    static void ArcToBezier(SkPath& path, 
                           float x1, float y1,  // 起点
                           float rx, float ry,  // 半径
                           float angle,         // X轴旋转角度（度）
                           bool large_arc,      // 大弧标志
                           bool sweep,          // 顺时针标志
                           float x2, float y2); // 终点
};

/**
 * @brief SVG变换解析器
 * 
 * 解析SVG transform属性为Skia矩阵。
 * 
 * 支持的变换：
 * - translate(tx [ty])
 * - scale(sx [sy])
 * - rotate(angle [cx cy])
 * - skewX(angle)
 * - skewY(angle)
 * - matrix(a b c d e f)
 */
class SVGTransformParser {
public:
    /**
     * @brief 解析transform属性为SkMatrix
     * @param transform transform属性值
     * @return 解析后的矩阵
     */
    static SkMatrix Parse(const std::string& transform);

private:
    /**
     * @brief 解析单个变换函数
     */
    static bool ParseFunction(const std::string& str, size_t& pos, SkMatrix& matrix);

    /**
     * @brief 解析参数列表
     */
    static std::vector<float> ParseParams(const std::string& str, size_t& pos);
};

/**
 * @brief SVG points属性解析器
 * 
 * 解析polyline和polygon的points属性。
 */
class SVGPointsParser {
public:
    /**
     * @brief 解析points属性为点列表
     * @param points points属性值（如"10,10 40,60 70,10"）
     * @return 点列表（x1,y1,x2,y2,...）
     */
    static std::vector<float> Parse(const std::string& points);

    /**
     * @brief 将points转换为SkPath（polyline）
     * @param points points属性值
     * @return 解析后的SkPath（不闭合）
     */
    static SkPath ToPolylinePath(const std::string& points);

    /**
     * @brief 将points转换为SkPath（polygon）
     * @param points points属性值
     * @return 解析后的SkPath（闭合）
     */
    static SkPath ToPolygonPath(const std::string& points);
};

} // namespace mblink

