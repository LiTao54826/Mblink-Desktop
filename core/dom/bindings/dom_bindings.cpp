/**
 * @file dom_bindings.cpp
 * @brief DOM JavaScript 绑定实现
 *
 * @note 大文件说明 (2996 行)
 * 本文件包含 DOM API 的 JavaScript 绑定实现。
 * 文件较大的原因：
 * 1. 实现完整的 DOM API（Element、Node、Document 等）
 * 2. 包含大量属性 getter/setter 绑定
 * 3. 包含事件处理绑定
 * 4. 包含样式操作绑定
 * 5. 需要处理 C++ 和 JavaScript 之间的类型转换
 *
 * 计划重构：
 * - 按 DOM 接口拆分到 quickjs/bindings/ 子目录
 * - 使用代码生成减少重复代码
 */

#include "dom_bindings.h"
#include "canvas_bindings.h"
#include "terminal_bindings.h"
#include "core/dom/elements/html_canvas_element.h"
#include "core/dom/elements/html_image_element.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/elements/terminal/html_terminal_element.h"
#include "core/dom/elements/logview/html_logview_element.h"
#include "core/dom/selection/range.h"
#include "quickjs/dom_binding_map.h"
#include "quickjs/quickjs-libc.h"
#include "quickjs/js_value_wrapper.h"
#include "quickjs/bindings/js_element.h"
#include "quickjs/bindings/js_range.h"
#include "core/event/loop/event_loop.h"
#include "core/editing/selection_manager.h"
#include "core/editing/contenteditable_handler.h"
#include "core/editing/clipboard_manager.h"
#include <cstring>
#include <algorithm>
#include <cctype>

namespace mbink {

// 前向声明
void InitImageConstructor(JSContext* ctx);

// ========== 静态成员初始化 ==========

JSClassID DOMBindings::element_class_id = 0;
JSClassID DOMBindings::text_class_id = 0;
JSClassID DOMBindings::document_class_id = 0;
JSClassID DOMBindings::event_class_id = 0;
JSClassID DOMBindings::dom_token_list_class_id = 0;
JSClassID DOMBindings::css_style_declaration_class_id = 0;
JSClassID DOMBindings::dom_string_map_class_id = 0;
bool DOMBindings::initialized = false;

// 对象缓存
std::unordered_map<Element*, std::pair<JSContext*, JSValue>> DOMBindings::element_cache_;
std::unordered_map<Text*, std::pair<JSContext*, JSValue>> DOMBindings::text_cache_;
std::unordered_map<Document*, std::pair<JSContext*, JSValue>> DOMBindings::document_cache_;


// 全局 TaskScheduler / EventLoop 实例（供定时器与 RAF 绑定使用）
namespace {
std::shared_ptr<TaskScheduler> g_task_scheduler = nullptr;
EventLoop* g_event_loop = nullptr;

static JSMemoryUsage GetRuntimeUsage(JSContext* ctx) {
    JSMemoryUsage usage{};
    JSRuntime* rt = ctx ? JS_GetRuntime(ctx) : nullptr;
    if (rt) {
        JS_ComputeMemoryUsage(rt, &usage);
    }
    return usage;
}
}

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
// Note: According to HTML standard, tagName returns uppercase for HTML elements
static JSValue js_element_get_tag_name(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }
    std::string tag_name = element->GetTagName();
    // Convert to uppercase for HTML standard compliance
    std::transform(tag_name.begin(), tag_name.end(), tag_name.begin(), ::toupper);
    return JS_NewString(ctx, tag_name.c_str());
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

// HTMLImageElement.src getter
static JSValue js_element_get_img_src(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto img = std::dynamic_pointer_cast<HTMLImageElement>(element);
    if (!img) {
        return JS_UNDEFINED;
    }

    return JS_NewString(ctx, img->GetSrc().c_str());
}

// HTMLImageElement.src setter
static JSValue js_element_set_img_src(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto img = std::dynamic_pointer_cast<HTMLImageElement>(element);
    if (!img) {
        return JS_UNDEFINED;
    }

    const char* src = JS_ToCString(ctx, val);
    if (!src) {
        return JS_EXCEPTION;
    }

    img->SetSrc(src);
    JS_FreeCString(ctx, src);
    return JS_UNDEFINED;
}

// HTMLImageElement.alt getter
static JSValue js_element_get_img_alt(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto img = std::dynamic_pointer_cast<HTMLImageElement>(element);
    if (!img) {
        return JS_UNDEFINED;
    }

    return JS_NewString(ctx, img->GetAlt().c_str());
}

// HTMLImageElement.alt setter
static JSValue js_element_set_img_alt(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto img = std::dynamic_pointer_cast<HTMLImageElement>(element);
    if (!img) {
        return JS_UNDEFINED;
    }

    const char* alt = JS_ToCString(ctx, val);
    if (!alt) {
        return JS_EXCEPTION;
    }

    img->SetAlt(alt);
    JS_FreeCString(ctx, alt);
    return JS_UNDEFINED;
}

// HTMLImageElement.naturalWidth getter
static JSValue js_element_get_img_natural_width(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto img = std::dynamic_pointer_cast<HTMLImageElement>(element);
    if (!img) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, static_cast<int>(img->GetNaturalWidth()));
}

// HTMLImageElement.naturalHeight getter
static JSValue js_element_get_img_natural_height(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto img = std::dynamic_pointer_cast<HTMLImageElement>(element);
    if (!img) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, static_cast<int>(img->GetNaturalHeight()));
}

// HTMLImageElement.complete getter
static JSValue js_element_get_img_complete(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto img = std::dynamic_pointer_cast<HTMLImageElement>(element);
    if (!img) {
        return JS_NewBool(ctx, false);
    }

    return JS_NewBool(ctx, img->GetComplete());
}

// HTMLImageElement.crossOrigin getter
static JSValue js_element_get_img_cross_origin(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto img = std::dynamic_pointer_cast<HTMLImageElement>(element);
    if (!img) {
        return JS_UNDEFINED;
    }

    return JS_NewString(ctx, img->GetCrossOrigin().c_str());
}

// HTMLImageElement.crossOrigin setter
static JSValue js_element_set_img_cross_origin(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto img = std::dynamic_pointer_cast<HTMLImageElement>(element);
    if (!img) {
        return JS_UNDEFINED;
    }

    const char* cross_origin = JS_ToCString(ctx, val);
    if (!cross_origin) {
        return JS_EXCEPTION;
    }

    img->SetCrossOrigin(cross_origin);
    JS_FreeCString(ctx, cross_origin);
    return JS_UNDEFINED;
}

// Element.classList getter (阶段3)
static JSValue js_element_get_class_list(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    // 获取classList对象
    auto class_list = element->GetClassList();
    if (!class_list) {
        return JS_NULL;
    }

    // 包装为JS对象
    JSValue obj = JS_NewObjectClass(ctx, DOMBindings::dom_token_list_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto ptr = new std::shared_ptr<DOMTokenList>(class_list);
    JS_SetOpaque(obj, ptr);

    return obj;
}

// Element.style getter (阶段3)
static JSValue js_element_get_style(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    // 获取style对象
    auto style = element->GetStyleDeclaration();
    if (!style) {
        return JS_NULL;
    }

    // 包装为JS对象
    JSValue obj = JS_NewObjectClass(ctx, DOMBindings::css_style_declaration_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto ptr = new std::shared_ptr<CSSStyleDeclaration>(style);
    JS_SetOpaque(obj, ptr);

    return obj;
}

// Element.dataset getter (阶段3)
static JSValue js_element_get_dataset(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    // 获取dataset对象
    auto dataset = element->GetDataset();
    if (!dataset) {
        return JS_NULL;
    }

    // 包装为JS对象
    JSValue obj = JS_NewObjectClass(ctx, DOMBindings::dom_string_map_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto ptr = new std::shared_ptr<DOMStringMap>(dataset);
    JS_SetOpaque(obj, ptr);

    return obj;
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

// Element.parentNode getter
static JSValue js_element_get_parent_node(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto parent = element->GetParentNode();
    if (!parent) {
        return JS_NULL;
    }

    // 尝试转换为 Element
    auto parent_element = std::dynamic_pointer_cast<Element>(parent);
    if (parent_element) {
        return DOMBindings::WrapElement(ctx, parent_element);
    }

    // 如果不是 Element，返回 null
    return JS_NULL;
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
            // JS_SetPropertyUint32会steal引用（接管所有权），所以不需要FreeValue
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

// Element.removeChild(child)
static JSValue js_element_remove_child(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "removeChild requires 1 argument");
    }

    // 尝试解包为 Element 或 Text
    auto child_element = DOMBindings::UnwrapElement(ctx, argv[0]);
    if (child_element) {
        element->RemoveChild(child_element);
        return JS_DupValue(ctx, argv[0]);
    }

    auto child_text = DOMBindings::UnwrapText(ctx, argv[0]);
    if (child_text) {
        element->RemoveChild(child_text);
        return JS_DupValue(ctx, argv[0]);
    }

    return JS_ThrowTypeError(ctx, "removeChild requires a Node argument");
}

// Element.replaceChild(newChild, oldChild)
static JSValue js_element_replace_child(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "replaceChild requires 2 arguments");
    }

    // 解包 newChild
    std::shared_ptr<Node> new_child;
    auto new_element = DOMBindings::UnwrapElement(ctx, argv[0]);
    if (new_element) {
        new_child = new_element;
    } else {
        auto new_text = DOMBindings::UnwrapText(ctx, argv[0]);
        if (new_text) {
            new_child = new_text;
        }
    }

    if (!new_child) {
        return JS_ThrowTypeError(ctx, "replaceChild requires a Node as first argument");
    }

    // 解包 oldChild
    std::shared_ptr<Node> old_child;
    auto old_element = DOMBindings::UnwrapElement(ctx, argv[1]);
    if (old_element) {
        old_child = old_element;
    } else {
        auto old_text = DOMBindings::UnwrapText(ctx, argv[1]);
        if (old_text) {
            old_child = old_text;
        }
    }

    if (!old_child) {
        return JS_ThrowTypeError(ctx, "replaceChild requires a Node as second argument");
    }

    // 添加安全检查：确保old_child确实是element的子节点
    bool found = false;
    for (const auto& child : element->GetChildNodes()) {
        if (child == old_child) {
            found = true;
            break;
        }
    }

    if (!found) {
        return JS_ThrowTypeError(ctx, "oldChild is not a child of this element");
    }

    try {
        element->ReplaceChild(new_child, old_child);
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "replaceChild failed: %s", e.what());
    }

    return JS_DupValue(ctx, argv[1]);
}

