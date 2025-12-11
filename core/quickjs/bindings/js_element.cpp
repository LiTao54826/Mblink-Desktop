/**
 * @file js_element.cpp
 * @brief Element 类的 JavaScript 绑定实现
 */

#include "js_element.h"
#include "js_node.h"
#include "js_style_declaration.h"
#include "js_event.h"
#include "core/quickjs/dom_binding_map.h"
#include "core/quickjs/js_value_wrapper.h"
#include <iostream>

namespace lightui {
namespace bindings {

// ========== Opaque 数据结构 ==========

struct JSElementData {
    std::shared_ptr<Element> element;
};

// ========== ClassID ==========

static JSClassID js_element_class_id = 0;

// ========== 析构函数 ==========

static void JSElementFinalizer(JSRuntime* rt, JSValue val) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(val, js_element_class_id));
    if (data) {
        // 从映射表中移除
        if (data->element) {
            DOMBindingMap::GetInstance().Remove(data->element.get());
        }
        delete data;
    }
}

// ========== 属性访问器 ==========

// tagName
static JSValue JSElement_get_tagName(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_NULL;
    }

    std::string tag = data->element->GetTagName();
    return JS_NewString(ctx, tag.c_str());
}

// id getter
static JSValue JSElement_get_id(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_NULL;
    }

    std::string id = data->element->GetAttribute("id");
    return JS_NewString(ctx, id.c_str());
}

// id setter
static JSValue JSElement_set_id(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_UNDEFINED;
    }

    const char* str = JS_ToCString(ctx, val);
    if (!str) {
        return JS_EXCEPTION;
    }

    data->element->SetAttribute("id", str);
    JS_FreeCString(ctx, str);

    return JS_UNDEFINED;
}

// className getter
static JSValue JSElement_get_className(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_NULL;
    }

    std::string class_name = data->element->GetClassName();
    return JS_NewString(ctx, class_name.c_str());
}

// className setter
static JSValue JSElement_set_className(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_UNDEFINED;
    }

    const char* str = JS_ToCString(ctx, val);
    if (!str) {
        return JS_EXCEPTION;
    }

    data->element->SetClassName(str);
    JS_FreeCString(ctx, str);

    return JS_UNDEFINED;
}

// style getter
static JSValue JSElement_get_style(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_NULL;
    }

    // 获取 CSSStyleDeclaration 对象
    auto style = data->element->GetStyleDeclaration();
    if (!style) {
        return JS_NULL;
    }

    // 包装并返回
    return WrapStyleDeclaration(ctx, style);
}

// ========== 方法实现 ==========

// setAttribute(name, value)
static JSValue JSElement_setAttribute(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setAttribute requires 2 arguments");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    const char* value = JS_ToCString(ctx, argv[1]);
    
    if (!name || !value) {
        if (name) JS_FreeCString(ctx, name);
        if (value) JS_FreeCString(ctx, value);
        return JS_EXCEPTION;
    }

    data->element->SetAttribute(name, value);

    JS_FreeCString(ctx, name);
    JS_FreeCString(ctx, value);

    return JS_UNDEFINED;
}

// getAttribute(name)
static JSValue JSElement_getAttribute(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getAttribute requires 1 argument");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_EXCEPTION;
    }

    std::string value = data->element->GetAttribute(name);
    JS_FreeCString(ctx, name);

    if (value.empty()) {
        return JS_NULL;
    }

    return JS_NewString(ctx, value.c_str());
}

// removeAttribute(name)
static JSValue JSElement_removeAttribute(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "removeAttribute requires 1 argument");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_EXCEPTION;
    }

    data->element->RemoveAttribute(name);
    JS_FreeCString(ctx, name);

    return JS_UNDEFINED;
}

