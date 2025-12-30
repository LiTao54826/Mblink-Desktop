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
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/elements/html_select_element.h"
#include "core/dom/elements/html_canvas_element.h"
#include "core/dom/elements/html_image_element.h"
#include "core/dom/elements/terminal/html_terminal_element.h"
#include "core/dom/elements/logview/html_logview_element.h"
#include "core/dom/bindings/canvas_bindings.h"
#include "core/dom/bindings/terminal_bindings.h"
#include "core/dom/selection/selector_engine.h"
#include "core/quickjs/dom_binding_map.h"
#include "core/render/objects/render_object.h"
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

// children getter - 返回所有子元素（HTMLCollection）
static JSValue JSElement_get_children(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_NewArray(ctx);
    }

    // 创建 JavaScript 数组表示 HTMLCollection
    JSValue array = JS_NewArray(ctx);
    uint32_t index = 0;

    // 遍历所有子节点，只添加元素节点（跳过文本节点）
    auto child = data->element->GetFirstChild();
    while (child) {
        // 检查是否是 Element 类型
        auto element_child = std::dynamic_pointer_cast<Element>(child);
        if (element_child) {
            JSValue elem = WrapElement(ctx, element_child);
            JS_SetPropertyUint32(ctx, array, index++, elem);
        }
        child = child->GetNextSibling();
    }

    return array;
}

// attributes getter - 返回 NamedNodeMap 对象
static JSValue JSElement_get_attributes(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        // 返回空的类数组对象
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "length", JS_NewInt32(ctx, 0));
        return obj;
    }

    // 获取所有属性
    const auto& attrs = data->element->GetAllAttributes();
    
    // 创建类数组对象（模拟 NamedNodeMap）
    JSValue obj = JS_NewObject(ctx);
    int index = 0;
    
    for (const auto& [name, value] : attrs) {
        // 创建 Attr 对象
        JSValue attr = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, attr, "name", JS_NewString(ctx, name.c_str()));
        JS_SetPropertyStr(ctx, attr, "value", JS_NewString(ctx, value.c_str()));
        JS_SetPropertyStr(ctx, attr, "nodeName", JS_NewString(ctx, name.c_str()));
        JS_SetPropertyStr(ctx, attr, "nodeValue", JS_NewString(ctx, value.c_str()));
        
        // 按索引设置
        JS_SetPropertyUint32(ctx, obj, index, attr);
        
        // 按名称设置（用于 getNamedItem）
        JS_SetPropertyStr(ctx, obj, name.c_str(), JS_DupValue(ctx, attr));
        
        index++;
    }
    
    // 设置 length 属性
    JS_SetPropertyStr(ctx, obj, "length", JS_NewInt32(ctx, index));
    
    // 添加 getNamedItem 方法
    JS_SetPropertyStr(ctx, obj, "getNamedItem",
        JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
            if (argc < 1) return JS_NULL;
            const char* name = JS_ToCString(ctx, argv[0]);
            if (!name) return JS_NULL;
            JSValue result = JS_GetPropertyStr(ctx, this_val, name);
            JS_FreeCString(ctx, name);
            return result;
        }, "getNamedItem", 1));
    
    // 添加 item 方法
    JS_SetPropertyStr(ctx, obj, "item",
        JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
            if (argc < 1) return JS_NULL;
            uint32_t index;
            if (JS_ToUint32(ctx, &index, argv[0]) != 0) return JS_NULL;
            return JS_GetPropertyUint32(ctx, this_val, index);
        }, "item", 1));
    
    return obj;
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

    // 调试输出
    std::cout << "[JSElement_set_value] Setting value to: '" << str << "'" << std::endl;

    // 尝试作为 HTMLInputElement
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(data->element);
    if (input_element) {
        input_element->SetValue(str, false);  // false = 不触发事件
        std::cout << "[JSElement_set_value] HTMLInputElement value set, GetValue() = '" << input_element->GetValue() << "'" << std::endl;
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
                // 尝试获取堆栈信息
                JSValue stack = JS_GetPropertyStr(ctx, exception, "stack");
                if (!JS_IsUndefined(stack)) {
                    const char* stack_str = JS_ToCString(ctx, stack);
                    if (stack_str) {
                        std::cerr << "[Event Listener Stack] " << stack_str << std::endl;
                        JS_FreeCString(ctx, stack_str);
                    }
                    JS_FreeValue(ctx, stack);
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

// HTMLCanvasElement.width getter
static JSValue JSElement_get_canvas_width(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NewInt32(ctx, 0);
    
    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(data->element);
    if (!canvas) return JS_NewInt32(ctx, 0);
    
    return JS_NewInt32(ctx, static_cast<int>(canvas->GetWidth()));
}

// HTMLCanvasElement.width setter
static JSValue JSElement_set_canvas_width(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(data->element);
    if (!canvas) return JS_UNDEFINED;
    
    int32_t width;
    if (JS_ToInt32(ctx, &width, val) != 0) return JS_EXCEPTION;
    if (width > 0) {
        canvas->SetWidth(static_cast<unsigned long>(width));
    }
    return JS_UNDEFINED;
}

// HTMLCanvasElement.height getter
static JSValue JSElement_get_canvas_height(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NewInt32(ctx, 0);
    
    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(data->element);
    if (!canvas) return JS_NewInt32(ctx, 0);
    
    return JS_NewInt32(ctx, static_cast<int>(canvas->GetHeight()));
}

// HTMLCanvasElement.height setter
static JSValue JSElement_set_canvas_height(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(data->element);
    if (!canvas) return JS_UNDEFINED;
    
    int32_t height;
    if (JS_ToInt32(ctx, &height, val) != 0) return JS_EXCEPTION;
    if (height > 0) {
        canvas->SetHeight(static_cast<unsigned long>(height));
    }
    return JS_UNDEFINED;
}

// getContext(contextId) - for HTMLCanvasElement
static JSValue JSElement_getContext(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        return JS_UNDEFINED;
    }
    
    // 尝试转换为 HTMLCanvasElement
    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(data->element);
    if (!canvas) {
        return JS_UNDEFINED;  // 不是canvas元素，返回undefined
    }
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getContext requires 1 argument");
    }
    
    const char* context_id = JS_ToCString(ctx, argv[0]);
    if (!context_id) {
        return JS_EXCEPTION;
    }
    
    // 保存context_id用于后续比较
    std::string context_id_str(context_id);
    void* context = canvas->GetContext(context_id);
    JS_FreeCString(ctx, context_id);
    
    if (!context) {
        return JS_NULL;
    }
    
    // 目前只支持 "2d" context
    if (context_id_str == "2d") {
        auto context_2d = static_cast<CanvasRenderingContext2D*>(context);
        JSValue context_obj = CanvasBindings::WrapContext2D(ctx, context_2d);
        
        // 重要：设置 canvas 属性，指向原始的 canvas 元素
        // Chart.js 需要通过 ctx.canvas 来访问 canvas 元素
        if (!JS_IsException(context_obj)) {
            JS_SetPropertyStr(ctx, context_obj, "canvas", JS_DupValue(ctx, this_val));
        }
        
        return context_obj;
    }
    
    return JS_NULL;
}

