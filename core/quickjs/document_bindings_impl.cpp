/**
 * @file document_bindings_impl.cpp
 * @brief Document 对象的 JavaScript 绑定实现（直接使用 QuickJS C API）
 */

#include "window_bindings.h"
#include "bindings/js_node.h"
#include "bindings/js_element.h"
#include "core/dom/document.h"
#include "core/window/window.h"

namespace lightui {

// ========== document.getElementById 实现 ==========

static JSValue JS_Document_getElementById(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getElementById requires 1 argument");
    }

    const char* id = JS_ToCString(ctx, argv[0]);
    if (!id) {
        return JS_EXCEPTION;
    }

    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__lightui_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        JS_FreeCString(ctx, id);
        return JS_NULL;
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        JS_FreeCString(ctx, id);
        return JS_NULL;
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        JS_FreeCString(ctx, id);
        return JS_NULL;
    }

    auto element = doc->GetElementById(id);
    JS_FreeCString(ctx, id);

    if (!element) {
        return JS_NULL;
    }

    return bindings::WrapElement(ctx, element);
}

// ========== document.createElement 实现 ==========

static JSValue JS_Document_createElement(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "createElement requires 1 argument");
    }

    const char* tag_name = JS_ToCString(ctx, argv[0]);
    if (!tag_name) {
        return JS_EXCEPTION;
    }

    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__lightui_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        JS_FreeCString(ctx, tag_name);
        return JS_NULL;
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        JS_FreeCString(ctx, tag_name);
        return JS_NULL;
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        JS_FreeCString(ctx, tag_name);
        return JS_NULL;
    }

    auto element = doc->CreateElement(tag_name);
    JS_FreeCString(ctx, tag_name);

    if (!element) {
        return JS_NULL;
    }

    return bindings::WrapElement(ctx, element);
}

// ========== document.body getter 实现 ==========

static JSValue JS_Document_get_body(JSContext* ctx, JSValueConst this_val, int magic) {
    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__lightui_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        return JS_NULL;
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        return JS_NULL;
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        return JS_NULL;
    }

    auto body = doc->GetBody();
    if (!body) {
        return JS_NULL;
    }

    return bindings::WrapElement(ctx, body);
}

// ========== 绑定函数 ==========

void BindDocumentAPIs(JSContext* ctx, Window* window) {
    // 保存 window 指针到全局对象（用于回调中访问）
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_ptr = JS_NewInt64(ctx, (int64_t)window);
    JS_SetPropertyStr(ctx, global, "__lightui_window_ptr", window_ptr);

    // 注册 document 对象
    JSValue document = JS_NewObject(ctx);

    // 设置 body 属性（直接获取并包装）
    auto doc = window->GetDocument();
    if (doc) {
        auto body = doc->GetBody();
        if (body) {
            JSValue body_val = bindings::WrapElement(ctx, body);
            JS_SetPropertyStr(ctx, document, "body", body_val);
        }
    }

    // 设置 getElementById 方法
    JS_SetPropertyStr(ctx, document, "getElementById",
        JS_NewCFunction(ctx, JS_Document_getElementById, "getElementById", 1));

    // 设置 createElement 方法
    JS_SetPropertyStr(ctx, document, "createElement",
        JS_NewCFunction(ctx, JS_Document_createElement, "createElement", 1));

    // 设置到全局对象
    JS_SetPropertyStr(ctx, global, "document", document);
    JS_FreeValue(ctx, global);
}

} // namespace lightui
