/**
 * @file canvas_bindings.cpp
 * @brief Canvas JavaScript 绑定实现
 */

#include "canvas_bindings.h"
#include "dom_bindings.h"
#include "html_image_element.h"
#include "html_canvas_element.h"
#include "quickjs/quickjs-libc.h"
#include "core/quickjs/bindings/js_element.h"
#include <iostream>

namespace lightui {

// 静态成员初始化
JSClassID CanvasBindings::context_2d_class_id = 0;
bool CanvasBindings::initialized = false;

// ========== CanvasRenderingContext2D 绑定 ==========

// Context2D finalizer
static void js_context_2d_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<CanvasRenderingContext2D*>(JS_GetOpaque(val, CanvasBindings::context_2d_class_id));
    // Context2D 由 HTMLCanvasElement 管理，这里不需要 delete
    // 只是清理opaque指针即可
}

// ========== Context2D 属性 getters/setters ==========

// fillStyle getter
static JSValue js_context_2d_get_fill_style(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewString(ctx, context->GetFillStyle().c_str());
}

// fillStyle setter (supports string color or CanvasGradient or CanvasPattern)
static JSValue js_context_2d_set_fill_style(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    // 检查是否是渐变对象
    if (JS_IsObject(val)) {
        auto gradient = CanvasBindings::UnwrapGradient(ctx, val);
        if (gradient) {
            context->SetFillStyle(gradient);
            return JS_UNDEFINED;
        }
        
        // 检查是否是图案对象
        auto pattern = CanvasBindings::UnwrapPattern(ctx, val);
        if (pattern) {
            context->SetFillStyle(pattern);
            return JS_UNDEFINED;
        }
    }
    
    // 否则作为颜色字符串处理
    const char* style = JS_ToCString(ctx, val);
    if (!style) return JS_EXCEPTION;
    
    context->SetFillStyle(style);
    JS_FreeCString(ctx, style);
    return JS_UNDEFINED;
}

// strokeStyle getter
static JSValue js_context_2d_get_stroke_style(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewString(ctx, context->GetStrokeStyle().c_str());
}

// strokeStyle setter (supports string color or CanvasGradient or CanvasPattern)
static JSValue js_context_2d_set_stroke_style(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    // 检查是否是渐变对象
    if (JS_IsObject(val)) {
        auto gradient = CanvasBindings::UnwrapGradient(ctx, val);
        if (gradient) {
            context->SetStrokeStyle(gradient);
            return JS_UNDEFINED;
        }
        
        // 检查是否是图案对象
        auto pattern = CanvasBindings::UnwrapPattern(ctx, val);
        if (pattern) {
            context->SetStrokeStyle(pattern);
            return JS_UNDEFINED;
        }
    }
    
    // 否则作为颜色字符串处理
    const char* style = JS_ToCString(ctx, val);
    if (!style) return JS_EXCEPTION;
    
    context->SetStrokeStyle(style);
    JS_FreeCString(ctx, style);
    return JS_UNDEFINED;
}

// lineWidth getter/setter
static JSValue js_context_2d_get_line_width(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewFloat64(ctx, context->GetLineWidth());
}

static JSValue js_context_2d_set_line_width(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    double width;
    if (JS_ToFloat64(ctx, &width, val) != 0) return JS_EXCEPTION;
    
    context->SetLineWidth(width);
    return JS_UNDEFINED;
}

// lineCap getter/setter
static JSValue js_context_2d_get_line_cap(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewString(ctx, context->GetLineCap().c_str());
}

static JSValue js_context_2d_set_line_cap(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    const char* cap = JS_ToCString(ctx, val);
    if (!cap) return JS_EXCEPTION;
    
    context->SetLineCap(cap);
    JS_FreeCString(ctx, cap);
    return JS_UNDEFINED;
}

// lineJoin getter/setter
static JSValue js_context_2d_get_line_join(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewString(ctx, context->GetLineJoin().c_str());
}

static JSValue js_context_2d_set_line_join(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    const char* join = JS_ToCString(ctx, val);
    if (!join) return JS_EXCEPTION;
    
    context->SetLineJoin(join);
    JS_FreeCString(ctx, join);
    return JS_UNDEFINED;
}

// miterLimit getter/setter
static JSValue js_context_2d_get_miter_limit(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewFloat64(ctx, context->GetMiterLimit());
}

static JSValue js_context_2d_set_miter_limit(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    double limit;
    if (JS_ToFloat64(ctx, &limit, val) != 0) return JS_EXCEPTION;
    
    context->SetMiterLimit(limit);
    return JS_UNDEFINED;
}

// lineDashOffset getter/setter
static JSValue js_context_2d_get_line_dash_offset(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewFloat64(ctx, context->GetLineDashOffset());
}

static JSValue js_context_2d_set_line_dash_offset(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    double offset;
    if (JS_ToFloat64(ctx, &offset, val) != 0) return JS_EXCEPTION;
    
    context->SetLineDashOffset(offset);
    return JS_UNDEFINED;
}

// setLineDash(segments)
static JSValue js_context_2d_set_line_dash(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 1) return JS_ThrowTypeError(ctx, "setLineDash requires 1 argument");
    
    // 通过获取length属性来检查是否类似数组
    JSValue length_val = JS_GetPropertyStr(ctx, argv[0], "length");
    if (JS_IsUndefined(length_val)) {
        return JS_ThrowTypeError(ctx, "setLineDash argument must be an array");
    }
    
    int64_t length;
    if (JS_ToInt64(ctx, &length, length_val) != 0) {
        JS_FreeValue(ctx, length_val);
        return JS_ThrowTypeError(ctx, "setLineDash argument must be an array");
    }
    JS_FreeValue(ctx, length_val);
    
    std::vector<double> segments;
    for (int64_t i = 0; i < length; i++) {
        JSValue elem = JS_GetPropertyUint32(ctx, argv[0], i);
        double val;
        if (JS_ToFloat64(ctx, &val, elem) == 0) {
            segments.push_back(val);
        }
        JS_FreeValue(ctx, elem);
    }
    
    context->SetLineDash(segments);
    return JS_UNDEFINED;
}