// Element.insertBefore(newChild, refChild)
static JSValue js_element_insert_before(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "insertBefore requires 2 arguments");
    }

    // 解包 newChild
    std::shared_ptr<Node> new_child;
    auto new_element = DOMBindings::UnwrapElement(ctx, argv[0]);
    if (new_element) {
        new_child = new_element;
    } else {
        auto new_text = DOMBindings::UnwrapText(ctx, argv[0]);
        if (new_text) {
            new_child = new_text;
        }
    }

    if (!new_child) {
        return JS_ThrowTypeError(ctx, "insertBefore requires a Node as first argument");
    }

    // 解包 refChild (可以为 null)
    std::shared_ptr<Node> ref_child;
    if (!JS_IsNull(argv[1]) && !JS_IsUndefined(argv[1])) {
        auto ref_element = DOMBindings::UnwrapElement(ctx, argv[1]);
        if (ref_element) {
            ref_child = ref_element;
        } else {
            auto ref_text = DOMBindings::UnwrapText(ctx, argv[1]);
            if (ref_text) {
                ref_child = ref_text;
            }
        }
    }

    element->InsertBefore(new_child, ref_child);
    return JS_DupValue(ctx, argv[0]);
}

// Element.addEventListener(type, listener)
// 返回listener ID，可用于removeEventListener
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

    // 解析第3和第4个参数 (useCapture, once)
    bool use_capture = false;
    bool once = false;

    if (argc >= 3) {
        use_capture = JS_ToBool(ctx, argv[2]);
    }
    if (argc >= 4) {
        once = JS_ToBool(ctx, argv[3]);
    }

    // 使用 JSValueWrapper 管理 listener 的生命周期
    // shared_ptr 确保在 lambda 被销毁时自动释放 JSValue
    auto listener_wrapper = std::make_shared<JSValueWrapper>(ctx, argv[1]);
    std::string event_type_str(type);  // 保存事件类型用于日志

    // 创建 C++ lambda 包装 JS 函数
    // Lambda 捕获 shared_ptr，当 Element 被销毁时，lambda 也会被销毁，
    // shared_ptr 引用计数归零，JSValueWrapper 析构函数自动调用 JS_FreeValue
    uint64_t listener_id = element->AddEventListener(type, [ctx, listener_wrapper, event_type_str](std::shared_ptr<Event> event) {
        JSValue event_obj = DOMBindings::WrapEvent(ctx, event);
        JSValue ret = JS_Call(ctx, listener_wrapper->Get(), JS_UNDEFINED, 1, &event_obj);
        JS_FreeValue(ctx, event_obj);
        if (JS_IsException(ret)) {
            js_std_dump_error(ctx);
        }
        JS_FreeValue(ctx, ret);
    }, use_capture, once);

    JS_FreeCString(ctx, type);

    // 返回listener ID
    return JS_NewBigUint64(ctx, listener_id);
}

// Element.dispatchEvent(event) (阶段4)
static JSValue js_element_dispatch_event(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "dispatchEvent requires 1 argument");
    }

    auto event = DOMBindings::UnwrapEvent(ctx, argv[0]);
    if (!event) {
        return JS_ThrowTypeError(ctx, "dispatchEvent requires an Event object");
    }

    bool result = element->DispatchEvent(event);
    return JS_NewBool(ctx, result);
}

// Element.removeEventListener(type, listenerId)
static JSValue js_element_remove_event_listener(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "removeEventListener requires 2 arguments");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    // 获取listener ID (支持普通数字和BigInt)
    uint64_t listener_id;
    if (JS_IsBigInt(argv[1])) {
        if (JS_ToBigUint64(ctx, &listener_id, argv[1]) != 0) {
            JS_FreeCString(ctx, type);
            return JS_ThrowTypeError(ctx, "Invalid listener ID");
        }
    } else {
        int64_t id;
        if (JS_ToInt64(ctx, &id, argv[1]) != 0) {
            JS_FreeCString(ctx, type);
            return JS_ThrowTypeError(ctx, "removeEventListener requires a listener ID as second argument");
        }
        listener_id = static_cast<uint64_t>(id);
    }

    // 移除监听器
    bool removed = element->RemoveEventListener(type, listener_id);

    JS_FreeCString(ctx, type);

    // 返回是否成功移除
    return JS_NewBool(ctx, removed);
}

// ========== 阶段1: 核心 Node API 绑定 ==========

// Node.childNodes getter
static JSValue js_element_get_child_nodes(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    // 创建数组
    JSValue nodes_array = JS_NewArray(ctx);

    // 获取所有子节点（包含Element和Text）
    const auto& child_nodes = element->GetChildNodes();
    uint32_t index = 0;

    for (const auto& child : child_nodes) {
        JSValue child_obj = DOMBindings::WrapNode(ctx, child);
        JS_SetPropertyUint32(ctx, nodes_array, index++, child_obj);
    }

    return nodes_array;
}

// Node.firstChild getter
static JSValue js_element_get_first_child(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto first_child = element->GetFirstChild();
    if (!first_child) {
        return JS_NULL;
    }

    return DOMBindings::WrapNode(ctx, first_child);
}

// Node.lastChild getter
static JSValue js_element_get_last_child(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto last_child = element->GetLastChild();
    if (!last_child) {
        return JS_NULL;
    }

    return DOMBindings::WrapNode(ctx, last_child);
}

// Node.nextSibling getter
static JSValue js_element_get_next_sibling(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto next_sibling = element->GetNextSibling();
    if (!next_sibling) {
        return JS_NULL;
    }

    return DOMBindings::WrapNode(ctx, next_sibling);
}

// Node.previousSibling getter
static JSValue js_element_get_previous_sibling(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto previous_sibling = element->GetPreviousSibling();
    if (!previous_sibling) {
        return JS_NULL;
    }

    return DOMBindings::WrapNode(ctx, previous_sibling);
}

// Node.nodeType getter
static JSValue js_element_get_node_type(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    return JS_NewInt32(ctx, static_cast<int>(element->GetNodeType()));
}

// Node.cloneNode(deep)
static JSValue js_element_clone_node(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    bool deep = false;
    if (argc > 0) {
        deep = JS_ToBool(ctx, argv[0]);
    }

    auto cloned = element->CloneNode(deep);
    if (!cloned) {
        return JS_NULL;
    }

    return DOMBindings::WrapNode(ctx, cloned);
}

// Node.contains(node)
static JSValue js_element_contains(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "contains requires 1 argument");
    }

    // 尝试解包为 Node
    auto other_element = DOMBindings::UnwrapElement(ctx, argv[0]);
    if (other_element) {
        return JS_NewBool(ctx, element->Contains(other_element));
    }

    auto other_text = DOMBindings::UnwrapText(ctx, argv[0]);
    if (other_text) {
        return JS_NewBool(ctx, element->Contains(other_text));
    }

    return JS_ThrowTypeError(ctx, "contains requires a Node argument");
}

// Node.hasChildNodes()
static JSValue js_element_has_child_nodes(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    return JS_NewBool(ctx, element->GetChildNodes().size() > 0);
}

// Element.hasAttribute(name)
static JSValue js_element_has_attribute(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "hasAttribute requires 1 argument");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_EXCEPTION;
    }

    bool has_attr = element->HasAttribute(name);
    JS_FreeCString(ctx, name);

    return JS_NewBool(ctx, has_attr);
}

// Element.removeAttribute(name)
static JSValue js_element_remove_attribute(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "removeAttribute requires 1 argument");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_EXCEPTION;
    }

    element->RemoveAttribute(name);
    JS_FreeCString(ctx, name);

    return JS_UNDEFINED;
}

// Element.innerHTML getter
static JSValue js_element_get_inner_html(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    return JS_NewString(ctx, element->GetInnerHTML().c_str());
}

// Element.innerHTML setter
static JSValue js_element_set_inner_html(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    const char* html = JS_ToCString(ctx, val);
    if (!html) {
        return JS_EXCEPTION;
    }

    // 在设置innerHTML之前，需要做两件事：
    // 1. 清除焦点（如果焦点元素在子树中）
    // 2. 清理所有子节点的缓存

    // 1. 清除焦点
    // 获取当前焦点元素，检查是否在即将被删除的子树中
    auto doc = element->GetOwnerDocument();
    if (doc) {
        // 注意：我们无法直接访问 FocusManager，所以我们通过 Document 来清除焦点
        // 这里我们简单地遍历子树，如果发现有元素有 :focus 伪类，就清除它
        std::function<void(std::shared_ptr<Node>)> clear_focus_recursive;
        clear_focus_recursive = [&](std::shared_ptr<Node> node) {
            if (!node) return;

            if (auto elem = std::dynamic_pointer_cast<Element>(node)) {
                // 如果元素有焦点，移除焦点伪类
                if (elem->HasPseudoClass("focus")) {
                    elem->SetPseudoClass("focus", false);
                    elem->SetPseudoClass("focus-visible", false);
                }
            }

            // 递归处理子节点
            for (const auto& child : node->GetChildNodes()) {
                clear_focus_recursive(child);
            }
        };

        for (const auto& child : element->GetChildNodes()) {
            clear_focus_recursive(child);
        }
    }

    // 2. 清理缓存
    std::function<void(std::shared_ptr<Node>)> clear_cache_recursive;
    clear_cache_recursive = [&](std::shared_ptr<Node> node) {
        if (!node) return;

        // 清理当前节点的缓存
        if (auto elem = std::dynamic_pointer_cast<Element>(node)) {
            DOMBindings::RemoveFromElementCache(elem.get());
        } else if (auto text = std::dynamic_pointer_cast<Text>(node)) {
            DOMBindings::RemoveFromTextCache(text.get());
        }

        // 递归清理子节点
        for (const auto& child : node->GetChildNodes()) {
            clear_cache_recursive(child);
        }
    };

    // 清理所有子节点的缓存
    for (const auto& child : element->GetChildNodes()) {
        clear_cache_recursive(child);
    }

    element->SetInnerHTML(html);
    JS_FreeCString(ctx, html);

    return JS_UNDEFINED;
}

