/**
 * @file js_event.cpp
 * @brief Event 类的 JavaScript 绑定实现
 */

#include "js_event.h"
#include "js_node.h"
#include "js_data_transfer.h"
#include "core/dom/drag_event.h"
#include "core/event/types/data_transfer.h"
#include <SDL3/SDL.h>
#include <iostream>

namespace mblink {
namespace bindings {

// ========== Opaque 数据结构 ==========

struct JSEventData {
    std::shared_ptr<Event> event;
};

// ========== ClassID ==========

static JSClassID js_event_class_id = 0;

namespace {

std::string JSValueToJSONString(JSContext* ctx, JSValueConst value) {
    JSValue json_value = JS_JSONStringify(ctx, value, JS_UNDEFINED, JS_UNDEFINED);
    if (JS_IsException(json_value)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        return "null";
    }

    if (JS_IsUndefined(json_value) || JS_IsNull(json_value)) {
        JS_FreeValue(ctx, json_value);
        return "null";
    }

    const char* json_cstr = JS_ToCString(ctx, json_value);
    std::string result = json_cstr ? json_cstr : "null";
    if (json_cstr) {
        JS_FreeCString(ctx, json_cstr);
    }
    JS_FreeValue(ctx, json_value);
    return result;
}

JSValue JSONStringToJSValue(JSContext* ctx, const std::string& json_string) {
    if (json_string.empty()) {
        return JS_NULL;
    }

    JSValue value = JS_ParseJSON(ctx, json_string.c_str(), json_string.size(), "<custom-event-detail>");
    if (JS_IsException(value)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        return JS_NULL;
    }
    return value;
}

bool GetBooleanOption(JSContext* ctx, JSValueConst options, const char* name, bool fallback) {
    if (!JS_IsObject(options)) {
        return fallback;
    }
    JSValue value = JS_GetPropertyStr(ctx, options, name);
    bool result = fallback;
    if (!JS_IsUndefined(value) && !JS_IsNull(value)) {
        result = JS_ToBool(ctx, value);
    }
    JS_FreeValue(ctx, value);
    return result;
}

int GetIntOption(JSContext* ctx, JSValueConst options, const char* name, int fallback) {
    if (!JS_IsObject(options)) {
        return fallback;
    }
    JSValue value = JS_GetPropertyStr(ctx, options, name);
    int32_t result = fallback;
    if (!JS_IsUndefined(value) && !JS_IsNull(value)) {
        if (JS_ToInt32(ctx, &result, value) != 0) {
            result = fallback;
        }
    }
    JS_FreeValue(ctx, value);
    return result;
}

std::string GetStringOption(JSContext* ctx, JSValueConst options, const char* name, const char* fallback) {
    if (!JS_IsObject(options)) {
        return fallback;
    }
    JSValue value = JS_GetPropertyStr(ctx, options, name);
    std::string result = fallback;
    if (!JS_IsUndefined(value) && !JS_IsNull(value)) {
        const char* text = JS_ToCString(ctx, value);
        if (text) {
            result = text;
            JS_FreeCString(ctx, text);
        }
    }
    JS_FreeValue(ctx, value);
    return result;
}

JSValue WrapEventWithPrototype(JSContext* ctx, JSValueConst new_target, std::shared_ptr<Event> event) {
    JSValue obj = WrapEvent(ctx, event);
    if (JS_IsException(obj)) {
        return obj;
    }

    JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsObject(proto)) {
        JS_SetPrototype(ctx, obj, proto);
    }
    JS_FreeValue(ctx, proto);
    return obj;
}

void RegisterEventConstructor(JSContext* ctx,
                              JSValueConst global,
                              const char* name,
                              JSCFunction* constructor,
                              JSValueConst base_proto) {
    JSValue proto = JS_NewObject(ctx);
    JS_SetPrototype(ctx, proto, base_proto);

    JSValue ctor = JS_NewCFunction2(ctx, constructor, name, 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetPropertyStr(ctx, global, name, ctor);
    JS_FreeValue(ctx, proto);
}

} // namespace

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

// eventPhase
static JSValue JSEvent_get_eventPhase(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewInt32(ctx, 0);
    }

    return JS_NewInt32(ctx, static_cast<int>(data->event->GetEventPhase()));
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