// getLineDash()
static JSValue js_context_2d_get_line_dash(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    std::vector<double> segments = context->GetLineDash();
    JSValue arr = JS_NewArray(ctx);
    
    for (size_t i = 0; i < segments.size(); i++) {
        JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, segments[i]));
    }
    
    return arr;
}

static JSValue js_context_2d_get_global_alpha(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewFloat64(ctx, context->GetGlobalAlpha());
}

static JSValue js_context_2d_set_global_alpha(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    double alpha;
    if (JS_ToFloat64(ctx, &alpha, val) != 0) return JS_EXCEPTION;
    
    context->SetGlobalAlpha(alpha);
    return JS_UNDEFINED;
}

// globalCompositeOperation getter/setter
static JSValue js_context_2d_get_global_composite_operation(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewString(ctx, context->GetGlobalCompositeOperation().c_str());
}

static JSValue js_context_2d_set_global_composite_operation(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    const char* op = JS_ToCString(ctx, val);
    if (!op) return JS_EXCEPTION;
    
    context->SetGlobalCompositeOperation(op);
    JS_FreeCString(ctx, op);
    return JS_UNDEFINED;
}

// shadowColor getter/setter
static JSValue js_context_2d_get_shadow_color(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewString(ctx, context->GetShadowColor().c_str());
}

static JSValue js_context_2d_set_shadow_color(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    const char* color = JS_ToCString(ctx, val);
    if (!color) return JS_EXCEPTION;
    
    context->SetShadowColor(color);
    JS_FreeCString(ctx, color);
    return JS_UNDEFINED;
}

// shadowBlur getter/setter
static JSValue js_context_2d_get_shadow_blur(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewFloat64(ctx, context->GetShadowBlur());
}

static JSValue js_context_2d_set_shadow_blur(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    double blur;
    if (JS_ToFloat64(ctx, &blur, val) != 0) return JS_EXCEPTION;
    
    context->SetShadowBlur(blur);
    return JS_UNDEFINED;
}

// shadowOffsetX getter/setter
static JSValue js_context_2d_get_shadow_offset_x(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewFloat64(ctx, context->GetShadowOffsetX());
}

static JSValue js_context_2d_set_shadow_offset_x(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    double offset;
    if (JS_ToFloat64(ctx, &offset, val) != 0) return JS_EXCEPTION;
    
    context->SetShadowOffsetX(offset);
    return JS_UNDEFINED;
}

// shadowOffsetY getter/setter
static JSValue js_context_2d_get_shadow_offset_y(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewFloat64(ctx, context->GetShadowOffsetY());
}

static JSValue js_context_2d_set_shadow_offset_y(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    double offset;
    if (JS_ToFloat64(ctx, &offset, val) != 0) return JS_EXCEPTION;
    
    context->SetShadowOffsetY(offset);
    return JS_UNDEFINED;
}

// ========== Context2D 矩形方法 ==========

// clearRect(x, y, width, height)
static JSValue js_context_2d_clear_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 4) return JS_ThrowTypeError(ctx, "clearRect requires 4 arguments");
    
    double x, y, width, height;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &width, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &height, argv[3]) != 0) return JS_EXCEPTION;
    
    context->ClearRect(x, y, width, height);
    return JS_UNDEFINED;
}

// fillRect(x, y, width, height)
static JSValue js_context_2d_fill_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 4) return JS_ThrowTypeError(ctx, "fillRect requires 4 arguments");
    
    double x, y, width, height;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &width, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &height, argv[3]) != 0) return JS_EXCEPTION;
    
    context->FillRect(x, y, width, height);
    return JS_UNDEFINED;
}

// strokeRect(x, y, width, height)
static JSValue js_context_2d_stroke_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 4) return JS_ThrowTypeError(ctx, "strokeRect requires 4 arguments");
    
    double x, y, width, height;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &width, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &height, argv[3]) != 0) return JS_EXCEPTION;
    
    context->StrokeRect(x, y, width, height);
    return JS_UNDEFINED;
}

// ========== Context2D 路径方法 ==========

// beginPath()
static JSValue js_context_2d_begin_path(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    context->BeginPath();
    return JS_UNDEFINED;
}

// closePath()
static JSValue js_context_2d_close_path(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    context->ClosePath();
    return JS_UNDEFINED;
}

// moveTo(x, y)
static JSValue js_context_2d_move_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 2) return JS_ThrowTypeError(ctx, "moveTo requires 2 arguments");
    
    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    
    context->MoveTo(x, y);
    return JS_UNDEFINED;
}

// lineTo(x, y)
static JSValue js_context_2d_line_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 2) return JS_ThrowTypeError(ctx, "lineTo requires 2 arguments");
    
    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    
    context->LineTo(x, y);
    return JS_UNDEFINED;
}

// arc(x, y, radius, startAngle, endAngle, anticlockwise)
static JSValue js_context_2d_arc(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 5) return JS_ThrowTypeError(ctx, "arc requires at least 5 arguments");
    
    double x, y, radius, startAngle, endAngle;
    bool anticlockwise = false;
    
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &radius, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &startAngle, argv[3]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &endAngle, argv[4]) != 0) return JS_EXCEPTION;
    if (argc >= 6) anticlockwise = JS_ToBool(ctx, argv[5]);
    
    context->Arc(x, y, radius, startAngle, endAngle, anticlockwise);
    return JS_UNDEFINED;
}

