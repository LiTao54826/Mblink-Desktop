/**
 * @file shapes.h
 * @brief 基础图形绘制模块
 * 
 * 功能：
 * - 矩形绘制 (DrawRect, FillRect, StrokeRect)
 * - 圆形/椭圆绘制 (DrawCircle, DrawOval, FillCircle, FillOval)
 * - 线条绘制 (DrawLine, DrawPolyline)
 * - 路径绘制 (Path, DrawPath, FillPath)
 * - 圆角矩形 (DrawRoundRect, FillRoundRect)
 */

#pragma once

#include <vector>
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkPath.h"
#include "include/core/SkPoint.h"
#include "paint.h"

namespace lightui {

/**
 * @brief 图形绘制类
 * 
 * 提供基础图形绘制功能
 */
class Shapes {
public:
    /**
     * @brief 构造函数
     * @param canvas Skia 画布指针
     */
    explicit Shapes(SkCanvas* canvas);
    
    /**
     * @brief 析构函数
     */
    ~Shapes() = default;
    
    // ========== 矩形绘制 ==========
    
    /**
     * @brief 绘制矩形（填充）
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param paint 画笔
     */
    void FillRect(float x, float y, float width, float height, const Paint& paint);
    
    /**
     * @brief 绘制矩形（描边）
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param paint 画笔
     */
    void StrokeRect(float x, float y, float width, float height, const Paint& paint);
    
    /**
     * @brief 绘制矩形（使用 SkRect）
     * @param rect 矩形
     * @param paint 画笔
     */
    void DrawRect(const SkRect& rect, const Paint& paint);
    
    // ========== 圆形绘制 ==========
    
    /**
     * @brief 绘制圆形（填充）
     * @param cx 圆心 X 坐标
     * @param cy 圆心 Y 坐标
     * @param radius 半径
     * @param paint 画笔
     */
    void FillCircle(float cx, float cy, float radius, const Paint& paint);
    
    /**
     * @brief 绘制圆形（描边）
     * @param cx 圆心 X 坐标
     * @param cy 圆心 Y 坐标
     * @param radius 半径
     * @param paint 画笔
     */
    void StrokeCircle(float cx, float cy, float radius, const Paint& paint);
    
    /**
     * @brief 绘制圆形
     * @param cx 圆心 X 坐标
     * @param cy 圆心 Y 坐标
     * @param radius 半径
     * @param paint 画笔
     */
    void DrawCircle(float cx, float cy, float radius, const Paint& paint);
    
    // ========== 椭圆绘制 ==========
    
    /**
     * @brief 绘制椭圆（填充）
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param paint 画笔
     */
    void FillOval(float x, float y, float width, float height, const Paint& paint);
    
    /**
     * @brief 绘制椭圆（描边）
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param paint 画笔
     */
    void StrokeOval(float x, float y, float width, float height, const Paint& paint);
    
    /**
     * @brief 绘制椭圆
     * @param rect 椭圆外接矩形
     * @param paint 画笔
     */
    void DrawOval(const SkRect& rect, const Paint& paint);
    
    // ========== 线条绘制 ==========
    
    /**
     * @brief 绘制直线
     * @param x1 起点 X 坐标
     * @param y1 起点 Y 坐标
     * @param x2 终点 X 坐标
     * @param y2 终点 Y 坐标
     * @param paint 画笔
     */
    void DrawLine(float x1, float y1, float x2, float y2, const Paint& paint);
    
    /**
     * @brief 绘制折线
     * @param points 点数组
     * @param paint 画笔
     */
    void DrawPolyline(const std::vector<SkPoint>& points, const Paint& paint);
    
    // ========== 圆角矩形绘制 ==========
    
    /**
     * @brief 绘制圆角矩形（填充）
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param radius 圆角半径
     * @param paint 画笔
     */
    void FillRoundRect(float x, float y, float width, float height, float radius, const Paint& paint);
    
    /**
     * @brief 绘制圆角矩形（描边）
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param radius 圆角半径
     * @param paint 画笔
     */
    void StrokeRoundRect(float x, float y, float width, float height, float radius, const Paint& paint);
    
    /**
     * @brief 绘制圆角矩形（各角独立圆角）
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param radii 四个角的圆角半径 [左上, 右上, 右下, 左下]
     * @param paint 画笔
     */
    void DrawRoundRect(float x, float y, float width, float height, 
                       const std::vector<float>& radii, const Paint& paint);
    
    /**
     * @brief 绘制圆角矩形
     * @param rrect 圆角矩形
     * @param paint 画笔
     */
    void DrawRoundRect(const SkRRect& rrect, const Paint& paint);
    
    // ========== 路径绘制 ==========
    
    /**
     * @brief 绘制路径（填充）
     * @param path 路径
     * @param paint 画笔
     */
    void FillPath(const SkPath& path, const Paint& paint);
    
    /**
     * @brief 绘制路径（描边）
     * @param path 路径
     * @param paint 画笔
     */
    void StrokePath(const SkPath& path, const Paint& paint);
    
    /**
     * @brief 绘制路径
     * @param path 路径
     * @param paint 画笔
     */
    void DrawPath(const SkPath& path, const Paint& paint);
    
    // ========== 画布访问 ==========
    
    /**
     * @brief 获取画布指针
     * @return SkCanvas 指针
     */
    SkCanvas* GetCanvas() const { return canvas_; }

private:
    SkCanvas* canvas_;  ///< Skia 画布指针
};

/**
 * @brief 路径构建器类
 * 
 * 提供便捷的路径构建接口
 */
class PathBuilder {
public:
    /**
     * @brief 构造函数
     */
    PathBuilder();
    
    /**
     * @brief 移动到指定点
     * @param x X 坐标
     * @param y Y 坐标
     * @return 自身引用（支持链式调用）
     */
    PathBuilder& MoveTo(float x, float y);
    
    /**
     * @brief 绘制直线到指定点
     * @param x X 坐标
     * @param y Y 坐标
     * @return 自身引用（支持链式调用）
     */
    PathBuilder& LineTo(float x, float y);
    
    /**
     * @brief 绘制二次贝塞尔曲线
     * @param x1 控制点 X 坐标
     * @param y1 控制点 Y 坐标
     * @param x2 终点 X 坐标
     * @param y2 终点 Y 坐标
     * @return 自身引用（支持链式调用）
     */
    PathBuilder& QuadTo(float x1, float y1, float x2, float y2);
    
    /**
     * @brief 绘制三次贝塞尔曲线
     * @param x1 第一个控制点 X 坐标
     * @param y1 第一个控制点 Y 坐标
     * @param x2 第二个控制点 X 坐标
     * @param y2 第二个控制点 Y 坐标
     * @param x3 终点 X 坐标
     * @param y3 终点 Y 坐标
     * @return 自身引用（支持链式调用）
     */
    PathBuilder& CubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
    
    /**
     * @brief 闭合路径
     * @return 自身引用（支持链式调用）
     */
    PathBuilder& Close();
    
    /**
     * @brief 获取构建的路径
     * @return SkPath
     */
    SkPath Build() const { return path_; }
    
    /**
     * @brief 重置路径
     */
    void Reset();

private:
    SkPath path_;  ///< Skia 路径对象
};

} // namespace lightui

