/**
 * @file dom_bindings.cpp
 * @brief Legacy DOMBindings compatibility entrypoints
 *
 * Legacy DOM wrapper/class registration has been removed.
 * QuickJS mainline bindings now live under core/quickjs/*.
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

namespace mblink {

// Forward declaration
void InitImageConstructor(JSContext* ctx);

// legacy DOMBindings implementation removed; quickjs mainline lives in core/quickjs/*

void DOMBindings::Cleanup(JSContext* ctx) {
    if (!ctx) {
        return;
    }

    auto& dom_binding_map = DOMBindingMap::GetInstance();
    dom_binding_map.ForEach([ctx](Node* node, JSContext*, JSValueConst value) {
        if (auto* element = dynamic_cast<Element*>(node)) {
            if (!JS_IsUndefined(value) && !JS_IsNull(value) &&
                JS_GetOpaque(value, bindings::GetElementClassID())) {
                bindings::ClearElementListenerBindings(ctx, value);
            }
            element->ClearAllEventListeners();
        }
    });

    JS_RunGC(JS_GetRuntime(ctx));
}


// ========== Image constructor ==========

// Image constructor: new Image([width], [height])
// HTMLImageElement properties (src, onload, onerror, etc.) are provided by js_element.cpp.
static JSValue js_image_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    // Create HTMLImageElement
    auto img_element = std::make_shared<HTMLImageElement>();

    // Apply optional width/height arguments
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

    // Wrap with the QuickJS Element binding.
    return bindings::WrapElement(ctx, img_element);
}

// Register Image constructor on the global object.
void InitImageConstructor(JSContext* ctx) {
    JSValue global = JS_GetGlobalObject(ctx);

    // Create Image constructor
    JSValue image_ctor = JS_NewCFunction2(ctx, js_image_constructor, "Image", 0, JS_CFUNC_constructor, 0);

    // Reuse the QuickJS Element prototype.
    JSValue proto = JS_GetClassProto(ctx, bindings::GetElementClassID());

    JS_SetConstructor(ctx, image_ctor, proto);
    JS_SetPropertyStr(ctx, global, "Image", image_ctor);

    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global);
}

} // namespace mblink
