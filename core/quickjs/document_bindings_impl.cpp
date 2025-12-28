/**
 * @file document_bindings_impl.cpp
 * @brief Document 对象的 JavaScript 绑定实现（直接使用 QuickJS C API）
 */

#include "window_bindings.h"
#include "bindings/js_node.h"
#include "bindings/js_element.h"
#include "bindings/js_selection.h"
#include "bindings/js_range.h"
#include "core/dom/document.h"
#include "core/dom/bindings/dom_bindings.h"
#include "core/window/window.h"
#include "core/event/loop/event_loop.h"
#include "core/editing/selection_manager.h"
#include "core/editing/contenteditable_handler.h"

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

// ========== document.createTextNode 实现 ==========

static JSValue JS_Document_createTextNode(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "createTextNode requires 1 argument");
    }

    const char* data = JS_ToCString(ctx, argv[0]);
    if (!data) {
        return JS_EXCEPTION;
    }

    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__lightui_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        JS_FreeCString(ctx, data);
        return JS_NULL;
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        JS_FreeCString(ctx, data);
        return JS_NULL;
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        JS_FreeCString(ctx, data);
        return JS_NULL;
    }

    auto text_node = doc->CreateTextNode(data);
    JS_FreeCString(ctx, data);

    if (!text_node) {
        return JS_NULL;
    }

    return bindings::WrapNode(ctx, text_node);
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

// ========== document.head getter 实现 ==========

static JSValue JS_Document_get_head(JSContext* ctx, JSValueConst this_val, int magic) {
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

    auto head = doc->GetHead();
    if (!head) {
        return JS_NULL;
    }

    return bindings::WrapElement(ctx, head);
}

// ========== window.getSelection / document.getSelection 实现 ==========

static JSValue JS_Window_getSelection(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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

    // 获取 EventLoop 中的 SelectionManager
    auto event_loop = DOMBindings::GetGlobalEventLoop();
    if (!event_loop) {
        return JS_NULL;
    }

    auto selection_manager = event_loop->GetSelectionManager();
    if (!selection_manager) {
        return JS_NULL;
    }

    auto selection = selection_manager->GetSelection(doc);
    if (!selection) {
        return JS_NULL;
    }

    return bindings::WrapSelection(ctx, selection);
}

// ========== document.createRange 实现 ==========

static JSValue JS_Document_createRange(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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

    auto range = doc->CreateRange();
    if (!range) {
        return JS_NULL;
    }

    return bindings::WrapRange(ctx, range);
}

// ========== document.execCommand 实现 ==========

static JSValue JS_Document_execCommand(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "execCommand requires at least 1 argument");
    }

    const char* command = JS_ToCString(ctx, argv[0]);
    if (!command) {
        return JS_EXCEPTION;
    }

    std::string cmd(command);
    JS_FreeCString(ctx, command);

    // 获取可选的 value 参数
    std::string value;
    if (argc >= 3) {
        const char* val = JS_ToCString(ctx, argv[2]);
        if (val) {
            value = val;
            JS_FreeCString(ctx, val);
        }
    }

    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__lightui_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        return JS_NewBool(ctx, false);
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        return JS_NewBool(ctx, false);
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        return JS_NewBool(ctx, false);
    }

    // 获取 EventLoop 中的 ContentEditableHandler
    auto event_loop = DOMBindings::GetGlobalEventLoop();
    if (!event_loop) {
        return JS_NewBool(ctx, false);
    }

    auto handler = event_loop->GetContentEditableHandler();
    if (!handler) {
        return JS_NewBool(ctx, false);
    }

    bool result = handler->ExecCommand(doc, cmd, value);
    return JS_NewBool(ctx, result);
}

// ========== document.queryCommandState 实现 ==========

