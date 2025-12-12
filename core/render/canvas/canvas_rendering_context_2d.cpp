/**
 * @file canvas_rendering_context_2d.cpp
 * @brief Canvas 2D渲染上下文实现
 */

// 定义 M_PI（MSVC需要）
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "canvas_rendering_context_2d.h"
#include "canvas_gradient.h"
#include "canvas_image_data.h"
#include "core/render/color.h"
#include "core/render/render_object.h"
#include "include/core/SkData.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/encode/SkPngEncoder.h"
#include "include/encode/SkJpegEncoder.h"
#include "include/core/SkStream.h"
#include "include/core/SkPathEffect.h"
#include "include/effects/SkDashPathEffect.h"
#include <cmath>
#include <stdexcept>

namespace lightui {

// Base64 编码表
static const char kBase64Chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64编码函数
static std::string Base64Encode(const void* data, size_t length) {
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    std::string result;
    result.reserve(((length + 2) / 3) * 4);
    
    for (size_t i = 0; i < length; i += 3) {
        unsigned int n = bytes[i] << 16;
        if (i + 1 < length) n |= bytes[i + 1] << 8;
        if (i + 2 < length) n |= bytes[i + 2];
        
        result.push_back(kBase64Chars[(n >> 18) & 0x3F]);
        result.push_back(kBase64Chars[(n >> 12) & 0x3F]);
        result.push_back((i + 1 < length) ? kBase64Chars[(n >> 6) & 0x3F] : '=');
        result.push_back((i + 2 < length) ? kBase64Chars[n & 0x3F] : '=');
    }
    
    return result;
}

// ========== DrawingState 实现 ==========

CanvasRenderingContext2D::DrawingState::DrawingState() 
    : text_align("start")
    , text_baseline("alphabetic")
    , global_alpha(1.0)
    , line_dash_offset(0.0)
    , global_composite_operation("source-over")
    , shadow_color(SK_ColorTRANSPARENT)
    , shadow_blur(0.0)
    , shadow_offset_x(0.0)
    , shadow_offset_y(0.0) {
    // 初始化填充画笔
    fill_paint.setAntiAlias(true);
    fill_paint.setStyle(SkPaint::kFill_Style);
    fill_paint.setColor(SK_ColorBLACK);
    
    // 初始化描边画笔
    stroke_paint.setAntiAlias(true);
    stroke_paint.setStyle(SkPaint::kStroke_Style);
    stroke_paint.setColor(SK_ColorBLACK);
    stroke_paint.setStrokeWidth(1.0f);
    
    // 初始化字体
    font.setSize(10);
    
    // 初始化变换矩阵（单位矩阵）
    transform.reset();
}

// ========== 构造/析构 ==========

CanvasRenderingContext2D::CanvasRenderingContext2D(unsigned int width, unsigned int height)
    : width_(width)
    , height_(height) {
    // 创建离屏渲染 Surface
    surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height));
    if (!surface_) {
        throw std::runtime_error("Failed to create canvas surface");
    }
    
    current_state_ = DrawingState();
}

void CanvasRenderingContext2D::Resize(unsigned int width, unsigned int height) {
    width_ = width;
    height_ = height;
    
    // 重新创建 Surface（这会清空画布内容，符合HTML5标准）
    surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height));
    if (!surface_) {
        throw std::runtime_error("Failed to resize canvas surface");
    }
    
    // 重置状态
    current_state_ = DrawingState();
    state_stack_.clear();
    current_path_.reset();
}

std::string CanvasRenderingContext2D::ToDataURL(const std::string& type, double quality) {
    if (!surface_) {
        return "";
    }
    
    // 获取图像快照
    sk_sp<SkImage> image = surface_->makeImageSnapshot();
    if (!image) {
        return "";
    }
    
    // 编码为PNG或JPEG
    sk_sp<SkData> data;
    if (type == "image/jpeg" || type == "image/jpg") {
        SkJpegEncoder::Options options;
        options.fQuality = static_cast<int>(quality * 100);
        data = SkJpegEncoder::Encode(nullptr, image.get(), options);
    } else {
        // 默认使用PNG
        SkPngEncoder::Options options;
        data = SkPngEncoder::Encode(nullptr, image.get(), options);
    }
    
    if (!data) {
        return "";
    }
    
    // 转换为Base64
    std::string base64 = Base64Encode(data->data(), data->size());
    
    // 构建Data URL
    std::string mime_type = (type == "image/jpeg" || type == "image/jpg") ? "image/jpeg" : "image/png";
    return "data:" + mime_type + ";base64," + base64;
}

