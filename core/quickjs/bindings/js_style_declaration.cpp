/**
 * @file js_style_declaration.cpp
 * @brief CSSStyleDeclaration 类的 JavaScript 绑定实现
 */

#include "js_style_declaration.h"
#include <iostream>

namespace lightui {
namespace bindings {

// ========== Opaque 数据结构 ==========

struct JSStyleDeclarationData {
    std::shared_ptr<CSSStyleDeclaration> style;
};

// ========== ClassID ==========

static JSClassID js_style_declaration_class_id = 0;

// ========== 析构函数 ==========

static void JSStyleDeclarationFinalizer(JSRuntime* rt, JSValue val) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(val, js_style_declaration_class_id));
    if (data) {
        delete data;
    }
}

// ========== 方法实现 ==========

// setProperty(property, value, priority?)
static JSValue JSStyleDeclaration_setProperty(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(this_val, js_style_declaration_class_id));
    if (!data || !data->style) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setProperty requires at least 2 arguments");
    }

    const char* property = JS_ToCString(ctx, argv[0]);
    const char* value = JS_ToCString(ctx, argv[1]);
    
    if (!property || !value) {
        if (property) JS_FreeCString(ctx, property);
        if (value) JS_FreeCString(ctx, value);
        return JS_EXCEPTION;
    }

    std::string priority = "";
    if (argc >= 3) {
        const char* priority_str = JS_ToCString(ctx, argv[2]);
        if (priority_str) {
            priority = priority_str;
            JS_FreeCString(ctx, priority_str);
        }
    }

    data->style->SetProperty(property, value, priority);

    JS_FreeCString(ctx, property);
    JS_FreeCString(ctx, value);

    return JS_UNDEFINED;
}

// getPropertyValue(property)
static JSValue JSStyleDeclaration_getPropertyValue(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(this_val, js_style_declaration_class_id));
    if (!data || !data->style) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getPropertyValue requires 1 argument");
    }

    const char* property = JS_ToCString(ctx, argv[0]);
    if (!property) {
        return JS_EXCEPTION;
    }

    std::string value = data->style->GetPropertyValue(property);
    JS_FreeCString(ctx, property);

    return JS_NewString(ctx, value.c_str());
}

// removeProperty(property)
static JSValue JSStyleDeclaration_removeProperty(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(this_val, js_style_declaration_class_id));
    if (!data || !data->style) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "removeProperty requires 1 argument");
    }

    const char* property = JS_ToCString(ctx, argv[0]);
    if (!property) {
        return JS_EXCEPTION;
    }

    std::string old_value = data->style->RemoveProperty(property);
    JS_FreeCString(ctx, property);

    return JS_NewString(ctx, old_value.c_str());
}

// ========== 属性访问器 ==========

// cssText getter
static JSValue JSStyleDeclaration_get_cssText(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(this_val, js_style_declaration_class_id));
    if (!data || !data->style) {
        return JS_NULL;
    }

    std::string css_text = data->style->GetCssText();
    return JS_NewString(ctx, css_text.c_str());
}

// cssText setter
static JSValue JSStyleDeclaration_set_cssText(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(this_val, js_style_declaration_class_id));
    if (!data || !data->style) {
        return JS_UNDEFINED;
    }

    const char* str = JS_ToCString(ctx, val);
    if (!str) {
        return JS_EXCEPTION;
    }

    data->style->SetCssText(str);
    JS_FreeCString(ctx, str);

    return JS_UNDEFINED;
}

// length getter
static JSValue JSStyleDeclaration_get_length(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(this_val, js_style_declaration_class_id));
    if (!data || !data->style) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, static_cast<int32_t>(data->style->Length()));
}

// ========== 类定义 ==========

static const JSCFunctionListEntry js_style_declaration_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("cssText", JSStyleDeclaration_get_cssText, JSStyleDeclaration_set_cssText, 0),
    JS_CGETSET_MAGIC_DEF("length", JSStyleDeclaration_get_length, nullptr, 0),
    JS_CFUNC_DEF("setProperty", 3, JSStyleDeclaration_setProperty),
    JS_CFUNC_DEF("getPropertyValue", 1, JSStyleDeclaration_getPropertyValue),
    JS_CFUNC_DEF("removeProperty", 1, JSStyleDeclaration_removeProperty),
};

static JSClassDef js_style_declaration_class = {
    /* class_name */ "CSSStyleDeclaration",
    /* finalizer */ JSStyleDeclarationFinalizer,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr,
};

// ========== 公共 API ==========

void InitStyleDeclarationBinding(JSContext* ctx) {
    // 创建 ClassID
    JS_NewClassID(JS_GetRuntime(ctx), &js_style_declaration_class_id);

    // 注册类
    JS_NewClass(JS_GetRuntime(ctx), js_style_declaration_class_id, &js_style_declaration_class);

    // 创建原型对象
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_style_declaration_proto_funcs, 
                               sizeof(js_style_declaration_proto_funcs) / sizeof(js_style_declaration_proto_funcs[0]));

    // 设置类的原型
    JS_SetClassProto(ctx, js_style_declaration_class_id, proto);
}

JSValue WrapStyleDeclaration(JSContext* ctx, std::shared_ptr<CSSStyleDeclaration> style) {
    if (!style) {
        return JS_NULL;
    }

    // 创建新的 JS 对象
    JSValue obj = JS_NewObjectClass(ctx, js_style_declaration_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    // 设置 opaque 数据
    auto* data = new JSStyleDeclarationData();
    data->style = style;
    JS_SetOpaque(obj, data);

    return obj;
}

std::shared_ptr<CSSStyleDeclaration> UnwrapStyleDeclaration(JSContext* ctx, JSValue value) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(value, js_style_declaration_class_id));
    if (!data) {
        return nullptr;
    }
    return data->style;
}

} // namespace bindings
} // namespace lightui