// fill()
static JSValue js_context_2d_fill(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    context->Fill();
    return JS_UNDEFINED;
}

// stroke()
static JSValue js_context_2d_stroke(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    context->Stroke();
    return JS_UNDEFINED;
}

// rect(x, y, width, height)
static JSValue js_context_2d_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 4) return JS_ThrowTypeError(ctx, "rect requires 4 arguments");
    
    double x, y, w, h;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &w, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &h, argv[3]) != 0) return JS_EXCEPTION;
    
    context->Rect(x, y, w, h);
    return JS_UNDEFINED;
}

// arcTo(x1, y1, x2, y2, radius)
static JSValue js_context_2d_arc_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 5) return JS_ThrowTypeError(ctx, "arcTo requires 5 arguments");
    
    double x1, y1, x2, y2, radius;
    if (JS_ToFloat64(ctx, &x1, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y1, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &x2, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y2, argv[3]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &radius, argv[4]) != 0) return JS_EXCEPTION;
    
    context->ArcTo(x1, y1, x2, y2, radius);
    return JS_UNDEFINED;
}

// quadraticCurveTo(cpx, cpy, x, y)
static JSValue js_context_2d_quadratic_curve_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 4) return JS_ThrowTypeError(ctx, "quadraticCurveTo requires 4 arguments");
    
    double cpx, cpy, x, y;
    if (JS_ToFloat64(ctx, &cpx, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &cpy, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &x, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[3]) != 0) return JS_EXCEPTION;
    
    context->QuadraticCurveTo(cpx, cpy, x, y);
    return JS_UNDEFINED;
}

// bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y)
static JSValue js_context_2d_bezier_curve_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 6) return JS_ThrowTypeError(ctx, "bezierCurveTo requires 6 arguments");
    
    double cp1x, cp1y, cp2x, cp2y, x, y;
    if (JS_ToFloat64(ctx, &cp1x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &cp1y, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &cp2x, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &cp2y, argv[3]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &x, argv[4]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[5]) != 0) return JS_EXCEPTION;
    
    context->BezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
    return JS_UNDEFINED;
}

// ellipse(x, y, radiusX, radiusY, rotation, startAngle, endAngle, anticlockwise)
static JSValue js_context_2d_ellipse(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 7) return JS_ThrowTypeError(ctx, "ellipse requires at least 7 arguments");
    
    double x, y, radiusX, radiusY, rotation, startAngle, endAngle;
    bool anticlockwise = false;
    
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &radiusX, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &radiusY, argv[3]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &rotation, argv[4]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &startAngle, argv[5]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &endAngle, argv[6]) != 0) return JS_EXCEPTION;
    if (argc >= 8) anticlockwise = JS_ToBool(ctx, argv[7]);
    
    context->Ellipse(x, y, radiusX, radiusY, rotation, startAngle, endAngle, anticlockwise);
    return JS_UNDEFINED;
}

// roundRect(x, y, width, height, radii)
static JSValue js_context_2d_round_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 5) return JS_ThrowTypeError(ctx, "roundRect requires 5 arguments");
    
    double x, y, width, height, radius;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &width, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &height, argv[3]) != 0) return JS_EXCEPTION;
    
    // 第5个参数可以是数字或数组，这里简化处理只支持单个数字
    if (JS_ToFloat64(ctx, &radius, argv[4]) != 0) return JS_EXCEPTION;
    
    context->RoundRect(x, y, width, height, radius);
    return JS_UNDEFINED;
}

// clip()
static JSValue js_context_2d_clip(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    context->Clip();
    return JS_UNDEFINED;
}

// isPointInPath(x, y)
static JSValue js_context_2d_is_point_in_path(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 2) return JS_ThrowTypeError(ctx, "isPointInPath requires 2 arguments");
    
    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    
    return JS_NewBool(ctx, context->IsPointInPath(x, y));
}

// isPointInStroke(x, y)
static JSValue js_context_2d_is_point_in_stroke(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 2) return JS_ThrowTypeError(ctx, "isPointInStroke requires 2 arguments");
    
    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    
    return JS_NewBool(ctx, context->IsPointInStroke(x, y));
}

// ========== Context2D 文本方法 ==========

// fillText(text, x, y)
static JSValue js_context_2d_fill_text(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 3) return JS_ThrowTypeError(ctx, "fillText requires 3 arguments");
    
    const char* text = JS_ToCString(ctx, argv[0]);
    if (!text) return JS_EXCEPTION;
    
    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[1]) != 0) {
        JS_FreeCString(ctx, text);
        return JS_EXCEPTION;
    }
    if (JS_ToFloat64(ctx, &y, argv[2]) != 0) {
        JS_FreeCString(ctx, text);
        return JS_EXCEPTION;
    }
    
    context->FillText(text, x, y);
    JS_FreeCString(ctx, text);
    return JS_UNDEFINED;
}

// strokeText(text, x, y)
static JSValue js_context_2d_stroke_text(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 3) return JS_ThrowTypeError(ctx, "strokeText requires 3 arguments");
    
    const char* text = JS_ToCString(ctx, argv[0]);
    if (!text) return JS_EXCEPTION;
    
    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[1]) != 0) {
        JS_FreeCString(ctx, text);
        return JS_EXCEPTION;
    }
    if (JS_ToFloat64(ctx, &y, argv[2]) != 0) {
        JS_FreeCString(ctx, text);
        return JS_EXCEPTION;
    }
    
    context->StrokeText(text, x, y);
    JS_FreeCString(ctx, text);
    return JS_UNDEFINED;
}

