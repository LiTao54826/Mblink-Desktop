/**
 * @file js_event.cpp
 * @brief Event 类的 JavaScript 绑定实现
 */

#include "js_event.h"
#include "js_node.h"
#include "js_data_transfer.h"
#include "core/dom/drag_event.h"
#include <iostream>

namespace lightui {
namespace bindings {

// ========== Opaque 数据结构 ==========

struct JSEventData {
    std::shared_ptr<Event> event;
};

// ========== ClassID ==========

static JSClassID js_event_class_id = 0;

// ========== 析构函数 ==========

static void JSEventFinalizer(JSRuntime* rt, JSValue val) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(val, js_event_class_id));
    if (data) {
        delete data;
    }
}

// ========== 属性访问器 ==========

// type
static JSValue JSEvent_get_type(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NULL;
    }

    std::string type = data->event->GetType();
    return JS_NewString(ctx, type.c_str());
}

// target
static JSValue JSEvent_get_target(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NULL;
    }

    auto target = data->event->GetTarget();
    if (!target) {
        return JS_NULL;
    }

    return WrapNode(ctx, target);
}

// currentTarget
static JSValue JSEvent_get_currentTarget(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NULL;
    }

    auto current_target = data->event->GetCurrentTarget();
    if (!current_target) {
        return JS_NULL;
    }

    return WrapNode(ctx, current_target);
}

// bubbles
static JSValue JSEvent_get_bubbles(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    return JS_NewBool(ctx, data->event->GetBubbles());
}

// cancelable
static JSValue JSEvent_get_cancelable(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    return JS_NewBool(ctx, data->event->GetCancelable());
}

// ========== MouseEvent 属性访问器 ==========

// clientX
static JSValue JSEvent_get_clientX(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    auto mouse_event = std::dynamic_pointer_cast<MouseEvent>(data->event);
    if (!mouse_event) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, mouse_event->GetClientX());
}

// clientY
static JSValue JSEvent_get_clientY(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    auto mouse_event = std::dynamic_pointer_cast<MouseEvent>(data->event);
    if (!mouse_event) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, mouse_event->GetClientY());
}

// button
static JSValue JSEvent_get_button(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    auto mouse_event = std::dynamic_pointer_cast<MouseEvent>(data->event);
    if (!mouse_event) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, mouse_event->GetButton());
}

// ========== DragEvent 属性访问器 ==========

// dataTransfer
static JSValue JSEvent_get_dataTransfer(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NULL;
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (!drag_event) {
        return JS_NULL;
    }

    auto data_transfer = drag_event->GetDataTransfer();
    if (!data_transfer) {
        return JS_NULL;
    }

    return WrapDataTransfer(ctx, data_transfer);
}

// screenX
static JSValue JSEvent_get_screenX(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewInt32(ctx, drag_event->GetScreenX());
    }

    // 对于普通 MouseEvent，返回 clientX 作为 screenX
    auto mouse_event = std::dynamic_pointer_cast<MouseEvent>(data->event);
    if (mouse_event) {
        return JS_NewInt32(ctx, mouse_event->GetClientX());
    }

    return JS_NewInt32(ctx, 0);
}

// screenY
static JSValue JSEvent_get_screenY(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewInt32(ctx, drag_event->GetScreenY());
    }

    // 对于普通 MouseEvent，返回 clientY 作为 screenY
    auto mouse_event = std::dynamic_pointer_cast<MouseEvent>(data->event);
    if (mouse_event) {
        return JS_NewInt32(ctx, mouse_event->GetClientY());
    }

    return JS_NewInt32(ctx, 0);
}

// ctrlKey
static JSValue JSEvent_get_ctrlKey(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewBool(ctx, drag_event->GetCtrlKey());
    }

    return JS_FALSE;
}

// shiftKey
static JSValue JSEvent_get_shiftKey(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewBool(ctx, drag_event->GetShiftKey());
    }

    return JS_FALSE;
}

// altKey
static JSValue JSEvent_get_altKey(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewBool(ctx, drag_event->GetAltKey());
    }

    return JS_FALSE;
}

// metaKey
static JSValue JSEvent_get_metaKey(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewBool(ctx, drag_event->GetMetaKey());
    }

    return JS_FALSE;
}

// ========== 方法实现 ==========

// stopPropagation()
static JSValue JSEvent_stopPropagation(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_EXCEPTION;
    }

    data->event->StopPropagation();
    return JS_UNDEFINED;
}

// preventDefault()
static JSValue JSEvent_preventDefault(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_EXCEPTION;
    }

    data->event->PreventDefault();
    return JS_UNDEFINED;
}

// ========== 类定义 ==========

static const JSCFunctionListEntry js_event_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("type", JSEvent_get_type, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("target", JSEvent_get_target, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("currentTarget", JSEvent_get_currentTarget, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("bubbles", JSEvent_get_bubbles, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("cancelable", JSEvent_get_cancelable, nullptr, 0),
    // MouseEvent 属性
    JS_CGETSET_MAGIC_DEF("clientX", JSEvent_get_clientX, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("clientY", JSEvent_get_clientY, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("button", JSEvent_get_button, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("screenX", JSEvent_get_screenX, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("screenY", JSEvent_get_screenY, nullptr, 0),
    // DragEvent 属性
    JS_CGETSET_MAGIC_DEF("dataTransfer", JSEvent_get_dataTransfer, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("ctrlKey", JSEvent_get_ctrlKey, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("shiftKey", JSEvent_get_shiftKey, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("altKey", JSEvent_get_altKey, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("metaKey", JSEvent_get_metaKey, nullptr, 0),
    // 方法
    JS_CFUNC_DEF("stopPropagation", 0, JSEvent_stopPropagation),
    JS_CFUNC_DEF("preventDefault", 0, JSEvent_preventDefault),
};

static JSClassDef js_event_class = {
    /* class_name */ "Event",
    /* finalizer */ JSEventFinalizer,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr,
};

// ========== 公共 API ==========

void InitEventBinding(JSContext* ctx) {
    // 创建 ClassID
    JS_NewClassID(JS_GetRuntime(ctx), &js_event_class_id);

    // 注册类
    JS_NewClass(JS_GetRuntime(ctx), js_event_class_id, &js_event_class);

    // 创建原型对象
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_event_proto_funcs, 
                               sizeof(js_event_proto_funcs) / sizeof(js_event_proto_funcs[0]));

    // 设置类的原型
    JS_SetClassProto(ctx, js_event_class_id, proto);
}

JSValue WrapEvent(JSContext* ctx, std::shared_ptr<Event> event) {
    if (!event) {
        return JS_NULL;
    }

    // 创建新的 JS 对象
    JSValue obj = JS_NewObjectClass(ctx, js_event_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    // 设置 opaque 数据
    auto* data = new JSEventData();
    data->event = event;
    JS_SetOpaque(obj, data);

    return obj;
}

std::shared_ptr<Event> UnwrapEvent(JSContext* ctx, JSValue value) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(value, js_event_class_id));
    if (!data) {
        return nullptr;
    }
    return data->event;
}

} // namespace bindings
} // namespace lightui
