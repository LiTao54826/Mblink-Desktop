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
#include "core/dom/text.h"
#include "core/dom/bindings/dom_bindings.h"
#include "core/event/input/hit_test_controller.h"
#include "core/event/loop/event_loop.h"
#include "core/editing/selection_manager.h"
#include "core/editing/contenteditable_handler.h"
#include "core/render/objects/render_object.h"
#include "core/render/css/style_resolver.h"
#include "core/render/text/font_manager.h"
#include "core/render/text/text_renderer.h"
#include "core/utils/utf8_utils.h"
#include "core/window/window.h"
#include <iostream>

namespace mbink {

// ========== 辅助函数：将 ComputedStyle 转换为 CSS 属性值字符串 ==========

static std::string ColorToString(SkColor color) {
    int r = SkColorGetR(color);
    int g = SkColorGetG(color);
    int b = SkColorGetB(color);
    int a = SkColorGetA(color);
    if (a == 255) {
        char buf[32];
        snprintf(buf, sizeof(buf), "rgb(%d, %d, %d)", r, g, b);
        return buf;
    } else {
        char buf[48];
        snprintf(buf, sizeof(buf), "rgba(%d, %d, %d, %.2f)", r, g, b, a / 255.0f);
        return buf;
    }
}

static std::string CSSLengthToString(const CSSLength& len) {
    switch (len.unit) {
        case CSSUnit::PX: return std::to_string(static_cast<int>(len.value)) + "px";
        case CSSUnit::PERCENT: return std::to_string(static_cast<int>(len.value)) + "%";
        case CSSUnit::EM: return std::to_string(len.value) + "em";
        case CSSUnit::REM: return std::to_string(len.value) + "rem";
        case CSSUnit::VW: return std::to_string(len.value) + "vw";
        case CSSUnit::VH: return std::to_string(len.value) + "vh";
        case CSSUnit::AUTO: return "auto";
        case CSSUnit::NONE: return "none";
        default: return std::to_string(static_cast<int>(len.value)) + "px";
    }
}

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
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

// ========== document.createElementNS 实现 ==========

static JSValue JS_Document_createElementNS(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "createElementNS requires 2 arguments");
    }

    // 第一个参数是命名空间 URI（我们忽略它）
    // 第二个参数是标签名
    const char* qualified_name = JS_ToCString(ctx, argv[1]);
    if (!qualified_name) {
        return JS_EXCEPTION;
    }

    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        JS_FreeCString(ctx, qualified_name);
        return JS_NULL;
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        JS_FreeCString(ctx, qualified_name);
        return JS_NULL;
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        JS_FreeCString(ctx, qualified_name);
        return JS_NULL;
    }

    // 使用 CreateElement 创建元素（它会根据标签名自动处理 SVG 元素）
    auto element = doc->CreateElement(qualified_name);
    JS_FreeCString(ctx, qualified_name);

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
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

// ========== document.createDocumentFragment 实现 ==========

static JSValue JS_Document_createDocumentFragment(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

    // 创建 DocumentFragment
    auto fragment = doc->CreateDocumentFragment();
    if (!fragment) {
        return JS_NULL;
    }

    return bindings::WrapNode(ctx, fragment);
}

// ========== document.body getter 实现 ==========

static JSValue JS_Document_get_body(JSContext* ctx, JSValueConst this_val, int magic) {
    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

// ========== document.documentElement getter 实现 ==========

static JSValue JS_Document_get_documentElement(JSContext* ctx, JSValueConst this_val, int magic) {
    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

    auto document_element = doc->GetDocumentElement();
    if (!document_element) {
        return JS_NULL;
    }

    return bindings::WrapElement(ctx, document_element);
}

// ========== document.activeElement getter 实现 ==========

static JSValue JS_Document_get_activeElement(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

    auto active_element = doc->GetActiveElement();
    if (!active_element) {
        // 如果没有活动元素，返回 body（符合 W3C 规范）
        auto body = doc->GetBody();
        if (body) {
            return bindings::WrapElement(ctx, body);
        }
        return JS_NULL;
    }

    return bindings::WrapElement(ctx, active_element);
}

// ========== window.getSelection / document.getSelection 实现 ==========

static JSValue JS_Window_getSelection(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

// ========== document.querySelector 实现 ==========

static JSValue JS_Document_querySelector(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "querySelector requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_EXCEPTION;
    }

    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        JS_FreeCString(ctx, selector);
        return JS_NULL;
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        JS_FreeCString(ctx, selector);
        return JS_NULL;
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        JS_FreeCString(ctx, selector);
        return JS_NULL;
    }

    // 从 body 开始查询
    auto body = doc->GetBody();
    if (!body) {
        JS_FreeCString(ctx, selector);
        return JS_NULL;
    }

    auto result = body->QuerySelector(selector);
    JS_FreeCString(ctx, selector);

    if (!result) {
        return JS_NULL;
    }

    return bindings::WrapElement(ctx, result);
}