// measureText(text) - returns object with width property
static JSValue js_context_2d_measure_text(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 1) return JS_ThrowTypeError(ctx, "measureText requires 1 argument");
    
    const char* text = JS_ToCString(ctx, argv[0]);
    if (!text) return JS_EXCEPTION;
    
    double width = context->MeasureText(text);
    JS_FreeCString(ctx, text);
    
    // 返回TextMetrics对象 { width: number }
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "width", JS_NewFloat64(ctx, width));
    return result;
}

// font getter/setter
static JSValue js_context_2d_get_font(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewString(ctx, context->GetFont().c_str());
}

static JSValue js_context_2d_set_font(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    const char* font = JS_ToCString(ctx, val);
    if (!font) return JS_EXCEPTION;
    
    context->SetFont(font);
    JS_FreeCString(ctx, font);
    return JS_UNDEFINED;
}

// textAlign getter/setter
static JSValue js_context_2d_get_text_align(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewString(ctx, context->GetTextAlign().c_str());
}

static JSValue js_context_2d_set_text_align(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    const char* align = JS_ToCString(ctx, val);
    if (!align) return JS_EXCEPTION;
    
    context->SetTextAlign(align);
    JS_FreeCString(ctx, align);
    return JS_UNDEFINED;
}

// textBaseline getter/setter
static JSValue js_context_2d_get_text_baseline(JSContext* ctx, JSValueConst this_val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    return JS_NewString(ctx, context->GetTextBaseline().c_str());
}

static JSValue js_context_2d_set_text_baseline(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    const char* baseline = JS_ToCString(ctx, val);
    if (!baseline) return JS_EXCEPTION;
    
    context->SetTextBaseline(baseline);
    JS_FreeCString(ctx, baseline);
    return JS_UNDEFINED;
}

// ========== Context2D 渐变方法 ==========

// createLinearGradient(x0, y0, x1, y1)
static JSValue js_context_2d_create_linear_gradient(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 4) return JS_ThrowTypeError(ctx, "createLinearGradient requires 4 arguments");
    
    double x0, y0, x1, y1;
    if (JS_ToFloat64(ctx, &x0, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y0, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &x1, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y1, argv[3]) != 0) return JS_EXCEPTION;
    
    CanvasGradient* gradient = context->CreateLinearGradient(x0, y0, x1, y1);
    return CanvasBindings::WrapGradient(ctx, gradient);
}

// createRadialGradient(x0, y0, r0, x1, y1, r1)
static JSValue js_context_2d_create_radial_gradient(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 6) return JS_ThrowTypeError(ctx, "createRadialGradient requires 6 arguments");
    
    double x0, y0, r0, x1, y1, r1;
    if (JS_ToFloat64(ctx, &x0, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y0, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &r0, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &x1, argv[3]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y1, argv[4]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &r1, argv[5]) != 0) return JS_EXCEPTION;
    
    CanvasGradient* gradient = context->CreateRadialGradient(x0, y0, r0, x1, y1, r1);
    return CanvasBindings::WrapGradient(ctx, gradient);
}

// createConicGradient(startAngle, x, y)
static JSValue js_context_2d_create_conic_gradient(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 3) return JS_ThrowTypeError(ctx, "createConicGradient requires 3 arguments");
    
    double startAngle, x, y;
    if (JS_ToFloat64(ctx, &startAngle, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &x, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[2]) != 0) return JS_EXCEPTION;
    
    CanvasGradient* gradient = context->CreateConicGradient(startAngle, x, y);
    return CanvasBindings::WrapGradient(ctx, gradient);
}

// createPattern(image, repetition)
// 注意：暂时image参数可以传null，因为完整的图像加载需要HTMLImageElement支持
static JSValue js_context_2d_create_pattern(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 2) return JS_ThrowTypeError(ctx, "createPattern requires 2 arguments");
    
    // 暂时简化实现：image参数传null会返回null
    // 完整实现需要从HTMLImageElement或HTMLCanvasElement提取SkImage
    if (JS_IsNull(argv[0]) || JS_IsUndefined(argv[0])) {
        return JS_NULL;
    }
    
    const char* repetition = JS_ToCString(ctx, argv[1]);
    if (!repetition) return JS_EXCEPTION;
    
    // TODO: 从argv[0]提取SkImage*，这里暂时返回null
    // 完整实现需要检查argv[0]是否是HTMLImageElement或HTMLCanvasElement
    // 然后获取其内部的SkImage*
    
    JS_FreeCString(ctx, repetition);
    
    // 暂时返回null，表示不支持
    // 等有了HTMLImageElement绑定后可以完善
    return JS_NULL;
}


// ========== Context2D 像素操作方法 ==========

// createImageData(width, height)
static JSValue js_context_2d_create_image_data(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 2) return JS_ThrowTypeError(ctx, "createImageData requires 2 arguments");
    
    int32_t width, height;
    if (JS_ToInt32(ctx, &width, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &height, argv[1]) != 0) return JS_EXCEPTION;
    
    ImageData* imageData = context->CreateImageData(width, height);
    return CanvasBindings::WrapImageData(ctx, imageData);
}

// getImageData(x, y, width, height)
static JSValue js_context_2d_get_image_data(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 4) return JS_ThrowTypeError(ctx, "getImageData requires 4 arguments");
    
    int32_t x, y, width, height;
    if (JS_ToInt32(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &width, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &height, argv[3]) != 0) return JS_EXCEPTION;
    
    ImageData* imageData = context->GetImageData(x, y, width, height);
    return CanvasBindings::WrapImageData(ctx, imageData);
}