// ========== HTMLImageElement 属性 ==========

// HTMLImageElement.src getter
static JSValue JSElement_get_img_src(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NULL;
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(data->element);
    if (!img) {
        // 不是 img 元素，返回 undefined
        return JS_UNDEFINED;
    }
    
    return JS_NewString(ctx, img->GetSrc().c_str());
}

// HTMLImageElement.src setter
static JSValue JSElement_set_img_src(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(data->element);
    if (!img) return JS_UNDEFINED;
    
    const char* src = JS_ToCString(ctx, val);
    if (!src) return JS_EXCEPTION;
    
    img->SetSrc(src);
    JS_FreeCString(ctx, src);
    
    return JS_UNDEFINED;
}

// HTMLImageElement.alt getter
static JSValue JSElement_get_img_alt(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NULL;
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(data->element);
    if (!img) return JS_UNDEFINED;
    
    return JS_NewString(ctx, img->GetAlt().c_str());
}

// HTMLImageElement.alt setter
static JSValue JSElement_set_img_alt(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(data->element);
    if (!img) return JS_UNDEFINED;
    
    const char* alt = JS_ToCString(ctx, val);
    if (!alt) return JS_EXCEPTION;
    
    img->SetAlt(alt);
    JS_FreeCString(ctx, alt);
    
    return JS_UNDEFINED;
}

// HTMLImageElement.naturalWidth getter
static JSValue JSElement_get_img_naturalWidth(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NewInt32(ctx, 0);
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(data->element);
    if (!img) return JS_NewInt32(ctx, 0);
    
    return JS_NewUint32(ctx, img->GetNaturalWidth());
}

// HTMLImageElement.naturalHeight getter
static JSValue JSElement_get_img_naturalHeight(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NewInt32(ctx, 0);
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(data->element);
    if (!img) return JS_NewInt32(ctx, 0);
    
    return JS_NewUint32(ctx, img->GetNaturalHeight());
}

// HTMLImageElement.complete getter
static JSValue JSElement_get_img_complete(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_FALSE;
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(data->element);
    if (!img) return JS_FALSE;
    
    return JS_NewBool(ctx, img->GetComplete());
}

// HTMLImageElement.crossOrigin getter
static JSValue JSElement_get_img_crossOrigin(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NULL;
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(data->element);
    if (!img) return JS_NULL;
    
    std::string cross_origin = img->GetCrossOrigin();
    if (cross_origin.empty()) {
        return JS_NULL;
    }
    return JS_NewString(ctx, cross_origin.c_str());
}

