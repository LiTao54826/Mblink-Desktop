/**
 * @file js_style_declaration.cpp
 * @brief CSSStyleDeclaration 类的 JavaScript 绑定实现
 */

#include "js_style_declaration.h"
#include <iostream>
#include <cctype>
#include <string>

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

// ========== 动态属性拦截器 ==========

/**
 * @brief 将 camelCase 转换为 kebab-case
 * 例如: backgroundColor -> background-color
 */
static std::string CamelToKebab(const std::string& camel) {
    std::string result;
    for (size_t i = 0; i < camel.length(); ++i) {
        char c = camel[i];
        if (std::isupper(c)) {
            if (i > 0) {
                result += '-';
            }
            result += std::tolower(c);
        } else {
            result += c;
        }
    }
    return result;
}

/**
 * @brief 属性 getter 拦截器
 * 支持 style.backgroundColor 这样的访问
 */
static int JSStyleDeclaration_get_own_property(JSContext* ctx, JSPropertyDescriptor* desc,
                                                 JSValueConst obj, JSAtom prop) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(obj, js_style_declaration_class_id));
    if (!data || !data->style) {
        return 0;  // 属性不存在
    }

    // 获取属性名
    const char* prop_name = JS_AtomToCString(ctx, prop);
    if (!prop_name) {
        return -1;  // 错误
    }

    std::string prop_str(prop_name);
    JS_FreeCString(ctx, prop_name);

    // 跳过内置属性和方法
    if (prop_str == "cssText" || prop_str == "length" || 
        prop_str == "setProperty" || prop_str == "getPropertyValue" || 
        prop_str == "removeProperty") {
        return 0;  // 让默认处理器处理
    }

    // 将 camelCase 转换为 kebab-case
    std::string css_property = CamelToKebab(prop_str);
    
    // 获取属性值
    std::string value = data->style->GetPropertyValue(css_property);
    
    // 设置描述符
    if (desc) {
        desc->flags = JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE | JS_PROP_WRITABLE;
        desc->value = JS_NewString(ctx, value.c_str());
        desc->getter = JS_UNDEFINED;
        desc->setter = JS_UNDEFINED;
    }

    return 1;  // 属性存在
}

/**
 * @brief 属性 setter 拦截器
 * 支持 style.backgroundColor = 'blue' 这样的赋值
 */
static int JSStyleDeclaration_set_property(JSContext* ctx, JSValueConst obj,
                                             JSAtom prop, JSValueConst value,
                                             JSValueConst receiver, int flags) {
    auto* data = static_cast<JSStyleDeclarationData*>(JS_GetOpaque(obj, js_style_declaration_class_id));
    if (!data || !data->style) {
        return -1;  // 错误
    }

    // 获取属性名
    const char* prop_name = JS_AtomToCString(ctx, prop);
    if (!prop_name) {
        return -1;  // 错误
    }

    std::string prop_str(prop_name);
    JS_FreeCString(ctx, prop_name);

    // 跳过 length 属性（只读）
    if (prop_str == "length") {
        return 0;  // 让默认处理器处理
    }
    
    // 特殊处理 cssText 属性
    if (prop_str == "cssText") {
        const char* value_str = JS_ToCString(ctx, value);
        if (!value_str) {
            return -1;
        }
        data->style->SetCssText(value_str);
        JS_FreeCString(ctx, value_str);
        return 1;
    }

    // 将 camelCase 转换为 kebab-case
    std::string css_property = CamelToKebab(prop_str);
    
    // 获取值
    const char* value_str = JS_ToCString(ctx, value);
    if (!value_str) {
        return -1;  // 错误
    }

    // 调试日志
    static bool debug_style = std::getenv("DEBUG_STYLE") != nullptr;
    if (debug_style && css_property == "line-height") {
    }

    // 设置属性
    data->style->SetProperty(css_property, value_str, "");
    JS_FreeCString(ctx, value_str);

    return 1;  // 成功
}

// Exotic 对象处理器
static JSClassExoticMethods js_style_declaration_exotic = {
    /* get_own_property */ JSStyleDeclaration_get_own_property,
    /* get_own_property_names */ nullptr,
    /* delete_property */ nullptr,
    /* define_own_property */ nullptr,
    /* has_property */ nullptr,
    /* get_property */ nullptr,
    /* set_property */ JSStyleDeclaration_set_property,
};

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
    /* exotic */ &js_style_declaration_exotic,
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