static JSValue JS_Document_queryCommandState(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "queryCommandState requires 1 argument");
    }

    const char* command = JS_ToCString(ctx, argv[0]);
    if (!command) {
        return JS_EXCEPTION;
    }

    std::string cmd(command);
    JS_FreeCString(ctx, command);

    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__lightui_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        return JS_NewBool(ctx, false);
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        return JS_NewBool(ctx, false);
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        return JS_NewBool(ctx, false);
    }

    // 获取 EventLoop 中的 ContentEditableHandler
    auto event_loop = DOMBindings::GetGlobalEventLoop();
    if (!event_loop) {
        return JS_NewBool(ctx, false);
    }

    auto handler = event_loop->GetContentEditableHandler();
    if (!handler) {
        return JS_NewBool(ctx, false);
    }

    bool result = handler->QueryCommandState(doc, cmd);
    return JS_NewBool(ctx, result);
}

// ========== document.queryCommandEnabled 实现 ==========

static JSValue JS_Document_queryCommandEnabled(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "queryCommandEnabled requires 1 argument");
    }

    const char* command = JS_ToCString(ctx, argv[0]);
    if (!command) {
        return JS_EXCEPTION;
    }

    std::string cmd(command);
    JS_FreeCString(ctx, command);

    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__lightui_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        return JS_NewBool(ctx, false);
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        return JS_NewBool(ctx, false);
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        return JS_NewBool(ctx, false);
    }

    // 获取 EventLoop 中的 ContentEditableHandler
    auto event_loop = DOMBindings::GetGlobalEventLoop();
    if (!event_loop) {
        return JS_NewBool(ctx, false);
    }

    auto handler = event_loop->GetContentEditableHandler();
    if (!handler) {
        return JS_NewBool(ctx, false);
    }

    bool result = handler->QueryCommandEnabled(doc, cmd);
    return JS_NewBool(ctx, result);
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
        
        // 设置 head 属性
        auto head = doc->GetHead();
        if (head) {
            JSValue head_val = bindings::WrapElement(ctx, head);
            JS_SetPropertyStr(ctx, document, "head", head_val);
        }
    }

    // 设置 getElementById 方法
    JS_SetPropertyStr(ctx, document, "getElementById",
        JS_NewCFunction(ctx, JS_Document_getElementById, "getElementById", 1));

    // 设置 createElement 方法
    JS_SetPropertyStr(ctx, document, "createElement",
        JS_NewCFunction(ctx, JS_Document_createElement, "createElement", 1));

    // 设置 createTextNode 方法
    JS_SetPropertyStr(ctx, document, "createTextNode",
        JS_NewCFunction(ctx, JS_Document_createTextNode, "createTextNode", 1));

    // 设置 createRange 方法
    JS_SetPropertyStr(ctx, document, "createRange",
        JS_NewCFunction(ctx, JS_Document_createRange, "createRange", 0));

    // 设置 execCommand 方法
    JS_SetPropertyStr(ctx, document, "execCommand",
        JS_NewCFunction(ctx, JS_Document_execCommand, "execCommand", 3));

    // 设置 queryCommandState 方法
    JS_SetPropertyStr(ctx, document, "queryCommandState",
        JS_NewCFunction(ctx, JS_Document_queryCommandState, "queryCommandState", 1));

    // 设置 queryCommandEnabled 方法
    JS_SetPropertyStr(ctx, document, "queryCommandEnabled",
        JS_NewCFunction(ctx, JS_Document_queryCommandEnabled, "queryCommandEnabled", 1));

    // 设置 getSelection 方法（document.getSelection 是 window.getSelection 的别名）
    JS_SetPropertyStr(ctx, document, "getSelection",
        JS_NewCFunction(ctx, JS_Window_getSelection, "getSelection", 0));

    // 设置到全局对象
    JS_SetPropertyStr(ctx, global, "document", document);

    // 设置 window.getSelection
    JSValue window_obj = JS_GetPropertyStr(ctx, global, "window");
    if (!JS_IsUndefined(window_obj) && !JS_IsNull(window_obj)) {
        JS_SetPropertyStr(ctx, window_obj, "getSelection",
            JS_NewCFunction(ctx, JS_Window_getSelection, "getSelection", 0));
        JS_FreeValue(ctx, window_obj);
    }

    // 也设置到 globalThis 上（有些代码直接调用 getSelection()）
    JS_SetPropertyStr(ctx, global, "getSelection",
        JS_NewCFunction(ctx, JS_Window_getSelection, "getSelection", 0));

    JS_FreeValue(ctx, global);
}

} // namespace lightui