// ========== 矩形绘制 ==========

void CanvasRenderingContext2D::ClearRect(double x, double y, double width, double height) {
    if (!surface_) return;
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->concat(current_state_.transform);
    
    SkPaint clear_paint;
    clear_paint.setBlendMode(SkBlendMode::kClear);
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), clear_paint);
    
    canvas->restore();
}

void CanvasRenderingContext2D::FillRect(double x, double y, double width, double height) {
    if (!surface_) return;
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->concat(current_state_.transform);
    
    SkPaint paint = current_state_.fill_paint;
    ApplyGlobalAlpha(paint);
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);
    
    canvas->restore();
}

void CanvasRenderingContext2D::StrokeRect(double x, double y, double width, double height) {
    if (!surface_) return;
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->concat(current_state_.transform);
    
    SkPaint paint = current_state_.stroke_paint;
    ApplyGlobalAlpha(paint);
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);
    
    canvas->restore();
}

// ========== 路径 ==========

void CanvasRenderingContext2D::BeginPath() {
    current_path_.reset();
}

void CanvasRenderingContext2D::ClosePath() {
    current_path_.close();
}

void CanvasRenderingContext2D::MoveTo(double x, double y) {
    current_path_.moveTo(x, y);
}

void CanvasRenderingContext2D::LineTo(double x, double y) {
    current_path_.lineTo(x, y);
}

void CanvasRenderingContext2D::Arc(double x, double y, double radius, double startAngle, double endAngle, bool anticlockwise) {
    // 转换弧度为度数
    float startDeg = static_cast<float>(startAngle * 180.0 / M_PI);
    float endDeg = static_cast<float>(endAngle * 180.0 / M_PI);
    float sweepDeg = endDeg - startDeg;
    
    // 处理逆时针方向
    if (anticlockwise) {
        if (sweepDeg > 0) sweepDeg -= 360.0f;
    } else {
        if (sweepDeg < 0) sweepDeg += 360.0f;
    }
    
    SkRect oval = SkRect::MakeLTRB(
        static_cast<float>(x - radius), 
        static_cast<float>(y - radius), 
        static_cast<float>(x + radius), 
        static_cast<float>(y + radius)
    );
    
    // 检查是否是完整的圆（扫描角度接近360度）
    if (std::abs(std::abs(sweepDeg) - 360.0f) < 0.001f || 
        std::abs(endAngle - startAngle) >= 2 * M_PI - 0.001) {
        // 完整圆使用addOval
        current_path_.addOval(oval);
    } else {
        // 部分弧使用arcTo
        // forceMoveTo=false会在当前点和弧起点之间画线（HTML5标准行为）
        current_path_.arcTo(oval, startDeg, sweepDeg, false);
    }
}

void CanvasRenderingContext2D::ArcTo(double x1, double y1, double x2, double y2, double radius) {
    current_path_.arcTo(x1, y1, x2, y2, radius);
}

void CanvasRenderingContext2D::QuadraticCurveTo(double cpx, double cpy, double x, double y) {
    current_path_.quadTo(cpx, cpy, x, y);
}

void CanvasRenderingContext2D::BezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
    current_path_.cubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
}

void CanvasRenderingContext2D::Rect(double x, double y, double width, double height) {
    current_path_.addRect(SkRect::MakeXYWH(x, y, width, height));
}

void CanvasRenderingContext2D::Fill() {
    if (!surface_) return;
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->concat(current_state_.transform);
    
    SkPaint paint = current_state_.fill_paint;
    ApplyGlobalAlpha(paint);
    canvas->drawPath(current_path_, paint);
    
    canvas->restore();
}

void CanvasRenderingContext2D::Stroke() {
    if (!surface_) return;
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->concat(current_state_.transform);
    
    SkPaint paint = current_state_.stroke_paint;
    ApplyGlobalAlpha(paint);
    canvas->drawPath(current_path_, paint);
    
    canvas->restore();
}

void CanvasRenderingContext2D::Clip() {
    if (!surface_) return;
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->clipPath(current_path_, SkClipOp::kIntersect, true);
}

