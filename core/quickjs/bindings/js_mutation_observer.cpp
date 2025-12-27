/**
 * @file js_mutation_observer.cpp
 * @brief MutationObserver 类的 JavaScript 绑定实现
 */

#include "js_mutation_observer.h"
#include "js_node.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include <iostream>
#include <vector>

namespace lightui {
namespace bindings {

// ========== Opaque 数据结构 ==========

struct JSMutationObserverData {
    std::shared_ptr<MutationObserver> observer;
    JSContext* ctx = nullptr;
    JSValue callback = JS_UNDEFINED;
};

// ========== ClassID ==========

static JSClassID js_mutation_observer_class_id = 0;
static JSClassID js_mutation_record_class_id = 0;

JSClassID GetMutationObserverClassID() {
    return js_mutation_observer_class_id;
}

JSClassID GetMutationRecordClassID() {
    return js_mutation_record_class_id;
}

// ========== MutationObserver Destructor ==========

static void js_mutation_observer_finalizer(JSRuntime* rt, JSValue val) {
    auto* data = static_cast<JSMutationObserverData*>(
        JS_GetOpaque(val, js_mutation_observer_class_id));
    if (data) {
        if (!JS_IsUndefined(data->callback)) {
            JS_FreeValueRT(rt, data->callback);
        }
        delete data;
    }
}

static JSClassDef js_mutation_observer_class = {
    /* class_name */ "MutationObserver",
    /* finalizer */ js_mutation_observer_finalizer,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr
};

// ========== MutationRecord Class ==========

static JSClassDef js_mutation_record_class = {
    /* class_name */ "MutationRecord",
    /* finalizer */ nullptr,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr
};

// ========== Helper Functions ==========

static MutationObserverInit ParseObserverInit(JSContext* ctx, JSValue options) {
    MutationObserverInit init;

    if (JS_IsUndefined(options) || JS_IsNull(options)) {
        return init;
    }

    JSValue val;

    // childList
    val = JS_GetPropertyStr(ctx, options, "childList");
    if (!JS_IsUndefined(val)) {
        init.child_list = JS_ToBool(ctx, val);
    }
    JS_FreeValue(ctx, val);

    // attributes
    val = JS_GetPropertyStr(ctx, options, "attributes");
    if (!JS_IsUndefined(val)) {
        init.attributes = JS_ToBool(ctx, val);
    }
    JS_FreeValue(ctx, val);

    // characterData
    val = JS_GetPropertyStr(ctx, options, "characterData");
    if (!JS_IsUndefined(val)) {
        init.character_data = JS_ToBool(ctx, val);
    }
    JS_FreeValue(ctx, val);

    // subtree
    val = JS_GetPropertyStr(ctx, options, "subtree");
    if (!JS_IsUndefined(val)) {
        init.subtree = JS_ToBool(ctx, val);
    }
    JS_FreeValue(ctx, val);

    // attributeOldValue
    val = JS_GetPropertyStr(ctx, options, "attributeOldValue");
    if (!JS_IsUndefined(val)) {
        init.attribute_old_value = JS_ToBool(ctx, val);
        if (init.attribute_old_value && !init.attributes) {
            init.attributes = true;
        }
    }
    JS_FreeValue(ctx, val);

    // characterDataOldValue
    val = JS_GetPropertyStr(ctx, options, "characterDataOldValue");
    if (!JS_IsUndefined(val)) {
        init.character_data_old_value = JS_ToBool(ctx, val);
        if (init.character_data_old_value && !init.character_data) {
            init.character_data = true;
        }
    }
    JS_FreeValue(ctx, val);

    // attributeFilter
    val = JS_GetPropertyStr(ctx, options, "attributeFilter");
    if (JS_IsArray(ctx, val) > 0) {
        uint32_t len = 0;
        JSValue lenVal = JS_GetPropertyStr(ctx, val, "length");
        JS_ToUint32(ctx, &len, lenVal);
        JS_FreeValue(ctx, lenVal);

        for (uint32_t i = 0; i < len; i++) {
            JSValue item = JS_GetPropertyUint32(ctx, val, i);
            const char* str = JS_ToCString(ctx, item);
            if (str) {
                init.attribute_filter.push_back(str);
                JS_FreeCString(ctx, str);
            }
            JS_FreeValue(ctx, item);
        }

        if (!init.attribute_filter.empty() && !init.attributes) {
            init.attributes = true;
        }
    }
    JS_FreeValue(ctx, val);

    return init;
}


// ========== MutationRecord to JS ==========

JSValue MutationRecordToJS(JSContext* ctx, const MutationRecord& record) {
    JSValue obj = JS_NewObject(ctx);

    // type
    JS_SetPropertyStr(ctx, obj, "type",
        JS_NewString(ctx, record.type.c_str()));

    // target
    auto target = record.target.lock();
    if (target) {
        JS_SetPropertyStr(ctx, obj, "target", WrapNode(ctx, target));
    } else {
        JS_SetPropertyStr(ctx, obj, "target", JS_NULL);
    }

    // addedNodes
    JSValue addedNodes = JS_NewArray(ctx);
    for (size_t i = 0; i < record.added_nodes.size(); i++) {
        JS_SetPropertyUint32(ctx, addedNodes, i,
            WrapNode(ctx, record.added_nodes[i]));
    }
    JS_SetPropertyStr(ctx, obj, "addedNodes", addedNodes);

    // removedNodes
    JSValue removedNodes = JS_NewArray(ctx);
    for (size_t i = 0; i < record.removed_nodes.size(); i++) {
        JS_SetPropertyUint32(ctx, removedNodes, i,
            WrapNode(ctx, record.removed_nodes[i]));
    }
    JS_SetPropertyStr(ctx, obj, "removedNodes", removedNodes);

    // previousSibling
    auto prevSibling = record.previous_sibling.lock();
    if (prevSibling) {
        JS_SetPropertyStr(ctx, obj, "previousSibling", WrapNode(ctx, prevSibling));
    } else {
        JS_SetPropertyStr(ctx, obj, "previousSibling", JS_NULL);
    }

    // nextSibling
    auto nextSibling = record.next_sibling.lock();
    if (nextSibling) {
        JS_SetPropertyStr(ctx, obj, "nextSibling", WrapNode(ctx, nextSibling));
    } else {
        JS_SetPropertyStr(ctx, obj, "nextSibling", JS_NULL);
    }

    // attributeName
    if (!record.attribute_name.empty()) {
        JS_SetPropertyStr(ctx, obj, "attributeName",
            JS_NewString(ctx, record.attribute_name.c_str()));
    } else {
        JS_SetPropertyStr(ctx, obj, "attributeName", JS_NULL);
    }

    // attributeNamespace
    if (!record.attribute_namespace.empty()) {
        JS_SetPropertyStr(ctx, obj, "attributeNamespace",
            JS_NewString(ctx, record.attribute_namespace.c_str()));
    } else {
        JS_SetPropertyStr(ctx, obj, "attributeNamespace", JS_NULL);
    }

    // oldValue
    if (!record.old_value.empty()) {
        JS_SetPropertyStr(ctx, obj, "oldValue",
            JS_NewString(ctx, record.old_value.c_str()));
    } else {
        JS_SetPropertyStr(ctx, obj, "oldValue", JS_NULL);
    }

    return obj;
}

// ========== MutationObserver Methods ==========

// observe(target, options)
static JSValue js_mutation_observer_observe(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

    auto* data = static_cast<JSMutationObserverData*>(
        JS_GetOpaque(this_val, js_mutation_observer_class_id));
    if (!data || !data->observer) {
        return JS_ThrowTypeError(ctx, "Invalid MutationObserver");
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "observe requires at least 1 argument");
    }

