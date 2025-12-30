/**
 * @file js_node.cpp
 * @brief Node 类的 JavaScript 绑定实现
 */

#include "js_node.h"
#include "js_element.h"
#include "core/quickjs/dom_binding_map.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include <iostream>
#include <cctype>

namespace lightui {
namespace bindings {

// ========== Opaque 数据结构 ==========

struct JSNodeData {
    std::shared_ptr<Node> node;
};

// ========== ClassID ==========

static JSClassID js_node_class_id = 0;

// ========== 公共函数 ==========

JSClassID GetNodeClassID() {
    return js_node_class_id;
}

// ========== 析构函数 ==========

static void JSNodeFinalizer(JSRuntime* rt, JSValue val) {
    auto* data = static_cast<JSNodeData*>(JS_GetOpaque(val, js_node_class_id));
    if (data) {
        // 从映射表中移除
        if (data->node) {
            DOMBindingMap::GetInstance().Remove(data->node.get());
        }
        delete data;
    }
}

// ========== 属性访问器 ==========

// parentNode
static JSValue JSNode_get_parentNode(JSContext* ctx, JSValueConst this_val, int magic) {
    // 支持 Element 对象（Element 继承自 Node）
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NULL;
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        return JS_NULL;
    }

    return WrapNode(ctx, parent);
}

// firstChild  
static JSValue JSNode_get_firstChild(JSContext* ctx, JSValueConst this_val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NULL;
    }

    auto first_child = node->GetFirstChild();
    if (!first_child) {
        return JS_NULL;
    }

    return WrapNode(ctx, first_child);
}

// nextSibling
static JSValue JSNode_get_nextSibling(JSContext* ctx, JSValueConst this_val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NULL;
    }

    auto next_sibling = node->GetNextSibling();
    if (!next_sibling) {
        return JS_NULL;
    }

    return WrapNode(ctx, next_sibling);
}

// lastChild
static JSValue JSNode_get_lastChild(JSContext* ctx, JSValueConst this_val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NULL;
    }

    auto last_child = node->GetLastChild();
    if (!last_child) {
        return JS_NULL;
    }

    return WrapNode(ctx, last_child);
}

// previousSibling
static JSValue JSNode_get_previousSibling(JSContext* ctx, JSValueConst this_val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NULL;
    }

    auto prev_sibling = node->GetPreviousSibling();
    if (!prev_sibling) {
        return JS_NULL;
    }

    return WrapNode(ctx, prev_sibling);
}

// textContent getter
static JSValue JSNode_get_textContent(JSContext* ctx, JSValueConst this_val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NULL;
    }

    std::string text = node->GetTextContent();
    return JS_NewString(ctx, text.c_str());
}

// nodeName getter
static JSValue JSNode_get_nodeName(JSContext* ctx, JSValueConst this_val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NULL;
    }

    // 对于 Element，返回大写的标签名
    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element) {
        std::string tag_name = element->GetTagName();
        // 转换为大写
        for (auto& c : tag_name) {
            c = std::toupper(c);
        }
        return JS_NewString(ctx, tag_name.c_str());
    }
    
    // 对于 Text 节点，返回 "#text"
    if (node->GetNodeType() == NodeType::TEXT_NODE) {
        return JS_NewString(ctx, "#text");
    }
    
    // 对于 Document 节点，返回 "#document"
    if (node->GetNodeType() == NodeType::DOCUMENT_NODE) {
        return JS_NewString(ctx, "#document");
    }
    
    return JS_NewString(ctx, "");
}

// nodeType getter
static JSValue JSNode_get_nodeType(JSContext* ctx, JSValueConst this_val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NewInt32(ctx, 0);
    }

    // 返回 DOM 标准的 nodeType 值
    // ELEMENT_NODE = 1, TEXT_NODE = 3, DOCUMENT_NODE = 9
    int type = 0;
    switch (node->GetNodeType()) {
        case NodeType::ELEMENT_NODE:
            type = 1;
            break;
        case NodeType::TEXT_NODE:
            type = 3;
            break;
        case NodeType::DOCUMENT_NODE:
            type = 9;
            break;
        default:
            type = 0;
            break;
    }
    return JS_NewInt32(ctx, type);
}

