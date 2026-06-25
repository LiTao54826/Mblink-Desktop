/**
 * @file js_range.cpp
 * @brief Range 类的 JavaScript 绑定实现
 */

#include "js_range.h"
#include "js_node.h"
#include "core/dom/document.h"
#include <iostream>

namespace mblink {
namespace bindings {

// ========== Opaque 数据结构 ==========

struct JSRangeData {
    std::shared_ptr<Range> range;
};

// ========== ClassID ==========

static JSClassID js_range_class_id = 0;

// ========== 公共函数 ==========

JSClassID GetRangeClassID() {
    return js_range_class_id;
}

// ========== 析构函数 ==========

static void JSRangeFinalizer(JSRuntime* rt, JSValue val) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(val, js_range_class_id));
    if (data) {
        delete data;
    }
}

// ========== 属性访问器 ==========

// startContainer
static JSValue JSRange_get_startContainer(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_NULL;
    }

    auto node = data->range->GetStartContainer();
    if (!node) {
        return JS_NULL;
    }

    return WrapNode(ctx, node);
}

// endContainer
static JSValue JSRange_get_endContainer(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_NULL;
    }

    auto node = data->range->GetEndContainer();
    if (!node) {
        return JS_NULL;
    }

    return WrapNode(ctx, node);
}

// startOffset
static JSValue JSRange_get_startOffset(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, data->range->GetStartOffset());
}

// endOffset
static JSValue JSRange_get_endOffset(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, data->range->GetEndOffset());
}

// collapsed
static JSValue JSRange_get_collapsed(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_TRUE;
    }

    return JS_NewBool(ctx, data->range->IsCollapsed());
}

// commonAncestorContainer
static JSValue JSRange_get_commonAncestorContainer(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_NULL;
    }

    auto node = data->range->GetCommonAncestorContainer();
    if (!node) {
        return JS_NULL;
    }

    return WrapNode(ctx, node);
}

// ========== 方法实现 ==========

// setStart(node, offset)
static JSValue JSRange_setStart(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setStart requires 2 arguments");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "First argument must be a Node");
    }

    int32_t offset;
    if (JS_ToInt32(ctx, &offset, argv[1]) < 0) {
        return JS_EXCEPTION;
    }

    try {
        data->range->SetStart(node, offset);
        return JS_UNDEFINED;
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "setStart failed: %s", e.what());
    }
}

// setEnd(node, offset)
static JSValue JSRange_setEnd(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setEnd requires 2 arguments");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "First argument must be a Node");
    }

    int32_t offset;
    if (JS_ToInt32(ctx, &offset, argv[1]) < 0) {
        return JS_EXCEPTION;
    }

    try {
        data->range->SetEnd(node, offset);
        return JS_UNDEFINED;
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "setEnd failed: %s", e.what());
    }
}

// setStartBefore(node)
static JSValue JSRange_setStartBefore(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "setStartBefore requires 1 argument");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "Argument must be a Node");
    }

    try {
        data->range->SetStartBefore(node);
        return JS_UNDEFINED;
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "setStartBefore failed: %s", e.what());
    }
}

// setStartAfter(node)
static JSValue JSRange_setStartAfter(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "setStartAfter requires 1 argument");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "Argument must be a Node");
    }

    try {
        data->range->SetStartAfter(node);
        return JS_UNDEFINED;
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "setStartAfter failed: %s", e.what());
    }
}

// setEndBefore(node)
static JSValue JSRange_setEndBefore(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "setEndBefore requires 1 argument");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "Argument must be a Node");
    }

    try {
        data->range->SetEndBefore(node);
        return JS_UNDEFINED;
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "setEndBefore failed: %s", e.what());
    }
}

// setEndAfter(node)
static JSValue JSRange_setEndAfter(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "setEndAfter requires 1 argument");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "Argument must be a Node");
    }

    try {
        data->range->SetEndAfter(node);
        return JS_UNDEFINED;
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "setEndAfter failed: %s", e.what());
    }
}

// selectNode(node)
static JSValue JSRange_selectNode(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "selectNode requires 1 argument");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "Argument must be a Node");
    }

    try {
        data->range->SelectNode(node);
        return JS_UNDEFINED;
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "selectNode failed: %s", e.what());
    }
}

// selectNodeContents(node)
static JSValue JSRange_selectNodeContents(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "selectNodeContents requires 1 argument");
    }

    auto node = UnwrapNode(ctx, argv[0]);
    if (!node) {
        return JS_ThrowTypeError(ctx, "Argument must be a Node");
    }

    try {
        data->range->SelectNodeContents(node);
        return JS_UNDEFINED;
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "selectNodeContents failed: %s", e.what());
    }
}

