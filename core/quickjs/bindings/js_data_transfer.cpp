/**
 * @file js_data_transfer.cpp
 * @brief DataTransfer 类的 JavaScript 绑定实现
 *
 * 实现 W3C DataTransfer 接口：
 * - setData(format, data) - 设置数据
 * - getData(format) - 获取数据
 * - clearData(format?) - 清除数据
 * - types - 获取所有数据格式
 * - effectAllowed - 允许的拖拽效果
 * - dropEffect - 当前拖拽效果
 */

#include "js_data_transfer.h"
#include "js_file_list.h"
#include "core/event/types/data_transfer.h"
#include <iostream>

namespace mblink {
namespace bindings {

// ========== Opaque 数据结构 ==========

struct JSDataTransferData {
    std::shared_ptr<DataTransfer> data_transfer;
};

// ========== ClassID ==========

static JSClassID js_data_transfer_class_id = 0;

// ========== 析构函数 ==========

static void JSDataTransferFinalizer(JSRuntime* rt, JSValue val) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(val, js_data_transfer_class_id));
    if (data) {
        delete data;
    }
}

// ========== 方法实现 ==========

// setData(format, data)
static JSValue JSDataTransfer_setData(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(this_val, js_data_transfer_class_id));
    if (!data || !data->data_transfer) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "setData requires 2 arguments");
    }

    const char* format = JS_ToCString(ctx, argv[0]);
    if (!format) {
        return JS_EXCEPTION;
    }

    const char* value = JS_ToCString(ctx, argv[1]);
    if (!value) {
        JS_FreeCString(ctx, format);
        return JS_EXCEPTION;
    }

    data->data_transfer->SetData(format, value);

    JS_FreeCString(ctx, format);
    JS_FreeCString(ctx, value);

    return JS_UNDEFINED;
}

// getData(format)
static JSValue JSDataTransfer_getData(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(this_val, js_data_transfer_class_id));
    if (!data || !data->data_transfer) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getData requires 1 argument");
    }

    const char* format = JS_ToCString(ctx, argv[0]);
    if (!format) {
        return JS_EXCEPTION;
    }

    std::string result = data->data_transfer->GetData(format);
    JS_FreeCString(ctx, format);

    return JS_NewString(ctx, result.c_str());
}

// clearData(format?)
static JSValue JSDataTransfer_clearData(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(this_val, js_data_transfer_class_id));
    if (!data || !data->data_transfer) {
        return JS_EXCEPTION;
    }

    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        const char* format = JS_ToCString(ctx, argv[0]);
        if (!format) {
            return JS_EXCEPTION;
        }
        data->data_transfer->ClearData(format);
        JS_FreeCString(ctx, format);
    } else {
        data->data_transfer->ClearData();
    }

    return JS_UNDEFINED;
}

// ========== 属性访问器 ==========

// types (getter)
static JSValue JSDataTransfer_get_types(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(this_val, js_data_transfer_class_id));
    if (!data || !data->data_transfer) {
        return JS_NULL;
    }

    std::vector<std::string> types = data->data_transfer->GetTypes();
    JSValue arr = JS_NewArray(ctx);
    if (JS_IsException(arr)) {
        return arr;
    }

    for (size_t i = 0; i < types.size(); ++i) {
        JS_SetPropertyUint32(ctx, arr, static_cast<uint32_t>(i), 
                             JS_NewString(ctx, types[i].c_str()));
    }

    return arr;
}

// files (getter)
static JSValue JSDataTransfer_get_files(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(this_val, js_data_transfer_class_id));
    if (!data || !data->data_transfer) {
        return JS_NULL;
    }

    return WrapFileList(ctx, data->data_transfer->GetFiles());
}


// effectAllowed (getter)
static JSValue JSDataTransfer_get_effectAllowed(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(this_val, js_data_transfer_class_id));
    if (!data || !data->data_transfer) {
        return JS_NULL;
    }

    DragEffect effect = data->data_transfer->GetEffectAllowed();
    std::string effect_str = DataTransfer::EffectToString(effect);
    return JS_NewString(ctx, effect_str.c_str());
}

