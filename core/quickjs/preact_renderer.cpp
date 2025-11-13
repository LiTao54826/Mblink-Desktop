/**
 * @file preact_renderer.cpp
 * @brief Preact渲染器实现
 */

#include "preact_renderer.h"
#include "core/dom/dom_bindings.h"
#include "core/dom/text.h"
#include "core/dom/element.h"
#include "core/dom/event.h"
#include <iostream>

namespace lightui {

PreactRenderer::PreactRenderer(QuickJSRuntime* runtime, std::shared_ptr<Document> document)
    : runtime_(runtime)
    , ctx_(runtime->GetContext())
    , document_(document)
    , current_component_(JS_UNDEFINED)
    , current_hook_index_(0) {
}

PreactRenderer::~PreactRenderer() {
    // 清理事件处理器
    // JSValueWrapper 的析构函数会自动调用 JS_FreeValue
    event_handlers_.clear();

    if (!JS_IsUndefined(current_component_)) {
        JS_FreeValue(ctx_, current_component_);
    }
}

bool PreactRenderer::Render(JSValue vnode_val, JSValue container_val) {
    // 使用DOM绑定解包容器元素
    auto container = DOMBindings::UnwrapElement(ctx_, container_val);

    if (!container) {
        return false;
    }

    // 清空容器
    while (container->GetFirstChild()) {
        container->RemoveChild(container->GetFirstChild());
    }

    // 从VNode创建DOM
    auto dom_node = CreateDOMFromVNode(vnode_val);
    if (!dom_node) {
        return false;
    }

    // 添加到容器
    container->AppendChild(dom_node);

    return true;
}

std::shared_ptr<Node> PreactRenderer::CreateDOMFromVNode(JSValue vnode_val) {
    // 处理null/undefined
    if (JS_IsNull(vnode_val) || JS_IsUndefined(vnode_val)) {
        return nullptr;
    }

    // 处理字符串/数字（文本节点）
    if (JS_IsString(vnode_val) || JS_IsNumber(vnode_val)) {
        std::string text = GetString(vnode_val);
        return CreateTextNode(text);
    }

    // 处理布尔值（不渲染）
    if (JS_IsBool(vnode_val)) {
        return nullptr;
    }

    // 处理VNode对象
    if (!IsVNode(vnode_val)) {
        return nullptr;
    }

    // 获取type
    JSValue type_val = GetProperty(vnode_val, "type");

    // 处理函数组件
    if (IsComponentFunction(type_val)) {
        JSValue props_val = GetProperty(vnode_val, "props");
        JSValue result_vnode = RenderComponent(type_val, props_val);
        JS_FreeValue(ctx_, props_val);
        JS_FreeValue(ctx_, type_val);

        auto dom_node = CreateDOMFromVNode(result_vnode);
        JS_FreeValue(ctx_, result_vnode);
        return dom_node;
    }

    // 处理普通元素
    std::string tag_name = GetString(type_val);
    JS_FreeValue(ctx_, type_val);

    auto element = CreateElementNode(tag_name);
    if (!element) {
        return nullptr;
    }

    // 应用属性
    JSValue props_val = GetProperty(vnode_val, "props");
    ApplyProps(element, props_val);
    JS_FreeValue(ctx_, props_val);

    // 应用子节点
    JSValue children_val = GetProperty(vnode_val, "children");
    ApplyChildren(element, children_val);
    JS_FreeValue(ctx_, children_val);

    return element;
}

void PreactRenderer::ApplyProps(std::shared_ptr<Element> element, JSValue props_val) {
    if (JS_IsNull(props_val) || JS_IsUndefined(props_val)) {
        return;
    }
    
    // 遍历属性
    JSPropertyEnum* props;
    uint32_t prop_count;
    if (JS_GetOwnPropertyNames(ctx_, &props, &prop_count, props_val, 
                               JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) {
        return;
    }
    
    for (uint32_t i = 0; i < prop_count; i++) {
        JSValue key_val = JS_AtomToValue(ctx_, props[i].atom);
        std::string key = GetString(key_val);
        JS_FreeValue(ctx_, key_val);
        
        // 跳过特殊属性
        if (key == "key" || key == "ref" || key == "children") {
            continue;
        }
        
        JSValue value_val = JS_GetProperty(ctx_, props_val, props[i].atom);
        
        // 处理事件监听器
        if (key.size() > 2 && key[0] == 'o' && key[1] == 'n') {
            std::string event_name = key.substr(2);
            // 转换为小写
            for (char& c : event_name) {
                c = std::tolower(c);
            }
            AddEventListener(element, event_name, value_val);
        }
        // 处理className
        else if (key == "className") {
            element->SetAttribute("class", GetString(value_val));
        }
        // 处理style对象
        else if (key == "style" && JS_IsObject(value_val)) {
            // TODO: 处理style对象
            // 暂时跳过
        }
        // 处理布尔属性
        else if (JS_IsBool(value_val)) {
            if (GetBool(value_val)) {
                element->SetAttribute(key, "");
            }
        }
        // 处理普通属性
        else if (!JS_IsNull(value_val) && !JS_IsUndefined(value_val)) {
            element->SetAttribute(key, GetString(value_val));
        }
        
        JS_FreeValue(ctx_, value_val);
    }
    
    js_free(ctx_, props);
}

void PreactRenderer::ApplyChildren(std::shared_ptr<Element> element, JSValue children_val) {
    if (JS_IsNull(children_val) || JS_IsUndefined(children_val)) {
        return;
    }

    // 处理数组
    if (IsArray(children_val)) {
        int length = GetArrayLength(children_val);
        for (int i = 0; i < length; i++) {
            JSValue child_val = GetArrayElement(children_val, i);
            auto child_node = CreateDOMFromVNode(child_val);
            JS_FreeValue(ctx_, child_val);

            if (child_node) {
                element->AppendChild(child_node);
            }
        }
    }
    // 处理单个子节点
    else {
        auto child_node = CreateDOMFromVNode(children_val);
        if (child_node) {
            element->AppendChild(child_node);
        }
    }
}

JSValue PreactRenderer::RenderComponent(JSValue component_func, JSValue props_val) {
    // 创建组件对象
    JSValue component_obj = JS_NewObject(ctx_);
    SetProperty(component_obj, "props", props_val);
    SetProperty(component_obj, "__hooks", JS_NewArray(ctx_));
    
    // 设置当前组件
    SetCurrentComponent(component_obj);
    
    // 调用组件函数
    JSValue result = JS_Call(ctx_, component_func, JS_UNDEFINED, 1, &props_val);
    
    // 清除当前组件
    SetCurrentComponent(JS_UNDEFINED);
    
    JS_FreeValue(ctx_, component_obj);
    
    return result;
}

void PreactRenderer::SetCurrentComponent(JSValue component) {
    if (!JS_IsUndefined(current_component_)) {
        JS_FreeValue(ctx_, current_component_);
    }
    
    current_component_ = JS_DupValue(ctx_, component);
    current_hook_index_ = 0;
}

void PreactRenderer::AddEventListener(std::shared_ptr<Element> element,
                                      const std::string& event_name,
                                      JSValue handler) {
    if (!JS_IsFunction(ctx_, handler)) {
        return;
    }

    // 使用 JSValueWrapper 管理 handler 的生命周期
    // shared_ptr 确保在 lambda 被销毁时自动释放 JSValue
    auto handler_wrapper = std::make_shared<JSValueWrapper>(ctx_, handler);

    // 保存handler引用（防止GC）
    event_handlers_[element].push_back(handler_wrapper);

    // 创建C++事件监听器
    // Lambda 捕获 shared_ptr，当 Element 被销毁或 event_handlers_ 被清空时，
    // shared_ptr 引用计数归零，JSValueWrapper 析构函数自动调用 JS_FreeValue
    auto listener = [this, handler_wrapper](std::shared_ptr<Event> event) {
        // 创建简单的事件对象
        JSValue event_obj = JS_NewObject(ctx_);
        const char* type_str = event->GetType().c_str();
        JSValue type_val = JS_NewStringLen(ctx_, type_str, strlen(type_str));
        JS_SetPropertyStr(ctx_, event_obj, "type", type_val);

        // 调用JS handler
        JSValue result = JS_Call(ctx_, handler_wrapper->Get(), JS_UNDEFINED, 1, &event_obj);
        JS_FreeValue(ctx_, event_obj);

        if (JS_IsException(result)) {
            JSValue exception = JS_GetException(ctx_);
            const char* str = JS_ToCString(ctx_, exception);
            if (str) {
                std::cerr << "JavaScript Error: " << str << std::endl;
                JS_FreeCString(ctx_, str);
            }
            JS_FreeValue(ctx_, exception);
        }
        JS_FreeValue(ctx_, result);
    };

    element->AddEventListener(event_name, listener);
}

// 辅助方法实现

std::string PreactRenderer::GetString(JSValue val) {
    const char* str = JS_ToCString(ctx_, val);
    if (!str) return "";
    std::string result(str);
    JS_FreeCString(ctx_, str);
    return result;
}

double PreactRenderer::GetNumber(JSValue val) {
    double num;
    JS_ToFloat64(ctx_, &num, val);
    return num;
}

bool PreactRenderer::GetBool(JSValue val) {
    return JS_ToBool(ctx_, val) == 1;
}

bool PreactRenderer::IsArray(JSValue val) {
    JSValue is_array_val = JS_GetPropertyStr(ctx_, val, "length");
    bool result = !JS_IsUndefined(is_array_val);
    JS_FreeValue(ctx_, is_array_val);
    return result && JS_IsObject(val);
}

int PreactRenderer::GetArrayLength(JSValue val) {
    JSValue length_val = JS_GetPropertyStr(ctx_, val, "length");
    int length = 0;
    JS_ToInt32(ctx_, &length, length_val);
    JS_FreeValue(ctx_, length_val);
    return length;
}

JSValue PreactRenderer::GetArrayElement(JSValue val, int index) {
    return JS_GetPropertyUint32(ctx_, val, index);
}

JSValue PreactRenderer::GetProperty(JSValue val, const char* name) {
    return JS_GetPropertyStr(ctx_, val, name);
}

void PreactRenderer::SetProperty(JSValue val, const char* name, JSValue prop_val) {
    JS_SetPropertyStr(ctx_, val, name, prop_val);
}

bool PreactRenderer::IsVNode(JSValue val) {
    if (!JS_IsObject(val)) {
        return false;
    }
    JSValue type_val = GetProperty(val, "type");
    bool has_type = !JS_IsUndefined(type_val) && !JS_IsNull(type_val);
    JS_FreeValue(ctx_, type_val);
    return has_type;
}

bool PreactRenderer::IsComponentFunction(JSValue val) {
    return JS_IsFunction(ctx_, val);
}

std::shared_ptr<Node> PreactRenderer::CreateTextNode(const std::string& text) {
    return document_->CreateTextNode(text);
}

std::shared_ptr<Element> PreactRenderer::CreateElementNode(const std::string& tag_name) {
    return document_->CreateElement(tag_name);
}

} // namespace lightui