// collapse(toStart)
static JSValue JSRange_collapse(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_EXCEPTION;
    }

    bool to_start = true;
    if (argc >= 1) {
        to_start = JS_ToBool(ctx, argv[0]);
    }

    data->range->Collapse(to_start);
    return JS_UNDEFINED;
}

// cloneRange()
static JSValue JSRange_cloneRange(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_NULL;
    }

    auto cloned = data->range->CloneRange();
    return WrapRange(ctx, cloned);
}

// toString()
static JSValue JSRange_toString(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_NewString(ctx, "");
    }

    std::string text = data->range->ToString();
    return JS_NewString(ctx, text.c_str());
}

// getBoundingClientRect()
static JSValue JSRange_getBoundingClientRect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    if (!data || !data->range) {
        return JS_NULL;
    }

    auto rect = data->range->GetBoundingClientRect();
    
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

// getClientRects() - 返回 Range 的所有边界矩形
static JSValue JSRange_getClientRects(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(this_val, js_range_class_id));
    
    // 创建数组来存储矩形
    JSValue arr = JS_NewArray(ctx);
    
    if (!data || !data->range) {
        return arr;  // 返回空数组
    }

    auto rects = data->range->GetClientRects();
    
    for (size_t i = 0; i < rects.size(); ++i) {
        const auto& rect = rects[i];
        
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
        JS_SetPropertyUint32(ctx, arr, static_cast<uint32_t>(i), obj);
    }
    
    return arr;
}

// ========== 类定义 ==========

static const JSCFunctionListEntry js_range_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("startContainer", JSRange_get_startContainer, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("endContainer", JSRange_get_endContainer, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("startOffset", JSRange_get_startOffset, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("endOffset", JSRange_get_endOffset, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("collapsed", JSRange_get_collapsed, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("commonAncestorContainer", JSRange_get_commonAncestorContainer, nullptr, 0),
    JS_CFUNC_DEF("setStart", 2, JSRange_setStart),
    JS_CFUNC_DEF("setEnd", 2, JSRange_setEnd),
    JS_CFUNC_DEF("setStartBefore", 1, JSRange_setStartBefore),
    JS_CFUNC_DEF("setStartAfter", 1, JSRange_setStartAfter),
    JS_CFUNC_DEF("setEndBefore", 1, JSRange_setEndBefore),
    JS_CFUNC_DEF("setEndAfter", 1, JSRange_setEndAfter),
    JS_CFUNC_DEF("selectNode", 1, JSRange_selectNode),
    JS_CFUNC_DEF("selectNodeContents", 1, JSRange_selectNodeContents),
    JS_CFUNC_DEF("collapse", 1, JSRange_collapse),
    JS_CFUNC_DEF("cloneRange", 0, JSRange_cloneRange),
    JS_CFUNC_DEF("toString", 0, JSRange_toString),
    JS_CFUNC_DEF("getBoundingClientRect", 0, JSRange_getBoundingClientRect),
    JS_CFUNC_DEF("getClientRects", 0, JSRange_getClientRects),
};

static JSClassDef js_range_class = {
    /* class_name */ "Range",
    /* finalizer */ JSRangeFinalizer,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr,
};

// ========== 公共 API ==========

void InitRangeBinding(JSContext* ctx) {
    // 创建 ClassID
    JS_NewClassID(JS_GetRuntime(ctx), &js_range_class_id);

    // 注册类
    JS_NewClass(JS_GetRuntime(ctx), js_range_class_id, &js_range_class);

    // 创建原型对象
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_range_proto_funcs,
                               sizeof(js_range_proto_funcs) / sizeof(js_range_proto_funcs[0]));

    // 设置类的原型
    JS_SetClassProto(ctx, js_range_class_id, proto);
}

JSValue WrapRange(JSContext* ctx, std::shared_ptr<Range> range) {
    if (!range) {
        return JS_NULL;
    }

    // 创建新的 JS 对象
    JSValue obj = JS_NewObjectClass(ctx, js_range_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    // 设置 opaque 数据
    auto* data = new JSRangeData();
    data->range = range;
    JS_SetOpaque(obj, data);

    return obj;
}

std::shared_ptr<Range> UnwrapRange(JSContext* ctx, JSValue value) {
    auto* data = static_cast<JSRangeData*>(JS_GetOpaque(value, js_range_class_id));
    if (data) {
        return data->range;
    }
    return nullptr;
}

} // namespace bindings
} // namespace mblink
