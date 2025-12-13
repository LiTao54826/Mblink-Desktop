/**
 * @file canvas_bindings.h
 * @brief Canvas JavaScript 绑定
 *
 * 功能：
 * - 将 HTMLCanvasElement 绑定到 QuickJS
 * - 将 CanvasRenderingContext2D 绑定到 QuickJS
 * - 将 CanvasGradient 绑定到 QuickJS
 * - 提供 JavaScript 可访问的 Canvas 2D API
 */

#pragma once

#include "quickjs/quickjs.h"
#include "html_canvas_element.h"
#include "core/render/canvas/canvas_rendering_context_2d.h"
#include "core/render/canvas/canvas_gradient.h"
#include "core/render/canvas/canvas_pattern.h"
#include "core/render/canvas/canvas_image_data.h"
#include <memory>

namespace lightui {

/**
 * @brief Canvas 绑定类
 */
class CanvasBindings {
public:
    /**
     * @brief 初始化 Canvas 绑定
     * @param ctx QuickJS 上下文
     */
    static void Init(JSContext* ctx);

    /**
     * @brief 将 CanvasRenderingContext2D 包装为 JS 对象
     */
    static JSValue WrapContext2D(JSContext* ctx, CanvasRenderingContext2D* context);

    /**
     * @brief 从 JS 对象解包 CanvasRenderingContext2D
     */
    static CanvasRenderingContext2D* UnwrapContext2D(JSContext* ctx, JSValue obj);
    
    /**
     * @brief 将 CanvasGradient 包装为 JS 对象
     */
    static JSValue WrapGradient(JSContext* ctx, CanvasGradient* gradient);
    
    /**
     * @brief 从 JS 对象解包 CanvasGradient
     */
    static CanvasGradient* UnwrapGradient(JSContext* ctx, JSValue obj);
    
    /**
     * @brief 将 ImageData 包装为 JS 对象
     */
    static JSValue WrapImageData(JSContext* ctx, ImageData* imageData);
    
    /**
     * @brief 从 JS 对象解包 ImageData
     */
    static ImageData* UnwrapImageData(JSContext* ctx, JSValue obj);
    
    /**
     * @brief 将 CanvasPattern 包装为 JS 对象
     */
    static JSValue WrapPattern(JSContext* ctx, CanvasPattern* pattern);
    
    /**
     * @brief 从 JS 对象解包 CanvasPattern
     */
    static CanvasPattern* UnwrapPattern(JSContext* ctx, JSValue obj);

    // Class IDs
    static JSClassID context_2d_class_id;
    static JSClassID gradient_class_id;
    static JSClassID pattern_class_id;
    static JSClassID image_data_class_id;

private:
    static bool initialized;
    static void InitContext2DClass(JSContext* ctx);
    static void InitGradientClass(JSContext* ctx);
    static void InitPatternClass(JSContext* ctx);
    static void InitImageDataClass(JSContext* ctx);
};

} // namespace lightui