// putImageData(imageData, dx, dy)
static JSValue js_context_2d_put_image_data(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 3) return JS_ThrowTypeError(ctx, "putImageData requires 3 arguments");
    
    ImageData* imageData = CanvasBindings::UnwrapImageData(ctx, argv[0]);
    if (!imageData) return JS_ThrowTypeError(ctx, "First argument must be an ImageData object");
    
    int32_t dx, dy;
    if (JS_ToInt32(ctx, &dx, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &dy, argv[2]) != 0) return JS_EXCEPTION;
    
    context->PutImageData(imageData, dx, dy);
    return JS_UNDEFINED;
}

// ========== Context2D 图像绘制方法 ==========

// drawImage(image, dx, dy) 或 drawImage(image, dx, dy, dw, dh)
// 或 drawImage(image, sx, sy, sw, sh, dx, dy, dw, dh)
static JSValue js_context_2d_draw_image(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 3) return JS_ThrowTypeError(ctx, "drawImage requires at least 3 arguments");
    
    // 获取图片源
    sk_sp<SkImage> image = nullptr;
    
    // 首先尝试使用新绑定系统解包
    auto element = bindings::UnwrapElement(ctx, argv[0]);
    if (!element) {
        // 如果新绑定系统失败，尝试旧绑定系统
        element = DOMBindings::UnwrapElement(ctx, argv[0]);
    }
    
    if (element) {
        auto img_element = std::dynamic_pointer_cast<HTMLImageElement>(element);
        if (img_element) {
            image = img_element->GetSkImage();
            if (!image) {
                // 尝试同步加载
                img_element->LoadImageSync();
                image = img_element->GetSkImage();
            }
        } else {
            // 检查是否是 HTMLCanvasElement
            auto canvas_element = std::dynamic_pointer_cast<HTMLCanvasElement>(element);
            if (canvas_element) {
                auto ctx2d = canvas_element->GetContext2D();
                if (ctx2d) {
                    SkSurface* surface = ctx2d->GetSurface();
                    if (surface) {
                        image = surface->makeImageSnapshot();
                    }
                }
            }
        }
    }
    
    if (!image) {
        // 图片未加载或无效
        return JS_UNDEFINED;
    }
    
    // 解析参数
    if (argc == 3) {
        // drawImage(image, dx, dy)
        double dx, dy;
        if (JS_ToFloat64(ctx, &dx, argv[1]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &dy, argv[2]) != 0) return JS_EXCEPTION;
        
        context->DrawImage(image, static_cast<float>(dx), static_cast<float>(dy));
    } else if (argc == 5) {
        // drawImage(image, dx, dy, dw, dh)
        double dx, dy, dw, dh;
        if (JS_ToFloat64(ctx, &dx, argv[1]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &dy, argv[2]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &dw, argv[3]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &dh, argv[4]) != 0) return JS_EXCEPTION;
        
        context->DrawImage(image, 
            static_cast<float>(dx), static_cast<float>(dy),
            static_cast<float>(dw), static_cast<float>(dh));
    } else if (argc >= 9) {
        // drawImage(image, sx, sy, sw, sh, dx, dy, dw, dh)
        double sx, sy, sw, sh, dx, dy, dw, dh;
        if (JS_ToFloat64(ctx, &sx, argv[1]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &sy, argv[2]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &sw, argv[3]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &sh, argv[4]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &dx, argv[5]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &dy, argv[6]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &dw, argv[7]) != 0) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &dh, argv[8]) != 0) return JS_EXCEPTION;
        
        context->DrawImage(image,
            static_cast<float>(sx), static_cast<float>(sy),
            static_cast<float>(sw), static_cast<float>(sh),
            static_cast<float>(dx), static_cast<float>(dy),
            static_cast<float>(dw), static_cast<float>(dh));
    } else {
        return JS_ThrowTypeError(ctx, "drawImage requires 3, 5, or 9 arguments");
    }
    
    return JS_UNDEFINED;
}

// ========== Context2D 变换方法 ==========

// save()
static JSValue js_context_2d_save(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    context->Save();
    return JS_UNDEFINED;
}

// restore()
static JSValue js_context_2d_restore(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    context->Restore();
    return JS_UNDEFINED;
}

// scale(x, y)
static JSValue js_context_2d_scale(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 2) return JS_ThrowTypeError(ctx, "scale requires 2 arguments");
    
    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    
    context->Scale(x, y);
    return JS_UNDEFINED;
}

// rotate(angle)
static JSValue js_context_2d_rotate(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 1) return JS_ThrowTypeError(ctx, "rotate requires 1 argument");
    
    double angle;
    if (JS_ToFloat64(ctx, &angle, argv[0]) != 0) return JS_EXCEPTION;
    
    context->Rotate(angle);
    return JS_UNDEFINED;
}

// translate(x, y)
static JSValue js_context_2d_translate(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 2) return JS_ThrowTypeError(ctx, "translate requires 2 arguments");
    
    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]) != 0) return JS_EXCEPTION;
    
    context->Translate(x, y);
    return JS_UNDEFINED;
}

// transform(a, b, c, d, e, f)
static JSValue js_context_2d_transform(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 6) return JS_ThrowTypeError(ctx, "transform requires 6 arguments");
    
    double a, b, c, d, e, f;
    if (JS_ToFloat64(ctx, &a, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &b, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &c, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &d, argv[3]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &e, argv[4]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &f, argv[5]) != 0) return JS_EXCEPTION;
    
    context->Transform(a, b, c, d, e, f);
    return JS_UNDEFINED;
}

