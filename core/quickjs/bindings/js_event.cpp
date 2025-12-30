/**
 * @file js_event.cpp
 * @brief Event 类的 JavaScript 绑定实现
 */

#include "js_event.h"
#include "js_node.h"
#include "js_data_transfer.h"
#include "core/dom/drag_event.h"
#include <SDL3/SDL.h>
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

// buttons (bitmask of currently pressed buttons: 1=left, 2=right, 4=middle)
static JSValue JSEvent_get_buttons(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    auto mouse_event = std::dynamic_pointer_cast<MouseEvent>(data->event);
    if (!mouse_event) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, mouse_event->GetButtons());
}

// detail (click count: 1=single, 2=double, 3=triple)
static JSValue JSEvent_get_detail(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    auto mouse_event = std::dynamic_pointer_cast<MouseEvent>(data->event);
    if (!mouse_event) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, mouse_event->GetDetail());
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

// DragEvent 属性访问器已合并到 KeyboardEvent 部分

// ========== KeyboardEvent 属性访问器 ==========

// key
static JSValue JSEvent_get_key(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_UNDEFINED;
    }

    auto keyboard_event = std::dynamic_pointer_cast<KeyboardEvent>(data->event);
    if (!keyboard_event) {
        return JS_UNDEFINED;
    }

    return JS_NewString(ctx, keyboard_event->GetKey().c_str());
}

// code
static JSValue JSEvent_get_code(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_UNDEFINED;
    }

    auto keyboard_event = std::dynamic_pointer_cast<KeyboardEvent>(data->event);
    if (!keyboard_event) {
        return JS_UNDEFINED;
    }

    return JS_NewString(ctx, keyboard_event->GetCode().c_str());
}

// keyCode
static JSValue JSEvent_get_keyCode(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    auto keyboard_event = std::dynamic_pointer_cast<KeyboardEvent>(data->event);
    if (!keyboard_event) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, keyboard_event->GetKeyCode());
}

// charCode
static JSValue JSEvent_get_charCode(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    auto keyboard_event = std::dynamic_pointer_cast<KeyboardEvent>(data->event);
    if (!keyboard_event) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, keyboard_event->GetCharCode());
}

// repeat
static JSValue JSEvent_get_repeat(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto keyboard_event = std::dynamic_pointer_cast<KeyboardEvent>(data->event);
    if (!keyboard_event) {
        return JS_FALSE;
    }

    return JS_NewBool(ctx, keyboard_event->GetRepeat());
}

// KeyboardEvent 的修饰键属性（覆盖 DragEvent 的实现）
static JSValue JSEvent_get_keyboard_ctrlKey(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto keyboard_event = std::dynamic_pointer_cast<KeyboardEvent>(data->event);
    if (keyboard_event) {
        return JS_NewBool(ctx, keyboard_event->GetCtrlKey());
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewBool(ctx, drag_event->GetCtrlKey());
    }

    return JS_FALSE;
}

static JSValue JSEvent_get_keyboard_shiftKey(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto keyboard_event = std::dynamic_pointer_cast<KeyboardEvent>(data->event);
    if (keyboard_event) {
        return JS_NewBool(ctx, keyboard_event->GetShiftKey());
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewBool(ctx, drag_event->GetShiftKey());
    }

    return JS_FALSE;
}

static JSValue JSEvent_get_keyboard_altKey(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto keyboard_event = std::dynamic_pointer_cast<KeyboardEvent>(data->event);
    if (keyboard_event) {
        return JS_NewBool(ctx, keyboard_event->GetAltKey());
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewBool(ctx, drag_event->GetAltKey());
    }

    return JS_FALSE;
}

static JSValue JSEvent_get_keyboard_metaKey(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto keyboard_event = std::dynamic_pointer_cast<KeyboardEvent>(data->event);
    if (keyboard_event) {
        return JS_NewBool(ctx, keyboard_event->GetMetaKey());
    }

    auto drag_event = std::dynamic_pointer_cast<DragEvent>(data->event);
    if (drag_event) {
        return JS_NewBool(ctx, drag_event->GetMetaKey());
    }

    return JS_FALSE;
}