// HTMLImageElement.crossOrigin setter
static JSValue JSElement_set_img_crossOrigin(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(data->element);
    if (!img) return JS_UNDEFINED;
    
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        img->SetCrossOrigin("");
    } else {
        const char* cross_origin = JS_ToCString(ctx, val);
        if (!cross_origin) return JS_EXCEPTION;
        img->SetCrossOrigin(cross_origin);
        JS_FreeCString(ctx, cross_origin);
    }
    
    return JS_UNDEFINED;
}

// onload getter - 存储在 JS 对象的隐藏属性中
static JSValue JSElement_get_onload(JSContext* ctx, JSValueConst this_val, int magic) {
    return JS_GetPropertyStr(ctx, this_val, "__onload__");
}

// onload setter - 设置 load 事件监听器
static JSValue JSElement_set_onload(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    std::cout << "[JSElement_set_onload] Setting onload handler" << std::endl;
    
    // 存储回调函数
    JS_SetPropertyStr(ctx, this_val, "__onload__", JS_DupValue(ctx, val));
    
    if (JS_IsFunction(ctx, val)) {
        std::cout << "[JSElement_set_onload] Adding load event listener" << std::endl;
        
        // 包装 JS 函数为 C++ lambda
        auto listener_wrapper = std::make_shared<JSValueWrapper>(ctx, val);
        
        uint64_t listener_id = data->element->AddEventListener("load", 
            [ctx, listener_wrapper](std::shared_ptr<Event> event) {
                std::cout << "[onload callback] Executing onload callback" << std::endl;
                JSValue event_val = WrapEvent(ctx, event);
                JSValue result = listener_wrapper->Call(JS_UNDEFINED, 1, &event_val);
                if (JS_IsException(result)) {
                    JSValue exception = JS_GetException(ctx);
                    const char* err = JS_ToCString(ctx, exception);
                    if (err) {
                        std::cerr << "[onload Error] " << err << std::endl;
                        JS_FreeCString(ctx, err);
                    }
                    JS_FreeValue(ctx, exception);
                }
                JS_FreeValue(ctx, result);
                JS_FreeValue(ctx, event_val);
                std::cout << "[onload callback] Callback completed" << std::endl;
            }, 
            false, false
        );
        
        std::cout << "[JSElement_set_onload] Listener added with id: " << listener_id << std::endl;
    }
    
    return JS_UNDEFINED;
}

// onerror getter
static JSValue JSElement_get_onerror(JSContext* ctx, JSValueConst this_val, int magic) {
    return JS_GetPropertyStr(ctx, this_val, "__onerror__");
}

// onerror setter - 设置 error 事件监听器
static JSValue JSElement_set_onerror(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    // 存储回调函数
    JS_SetPropertyStr(ctx, this_val, "__onerror__", JS_DupValue(ctx, val));
    
    if (JS_IsFunction(ctx, val)) {
        // 包装 JS 函数为 C++ lambda
        auto listener_wrapper = std::make_shared<JSValueWrapper>(ctx, val);
        
        data->element->AddEventListener("error", 
            [ctx, listener_wrapper](std::shared_ptr<Event> event) {
                JSValue event_val = WrapEvent(ctx, event);
                JSValue result = listener_wrapper->Call(JS_UNDEFINED, 1, &event_val);
                if (JS_IsException(result)) {
                    JSValue exception = JS_GetException(ctx);
                    const char* err = JS_ToCString(ctx, exception);
                    if (err) {
                        std::cerr << "[onerror Error] " << err << std::endl;
                        JS_FreeCString(ctx, err);
                    }
                    JS_FreeValue(ctx, exception);
                }
                JS_FreeValue(ctx, result);
                JS_FreeValue(ctx, event_val);
            }, 
            false, false
        );
    }
    
    return JS_UNDEFINED;
}

// scrollTop getter - 获取垂直滚动位置
static JSValue JSElement_get_scrollTop(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NewFloat64(ctx, 0);
    
    auto render_obj = data->element->GetRenderObject();
    if (!render_obj) return JS_NewFloat64(ctx, 0);
    
    return JS_NewFloat64(ctx, render_obj->GetScrollY());
}

// scrollTop setter - 设置垂直滚动位置
static JSValue JSElement_set_scrollTop(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    auto render_obj = data->element->GetRenderObject();
    if (!render_obj) return JS_UNDEFINED;
    
    double scroll_top;
    if (JS_ToFloat64(ctx, &scroll_top, val) != 0) return JS_EXCEPTION;
    
    // 确保滚动位置不为负
    if (scroll_top < 0) scroll_top = 0;
    
    render_obj->SetScrollY(static_cast<float>(scroll_top));
    
    return JS_UNDEFINED;
}

