/**
 * @file js_selection.cpp
 * @brief Selection 类的 JavaScript 绑定实现
 */

#include "js_selection.h"
#include "js_node.h"
#include "js_range.h"
#include "core/dom/document.h"
#include <iostream>

namespace lightui {
namespace bindings {

// ========== Opaque 数据结构 ==========

struct JSSelectionData {
    std::shared_ptr<Selection> selection;
};

// ========== ClassID ==========

static JSClassID js_selection_class_id = 0;

// ========== 公共函数 ==========

JSClassID GetSelectionClassID() {
    return js_selection_class_id;
}

// ========== 析构函数 ==========

static void JSSelectionFinalizer(JSRuntime* rt, JSValue val) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(val, js_selection_class_id));
    if (data) {
        delete data;
    }
}

// ========== 属性访问器 ==========

// anchorNode
static JSValue JSSelection_get_anchorNode(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_NULL;
    }

    auto node = data->selection->GetAnchorNode();
    if (!node) {
        return JS_NULL;
    }

    return WrapNode(ctx, node);
}

// focusNode
static JSValue JSSelection_get_focusNode(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_NULL;
    }

    auto node = data->selection->GetFocusNode();
    if (!node) {
        return JS_NULL;
    }

    return WrapNode(ctx, node);
}

// anchorOffset
static JSValue JSSelection_get_anchorOffset(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, data->selection->GetAnchorOffset());
}

// focusOffset
static JSValue JSSelection_get_focusOffset(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, data->selection->GetFocusOffset());
}

// isCollapsed
static JSValue JSSelection_get_isCollapsed(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_TRUE;
    }

    return JS_NewBool(ctx, data->selection->IsCollapsed());
}

// rangeCount
static JSValue JSSelection_get_rangeCount(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, data->selection->GetRangeCount());
}

// ========== 方法实现 ==========

// collapse(node, offset)
static JSValue JSSelection_collapse(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_EXCEPTION;
    }

    std::shared_ptr<Node> node = nullptr;
    int32_t offset = 0;

    if (argc >= 1 && !JS_IsNull(argv[0]) && !JS_IsUndefined(argv[0])) {
        node = UnwrapNode(ctx, argv[0]);
    }

    if (argc >= 2) {
        if (JS_ToInt32(ctx, &offset, argv[1]) < 0) {
            return JS_EXCEPTION;
        }
    }

    data->selection->Collapse(node, offset);
    return JS_UNDEFINED;
}

// extend(node, offset)
static JSValue JSSelection_extend(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "extend requires 2 arguments");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "First argument must be a Node");
    }

    int32_t offset;
    if (JS_ToInt32(ctx, &offset, argv[1]) < 0) {
        return JS_EXCEPTION;
    }

    data->selection->Extend(node, offset);
    return JS_UNDEFINED;
}

// selectAllChildren(node)
static JSValue JSSelection_selectAllChildren(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "selectAllChildren requires 1 argument");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "Argument must be a Node");
    }

    data->selection->SelectAllChildren(node);
    return JS_UNDEFINED;
}

// collapseToStart()
static JSValue JSSelection_collapseToStart(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_EXCEPTION;
    }

    data->selection->CollapseToStart();
    return JS_UNDEFINED;
}

// collapseToEnd()
static JSValue JSSelection_collapseToEnd(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_EXCEPTION;
    }

    data->selection->CollapseToEnd();
    return JS_UNDEFINED;
}

// removeAllRanges()
static JSValue JSSelection_removeAllRanges(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_EXCEPTION;
    }

    data->selection->RemoveAllRanges();
    return JS_UNDEFINED;
}

// addRange(range)
static JSValue JSSelection_addRange(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "addRange requires 1 argument");
    }

    auto range = UnwrapRange(ctx, argv[0]);
    if (!range) {
        return JS_ThrowTypeError(ctx, "Argument must be a Range");
    }

    data->selection->AddRange(range);
    return JS_UNDEFINED;
}

// getRangeAt(index)
static JSValue JSSelection_getRangeAt(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getRangeAt requires 1 argument");
    }

    int32_t index;
    if (JS_ToInt32(ctx, &index, argv[0]) < 0) {
        return JS_EXCEPTION;
    }

    auto range = data->selection->GetRangeAt(index);
    if (!range) {
        return JS_ThrowRangeError(ctx, "Index out of range");
    }

    return WrapRange(ctx, range);
}

// toString()
static JSValue JSSelection_toString(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(this_val, js_selection_class_id));
    if (!data || !data->selection) {
        return JS_NewString(ctx, "");
    }

    std::string text = data->selection->ToString();
    return JS_NewString(ctx, text.c_str());
}

// ========== 类定义 ==========

static const JSCFunctionListEntry js_selection_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("anchorNode", JSSelection_get_anchorNode, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("focusNode", JSSelection_get_focusNode, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("anchorOffset", JSSelection_get_anchorOffset, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("focusOffset", JSSelection_get_focusOffset, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("isCollapsed", JSSelection_get_isCollapsed, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("rangeCount", JSSelection_get_rangeCount, nullptr, 0),
    JS_CFUNC_DEF("collapse", 2, JSSelection_collapse),
    JS_CFUNC_DEF("extend", 2, JSSelection_extend),
    JS_CFUNC_DEF("selectAllChildren", 1, JSSelection_selectAllChildren),
    JS_CFUNC_DEF("collapseToStart", 0, JSSelection_collapseToStart),
    JS_CFUNC_DEF("collapseToEnd", 0, JSSelection_collapseToEnd),
    JS_CFUNC_DEF("removeAllRanges", 0, JSSelection_removeAllRanges),
    JS_CFUNC_DEF("addRange", 1, JSSelection_addRange),
    JS_CFUNC_DEF("getRangeAt", 1, JSSelection_getRangeAt),
    JS_CFUNC_DEF("toString", 0, JSSelection_toString),
};

static JSClassDef js_selection_class = {
    /* class_name */ "Selection",
    /* finalizer */ JSSelectionFinalizer,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr,
};

// ========== 公共 API ==========

void InitSelectionBinding(JSContext* ctx) {
    // 创建 ClassID
    JS_NewClassID(JS_GetRuntime(ctx), &js_selection_class_id);

    // 注册类
    JS_NewClass(JS_GetRuntime(ctx), js_selection_class_id, &js_selection_class);

    // 创建原型对象
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_selection_proto_funcs,
                               sizeof(js_selection_proto_funcs) / sizeof(js_selection_proto_funcs[0]));

    // 设置类的原型
    JS_SetClassProto(ctx, js_selection_class_id, proto);
}

JSValue WrapSelection(JSContext* ctx, std::shared_ptr<Selection> selection) {
    if (!selection) {
        return JS_NULL;
    }

    // 创建新的 JS 对象
    JSValue obj = JS_NewObjectClass(ctx, js_selection_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    // 设置 opaque 数据
    auto* data = new JSSelectionData();
    data->selection = selection;
    JS_SetOpaque(obj, data);

    return obj;
}

std::shared_ptr<Selection> UnwrapSelection(JSContext* ctx, JSValue value) {
    auto* data = static_cast<JSSelectionData*>(JS_GetOpaque(value, js_selection_class_id));
    if (data) {
        return data->selection;
    }
    return nullptr;
}

} // namespace bindings
} // namespace lightui