// ========== InputEvent 属性访问器 ==========

// inputType
static JSValue JSEvent_get_inputType(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NULL;
    }

    auto input_event = std::dynamic_pointer_cast<InputEvent>(data->event);
    if (!input_event) {
        return JS_NULL;
    }

    return JS_NewString(ctx, input_event->GetInputType().c_str());
}

// data (InputEvent)
static JSValue JSEvent_get_data(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NULL;
    }

    auto input_event = std::dynamic_pointer_cast<InputEvent>(data->event);
    if (!input_event) {
        return JS_NULL;
    }

    std::string event_data = input_event->GetData();
    if (event_data.empty()) {
        return JS_NULL;
    }

    return JS_NewString(ctx, event_data.c_str());
}

// isComposing
static JSValue JSEvent_get_isComposing(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    auto input_event = std::dynamic_pointer_cast<InputEvent>(data->event);
    if (!input_event) {
        return JS_FALSE;
    }

    return JS_NewBool(ctx, input_event->IsComposing());
}

// defaultPrevented
static JSValue JSEvent_get_defaultPrevented(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_FALSE;
    }

    return JS_NewBool(ctx, data->event->IsDefaultPrevented());
}

// ========== ClipboardEvent 属性访问器 ==========

// clipboardData (返回一个包含 getData/setData 方法的对象)
static JSValue JSEvent_get_clipboardData(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NULL;
    }

    auto clipboard_event = std::dynamic_pointer_cast<ClipboardEvent>(data->event);
    if (!clipboard_event) {
        return JS_NULL;
    }

    // 创建一个简单的 clipboardData 对象
    // 注意：完整的 DataTransfer API 更复杂，这里实现简化版本
    JSValue clipboard_data_obj = JS_NewObject(ctx);

    // 存储剪贴板数据到对象属性中（用于 getData/setData）
    std::string text_data = clipboard_event->GetClipboardData();
    JS_SetPropertyStr(ctx, clipboard_data_obj, "_textData", JS_NewString(ctx, text_data.c_str()));

    // 保存事件引用以便 setData 可以更新
    JS_SetPropertyStr(ctx, clipboard_data_obj, "_event", JS_DupValue(ctx, this_val));

    // getData(format) 方法
    JSValue get_data_func = JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
        if (argc < 1) {
            return JS_NewString(ctx, "");
        }

        const char* format = JS_ToCString(ctx, argv[0]);
        if (!format) {
            return JS_NewString(ctx, "");
        }

        std::string format_str(format);
        JS_FreeCString(ctx, format);

        // 只支持 text/plain 格式
        if (format_str == "text/plain" || format_str == "text" || format_str == "text/uri-list") {
            // 首先尝试从事件数据获取
            JSValue text_data = JS_GetPropertyStr(ctx, this_val, "_textData");
            if (JS_IsString(text_data)) {
                const char* str = JS_ToCString(ctx, text_data);
                if (str && strlen(str) > 0) {
                    JSValue result = JS_NewString(ctx, str);
                    JS_FreeCString(ctx, str);
                    JS_FreeValue(ctx, text_data);
                    return result;
                }
                if (str) JS_FreeCString(ctx, str);
            }
            JS_FreeValue(ctx, text_data);
            
            // 如果事件数据为空，从系统剪贴板读取
            char* clipboard_text = SDL_GetClipboardText();
            if (clipboard_text) {
                JSValue result = JS_NewString(ctx, clipboard_text);
                SDL_free(clipboard_text);
                return result;
            }
        }

        return JS_NewString(ctx, "");
    }, "getData", 1);
    JS_SetPropertyStr(ctx, clipboard_data_obj, "getData", get_data_func);

    // setData(format, data) 方法
    JSValue set_data_func = JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
        if (argc < 2) {
            return JS_UNDEFINED;
        }

        const char* format = JS_ToCString(ctx, argv[0]);
        const char* data_str = JS_ToCString(ctx, argv[1]);

        if (!format || !data_str) {
            if (format) JS_FreeCString(ctx, format);
            if (data_str) JS_FreeCString(ctx, data_str);
            return JS_UNDEFINED;
        }

        std::string format_str(format);
        JS_FreeCString(ctx, format);

        // 只支持 text/plain 格式
        if (format_str == "text/plain" || format_str == "text") {
            JS_SetPropertyStr(ctx, this_val, "_textData", JS_NewString(ctx, data_str));

            // 写入系统剪贴板
            SDL_SetClipboardText(data_str);

            // 更新原始事件中的数据
            JSValue event_val = JS_GetPropertyStr(ctx, this_val, "_event");
            if (!JS_IsNull(event_val) && !JS_IsUndefined(event_val)) {
                auto* event_data = static_cast<JSEventData*>(JS_GetOpaque(event_val, js_event_class_id));
                if (event_data && event_data->event) {
                    auto clipboard_event = std::dynamic_pointer_cast<ClipboardEvent>(event_data->event);
                    if (clipboard_event) {
                        clipboard_event->SetClipboardData(data_str);
                    }
                }
            }
            JS_FreeValue(ctx, event_val);
        }

        JS_FreeCString(ctx, data_str);
        return JS_UNDEFINED;
    }, "setData", 2);
    JS_SetPropertyStr(ctx, clipboard_data_obj, "setData", set_data_func);

    // clearData(format) 方法 - 清除指定格式的数据
    JSValue clear_data_func = JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
        // 清除文本数据
        JS_SetPropertyStr(ctx, this_val, "_textData", JS_NewString(ctx, ""));
        return JS_UNDEFINED;
    }, "clearData", 1);
    JS_SetPropertyStr(ctx, clipboard_data_obj, "clearData", clear_data_func);

    return clipboard_data_obj;
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
    JS_CGETSET_MAGIC_DEF("defaultPrevented", JSEvent_get_defaultPrevented, nullptr, 0),
    // MouseEvent 属性
    JS_CGETSET_MAGIC_DEF("clientX", JSEvent_get_clientX, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("clientY", JSEvent_get_clientY, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("button", JSEvent_get_button, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("buttons", JSEvent_get_buttons, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("detail", JSEvent_get_detail, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("screenX", JSEvent_get_screenX, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("screenY", JSEvent_get_screenY, nullptr, 0),
    // DragEvent 属性
    JS_CGETSET_MAGIC_DEF("dataTransfer", JSEvent_get_dataTransfer, nullptr, 0),
    // KeyboardEvent 属性
    JS_CGETSET_MAGIC_DEF("key", JSEvent_get_key, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("code", JSEvent_get_code, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("keyCode", JSEvent_get_keyCode, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("charCode", JSEvent_get_charCode, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("repeat", JSEvent_get_repeat, nullptr, 0),
    // 修饰键属性（支持 KeyboardEvent 和 DragEvent）
    JS_CGETSET_MAGIC_DEF("ctrlKey", JSEvent_get_keyboard_ctrlKey, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("shiftKey", JSEvent_get_keyboard_shiftKey, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("altKey", JSEvent_get_keyboard_altKey, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("metaKey", JSEvent_get_keyboard_metaKey, nullptr, 0),
    // InputEvent 属性
    JS_CGETSET_MAGIC_DEF("inputType", JSEvent_get_inputType, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("data", JSEvent_get_data, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("isComposing", JSEvent_get_isComposing, nullptr, 0),
    // ClipboardEvent 属性
    JS_CGETSET_MAGIC_DEF("clipboardData", JSEvent_get_clipboardData, nullptr, 0),
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