    auto custom_event = std::dynamic_pointer_cast<CustomEvent>(data->event);
    if (custom_event) {
        return JSONStringToJSValue(ctx, custom_event->GetDetailJSON());
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

// timeStamp
static JSValue JSEvent_get_timeStamp(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_NewFloat64(ctx, 0);
    }

    return JS_NewFloat64(ctx, data->event->GetTimeStamp());
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

// stopImmediatePropagation()
static JSValue JSEvent_stopImmediatePropagation(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_EXCEPTION;
    }

    data->event->StopImmediatePropagation();
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

// initEvent(type, bubbles, cancelable)
static JSValue JSEvent_initEvent(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_EXCEPTION;
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "initEvent requires at least 1 argument");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    bool bubbles = false;
    bool cancelable = false;
    if (argc >= 2) {
        bubbles = JS_ToBool(ctx, argv[1]);
    }
    if (argc >= 3) {
        cancelable = JS_ToBool(ctx, argv[2]);
    }

    data->event->InitEvent(type, bubbles, cancelable);
    JS_FreeCString(ctx, type);
    return JS_UNDEFINED;
}

// initCustomEvent(type, bubbles, cancelable, detail)
static JSValue JSEvent_initCustomEvent(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSEventData*>(JS_GetOpaque(this_val, js_event_class_id));
    if (!data || !data->event) {
        return JS_EXCEPTION;
    }

    auto custom_event = std::dynamic_pointer_cast<CustomEvent>(data->event);
    if (!custom_event) {
        return JS_ThrowTypeError(ctx, "initCustomEvent requires a CustomEvent object");
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "initCustomEvent requires at least 1 argument");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    bool bubbles = false;
    bool cancelable = false;
    if (argc >= 2) {
        bubbles = JS_ToBool(ctx, argv[1]);
    }
    if (argc >= 3) {
        cancelable = JS_ToBool(ctx, argv[2]);
    }

    const std::string detail_json = argc >= 4 ? JSValueToJSONString(ctx, argv[3]) : "null";
    custom_event->InitCustomEvent(type, bubbles, cancelable, detail_json);
    JS_FreeCString(ctx, type);
    return JS_UNDEFINED;
}

// new Event(type, { bubbles, cancelable })
static JSValue JSEvent_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "Event constructor requires 1 argument");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    bool bubbles = true;
    bool cancelable = true;
    if (argc >= 2 && JS_IsObject(argv[1])) {
        JSValue bubbles_value = JS_GetPropertyStr(ctx, argv[1], "bubbles");
        JSValue cancelable_value = JS_GetPropertyStr(ctx, argv[1], "cancelable");

        if (!JS_IsUndefined(bubbles_value) && !JS_IsNull(bubbles_value)) {
            bubbles = JS_ToBool(ctx, bubbles_value);
        }
        if (!JS_IsUndefined(cancelable_value) && !JS_IsNull(cancelable_value)) {
            cancelable = JS_ToBool(ctx, cancelable_value);
        }

        JS_FreeValue(ctx, bubbles_value);
        JS_FreeValue(ctx, cancelable_value);
    }

    auto event = std::make_shared<Event>(type, bubbles, cancelable);
    JS_FreeCString(ctx, type);
    return WrapEvent(ctx, event);
}

// new CustomEvent(type, { detail, bubbles, cancelable })
static JSValue JSCustomEvent_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "CustomEvent constructor requires 1 argument");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    bool bubbles = false;
    bool cancelable = false;
    std::string detail_json = "null";
    if (argc >= 2 && JS_IsObject(argv[1])) {
        JSValue bubbles_value = JS_GetPropertyStr(ctx, argv[1], "bubbles");
        JSValue cancelable_value = JS_GetPropertyStr(ctx, argv[1], "cancelable");
        JSValue detail_value = JS_GetPropertyStr(ctx, argv[1], "detail");

        if (!JS_IsUndefined(bubbles_value) && !JS_IsNull(bubbles_value)) {
            bubbles = JS_ToBool(ctx, bubbles_value);
        }
        if (!JS_IsUndefined(cancelable_value) && !JS_IsNull(cancelable_value)) {
            cancelable = JS_ToBool(ctx, cancelable_value);
        }
        if (!JS_IsUndefined(detail_value)) {
            detail_json = JSValueToJSONString(ctx, detail_value);
        }

        JS_FreeValue(ctx, bubbles_value);
        JS_FreeValue(ctx, cancelable_value);
        JS_FreeValue(ctx, detail_value);
    }

    auto event = std::make_shared<CustomEvent>(type, bubbles, cancelable, detail_json);
    JS_FreeCString(ctx, type);
    return WrapEvent(ctx, event);
}

// ========== 类定义 ==========

static JSValue JSMouseEvent_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "MouseEvent constructor requires 1 argument");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    JSValueConst options = argc >= 2 ? argv[1] : JS_UNDEFINED;
    auto event = std::make_shared<MouseEvent>(
        type,
        GetIntOption(ctx, options, "clientX", 0),
        GetIntOption(ctx, options, "clientY", 0),
        GetIntOption(ctx, options, "button", 0),
        GetIntOption(ctx, options, "detail", 1),
        GetIntOption(ctx, options, "buttons", 0));
    JS_FreeCString(ctx, type);
    return WrapEventWithPrototype(ctx, new_target, event);
}

static JSValue JSPointerEvent_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    return JSMouseEvent_constructor(ctx, new_target, argc, argv);
}