// isContentEditable getter - 检查元素是否可编辑
static JSValue JSElement_get_isContentEditable(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_FALSE;
    
    return JS_NewBool(ctx, data->element->IsContentEditable());
}

// contentEditable getter - 获取 contenteditable 属性值
static JSValue JSElement_get_contentEditable(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NewString(ctx, "inherit");
    
    std::string value = data->element->GetAttribute("contenteditable");
    if (value.empty()) {
        return JS_NewString(ctx, "inherit");
    }
    return JS_NewString(ctx, value.c_str());
}

// contentEditable setter - 设置 contenteditable 属性值
static JSValue JSElement_set_contentEditable(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    const char* str = JS_ToCString(ctx, val);
    if (!str) return JS_EXCEPTION;
    
    std::string value(str);
    JS_FreeCString(ctx, str);
    
    if (value == "true" || value == "false" || value == "inherit") {
        data->element->SetAttribute("contenteditable", value);
    } else {
        return JS_ThrowTypeError(ctx, "contentEditable must be 'true', 'false', or 'inherit'");
    }
    
    return JS_UNDEFINED;
}

// innerHTML getter
static JSValue JSElement_get_innerHTML(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NewString(ctx, "");
    
    return JS_NewString(ctx, data->element->GetInnerHTML().c_str());
}

// innerHTML setter
static JSValue JSElement_set_innerHTML(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    const char* str = JS_ToCString(ctx, val);
    if (!str) return JS_EXCEPTION;
    
    data->element->SetInnerHTML(str);
    JS_FreeCString(ctx, str);
    
    return JS_UNDEFINED;
}

// outerHTML getter
static JSValue JSElement_get_outerHTML(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NewString(ctx, "");
    
    return JS_NewString(ctx, data->element->GetOuterHTML().c_str());
}

// scrollLeft getter - 获取水平滚动位置
static JSValue JSElement_get_scrollLeft(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NewFloat64(ctx, 0);
    
    auto render_obj = data->element->GetRenderObject();
    if (!render_obj) return JS_NewFloat64(ctx, 0);
    
    return JS_NewFloat64(ctx, render_obj->GetScrollX());
}

// scrollLeft setter - 设置水平滚动位置
static JSValue JSElement_set_scrollLeft(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    auto render_obj = data->element->GetRenderObject();
    if (!render_obj) return JS_UNDEFINED;
    
    double scroll_left;
    if (JS_ToFloat64(ctx, &scroll_left, val) != 0) return JS_EXCEPTION;
    
    // 确保滚动位置不为负
    if (scroll_left < 0) scroll_left = 0;
    
    render_obj->SetScrollX(static_cast<float>(scroll_left));
    
    return JS_UNDEFINED;
}

// getBoundingClientRect - 获取元素的边界矩形
static JSValue JSElement_getBoundingClientRect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        // 返回空的 DOMRect
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "x", JS_NewFloat64(ctx, 0));
        JS_SetPropertyStr(ctx, obj, "y", JS_NewFloat64(ctx, 0));
        JS_SetPropertyStr(ctx, obj, "width", JS_NewFloat64(ctx, 0));
        JS_SetPropertyStr(ctx, obj, "height", JS_NewFloat64(ctx, 0));
        JS_SetPropertyStr(ctx, obj, "top", JS_NewFloat64(ctx, 0));
        JS_SetPropertyStr(ctx, obj, "right", JS_NewFloat64(ctx, 0));
        JS_SetPropertyStr(ctx, obj, "bottom", JS_NewFloat64(ctx, 0));
        JS_SetPropertyStr(ctx, obj, "left", JS_NewFloat64(ctx, 0));
        return obj;
    }

    auto rect = data->element->GetBoundingClientRect();

    // 创建 DOMRect 对象
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "x", JS_NewFloat64(ctx, rect.x));
    JS_SetPropertyStr(ctx, obj, "y", JS_NewFloat64(ctx, rect.y));
    JS_SetPropertyStr(ctx, obj, "width", JS_NewFloat64(ctx, rect.width));
    JS_SetPropertyStr(ctx, obj, "height", JS_NewFloat64(ctx, rect.height));
    JS_SetPropertyStr(ctx, obj, "top", JS_NewFloat64(ctx, rect.top));
    JS_SetPropertyStr(ctx, obj, "right", JS_NewFloat64(ctx, rect.right));
    JS_SetPropertyStr(ctx, obj, "bottom", JS_NewFloat64(ctx, rect.bottom));
    JS_SetPropertyStr(ctx, obj, "left", JS_NewFloat64(ctx, rect.left));

    return obj;
}