// setTransform(a, b, c, d, e, f)
static JSValue js_context_2d_set_transform(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    if (argc < 6) return JS_ThrowTypeError(ctx, "setTransform requires 6 arguments");
    
    double a, b, c, d, e, f;
    if (JS_ToFloat64(ctx, &a, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &b, argv[1]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &c, argv[2]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &d, argv[3]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &e, argv[4]) != 0) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &f, argv[5]) != 0) return JS_EXCEPTION;
    
    context->SetTransform(a, b, c, d, e, f);
    return JS_UNDEFINED;
}

// resetTransform()
static JSValue js_context_2d_reset_transform(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    context->ResetTransform();
    return JS_UNDEFINED;
}

// getTransform() - 返回 DOMMatrix 对象或简化的对象
static JSValue js_context_2d_get_transform(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto context = CanvasBindings::UnwrapContext2D(ctx, this_val);
    if (!context) return JS_EXCEPTION;
    
    std::vector<double> matrix = context->GetTransform();
    
    // 返回一个包含变换矩阵值的对象
    // 简化实现：返回 {a, b, c, d, e, f} 对象而不是完整的 DOMMatrix
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "a", JS_NewFloat64(ctx, matrix[0]));
    JS_SetPropertyStr(ctx, result, "b", JS_NewFloat64(ctx, matrix[1]));
    JS_SetPropertyStr(ctx, result, "c", JS_NewFloat64(ctx, matrix[2]));
    JS_SetPropertyStr(ctx, result, "d", JS_NewFloat64(ctx, matrix[3]));
    JS_SetPropertyStr(ctx, result, "e", JS_NewFloat64(ctx, matrix[4]));
    JS_SetPropertyStr(ctx, result, "f", JS_NewFloat64(ctx, matrix[5]));
    
    return result;
}

// ========== Context2D 类初始化 ==========

// ========== Context2D 类定义 ==========

static const JSCFunctionListEntry js_context_2d_proto_funcs[] = {
    // 样式属性
    JS_CGETSET_MAGIC_DEF("fillStyle", js_context_2d_get_fill_style, js_context_2d_set_fill_style, 0),
    JS_CGETSET_MAGIC_DEF("strokeStyle", js_context_2d_get_stroke_style, js_context_2d_set_stroke_style, 0),
    JS_CGETSET_MAGIC_DEF("lineWidth", js_context_2d_get_line_width, js_context_2d_set_line_width, 0),
    JS_CGETSET_MAGIC_DEF("lineCap", js_context_2d_get_line_cap, js_context_2d_set_line_cap, 0),
    JS_CGETSET_MAGIC_DEF("lineJoin", js_context_2d_get_line_join, js_context_2d_set_line_join, 0),
    JS_CGETSET_MAGIC_DEF("miterLimit", js_context_2d_get_miter_limit, js_context_2d_set_miter_limit, 0),
    JS_CGETSET_MAGIC_DEF("lineDashOffset", js_context_2d_get_line_dash_offset, js_context_2d_set_line_dash_offset, 0),
    JS_CGETSET_MAGIC_DEF("globalAlpha", js_context_2d_get_global_alpha, js_context_2d_set_global_alpha, 0),
    JS_CGETSET_MAGIC_DEF("font", js_context_2d_get_font, js_context_2d_set_font, 0),
    JS_CGETSET_MAGIC_DEF("textAlign", js_context_2d_get_text_align, js_context_2d_set_text_align, 0),
    JS_CGETSET_MAGIC_DEF("textBaseline", js_context_2d_get_text_baseline, js_context_2d_set_text_baseline, 0),
    JS_CGETSET_MAGIC_DEF("globalCompositeOperation", js_context_2d_get_global_composite_operation, js_context_2d_set_global_composite_operation, 0),
    JS_CGETSET_MAGIC_DEF("shadowColor", js_context_2d_get_shadow_color, js_context_2d_set_shadow_color, 0),
    JS_CGETSET_MAGIC_DEF("shadowBlur", js_context_2d_get_shadow_blur, js_context_2d_set_shadow_blur, 0),
    JS_CGETSET_MAGIC_DEF("shadowOffsetX", js_context_2d_get_shadow_offset_x, js_context_2d_set_shadow_offset_x, 0),
    JS_CGETSET_MAGIC_DEF("shadowOffsetY", js_context_2d_get_shadow_offset_y, js_context_2d_set_shadow_offset_y, 0),
    
    // 虚线方法
    JS_CFUNC_DEF("setLineDash", 1, js_context_2d_set_line_dash),
    JS_CFUNC_DEF("getLineDash", 0, js_context_2d_get_line_dash),
    
    // 矩形方法
    JS_CFUNC_DEF("clearRect", 4, js_context_2d_clear_rect),
    JS_CFUNC_DEF("fillRect", 4, js_context_2d_fill_rect),
    JS_CFUNC_DEF("strokeRect", 4, js_context_2d_stroke_rect),
    
    // 路径方法
    JS_CFUNC_DEF("beginPath", 0, js_context_2d_begin_path),
    JS_CFUNC_DEF("closePath", 0, js_context_2d_close_path),
    JS_CFUNC_DEF("moveTo", 2, js_context_2d_move_to),
    JS_CFUNC_DEF("lineTo", 2, js_context_2d_line_to),
    JS_CFUNC_DEF("rect", 4, js_context_2d_rect),
    JS_CFUNC_DEF("roundRect", 5, js_context_2d_round_rect),
    JS_CFUNC_DEF("arc", 6, js_context_2d_arc),
    JS_CFUNC_DEF("arcTo", 5, js_context_2d_arc_to),
    JS_CFUNC_DEF("quadraticCurveTo", 4, js_context_2d_quadratic_curve_to),
    JS_CFUNC_DEF("bezierCurveTo", 6, js_context_2d_bezier_curve_to),
    JS_CFUNC_DEF("ellipse", 8, js_context_2d_ellipse),
    JS_CFUNC_DEF("fill", 0, js_context_2d_fill),
    JS_CFUNC_DEF("stroke", 0, js_context_2d_stroke),
    JS_CFUNC_DEF("clip", 0, js_context_2d_clip),
    
    // 文本方法
    JS_CFUNC_DEF("fillText", 3, js_context_2d_fill_text),
    JS_CFUNC_DEF("strokeText", 3, js_context_2d_stroke_text),
    JS_CFUNC_DEF("measureText", 1, js_context_2d_measure_text),
    
    // 渐变与图案方法
    JS_CFUNC_DEF("createLinearGradient", 4, js_context_2d_create_linear_gradient),
    JS_CFUNC_DEF("createRadialGradient", 6, js_context_2d_create_radial_gradient),
    JS_CFUNC_DEF("createConicGradient", 3, js_context_2d_create_conic_gradient),
    JS_CFUNC_DEF("createPattern", 2, js_context_2d_create_pattern),
    
    // 像素操作方法
    JS_CFUNC_DEF("createImageData", 2, js_context_2d_create_image_data),
    JS_CFUNC_DEF("getImageData", 4, js_context_2d_get_image_data),
    JS_CFUNC_DEF("putImageData", 3, js_context_2d_put_image_data),
    
    // 图像绘制方法
    JS_CFUNC_DEF("drawImage", 3, js_context_2d_draw_image),  // 最少3个参数
    
    // 点击检测方法
    JS_CFUNC_DEF("isPointInPath", 2, js_context_2d_is_point_in_path),
    JS_CFUNC_DEF("isPointInStroke", 2, js_context_2d_is_point_in_stroke),
    
    // 变换方法
    JS_CFUNC_DEF("save", 0, js_context_2d_save),
    JS_CFUNC_DEF("restore", 0, js_context_2d_restore),
    JS_CFUNC_DEF("scale", 2, js_context_2d_scale),
    JS_CFUNC_DEF("rotate", 1, js_context_2d_rotate),
    JS_CFUNC_DEF("translate", 2, js_context_2d_translate),
    JS_CFUNC_DEF("transform", 6, js_context_2d_transform),
    JS_CFUNC_DEF("setTransform", 6, js_context_2d_set_transform),
    JS_CFUNC_DEF("resetTransform", 0, js_context_2d_reset_transform),
    JS_CFUNC_DEF("getTransform", 0, js_context_2d_get_transform),
};

