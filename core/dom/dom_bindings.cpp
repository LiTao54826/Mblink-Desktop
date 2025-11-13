/**
 * @file dom_bindings.cpp
 * @brief DOM JavaScript 绑定实现
 */

#include "dom_bindings.h"
#include "quickjs/quickjs-libc.h"
#include "quickjs/js_value_wrapper.h"
#include <cstring>

namespace lightui {

// ========== 静态成员初始化 ==========

JSClassID DOMBindings::element_class_id = 0;
JSClassID DOMBindings::text_class_id = 0;
JSClassID DOMBindings::document_class_id = 0;
JSClassID DOMBindings::event_class_id = 0;
bool DOMBindings::initialized = false;

// 对象缓存
std::unordered_map<Element*, std::pair<JSContext*, JSValue>> DOMBindings::element_cache_;
std::unordered_map<Text*, std::pair<JSContext*, JSValue>> DOMBindings::text_cache_;
std::unordered_map<Document*, std::pair<JSContext*, JSValue>> DOMBindings::document_cache_;

// ========== 辅助函数 ==========

// 从 opaque 指针获取 shared_ptr
template<typename T>
static std::shared_ptr<T>* GetOpaquePtr(void* opaque) {
    return static_cast<std::shared_ptr<T>*>(opaque);
}

// ========== Element 类绑定 ==========

// Element finalizer
static void js_element_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<Element>*>(JS_GetOpaque(val, DOMBindings::element_class_id));
    if (ptr) {
        // 从缓存中移除
        DOMBindings::RemoveFromElementCache(ptr->get());
        delete ptr;
    }
}

// Element.tagName getter
static JSValue js_element_get_tag_name(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }
    return JS_NewString(ctx, element->GetTagName().c_str());
}

// Element.id getter
static JSValue js_element_get_id(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }
    return JS_NewString(ctx, element->GetAttribute("id").c_str());
}

// Element.id setter
static JSValue js_element_set_id(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    const char* id = JS_ToCString(ctx, val);
    if (!id) {
        return JS_EXCEPTION;
    }

    element->SetAttribute("id", id);
    JS_FreeCString(ctx, id);

    return JS_UNDEFINED;
}

// Element.className getter
static JSValue js_element_get_class_name(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }
    return JS_NewString(ctx, element->GetClassName().c_str());
}

// Element.className setter
static JSValue js_element_set_class_name(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    const char* class_name = JS_ToCString(ctx, val);
    if (!class_name) {
        return JS_EXCEPTION;
    }

    element->SetClassName(class_name);
    JS_FreeCString(ctx, class_name);

    return JS_UNDEFINED;
}

// Element.textContent getter
static JSValue js_element_get_text_content(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }
    return JS_NewString(ctx, element->GetTextContent().c_str());
}

// Element.textContent setter
static JSValue js_element_set_text_content(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    const char* text = JS_ToCString(ctx, val);
    if (!text) {
        return JS_EXCEPTION;
    }

    element->SetTextContent(text);
    JS_FreeCString(ctx, text);

    return JS_UNDEFINED;
}

// Element.children getter
static JSValue js_element_get_children(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    // 创建数组
    JSValue children_array = JS_NewArray(ctx);

    // 获取所有子节点（只包含Element类型）
    const auto& child_nodes = element->GetChildNodes();
    uint32_t index = 0;

    for (const auto& child : child_nodes) {
        // 只包含Element类型的子节点
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto child_element = std::static_pointer_cast<Element>(child);
            JSValue child_obj = DOMBindings::WrapElement(ctx, child_element);
            JS_SetPropertyUint32(ctx, children_array, index++, child_obj);
        }
    }

    // 添加length属性
    JS_SetPropertyStr(ctx, children_array, "length", JS_NewUint32(ctx, index));

    return children_array;
}

// Element.getAttribute(name)
static JSValue js_element_get_attribute(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getAttribute requires 1 argument");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_EXCEPTION;
    }

    std::string value = element->GetAttribute(name);
    JS_FreeCString(ctx, name);

    return JS_NewString(ctx, value.c_str());
}