// getClientRects - 获取元素的所有边界矩形（用于多行文本等）
static JSValue JSElement_getClientRects(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    
    // 创建数组来存储矩形
    JSValue arr = JS_NewArray(ctx);
    
    if (!data || !data->element) {
        return arr;  // 返回空数组
    }

    // 获取单个边界矩形（简化实现，对于大多数元素只有一个矩形）
    auto rect = data->element->GetBoundingClientRect();

    // 创建 DOMRect 对象
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "x", JS_NewFloat64(ctx, rect.x));
    JS_SetPropertyStr(ctx, obj, "y", JS_NewFloat64(ctx, rect.y));
    JS_SetPropertyStr(ctx, obj, "width", JS_NewFloat64(ctx, rect.width));
    JS_SetPropertyStr(ctx, obj, "height", JS_NewFloat64(ctx, rect.height));
    JS_SetPropertyStr(ctx, obj, "top", JS_NewFloat64(ctx, rect.top));
    JS_SetPropertyStr(ctx, obj, "right", JS_NewFloat64(ctx, rect.right));
    JS_SetPropertyStr(ctx, obj, "bottom", JS_NewFloat64(ctx, rect.bottom));
    JS_SetPropertyStr(ctx, obj, "left", JS_NewFloat64(ctx, rect.left));

    // 添加到数组
    JS_SetPropertyUint32(ctx, arr, 0, obj);

    return arr;
}

// scrollIntoView - 滚动元素到可见区域
static JSValue JSElement_scrollIntoView(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;

    bool align_to_top = true;
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        align_to_top = JS_ToBool(ctx, argv[0]);
    }

    data->element->ScrollIntoView(align_to_top);
    return JS_UNDEFINED;
}

// select - 选中输入框中的所有文本（用于 input/textarea）
static JSValue JSElement_select(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;

    // 获取 value 属性的长度
    std::string value = data->element->GetAttribute("value");
    
    // 设置 selectionStart 和 selectionEnd
    data->element->SetAttribute("selectionStart", "0");
    data->element->SetAttribute("selectionEnd", std::to_string(value.length()));
    
    return JS_UNDEFINED;
}

// focus - 使元素获得焦点
static JSValue JSElement_focus(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    std::cout << "[JSElement_focus] Called" << std::endl;
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) {
        std::cout << "[JSElement_focus] ERROR: No element data!" << std::endl;
        return JS_UNDEFINED;
    }
    
    std::cout << "[JSElement_focus] Calling Focus() on <" << data->element->GetTagName() << ">" << std::endl;
    data->element->Focus();
    std::cout << "[JSElement_focus] Focus() returned" << std::endl;
    return JS_UNDEFINED;
}

// blur - 使元素失去焦点
static JSValue JSElement_blur(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    data->element->Blur();
    return JS_UNDEFINED;
}

// contains - 检查元素是否包含另一个节点
static JSValue JSElement_contains(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_FALSE;
    
    if (argc < 1) return JS_FALSE;
    
    // 尝试解包为 Node（支持 Element 和 Text 节点）
    auto other = UnwrapNode(ctx, argv[0]);
    if (!other) return JS_FALSE;
    
    // 使用 Node::Contains 方法检查
    return JS_NewBool(ctx, data->element->Contains(other));
}

// matches - 检查元素是否匹配选择器
static JSValue JSElement_matches(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_FALSE;
    
    if (argc < 1) return JS_ThrowTypeError(ctx, "matches requires 1 argument");
    
    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) return JS_EXCEPTION;
    
    bool result = data->element->Matches(selector);
    JS_FreeCString(ctx, selector);
    
    return JS_NewBool(ctx, result);
}

// closest - 查找最近的匹配选择器的祖先元素
static JSValue JSElement_closest(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NULL;
    
    if (argc < 1) return JS_ThrowTypeError(ctx, "closest requires 1 argument");
    
    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) return JS_EXCEPTION;
    
    auto result = data->element->Closest(selector);
    JS_FreeCString(ctx, selector);
    
    if (!result) return JS_NULL;
    return WrapElement(ctx, result);
}

// cloneNode - 克隆节点
static JSValue JSElement_cloneNode(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NULL;
    
    bool deep = false;
    if (argc > 0) {
        deep = JS_ToBool(ctx, argv[0]);
    }
    
    auto cloned = data->element->CloneNode(deep);
    if (!cloned) return JS_NULL;
    
    auto cloned_element = std::dynamic_pointer_cast<Element>(cloned);
    if (!cloned_element) return JS_NULL;
    
    return WrapElement(ctx, cloned_element);
}

// remove - 从 DOM 中移除元素（DOM4 方法）
static JSValue JSElement_remove(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_UNDEFINED;
    
    auto parent = data->element->GetParentNode();
    if (parent) {
        parent->RemoveChild(data->element);
    }
    
    return JS_UNDEFINED;
}

// ownerDocument getter - 获取元素所属的文档
static JSValue JSElement_get_ownerDocument(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NULL;

    // 返回全局的 document 对象
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue document = JS_GetPropertyStr(ctx, global, "document");
    JS_FreeValue(ctx, global);
    
    return document;
}