void CanvasBindings::InitContext2DClass(JSContext* ctx) {
    // 创建 CanvasRenderingContext2D 类
    JS_NewClassID(JS_GetRuntime(ctx), &context_2d_class_id);
    
    JSClassDef context_2d_class = {
        "CanvasRenderingContext2D",
        /* finalizer */ js_context_2d_finalizer,
        /* gc_mark */ nullptr,
        /* call */ nullptr,
        /* exotic */ nullptr,
    };
    
    JS_NewClass(JS_GetRuntime(ctx), context_2d_class_id, &context_2d_class);
    
    // 创建原型对象并使用 JS_SetPropertyFunctionList 注册所有属性和方法
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_context_2d_proto_funcs,
                               sizeof(js_context_2d_proto_funcs) / sizeof(js_context_2d_proto_funcs[0]));
    JS_SetClassProto(ctx, context_2d_class_id, proto);
}

// ========== CanvasGradient 绑定 ==========

JSClassID CanvasBindings::gradient_class_id = 0;

static void js_gradient_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<CanvasGradient*>(JS_GetOpaque(val, CanvasBindings::gradient_class_id));
    if (ptr) {
        delete ptr;
    }
}

// addColorStop(offset, color)
static JSValue js_gradient_add_color_stop(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto gradient = CanvasBindings::UnwrapGradient(ctx, this_val);
    if (!gradient) return JS_EXCEPTION;
    
    if (argc < 2) return JS_ThrowTypeError(ctx, "addColorStop requires 2 arguments");
    
    double offset;
    if (JS_ToFloat64(ctx, &offset, argv[0]) != 0) return JS_EXCEPTION;
    
    const char* color = JS_ToCString(ctx, argv[1]);
    if (!color) return JS_EXCEPTION;
    
    gradient->AddColorStop(offset, color);
    JS_FreeCString(ctx, color);
    return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_gradient_proto_funcs[] = {
    JS_CFUNC_DEF("addColorStop", 2, js_gradient_add_color_stop),
};

void CanvasBindings::InitGradientClass(JSContext* ctx) {
    JS_NewClassID(JS_GetRuntime(ctx), &gradient_class_id);
    
    JSClassDef gradient_class = {
        "CanvasGradient",
        js_gradient_finalizer,
        nullptr,
        nullptr,
        nullptr,
    };
    
    JS_NewClass(JS_GetRuntime(ctx), gradient_class_id, &gradient_class);
    
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_gradient_proto_funcs,
                               sizeof(js_gradient_proto_funcs) / sizeof(js_gradient_proto_funcs[0]));
    JS_SetClassProto(ctx, gradient_class_id, proto);
}

JSValue CanvasBindings::WrapGradient(JSContext* ctx, CanvasGradient* gradient) {
    if (!gradient) return JS_NULL;
    
    JSValue obj = JS_NewObjectClass(ctx, gradient_class_id);
    if (JS_IsException(obj)) return obj;
    
    JS_SetOpaque(obj, gradient);
    return obj;
}