// nodeValue getter - 对于文本节点返回文本内容，对于元素节点返回 null
static JSValue JSNode_get_nodeValue(JSContext* ctx, JSValueConst this_val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NULL;
    }

    // 对于文本节点，返回文本内容
    if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(node);
        if (text_node) {
            return JS_NewString(ctx, text_node->GetData().c_str());
        }
        // 回退到 textContent
        return JS_NewString(ctx, node->GetTextContent().c_str());
    }
    
    // 对于元素节点和文档节点，返回 null
    return JS_NULL;
}

// nodeValue setter - 对于文本节点设置文本内容
static JSValue JSNode_set_nodeValue(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_UNDEFINED;
    }

    // 只有文本节点可以设置 nodeValue
    if (node->GetNodeType() == NodeType::TEXT_NODE) {
        const char* str = JS_ToCString(ctx, val);
        if (!str) {
            return JS_EXCEPTION;
        }
        
        auto text_node = std::dynamic_pointer_cast<Text>(node);
        if (text_node) {
            text_node->SetData(str);
        } else {
            node->SetTextContent(str);
        }
        JS_FreeCString(ctx, str);
    }
    
    return JS_UNDEFINED;
}

// textContent setter
static JSValue JSNode_set_textContent(JSContext* ctx, JSValueConst this_val, JSValue val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_UNDEFINED;
    }

    const char* str = JS_ToCString(ctx, val);
    if (!str) {
        return JS_EXCEPTION;
    }

    node->SetTextContent(str);
    JS_FreeCString(ctx, str);

    return JS_UNDEFINED;
}

// childNodes getter - 返回一个包含所有子节点的 NodeList (简化为 Array)
static JSValue JSNode_get_childNodes(JSContext* ctx, JSValueConst this_val, int magic) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_NewArray(ctx);  // 返回空数组而不是 null
    }

    // 获取所有子节点
    JSValue arr = JS_NewArray(ctx);
    uint32_t index = 0;
    
    // 遍历所有子节点
    for (auto child = node->GetFirstChild(); child; child = child->GetNextSibling()) {
        JSValue child_val = WrapNode(ctx, child);
        JS_SetPropertyUint32(ctx, arr, index++, child_val);
    }

    return arr;
}

// ========== 方法实现 ==========

// appendChild(child)
static JSValue JSNode_appendChild(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "appendChild requires 1 argument");
    }

    auto child = UnwrapNode(ctx, argv[0]);
    if (!child) {
        return JS_ThrowTypeError(ctx, "Argument must be a Node");
    }

    try {
        node->AppendChild(child);
        return JS_DupValue(ctx, argv[0]);
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "appendChild failed: %s", e.what());
    }
}

// removeChild(child)
static JSValue JSNode_removeChild(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "removeChild requires 1 argument");
    }

    auto child = UnwrapNode(ctx, argv[0]);
    if (!child) {
        return JS_ThrowTypeError(ctx, "Argument must be a Node");
    }

    try {
        node->RemoveChild(child);
        return JS_DupValue(ctx, argv[0]);
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "removeChild failed: %s", e.what());
    }
}

// insertBefore(newNode, refNode)
static JSValue JSNode_insertBefore(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "insertBefore requires 2 arguments");
    }

    auto new_node = UnwrapNode(ctx, argv[0]);
    if (!new_node) {
        return JS_ThrowTypeError(ctx, "First argument must be a Node");
    }

    std::shared_ptr<Node> ref_node;
    if (!JS_IsNull(argv[1]) && !JS_IsUndefined(argv[1])) {
        ref_node = UnwrapNode(ctx, argv[1]);
        if (!ref_node) {
            return JS_ThrowTypeError(ctx, "Second argument must be a Node or null");
        }
    }

    try {
        node->InsertBefore(new_node, ref_node);
        return JS_DupValue(ctx, argv[0]);
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "insertBefore failed: %s", e.what());
    }
}

