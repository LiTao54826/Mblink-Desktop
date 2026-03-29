/**
 * @file shapes.h
 * @brief 基础图形绘制模块
 */

#pragma once

#include <vector>
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkPath.h"
#include "include/core/SkPoint.h"
#include "paint.h"

namespace mbink {

/**
 * @brief 图形绘制类
 */
class Shapes {
public:
    explicit Shapes(SkCanvas* canvas);
    ~Shapes() = default;
    
    // 矩形绘制
    void FillRect(float x, float y, float width, float height, const Paint& paint);
    void StrokeRect(float x, float y, float width, float height, const Paint& paint);
    void DrawRect(const SkRect& rect, const Paint& paint);
    
    // 圆形绘制
    void FillCircle(float cx, float cy, float radius, const Paint& paint);
    void StrokeCircle(float cx, float cy, float radius, const Paint& paint);
    void DrawCircle(float cx, float cy, float radius, const Paint& paint);
    
    // 椭圆绘制
    void FillOval(float x, float y, float width, float height, const Paint& paint);
    void StrokeOval(float x, float y, float width, float height, const Paint& paint);
    void DrawOval(const SkRect& rect, const Paint& paint);
    
    // 线条绘制
    void DrawLine(float x1, float y1, float x2, float y2, const Paint& paint);
    void DrawPolyline(const std::vector<SkPoint>& points, const Paint& paint);
    
    // 圆角矩形绘制
    void FillRoundRect(float x, float y, float w, float h, float r, const Paint& paint);
    void StrokeRoundRect(float x, float y, float w, float h, float r, const Paint& paint);
    void DrawRoundRect(float x, float y, float w, float h, 
                       const std::vector<float>& radii, const Paint& paint);
    void DrawRoundRect(const SkRRect& rrect, const Paint& paint);
    
    // 路径绘制
    void FillPath(const SkPath& path, const Paint& paint);
    void StrokePath(const SkPath& path, const Paint& paint);
    void DrawPath(const SkPath& path, const Paint& paint);
    
    SkCanvas* GetCanvas() const { return canvas_; }

private:
    SkCanvas* canvas_;
};

/**
 * @brief 路径构建器类
 */
class PathBuilder {
public:
    PathBuilder();
    PathBuilder& MoveTo(float x, float y);
    PathBuilder& LineTo(float x, float y);
    PathBuilder& QuadTo(float x1, float y1, float x2, float y2);
    PathBuilder& CubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
    PathBuilder& Close();
    SkPath Build() const { return path_; }
    void Reset();

private:
    SkPath path_;
};

} // namespace mbink