// Element.outerHTML getter
static JSValue js_element_get_outer_html(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    return JS_NewString(ctx, element->GetOuterHTML().c_str());
}

// Element.outerHTML setter
static JSValue js_element_set_outer_html(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    const char* html = JS_ToCString(ctx, val);
    if (!html) {
        return JS_EXCEPTION;
    }

    element->SetOuterHTML(html);
    JS_FreeCString(ctx, html);

    return JS_UNDEFINED;
}

// ========== HTMLInputElement 特殊属性 ==========

static std::shared_ptr<HTMLInputElement> get_text_selectable_input_element(const std::shared_ptr<Element>& element) {
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
    if (!input || !input->SupportsTextEditing()) {
        return nullptr;
    }
    return input;
}

static std::shared_ptr<HTMLTextAreaElement> get_text_selectable_textarea_element(const std::shared_ptr<Element>& element) {
    return std::dynamic_pointer_cast<HTMLTextAreaElement>(element);
}

// Element.value getter (for input/textarea elements)
static JSValue js_element_get_value(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    // 检查是否是 input 或 textarea 元素
    std::string tag_name = element->GetTagName();
    if (tag_name == "input") {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (input) {
            return JS_NewString(ctx, input->GetValue().c_str());
        }
    } else if (tag_name == "textarea") {
        auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(element);
        if (textarea) {
            return JS_NewString(ctx, textarea->GetValue().c_str());
        }
    }

    // 其他元素返回 undefined
    return JS_UNDEFINED;
}

// Element.value setter (for input/textarea elements)
static JSValue js_element_set_value(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    const char* value = JS_ToCString(ctx, val);
    if (!value) {
        return JS_EXCEPTION;
    }

    // 检查是否是 input 或 textarea 元素
    std::string tag_name = element->GetTagName();

    if (tag_name == "input") {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (input) {
            input->SetValue(value, false);  // 不触发事件
        }
    } else if (tag_name == "textarea") {
        auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(element);
        if (textarea) {
            textarea->SetValue(value, false);  // 不触发事件
        }
    }

    JS_FreeCString(ctx, value);
    return JS_UNDEFINED;
}

static JSValue js_element_get_selection_start(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (auto input = get_text_selectable_input_element(element)) {
        return JS_NewInt32(ctx, input->GetSelectionStart());
    }
    if (auto textarea = get_text_selectable_textarea_element(element)) {
        return JS_NewInt32(ctx, textarea->GetSelectionStart());
    }

    return JS_UNDEFINED;
}

static JSValue js_element_set_selection_start(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    int start = 0;
    if (JS_ToInt32(ctx, &start, val) != 0) {
        return JS_EXCEPTION;
    }

    if (auto input = get_text_selectable_input_element(element)) {
        int end = input->GetSelectionEnd();
        input->SetSelectionRange(start, std::max(start, end));
        return JS_UNDEFINED;
    }
    if (auto textarea = get_text_selectable_textarea_element(element)) {
        int end = textarea->GetSelectionEnd();
        textarea->SetSelectionRange(start, std::max(start, end));
        return JS_UNDEFINED;
    }

    return JS_UNDEFINED;
}

static JSValue js_element_get_selection_end(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (auto input = get_text_selectable_input_element(element)) {
        return JS_NewInt32(ctx, input->GetSelectionEnd());
    }
    if (auto textarea = get_text_selectable_textarea_element(element)) {
        return JS_NewInt32(ctx, textarea->GetSelectionEnd());
    }

    return JS_UNDEFINED;
}

static JSValue js_element_set_selection_end(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    int end = 0;
    if (JS_ToInt32(ctx, &end, val) != 0) {
        return JS_EXCEPTION;
    }

    if (auto input = get_text_selectable_input_element(element)) {
        int start = input->GetSelectionStart();
        input->SetSelectionRange(std::min(start, end), end);
        return JS_UNDEFINED;
    }
    if (auto textarea = get_text_selectable_textarea_element(element)) {
        int start = textarea->GetSelectionStart();
        textarea->SetSelectionRange(std::min(start, end), end);
        return JS_UNDEFINED;
    }

    return JS_UNDEFINED;
}

static JSValue js_element_set_selection_range(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setSelectionRange requires 2 arguments");
    }

    int start = 0;
    int end = 0;
    if (JS_ToInt32(ctx, &start, argv[0]) != 0 || JS_ToInt32(ctx, &end, argv[1]) != 0) {
        return JS_EXCEPTION;
    }

    if (auto input = get_text_selectable_input_element(element)) {
        input->SetSelectionRange(start, end);
        return JS_UNDEFINED;
    }
    if (auto textarea = get_text_selectable_textarea_element(element)) {
        textarea->SetSelectionRange(start, end);
        return JS_UNDEFINED;
    }

    return JS_UNDEFINED;
}

static JSValue js_element_select(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (auto input = get_text_selectable_input_element(element)) {
        input->Select();
        return JS_UNDEFINED;
    }
    if (auto textarea = get_text_selectable_textarea_element(element)) {
        textarea->Select();
        return JS_UNDEFINED;
    }

    return JS_UNDEFINED;
}

static JSValue js_element_focus(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    element->Focus();
    return JS_UNDEFINED;
}

static JSValue js_element_blur(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    element->Blur();
    return JS_UNDEFINED;
}

// Element.checked getter (for checkbox/radio input elements)
static JSValue js_element_get_checked(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    // 只对 input 元素有效
    std::string tag_name = element->GetTagName();
    if (tag_name == "input") {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (input) {
            return JS_NewBool(ctx, input->GetChecked());
        }
    }

    return JS_UNDEFINED;
}

// Element.checked setter (for checkbox/radio input elements)
static JSValue js_element_set_checked(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    // 只对 input 元素有效
    std::string tag_name = element->GetTagName();
    if (tag_name == "input") {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (input) {
            bool checked = JS_ToBool(ctx, val);
            input->SetChecked(checked, false);  // 不触发事件
        }
    }

    return JS_UNDEFINED;
}

// ========== 阶段2: 查询选择器 API 绑定 ==========

// Element.querySelector(selector)
static JSValue js_element_query_selector(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "querySelector requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_EXCEPTION;
    }

    auto result = element->QuerySelector(selector);
    JS_FreeCString(ctx, selector);

    if (!result) {
        return JS_NULL;
    }

    return DOMBindings::WrapElement(ctx, result);
}

// Element.querySelectorAll(selector)
static JSValue js_element_query_selector_all(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "querySelectorAll requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_EXCEPTION;
    }

    auto results = element->QuerySelectorAll(selector);
    JS_FreeCString(ctx, selector);

    // 创建数组
    JSValue array = JS_NewArray(ctx);
    uint32_t index = 0;

    for (const auto& elem : results) {
        JSValue elem_obj = DOMBindings::WrapElement(ctx, elem);
        JS_SetPropertyUint32(ctx, array, index++, elem_obj);
    }

    return array;
}

// Element.matches(selector)
static JSValue js_element_matches(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "matches requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_EXCEPTION;
    }

    bool matches = element->Matches(selector);
    JS_FreeCString(ctx, selector);

    return JS_NewBool(ctx, matches);
}

// Element.closest(selector)
static JSValue js_element_closest(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "closest requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_EXCEPTION;
    }

    auto result = element->Closest(selector);
    JS_FreeCString(ctx, selector);

    if (!result) {
        return JS_NULL;
    }

    return DOMBindings::WrapElement(ctx, result);
}

// HTMLCanvasElement.width getter
static JSValue js_element_get_canvas_width(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) return JS_EXCEPTION;

    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(element);
    if (!canvas) return JS_NewInt32(ctx, 0);

    return JS_NewInt32(ctx, static_cast<int>(canvas->GetWidth()));
}

// HTMLCanvasElement.width setter
static JSValue js_element_set_canvas_width(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) return JS_EXCEPTION;

    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(element);
    if (!canvas) return JS_UNDEFINED;

    int32_t width;
    if (JS_ToInt32(ctx, &width, val) != 0) return JS_EXCEPTION;
    if (width > 0) {
        canvas->SetWidth(static_cast<unsigned long>(width));
    }
    return JS_UNDEFINED;
}

// HTMLCanvasElement.height getter
static JSValue js_element_get_canvas_height(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) return JS_EXCEPTION;

    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(element);
    if (!canvas) return JS_NewInt32(ctx, 0);

    return JS_NewInt32(ctx, static_cast<int>(canvas->GetHeight()));
}

// HTMLCanvasElement.height setter
static JSValue js_element_set_canvas_height(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) return JS_EXCEPTION;

    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(element);
    if (!canvas) return JS_UNDEFINED;

    int32_t height;
    if (JS_ToInt32(ctx, &height, val) != 0) return JS_EXCEPTION;
    if (height > 0) {
        canvas->SetHeight(static_cast<unsigned long>(height));
    }
    return JS_UNDEFINED;
}