// addEventListener(type, listener, options?)
static JSValue JSElement_addEventListener(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "addEventListener requires at least 2 arguments");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    if (!JS_IsFunction(ctx, argv[1])) {
        JS_FreeCString(ctx, type);
        return JS_ThrowTypeError(ctx, "Second argument must be a function");
    }

    // 解析 options（支持布尔值或对象）
    bool use_capture = false;
    bool once = false;

    if (argc >= 3) {
        if (JS_IsBool(argv[2])) {
            // 第三个参数是布尔值，表示 useCapture
            use_capture = JS_ToBool(ctx, argv[2]);
        } else if (JS_IsObject(argv[2])) {
            // 第三个参数是对象，提取 capture 和 once
            JSValue capture_val = JS_GetPropertyStr(ctx, argv[2], "capture");
            if (JS_IsBool(capture_val)) {
                use_capture = JS_ToBool(ctx, capture_val);
            }
            JS_FreeValue(ctx, capture_val);

            JSValue once_val = JS_GetPropertyStr(ctx, argv[2], "once");
            if (JS_IsBool(once_val)) {
                once = JS_ToBool(ctx, once_val);
            }
            JS_FreeValue(ctx, once_val);
        }
    }

    // 包装 JS 函数为 C++ lambda
    auto listener_wrapper = std::make_shared<JSValueWrapper>(ctx, argv[1]);
    
    uint64_t listener_id = data->element->AddEventListener(type, 
        [ctx, listener_wrapper](std::shared_ptr<Event> event) {
            // 包装 Event 对象
            JSValue event_val = WrapEvent(ctx, event);
            
            // 调用 JS 监听器函数
            JSValue result = listener_wrapper->Call(JS_UNDEFINED, 1, &event_val);
            
            // 释放
            if (JS_IsException(result)) {
                // 输出错误但不中断
                JSValue exception = JS_GetException(ctx);
                const char* err = JS_ToCString(ctx, exception);
                if (err) {
                    std::cerr << "[Event Listener Error] " << err << std::endl;
                    JS_FreeCString(ctx, err);
                }
                JS_FreeValue(ctx, exception);
            }
            JS_FreeValue(ctx, result);
            JS_FreeValue(ctx, event_val);
        }, 
        use_capture, 
        once
    );

    JS_FreeCString(ctx, type);

    return JS_NewInt64(ctx, listener_id);
}

// ========== 类定义 ==========

static const JSCFunctionListEntry js_element_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("tagName", JSElement_get_tagName, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("id", JSElement_get_id, JSElement_set_id, 0),
    JS_CGETSET_MAGIC_DEF("className", JSElement_get_className, JSElement_set_className, 0),
    JS_CGETSET_MAGIC_DEF("style", JSElement_get_style, nullptr, 0),
    JS_CFUNC_DEF("setAttribute", 2, JSElement_setAttribute),
    JS_CFUNC_DEF("getAttribute", 1, JSElement_getAttribute),
    JS_CFUNC_DEF("removeAttribute", 1, JSElement_removeAttribute),
    JS_CFUNC_DEF("addEventListener", 3, JSElement_addEventListener),
};

static JSClassDef js_element_class = {
    /* class_name */ "Element",
    /* finalizer */ JSElementFinalizer,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr,
};

// ========== 公共 API ==========

void InitElementBinding(JSContext* ctx) {
    // 创建 ClassID
    JS_NewClassID(JS_GetRuntime(ctx), &js_element_class_id);

    // 注册类
    JS_NewClass(JS_GetRuntime(ctx), js_element_class_id, &js_element_class);

    // 创建原型对象
    JSValue proto = JS_NewObject(ctx);
    
    // 设置原型链：Element.prototype.__proto__ = Node.prototype
    // 这样 Element 就能继承 Node 的所有属性和方法（firstChild, nextSibling等）
    JSValue node_proto = JS_GetClassProto(ctx, GetNodeClassID());
    if (!JS_IsNull(node_proto)) {
        JS_SetPrototype(ctx, proto, node_proto);
        JS_FreeValue(ctx, node_proto);
    }
    
    // 设置 Element 自己的属性和方法
    JS_SetPropertyFunctionList(ctx, proto, js_element_proto_funcs, 
                               sizeof(js_element_proto_funcs) / sizeof(js_element_proto_funcs[0]));

    // 设置类的原型
    JS_SetClassProto(ctx, js_element_class_id, proto);
}

JSValue WrapElement(JSContext* ctx, std::shared_ptr<Element> element) {
    if (!element) {
        return JS_NULL;
    }

    // 检查是否已经包装过（引用相等性）
    auto& map = DOMBindingMap::GetInstance();
    if (map.Has(element.get())) {
        JSValue existing = map.GetJSValue(element.get());
        return JS_DupValue(ctx, existing);
    }

    // 创建新的 JS 对象
    JSValue obj = JS_NewObjectClass(ctx, js_element_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    // 设置 opaque 数据
    auto* data = new JSElementData();
    data->element = element;
    JS_SetOpaque(obj, data);

    // 记录到映射表
    map.SetJSValue(element.get(), obj, ctx);

    return obj;
}

std::shared_ptr<Element> UnwrapElement(JSContext* ctx, JSValue value) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(value, js_element_class_id));
    if (!data) {
        return nullptr;
    }
    return data->element;
}

} // namespace bindings
} // namespace lightui