// Element.setAttribute(name, value)
static JSValue js_element_set_attribute(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
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

    element->SetAttribute(name, value);

    JS_FreeCString(ctx, name);
    JS_FreeCString(ctx, value);

    return JS_UNDEFINED;
}

// Element.appendChild(child)
static JSValue js_element_append_child(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "appendChild requires 1 argument");
    }

    // 尝试解包为 Element 或 Text
    auto child_element = DOMBindings::UnwrapElement(ctx, argv[0]);
    if (child_element) {
        element->AppendChild(child_element);
        return JS_DupValue(ctx, argv[0]);
    }

    auto child_text = DOMBindings::UnwrapText(ctx, argv[0]);
    if (child_text) {
        element->AppendChild(child_text);
        return JS_DupValue(ctx, argv[0]);
    }

    return JS_ThrowTypeError(ctx, "appendChild requires a Node argument");
}

// Element.addEventListener(type, listener)
static JSValue js_element_add_event_listener(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "addEventListener requires 2 arguments");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    if (!JS_IsFunction(ctx, argv[1])) {
        JS_FreeCString(ctx, type);
        return JS_ThrowTypeError(ctx, "addEventListener requires a function as second argument");
    }

    // 使用 JSValueWrapper 管理 listener 的生命周期
    // shared_ptr 确保在 lambda 被销毁时自动释放 JSValue
    auto listener_wrapper = std::make_shared<JSValueWrapper>(ctx, argv[1]);

    // 创建 C++ lambda 包装 JS 函数
    // Lambda 捕获 shared_ptr，当 Element 被销毁时，lambda 也会被销毁，
    // shared_ptr 引用计数归零，JSValueWrapper 析构函数自动调用 JS_FreeValue
    element->AddEventListener(type, [ctx, listener_wrapper](std::shared_ptr<Event> event) {
        JSValue event_obj = DOMBindings::WrapEvent(ctx, event);
        JSValue ret = JS_Call(ctx, listener_wrapper->Get(), JS_UNDEFINED, 1, &event_obj);
        JS_FreeValue(ctx, event_obj);
        if (JS_IsException(ret)) {
            js_std_dump_error(ctx);
        }
        JS_FreeValue(ctx, ret);
    });

    JS_FreeCString(ctx, type);

    return JS_UNDEFINED;
}

// Element 类定义
static const JSCFunctionListEntry js_element_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("tagName", js_element_get_tag_name, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("id", js_element_get_id, js_element_set_id, 0),
    JS_CGETSET_MAGIC_DEF("className", js_element_get_class_name, js_element_set_class_name, 0),
    JS_CGETSET_MAGIC_DEF("textContent", js_element_get_text_content, js_element_set_text_content, 0),
    JS_CGETSET_MAGIC_DEF("children", js_element_get_children, nullptr, 0),
    JS_CFUNC_DEF("getAttribute", 1, js_element_get_attribute),
    JS_CFUNC_DEF("setAttribute", 2, js_element_set_attribute),
    JS_CFUNC_DEF("appendChild", 1, js_element_append_child),
    JS_CFUNC_DEF("addEventListener", 2, js_element_add_event_listener),
};

void DOMBindings::InitElementClass(JSContext* ctx) {
    JSClassDef element_class = {
        /* class_name */ "Element",
        /* finalizer */ js_element_finalizer,
        /* gc_mark */ nullptr,
        /* call */ nullptr,
        /* exotic */ nullptr,
    };

    JS_NewClassID(JS_GetRuntime(ctx), &element_class_id);
    JS_NewClass(JS_GetRuntime(ctx), element_class_id, &element_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_element_proto_funcs,
                               sizeof(js_element_proto_funcs) / sizeof(js_element_proto_funcs[0]));
    JS_SetClassProto(ctx, element_class_id, proto);
}

// ========== Text 类绑定 ==========

// Text finalizer
static void js_text_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<Text>*>(JS_GetOpaque(val, DOMBindings::text_class_id));
    if (ptr) {
        // 从缓存中移除
        DOMBindings::RemoveFromTextCache(ptr->get());
        delete ptr;
    }
}

// Text.data getter
static JSValue js_text_get_data(JSContext* ctx, JSValueConst this_val, int magic) {
    auto text = DOMBindings::UnwrapText(ctx, this_val);
    if (!text) {
        return JS_EXCEPTION;
    }
    return JS_NewString(ctx, text->GetData().c_str());
}