void CanvasRenderingContext2D::Ellipse(double x, double y, double radiusX, double radiusY, 
                                        double rotation, double startAngle, double endAngle, bool anticlockwise) {
    // 创建椭圆路径
    SkPath ellipse_path;
    SkRect oval = SkRect::MakeLTRB(x - radiusX, y - radiusY, x + radiusX, y + radiusY);
    
    // 计算起始和扫描角度
    float startDeg = startAngle * 180.0f / M_PI;
    float endDeg = endAngle * 180.0f / M_PI;
    float sweepDeg = endDeg - startDeg;
    
    if (anticlockwise) {
        if (sweepDeg > 0) sweepDeg -= 360.0f;
    } else {
        if (sweepDeg < 0) sweepDeg += 360.0f;
    }
    
    // 如果是完整椭圆
    if (std::abs(std::abs(sweepDeg) - 360.0f) < 0.001f) {
        ellipse_path.addOval(oval);
    } else {
        ellipse_path.arcTo(oval, startDeg, sweepDeg, false);
    }
    
    // 应用旋转变换
    if (rotation != 0) {
        SkMatrix rotate_matrix;
        rotate_matrix.setRotate(rotation * 180.0f / M_PI, x, y);
        ellipse_path.transform(rotate_matrix);
    }
    
    // 添加到当前路径
    current_path_.addPath(ellipse_path);
}

// ========== 点击检测 ==========

bool CanvasRenderingContext2D::IsPointInPath(double x, double y) {
    // 应用当前变换的逆变换到点坐标
    SkMatrix inverse;
    if (!current_state_.transform.invert(&inverse)) {
        return false;
    }
    
    SkPoint pt = SkPoint::Make(x, y);
    inverse.mapPoints(&pt, 1);
    
    return current_path_.contains(pt.x(), pt.y());
}

bool CanvasRenderingContext2D::IsPointInStroke(double x, double y) {
    // 应用当前变换的逆变换到点坐标
    SkMatrix inverse;
    if (!current_state_.transform.invert(&inverse)) {
        return false;
    }
    
    SkPoint pt = SkPoint::Make(x, y);
    inverse.mapPoints(&pt, 1);
    
    // 简化实现：检查点是否在路径边界框附近
    // 注意：这是一个近似实现，完整实现需要使用路径描边后的hit test
    SkRect bounds = current_path_.getBounds();
    float strokeWidth = current_state_.stroke_paint.getStrokeWidth();
    bounds.outset(strokeWidth / 2, strokeWidth / 2);
    
    return bounds.contains(pt.x(), pt.y());
}

// ========== 文本 ==========

void CanvasRenderingContext2D::FillText(const std::string& text, double x, double y) {
    if (!surface_) return;
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->concat(current_state_.transform);
    
    SkPaint paint = current_state_.fill_paint;
    ApplyGlobalAlpha(paint);
    
    canvas->drawString(text.c_str(), x, y, current_state_.font, paint);
    
    canvas->restore();
}

void CanvasRenderingContext2D::StrokeText(const std::string& text, double x, double y) {
    if (!surface_) return;
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->concat(current_state_.transform);
    
    SkPaint paint = current_state_.stroke_paint;
    ApplyGlobalAlpha(paint);
    
    canvas->drawString(text.c_str(), x, y, current_state_.font, paint);
    
    canvas->restore();
}

double CanvasRenderingContext2D::MeasureText(const std::string& text) {
    return current_state_.font.measureText(text.c_str(), text.length(), SkTextEncoding::kUTF8);
}

// ========== 样式属性 ==========

void CanvasRenderingContext2D::SetFillStyle(const std::string& color) {
    try {
        current_state_.fill_paint.setColor(ParseColor(color));
        current_state_.fill_paint.setShader(nullptr);  // 清除之前的渐变
    } catch (const std::exception& e) {
        std::cerr << "Error in SetFillStyle: " << e.what() << std::endl;
        current_state_.fill_paint.setColor(SK_ColorBLACK);
    }
}

void CanvasRenderingContext2D::SetFillStyle(CanvasGradient* gradient) {
    if (gradient) {
        current_state_.fill_paint.setShader(gradient->GetShader());
    }
}

void CanvasRenderingContext2D::SetStrokeStyle(const std::string& color) {
    current_state_.stroke_paint.setColor(ParseColor(color));
    current_state_.stroke_paint.setShader(nullptr);  // 清除之前的渐变
}

void CanvasRenderingContext2D::SetStrokeStyle(CanvasGradient* gradient) {
    if (gradient) {
        current_state_.stroke_paint.setShader(gradient->GetShader());
    }
}