// ========== document.querySelectorAll 实现 ==========

static JSValue JS_Document_querySelectorAll(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "querySelectorAll requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_EXCEPTION;
    }

    // 从全局对象获取 window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
    JS_FreeValue(ctx, global);

    if (JS_IsUndefined(window_val)) {
        JS_FreeCString(ctx, selector);
        return JS_NewArray(ctx);
    }

    void* ptr = nullptr;
    JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
    JS_FreeValue(ctx, window_val);

    if (!ptr) {
        JS_FreeCString(ctx, selector);
        return JS_NewArray(ctx);
    }

    auto* window = static_cast<Window*>(ptr);
    auto doc = window->GetDocument();
    if (!doc) {
        JS_FreeCString(ctx, selector);
        return JS_NewArray(ctx);
    }

    // 从 body 开始查询
    auto body = doc->GetBody();
    if (!body) {
        JS_FreeCString(ctx, selector);
        return JS_NewArray(ctx);
    }

    auto results = body->QuerySelectorAll(selector);
    JS_FreeCString(ctx, selector);

    // 创建 JS 数组
    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < results.size(); ++i) {
        JS_SetPropertyUint32(ctx, arr, i, bindings::WrapElement(ctx, results[i]));
    }

    return arr;
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
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

// ========== window.scrollBy 实现 ==========

static JSValue JS_Window_scrollBy(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    // scrollBy(x, y) 或 scrollBy({left, top})
    double x = 0, y = 0;
    
    if (argc >= 1) {
        if (JS_IsObject(argv[0])) {
            // scrollBy({left, top}) 形式
            JSValue left_val = JS_GetPropertyStr(ctx, argv[0], "left");
            JSValue top_val = JS_GetPropertyStr(ctx, argv[0], "top");
            if (!JS_IsUndefined(left_val)) {
                JS_ToFloat64(ctx, &x, left_val);
            }
            if (!JS_IsUndefined(top_val)) {
                JS_ToFloat64(ctx, &y, top_val);
            }
            JS_FreeValue(ctx, left_val);
            JS_FreeValue(ctx, top_val);
        } else {
            // scrollBy(x, y) 形式
            JS_ToFloat64(ctx, &x, argv[0]);
            if (argc >= 2) {
                JS_ToFloat64(ctx, &y, argv[1]);
            }
        }
    }
    
    // 目前 window 级别的滚动暂不实现，返回 undefined
    // 大多数情况下，滚动是在具体的可滚动元素上进行的
    return JS_UNDEFINED;
}

// ========== window.scrollTo 实现 ==========

static JSValue JS_Window_scrollTo(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    // scrollTo(x, y) 或 scrollTo({left, top})
    // 目前 window 级别的滚动暂不实现，返回 undefined
    return JS_UNDEFINED;
}

// ========== window.getComputedStyle 实现 ==========