// HTMLCanvasElement.getContext(contextId)
static JSValue js_element_get_context(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    // 尝试转换为 HTMLCanvasElement
    auto canvas = std::dynamic_pointer_cast<HTMLCanvasElement>(element);
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

    // 保存context_id比较结果
    std::string context_id_str(context_id);
    void* context = canvas->GetContext(context_id);
    JS_FreeCString(ctx, context_id);

    if (!context) {
        return JS_NULL;
    }

    // 目前只支持 "2d" context
    if (context_id_str == "2d") {
        auto context_2d = static_cast<CanvasRenderingContext2D*>(context);
        return CanvasBindings::WrapContext2D(ctx, context_2d);
    }

    return JS_NULL;
}

// Element.getBoundingClientRect()
static JSValue js_element_get_bounding_client_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    auto rect = element->GetBoundingClientRect();

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

// Element.scrollIntoView(alignToTop)
static JSValue js_element_scroll_into_view(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }

    bool align_to_top = true;
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        align_to_top = JS_ToBool(ctx, argv[0]);
    }

    element->ScrollIntoView(align_to_top);
    return JS_UNDEFINED;
}

// Element.isContentEditable getter
static JSValue js_element_get_is_content_editable(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) {
        return JS_EXCEPTION;
    }
    return JS_NewBool(ctx, element->IsContentEditable());
}

// Element 类定义
static const JSCFunctionListEntry js_element_proto_funcs[] = {
    // 基础属性
    JS_CGETSET_MAGIC_DEF("tagName", js_element_get_tag_name, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("id", js_element_get_id, js_element_set_id, 0),
    JS_CGETSET_MAGIC_DEF("className", js_element_get_class_name, js_element_set_class_name, 0),
    JS_CGETSET_MAGIC_DEF("textContent", js_element_get_text_content, js_element_set_text_content, 0),
    JS_CGETSET_MAGIC_DEF("parentNode", js_element_get_parent_node, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("children", js_element_get_children, nullptr, 0),

    // 阶段1: Node 属性
    JS_CGETSET_MAGIC_DEF("childNodes", js_element_get_child_nodes, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("firstChild", js_element_get_first_child, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("lastChild", js_element_get_last_child, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("nextSibling", js_element_get_next_sibling, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("previousSibling", js_element_get_previous_sibling, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("nodeType", js_element_get_node_type, nullptr, 0),

    // 阶段1: HTML 内容
    JS_CGETSET_MAGIC_DEF("innerHTML", js_element_get_inner_html, js_element_set_inner_html, 0),
    JS_CGETSET_MAGIC_DEF("outerHTML", js_element_get_outer_html, js_element_set_outer_html, 0),

    // 阶段3: 对象属性
    JS_CGETSET_MAGIC_DEF("classList", js_element_get_class_list, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("style", js_element_get_style, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("dataset", js_element_get_dataset, nullptr, 0),

    // HTMLInputElement / HTMLTextAreaElement 特殊属性
    JS_CGETSET_MAGIC_DEF("value", js_element_get_value, js_element_set_value, 0),
    JS_CGETSET_MAGIC_DEF("checked", js_element_get_checked, js_element_set_checked, 0),
    JS_CGETSET_MAGIC_DEF("selectionStart", js_element_get_selection_start, js_element_set_selection_start, 0),
    JS_CGETSET_MAGIC_DEF("selectionEnd", js_element_get_selection_end, js_element_set_selection_end, 0),

    // HTMLCanvasElement 属性
    JS_CGETSET_MAGIC_DEF("width", js_element_get_canvas_width, js_element_set_canvas_width, 0),
    JS_CGETSET_MAGIC_DEF("height", js_element_get_canvas_height, js_element_set_canvas_height, 0),

    // HTMLImageElement 属性
    JS_CGETSET_MAGIC_DEF("src", js_element_get_img_src, js_element_set_img_src, 0),
    JS_CGETSET_MAGIC_DEF("alt", js_element_get_img_alt, js_element_set_img_alt, 0),
    JS_CGETSET_MAGIC_DEF("naturalWidth", js_element_get_img_natural_width, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("naturalHeight", js_element_get_img_natural_height, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("complete", js_element_get_img_complete, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("crossOrigin", js_element_get_img_cross_origin, js_element_set_img_cross_origin, 0),

    // 基础方法
    JS_CFUNC_DEF("getAttribute", 1, js_element_get_attribute),
    JS_CFUNC_DEF("setAttribute", 2, js_element_set_attribute),
    JS_CFUNC_DEF("appendChild", 1, js_element_append_child),
    JS_CFUNC_DEF("removeChild", 1, js_element_remove_child),
    JS_CFUNC_DEF("replaceChild", 2, js_element_replace_child),
    JS_CFUNC_DEF("insertBefore", 2, js_element_insert_before),
    JS_CFUNC_DEF("addEventListener", 2, js_element_add_event_listener),
    JS_CFUNC_DEF("removeEventListener", 2, js_element_remove_event_listener),
    JS_CFUNC_DEF("dispatchEvent", 1, js_element_dispatch_event),

    // 阶段1: Node 方法
    JS_CFUNC_DEF("cloneNode", 1, js_element_clone_node),
    JS_CFUNC_DEF("contains", 1, js_element_contains),
    JS_CFUNC_DEF("hasChildNodes", 0, js_element_has_child_nodes),

    // 阶段1: Element 属性操作
    JS_CFUNC_DEF("hasAttribute", 1, js_element_has_attribute),
    JS_CFUNC_DEF("removeAttribute", 1, js_element_remove_attribute),

    // 阶段2: 查询选择器
    JS_CFUNC_DEF("querySelector", 1, js_element_query_selector),
    JS_CFUNC_DEF("querySelectorAll", 1, js_element_query_selector_all),
    JS_CFUNC_DEF("matches", 1, js_element_matches),
    JS_CFUNC_DEF("closest", 1, js_element_closest),

    // HTMLCanvasElement: getContext方法
    JS_CFUNC_DEF("getContext", 1, js_element_get_context),

    // 几何信息
    JS_CFUNC_DEF("getBoundingClientRect", 0, js_element_get_bounding_client_rect),
    JS_CFUNC_DEF("scrollIntoView", 1, js_element_scroll_into_view),
    JS_CFUNC_DEF("setSelectionRange", 2, js_element_set_selection_range),
    JS_CFUNC_DEF("select", 0, js_element_select),
    JS_CFUNC_DEF("focus", 0, js_element_focus),
    JS_CFUNC_DEF("blur", 0, js_element_blur),

    // ContentEditable
    JS_CGETSET_MAGIC_DEF("isContentEditable", js_element_get_is_content_editable, nullptr, 0),
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
// Note: textContent for Text nodes is equivalent to data property
static const JSCFunctionListEntry js_text_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("data", js_text_get_data, js_text_set_data, 0),
    JS_CGETSET_MAGIC_DEF("textContent", js_text_get_data, js_text_set_data, 0),
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

// Document.createElementNS(namespaceURI, qualifiedName)
// For SVG elements, namespaceURI is "http://www.w3.org/2000/svg"
// We ignore the namespace and just use the tag name, since our Document::CreateElement
// already handles SVG elements correctly based on tag name.
static JSValue js_document_create_element_ns(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "createElementNS requires 2 arguments");
    }

    // First argument is namespace URI (we ignore it since we handle SVG by tag name)
    // Second argument is the qualified name (tag name)
    const char* qualified_name = JS_ToCString(ctx, argv[1]);
    if (!qualified_name) {
        return JS_EXCEPTION;
    }

    auto element = document->CreateElement(qualified_name);
    JS_FreeCString(ctx, qualified_name);

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

// Document.createRange()
static JSValue js_document_create_range(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    auto range = document->CreateRange();
    return bindings::WrapRange(ctx, range);
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

// Document.querySelector(selector)
static JSValue js_document_query_selector(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "querySelector requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_EXCEPTION;
    }

    // 从 document element 开始查询
    auto doc_element = document->GetDocumentElement();
    if (!doc_element) {
        JS_FreeCString(ctx, selector);
        return JS_NULL;
    }

    auto result = doc_element->QuerySelector(selector);
    JS_FreeCString(ctx, selector);

    if (!result) {
        return JS_NULL;
    }

    return DOMBindings::WrapElement(ctx, result);
}

// Document.querySelectorAll(selector)
static JSValue js_document_query_selector_all(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "querySelectorAll requires 1 argument");
    }

    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_EXCEPTION;
    }

    // 从 document element 开始查询
    auto doc_element = document->GetDocumentElement();
    std::vector<std::shared_ptr<Element>> results;

    if (doc_element) {
        results = doc_element->QuerySelectorAll(selector);
    }

    JS_FreeCString(ctx, selector);

    // 创建数组
    JSValue array = JS_NewArray(ctx);
    uint32_t index = 0;

    for (const auto& elem : results) {
        JSValue elem_obj = DOMBindings::WrapElement(ctx, elem);
        JS_SetPropertyUint32(ctx, array, index++, elem_obj);
    }

    return array;
}

// Document.getElementsByClassName(className)
static JSValue js_document_get_elements_by_class_name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getElementsByClassName requires 1 argument");
    }

    const char* class_name = JS_ToCString(ctx, argv[0]);
    if (!class_name) {
        return JS_EXCEPTION;
    }

    auto results = document->GetElementsByClassName(class_name);
    JS_FreeCString(ctx, class_name);

    // 创建数组
    JSValue array = JS_NewArray(ctx);
    uint32_t index = 0;

    for (const auto& elem : results) {
        JSValue elem_obj = DOMBindings::WrapElement(ctx, elem);
        JS_SetPropertyUint32(ctx, array, index++, elem_obj);
    }

    return array;
}

// Document.getElementsByTagName(tagName)
static JSValue js_document_get_elements_by_tag_name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getElementsByTagName requires 1 argument");
    }

    const char* tag_name = JS_ToCString(ctx, argv[0]);
    if (!tag_name) {
        return JS_EXCEPTION;
    }

    auto results = document->GetElementsByTagName(tag_name);
    JS_FreeCString(ctx, tag_name);

    // 创建数组
    JSValue array = JS_NewArray(ctx);
    uint32_t index = 0;

    for (const auto& elem : results) {
        JSValue elem_obj = DOMBindings::WrapElement(ctx, elem);
        JS_SetPropertyUint32(ctx, array, index++, elem_obj);
    }

    return array;
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

// Document.documentElement getter
static JSValue js_document_get_document_element(JSContext* ctx, JSValueConst this_val, int magic) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    auto document_element = document->GetDocumentElement();
    if (!document_element) {
        return JS_NULL;
    }

    return DOMBindings::WrapElement(ctx, document_element);
}

// Document.head getter
static JSValue js_document_get_head(JSContext* ctx, JSValueConst this_val, int magic) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    auto head = document->GetHead();
    if (!head) {
        return JS_NULL;
    }

    return DOMBindings::WrapElement(ctx, head);
}

// Document.activeElement getter
static JSValue js_document_get_active_element(JSContext* ctx, JSValueConst this_val, int magic) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    auto active = document->GetActiveElement();
    if (!active) {
        // 如果没有焦点元素，返回 body
        auto body = document->GetBody();
        if (body) {
            return DOMBindings::WrapElement(ctx, body);
        }
        return JS_NULL;
    }

    return DOMBindings::WrapElement(ctx, active);
}

// Phase 4: 批量更新 API - document.__beginBatch()
static JSValue js_document_begin_batch(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    document->BeginBatch();
    return JS_UNDEFINED;
}

// Phase 4: 批量更新 API - document.__endBatch()
static JSValue js_document_end_batch(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    document->EndBatch();
    return JS_UNDEFINED;
}

// Phase 4: 批量更新 API - document.__isInBatch()
static JSValue js_document_is_in_batch(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    return JS_NewBool(ctx, document->IsInBatch());
}

// ========== execCommand API ==========

// document.execCommand(command, showUI, value)
static JSValue js_document_exec_command(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

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

    // 获取 EventLoop 中的 ContentEditableHandler
    auto event_loop = DOMBindings::GetGlobalEventLoop();
    if (!event_loop) {
        return JS_NewBool(ctx, false);
    }

    auto handler = event_loop->GetContentEditableHandler();
    if (!handler) {
        return JS_NewBool(ctx, false);
    }

    bool result = handler->ExecCommand(document, cmd, value);
    return JS_NewBool(ctx, result);
}

// document.queryCommandState(command)
static JSValue js_document_query_command_state(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "queryCommandState requires 1 argument");
    }

    const char* command = JS_ToCString(ctx, argv[0]);
    if (!command) {
        return JS_EXCEPTION;
    }

    std::string cmd(command);
    JS_FreeCString(ctx, command);

    // 获取 EventLoop 中的 ContentEditableHandler
    auto event_loop = DOMBindings::GetGlobalEventLoop();
    if (!event_loop) {
        return JS_NewBool(ctx, false);
    }

    auto handler = event_loop->GetContentEditableHandler();
    if (!handler) {
        return JS_NewBool(ctx, false);
    }

    bool result = handler->QueryCommandState(document, cmd);
    return JS_NewBool(ctx, result);
}

// document.queryCommandEnabled(command)
static JSValue js_document_query_command_enabled(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto document = DOMBindings::UnwrapDocument(ctx, this_val);
    if (!document) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "queryCommandEnabled requires 1 argument");
    }

    const char* command = JS_ToCString(ctx, argv[0]);
    if (!command) {
        return JS_EXCEPTION;
    }

    std::string cmd(command);
    JS_FreeCString(ctx, command);

    // 获取 EventLoop 中的 ContentEditableHandler
    auto event_loop = DOMBindings::GetGlobalEventLoop();
    if (!event_loop) {
        return JS_NewBool(ctx, false);
    }

    auto handler = event_loop->GetContentEditableHandler();
    if (!handler) {
        return JS_NewBool(ctx, false);
    }

    bool result = handler->QueryCommandEnabled(document, cmd);
    return JS_NewBool(ctx, result);
}