// replaceChild(newNode, oldNode)
static JSValue JSNode_replaceChild(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "replaceChild requires 2 arguments");
    }

    auto new_node = UnwrapNode(ctx, argv[0]);
    if (!new_node) {
        return JS_ThrowTypeError(ctx, "First argument must be a Node");
    }

    auto old_node = UnwrapNode(ctx, argv[1]);
    if (!old_node) {
        return JS_ThrowTypeError(ctx, "Second argument must be a Node");
    }

    try {
        node->ReplaceChild(new_node, old_node);
        return JS_DupValue(ctx, argv[1]);
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "replaceChild failed: %s", e.what());
    }
}

// remove() - DOM4 方法，从 DOM 中移除节点
static JSValue JSNode_remove(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto node = UnwrapNode(ctx, this_val);
    if (!node) {
        return JS_UNDEFINED;
    }

    auto parent = node->GetParentNode();
    if (parent) {
        parent->RemoveChild(node);
    }

    return JS_UNDEFINED;
}

// ========== 类定义 ==========

static const JSCFunctionListEntry js_node_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("parentNode", JSNode_get_parentNode, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("firstChild", JSNode_get_firstChild, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("lastChild", JSNode_get_lastChild, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("nextSibling", JSNode_get_nextSibling, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("previousSibling", JSNode_get_previousSibling, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("textContent", JSNode_get_textContent, JSNode_set_textContent, 0),
    JS_CGETSET_MAGIC_DEF("childNodes", JSNode_get_childNodes, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("nodeName", JSNode_get_nodeName, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("nodeType", JSNode_get_nodeType, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("nodeValue", JSNode_get_nodeValue, JSNode_set_nodeValue, 0),
    JS_CFUNC_DEF("appendChild", 1, JSNode_appendChild),
    JS_CFUNC_DEF("removeChild", 1, JSNode_removeChild),
    JS_CFUNC_DEF("insertBefore", 2, JSNode_insertBefore),
    JS_CFUNC_DEF("replaceChild", 2, JSNode_replaceChild),
    JS_CFUNC_DEF("remove", 0, JSNode_remove),
};

static JSClassDef js_node_class = {
    /* class_name */ "Node",
    /* finalizer */ JSNodeFinalizer,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr,
};

// ========== 公共 API ==========

void InitNodeBinding(JSContext* ctx) {
    // 创建 ClassID
    JS_NewClassID(JS_GetRuntime(ctx), &js_node_class_id);

    // 注册类
    JS_NewClass(JS_GetRuntime(ctx), js_node_class_id, &js_node_class);

    // 创建原型对象
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_node_proto_funcs, 
                               sizeof(js_node_proto_funcs) / sizeof(js_node_proto_funcs[0]));

    // 设置类的原型
    JS_SetClassProto(ctx, js_node_class_id, proto);
}

JSValue WrapNode(JSContext* ctx, std::shared_ptr<Node> node) {
    if (!node) {
        return JS_NULL;
    }

    // 检查是否已经包装过（引用相等性）
    auto& map = DOMBindingMap::GetInstance();
    if (map.Has(node.get())) {
        JSValue existing = map.GetJSValue(node.get());
        return JS_DupValue(ctx, existing);
    }

    // 创建新的 JS 对象
    JSValue obj = JS_NewObjectClass(ctx, js_node_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    // 设置 opaque 数据
    auto* data = new JSNodeData();
    data->node = node;
    JS_SetOpaque(obj, data);

    // 记录到映射表
    map.SetJSValue(node.get(), obj, ctx);

    return obj;
}

std::shared_ptr<Node> UnwrapNode(JSContext* ctx, JSValue value) {
    // 首先尝试作为 Node
    auto* node_data = static_cast<JSNodeData*>(JS_GetOpaque(value, js_node_class_id));
    if (node_data) {
        return node_data->node;
    }
    
    // 如果不是 Node，尝试作为 Element（Element 继承自 Node）
    auto element = UnwrapElement(ctx, value);
    if (element) {
        // Element 是 Node 的子类，可强直接返回
        return std::static_pointer_cast<Node>(element);
    }
    
    return nullptr;
}

} // namespace bindings
} // namespace lightui
