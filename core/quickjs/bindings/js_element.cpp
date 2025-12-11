/**
 * @file js_element.cpp
 * @brief Element 类的 JavaScript 绑定实现
 */

#include "js_element.h"
#include "js_node.h"
#include "js_style_declaration.h"
#include "js_element.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include "core/dom/html_input_element.h"
#include "core/dom/html_textarea_element.h"
#include "core/dom/html_select_element.h"
#include "core/dom/selector_engine.h"
#include "core/quickjs/dom_binding_map.h"
#include "js_node.h"
#include "js_style_declaration.h"
#include "js_event.h"
#include "core/quickjs/js_value_wrapper.h"
#include <memory>
#include <string>
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

// classList getter - 返回 DOMTokenList 对象
static JSValue JSElement_get_classList(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_NULL;
    }

    // 创建 classList 对象
    JSValue classList = JS_NewObject(ctx);
    
    // add(className) 方法
    JSValue add_func = JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
        // 从闭包中获取 element
        JSValue element_val = JS_GetPropertyStr(ctx, this_val, "__element__");
        auto* data = static_cast<JSElementData*>(JS_GetOpaque(element_val, js_element_class_id));
        JS_FreeValue(ctx, element_val);
        
        if (!data || !data->element || argc < 1) {
            return JS_UNDEFINED;
        }
        
        const char* className = JS_ToCString(ctx, argv[0]);
        if (!className) {
            return JS_UNDEFINED;
        }
        
        std::string currentClasses = data->element->GetClassName();
        std::string newClass = className;
        
        // 检查是否已存在
        if (currentClasses.find(newClass) == std::string::npos) {
            if (!currentClasses.empty()) {
                currentClasses += " ";
            }
            currentClasses += newClass;
            data->element->SetClassName(currentClasses);
        }
        
        JS_FreeCString(ctx, className);
        return JS_UNDEFINED;
    }, "__add__", 1);
    
    // remove(className) 方法
    JSValue remove_func = JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
        JSValue element_val = JS_GetPropertyStr(ctx, this_val, "__element__");
        auto* data = static_cast<JSElementData*>(JS_GetOpaque(element_val, js_element_class_id));
        JS_FreeValue(ctx, element_val);
        
        if (!data || !data->element || argc < 1) {
            return JS_UNDEFINED;
        }
        
        const char* className = JS_ToCString(ctx, argv[0]);
        if (!className) {
            return JS_UNDEFINED;
        }
        
        std::string currentClasses = data->element->GetClassName();
        std::string toRemove = className;
        size_t pos = currentClasses.find(toRemove);
        
        if (pos != std::string::npos) {
            // 移除类名
            currentClasses.erase(pos, toRemove.length());
            // 清理多余空格
            while (currentClasses.find("  ") != std::string::npos) {
                currentClasses.replace(currentClasses.find("  "), 2, " ");
            }
            if (!currentClasses.empty() && currentClasses[0] == ' ') {
                currentClasses = currentClasses.substr(1);
            }
            if (!currentClasses.empty() && currentClasses.back() == ' ') {
                currentClasses.pop_back();
            }
            data->element->SetClassName(currentClasses);
        }
        
        JS_FreeCString(ctx, className);
        return JS_UNDEFINED;
    }, "__remove__", 1);
    
    // toggle(className) 方法
    JSValue toggle_func = JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
        JSValue element_val = JS_GetPropertyStr(ctx, this_val, "__element__");
        auto* data = static_cast<JSElementData*>(JS_GetOpaque(element_val, js_element_class_id));
        JS_FreeValue(ctx, element_val);
        
        if (!data || !data->element || argc < 1) {
            return JS_FALSE;
        }
        
        const char* className = JS_ToCString(ctx, argv[0]);
        if (!className) {
            return JS_FALSE;
        }
        
        std::string currentClasses = data->element->GetClassName();
        std::string toToggle = className;
        bool exists = currentClasses.find(toToggle) != std::string::npos;
        
        if (exists) {
            // 移除
            size_t pos = currentClasses.find(toToggle);
            currentClasses.erase(pos, toToggle.length());
            while (currentClasses.find("  ") != std::string::npos) {
                currentClasses.replace(currentClasses.find("  "), 2, " ");
            }
            if (!currentClasses.empty() && currentClasses[0] == ' ') {
                currentClasses = currentClasses.substr(1);
            }
            if (!currentClasses.empty() && currentClasses.back() == ' ') {
                currentClasses.pop_back();
            }
            data->element->SetClassName(currentClasses);
        } else {
            // 添加
            if (!currentClasses.empty()) {
                currentClasses += " ";
            }
            currentClasses += toToggle;
            data->element->SetClassName(currentClasses);
        }
        
        JS_FreeCString(ctx, className);
        return JS_NewBool(ctx, !exists);  // 返回切换后的状态
    }, "__toggle__", 1);
    
    // contains(className) 方法
    JSValue contains_func = JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
        JSValue element_val = JS_GetPropertyStr(ctx, this_val, "__element__");
        auto* data = static_cast<JSElementData*>(JS_GetOpaque(element_val, js_element_class_id));
        JS_FreeValue(ctx, element_val);
        
        if (!data || !data->element || argc < 1) {
            return JS_FALSE;
        }
        
        const char* className = JS_ToCString(ctx, argv[0]);
        if (!className) {
            return JS_FALSE;
        }
        
        std::string currentClasses = data->element->GetClassName();
        std::string toCheck = className;
        bool exists = currentClasses.find(toCheck) != std::string::npos;
        
        JS_FreeCString(ctx, className);
        return JS_NewBool(ctx, exists);
    }, "__contains__", 1);
    
    // 将 element 引用存储到 classList 对象
    JS_SetPropertyStr(ctx, classList, "__element__", JS_DupValue(ctx, this_val));
    
    // 设置方法
    JS_SetPropertyStr(ctx, classList, "add", add_func);
    JS_SetPropertyStr(ctx, classList, "remove", remove_func);
    JS_SetPropertyStr(ctx, classList, "toggle", toggle_func);
    JS_SetPropertyStr(ctx, classList, "contains", contains_func);
    
    return classList;
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