static JSValue JS_Window_getComputedStyle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getComputedStyle requires 1 argument");
    }

    // 获取 Element
    auto element = bindings::UnwrapElement(ctx, argv[0]);
    if (!element) {
        return JS_ThrowTypeError(ctx, "getComputedStyle: argument is not an Element");
    }

    // 获取 Window 指针
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
    JS_FreeValue(ctx, global);

    Window* window = nullptr;
    if (!JS_IsUndefined(window_val)) {
        void* ptr = nullptr;
        JS_ToInt64Ext(ctx, (int64_t*)&ptr, window_val);
        window = static_cast<Window*>(ptr);
    }
    JS_FreeValue(ctx, window_val);

    // 获取计算样式
    ComputedStyle style;
    bool has_style = false;

    // 首先尝试从 RenderObject 获取
    auto render_obj = element->GetRenderObject();
    if (render_obj) {
        style = render_obj->GetComputedStyle();
        has_style = true;
    } else {
        // 如果没有 RenderObject，使用 StyleResolver 直接计算样式
        StyleResolver resolver;
        if (window && window->GetDocument() && window->GetDocument()->GetStyleManager()) {
            resolver.SetStyleManager(window->GetDocument()->GetStyleManager());
        }
        
        // 获取父元素样式用于继承
        const ComputedStyle* parent_style = nullptr;
        ComputedStyle parent_computed;
        if (auto parent_node = element->GetParentNode()) {
            if (parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto parent_elem = std::static_pointer_cast<Element>(parent_node);
                if (auto parent_render = parent_elem->GetRenderObject()) {
                    parent_style = &parent_render->GetComputedStyle();
                } else {
                    // 父元素也没有 RenderObject，递归计算
                    parent_computed = resolver.ResolveStyle(parent_elem, nullptr);
                    parent_style = &parent_computed;
                }
            }
        }
        
        style = resolver.ResolveStyle(element, parent_style);
        has_style = true;
    }

    // 创建返回的样式对象（模拟 CSSStyleDeclaration）
    JSValue style_obj = JS_NewObject(ctx);
    
    if (!has_style) {
        // 返回空字符串的样式对象
        JS_SetPropertyStr(ctx, style_obj, "display", JS_NewString(ctx, ""));
        JS_SetPropertyStr(ctx, style_obj, "getPropertyValue",
            JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                return JS_NewString(ctx, "");
            }, "getPropertyValue", 1));
        return style_obj;
    }

    // 设置常用的 CSS 属性
    // 显示和定位
    const char* display_str = "block";
    switch (style.display) {
        case RenderObjectType::BLOCK: display_str = "block"; break;
        case RenderObjectType::INLINE: display_str = "inline"; break;
        case RenderObjectType::INLINE_BLOCK: display_str = "inline-block"; break;
        case RenderObjectType::FLEX: display_str = "flex"; break;
        case RenderObjectType::GRID: display_str = "grid"; break;
        case RenderObjectType::NONE: display_str = "none"; break;
        case RenderObjectType::CONTENTS: display_str = "contents"; break;
        default: display_str = "block"; break;
    }
    JS_SetPropertyStr(ctx, style_obj, "display", JS_NewString(ctx, display_str));
    JS_SetPropertyStr(ctx, style_obj, "position", JS_NewString(ctx, style.position.empty() ? "static" : style.position.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "visibility", JS_NewString(ctx, style.visibility.c_str()));

    // 尺寸
    JS_SetPropertyStr(ctx, style_obj, "width", JS_NewString(ctx, CSSLengthToString(style.width).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "height", JS_NewString(ctx, CSSLengthToString(style.height).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "minWidth", JS_NewString(ctx, CSSLengthToString(style.min_width).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "maxWidth", JS_NewString(ctx, CSSLengthToString(style.max_width).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "minHeight", JS_NewString(ctx, CSSLengthToString(style.min_height).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "maxHeight", JS_NewString(ctx, CSSLengthToString(style.max_height).c_str()));

    // 盒模型
    JS_SetPropertyStr(ctx, style_obj, "boxSizing", JS_NewString(ctx, style.box_sizing.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "marginTop", JS_NewString(ctx, CSSLengthToString(style.margin_top).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "marginRight", JS_NewString(ctx, CSSLengthToString(style.margin_right).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "marginBottom", JS_NewString(ctx, CSSLengthToString(style.margin_bottom).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "marginLeft", JS_NewString(ctx, CSSLengthToString(style.margin_left).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "paddingTop", JS_NewString(ctx, CSSLengthToString(style.padding_top).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "paddingRight", JS_NewString(ctx, CSSLengthToString(style.padding_right).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "paddingBottom", JS_NewString(ctx, CSSLengthToString(style.padding_bottom).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "paddingLeft", JS_NewString(ctx, CSSLengthToString(style.padding_left).c_str()));

    // 边框
    char border_width_buf[32];
    snprintf(border_width_buf, sizeof(border_width_buf), "%.0fpx", style.border_top_width);
    JS_SetPropertyStr(ctx, style_obj, "borderTopWidth", JS_NewString(ctx, border_width_buf));
    snprintf(border_width_buf, sizeof(border_width_buf), "%.0fpx", style.border_right_width);
    JS_SetPropertyStr(ctx, style_obj, "borderRightWidth", JS_NewString(ctx, border_width_buf));
    snprintf(border_width_buf, sizeof(border_width_buf), "%.0fpx", style.border_bottom_width);
    JS_SetPropertyStr(ctx, style_obj, "borderBottomWidth", JS_NewString(ctx, border_width_buf));
    snprintf(border_width_buf, sizeof(border_width_buf), "%.0fpx", style.border_left_width);
    JS_SetPropertyStr(ctx, style_obj, "borderLeftWidth", JS_NewString(ctx, border_width_buf));

    // 背景
    JS_SetPropertyStr(ctx, style_obj, "backgroundColor", JS_NewString(ctx, style.background_color.empty() ? "transparent" : style.background_color.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "background", JS_NewString(ctx, style.background_color.empty() ? "transparent" : style.background_color.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "backgroundImage", JS_NewString(ctx, style.background_image.empty() ? "none" : style.background_image.c_str()));

    // 文本
    JS_SetPropertyStr(ctx, style_obj, "color", JS_NewString(ctx, style.color.empty() ? "rgb(0, 0, 0)" : style.color.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "fontFamily", JS_NewString(ctx, style.font_family.empty() ? "sans-serif" : style.font_family.c_str()));
    char font_size_buf[32];
    snprintf(font_size_buf, sizeof(font_size_buf), "%.0fpx", style.font_size);
    JS_SetPropertyStr(ctx, style_obj, "fontSize", JS_NewString(ctx, font_size_buf));
    JS_SetPropertyStr(ctx, style_obj, "fontWeight", JS_NewString(ctx, style.font_weight.empty() ? "normal" : style.font_weight.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "fontStyle", JS_NewString(ctx, style.font_style.empty() ? "normal" : style.font_style.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "textAlign", JS_NewString(ctx, style.text_align.empty() ? "left" : style.text_align.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "textDecoration", JS_NewString(ctx, style.text_decoration.empty() ? "none" : style.text_decoration.c_str()));
    char line_height_buf[32];
    snprintf(line_height_buf, sizeof(line_height_buf), "%.2f", style.line_height);
    JS_SetPropertyStr(ctx, style_obj, "lineHeight", JS_NewString(ctx, line_height_buf));

    // 透明度
    char opacity_buf[32];
    snprintf(opacity_buf, sizeof(opacity_buf), "%.2f", style.opacity);
    JS_SetPropertyStr(ctx, style_obj, "opacity", JS_NewString(ctx, opacity_buf));

    // 溢出
    JS_SetPropertyStr(ctx, style_obj, "overflow", JS_NewString(ctx, style.overflow.empty() ? "visible" : style.overflow.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "overflowX", JS_NewString(ctx, style.overflow_x.empty() ? "visible" : style.overflow_x.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "overflowY", JS_NewString(ctx, style.overflow_y.empty() ? "visible" : style.overflow_y.c_str()));

    // 定位
    JS_SetPropertyStr(ctx, style_obj, "top", JS_NewString(ctx, CSSLengthToString(style.top).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "right", JS_NewString(ctx, CSSLengthToString(style.right).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "bottom", JS_NewString(ctx, CSSLengthToString(style.bottom).c_str()));
    JS_SetPropertyStr(ctx, style_obj, "left", JS_NewString(ctx, CSSLengthToString(style.left).c_str()));
    char z_index_buf[32];
    snprintf(z_index_buf, sizeof(z_index_buf), "%d", style.z_index);
    JS_SetPropertyStr(ctx, style_obj, "zIndex", JS_NewString(ctx, z_index_buf));

    // Flexbox
    JS_SetPropertyStr(ctx, style_obj, "flexDirection", JS_NewString(ctx, style.flex_direction.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "flexWrap", JS_NewString(ctx, style.flex_wrap.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "justifyContent", JS_NewString(ctx, style.justify_content.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "alignItems", JS_NewString(ctx, style.align_items.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "alignContent", JS_NewString(ctx, style.align_content.c_str()));
    char flex_grow_buf[32];
    snprintf(flex_grow_buf, sizeof(flex_grow_buf), "%.0f", style.flex_grow);
    JS_SetPropertyStr(ctx, style_obj, "flexGrow", JS_NewString(ctx, flex_grow_buf));
    char flex_shrink_buf[32];
    snprintf(flex_shrink_buf, sizeof(flex_shrink_buf), "%.0f", style.flex_shrink);
    JS_SetPropertyStr(ctx, style_obj, "flexShrink", JS_NewString(ctx, flex_shrink_buf));
    JS_SetPropertyStr(ctx, style_obj, "flexBasis", JS_NewString(ctx, CSSLengthToString(style.flex_basis).c_str()));

    // 文本属性
    JS_SetPropertyStr(ctx, style_obj, "whiteSpace", JS_NewString(ctx, style.white_space.empty() ? "normal" : style.white_space.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "wordWrap", JS_NewString(ctx, style.word_wrap.empty() ? "normal" : style.word_wrap.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "textOverflow", JS_NewString(ctx, style.text_overflow.empty() ? "clip" : style.text_overflow.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "verticalAlign", JS_NewString(ctx, style.vertical_align.empty() ? "baseline" : style.vertical_align.c_str()));
    JS_SetPropertyStr(ctx, style_obj, "cursor", JS_NewString(ctx, style.cursor.empty() ? "auto" : style.cursor.c_str()));
    
    // 文本缩进（CodeMirror 需要）
    JS_SetPropertyStr(ctx, style_obj, "textIndent", JS_NewString(ctx, "0px"));
    
    // 字母和单词间距
    JS_SetPropertyStr(ctx, style_obj, "letterSpacing", JS_NewString(ctx, "normal"));
    JS_SetPropertyStr(ctx, style_obj, "wordSpacing", JS_NewString(ctx, "normal"));
    
    // 文本转换
    JS_SetPropertyStr(ctx, style_obj, "textTransform", JS_NewString(ctx, style.text_transform.empty() ? "none" : style.text_transform.c_str()));
    
    // 方向
    JS_SetPropertyStr(ctx, style_obj, "direction", JS_NewString(ctx, "ltr"));
    JS_SetPropertyStr(ctx, style_obj, "unicodeBidi", JS_NewString(ctx, "normal"));
    
    // 列表样式
    JS_SetPropertyStr(ctx, style_obj, "listStyle", JS_NewString(ctx, "none"));
    JS_SetPropertyStr(ctx, style_obj, "listStyleType", JS_NewString(ctx, "none"));
    JS_SetPropertyStr(ctx, style_obj, "listStylePosition", JS_NewString(ctx, "outside"));
    
    // 表格相关
    JS_SetPropertyStr(ctx, style_obj, "borderCollapse", JS_NewString(ctx, "separate"));
    JS_SetPropertyStr(ctx, style_obj, "borderSpacing", JS_NewString(ctx, "0px"));
    JS_SetPropertyStr(ctx, style_obj, "tableLayout", JS_NewString(ctx, "auto"));
    
    // 轮廓
    JS_SetPropertyStr(ctx, style_obj, "outline", JS_NewString(ctx, "none"));
    JS_SetPropertyStr(ctx, style_obj, "outlineWidth", JS_NewString(ctx, "0px"));
    JS_SetPropertyStr(ctx, style_obj, "outlineStyle", JS_NewString(ctx, "none"));
    JS_SetPropertyStr(ctx, style_obj, "outlineColor", JS_NewString(ctx, "currentcolor"));
    
    // 浮动和清除
    JS_SetPropertyStr(ctx, style_obj, "float", JS_NewString(ctx, "none"));
    JS_SetPropertyStr(ctx, style_obj, "clear", JS_NewString(ctx, "none"));
    
    // 内容
    JS_SetPropertyStr(ctx, style_obj, "content", JS_NewString(ctx, "normal"));
    
    // 指针事件
    JS_SetPropertyStr(ctx, style_obj, "pointerEvents", JS_NewString(ctx, style.pointer_events.empty() ? "auto" : style.pointer_events.c_str()));
    
    // 用户选择
    JS_SetPropertyStr(ctx, style_obj, "userSelect", JS_NewString(ctx, "auto"));
    
    // 触摸操作
    JS_SetPropertyStr(ctx, style_obj, "touchAction", JS_NewString(ctx, "auto"));
    
    // 滚动行为
    JS_SetPropertyStr(ctx, style_obj, "scrollBehavior", JS_NewString(ctx, "auto"));

    // 添加 getPropertyValue 方法（用于兼容性）
    JS_SetPropertyStr(ctx, style_obj, "getPropertyValue",
        JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
            if (argc < 1) {
                return JS_NewString(ctx, "");
            }
            const char* prop = JS_ToCString(ctx, argv[0]);
            if (!prop) {
                return JS_NewString(ctx, "");
            }
            // 将 kebab-case 转换为 camelCase
            std::string camel_prop;
            bool next_upper = false;
            for (const char* p = prop; *p; ++p) {
                if (*p == '-') {
                    next_upper = true;
                } else if (next_upper) {
                    camel_prop += static_cast<char>(toupper(*p));
                    next_upper = false;
                } else {
                    camel_prop += *p;
                }
            }
            JS_FreeCString(ctx, prop);
            
            JSValue val = JS_GetPropertyStr(ctx, this_val, camel_prop.c_str());
            if (JS_IsUndefined(val) || JS_IsNull(val)) {
                JS_FreeValue(ctx, val);
                return JS_NewString(ctx, "");
            }
            return val;
        }, "getPropertyValue", 1));

    return style_obj;
}

// ========== document.addEventListener 实现 ==========
// 注意：document 的事件监听器实际上委托给 body 元素处理

static JSValue JS_Document_addEventListener(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "addEventListener requires at least 2 arguments");
    }

    // 获取 document.body 并委托给它
    JSValue body = JS_GetPropertyStr(ctx, this_val, "body");
    if (JS_IsNull(body) || JS_IsUndefined(body)) {
        JS_FreeValue(ctx, body);
        return JS_UNDEFINED;
    }

    JSValue addEventListener = JS_GetPropertyStr(ctx, body, "addEventListener");
    if (JS_IsFunction(ctx, addEventListener)) {
        JSValue result = JS_Call(ctx, addEventListener, body, argc, argv);
        JS_FreeValue(ctx, addEventListener);
        JS_FreeValue(ctx, body);
        return result;
    }

    JS_FreeValue(ctx, addEventListener);
    JS_FreeValue(ctx, body);
    return JS_UNDEFINED;
}

static JSValue JS_Document_removeEventListener(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "removeEventListener requires at least 2 arguments");
    }

    // 获取 document.body 并委托给它
    JSValue body = JS_GetPropertyStr(ctx, this_val, "body");
    if (JS_IsNull(body) || JS_IsUndefined(body)) {
        JS_FreeValue(ctx, body);
        return JS_UNDEFINED;
    }

    JSValue removeEventListener = JS_GetPropertyStr(ctx, body, "removeEventListener");
    if (JS_IsFunction(ctx, removeEventListener)) {
        JSValue result = JS_Call(ctx, removeEventListener, body, argc, argv);
        JS_FreeValue(ctx, removeEventListener);
        JS_FreeValue(ctx, body);
        return result;
    }

    JS_FreeValue(ctx, removeEventListener);
    JS_FreeValue(ctx, body);
    return JS_UNDEFINED;
}

// ========== document.caretRangeFromPoint 实现 ==========

static JSValue JS_Document_caretRangeFromPoint(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2) {
        return JS_NULL;
    }

    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[0]) || JS_ToFloat64(ctx, &y, argv[1])) {
        return JS_NULL;
    }

    // 获取 Window 指针
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

    // 使用 HitTestController 找到坐标处的元素
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) {
        return JS_NULL;
    }

    HitTestController hit_controller;
    HitTestRequest request;
    auto result_ex = hit_controller.HitTest(root_render, static_cast<float>(x), static_cast<float>(y), request);
    HitTestResult hit_result;
    if (result_ex.IsValid()) {
        hit_result.element = result_ex.element;
        hit_result.render_object = result_ex.render_object;
        hit_result.local_x = result_ex.local_x;
        hit_result.local_y = result_ex.local_y;
    }

    if (!hit_result.IsValid() || !hit_result.element) {
        return JS_NULL;
    }

    // 查找元素中的文本节点
    std::shared_ptr<Text> text_node = nullptr;
    std::shared_ptr<RenderObject> text_render = nullptr;
    
    // 首先检查命中的 RenderObject 是否是文本
    if (hit_result.render_object) {
        auto node = hit_result.render_object->GetNode();
        if (node && node->GetNodeType() == NodeType::TEXT_NODE) {
            text_node = std::dynamic_pointer_cast<Text>(node);
            text_render = hit_result.render_object;
        } else {
            // 遍历子 RenderObject 查找文本节点
            for (const auto& child : hit_result.render_object->GetChildren()) {
                auto child_node = child->GetNode();
                if (child_node && child_node->GetNodeType() == NodeType::TEXT_NODE) {
                    text_node = std::dynamic_pointer_cast<Text>(child_node);
                    text_render = child;
                    break;
                }
            }
        }
    }

    // 如果没有找到文本节点，尝试从元素的子节点中查找
    if (!text_node) {
        for (const auto& child : hit_result.element->GetChildNodes()) {
            if (child->GetNodeType() == NodeType::TEXT_NODE) {
                text_node = std::dynamic_pointer_cast<Text>(child);
                break;
            }
        }
    }

    // 创建 Range
    auto range = doc->CreateRange();
    if (!range) {
        return JS_NULL;
    }

    if (!text_node) {
        // 没有文本节点，返回指向元素开头的 Range
        range->SetStart(hit_result.element, 0);
        range->SetEnd(hit_result.element, 0);
        return bindings::WrapRange(ctx, range);
    }

    // 计算字符偏移量
    std::string text = text_node->GetTextContent();
    if (text.empty()) {
        range->SetStart(text_node, 0);
        range->SetEnd(text_node, 0);
        return bindings::WrapRange(ctx, range);
    }

    // 获取文本渲染的样式信息
    float font_size = 16.0f;
    std::string font_family = "sans-serif";
    
    if (text_render) {
        const auto& style = text_render->GetComputedStyle();
        font_size = style.font_size;
        font_family = style.font_family.empty() ? "sans-serif" : style.font_family;
    } else if (hit_result.render_object) {
        const auto& style = hit_result.render_object->GetComputedStyle();
        font_size = style.font_size;
        font_family = style.font_family.empty() ? "sans-serif" : style.font_family;
    }

    // 创建字体
    FontDescriptor desc;
    desc.family = font_family;
    desc.size = font_size;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // 计算 local_x（相对于文本起始位置）
    float local_x = hit_result.local_x;
    
    // 如果有 padding，需要减去
    if (hit_result.render_object) {
        const auto& style = hit_result.render_object->GetComputedStyle();
        local_x -= style.padding.left.ToPx();
    }

    // 遍历字符计算偏移量
    TextRenderer text_renderer(nullptr);
    size_t char_count = utf8::CharCount(text);
    int char_offset = 0;
    float accumulated_width = 0.0f;

    for (size_t i = 0; i < char_count; ++i) {
        size_t byte_start = utf8::CharPosToBytePos(text, static_cast<int>(i));
        size_t byte_end = utf8::CharPosToBytePos(text, static_cast<int>(i + 1));
        std::string char_str = text.substr(byte_start, byte_end - byte_start);
        
        float char_width = text_renderer.MeasureTextWidthWithEmoji(char_str, font);
        
        // 如果点击位置在字符中间偏左，选择当前字符；偏右则选择下一个
        if (local_x < accumulated_width + char_width / 2) {
            break;
        }
        
        accumulated_width += char_width;
        char_offset = static_cast<int>(i + 1);
    }

    // 设置 Range
    range->SetStart(text_node, char_offset);
    range->SetEnd(text_node, char_offset);

    return bindings::WrapRange(ctx, range);
}

// ========== document.elementFromPoint 实现 ==========

static JSValue JS_Document_elementFromPoint(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2) {
        return JS_NULL;
    }

    double x, y;
    if (JS_ToFloat64(ctx, &x, argv[0]) || JS_ToFloat64(ctx, &y, argv[1])) {
        return JS_NULL;
    }

    // 获取 Window 指针
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "__mbink_window_ptr");
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

    // 使用 HitTestController 找到坐标处的元素
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) {
        return JS_NULL;
    }

    HitTestController hit_controller;
    HitTestRequest request;
    auto result_ex = hit_controller.HitTest(root_render, static_cast<float>(x), static_cast<float>(y), request);
    HitTestResult hit_result;
    if (result_ex.IsValid()) {
        hit_result.element = result_ex.element;
        hit_result.render_object = result_ex.render_object;
        hit_result.local_x = result_ex.local_x;
        hit_result.local_y = result_ex.local_y;
    }

    if (!hit_result.IsValid() || !hit_result.element) {
        return JS_NULL;
    }

    return bindings::WrapElement(ctx, hit_result.element);
}

// ========== 绑定函数 ==========

void BindDocumentAPIs(JSContext* ctx, Window* window) {
    // 保存 window 指针到全局对象（用于回调中访问）
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_ptr = JS_NewInt64(ctx, (int64_t)window);
    JS_SetPropertyStr(ctx, global, "__mbink_window_ptr", window_ptr);

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

        // 设置 documentElement 属性（根元素，通常是 <html>）
        auto document_element = doc->GetDocumentElement();
        if (document_element) {
            JSValue doc_elem_val = bindings::WrapElement(ctx, document_element);
            JS_SetPropertyStr(ctx, document, "documentElement", doc_elem_val);
        }
    }

    // 设置 getElementById 方法
    JS_SetPropertyStr(ctx, document, "getElementById",
        JS_NewCFunction(ctx, JS_Document_getElementById, "getElementById", 1));

    // 设置 createElement 方法
    JS_SetPropertyStr(ctx, document, "createElement",
        JS_NewCFunction(ctx, JS_Document_createElement, "createElement", 1));

    // 设置 createElementNS 方法（用于 SVG 等命名空间元素）
    JS_SetPropertyStr(ctx, document, "createElementNS",
        JS_NewCFunction(ctx, JS_Document_createElementNS, "createElementNS", 2));

    // 设置 createTextNode 方法
    JS_SetPropertyStr(ctx, document, "createTextNode",
        JS_NewCFunction(ctx, JS_Document_createTextNode, "createTextNode", 1));

    // 设置 createDocumentFragment 方法
    JS_SetPropertyStr(ctx, document, "createDocumentFragment",
        JS_NewCFunction(ctx, JS_Document_createDocumentFragment, "createDocumentFragment", 0));

    // 设置 createRange 方法
    JS_SetPropertyStr(ctx, document, "createRange",
        JS_NewCFunction(ctx, JS_Document_createRange, "createRange", 0));

    // 设置 querySelector 方法
    JS_SetPropertyStr(ctx, document, "querySelector",
        JS_NewCFunction(ctx, JS_Document_querySelector, "querySelector", 1));

    // 设置 querySelectorAll 方法
    JS_SetPropertyStr(ctx, document, "querySelectorAll",
        JS_NewCFunction(ctx, JS_Document_querySelectorAll, "querySelectorAll", 1));

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

    // 设置 addEventListener 方法（委托给 body）
    JS_SetPropertyStr(ctx, document, "addEventListener",
        JS_NewCFunction(ctx, JS_Document_addEventListener, "addEventListener", 3));

    // 设置 removeEventListener 方法（委托给 body）
    JS_SetPropertyStr(ctx, document, "removeEventListener",
        JS_NewCFunction(ctx, JS_Document_removeEventListener, "removeEventListener", 3));

    // 设置 caretRangeFromPoint 方法
    JS_SetPropertyStr(ctx, document, "caretRangeFromPoint",
        JS_NewCFunction(ctx, JS_Document_caretRangeFromPoint, "caretRangeFromPoint", 2));

    // 设置 elementFromPoint 方法
    JS_SetPropertyStr(ctx, document, "elementFromPoint",
        JS_NewCFunction(ctx, JS_Document_elementFromPoint, "elementFromPoint", 2));

    // 设置 hasFocus 方法（返回 true，因为我们的窗口总是有焦点）
    JS_SetPropertyStr(ctx, document, "hasFocus",
        JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
            // 简单实现：总是返回 true（假设窗口有焦点）
            return JS_TRUE;
        }, "hasFocus", 0));

    // 设置 defaultView 属性（返回 window 对象）
    JSValue window_for_defaultView = JS_GetPropertyStr(ctx, global, "window");
    if (!JS_IsUndefined(window_for_defaultView) && !JS_IsNull(window_for_defaultView)) {
        JS_SetPropertyStr(ctx, document, "defaultView", window_for_defaultView);
        // 不需要 FreeValue，因为 SetPropertyStr 会持有引用
    } else {
        // 如果 window 不存在，设置为 global 对象本身
        JS_SetPropertyStr(ctx, document, "defaultView", JS_DupValue(ctx, global));
    }

    // 设置 activeElement getter（动态获取当前活动元素）
    JSAtom activeElement_atom = JS_NewAtom(ctx, "activeElement");
    JS_DefinePropertyGetSet(ctx, document, activeElement_atom,
        JS_NewCFunction(ctx, JS_Document_get_activeElement, "get activeElement", 0),
        JS_UNDEFINED,
        JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, activeElement_atom);

    // 设置到全局对象
    JS_SetPropertyStr(ctx, global, "document", document);

    // 设置 window.getSelection
    JSValue window_obj = JS_GetPropertyStr(ctx, global, "window");
    if (!JS_IsUndefined(window_obj) && !JS_IsNull(window_obj)) {
        // 设置 window.document（重要：很多库通过 window.document 访问）
        JS_SetPropertyStr(ctx, window_obj, "document", JS_DupValue(ctx, document));
        
        JS_SetPropertyStr(ctx, window_obj, "getSelection",
            JS_NewCFunction(ctx, JS_Window_getSelection, "getSelection", 0));
        
        // 设置 window.getComputedStyle
        JS_SetPropertyStr(ctx, window_obj, "getComputedStyle",
            JS_NewCFunction(ctx, JS_Window_getComputedStyle, "getComputedStyle", 2));
        
        // 设置 window.scrollBy 和 window.scrollTo
        JS_SetPropertyStr(ctx, window_obj, "scrollBy",
            JS_NewCFunction(ctx, JS_Window_scrollBy, "scrollBy", 2));
        JS_SetPropertyStr(ctx, window_obj, "scrollTo",
            JS_NewCFunction(ctx, JS_Window_scrollTo, "scrollTo", 2));
        JS_SetPropertyStr(ctx, window_obj, "scroll",
            JS_NewCFunction(ctx, JS_Window_scrollTo, "scroll", 2));
        
        // 设置 window.requestAnimationFrame（从 globalThis 复制）
        JSValue raf = JS_GetPropertyStr(ctx, global, "requestAnimationFrame");
        if (!JS_IsUndefined(raf)) {
            JS_SetPropertyStr(ctx, window_obj, "requestAnimationFrame", raf);
        }
        
        // 设置 window.cancelAnimationFrame（从 globalThis 复制）
        JSValue caf = JS_GetPropertyStr(ctx, global, "cancelAnimationFrame");
        if (!JS_IsUndefined(caf)) {
            JS_SetPropertyStr(ctx, window_obj, "cancelAnimationFrame", caf);
        }
        
        JS_FreeValue(ctx, window_obj);
    }

    // 也设置到 globalThis 上（有些代码直接调用 getSelection()）
    JS_SetPropertyStr(ctx, global, "getSelection",
        JS_NewCFunction(ctx, JS_Window_getSelection, "getSelection", 0));
    
    // 也设置 getComputedStyle 到 globalThis 上
    JS_SetPropertyStr(ctx, global, "getComputedStyle",
        JS_NewCFunction(ctx, JS_Window_getComputedStyle, "getComputedStyle", 2));

    JS_FreeValue(ctx, global);
}

} // namespace mbink