CanvasGradient* CanvasRenderingContext2D::CreateLinearGradient(double x0, double y0, double x1, double y1) {
    return CanvasGradient::CreateLinear(x0, y0, x1, y1);
}

CanvasGradient* CanvasRenderingContext2D::CreateRadialGradient(double x0, double y0, double r0, double x1, double y1, double r1) {
    return CanvasGradient::CreateRadial(x0, y0, r0, x1, y1, r1);
}

void CanvasRenderingContext2D::SetLineWidth(double width) {
    current_state_.stroke_paint.setStrokeWidth(width);
}

void CanvasRenderingContext2D::SetLineCap(const std::string& cap) {
    if (cap == "butt") {
        current_state_.stroke_paint.setStrokeCap(SkPaint::kButt_Cap);
    } else if (cap == "round") {
        current_state_.stroke_paint.setStrokeCap(SkPaint::kRound_Cap);
    } else if (cap == "square") {
        current_state_.stroke_paint.setStrokeCap(SkPaint::kSquare_Cap);
    }
}

void CanvasRenderingContext2D::SetLineJoin(const std::string& join) {
    if (join == "miter") {
        current_state_.stroke_paint.setStrokeJoin(SkPaint::kMiter_Join);
    } else if (join == "round") {
        current_state_.stroke_paint.setStrokeJoin(SkPaint::kRound_Join);
    } else if (join == "bevel") {
        current_state_.stroke_paint.setStrokeJoin(SkPaint::kBevel_Join);
    }
}

void CanvasRenderingContext2D::SetMiterLimit(double limit) {
    current_state_.stroke_paint.setStrokeMiter(limit);
}

void CanvasRenderingContext2D::SetFont(const std::string& font) {
    // 简单解析：提取字号（如 "12px Arial"）
    size_t px_pos = font.find("px");
    if (px_pos != std::string::npos) {
        try {
            std::string size_str = font.substr(0, px_pos);
            // 去除空格
            size_str.erase(0, size_str.find_first_not_of(" \t"));
            size_str.erase(size_str.find_last_not_of(" \t") + 1);
            float size = std::stof(size_str);
            current_state_.font.setSize(size);
        } catch (...) {
            // 解析失败，保持当前字体大小
        }
    }
}

void CanvasRenderingContext2D::SetTextAlign(const std::string& align) {
    current_state_.text_align = align;
}

void CanvasRenderingContext2D::SetTextBaseline(const std::string& baseline) {
    current_state_.text_baseline = baseline;
}

void CanvasRenderingContext2D::SetGlobalAlpha(double alpha) {
    current_state_.global_alpha = std::clamp(alpha, 0.0, 1.0);
}

std::string CanvasRenderingContext2D::GetFillStyle() const {
    return Color::ToHex(current_state_.fill_paint.getColor(), false);
}

std::string CanvasRenderingContext2D::GetStrokeStyle() const {
    return Color::ToHex(current_state_.stroke_paint.getColor(), false);
}

double CanvasRenderingContext2D::GetLineWidth() const {
    return current_state_.stroke_paint.getStrokeWidth();
}

std::string CanvasRenderingContext2D::GetLineCap() const {
    switch (current_state_.stroke_paint.getStrokeCap()) {
        case SkPaint::kRound_Cap: return "round";
        case SkPaint::kSquare_Cap: return "square";
        default: return "butt";
    }
}

std::string CanvasRenderingContext2D::GetLineJoin() const {
    switch (current_state_.stroke_paint.getStrokeJoin()) {
        case SkPaint::kRound_Join: return "round";
        case SkPaint::kBevel_Join: return "bevel";
        default: return "miter";
    }
}

double CanvasRenderingContext2D::GetMiterLimit() const {
    return current_state_.stroke_paint.getStrokeMiter();
}

std::string CanvasRenderingContext2D::GetFont() const {
    return std::to_string(static_cast<int>(current_state_.font.getSize())) + "px";
}

std::string CanvasRenderingContext2D::GetTextAlign() const {
    return current_state_.text_align;
}

std::string CanvasRenderingContext2D::GetTextBaseline() const {
    return current_state_.text_baseline;
}

double CanvasRenderingContext2D::GetGlobalAlpha() const {
    return current_state_.global_alpha;
}

// ========== 虚线 ==========