// Document 类定义
static const JSCFunctionListEntry js_document_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("documentElement", js_document_get_document_element, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("body", js_document_get_body, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("head", js_document_get_head, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("activeElement", js_document_get_active_element, nullptr, 0),
    JS_CFUNC_DEF("createElement", 1, js_document_create_element),
    JS_CFUNC_DEF("createElementNS", 2, js_document_create_element_ns),
    JS_CFUNC_DEF("createTextNode", 1, js_document_create_text_node),
    JS_CFUNC_DEF("createRange", 0, js_document_create_range),
    JS_CFUNC_DEF("getElementById", 1, js_document_get_element_by_id),

    // 阶段2: 查询选择器
    JS_CFUNC_DEF("querySelector", 1, js_document_query_selector),
    JS_CFUNC_DEF("querySelectorAll", 1, js_document_query_selector_all),
    JS_CFUNC_DEF("getElementsByClassName", 1, js_document_get_elements_by_class_name),
    JS_CFUNC_DEF("getElementsByTagName", 1, js_document_get_elements_by_tag_name),

    // Phase 4: 批量更新 API
    JS_CFUNC_DEF("__beginBatch", 0, js_document_begin_batch),
    JS_CFUNC_DEF("__endBatch", 0, js_document_end_batch),
    JS_CFUNC_DEF("__isInBatch", 0, js_document_is_in_batch),

    // execCommand API
    JS_CFUNC_DEF("execCommand", 3, js_document_exec_command),
    JS_CFUNC_DEF("queryCommandState", 1, js_document_query_command_state),
    JS_CFUNC_DEF("queryCommandEnabled", 1, js_document_query_command_enabled),
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

// Event 构造函数 (阶段4)
static JSValue js_event_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "Event constructor requires at least 1 argument");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    bool bubbles = false;
    bool cancelable = false;

    // 解析第二个参数（options对象）
    if (argc >= 2 && JS_IsObject(argv[1])) {
        JSValue bubbles_val = JS_GetPropertyStr(ctx, argv[1], "bubbles");
        if (!JS_IsUndefined(bubbles_val)) {
            bubbles = JS_ToBool(ctx, bubbles_val);
        }
        JS_FreeValue(ctx, bubbles_val);

        JSValue cancelable_val = JS_GetPropertyStr(ctx, argv[1], "cancelable");
        if (!JS_IsUndefined(cancelable_val)) {
            cancelable = JS_ToBool(ctx, cancelable_val);
        }
        JS_FreeValue(ctx, cancelable_val);
    }

    auto event = std::make_shared<Event>(type, bubbles, cancelable);
    JS_FreeCString(ctx, type);

    JSValue obj = JS_NewObjectClass(ctx, DOMBindings::event_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto ptr = new std::shared_ptr<Event>(event);
    JS_SetOpaque(obj, ptr);

    return obj;
}

// Event.type getter
static JSValue js_event_get_type(JSContext* ctx, JSValueConst this_val, int magic) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }
    return JS_NewString(ctx, event->GetType().c_str());
}

// Event.target getter (阶段4)
static JSValue js_event_get_target(JSContext* ctx, JSValueConst this_val, int magic) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }

    auto target = event->GetTarget();
    if (!target) {
        return JS_NULL;
    }

    return DOMBindings::WrapNode(ctx, target);
}

// Event.currentTarget getter (阶段4)
static JSValue js_event_get_current_target(JSContext* ctx, JSValueConst this_val, int magic) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }

    auto current_target = event->GetCurrentTarget();
    if (!current_target) {
        return JS_NULL;
    }

    return DOMBindings::WrapNode(ctx, current_target);
}

// Event.bubbles getter (阶段4)
static JSValue js_event_get_bubbles(JSContext* ctx, JSValueConst this_val, int magic) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }
    return JS_NewBool(ctx, event->GetBubbles());
}

// Event.cancelable getter (阶段4)
static JSValue js_event_get_cancelable(JSContext* ctx, JSValueConst this_val, int magic) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }
    return JS_NewBool(ctx, event->GetCancelable());
}

// Event.defaultPrevented getter (阶段4)
static JSValue js_event_get_default_prevented(JSContext* ctx, JSValueConst this_val, int magic) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }
    return JS_NewBool(ctx, event->IsDefaultPrevented());
}