// parentNode getter - 获取父节点
static JSValue JSElement_get_parentNode(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NULL;

    auto parent = data->element->GetParentNode();
    if (!parent) return JS_NULL;

    // 尝试转换为 Element
    auto parent_element = std::dynamic_pointer_cast<Element>(parent);
    if (parent_element) {
        return WrapElement(ctx, parent_element);
    }

    return JS_NULL;
}

// parentElement getter - 获取父元素
static JSValue JSElement_get_parentElement(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_NULL;

    auto parent = data->element->GetParentNode();
    if (!parent) return JS_NULL;

    // 只返回 Element 类型的父节点
    auto parent_element = std::dynamic_pointer_cast<Element>(parent);
    if (parent_element) {
        return WrapElement(ctx, parent_element);
    }

    return JS_NULL;
}

// ========== 类定义 ==========

static const JSCFunctionListEntry js_element_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("tagName", JSElement_get_tagName, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("id", JSElement_get_id, JSElement_set_id, 0),
    JS_CGETSET_MAGIC_DEF("className", JSElement_get_className, JSElement_set_className, 0),
    JS_CGETSET_MAGIC_DEF("classList", JSElement_get_classList, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("children", JSElement_get_children, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("attributes", JSElement_get_attributes, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("style", JSElement_get_style, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("value", JSElement_get_value, JSElement_set_value, 0),
    JS_CGETSET_MAGIC_DEF("checked", JSElement_get_checked, JSElement_set_checked, 0),
    // DOM 树导航
    JS_CGETSET_MAGIC_DEF("ownerDocument", JSElement_get_ownerDocument, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("parentNode", JSElement_get_parentNode, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("parentElement", JSElement_get_parentElement, nullptr, 0),
    // 滚动属性
    JS_CGETSET_MAGIC_DEF("scrollTop", JSElement_get_scrollTop, JSElement_set_scrollTop, 0),
    JS_CGETSET_MAGIC_DEF("scrollLeft", JSElement_get_scrollLeft, JSElement_set_scrollLeft, 0),
    // contentEditable 属性
    JS_CGETSET_MAGIC_DEF("isContentEditable", JSElement_get_isContentEditable, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("contentEditable", JSElement_get_contentEditable, JSElement_set_contentEditable, 0),
    // innerHTML/outerHTML 属性
    JS_CGETSET_MAGIC_DEF("innerHTML", JSElement_get_innerHTML, JSElement_set_innerHTML, 0),
    JS_CGETSET_MAGIC_DEF("outerHTML", JSElement_get_outerHTML, nullptr, 0),
    // HTMLCanvasElement 属性
    JS_CGETSET_MAGIC_DEF("width", JSElement_get_canvas_width, JSElement_set_canvas_width, 0),
    JS_CGETSET_MAGIC_DEF("height", JSElement_get_canvas_height, JSElement_set_canvas_height, 0),
    // HTMLImageElement 属性
    JS_CGETSET_MAGIC_DEF("src", JSElement_get_img_src, JSElement_set_img_src, 0),
    JS_CGETSET_MAGIC_DEF("alt", JSElement_get_img_alt, JSElement_set_img_alt, 0),
    JS_CGETSET_MAGIC_DEF("naturalWidth", JSElement_get_img_naturalWidth, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("naturalHeight", JSElement_get_img_naturalHeight, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("complete", JSElement_get_img_complete, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("crossOrigin", JSElement_get_img_crossOrigin, JSElement_set_img_crossOrigin, 0),
    // 事件处理属性
    JS_CGETSET_MAGIC_DEF("onload", JSElement_get_onload, JSElement_set_onload, 0),
    JS_CGETSET_MAGIC_DEF("onerror", JSElement_get_onerror, JSElement_set_onerror, 0),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Element", JS_PROP_CONFIGURABLE),
    JS_CFUNC_DEF("setAttribute", 2, JSElement_setAttribute),
    JS_CFUNC_DEF("getAttribute", 1, JSElement_getAttribute),
    JS_CFUNC_DEF("removeAttribute", 1, JSElement_removeAttribute),
    JS_CFUNC_DEF("querySelector", 1, JSElement_querySelector),
    JS_CFUNC_DEF("querySelectorAll", 1, JSElement_querySelectorAll),
    JS_CFUNC_DEF("addEventListener", 3, JSElement_addEventListener),
    JS_CFUNC_DEF("getContext", 1, JSElement_getContext),
    JS_CFUNC_DEF("getBoundingClientRect", 0, JSElement_getBoundingClientRect),
    JS_CFUNC_DEF("getClientRects", 0, JSElement_getClientRects),
    JS_CFUNC_DEF("scrollIntoView", 1, JSElement_scrollIntoView),
    JS_CFUNC_DEF("select", 0, JSElement_select),
    JS_CFUNC_DEF("focus", 0, JSElement_focus),
    JS_CFUNC_DEF("blur", 0, JSElement_blur),
    JS_CFUNC_DEF("contains", 1, JSElement_contains),
    JS_CFUNC_DEF("matches", 1, JSElement_matches),
    JS_CFUNC_DEF("closest", 1, JSElement_closest),
    JS_CFUNC_DEF("cloneNode", 1, JSElement_cloneNode),
    JS_CFUNC_DEF("remove", 0, JSElement_remove),
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

    // 如果是 Terminal 元素，添加 Terminal 特定方法
    if (auto* terminal = dynamic_cast<HTMLTerminalElement*>(element.get())) {
        // 添加 write 方法
        JS_SetPropertyStr(ctx, obj, "write", JS_NewCFunction(ctx, 
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());
                if (!terminal) return JS_EXCEPTION;
                if (argc < 1) return JS_ThrowTypeError(ctx, "write requires 1 argument");
                const char* data = JS_ToCString(ctx, argv[0]);
                if (!data) return JS_EXCEPTION;
                terminal->Write(data);
                JS_FreeCString(ctx, data);
                return JS_UNDEFINED;
            }, "write", 1));
        
        // 添加 clear 方法
        JS_SetPropertyStr(ctx, obj, "clear", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());
                if (!terminal) return JS_EXCEPTION;
                terminal->Clear();
                return JS_UNDEFINED;
            }, "clear", 0));

        // 添加 scrollTo 方法
        JS_SetPropertyStr(ctx, obj, "scrollTo", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());
                if (!terminal) return JS_EXCEPTION;
                if (argc < 1) return JS_ThrowTypeError(ctx, "scrollTo requires 1 argument");
                int32_t line;
                if (JS_ToInt32(ctx, &line, argv[0]) != 0) return JS_EXCEPTION;
                terminal->ScrollTo(line);
                return JS_UNDEFINED;
            }, "scrollTo", 1));

        // 添加 focus 方法
        JS_SetPropertyStr(ctx, obj, "focus", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());
                if (!terminal) return JS_EXCEPTION;
                terminal->Focus();
                return JS_UNDEFINED;
            }, "focus", 0));

        // 添加 execute 方法
        JS_SetPropertyStr(ctx, obj, "execute", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());
                if (!terminal) return JS_EXCEPTION;
                if (argc < 1) return JS_ThrowTypeError(ctx, "execute requires 1 argument");
                const char* cmd = JS_ToCString(ctx, argv[0]);
                if (!cmd) return JS_EXCEPTION;
                terminal->Execute(cmd);
                JS_FreeCString(ctx, cmd);
                return JS_UNDEFINED;
            }, "execute", 1));

        // 添加 serialize 方法
        JS_SetPropertyStr(ctx, obj, "serialize", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());
                if (!terminal) return JS_EXCEPTION;
                std::string text = terminal->Serialize();
                return JS_NewString(ctx, text.c_str());
            }, "serialize", 0));

        // 添加 startShell 方法
        JS_SetPropertyStr(ctx, obj, "startShell", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());
                if (!terminal) return JS_EXCEPTION;
                std::string shell;
                if (argc >= 1) {
                    const char* s = JS_ToCString(ctx, argv[0]);
                    if (s) {
                        shell = s;
                        JS_FreeCString(ctx, s);
                    }
                }
                terminal->StartShell(shell);
                return JS_UNDEFINED;
            }, "startShell", 1));

        // 添加 sendInput 方法
        JS_SetPropertyStr(ctx, obj, "sendInput", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());
                if (!terminal) return JS_EXCEPTION;
                if (argc < 1) return JS_ThrowTypeError(ctx, "sendInput requires 1 argument");
                const char* input = JS_ToCString(ctx, argv[0]);
                if (!input) return JS_EXCEPTION;
                terminal->SendInput(input);
                JS_FreeCString(ctx, input);
                return JS_UNDEFINED;
            }, "sendInput", 1));

        // 添加 resize 方法
        JS_SetPropertyStr(ctx, obj, "resize", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());
                if (!terminal) return JS_EXCEPTION;
                if (argc < 2) return JS_ThrowTypeError(ctx, "resize requires 2 arguments");
                int32_t rows, cols;
                if (JS_ToInt32(ctx, &rows, argv[0]) != 0) return JS_EXCEPTION;
                if (JS_ToInt32(ctx, &cols, argv[1]) != 0) return JS_EXCEPTION;
                terminal->Resize(rows, cols);
                return JS_UNDEFINED;
            }, "resize", 2));
    }

    // 如果是 LogView 元素，添加 LogView 特定方法
    if (auto* logview = dynamic_cast<HTMLLogViewElement*>(element.get())) {
        // 添加 append 方法
        JS_SetPropertyStr(ctx, obj, "append", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                if (argc < 3) return JS_ThrowTypeError(ctx, "append requires 3 arguments");
                const char* level = JS_ToCString(ctx, argv[0]);
                const char* source = JS_ToCString(ctx, argv[1]);
                const char* message = JS_ToCString(ctx, argv[2]);
                if (!level || !source || !message) {
                    if (level) JS_FreeCString(ctx, level);
                    if (source) JS_FreeCString(ctx, source);
                    if (message) JS_FreeCString(ctx, message);
                    return JS_EXCEPTION;
                }
                logview->Append(level, source, message);
                JS_FreeCString(ctx, level);
                JS_FreeCString(ctx, source);
                JS_FreeCString(ctx, message);
                return JS_UNDEFINED;
            }, "append", 3));
        
        // 添加 clear 方法
        JS_SetPropertyStr(ctx, obj, "clear", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                logview->Clear();
                return JS_UNDEFINED;
            }, "clear", 0));

        // 添加 scrollTo 方法
        JS_SetPropertyStr(ctx, obj, "scrollTo", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                if (argc < 1) return JS_ThrowTypeError(ctx, "scrollTo requires 1 argument");
                int32_t line;
                if (JS_ToInt32(ctx, &line, argv[0]) != 0) return JS_EXCEPTION;
                logview->ScrollTo(line);
                return JS_UNDEFINED;
            }, "scrollTo", 1));

        // 添加 search 方法
        JS_SetPropertyStr(ctx, obj, "search", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                if (argc < 1) return JS_ThrowTypeError(ctx, "search requires 1 argument");
                const char* query = JS_ToCString(ctx, argv[0]);
                if (!query) return JS_EXCEPTION;
                bool use_regex = (argc >= 2) ? JS_ToBool(ctx, argv[1]) : false;
                int count = logview->Search(query, use_regex);
                JS_FreeCString(ctx, query);
                return JS_NewInt32(ctx, count);
            }, "search", 1));

        // 添加 clearSearch 方法
        JS_SetPropertyStr(ctx, obj, "clearSearch", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                logview->ClearSearch();
                return JS_UNDEFINED;
            }, "clearSearch", 0));

        // 添加 export 方法
        JS_SetPropertyStr(ctx, obj, "export", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                const char* format = "text";
                if (argc >= 1) {
                    format = JS_ToCString(ctx, argv[0]);
                    if (!format) return JS_EXCEPTION;
                }
                std::string content = logview->Export(format);
                if (argc >= 1) JS_FreeCString(ctx, format);
                return JS_NewString(ctx, content.c_str());
            }, "export", 0));

        // 添加 setLevelFilter 方法
        JS_SetPropertyStr(ctx, obj, "setLevelFilter", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                if (argc < 1 || !JS_IsArray(argv[0])) {
                    return JS_ThrowTypeError(ctx, "setLevelFilter requires an array argument");
                }
                std::vector<std::string> levels;
                JSValue length_val = JS_GetPropertyStr(ctx, argv[0], "length");
                int32_t length;
                JS_ToInt32(ctx, &length, length_val);
                JS_FreeValue(ctx, length_val);
                for (int32_t i = 0; i < length; i++) {
                    JSValue item = JS_GetPropertyUint32(ctx, argv[0], i);
                    const char* str = JS_ToCString(ctx, item);
                    if (str) {
                        levels.push_back(str);
                        JS_FreeCString(ctx, str);
                    }
                    JS_FreeValue(ctx, item);
                }
                logview->SetLevelFilter(levels);
                return JS_UNDEFINED;
            }, "setLevelFilter", 1));

        // 添加 clearFilter 方法
        JS_SetPropertyStr(ctx, obj, "clearFilter", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                logview->ClearFilter();
                return JS_UNDEFINED;
            }, "clearFilter", 0));

        // 添加 nextMatch 方法
        JS_SetPropertyStr(ctx, obj, "nextMatch", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                logview->NextMatch();
                return JS_UNDEFINED;
            }, "nextMatch", 0));

        // 添加 prevMatch 方法
        JS_SetPropertyStr(ctx, obj, "prevMatch", JS_NewCFunction(ctx,
            [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                auto elem = UnwrapElement(ctx, this_val);
                if (!elem) return JS_EXCEPTION;
                auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());
                if (!logview) return JS_EXCEPTION;
                logview->PrevMatch();
                return JS_UNDEFINED;
            }, "prevMatch", 0));
    }

    return obj;
}

std::shared_ptr<Element> UnwrapElement(JSContext* ctx, JSValue value) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(value, js_element_class_id));
    if (!data) {
        return nullptr;
    }
    return data->element;
}

JSClassID GetElementClassID() {
    return js_element_class_id;
}

} // namespace bindings
} // namespace lightui
