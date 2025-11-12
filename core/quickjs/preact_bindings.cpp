/**
 * @file preact_bindings.cpp
 * @brief Preact JavaScript绑定实现
 */

#include "preact_bindings.h"
#include "core/dom/dom_bindings.h"
#include <iostream>

namespace lightui {

// 全局PreactRenderer实例（存储在JSContext的opaque中）
static const char* PREACT_RENDERER_KEY = "__preact_renderer";

void PreactBindings::Init(JSContext* ctx, std::shared_ptr<PreactRenderer> renderer) {
    SetRenderer(ctx, renderer);
    RegisterPreactModule(ctx, renderer);
}

void PreactBindings::Cleanup(JSContext* ctx) {
    // 清理renderer引用
    JSValue global = JS_GetGlobalObject(ctx);
    JSAtom atom = JS_NewAtom(ctx, PREACT_RENDERER_KEY);

    // 删除renderer属性并释放内存
    JSValue renderer_val = JS_GetProperty(ctx, global, atom);
    if (!JS_IsUndefined(renderer_val)) {
        void* ptr = nullptr;
        if (JS_ToInt64(ctx, (int64_t*)&ptr, renderer_val) == 0) {
            auto* renderer_ptr = static_cast<std::shared_ptr<PreactRenderer>*>(ptr);
            delete renderer_ptr;  // 释放new创建的shared_ptr
        }
    }
    JS_FreeValue(ctx, renderer_val);

    JS_DeleteProperty(ctx, global, atom, 0);
    JS_FreeAtom(ctx, atom);
    JS_FreeValue(ctx, global);
}

void PreactBindings::RegisterPreactModule(JSContext* ctx, std::shared_ptr<PreactRenderer> renderer) {
    JSValue global = JS_GetGlobalObject(ctx);
    
    // 创建__preact_internal对象
    JSValue preact_internal = JS_NewObject(ctx);
    
    // 注册render函数
    JS_SetPropertyStr(ctx, preact_internal, "render",
        JS_NewCFunction(ctx, js_render, "render", 2));
    
    // 注册createDOMElement函数
    JS_SetPropertyStr(ctx, preact_internal, "createDOMElement",
        JS_NewCFunction(ctx, js_createDOMElement, "createDOMElement", 1));
    
    // 注册diff函数
    JS_SetPropertyStr(ctx, preact_internal, "diff",
        JS_NewCFunction(ctx, js_diff, "diff", 3));
    
    // 将__preact_internal挂载到global
    JS_SetPropertyStr(ctx, global, "__preact_internal", preact_internal);

    JS_FreeValue(ctx, global);
}

// ========== JavaScript函数实现 ==========

JSValue PreactBindings::js_render(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "render() requires 2 arguments: vnode and container");
    }

    auto renderer = GetRenderer(ctx);
    if (!renderer) {
        return JS_ThrowInternalError(ctx, "PreactRenderer not initialized");
    }

    JSValue vnode = argv[0];
    JSValue container = argv[1];

    try {
        bool success = renderer->Render(vnode, container);
        return JS_NewBool(ctx, success);
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "Render failed: %s", e.what());
    }
}

JSValue PreactBindings::js_createDOMElement(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "createDOMElement() requires 1 argument: vnode");
    }
    
    auto renderer = GetRenderer(ctx);
    if (!renderer) {
        return JS_ThrowInternalError(ctx, "PreactRenderer not initialized");
    }
    
    JSValue vnode = argv[0];
    
    try {
        auto dom_node = renderer->CreateDOMFromVNode(vnode);
        if (!dom_node) {
            return JS_NULL;
        }
        
        // 包装为JS对象
        return DOMBindings::WrapNode(ctx, dom_node);
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "createDOMElement failed: %s", e.what());
    }
}

JSValue PreactBindings::js_diff(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 3) {
        return JS_ThrowTypeError(ctx, "diff() requires 3 arguments: oldVNode, newVNode, container");
    }
    
    // TODO: 实现diff算法
    // 目前简单地重新渲染
    return js_render(ctx, this_val, 2, &argv[1]);
}

// ========== 辅助函数 ==========

std::shared_ptr<PreactRenderer> PreactBindings::GetRenderer(JSContext* ctx) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue renderer_val = JS_GetPropertyStr(ctx, global, PREACT_RENDERER_KEY);
    
    std::shared_ptr<PreactRenderer>* renderer_ptr = nullptr;
    
    if (!JS_IsUndefined(renderer_val)) {
        void* ptr = nullptr;
        if (JS_ToInt64(ctx, (int64_t*)&ptr, renderer_val) == 0) {
            renderer_ptr = static_cast<std::shared_ptr<PreactRenderer>*>(ptr);
        }
    }
    
    JS_FreeValue(ctx, renderer_val);
    JS_FreeValue(ctx, global);
    
    return renderer_ptr ? *renderer_ptr : nullptr;
}

void PreactBindings::SetRenderer(JSContext* ctx, std::shared_ptr<PreactRenderer> renderer) {
    JSValue global = JS_GetGlobalObject(ctx);
    
    // 创建一个持久化的shared_ptr副本
    auto* renderer_ptr = new std::shared_ptr<PreactRenderer>(renderer);
    
    // 存储指针
    JSValue renderer_val = JS_NewInt64(ctx, (int64_t)renderer_ptr);
    JS_SetPropertyStr(ctx, global, PREACT_RENDERER_KEY, renderer_val);
    
    JS_FreeValue(ctx, global);
}

} // namespace lightui

