/**
 * @file js_event.cpp
 * @brief Event 类的 JavaScript 绑定实现
 */

#include "js_event.h"
#include "js_node.h"
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