// Text.data setter
static JSValue js_text_set_data(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto text = DOMBindings::UnwrapText(ctx, this_val);
    if (!text) {
        return JS_EXCEPTION;
    }

    const char* data = JS_ToCString(ctx, val);
    if (!data) {
        return JS_EXCEPTION;
    }

    text->SetData(data);
    JS_FreeCString(ctx, data);

    return JS_UNDEFINED;
}

// Text 类定义
static const JSCFunctionListEntry js_text_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("data", js_text_get_data, js_text_set_data, 0),
};

void DOMBindings::InitTextClass(JSContext* ctx) {
    JSClassDef text_class = {
        /* class_name */ "Text",
        /* finalizer */ js_text_finalizer,
        /* gc_mark */ nullptr,
        /* call */ nullptr,
        /* exotic */ nullptr,
    };

    JS_NewClassID(JS_GetRuntime(ctx), &text_class_id);
    JS_NewClass(JS_GetRuntime(ctx), text_class_id, &text_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_text_proto_funcs,
                               sizeof(js_text_proto_funcs) / sizeof(js_text_proto_funcs[0]));
    JS_SetClassProto(ctx, text_class_id, proto);
}

// ========== Document 类绑定 ==========

// Document finalizer
static void js_document_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<Document>*>(JS_GetOpaque(val, DOMBindings::document_class_id));
    if (ptr) {
        // 从缓存中移除
        DOMBindings::RemoveFromDocumentCache(ptr->get());
        delete ptr;
    }
}

// Document.createElement(tagName)
static JSValue js_document_create_element(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "createElement requires 1 argument");
    }

    const char* tag_name = JS_ToCString(ctx, argv[0]);
    if (!tag_name) {
        return JS_EXCEPTION;
    }

    auto element = document->CreateElement(tag_name);
    JS_FreeCString(ctx, tag_name);

    return DOMBindings::WrapElement(ctx, element);
}

// Document.createTextNode(data)
static JSValue js_document_create_text_node(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "createTextNode requires 1 argument");
    }

    const char* data = JS_ToCString(ctx, argv[0]);
    if (!data) {
        return JS_EXCEPTION;
    }

    auto text = document->CreateTextNode(data);
    JS_FreeCString(ctx, data);

    return DOMBindings::WrapText(ctx, text);
}

// Document.getElementById(id)
static JSValue js_document_get_element_by_id(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getElementById requires 1 argument");
    }

    const char* id = JS_ToCString(ctx, argv[0]);
    if (!id) {
        return JS_EXCEPTION;
    }

    auto element = document->GetElementById(id);
    JS_FreeCString(ctx, id);

    if (!element) {
        return JS_NULL;
    }

    return DOMBindings::WrapElement(ctx, element);
}

// Document.body getter
static JSValue js_document_get_body(JSContext* ctx, JSValueConst this_val, int magic) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    auto body = document->GetBody();
    if (!body) {
        return JS_NULL;
    }

    return DOMBindings::WrapElement(ctx, body);
}

// Document 类定义
static const JSCFunctionListEntry js_document_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("body", js_document_get_body, nullptr, 0),
    JS_CFUNC_DEF("createElement", 1, js_document_create_element),
    JS_CFUNC_DEF("createTextNode", 1, js_document_create_text_node),
    JS_CFUNC_DEF("getElementById", 1, js_document_get_element_by_id),
};

void DOMBindings::InitDocumentClass(JSContext* ctx) {
    JSClassDef document_class = {
        /* class_name */ "Document",
        /* finalizer */ js_document_finalizer,
        /* gc_mark */ nullptr,
        /* call */ nullptr,
        /* exotic */ nullptr,
    };

    JS_NewClassID(JS_GetRuntime(ctx), &document_class_id);
    JS_NewClass(JS_GetRuntime(ctx), document_class_id, &document_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_document_proto_funcs,
                               sizeof(js_document_proto_funcs) / sizeof(js_document_proto_funcs[0]));
    JS_SetClassProto(ctx, document_class_id, proto);
}

// ========== Event 类绑定 ==========

