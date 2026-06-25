/**
 * @file svg_element.h
 * @brief SVG元素类
 * 
 * 功能：
 * - 实现SVG元素（svg, path, g, circle, rect等）
 * - SVG特有属性支持（fill, stroke, transform等）
 * - 符合SVG 1.1规范
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>
#include <vector>

// 前向声明 Skia 类型
class SkPath;
class SkMatrix;

namespace mblink {

/**
 * @brief SVG元素基类
 * 
 * 所有SVG元素的基类，提供SVG特有的属性支持。
 * 
 * SVG命名空间：http://www.w3.org/2000/svg
 */
class SVGElement : public Element {
public:
    /**
     * @brief 构造函数
     * @param tag_name SVG标签名（如"svg", "path", "g"）
     */
    explicit SVGElement(const std::string& tag_name);

    /**
     * @brief 析构函数
     */
    ~SVGElement() override = default;

    /**
     * @brief 检查是否是SVG元素
     * @return 始终返回true
     */
    virtual bool IsSVGElement() const { return true; }

    // ========== SVG 通用属性 ==========

    /**
     * @brief 获取fill属性（填充颜色）
     */
    std::string GetFill() const;

    /**
     * @brief 设置fill属性
     */
    void SetFill(const std::string& value);

    /**
     * @brief 获取stroke属性（描边颜色）
     */
    std::string GetStroke() const;

    /**
     * @brief 设置stroke属性
     */
    void SetStroke(const std::string& value);

    /**
     * @brief 获取stroke-width属性
     */
    float GetStrokeWidth() const;

    /**
     * @brief 设置stroke-width属性
     */
    void SetStrokeWidth(float value);

    /**
     * @brief 获取opacity属性
     */
    float GetOpacity() const;

    /**
     * @brief 设置opacity属性
     */
    void SetOpacity(float value);

    /**
     * @brief 获取transform属性
     */
    std::string GetTransform() const;

    /**
     * @brief 设置transform属性
     */
    void SetTransform(const std::string& value);

    /**
     * @brief 获取fill-opacity属性
     */
    float GetFillOpacity() const;

    /**
     * @brief 设置fill-opacity属性
     */
    void SetFillOpacity(float value);

    /**
     * @brief 获取stroke-opacity属性
     */
    float GetStrokeOpacity() const;

    /**
     * @brief 设置stroke-opacity属性
     */
    void SetStrokeOpacity(float value);
};

/**
 * @brief SVG根元素 (<svg>)
 * 
 * SVG文档的根元素，定义SVG画布的尺寸和视口。
 * 
 * 支持属性：
 * - width, height: 画布尺寸
 * - viewBox: 视口定义 (minX, minY, width, height)
 * - preserveAspectRatio: 纵横比保持方式
 */
class SVGSVGElement : public SVGElement {
public:
    SVGSVGElement();
    ~SVGSVGElement() override = default;

    /**
     * @brief 获取width属性
     */
    std::string GetWidth() const;

    /**
     * @brief 设置width属性
     */
    void SetWidth(const std::string& value);

    /**
     * @brief 获取height属性
     */
    std::string GetHeight() const;

    /**
     * @brief 设置height属性
     */
    void SetHeight(const std::string& value);

    /**
     * @brief 获取viewBox属性
     */
    std::string GetViewBox() const;

    /**
     * @brief 设置viewBox属性
     */
    void SetViewBox(const std::string& value);

    /**
     * @brief 解析viewBox为数值
     * @param min_x 输出：最小X坐标
     * @param min_y 输出：最小Y坐标
     * @param width 输出：宽度
     * @param height 输出：高度
     * @return 是否解析成功
     */
    bool ParseViewBox(float& min_x, float& min_y, float& width, float& height) const;
};

/**
 * @brief SVG路径元素 (<path>)
 * 
 * 最重要的SVG元素，用于绘制复杂路径。
 * 图标库（如Font Awesome, Material Icons）主要使用path元素。
 * 
 * 支持的路径命令：
 * - M/m: moveto（移动到）
 * - L/l: lineto（画直线到）
 * - H/h: 水平线
 * - V/v: 垂直线
 * - C/c: 三次贝塞尔曲线
 * - S/s: 平滑三次贝塞尔
 * - Q/q: 二次贝塞尔曲线
 * - T/t: 平滑二次贝塞尔
 * - A/a: 椭圆弧
 * - Z/z: closepath（闭合路径）
 */
class SVGPathElement : public SVGElement {
public:
    SVGPathElement();
    ~SVGPathElement() override = default;

    /**
     * @brief 获取d属性（路径数据）
     */
    std::string GetD() const;

    /**
     * @brief 设置d属性
     */
    void SetD(const std::string& value);

    /**
     * @brief 获取路径总长度
     */
    float GetTotalLength() const;
};

/**
 * @brief SVG分组元素 (<g>)
 * 
 * 用于将多个SVG元素组合在一起。
 * 可以对整组元素应用变换和样式。
 */
class SVGGElement : public SVGElement {
public:
    SVGGElement();
    ~SVGGElement() override = default;
};

/**
 * @brief SVG圆形元素 (<circle>)
 */
class SVGCircleElement : public SVGElement {
public:
    SVGCircleElement();
    ~SVGCircleElement() override = default;

    float GetCx() const;
    void SetCx(float value);

    float GetCy() const;
    void SetCy(float value);

    float GetR() const;
    void SetR(float value);
};

/**
 * @brief SVG矩形元素 (<rect>)
 */
class SVGRectElement : public SVGElement {
public:
    SVGRectElement();
    ~SVGRectElement() override = default;

    float GetX() const;
    void SetX(float value);

    float GetY() const;
    void SetY(float value);

    float GetWidth() const;
    void SetWidth(float value);

    float GetHeight() const;
    void SetHeight(float value);

    float GetRx() const;
    void SetRx(float value);

    float GetRy() const;
    void SetRy(float value);
};

/**
 * @brief SVG椭圆元素 (<ellipse>)
 */
class SVGEllipseElement : public SVGElement {
public:
    SVGEllipseElement();
    ~SVGEllipseElement() override = default;

    float GetCx() const;
    void SetCx(float value);

    float GetCy() const;
    void SetCy(float value);

    float GetRx() const;
    void SetRx(float value);

    float GetRy() const;
    void SetRy(float value);
};

/**
 * @brief SVG直线元素 (<line>)
 */
class SVGLineElement : public SVGElement {
public:
    SVGLineElement();
    ~SVGLineElement() override = default;

    float GetX1() const;
    void SetX1(float value);

    float GetY1() const;
    void SetY1(float value);

    float GetX2() const;
    void SetX2(float value);

    float GetY2() const;
    void SetY2(float value);
};

/**
 * @brief SVG折线元素 (<polyline>)
 */
class SVGPolylineElement : public SVGElement {
public:
    SVGPolylineElement();
    ~SVGPolylineElement() override = default;

    std::string GetPoints() const;
    void SetPoints(const std::string& value);
};

/**
 * @brief SVG多边形元素 (<polygon>)
 */
class SVGPolygonElement : public SVGElement {
public:
    SVGPolygonElement();
    ~SVGPolygonElement() override = default;

    std::string GetPoints() const;
    void SetPoints(const std::string& value);
};

/**
 * @brief SVG文本元素 (<text>)
 */
class SVGTextElement : public SVGElement {
public:
    SVGTextElement();
    ~SVGTextElement() override = default;

    float GetX() const;
    void SetX(float value);

    float GetY() const;
    void SetY(float value);

    std::string GetTextAnchor() const;
    float GetFontSize() const;
};

} // namespace mblink

