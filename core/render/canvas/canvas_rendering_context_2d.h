/**
 * @file canvas_rendering_context_2d.h
 * @brief Canvas 2D渲染上下文
 * 
 * 功能：
 * - 实现HTML5 Canvas 2D API
 * - 基于Skia进行实际渲染
 * - 支持路径、图形、文本、图像绘制
 * - 支持变换、状态管理
 */

#pragma once

#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkFont.h"
#include "include/core/SkColor.h"
#include <string>
#include <vector>
#include <memory>

namespace lightui {

// 前向声明
class CanvasGradient;
class CanvasPattern;
class ImageData;

/**
 * @brief Canvas 2D渲染上下文
 * 
 * 对应HTML5 Canvas 2D Context标准：
 * https://html.spec.whatwg.org/multipage/canvas.html#2dcontext
 */
class CanvasRenderingContext2D {
public:
    /**
     * @brief 构造函数
     * @param width 画布宽度
     * @param height 画布高度
     */
    CanvasRenderingContext2D(unsigned int width, unsigned int height);

    /**
     * @brief 析构函数
     */
    ~CanvasRenderingContext2D() = default;

    /**
     * @brief 调整画布大小
     * @param width 新宽度
     * @param height 新高度
     */
    void Resize(unsigned int width, unsigned int height);

    /**
     * @brief 导出为Data URL
     * @param type MIME类型
     * @param quality 质量（0.0-1.0）
     * @return Data URL字符串
     */
    std::string ToDataURL(const std::string& type, double quality);

    /**
     * @brief 获取内部Skia Canvas（用于渲染到屏幕）
     * @return Skia Canvas指针
     */
    SkCanvas* GetCanvas() const { return surface_ ? surface_->getCanvas() : nullptr; }

    /**
     * @brief 获取Surface（用于读取像素数据）
     * @return Skia Surface指针
     */
    SkSurface* GetSurface() const { return surface_.get(); }

    // ========== 矩形绘制 ==========

    /**
     * @brief 清除矩形区域
     * @param x 左上角X坐标
     * @param y 左上角Y坐标
     * @param width 宽度
     * @param height 高度
     */
    void ClearRect(double x, double y, double width, double height);

    /**
     * @brief 填充矩形
     * @param x 左上角X坐标
     * @param y 左上角Y坐标
     * @param width 宽度
     * @param height 高度
     */
    void FillRect(double x, double y, double width, double height);

    /**
     * @brief 描边矩形
     * @param x 左上角X坐标
     * @param y 左上角Y坐标
     * @param width 宽度
     * @param height 高度
     */
    void StrokeRect(double x, double y, double width, double height);

    // ========== 路径 ==========

    void BeginPath();
    void ClosePath();
    void MoveTo(double x, double y);
    void LineTo(double x, double y);
    void Arc(double x, double y, double radius, double startAngle, double endAngle, bool anticlockwise = false);
    void ArcTo(double x1, double y1, double x2, double y2, double radius);
    void QuadraticCurveTo(double cpx, double cpy, double x, double y);
    void BezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);
    void Rect(double x, double y, double width, double height);
    
    /**
     * @brief 添加圆角矩形路径
     * @param x 左上角X坐标
     * @param y 左上角Y坐标
     * @param width 宽度
     * @param height 高度
     * @param radius 圆角半径
     */
    void RoundRect(double x, double y, double width, double height, double radius);
    