// Event finalizer
static void js_event_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<Event>*>(JS_GetOpaque(val, DOMBindings::event_class_id));
    if (ptr) {
        delete ptr;
    }
}

// Event.type getter
static JSValue js_event_get_type(JSContext* ctx, JSValueConst this_val, int magic) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }
    return JS_NewString(ctx, event->GetType().c_str());
}

// Event.stopPropagation()
static JSValue js_event_stop_propagation(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }

    event->StopPropagation();
    return JS_UNDEFINED;
}

// Event.preventDefault()
static JSValue js_event_prevent_default(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }

    event->PreventDefault();
    return JS_UNDEFINED;
}

// Event 类定义
static const JSCFunctionListEntry js_event_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("type", js_event_get_type, nullptr, 0),
    JS_CFUNC_DEF("stopPropagation", 0, js_event_stop_propagation),
    JS_CFUNC_DEF("preventDefault", 0, js_event_prevent_default),
};

void DOMBindings::InitEventClass(JSContext* ctx) {
    JSClassDef event_class = {
        /* class_name */ "Event",
        /* finalizer */ js_event_finalizer,
        /* gc_mark */ nullptr,
        /* call */ nullptr,
        /* exotic */ nullptr,
    };

    JS_NewClassID(JS_GetRuntime(ctx), &event_class_id);
    JS_NewClass(JS_GetRuntime(ctx), event_class_id, &event_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_event_proto_funcs,
                               sizeof(js_event_proto_funcs) / sizeof(js_event_proto_funcs[0]));
    JS_SetClassProto(ctx, event_class_id, proto);
}

// ========== 初始化和清理 ==========

void DOMBindings::Init(JSContext* ctx) {
    if (initialized) {
        return;
    }

    InitElementClass(ctx);
    InitTextClass(ctx);
    InitDocumentClass(ctx);
    InitEventClass(ctx);

    initialized = true;
}

void DOMBindings::Cleanup(JSContext* ctx) {
    // 清理所有缓存，释放JSValue引用
    for (auto& pair : element_cache_) {
        JS_FreeValue(pair.second.first, pair.second.second);
    }
    element_cache_.clear();

    for (auto& pair : text_cache_) {
        JS_FreeValue(pair.second.first, pair.second.second);
    }
    text_cache_.clear();

    for (auto& pair : document_cache_) {
        JS_FreeValue(pair.second.first, pair.second.second);
    }
    document_cache_.clear();

    // QuickJS 会自动清理类
    initialized = false;
}

// ========== 包装函数 ==========