    // 获取目标节点
    auto target = UnwrapNode(ctx, argv[0]);
    if (!target) {
        return JS_ThrowTypeError(ctx, "First argument must be a Node");
    }

    // 解析选项
    MutationObserverInit options;
    if (argc >= 2) {
        options = ParseObserverInit(ctx, argv[1]);
    }

    if (!options.IsValid()) {
        return JS_ThrowTypeError(ctx,
            "At least one of childList, attributes, or characterData must be true");
    }

    try {
        data->observer->Observe(target, options);
    } catch (const std::exception& e) {
        return JS_ThrowTypeError(ctx, "%s", e.what());
    }

    return JS_UNDEFINED;
}

// disconnect()
static JSValue js_mutation_observer_disconnect(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

    auto* data = static_cast<JSMutationObserverData*>(
        JS_GetOpaque(this_val, js_mutation_observer_class_id));
    if (!data || !data->observer) {
        return JS_ThrowTypeError(ctx, "Invalid MutationObserver");
    }

    data->observer->Disconnect();
    return JS_UNDEFINED;
}

// takeRecords()
static JSValue js_mutation_observer_take_records(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

    auto* data = static_cast<JSMutationObserverData*>(
        JS_GetOpaque(this_val, js_mutation_observer_class_id));
    if (!data || !data->observer) {
        return JS_ThrowTypeError(ctx, "Invalid MutationObserver");
    }

    auto records = data->observer->TakeRecords();

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < records.size(); i++) {
        JS_SetPropertyUint32(ctx, arr, i, MutationRecordToJS(ctx, records[i]));
    }