// value getter (for HTMLInputElement, HTMLTextAreaElement, HTMLSelectElement)
static JSValue JSElement_get_value(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_UNDEFINED;
    }

    // 尝试作为 HTMLInputElement
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(data->element);
    if (input_element) {
        std::string value = input_element->GetValue();
        return JS_NewString(ctx, value.c_str());
    }

    // 尝试作为 HTMLTextAreaElement
    auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(data->element);
    if (textarea_element) {
        std::string value = textarea_element->GetValue();
        return JS_NewString(ctx, value.c_str());
    }

    // 尝试作为 HTMLSelectElement
    auto select_element = std::dynamic_pointer_cast<HTMLSelectElement>(data->element);
    if (select_element) {
        std::string value = select_element->GetValue();
        return JS_NewString(ctx, value.c_str());
    }

    return JS_UNDEFINED;
}

// value setter (for HTMLInputElement, HTMLTextAreaElement, HTMLSelectElement)
static JSValue JSElement_set_value(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_UNDEFINED;
    }

    const char* str = JS_ToCString(ctx, val);
    if (!str) {
        return JS_UNDEFINED;
    }

    // 尝试作为 HTMLInputElement
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(data->element);
    if (input_element) {
        input_element->SetValue(str, false);  // false = 不触发事件
        JS_FreeCString(ctx, str);
        return JS_UNDEFINED;
    }

    // 尝试作为 HTMLTextAreaElement
    auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(data->element);
    if (textarea_element) {
        textarea_element->SetValue(str, false);  // false = 不触发事件
        JS_FreeCString(ctx, str);
        return JS_UNDEFINED;
    }

    // 尝试作为 HTMLSelectElement
    auto select_element = std::dynamic_pointer_cast<HTMLSelectElement>(data->element);
    if (select_element) {
        select_element->SetValue(str);
        JS_FreeCString(ctx, str);
        return JS_UNDEFINED;
    }

    JS_FreeCString(ctx, str);
    return JS_UNDEFINED;
}

// checked getter (for checkbox/radio)
static JSValue JSElement_get_checked(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_FALSE;
    }

    // 检查是否是 HTMLInputElement
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(data->element);
    if (!input_element) {
        return JS_FALSE;
    }

    bool checked = input_element->GetChecked();
    return JS_NewBool(ctx, checked);
}

// checked setter (for checkbox/radio)
static JSValue JSElement_set_checked(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_UNDEFINED;
    }

    // 检查是否是 HTMLInputElement
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(data->element);
    if (!input_element) {
        return JS_UNDEFINED;
    }

    bool checked = JS_ToBool(ctx, val);
    input_element->SetChecked(checked, false);  // false = 不触发事件

    return JS_UNDEFINED;
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

    // Web 标准：不存在的属性返回空字符串，而不是 null
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

// querySelector(selector)
static JSValue JSElement_querySelector(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_NULL;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "querySelector requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_NULL;
    }

    // 使用 SelectorEngine 查询
    auto result = SelectorEngine::QuerySelector(data->element, selector);
    JS_FreeCString(ctx, selector);

    if (!result) {
        return JS_NULL;
    }

    return WrapElement(ctx, result);
}

// querySelectorAll(selector)
static JSValue JSElement_querySelectorAll(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_NewArray(ctx);
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "querySelectorAll requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_NewArray(ctx);
    }

    // 使用 SelectorEngine 查询所有匹配元素
    auto results = SelectorEngine::QuerySelectorAll(data->element, selector);
    JS_FreeCString(ctx, selector);

    // 创建 JavaScript 数组
    JSValue array = JS_NewArray(ctx);
    for (size_t i = 0; i < results.size(); i++) {
        JSValue elem = WrapElement(ctx, results[i]);
        JS_SetPropertyUint32(ctx, array, static_cast<uint32_t>(i), elem);
    }

    return array;
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
    JS_CGETSET_MAGIC_DEF("classList", JSElement_get_classList, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("style", JSElement_get_style, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("value", JSElement_get_value, JSElement_set_value, 0),
    JS_CGETSET_MAGIC_DEF("checked", JSElement_get_checked, JSElement_set_checked, 0),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Element", JS_PROP_CONFIGURABLE),
    JS_CFUNC_DEF("setAttribute", 2, JSElement_setAttribute),
    JS_CFUNC_DEF("getAttribute", 1, JSElement_getAttribute),
    JS_CFUNC_DEF("removeAttribute", 1, JSElement_removeAttribute),
    JS_CFUNC_DEF("querySelector", 1, JSElement_querySelector),
    JS_CFUNC_DEF("querySelectorAll", 1, JSElement_querySelectorAll),
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