static JSValue JSDragEvent_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "DragEvent constructor requires 1 argument");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    JSValueConst options = argc >= 2 ? argv[1] : JS_UNDEFINED;
    std::shared_ptr<DataTransfer> data_transfer;
    if (JS_IsObject(options)) {
        JSValue data_transfer_value = JS_GetPropertyStr(ctx, options, "dataTransfer");
        data_transfer = UnwrapDataTransfer(ctx, data_transfer_value);
        JS_FreeValue(ctx, data_transfer_value);
    }
    if (!data_transfer) {
        data_transfer = std::make_shared<DataTransfer>();
    }

    auto event = std::make_shared<DragEvent>(
        type,
        GetIntOption(ctx, options, "clientX", 0),
        GetIntOption(ctx, options, "clientY", 0),
        GetIntOption(ctx, options, "button", 0),
        data_transfer,
        GetBooleanOption(ctx, options, "ctrlKey", false),
        GetBooleanOption(ctx, options, "shiftKey", false),
        GetBooleanOption(ctx, options, "altKey", false),
        GetBooleanOption(ctx, options, "metaKey", false));
    JS_FreeCString(ctx, type);
    return WrapEventWithPrototype(ctx, new_target, event);
}

static JSValue JSKeyboardEvent_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "KeyboardEvent constructor requires 1 argument");
    }

    const char* type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        return JS_EXCEPTION;
    }

    JSValueConst options = argc >= 2 ? argv[1] : JS_UNDEFINED;
    auto event = std::make_shared<KeyboardEvent>(
        type,
        GetStringOption(ctx, options, "key", ""),
        GetStringOption(ctx, options, "code", ""),
        GetIntOption(ctx, options, "keyCode", 0),
        GetBooleanOption(ctx, options, "ctrlKey", false),
        GetBooleanOption(ctx, options, "shiftKey", false),
        GetBooleanOption(ctx, options, "altKey", false),
        GetBooleanOption(ctx, options, "metaKey", false),
        GetBooleanOption(ctx, options, "repeat", false));
    JS_FreeCString(ctx, type);
    return WrapEventWithPrototype(ctx, new_target, event);
}

static const JSCFunctionListEntry js_event_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("type", JSEvent_get_type, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("target", JSEvent_get_target, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("currentTarget", JSEvent_get_currentTarget, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("bubbles", JSEvent_get_bubbles, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("cancelable", JSEvent_get_cancelable, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("defaultPrevented", JSEvent_get_defaultPrevented, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("timeStamp", JSEvent_get_timeStamp, nullptr, 0),
    JS_CGETSET_MAGIC_DEF("eventPhase", JSEvent_get_eventPhase, nullptr, 0),
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
    JS_CFUNC_DEF("stopImmediatePropagation", 0, JSEvent_stopImmediatePropagation),
    JS_CFUNC_DEF("preventDefault", 0, JSEvent_preventDefault),
    JS_CFUNC_DEF("initEvent", 3, JSEvent_initEvent),
};

static const JSCFunctionListEntry js_custom_event_proto_funcs[] = {
    JS_CFUNC_DEF("initCustomEvent", 4, JSEvent_initCustomEvent),
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

    JSValue custom_proto = JS_NewObject(ctx);
    JS_SetPrototype(ctx, custom_proto, proto);
    JS_SetPropertyFunctionList(ctx, custom_proto, js_custom_event_proto_funcs,
                               sizeof(js_custom_event_proto_funcs) / sizeof(js_custom_event_proto_funcs[0]));

    // 设置类的原型
    JS_SetClassProto(ctx, js_event_class_id, proto);

    // 注册全局 Event / CustomEvent 构造函数
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue event_ctor = JS_NewCFunction2(ctx, JSEvent_constructor, "Event", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, event_ctor, proto);
    JS_SetPropertyStr(ctx, global, "Event", event_ctor);

    JSValue custom_event_ctor = JS_NewCFunction2(ctx, JSCustomEvent_constructor, "CustomEvent", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, custom_event_ctor, custom_proto);
    JS_SetPropertyStr(ctx, global, "CustomEvent", custom_event_ctor);

    RegisterEventConstructor(ctx, global, "MouseEvent", JSMouseEvent_constructor, proto);
    RegisterEventConstructor(ctx, global, "PointerEvent", JSPointerEvent_constructor, proto);
    RegisterEventConstructor(ctx, global, "DragEvent", JSDragEvent_constructor, proto);
    RegisterEventConstructor(ctx, global, "KeyboardEvent", JSKeyboardEvent_constructor, proto);

    JS_FreeValue(ctx, custom_proto);
    JS_FreeValue(ctx, global);
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

    if (std::dynamic_pointer_cast<CustomEvent>(event)) {
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue custom_event_ctor = JS_GetPropertyStr(ctx, global, "CustomEvent");
        if (JS_IsObject(custom_event_ctor)) {
            JSValue custom_proto = JS_GetPropertyStr(ctx, custom_event_ctor, "prototype");
            if (JS_IsObject(custom_proto)) {
                JS_SetPrototype(ctx, obj, custom_proto);
            }
            JS_FreeValue(ctx, custom_proto);
        }
        JS_FreeValue(ctx, custom_event_ctor);
        JS_FreeValue(ctx, global);
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
} // namespace mblink