    return arr;
}

// ========== MutationObserver Constructor ==========

static JSValue js_mutation_observer_constructor(
    JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {

    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "MutationObserver requires a callback function");
    }

    // 创建 JS 对象
    JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto)) {
        return proto;
    }

    JSValue obj = JS_NewObjectProtoClass(ctx, proto, js_mutation_observer_class_id);
    JS_FreeValue(ctx, proto);

    if (JS_IsException(obj)) {
        return obj;
    }

    // 创建数据
    auto* data = new JSMutationObserverData();
    data->ctx = ctx;
    data->callback = JS_DupValue(ctx, argv[0]);

    // 创建 C++ MutationObserver，设置回调
    data->observer = std::make_shared<MutationObserver>(
        [data](const std::vector<MutationRecord>& records, MutationObserver* observer) {
            if (JS_IsUndefined(data->callback)) {
                return;
            }

            JSContext* ctx = data->ctx;

            // 创建 records 数组
            JSValue recordsArr = JS_NewArray(ctx);
            for (size_t i = 0; i < records.size(); i++) {
                JS_SetPropertyUint32(ctx, recordsArr, i,
                    MutationRecordToJS(ctx, records[i]));
            }

            // 创建 observer 参数（this_val）
            JSValue observerVal = JS_UNDEFINED;  // 简化处理

            // 调用回调
            JSValue args[2] = { recordsArr, observerVal };
            JSValue result = JS_Call(ctx, data->callback, JS_UNDEFINED, 2, args);

            JS_FreeValue(ctx, recordsArr);
            JS_FreeValue(ctx, result);
        });

    JS_SetOpaque(obj, data);
    return obj;
}

// ========== Method Table ==========

static const JSCFunctionListEntry js_mutation_observer_proto_funcs[] = {
    JS_CFUNC_DEF("observe", 2, js_mutation_observer_observe),
    JS_CFUNC_DEF("disconnect", 0, js_mutation_observer_disconnect),
    JS_CFUNC_DEF("takeRecords", 0, js_mutation_observer_take_records),
};

// ========== Initialization ==========

void InitMutationObserverBinding(JSContext* ctx) {
    JSRuntime* rt = JS_GetRuntime(ctx);

    // 注册 MutationObserver 类
    JS_NewClassID(&js_mutation_observer_class_id);
    JS_NewClass(rt, js_mutation_observer_class_id, &js_mutation_observer_class);

    // 注册 MutationRecord 类
    JS_NewClassID(&js_mutation_record_class_id);
    JS_NewClass(rt, js_mutation_record_class_id, &js_mutation_record_class);

    // 创建 MutationObserver 原型
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_mutation_observer_proto_funcs,
        sizeof(js_mutation_observer_proto_funcs) / sizeof(js_mutation_observer_proto_funcs[0]));
    JS_SetClassProto(ctx, js_mutation_observer_class_id, proto);

    // 创建 MutationObserver 构造函数
    JSValue ctor = JS_NewCFunction2(ctx, js_mutation_observer_constructor,
        "MutationObserver", 1, JS_CFUNC_constructor, 0);

    // 注册到全局对象
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "MutationObserver", ctor);
    JS_FreeValue(ctx, global);
}

// ========== Wrap/Unwrap Functions ==========

JSValue WrapMutationObserver(JSContext* ctx, std::shared_ptr<MutationObserver> observer) {
    if (!observer) {
        return JS_NULL;
    }

    JSValue obj = JS_NewObjectClass(ctx, js_mutation_observer_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto* data = new JSMutationObserverData();
    data->observer = observer;
    data->ctx = ctx;
    data->callback = JS_UNDEFINED;

    JS_SetOpaque(obj, data);
    return obj;
}

std::shared_ptr<MutationObserver> UnwrapMutationObserver(JSContext* ctx, JSValue value) {
    auto* data = static_cast<JSMutationObserverData*>(
        JS_GetOpaque(value, js_mutation_observer_class_id));
    if (data) {
        return data->observer;
    }
    return nullptr;
}

} // namespace bindings
} // namespace lightui