// effectAllowed (setter)
static JSValue JSDataTransfer_set_effectAllowed(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(this_val, js_data_transfer_class_id));
    if (!data || !data->data_transfer) {
        return JS_EXCEPTION;
    }

    const char* effect_str = JS_ToCString(ctx, val);
    if (!effect_str) {
        return JS_EXCEPTION;
    }

    DragEffect effect = DataTransfer::StringToEffect(effect_str);
    data->data_transfer->SetEffectAllowed(effect);
    JS_FreeCString(ctx, effect_str);

    return JS_UNDEFINED;
}

// dropEffect (getter)
static JSValue JSDataTransfer_get_dropEffect(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(this_val, js_data_transfer_class_id));
    if (!data || !data->data_transfer) {
        return JS_NULL;
    }

    DragEffect effect = data->data_transfer->GetDropEffect();
    std::string effect_str = DataTransfer::EffectToString(effect);
    return JS_NewString(ctx, effect_str.c_str());
}

// dropEffect (setter)
static JSValue JSDataTransfer_set_dropEffect(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(this_val, js_data_transfer_class_id));
    if (!data || !data->data_transfer) {
        return JS_EXCEPTION;
    }

    const char* effect_str = JS_ToCString(ctx, val);
    if (!effect_str) {
        return JS_EXCEPTION;
    }

    DragEffect effect = DataTransfer::StringToEffect(effect_str);
    data->data_transfer->SetDropEffect(effect);
    JS_FreeCString(ctx, effect_str);

    return JS_UNDEFINED;
}

// ========== 类定义 ==========

static const JSCFunctionListEntry js_data_transfer_proto_funcs[] = {
    JS_CFUNC_DEF("setData", 2, JSDataTransfer_setData),
    JS_CFUNC_DEF("getData", 1, JSDataTransfer_getData),
    JS_CFUNC_DEF("clearData", 0, JSDataTransfer_clearData),
    JS_CGETSET_MAGIC_DEF("types", JSDataTransfer_get_types, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("files", JSDataTransfer_get_files, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("effectAllowed", JSDataTransfer_get_effectAllowed, JSDataTransfer_set_effectAllowed, 0),
    JS_CGETSET_MAGIC_DEF("dropEffect", JSDataTransfer_get_dropEffect, JSDataTransfer_set_dropEffect, 0),
};

static JSClassDef js_data_transfer_class = {
    /* class_name */ "DataTransfer",
    /* finalizer */ JSDataTransferFinalizer,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr,
};

// ========== 公共 API ==========

void InitDataTransferBinding(JSContext* ctx) {
    // 创建 ClassID
    JS_NewClassID(JS_GetRuntime(ctx), &js_data_transfer_class_id);

    // 注册类
    JS_NewClass(JS_GetRuntime(ctx), js_data_transfer_class_id, &js_data_transfer_class);

    // 创建原型对象
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_data_transfer_proto_funcs,
                               sizeof(js_data_transfer_proto_funcs) / sizeof(js_data_transfer_proto_funcs[0]));

    // 设置类的原型
    JS_SetClassProto(ctx, js_data_transfer_class_id, proto);
}

JSValue WrapDataTransfer(JSContext* ctx, std::shared_ptr<DataTransfer> data_transfer) {
    if (!data_transfer) {
        return JS_NULL;
    }

    // 创建新的 JS 对象
    JSValue obj = JS_NewObjectClass(ctx, js_data_transfer_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    // 设置 opaque 数据
    auto* data = new JSDataTransferData();
    data->data_transfer = data_transfer;
    JS_SetOpaque(obj, data);

    return obj;
}

std::shared_ptr<DataTransfer> UnwrapDataTransfer(JSContext* ctx, JSValue value) {
    auto* data = static_cast<JSDataTransferData*>(JS_GetOpaque(value, js_data_transfer_class_id));
    if (!data) {
        return nullptr;
    }
    return data->data_transfer;
}

JSClassID GetDataTransferClassID() {
    return js_data_transfer_class_id;
}

} // namespace bindings
} // namespace mblink