// Event.timeStamp getter (阶段4)
static JSValue js_event_get_time_stamp(JSContext* ctx, JSValueConst this_val, int magic) {
    auto event = DOMBindings::UnwrapEvent(ctx, this_val);
    if (!event) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, event->GetTimeStamp());
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
    // 阶段4: Event 属性
    JS_CGETSET_MAGIC_DEF("type", js_event_get_type, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("target", js_event_get_target, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("currentTarget", js_event_get_current_target, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("bubbles", js_event_get_bubbles, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("cancelable", js_event_get_cancelable, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("defaultPrevented", js_event_get_default_prevented, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("timeStamp", js_event_get_time_stamp, nullptr, 0),

    // 阶段4: Event 方法
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

    // 注册Event构造函数到全局对象
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue event_ctor = JS_NewCFunction2(ctx, js_event_constructor, "Event", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, event_ctor, proto);
    JS_SetPropertyStr(ctx, global, "Event", event_ctor);
    JS_FreeValue(ctx, global);
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
    InitDOMTokenListClass(ctx);
    InitCSSStyleDeclarationClass(ctx);
    InitDOMStringMapClass(ctx);

    // 初始化 Canvas 绑定
    CanvasBindings::Init(ctx);

    // 初始化 Terminal 和 LogView 绑定
    TerminalBindings::Init(ctx);

    // 初始化 Image 构造函数
    InitImageConstructor(ctx);

    initialized = true;
}

void DOMBindings::SetGlobalDocument(JSContext* ctx, std::shared_ptr<Document> document) {
    if (!document) {
        return;
    }

    // 包装 Document 对象
    JSValue doc_obj = WrapDocument(ctx, document);

    // 设置为全局对象
    // 注意：JS_SetPropertyStr 会接管 doc_obj 的所有权，不需要手动 FreeValue
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "document", doc_obj);
    JS_FreeValue(ctx, global);
}

void DOMBindings::Cleanup(JSContext* ctx) {
    // ========== 阶段0：先停掉所有 timer / raf / microtask ==========
    if (g_task_scheduler) {
        g_task_scheduler->ClearAllTasks();
    }

    // ========== 阶段1：清理 C++ 侧 DOM 事件引用 ==========
    if (ctx) {
        auto& dom_binding_map = DOMBindingMap::GetInstance();
        dom_binding_map.ForEach([](Node* node, JSContext* entry_ctx, JSValueConst value) {
            if (!node || !entry_ctx || JS_IsUndefined(value) || JS_IsNull(value)) {
                return;
            }

            if (JS_GetOpaque(value, bindings::GetElementClassID())) {
                bindings::ClearElementEventProperties(entry_ctx, value);

                if (auto* element = dynamic_cast<Element*>(node)) {
                    element->ClearAllEventListeners();
                }
            }
        });

        std::vector<std::pair<JSContext*, JSValue>> cachedValues;
        cachedValues.reserve(element_cache_.size() + text_cache_.size() + document_cache_.size());

        for (auto& [_, entry] : element_cache_) {
            if (!JS_IsUndefined(entry.second) && !JS_IsNull(entry.second)) {
                bindings::ClearElementEventProperties(entry.first ? entry.first : ctx, entry.second);
                cachedValues.push_back(entry);
            }
        }
        for (auto& [_, entry] : text_cache_) {
            if (!JS_IsUndefined(entry.second) && !JS_IsNull(entry.second)) {
                cachedValues.push_back(entry);
            }
        }
        for (auto& [_, entry] : document_cache_) {
            if (!JS_IsUndefined(entry.second) && !JS_IsNull(entry.second)) {
                cachedValues.push_back(entry);
            }
        }

        element_cache_.clear();
        text_cache_.clear();
        document_cache_.clear();

        for (auto& entry : cachedValues) {
            if (entry.first) {
                JS_FreeValue(entry.first, entry.second);
            }
        }

        JS_RunGC(JS_GetRuntime(ctx));
    }

    // ========== 阶段2：清除全局 document 对象 ==========
    if (ctx) {
        JSValue global = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, global, "document", JS_UNDEFINED);
        JS_FreeValue(ctx, global);
    }

    // ========== 阶段3：清理调度器和 DOMBindingMap ==========
    if (g_task_scheduler) {
        g_task_scheduler->ClearAllTasks();
    }
    g_task_scheduler.reset();
    g_event_loop = nullptr;

    if (ctx) {
        JS_RunGC(JS_GetRuntime(ctx));
        DOMBindingMap::GetInstance().Clear();
    }

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

    // 添加到缓存（弱引用，不增加引用计数）
    // 缓存只是一个查找表，不影响GC
    // finalizer 会在对象被GC时清理缓存
    element_cache_[raw_ptr] = std::make_pair(ctx, obj);

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

    // 添加到缓存（弱引用，不增加引用计数）
    text_cache_[raw_ptr] = std::make_pair(ctx, obj);

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

    // 添加到缓存（弱引用，不增加引用计数）
    document_cache_[raw_ptr] = std::make_pair(ctx, obj);

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
        // 注意：这个函数从 finalizer 调用
        // 缓存使用弱引用，不需要调用 JS_FreeValue
        // 直接从缓存中移除即可
        element_cache_.erase(it);
    }
}

void DOMBindings::RemoveFromTextCache(Text* ptr) {
    auto it = text_cache_.find(ptr);
    if (it != text_cache_.end()) {
        // 缓存使用弱引用，不需要调用 JS_FreeValue
        text_cache_.erase(it);
    }
}

void DOMBindings::RemoveFromDocumentCache(Document* ptr) {
    auto it = document_cache_.find(ptr);
    if (it != document_cache_.end()) {
        // 缓存使用弱引用，不需要调用 JS_FreeValue
        document_cache_.erase(it);
    }
}

// ========== DOMTokenList 类绑定 (阶段3) ==========

// DOMTokenList finalizer
static void js_dom_token_list_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<DOMTokenList>*>(JS_GetOpaque(val, DOMBindings::dom_token_list_class_id));
    if (ptr) {
        delete ptr;
    }
}

// DOMTokenList.add(token)
static JSValue js_dom_token_list_add(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto list = static_cast<std::shared_ptr<DOMTokenList>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_token_list_class_id));
    if (!list || !*list) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "add requires at least 1 argument");
    }

    const char* token = JS_ToCString(ctx, argv[0]);
    if (!token) {
        return JS_EXCEPTION;
    }

    try {
        (*list)->Add(token);
    } catch (const std::exception& e) {
        JS_FreeCString(ctx, token);
        return JS_ThrowTypeError(ctx, "%s", e.what());
    }

    JS_FreeCString(ctx, token);
    return JS_UNDEFINED;
}

// DOMTokenList.remove(token)
static JSValue js_dom_token_list_remove(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto list = static_cast<std::shared_ptr<DOMTokenList>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_token_list_class_id));
    if (!list || !*list) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "remove requires at least 1 argument");
    }

    const char* token = JS_ToCString(ctx, argv[0]);
    if (!token) {
        return JS_EXCEPTION;
    }

    try {
        (*list)->Remove(token);
    } catch (const std::exception& e) {
        JS_FreeCString(ctx, token);
        return JS_ThrowTypeError(ctx, "%s", e.what());
    }

    JS_FreeCString(ctx, token);
    return JS_UNDEFINED;
}

// DOMTokenList.toggle(token)
static JSValue js_dom_token_list_toggle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto list = static_cast<std::shared_ptr<DOMTokenList>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_token_list_class_id));
    if (!list || !*list) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "toggle requires at least 1 argument");
    }

    const char* token = JS_ToCString(ctx, argv[0]);
    if (!token) {
        return JS_EXCEPTION;
    }

    bool result;
    try {
        result = (*list)->Toggle(token);
    } catch (const std::exception& e) {
        JS_FreeCString(ctx, token);
        return JS_ThrowTypeError(ctx, "%s", e.what());
    }

    JS_FreeCString(ctx, token);
    return JS_NewBool(ctx, result);
}

// DOMTokenList.contains(token)
static JSValue js_dom_token_list_contains(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto list = static_cast<std::shared_ptr<DOMTokenList>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_token_list_class_id));
    if (!list || !*list) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "contains requires at least 1 argument");
    }

    const char* token = JS_ToCString(ctx, argv[0]);
    if (!token) {
        return JS_EXCEPTION;
    }

    bool result = (*list)->Contains(token);
    JS_FreeCString(ctx, token);

    return JS_NewBool(ctx, result);
}

// DOMTokenList.item(index)
static JSValue js_dom_token_list_item(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto list = static_cast<std::shared_ptr<DOMTokenList>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_token_list_class_id));
    if (!list || !*list) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "item requires at least 1 argument");
    }

    uint32_t index;
    if (JS_ToUint32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }

    std::string result = (*list)->Item(index);
    return JS_NewString(ctx, result.c_str());
}

// DOMTokenList.length getter
static JSValue js_dom_token_list_get_length(JSContext* ctx, JSValueConst this_val, int magic) {
    auto list = static_cast<std::shared_ptr<DOMTokenList>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_token_list_class_id));
    if (!list || !*list) {
        return JS_EXCEPTION;
    }

    return JS_NewUint32(ctx, (*list)->Length());
}

// DOMTokenList 类定义
static const JSCFunctionListEntry js_dom_token_list_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("length", js_dom_token_list_get_length, nullptr, 0),
    JS_CFUNC_DEF("add", 1, js_dom_token_list_add),
    JS_CFUNC_DEF("remove", 1, js_dom_token_list_remove),
    JS_CFUNC_DEF("toggle", 1, js_dom_token_list_toggle),
    JS_CFUNC_DEF("contains", 1, js_dom_token_list_contains),
    JS_CFUNC_DEF("item", 1, js_dom_token_list_item),
};

void DOMBindings::InitDOMTokenListClass(JSContext* ctx) {
    JSClassDef dom_token_list_class = {
        /* class_name */ "DOMTokenList",
        /* finalizer */ js_dom_token_list_finalizer,
        /* gc_mark */ nullptr,
        /* call */ nullptr,
        /* exotic */ nullptr,
    };

    JS_NewClassID(JS_GetRuntime(ctx), &dom_token_list_class_id);
    JS_NewClass(JS_GetRuntime(ctx), dom_token_list_class_id, &dom_token_list_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_dom_token_list_proto_funcs,
                               sizeof(js_dom_token_list_proto_funcs) / sizeof(js_dom_token_list_proto_funcs[0]));
    JS_SetClassProto(ctx, dom_token_list_class_id, proto);
}

// ========== CSSStyleDeclaration 类绑定 (阶段3) ==========

// CSSStyleDeclaration finalizer
static void js_css_style_declaration_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<CSSStyleDeclaration>*>(
        JS_GetOpaque(val, DOMBindings::css_style_declaration_class_id));
    if (ptr) {
        delete ptr;
    }
}