CanvasGradient* CanvasBindings::UnwrapGradient(JSContext* ctx, JSValue obj) {
    return static_cast<CanvasGradient*>(JS_GetOpaque(obj, gradient_class_id));
}

// ========== ImageData 绑定 ==========

JSClassID CanvasBindings::image_data_class_id = 0;

static void js_image_data_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<ImageData*>(JS_GetOpaque(val, CanvasBindings::image_data_class_id));
    if (ptr) {
        delete ptr;
    }
}

// width getter
static JSValue js_image_data_get_width(JSContext* ctx, JSValueConst this_val, int magic) {
    auto imageData = CanvasBindings::UnwrapImageData(ctx, this_val);
    if (!imageData) return JS_EXCEPTION;
    return JS_NewInt32(ctx, imageData->GetWidth());
}

// height getter
static JSValue js_image_data_get_height(JSContext* ctx, JSValueConst this_val, int magic) {
    auto imageData = CanvasBindings::UnwrapImageData(ctx, this_val);
    if (!imageData) return JS_EXCEPTION;
    return JS_NewInt32(ctx, imageData->GetHeight());
}

// data getter (returns Uint8ClampedArray-like array)
static JSValue js_image_data_get_data(JSContext* ctx, JSValueConst this_val, int magic) {
    auto imageData = CanvasBindings::UnwrapImageData(ctx, this_val);
    if (!imageData) return JS_EXCEPTION;
    
    const auto& data = imageData->GetData();
    size_t length = data.size();
    
    // 创建一个JavaScript数组
    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < length; i++) {
        JS_SetPropertyUint32(ctx, arr, i, JS_NewInt32(ctx, data[i]));
    }
    
    return arr;
}

static const JSCFunctionListEntry js_image_data_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("width", js_image_data_get_width, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("height", js_image_data_get_height, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("data", js_image_data_get_data, nullptr, 0),
};

void CanvasBindings::InitImageDataClass(JSContext* ctx) {
    JS_NewClassID(JS_GetRuntime(ctx), &image_data_class_id);
    
    JSClassDef image_data_class = {
        "ImageData",
        js_image_data_finalizer,
        nullptr,
        nullptr,
        nullptr,
    };
    
    JS_NewClass(JS_GetRuntime(ctx), image_data_class_id, &image_data_class);
    
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_image_data_proto_funcs,
                               sizeof(js_image_data_proto_funcs) / sizeof(js_image_data_proto_funcs[0]));
    JS_SetClassProto(ctx, image_data_class_id, proto);
}

JSValue CanvasBindings::WrapImageData(JSContext* ctx, ImageData* imageData) {
    if (!imageData) return JS_NULL;
    
    JSValue obj = JS_NewObjectClass(ctx, image_data_class_id);
    if (JS_IsException(obj)) return obj;
    
    JS_SetOpaque(obj, imageData);
    return obj;
}

ImageData* CanvasBindings::UnwrapImageData(JSContext* ctx, JSValue obj) {
    return static_cast<ImageData*>(JS_GetOpaque(obj, image_data_class_id));
}

// ========== CanvasPattern 绑定 ==========

JSClassID CanvasBindings::pattern_class_id = 0;

static void js_pattern_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<CanvasPattern*>(JS_GetOpaque(val, CanvasBindings::pattern_class_id));
    if (ptr) {
        delete ptr;
    }
}

// CanvasPattern 目前没有需要暴露的方法，未来可能添加 setTransform 等

void CanvasBindings::InitPatternClass(JSContext* ctx) {
    JS_NewClassID(JS_GetRuntime(ctx), &pattern_class_id);
    
    JSClassDef pattern_class = {
        "CanvasPattern",
        js_pattern_finalizer,
        nullptr,
        nullptr,
        nullptr,
    };
    
    JS_NewClass(JS_GetRuntime(ctx), pattern_class_id, &pattern_class);
    
    JSValue proto = JS_NewObject(ctx);
    // CanvasPattern 目前没有公开方法，所以不设置函数列表
    // 如果未来添加方法（如setTransform），可以在这里添加
    JS_SetClassProto(ctx, pattern_class_id, proto);
}

JSValue CanvasBindings::WrapPattern(JSContext* ctx, CanvasPattern* pattern) {
    if (!pattern) return JS_NULL;
    
    JSValue obj = JS_NewObjectClass(ctx, pattern_class_id);
    if (JS_IsException(obj)) return obj;
    
    JS_SetOpaque(obj, pattern);
    return obj;
}

CanvasPattern* CanvasBindings::UnwrapPattern(JSContext* ctx, JSValue obj) {
    return static_cast<CanvasPattern*>(JS_GetOpaque(obj, pattern_class_id));
}


// ========== 公共接口 ==========

void CanvasBindings::Init(JSContext* ctx) {
    if (initialized) return;
    
    InitContext2DClass(ctx);
    InitGradientClass(ctx);
    InitPatternClass(ctx);
    InitImageDataClass(ctx);
    
    initialized = true;
}

JSValue CanvasBindings::WrapContext2D(JSContext* ctx, CanvasRenderingContext2D* context) {
    if (!context) return JS_NULL;
    
    JSValue obj = JS_NewObjectClass(ctx, context_2d_class_id);
    if (JS_IsException(obj)) return obj;
    
    // 设置 opaque 指针（不需要 new，因为由 HTMLCanvasElement 管理）
    JS_SetOpaque(obj, context);
    
    return obj;
}

CanvasRenderingContext2D* CanvasBindings::UnwrapContext2D(JSContext* ctx, JSValue obj) {
    return static_cast<CanvasRenderingContext2D*>(JS_GetOpaque(obj, context_2d_class_id));
}

} // namespace lightui

