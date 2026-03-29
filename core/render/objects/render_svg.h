/**
 * @file render_svg.h
 * @brief SVG渲染对象
 * 
 * 功能：
 * - 渲染SVG元素（svg, path, circle, rect等）
 * - 支持fill、stroke、transform属性
 * - 集成到现有渲染管线
 */

#pragma once

#include "render_object.h"
#include "svg_path_parser.h"
#include "include/core/SkPath.h"
#include "include/core/SkPaint.h"
#include "include/core/SkMatrix.h"
#include <memory>

namespace mbink {

// 前向声明
class SVGElement;
class SVGSVGElement;
class SVGPathElement;
class SVGCircleElement;
class SVGRectElement;
class SVGEllipseElement;
class SVGLineElement;
class SVGPolylineElement;
class SVGPolygonElement;
class SVGTextElement;
class SVGGElement;

/**
 * @brief SVG渲染对象基类
 */
class RenderSVG : public RenderObject {
public:
    RenderSVG();
    ~RenderSVG() override = default;

    /**
     * @brief 绘制SVG元素
     */
    void Paint(SkCanvas* canvas) override;

    /**
     * @brief 计算布局
     */
    void Layout(float parent_width, float parent_height) override;

    /**
     * @brief 检查是否是SVG渲染对象
     * @return 始终返回true
     */
    virtual bool IsSVGRenderObject() const { return true; }

    /**
     * @brief 设置关联的SVG元素
     */
    void SetSVGElement(std::shared_ptr<SVGElement> element);

protected:
    /**
     * @brief 创建填充画笔
     */
    SkPaint CreateFillPaint() const;

    /**
     * @brief 创建描边画笔
     */
    SkPaint CreateStrokePaint() const;

    /**
     * @brief 解析颜色字符串
     */
    static SkColor ParseColor(const std::string& color);

    std::weak_ptr<SVGElement> svg_element_;
};

/**
 * @brief SVG根元素渲染对象
 */
class RenderSVGRoot : public RenderSVG {
public:
    RenderSVGRoot();
    ~RenderSVGRoot() override = default;

    void Paint(SkCanvas* canvas) override;
    void Layout(float parent_width, float parent_height) override;

    /**
     * @brief 测量 SVG 元素的固有尺寸
     * 这是 flex 布局所需的
     */
    std::pair<float, float> MeasureIntrinsicSize(float available_width);

    /**
     * @brief 设置关联的SVG根元素
     */
    void SetSVGSVGElement(std::shared_ptr<SVGSVGElement> element);

    /**
     * @brief 获取关联的SVG根元素
     */
    std::shared_ptr<SVGSVGElement> GetSVGSVGElement() const { return svg_svg_element_.lock(); }

private:
    std::weak_ptr<SVGSVGElement> svg_svg_element_;
};

/**
 * @brief SVG路径渲染对象
 */
class RenderSVGPath : public RenderSVG {
public:
    RenderSVGPath();
    ~RenderSVGPath() override = default;

    void Paint(SkCanvas* canvas) override;
    void Layout(float parent_width, float parent_height) override;

    /**
     * @brief 设置关联的path元素
     */
    void SetSVGPathElement(std::shared_ptr<SVGPathElement> element);

    /**
     * @brief 获取解析后的路径
     */
    const SkPath& GetPath() const { return path_; }

    /**
     * @brief 更新路径（从d属性解析）
     */
    void UpdatePath();

private:
    std::weak_ptr<SVGPathElement> path_element_;
    SkPath path_;
    bool path_dirty_ = true;
};

/**
 * @brief SVG圆形渲染对象
 */
class RenderSVGCircle : public RenderSVG {
public:
    RenderSVGCircle();
    ~RenderSVGCircle() override = default;

    void Paint(SkCanvas* canvas) override;
    void Layout(float parent_width, float parent_height) override;

    void SetSVGCircleElement(std::shared_ptr<SVGCircleElement> element);

private:
    std::weak_ptr<SVGCircleElement> circle_element_;
};

/**
 * @brief SVG矩形渲染对象
 */
class RenderSVGRect : public RenderSVG {
public:
    RenderSVGRect();
    ~RenderSVGRect() override = default;

    void Paint(SkCanvas* canvas) override;
    void Layout(float parent_width, float parent_height) override;

    void SetSVGRectElement(std::shared_ptr<SVGRectElement> element);

private:
    std::weak_ptr<SVGRectElement> rect_element_;
};

/**
 * @brief SVG椭圆渲染对象
 */
class RenderSVGEllipse : public RenderSVG {
public:
    RenderSVGEllipse();
    ~RenderSVGEllipse() override = default;

    void Paint(SkCanvas* canvas) override;
    void Layout(float parent_width, float parent_height) override;

    void SetSVGEllipseElement(std::shared_ptr<SVGEllipseElement> element);

private:
    std::weak_ptr<SVGEllipseElement> ellipse_element_;
};

/**
 * @brief SVG直线渲染对象
 */
class RenderSVGLine : public RenderSVG {
public:
    RenderSVGLine();
    ~RenderSVGLine() override = default;

    void Paint(SkCanvas* canvas) override;
    void Layout(float parent_width, float parent_height) override;

    void SetSVGLineElement(std::shared_ptr<SVGLineElement> element);

private:
    std::weak_ptr<SVGLineElement> line_element_;
};

/**
 * @brief SVG折线渲染对象
 */
class RenderSVGPolyline : public RenderSVG {
public:
    RenderSVGPolyline();
    ~RenderSVGPolyline() override = default;

    void Paint(SkCanvas* canvas) override;

    void SetSVGPolylineElement(std::shared_ptr<SVGPolylineElement> element);

private:
    std::weak_ptr<SVGPolylineElement> polyline_element_;
    SkPath path_;
};

/**
 * @brief SVG多边形渲染对象
 */
class RenderSVGPolygon : public RenderSVG {
public:
    RenderSVGPolygon();
    ~RenderSVGPolygon() override = default;

    void Paint(SkCanvas* canvas) override;

    void SetSVGPolygonElement(std::shared_ptr<SVGPolygonElement> element);

private:
    std::weak_ptr<SVGPolygonElement> polygon_element_;
    SkPath path_;
};

/**
 * @brief SVG分组渲染对象
 */
class RenderSVGGroup : public RenderSVG {
public:
    RenderSVGGroup();
    ~RenderSVGGroup() override = default;

    void Paint(SkCanvas* canvas) override;

    void SetSVGGElement(std::shared_ptr<SVGGElement> element);

private:
    std::weak_ptr<SVGGElement> g_element_;
};

/**
 * @brief SVG文本渲染对象
 */
class RenderSVGText : public RenderSVG {
public:
    RenderSVGText();
    ~RenderSVGText() override = default;

    void Paint(SkCanvas* canvas) override;
    void Layout(float parent_width, float parent_height) override;

    void SetSVGTextElement(std::shared_ptr<SVGTextElement> element);

private:
    std::weak_ptr<SVGTextElement> text_element_;
};

} // namespace mbink

