/**
 * @file shapes.cpp
 * @brief 基础图形绘制模块实现
 */

#include "shapes.h"

namespace mbink {

Shapes::Shapes(SkCanvas* canvas) : canvas_(canvas) {}

void Shapes::FillRect(float x, float y, float width, float height, const Paint& paint) {
    if (!canvas_) return;
    SkRect rect = SkRect::MakeXYWH(x, y, width, height);
    Paint fill_paint = paint;
    fill_paint.SetStyle(PaintStyle::FILL);
    canvas_->drawRect(rect, fill_paint.GetSkPaint());
}

void Shapes::StrokeRect(float x, float y, float width, float height, const Paint& paint) {
    if (!canvas_) return;
    SkRect rect = SkRect::MakeXYWH(x, y, width, height);
    Paint stroke_paint = paint;
    stroke_paint.SetStyle(PaintStyle::STROKE);
    canvas_->drawRect(rect, stroke_paint.GetSkPaint());
}

void Shapes::DrawRect(const SkRect& rect, const Paint& paint) {
    if (!canvas_) return;
    canvas_->drawRect(rect, paint.GetSkPaint());
}

void Shapes::FillCircle(float cx, float cy, float radius, const Paint& paint) {
    if (!canvas_) return;
    Paint fill_paint = paint;
    fill_paint.SetStyle(PaintStyle::FILL);
    canvas_->drawCircle(cx, cy, radius, fill_paint.GetSkPaint());
}

void Shapes::StrokeCircle(float cx, float cy, float radius, const Paint& paint) {
    if (!canvas_) return;
    Paint stroke_paint = paint;
    stroke_paint.SetStyle(PaintStyle::STROKE);
    canvas_->drawCircle(cx, cy, radius, stroke_paint.GetSkPaint());
}

void Shapes::DrawCircle(float cx, float cy, float radius, const Paint& paint) {
    if (!canvas_) return;
    canvas_->drawCircle(cx, cy, radius, paint.GetSkPaint());
}

void Shapes::FillOval(float x, float y, float width, float height, const Paint& paint) {
    if (!canvas_) return;
    SkRect rect = SkRect::MakeXYWH(x, y, width, height);
    Paint fill_paint = paint;
    fill_paint.SetStyle(PaintStyle::FILL);
    canvas_->drawOval(rect, fill_paint.GetSkPaint());
}

void Shapes::StrokeOval(float x, float y, float width, float height, const Paint& paint) {
    if (!canvas_) return;
    SkRect rect = SkRect::MakeXYWH(x, y, width, height);
    Paint stroke_paint = paint;
    stroke_paint.SetStyle(PaintStyle::STROKE);
    canvas_->drawOval(rect, stroke_paint.GetSkPaint());
}

void Shapes::DrawOval(const SkRect& rect, const Paint& paint) {
    if (!canvas_) return;
    canvas_->drawOval(rect, paint.GetSkPaint());
}

void Shapes::DrawLine(float x1, float y1, float x2, float y2, const Paint& paint) {
    if (!canvas_) return;
    Paint stroke_paint = paint;
    stroke_paint.SetStyle(PaintStyle::STROKE);
    canvas_->drawLine(x1, y1, x2, y2, stroke_paint.GetSkPaint());
}

void Shapes::DrawPolyline(const std::vector<SkPoint>& points, const Paint& paint) {
    if (!canvas_ || points.size() < 2) return;
    SkPath path;
    path.moveTo(points[0]);
    for (size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i]);
    }
    Paint stroke_paint = paint;
    stroke_paint.SetStyle(PaintStyle::STROKE);
    canvas_->drawPath(path, stroke_paint.GetSkPaint());
}

void Shapes::FillRoundRect(float x, float y, float w, float h, float r, const Paint& paint) {
    if (!canvas_) return;
    SkRect rect = SkRect::MakeXYWH(x, y, w, h);
    SkRRect rrect = SkRRect::MakeRectXY(rect, r, r);
    Paint fill_paint = paint;
    fill_paint.SetStyle(PaintStyle::FILL);
    canvas_->drawRRect(rrect, fill_paint.GetSkPaint());
}

void Shapes::StrokeRoundRect(float x, float y, float w, float h, float r, const Paint& paint) {
    if (!canvas_) return;
    SkRect rect = SkRect::MakeXYWH(x, y, w, h);
    SkRRect rrect = SkRRect::MakeRectXY(rect, r, r);
    Paint stroke_paint = paint;
    stroke_paint.SetStyle(PaintStyle::STROKE);
    canvas_->drawRRect(rrect, stroke_paint.GetSkPaint());
}

void Shapes::DrawRoundRect(float x, float y, float w, float h, 
                           const std::vector<float>& radii, const Paint& paint) {
    if (!canvas_ || radii.size() < 4) return;
    SkRect rect = SkRect::MakeXYWH(x, y, w, h);
    SkVector corners[4] = {
        {radii[0], radii[0]}, {radii[1], radii[1]},
        {radii[2], radii[2]}, {radii[3], radii[3]}
    };
    SkRRect rrect;
    rrect.setRectRadii(rect, corners);
    canvas_->drawRRect(rrect, paint.GetSkPaint());
}

void Shapes::DrawRoundRect(const SkRRect& rrect, const Paint& paint) {
    if (!canvas_) return;
    canvas_->drawRRect(rrect, paint.GetSkPaint());
}

void Shapes::FillPath(const SkPath& path, const Paint& paint) {
    if (!canvas_) return;
    Paint fill_paint = paint;
    fill_paint.SetStyle(PaintStyle::FILL);
    canvas_->drawPath(path, fill_paint.GetSkPaint());
}

void Shapes::StrokePath(const SkPath& path, const Paint& paint) {
    if (!canvas_) return;
    Paint stroke_paint = paint;
    stroke_paint.SetStyle(PaintStyle::STROKE);
    canvas_->drawPath(path, stroke_paint.GetSkPaint());
}

void Shapes::DrawPath(const SkPath& path, const Paint& paint) {
    if (!canvas_) return;
    canvas_->drawPath(path, paint.GetSkPaint());
}

// PathBuilder 实现
PathBuilder::PathBuilder() {}

PathBuilder& PathBuilder::MoveTo(float x, float y) {
    path_.moveTo(x, y);
    return *this;
}

PathBuilder& PathBuilder::LineTo(float x, float y) {
    path_.lineTo(x, y);
    return *this;
}

PathBuilder& PathBuilder::QuadTo(float x1, float y1, float x2, float y2) {
    path_.quadTo(x1, y1, x2, y2);
    return *this;
}

PathBuilder& PathBuilder::CubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
    path_.cubicTo(x1, y1, x2, y2, x3, y3);
    return *this;
}

PathBuilder& PathBuilder::Close() {
    path_.close();
    return *this;
}

void PathBuilder::Reset() {
    path_.reset();
}

} // namespace mbink