void CanvasRenderingContext2D::SetLineDash(const std::vector<double>& segments) {
    current_state_.line_dash = segments;
    
    // 应用到Skia画笔
    if (segments.empty()) {
        current_state_.stroke_paint.setPathEffect(nullptr);
    } else {
        std::vector<SkScalar> intervals;
        for (double s : segments) {
            intervals.push_back(static_cast<SkScalar>(s));
        }
        // 如果段数为奇数，需要重复一次（Canvas规范）
        if (intervals.size() % 2 != 0) {
            size_t n = intervals.size();
            for (size_t i = 0; i < n; i++) {
                intervals.push_back(intervals[i]);
            }
        }
        current_state_.stroke_paint.setPathEffect(
            SkDashPathEffect::Make(intervals.data(), intervals.size(), current_state_.line_dash_offset)
        );
    }
}

std::vector<double> CanvasRenderingContext2D::GetLineDash() const {
    return current_state_.line_dash;
}

void CanvasRenderingContext2D::SetLineDashOffset(double offset) {
    current_state_.line_dash_offset = offset;
    // 重新应用虚线效果
    if (!current_state_.line_dash.empty()) {
        SetLineDash(current_state_.line_dash);
    }
}

double CanvasRenderingContext2D::GetLineDashOffset() const {
    return current_state_.line_dash_offset;
}

// ========== 合成与阴影 ==========

void CanvasRenderingContext2D::SetGlobalCompositeOperation(const std::string& op) {
    current_state_.global_composite_operation = op;
    // 设置Skia混合模式
    SkBlendMode mode = SkBlendMode::kSrcOver;
    if (op == "source-over") mode = SkBlendMode::kSrcOver;
    else if (op == "source-in") mode = SkBlendMode::kSrcIn;
    else if (op == "source-out") mode = SkBlendMode::kSrcOut;
    else if (op == "source-atop") mode = SkBlendMode::kSrcATop;
    else if (op == "destination-over") mode = SkBlendMode::kDstOver;
    else if (op == "destination-in") mode = SkBlendMode::kDstIn;
    else if (op == "destination-out") mode = SkBlendMode::kDstOut;
    else if (op == "destination-atop") mode = SkBlendMode::kDstATop;
    else if (op == "lighter") mode = SkBlendMode::kPlus;
    else if (op == "copy") mode = SkBlendMode::kSrc;
    else if (op == "xor") mode = SkBlendMode::kXor;
    else if (op == "multiply") mode = SkBlendMode::kMultiply;
    else if (op == "screen") mode = SkBlendMode::kScreen;
    else if (op == "overlay") mode = SkBlendMode::kOverlay;
    else if (op == "darken") mode = SkBlendMode::kDarken;
    else if (op == "lighten") mode = SkBlendMode::kLighten;
    else if (op == "color-dodge") mode = SkBlendMode::kColorDodge;
    else if (op == "color-burn") mode = SkBlendMode::kColorBurn;
    else if (op == "hard-light") mode = SkBlendMode::kHardLight;
    else if (op == "soft-light") mode = SkBlendMode::kSoftLight;
    else if (op == "difference") mode = SkBlendMode::kDifference;
    else if (op == "exclusion") mode = SkBlendMode::kExclusion;
    else if (op == "hue") mode = SkBlendMode::kHue;
    else if (op == "saturation") mode = SkBlendMode::kSaturation;
    else if (op == "color") mode = SkBlendMode::kColor;
    else if (op == "luminosity") mode = SkBlendMode::kLuminosity;
    
    current_state_.fill_paint.setBlendMode(mode);
    current_state_.stroke_paint.setBlendMode(mode);
}

std::string CanvasRenderingContext2D::GetGlobalCompositeOperation() const {
    return current_state_.global_composite_operation;
}

void CanvasRenderingContext2D::SetShadowColor(const std::string& color) {
    current_state_.shadow_color = ParseColor(color);
}

std::string CanvasRenderingContext2D::GetShadowColor() const {
    // 返回rgba格式
    int r = SkColorGetR(current_state_.shadow_color);
    int g = SkColorGetG(current_state_.shadow_color);
    int b = SkColorGetB(current_state_.shadow_color);
    int a = SkColorGetA(current_state_.shadow_color);
    char buf[64];
    snprintf(buf, sizeof(buf), "rgba(%d, %d, %d, %.2f)", r, g, b, a / 255.0);
    return buf;
}

void CanvasRenderingContext2D::SetShadowBlur(double blur) {
    current_state_.shadow_blur = blur;
}

double CanvasRenderingContext2D::GetShadowBlur() const {
    return current_state_.shadow_blur;
}