JSValue DOMBindings::WrapElement(JSContext* ctx, std::shared_ptr<Element> element) {
    if (!element) {
        return JS_NULL;
    }

    // 检查缓存，避免重复包装
    Element* raw_ptr = element.get();
    auto it = element_cache_.find(raw_ptr);
    if (it != element_cache_.end()) {
        // 缓存命中，返回已有的JSValue（需要DupValue增加引用计数）
        return JS_DupValue(ctx, it->second.second);
    }

    // 缓存未命中，创建新的JSValue
    JSValue obj = JS_NewObjectClass(ctx, element_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto ptr = new std::shared_ptr<Element>(element);
    JS_SetOpaque(obj, ptr);

    // 添加到缓存（DupValue让缓存持有一个引用）
    element_cache_[raw_ptr] = std::make_pair(ctx, JS_DupValue(ctx, obj));

    return obj;
}

JSValue DOMBindings::WrapText(JSContext* ctx, std::shared_ptr<Text> text) {
    if (!text) {
        return JS_NULL;
    }

    // 检查缓存，避免重复包装
    Text* raw_ptr = text.get();
    auto it = text_cache_.find(raw_ptr);
    if (it != text_cache_.end()) {
        // 缓存命中，返回已有的JSValue（需要DupValue增加引用计数）
        return JS_DupValue(ctx, it->second.second);
    }

    // 缓存未命中，创建新的JSValue
    JSValue obj = JS_NewObjectClass(ctx, text_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto ptr = new std::shared_ptr<Text>(text);
    JS_SetOpaque(obj, ptr);

    // 添加到缓存（DupValue让缓存持有一个引用）
    text_cache_[raw_ptr] = std::make_pair(ctx, JS_DupValue(ctx, obj));

    return obj;
}

JSValue DOMBindings::WrapDocument(JSContext* ctx, std::shared_ptr<Document> document) {
    if (!document) {
        return JS_NULL;
    }

    // 检查缓存，避免重复包装
    Document* raw_ptr = document.get();
    auto it = document_cache_.find(raw_ptr);
    if (it != document_cache_.end()) {
        // 缓存命中，返回已有的JSValue（需要DupValue增加引用计数）
        return JS_DupValue(ctx, it->second.second);
    }

    // 缓存未命中，创建新的JSValue
    JSValue obj = JS_NewObjectClass(ctx, document_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto ptr = new std::shared_ptr<Document>(document);
    JS_SetOpaque(obj, ptr);

    // 添加到缓存（DupValue让缓存持有一个引用）
    document_cache_[raw_ptr] = std::make_pair(ctx, JS_DupValue(ctx, obj));

    return obj;
}

JSValue DOMBindings::WrapEvent(JSContext* ctx, std::shared_ptr<Event> event) {
    if (!event) {
        return JS_NULL;
    }

    JSValue obj = JS_NewObjectClass(ctx, event_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto ptr = new std::shared_ptr<Event>(event);
    JS_SetOpaque(obj, ptr);

    return obj;
}

JSValue DOMBindings::WrapNode(JSContext* ctx, std::shared_ptr<Node> node) {
    if (!node) {
        return JS_NULL;
    }

    // 根据节点类型选择包装方式
    if (auto element = std::dynamic_pointer_cast<Element>(node)) {
        return WrapElement(ctx, element);
    } else if (auto text = std::dynamic_pointer_cast<Text>(node)) {
        return WrapText(ctx, text);
    } else if (auto document = std::dynamic_pointer_cast<Document>(node)) {
        return WrapDocument(ctx, document);
    }

    return JS_NULL;
}

// ========== 解包函数 ==========

std::shared_ptr<Element> DOMBindings::UnwrapElement(JSContext* ctx, JSValue obj) {
    auto ptr = static_cast<std::shared_ptr<Element>*>(JS_GetOpaque2(ctx, obj, element_class_id));
    if (!ptr) {
        return nullptr;
    }
    return *ptr;
}

std::shared_ptr<Text> DOMBindings::UnwrapText(JSContext* ctx, JSValue obj) {
    auto ptr = static_cast<std::shared_ptr<Text>*>(JS_GetOpaque2(ctx, obj, text_class_id));
    if (!ptr) {
        return nullptr;
    }
    return *ptr;
}

std::shared_ptr<Document> DOMBindings::UnwrapDocument(JSContext* ctx, JSValue obj) {
    auto ptr = static_cast<std::shared_ptr<Document>*>(JS_GetOpaque2(ctx, obj, document_class_id));
    if (!ptr) {
        return nullptr;
    }
    return *ptr;
}

std::shared_ptr<Event> DOMBindings::UnwrapEvent(JSContext* ctx, JSValue obj) {
    auto ptr = static_cast<std::shared_ptr<Event>*>(JS_GetOpaque2(ctx, obj, event_class_id));
    if (!ptr) {
        return nullptr;
    }
    return *ptr;
}

// ========== 缓存管理函数 ==========

void DOMBindings::RemoveFromElementCache(Element* ptr) {
    auto it = element_cache_.find(ptr);
    if (it != element_cache_.end()) {
        // 释放缓存持有的引用
        JS_FreeValue(it->second.first, it->second.second);
        element_cache_.erase(it);
    }
}

void DOMBindings::RemoveFromTextCache(Text* ptr) {
    auto it = text_cache_.find(ptr);
    if (it != text_cache_.end()) {
        // 释放缓存持有的引用
        JS_FreeValue(it->second.first, it->second.second);
        text_cache_.erase(it);
    }
}

void DOMBindings::RemoveFromDocumentCache(Document* ptr) {
    auto it = document_cache_.find(ptr);
    if (it != document_cache_.end()) {
        // 释放缓存持有的引用
        JS_FreeValue(it->second.first, it->second.second);
        document_cache_.erase(it);
    }
}

} // namespace lightui