// CSSStyleDeclaration.setProperty(property, value, priority?)
static JSValue js_css_style_declaration_set_property(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto style = static_cast<std::shared_ptr<CSSStyleDeclaration>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::css_style_declaration_class_id));
    if (!style || !*style) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setProperty requires at least 2 arguments");
    }

    const char* property = JS_ToCString(ctx, argv[0]);
    if (!property) {
        return JS_EXCEPTION;
    }

    const char* value = JS_ToCString(ctx, argv[1]);
    if (!value) {
        JS_FreeCString(ctx, property);
        return JS_EXCEPTION;
    }

    const char* priority = "";
    if (argc >= 3) {
        priority = JS_ToCString(ctx, argv[2]);
        if (!priority) {
            JS_FreeCString(ctx, property);
            JS_FreeCString(ctx, value);
            return JS_EXCEPTION;
        }
    }

    (*style)->SetProperty(property, value, priority);

    JS_FreeCString(ctx, property);
    JS_FreeCString(ctx, value);
    if (argc >= 3) {
        JS_FreeCString(ctx, priority);
    }

    return JS_UNDEFINED;
}

// CSSStyleDeclaration.getPropertyValue(property)
static JSValue js_css_style_declaration_get_property_value(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto style = static_cast<std::shared_ptr<CSSStyleDeclaration>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::css_style_declaration_class_id));
    if (!style || !*style) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getPropertyValue requires at least 1 argument");
    }

    const char* property = JS_ToCString(ctx, argv[0]);
    if (!property) {
        return JS_EXCEPTION;
    }

    std::string result = (*style)->GetPropertyValue(property);
    JS_FreeCString(ctx, property);

    return JS_NewString(ctx, result.c_str());
}

// CSSStyleDeclaration.removeProperty(property)
static JSValue js_css_style_declaration_remove_property(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto style = static_cast<std::shared_ptr<CSSStyleDeclaration>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::css_style_declaration_class_id));
    if (!style || !*style) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "removeProperty requires at least 1 argument");
    }

    const char* property = JS_ToCString(ctx, argv[0]);
    if (!property) {
        return JS_EXCEPTION;
    }

    std::string result = (*style)->RemoveProperty(property);
    JS_FreeCString(ctx, property);

    return JS_NewString(ctx, result.c_str());
}

// CSSStyleDeclaration.cssText getter
static JSValue js_css_style_declaration_get_css_text(JSContext* ctx, JSValueConst this_val, int magic) {
    auto style = static_cast<std::shared_ptr<CSSStyleDeclaration>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::css_style_declaration_class_id));
    if (!style || !*style) {
        return JS_EXCEPTION;
    }

    return JS_NewString(ctx, (*style)->GetCssText().c_str());
}

// CSSStyleDeclaration.cssText setter
static JSValue js_css_style_declaration_set_css_text(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto style = static_cast<std::shared_ptr<CSSStyleDeclaration>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::css_style_declaration_class_id));
    if (!style || !*style) {
        return JS_EXCEPTION;
    }

    const char* css_text = JS_ToCString(ctx, val);
    if (!css_text) {
        return JS_EXCEPTION;
    }

    (*style)->SetCssText(css_text);
    JS_FreeCString(ctx, css_text);

    return JS_UNDEFINED;
}

// CSSStyleDeclaration.length getter
static JSValue js_css_style_declaration_get_length(JSContext* ctx, JSValueConst this_val, int magic) {
    auto style = static_cast<std::shared_ptr<CSSStyleDeclaration>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::css_style_declaration_class_id));
    if (!style || !*style) {
        return JS_EXCEPTION;
    }

    return JS_NewInt32(ctx, (*style)->Length());
}

static std::string camel_to_kebab(const std::string& camel) {
    std::string result;
    result.reserve(camel.size() + 4);
    for (size_t i = 0; i < camel.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(camel[i]);
        if (std::isupper(c)) {
            if (i > 0) {
                result += '-';
            }
            result += static_cast<char>(std::tolower(c));
        } else {
            result += static_cast<char>(c);
        }
    }
    return result;
}

static int js_css_style_declaration_get_own_property(JSContext* ctx, JSPropertyDescriptor* desc,
                                                     JSValueConst obj, JSAtom prop) {
    auto style = static_cast<std::shared_ptr<CSSStyleDeclaration>*>(
        JS_GetOpaque(obj, DOMBindings::css_style_declaration_class_id));
    if (!style || !*style) {
        return 0;
    }

    const char* prop_name = JS_AtomToCString(ctx, prop);
    if (!prop_name) {
        return -1;
    }

    std::string prop_str(prop_name);
    JS_FreeCString(ctx, prop_name);

    if (prop_str == "cssText" || prop_str == "length" ||
        prop_str == "setProperty" || prop_str == "getPropertyValue" ||
        prop_str == "removeProperty") {
        return 0;
    }

    std::string css_property = camel_to_kebab(prop_str);
    std::string value = (*style)->GetPropertyValue(css_property);

    if (desc) {
        desc->flags = JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE | JS_PROP_WRITABLE;
        desc->value = JS_NewString(ctx, value.c_str());
        desc->getter = JS_UNDEFINED;
        desc->setter = JS_UNDEFINED;
    }

    return 1;
}

static int js_css_style_declaration_set_property_value(JSContext* ctx, JSValueConst obj,
                                                       JSAtom prop, JSValueConst value,
                                                       JSValueConst receiver, int flags) {
    auto style = static_cast<std::shared_ptr<CSSStyleDeclaration>*>(
        JS_GetOpaque(obj, DOMBindings::css_style_declaration_class_id));
    if (!style || !*style) {
        return -1;
    }

    const char* prop_name = JS_AtomToCString(ctx, prop);
    if (!prop_name) {
        return -1;
    }

    std::string prop_str(prop_name);
    JS_FreeCString(ctx, prop_name);

    if (prop_str == "length") {
        return 0;
    }

    if (prop_str == "cssText") {
        const char* css_text = JS_ToCString(ctx, value);
        if (!css_text) {
            return -1;
        }
        (*style)->SetCssText(css_text);
        JS_FreeCString(ctx, css_text);
        return 1;
    }

    std::string css_property = camel_to_kebab(prop_str);
    const char* value_str = JS_ToCString(ctx, value);
    if (!value_str) {
        return -1;
    }

    (*style)->SetProperty(css_property, value_str, "");
    JS_FreeCString(ctx, value_str);
    return 1;
}

static JSClassExoticMethods js_css_style_declaration_exotic = {
    /* get_own_property */ js_css_style_declaration_get_own_property,
    /* get_own_property_names */ nullptr,
    /* delete_property */ nullptr,
    /* define_own_property */ nullptr,
    /* has_property */ nullptr,
    /* get_property */ nullptr,
    /* set_property */ js_css_style_declaration_set_property_value,
};

// CSSStyleDeclaration 类定义
static const JSCFunctionListEntry js_css_style_declaration_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("cssText", js_css_style_declaration_get_css_text, js_css_style_declaration_set_css_text, 0),
    JS_CGETSET_MAGIC_DEF("length", js_css_style_declaration_get_length, nullptr, 0),
    JS_CFUNC_DEF("setProperty", 2, js_css_style_declaration_set_property),
    JS_CFUNC_DEF("getPropertyValue", 1, js_css_style_declaration_get_property_value),
    JS_CFUNC_DEF("removeProperty", 1, js_css_style_declaration_remove_property),
};

void DOMBindings::InitCSSStyleDeclarationClass(JSContext* ctx) {
    JSClassDef css_style_declaration_class = {
        /* class_name */ "CSSStyleDeclaration",
        /* finalizer */ js_css_style_declaration_finalizer,
        /* gc_mark */ nullptr,
        /* call */ nullptr,
        /* exotic */ &js_css_style_declaration_exotic,
    };

    JS_NewClassID(JS_GetRuntime(ctx), &css_style_declaration_class_id);
    JS_NewClass(JS_GetRuntime(ctx), css_style_declaration_class_id, &css_style_declaration_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_css_style_declaration_proto_funcs,
                               sizeof(js_css_style_declaration_proto_funcs) / sizeof(js_css_style_declaration_proto_funcs[0]));
    JS_SetClassProto(ctx, css_style_declaration_class_id, proto);
}

// ========== DOMStringMap 类绑定 (阶段3) ==========

// DOMStringMap finalizer
static void js_dom_string_map_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<DOMStringMap>*>(
        JS_GetOpaque(val, DOMBindings::dom_string_map_class_id));
    if (ptr) {
        delete ptr;
    }
}

// DOMStringMap.set(name, value)
static JSValue js_dom_string_map_set(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto dataset = static_cast<std::shared_ptr<DOMStringMap>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_string_map_class_id));
    if (!dataset || !*dataset) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "set requires 2 arguments");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_EXCEPTION;
    }

    const char* value = JS_ToCString(ctx, argv[1]);
    if (!value) {
        JS_FreeCString(ctx, name);
        return JS_EXCEPTION;
    }

    (*dataset)->Set(name, value);

    JS_FreeCString(ctx, name);
    JS_FreeCString(ctx, value);

    return JS_UNDEFINED;
}

// DOMStringMap.get(name)
static JSValue js_dom_string_map_get(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto dataset = static_cast<std::shared_ptr<DOMStringMap>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_string_map_class_id));
    if (!dataset || !*dataset) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "get requires 1 argument");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_EXCEPTION;
    }

    std::string result = (*dataset)->Get(name);
    JS_FreeCString(ctx, name);

    return JS_NewString(ctx, result.c_str());
}

// DOMStringMap.has(name)
static JSValue js_dom_string_map_has(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto dataset = static_cast<std::shared_ptr<DOMStringMap>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_string_map_class_id));
    if (!dataset || !*dataset) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "has requires 1 argument");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_EXCEPTION;
    }

    bool result = (*dataset)->Has(name);
    JS_FreeCString(ctx, name);

    return JS_NewBool(ctx, result);
}