    void Fill();
    void Stroke();
    void Clip();
    void Ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool anticlockwise = false);
    
    // 点击检测
    bool IsPointInPath(double x, double y);
    bool IsPointInStroke(double x, double y);

    // ========== 文本 ==========

    void FillText(const std::string& text, double x, double y);
    void StrokeText(const std::string& text, double x, double y);
    double MeasureText(const std::string& text);  // returns width

    // ========== 样式属性 ==========

    void SetFillStyle(const std::string& color);
    void SetFillStyle(CanvasGradient* gradient);
    void SetFillStyle(CanvasPattern* pattern);
    void SetStrokeStyle(const std::string& color);
    void SetStrokeStyle(CanvasGradient* gradient);
    void SetStrokeStyle(CanvasPattern* pattern);
    void SetLineWidth(double width);
    void SetLineCap(const std::string& cap);
    void SetLineJoin(const std::string& join);
    void SetMiterLimit(double limit);
    void SetFont(const std::string& font);
    void SetTextAlign(const std::string& align);
    void SetTextBaseline(const std::string& baseline);
    void SetGlobalAlpha(double alpha);
    
    // ========== 渐变与图案 ==========
    
    CanvasGradient* CreateLinearGradient(double x0, double y0, double x1, double y1);
    CanvasGradient* CreateRadialGradient(double x0, double y0, double r0, double x1, double y1, double r1);
    CanvasGradient* CreateConicGradient(double startAngle, double x, double y);
    
    /**
     * @brief 创建图案填充
     * @param image 图像数据指针（SkImage*）
     * @param repetition 重复模式："repeat", "repeat-x", "repeat-y", "no-repeat"
     * @return CanvasPattern对象指针
     */
    CanvasPattern* CreatePattern(void* image, const std::string& repetition);

    std::string GetFillStyle() const;
    std::string GetStrokeStyle() const;
    double GetLineWidth() const;
    std::string GetLineCap() const;
    std::string GetLineJoin() const;
    double GetMiterLimit() const;
    std::string GetFont() const;
    std::string GetTextAlign() const;
    std::string GetTextBaseline() const;
    double GetGlobalAlpha() const;
    
    // 虚线
    void SetLineDash(const std::vector<double>& segments);
    std::vector<double> GetLineDash() const;
    void SetLineDashOffset(double offset);
    double GetLineDashOffset() const;
    
    // ========== 合成与阴影 ==========
    
    void SetGlobalCompositeOperation(const std::string& op);
    std::string GetGlobalCompositeOperation() const;
    
    void SetShadowColor(const std::string& color);
    std::string GetShadowColor() const;
    void SetShadowBlur(double blur);
    double GetShadowBlur() const;
    void SetShadowOffsetX(double offset);
    double GetShadowOffsetX() const;
    void SetShadowOffsetY(double offset);
    double GetShadowOffsetY() const;

    // ========== 变换 ==========

    void Scale(double x, double y);
    void Rotate(double angle);
    void Translate(double x, double y);
    void Transform(double a, double b, double c, double d, double e, double f);
    void SetTransform(double a, double b, double c, double d, double e, double f);
    void ResetTransform();
    
    /**
     * @brief 获取当前变换矩阵
     * @return 变换矩阵的6个值 [a, b, c, d, e, f]
     */
    std::vector<double> GetTransform() const;

    // ========== 状态管理 ==========

    void Save();
    void Restore();

    // ========== 图像绘制 ==========

    /**
     * @brief 绘制图像
     * @param image 图像数据指针（SkImage*）
     * @param dx 目标X坐标
     * @param dy 目标Y坐标
     */
    void DrawImage(void* image, double dx, double dy);

    /**
     * @brief 绘制图像（带尺寸）
     * @param image 图像数据指针（SkImage*）
     * @param dx 目标X坐标
     * @param dy 目标Y坐标
     * @param dwidth 目标宽度
     * @param dheight 目标高度
     */
    void DrawImage(void* image, double dx, double dy, double dwidth, double dheight);
    
    // ========== 像素操作 ==========
    
    /**
     * @brief 创建空白ImageData
     */
    ImageData* CreateImageData(unsigned int width, unsigned int height);
    
    /**
     * @brief 获取画布像素数据
     */
    ImageData* GetImageData(int x, int y, unsigned int width, unsigned int height);
    
    /**
     * @brief 写入像素数据到画布
     */
    void PutImageData(ImageData* imageData, int dx, int dy);

private:
    /**
     * @brief 绘图状态
     */
    struct DrawingState {
        SkPaint fill_paint;       ///< 填充画笔
        SkPaint stroke_paint;     ///< 描边画笔
        SkFont font;              ///< 字体
        SkMatrix transform;       ///< 变换矩阵
        std::string text_align;   ///< 文本对齐
        std::string text_baseline;///< 文本基线
        double global_alpha;      ///< 全局透明度
        std::vector<double> line_dash;  ///< 虚线段数组
        double line_dash_offset;        ///< 虚线偏移
        
        // 合成与阴影
        std::string global_composite_operation;  ///< 合成操作
        SkColor shadow_color;     ///< 阴影颜色
        double shadow_blur;       ///< 阴影模糊半径
        double shadow_offset_x;   ///< 阴影X偏移
        double shadow_offset_y;   ///< 阴影Y偏移

        DrawingState();
    };

    /**
     * @brief 解析CSS颜色字符串
     * @param color_str 颜色字符串（如"#FF0000", "rgb(255,0,0)", "red"）
     * @return Skia颜色值
     */
    SkColor ParseColor(const std::string& color_str);

    /**
     * @brief 应用当前状态的全局透明度到画笔
     * @param paint 画笔
     */
    void ApplyGlobalAlpha(SkPaint& paint);

private:
    unsigned int width_;                        ///< 画布宽度
    unsigned int height_;                       ///< 画布高度
    sk_sp<SkSurface> surface_;                 ///< Skia Surface（离屏渲染）
    SkPath current_path_;                      ///< 当前路径
    std::vector<DrawingState> state_stack_;    ///< 状态栈
    DrawingState current_state_;               ///< 当前状态
};

} // namespace lightui