void CanvasRenderingContext2D::SetShadowOffsetX(double offset) {
    current_state_.shadow_offset_x = offset;
}

double CanvasRenderingContext2D::GetShadowOffsetX() const {
    return current_state_.shadow_offset_x;
}

void CanvasRenderingContext2D::SetShadowOffsetY(double offset) {
    current_state_.shadow_offset_y = offset;
}

double CanvasRenderingContext2D::GetShadowOffsetY() const {
    return current_state_.shadow_offset_y;
}

// ========== 变换 ==========

void CanvasRenderingContext2D::Scale(double x, double y) {
    current_state_.transform.preScale(x, y);
}

void CanvasRenderingContext2D::Rotate(double angle) {
    current_state_.transform.preRotate(angle * 180.0 / M_PI);
}

void CanvasRenderingContext2D::Translate(double x, double y) {
    current_state_.transform.preTranslate(x, y);
}

void CanvasRenderingContext2D::Transform(double a, double b, double c, double d, double e, double f) {
    SkMatrix matrix;
    matrix.setAll(a, c, e, b, d, f, 0, 0, 1);
    current_state_.transform.preConcat(matrix);
}

void CanvasRenderingContext2D::SetTransform(double a, double b, double c, double d, double e, double f) {
    current_state_.transform.setAll(a, c, e, b, d, f, 0, 0, 1);
}

void CanvasRenderingContext2D::ResetTransform() {
    current_state_.transform.reset();
}

// ========== 状态管理 ==========

void CanvasRenderingContext2D::Save() {
    state_stack_.push_back(current_state_);
    if (surface_) {
        surface_->getCanvas()->save();
    }
}

void CanvasRenderingContext2D::Restore() {
    if (!state_stack_.empty()) {
        current_state_ = state_stack_.back();
        state_stack_.pop_back();
        if (surface_) {
            surface_->getCanvas()->restore();
        }
    }
}

// ========== 图像绘制 ==========

void CanvasRenderingContext2D::DrawImage(void* image, double dx, double dy) {
    if (!surface_ || !image) return;
    
    SkImage* sk_image = static_cast<SkImage*>(image);
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->concat(current_state_.transform);
    
    canvas->drawImage(sk_sp<SkImage>(sk_image), dx, dy);
    
    canvas->restore();
}

void CanvasRenderingContext2D::DrawImage(void* image, double dx, double dy, double dwidth, double dheight) {
    if (!surface_ || !image) return;
    
    SkImage* sk_image = static_cast<SkImage*>(image);
    
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->concat(current_state_.transform);
    
    SkRect dest = SkRect::MakeXYWH(dx, dy, dwidth, dheight);
    canvas->drawImageRect(sk_sp<SkImage>(sk_image), dest, SkSamplingOptions());
    
    canvas->restore();
}

// ========== 像素操作 ==========

ImageData* CanvasRenderingContext2D::CreateImageData(unsigned int width, unsigned int height) {
    return new ImageData(width, height);
}

ImageData* CanvasRenderingContext2D::GetImageData(int x, int y, unsigned int width, unsigned int height) {
    if (!surface_) return new ImageData(width, height);
    
    // 创建结果ImageData
    ImageData* imageData = new ImageData(width, height);
    auto& data = imageData->GetData();
    
    // 从Surface读取像素
    SkImageInfo info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
    surface_->readPixels(info, data.data(), width * 4, x, y);
    
    return imageData;
}

void CanvasRenderingContext2D::PutImageData(ImageData* imageData, int dx, int dy) {
    if (!surface_ || !imageData) return;
    
    unsigned int width = imageData->GetWidth();
    unsigned int height = imageData->GetHeight();
    const auto& data = imageData->GetData();
    
    // 创建SkPixmap
    SkImageInfo info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
    SkPixmap pixmap(info, data.data(), width * 4);
    
    // 写入到Surface
    surface_->writePixels(pixmap, dx, dy);
}

// ========== 私有方法 ==========

SkColor CanvasRenderingContext2D::ParseColor(const std::string& color_str) {
    return Color::Parse(color_str);
}

void CanvasRenderingContext2D::ApplyGlobalAlpha(SkPaint& paint) {
    if (current_state_.global_alpha < 1.0) {
        SkColor color = paint.getColor();
        int alpha = SkColorGetA(color);
        paint.setAlpha(static_cast<uint8_t>(alpha * current_state_.global_alpha));
    }
}

} // namespace lightui