// DOMStringMap.remove(name)
static JSValue js_dom_string_map_remove(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto dataset = static_cast<std::shared_ptr<DOMStringMap>*>(
        JS_GetOpaque2(ctx, this_val, DOMBindings::dom_string_map_class_id));
    if (!dataset || !*dataset) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "remove requires 1 argument");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_EXCEPTION;
    }

    (*dataset)->Remove(name);
    JS_FreeCString(ctx, name);

    return JS_UNDEFINED;
}

// DOMStringMap 类定义
static const JSCFunctionListEntry js_dom_string_map_proto_funcs[] = {
    JS_CFUNC_DEF("set", 2, js_dom_string_map_set),
    JS_CFUNC_DEF("get", 1, js_dom_string_map_get),
    JS_CFUNC_DEF("has", 1, js_dom_string_map_has),
    JS_CFUNC_DEF("remove", 1, js_dom_string_map_remove),
};

void DOMBindings::InitDOMStringMapClass(JSContext* ctx) {
    JSClassDef dom_string_map_class = {
        /* class_name */ "DOMStringMap",
        /* finalizer */ js_dom_string_map_finalizer,
        /* gc_mark */ nullptr,
        /* call */ nullptr,
        /* exotic */ nullptr,
    };

    JS_NewClassID(JS_GetRuntime(ctx), &dom_string_map_class_id);
    JS_NewClass(JS_GetRuntime(ctx), dom_string_map_class_id, &dom_string_map_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_dom_string_map_proto_funcs,
                               sizeof(js_dom_string_map_proto_funcs) / sizeof(js_dom_string_map_proto_funcs[0]));
    JS_SetClassProto(ctx, dom_string_map_class_id, proto);
}

// ========== TaskScheduler 绑定 ==========

void DOMBindings::SetGlobalEventLoop(JSContext* ctx, EventLoop* event_loop) {
    g_event_loop = event_loop;
}

EventLoop* DOMBindings::GetGlobalEventLoop() {
    return g_event_loop;
}

// setTimeout(callback, delay)
static JSValue js_set_timeout(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (!g_task_scheduler) {
        return JS_ThrowInternalError(ctx, "TaskScheduler not initialized");
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setTimeout requires 2 arguments");
    }

    if (!JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "setTimeout requires a function as first argument");
    }

    int delay = 0;
    if (JS_ToInt32(ctx, &delay, argv[1]) != 0) {
        return JS_ThrowTypeError(ctx, "setTimeout requires a number as second argument");
    }

    // 使用 JSValueWrapper 管理回调函数的生命周期
    auto callback_wrapper = std::make_shared<JSValueWrapper>(ctx, argv[0]);

    int timer_id = g_task_scheduler->SetTimeout([ctx, callback_wrapper]() {
        JSValue ret = JS_Call(ctx, callback_wrapper->Get(), JS_UNDEFINED, 0, nullptr);
        if (JS_IsException(ret)) {
            js_std_dump_error(ctx);
        }
        JS_FreeValue(ctx, ret);
    }, delay);

    return JS_NewInt32(ctx, timer_id);
}

// clearTimeout(timerId)
static JSValue js_clear_timeout(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (!g_task_scheduler) {
        return JS_ThrowInternalError(ctx, "TaskScheduler not initialized");
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "clearTimeout requires 1 argument");
    }

    int timer_id = 0;
    if (JS_ToInt32(ctx, &timer_id, argv[0]) != 0) {
        return JS_ThrowTypeError(ctx, "clearTimeout requires a number as argument");
    }

    g_task_scheduler->ClearTimeout(timer_id);
    return JS_UNDEFINED;
}

// setInterval(callback, interval)
static JSValue js_set_interval(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

    if (!g_task_scheduler) {
        return JS_ThrowInternalError(ctx, "TaskScheduler not initialized");
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setInterval requires 2 arguments");
    }

    if (!JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "setInterval requires a function as first argument");
    }

    int interval = 0;
    if (JS_ToInt32(ctx, &interval, argv[1]) != 0) {
        return JS_ThrowTypeError(ctx, "setInterval requires a number as second argument");
    }


    // 使用 JSValueWrapper 管理回调函数的生命周期
    auto callback_wrapper = std::make_shared<JSValueWrapper>(ctx, argv[0]);

    int timer_id = g_task_scheduler->SetInterval([ctx, callback_wrapper]() {
        JSValue ret = JS_Call(ctx, callback_wrapper->Get(), JS_UNDEFINED, 0, nullptr);
        if (JS_IsException(ret)) {
            js_std_dump_error(ctx);
        }
        JS_FreeValue(ctx, ret);
    }, interval);

    return JS_NewInt32(ctx, timer_id);
}

// clearInterval(timerId)
static JSValue js_clear_interval(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (!g_task_scheduler) {
        return JS_ThrowInternalError(ctx, "TaskScheduler not initialized");
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "clearInterval requires 1 argument");
    }

    int timer_id = 0;
    if (JS_ToInt32(ctx, &timer_id, argv[0]) != 0) {
        return JS_ThrowTypeError(ctx, "clearInterval requires a number as argument");
    }

    g_task_scheduler->ClearInterval(timer_id);
    return JS_UNDEFINED;
}

// requestAnimationFrame(callback)
static JSValue js_request_animation_frame(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (!g_task_scheduler) {
        return JS_ThrowInternalError(ctx, "TaskScheduler not initialized");
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "requestAnimationFrame requires 1 argument");
    }

    if (!JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "requestAnimationFrame requires a function as argument");
    }

    // 使用 JSValueWrapper 管理回调函数的生命周期
    auto callback_wrapper = std::make_shared<JSValueWrapper>(ctx, argv[0]);

    int frame_id = g_task_scheduler->RequestAnimationFrame([ctx, callback_wrapper](double timestamp) {
        JSValue timestamp_val = JS_NewFloat64(ctx, timestamp);
        JSValue ret = JS_Call(ctx, callback_wrapper->Get(), JS_UNDEFINED, 1, &timestamp_val);
        JS_FreeValue(ctx, timestamp_val);
        if (JS_IsException(ret)) {
            js_std_dump_error(ctx);
        }
        JS_FreeValue(ctx, ret);
    });

    return JS_NewInt32(ctx, frame_id);
}

// cancelAnimationFrame(frameId)
static JSValue js_cancel_animation_frame(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (!g_task_scheduler) {
        return JS_ThrowInternalError(ctx, "TaskScheduler not initialized");
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "cancelAnimationFrame requires 1 argument");
    }

    int frame_id = 0;
    if (JS_ToInt32(ctx, &frame_id, argv[0]) != 0) {
        return JS_ThrowTypeError(ctx, "cancelAnimationFrame requires a number as argument");
    }

    g_task_scheduler->CancelAnimationFrame(frame_id);
    return JS_UNDEFINED;
}

void DOMBindings::SetGlobalTaskScheduler(JSContext* ctx, std::shared_ptr<TaskScheduler> scheduler) {
    g_task_scheduler = scheduler;

    // 注册全局函数
    JSValue global = JS_GetGlobalObject(ctx);

    JS_SetPropertyStr(ctx, global, "setTimeout", JS_NewCFunction(ctx, js_set_timeout, "setTimeout", 2));
    JS_SetPropertyStr(ctx, global, "clearTimeout", JS_NewCFunction(ctx, js_clear_timeout, "clearTimeout", 1));
    JS_SetPropertyStr(ctx, global, "setInterval", JS_NewCFunction(ctx, js_set_interval, "setInterval", 2));
    JS_SetPropertyStr(ctx, global, "clearInterval", JS_NewCFunction(ctx, js_clear_interval, "clearInterval", 1));
    JS_SetPropertyStr(ctx, global, "requestAnimationFrame", JS_NewCFunction(ctx, js_request_animation_frame, "requestAnimationFrame", 1));
    JS_SetPropertyStr(ctx, global, "cancelAnimationFrame", JS_NewCFunction(ctx, js_cancel_animation_frame, "cancelAnimationFrame", 1));

    JS_FreeValue(ctx, global);
}

// ========== Image 构造函数 ==========

// Image 构造函数: new Image([width], [height])
// 注意：HTMLImageElement 的属性（src, onload, onerror 等）已在新绑定系统 (js_element.cpp) 中实现
static JSValue js_image_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    // 创建 HTMLImageElement
    auto img_element = std::make_shared<HTMLImageElement>();

    // 处理可选的 width 和 height 参数
    if (argc >= 1) {
        uint32_t width = 0;
        if (JS_ToUint32(ctx, &width, argv[0]) == 0) {
            img_element->SetWidth(width);
        }
    }
    if (argc >= 2) {
        uint32_t height = 0;
        if (JS_ToUint32(ctx, &height, argv[1]) == 0) {
            img_element->SetHeight(height);
        }
    }

    // 使用新绑定系统包装为 JS 对象
    // 新绑定系统的 Element 原型已经包含了 src, onload, onerror 等属性
    return bindings::WrapElement(ctx, img_element);
}

// 注册 Image 构造函数到全局对象
void InitImageConstructor(JSContext* ctx) {
    JSValue global = JS_GetGlobalObject(ctx);

    // 创建 Image 构造函数
    JSValue image_ctor = JS_NewCFunction2(ctx, js_image_constructor, "Image", 0, JS_CFUNC_constructor, 0);

    // 设置原型（使用新绑定系统的 Element 原型）
    JSValue proto = JS_GetClassProto(ctx, bindings::GetElementClassID());

    JS_SetConstructor(ctx, image_ctor, proto);
    JS_SetPropertyStr(ctx, global, "Image", image_ctor);

    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global);
}

} // namespace mbink
