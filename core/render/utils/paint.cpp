/**
 * @file paint.cpp
 * @brief 画笔管理模块实现
 */

#include "paint.h"
#include <algorithm>

namespace lightui {

// ========== 构造函数 ==========

Paint::Paint() {
    // 设置默认值
    paint_.setAntiAlias(true);
    paint_.setColor(SK_ColorBLACK);
    paint_.setStyle(SkPaint::kFill_Style);
}

// ========== 颜色设置 ==========

void Paint::SetColor(SkColor color) {
    paint_.setColor(color);
}

SkColor Paint::GetColor() const {
    return paint_.getColor();
}

void Paint::SetAlpha(int alpha) {
    paint_.setAlpha(std::clamp(alpha, 0, 255));
}

int Paint::GetAlpha() const {
    return paint_.getAlpha();
}

// ========== 样式设置 ==========

void Paint::SetStyle(PaintStyle style) {
    switch (style) {
        case PaintStyle::FILL:
            paint_.setStyle(SkPaint::kFill_Style);
            break;
        case PaintStyle::STROKE:
            paint_.setStyle(SkPaint::kStroke_Style);
            break;
        case PaintStyle::FILL_STROKE:
            paint_.setStyle(SkPaint::kStrokeAndFill_Style);
            break;
    }
}

PaintStyle Paint::GetStyle() const {
    switch (paint_.getStyle()) {
        case SkPaint::kFill_Style:
            return PaintStyle::FILL;
        case SkPaint::kStroke_Style:
            return PaintStyle::STROKE;
        case SkPaint::kStrokeAndFill_Style:
            return PaintStyle::FILL_STROKE;
        default:
            return PaintStyle::FILL;
    }
}

// ========== 描边设置 ==========

void Paint::SetStrokeWidth(float width) {
    paint_.setStrokeWidth(std::max(0.0f, width));
}

float Paint::GetStrokeWidth() const {
    return paint_.getStrokeWidth();
}

void Paint::SetStrokeCap(StrokeCap cap) {
    switch (cap) {
        case StrokeCap::BUTT:
            paint_.setStrokeCap(SkPaint::kButt_Cap);
            break;
        case StrokeCap::ROUND:
            paint_.setStrokeCap(SkPaint::kRound_Cap);
            break;
        case StrokeCap::SQUARE:
            paint_.setStrokeCap(SkPaint::kSquare_Cap);
            break;
    }
}

StrokeCap Paint::GetStrokeCap() const {
    switch (paint_.getStrokeCap()) {
        case SkPaint::kButt_Cap:
            return StrokeCap::BUTT;
        case SkPaint::kRound_Cap:
            return StrokeCap::ROUND;
        case SkPaint::kSquare_Cap:
            return StrokeCap::SQUARE;
        default:
            return StrokeCap::BUTT;
    }
}

void Paint::SetStrokeJoin(StrokeJoin join) {
    switch (join) {
        case StrokeJoin::MITER:
            paint_.setStrokeJoin(SkPaint::kMiter_Join);
            break;
        case StrokeJoin::ROUND:
            paint_.setStrokeJoin(SkPaint::kRound_Join);
            break;
        case StrokeJoin::BEVEL:
            paint_.setStrokeJoin(SkPaint::kBevel_Join);
            break;
    }
}

StrokeJoin Paint::GetStrokeJoin() const {
    switch (paint_.getStrokeJoin()) {
        case SkPaint::kMiter_Join:
            return StrokeJoin::MITER;
        case SkPaint::kRound_Join:
            return StrokeJoin::ROUND;
        case SkPaint::kBevel_Join:
            return StrokeJoin::BEVEL;
        default:
            return StrokeJoin::MITER;
    }
}

void Paint::SetStrokeMiter(float miter) {
    paint_.setStrokeMiter(std::max(0.0f, miter));
}

float Paint::GetStrokeMiter() const {
    return paint_.getStrokeMiter();
}

// ========== 抗锯齿设置 ==========

void Paint::SetAntiAlias(bool antiAlias) {
    paint_.setAntiAlias(antiAlias);
}

bool Paint::IsAntiAlias() const {
    return paint_.isAntiAlias();
}

// ========== 其他设置 ==========

void Paint::SetDither(bool dither) {
    paint_.setDither(dither);
}

bool Paint::IsDither() const {
    return paint_.isDither();
}

void Paint::Reset() {
    paint_.reset();
    // 恢复默认值
    paint_.setAntiAlias(true);
    paint_.setColor(SK_ColorBLACK);
    paint_.setStyle(SkPaint::kFill_Style);
}

} // namespace lightui
