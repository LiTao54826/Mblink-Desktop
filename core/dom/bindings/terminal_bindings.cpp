/**
 * @file terminal_bindings.cpp
 * @brief Terminal 和 LogView JavaScript 绑定实现
 */

#include "terminal_bindings.h"

#include "core/dom/elements/terminal/html_terminal_element.h"
#include "core/dom/elements/logview/html_logview_element.h"

namespace mbink {

// 静态成员初始化
JSClassID TerminalBindings::terminal_class_id = 0;
JSClassID TerminalBindings::logview_class_id = 0;
bool TerminalBindings::initialized_ = false;

// ========== Terminal 绑定 ==========

static void js_terminal_finalizer(JSRuntime* rt, JSValue val) {
    // Terminal 由 DOM 管理，不需要 delete
}

// Terminal 属性 getters/setters
static JSValue js_terminal_get_rows(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    return JS_NewInt32(ctx, terminal->rows());
}

static JSValue js_terminal_set_rows(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    int32_t rows;
    if (JS_ToInt32(ctx, &rows, val) != 0) return JS_EXCEPTION;
    terminal->set_rows(rows);
    return JS_UNDEFINED;
}

static JSValue js_terminal_get_cols(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    return JS_NewInt32(ctx, terminal->cols());
}

static JSValue js_terminal_set_cols(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    int32_t cols;
    if (JS_ToInt32(ctx, &cols, val) != 0) return JS_EXCEPTION;
    terminal->set_cols(cols);
    return JS_UNDEFINED;
}

static JSValue js_terminal_get_scrollback(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    return JS_NewInt32(ctx, terminal->scrollback());
}

static JSValue js_terminal_set_scrollback(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    int32_t scrollback;
    if (JS_ToInt32(ctx, &scrollback, val) != 0) return JS_EXCEPTION;
    terminal->set_scrollback(scrollback);
    return JS_UNDEFINED;
}

// Terminal 方法
static JSValue js_terminal_write(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "write requires 1 argument");

    const char* data = JS_ToCString(ctx, argv[0]);
    if (!data) return JS_EXCEPTION;

    terminal->Write(data);
    JS_FreeCString(ctx, data);
    return JS_UNDEFINED;
}

static JSValue js_terminal_clear(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    terminal->Clear();
    return JS_UNDEFINED;
}

static JSValue js_terminal_scroll_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "scrollTo requires 1 argument");

    int32_t line;
    if (JS_ToInt32(ctx, &line, argv[0]) != 0) return JS_EXCEPTION;

    terminal->ScrollTo(line);
    return JS_UNDEFINED;
}

static JSValue js_terminal_serialize(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    std::string text = terminal->Serialize();
    return JS_NewString(ctx, text.c_str());
}

static JSValue js_terminal_focus(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    terminal->Focus();
    return JS_UNDEFINED;
}

static JSValue js_terminal_execute(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "execute requires 1 argument");

    const char* command = JS_ToCString(ctx, argv[0]);
    if (!command) return JS_EXCEPTION;

    terminal->Execute(command);
    JS_FreeCString(ctx, command);
    return JS_UNDEFINED;
}

static JSValue js_terminal_start_shell(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;

    std::string shell;
    if (argc >= 1) {
        const char* s = JS_ToCString(ctx, argv[0]);
        if (s) {
            shell = s;
            JS_FreeCString(ctx, s);
        }
    }

    terminal->StartShell(shell);
    return JS_UNDEFINED;
}

static JSValue js_terminal_send_input(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "sendInput requires 1 argument");

    const char* input = JS_ToCString(ctx, argv[0]);
    if (!input) return JS_EXCEPTION;

    terminal->SendInput(input);
    JS_FreeCString(ctx, input);
    return JS_UNDEFINED;
}

static JSValue js_terminal_resize(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* terminal = TerminalBindings::UnwrapTerminal(ctx, this_val);
    if (!terminal) return JS_EXCEPTION;
    if (argc < 2) return JS_ThrowTypeError(ctx, "resize requires 2 arguments");

    int32_t rows, cols;
    if (JS_ToInt32(ctx, &rows, argv[0]) != 0) return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &cols, argv[1]) != 0) return JS_EXCEPTION;

    terminal->Resize(rows, cols);
    return JS_UNDEFINED;
}

// Terminal 属性和方法定义
static const JSCFunctionListEntry js_terminal_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("rows", js_terminal_get_rows, js_terminal_set_rows, 0),
    JS_CGETSET_MAGIC_DEF("cols", js_terminal_get_cols, js_terminal_set_cols, 0),
    JS_CGETSET_MAGIC_DEF("scrollback", js_terminal_get_scrollback, js_terminal_set_scrollback, 0),
    JS_CFUNC_DEF("write", 1, js_terminal_write),
    JS_CFUNC_DEF("clear", 0, js_terminal_clear),
    JS_CFUNC_DEF("scrollTo", 1, js_terminal_scroll_to),
    JS_CFUNC_DEF("serialize", 0, js_terminal_serialize),
    JS_CFUNC_DEF("focus", 0, js_terminal_focus),
    JS_CFUNC_DEF("execute", 1, js_terminal_execute),
    JS_CFUNC_DEF("startShell", 1, js_terminal_start_shell),
    JS_CFUNC_DEF("sendInput", 1, js_terminal_send_input),
    JS_CFUNC_DEF("resize", 2, js_terminal_resize),
};

// ========== LogView 绑定 ==========

static void js_logview_finalizer(JSRuntime* rt, JSValue val) {
    // LogView 由 DOM 管理，不需要 delete
}

// LogView 属性 getters/setters
static JSValue js_logview_get_max_entries(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    return JS_NewInt32(ctx, logview->max_entries());
}

static JSValue js_logview_set_max_entries(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    int32_t max_entries;
    if (JS_ToInt32(ctx, &max_entries, val) != 0) return JS_EXCEPTION;
    logview->set_max_entries(max_entries);
    return JS_UNDEFINED;
}

static JSValue js_logview_get_auto_scroll(JSContext* ctx, JSValueConst this_val, int magic) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    return JS_NewBool(ctx, logview->auto_scroll());
}

static JSValue js_logview_set_auto_scroll(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    logview->set_auto_scroll(JS_ToBool(ctx, val));
    return JS_UNDEFINED;
}

// LogView 方法
static JSValue js_logview_append(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    if (argc < 3) return JS_ThrowTypeError(ctx, "append requires 3 arguments");

    const char* level = JS_ToCString(ctx, argv[0]);
    const char* source = JS_ToCString(ctx, argv[1]);
    const char* message = JS_ToCString(ctx, argv[2]);

    if (!level || !source || !message) {
        if (level) JS_FreeCString(ctx, level);
        if (source) JS_FreeCString(ctx, source);
        if (message) JS_FreeCString(ctx, message);
        return JS_EXCEPTION;
    }

    logview->Append(level, source, message);

    JS_FreeCString(ctx, level);
    JS_FreeCString(ctx, source);
    JS_FreeCString(ctx, message);
    return JS_UNDEFINED;
}

static JSValue js_logview_clear(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    logview->Clear();
    return JS_UNDEFINED;
}

static JSValue js_logview_scroll_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "scrollTo requires 1 argument");

    int32_t line;
    if (JS_ToInt32(ctx, &line, argv[0]) != 0) return JS_EXCEPTION;

    logview->ScrollTo(line);
    return JS_UNDEFINED;
}

static JSValue js_logview_set_level_filter(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "setLevelFilter requires 1 argument");

    // 获取数组长度
    JSValue length_val = JS_GetPropertyStr(ctx, argv[0], "length");
    if (JS_IsUndefined(length_val)) {
        return JS_ThrowTypeError(ctx, "setLevelFilter argument must be an array");
    }

    int64_t length;
    JS_ToInt64(ctx, &length, length_val);
    JS_FreeValue(ctx, length_val);

    std::vector<std::string> levels;
    for (int64_t i = 0; i < length; i++) {
        JSValue elem = JS_GetPropertyUint32(ctx, argv[0], i);
        const char* str = JS_ToCString(ctx, elem);
        if (str) {
            levels.push_back(str);
            JS_FreeCString(ctx, str);
        }
        JS_FreeValue(ctx, elem);
    }

    logview->SetLevelFilter(levels);
    return JS_UNDEFINED;
}

static JSValue js_logview_set_source_filter(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "setSourceFilter requires 1 argument");

    JSValue length_val = JS_GetPropertyStr(ctx, argv[0], "length");
    if (JS_IsUndefined(length_val)) {
        return JS_ThrowTypeError(ctx, "setSourceFilter argument must be an array");
    }

    int64_t length;
    JS_ToInt64(ctx, &length, length_val);
    JS_FreeValue(ctx, length_val);

    std::vector<std::string> sources;
    for (int64_t i = 0; i < length; i++) {
        JSValue elem = JS_GetPropertyUint32(ctx, argv[0], i);
        const char* str = JS_ToCString(ctx, elem);
        if (str) {
            sources.push_back(str);
            JS_FreeCString(ctx, str);
        }
        JS_FreeValue(ctx, elem);
    }

    logview->SetSourceFilter(sources);
    return JS_UNDEFINED;
}

static JSValue js_logview_clear_filter(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    logview->ClearFilter();
    return JS_UNDEFINED;
}

static JSValue js_logview_search(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "search requires 1 argument");

    const char* query = JS_ToCString(ctx, argv[0]);
    if (!query) return JS_EXCEPTION;

    bool use_regex = false;
    if (argc >= 2) {
        use_regex = JS_ToBool(ctx, argv[1]);
    }

    int count = logview->Search(query, use_regex);
    JS_FreeCString(ctx, query);
    return JS_NewInt32(ctx, count);
}

static JSValue js_logview_next_match(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    logview->NextMatch();
    return JS_UNDEFINED;
}

static JSValue js_logview_prev_match(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    logview->PrevMatch();
    return JS_UNDEFINED;
}

static JSValue js_logview_clear_search(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    logview->ClearSearch();
    return JS_UNDEFINED;
}

static JSValue js_logview_export(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;

    std::string format = "text";
    if (argc >= 1) {
        const char* f = JS_ToCString(ctx, argv[0]);
        if (f) {
            format = f;
            JS_FreeCString(ctx, f);
        }
    }

    std::string result = logview->Export(format);
    return JS_NewString(ctx, result.c_str());
}

static JSValue js_logview_get_match_count(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    return JS_NewInt32(ctx, logview->GetMatchCount());
}

static JSValue js_logview_get_current_match(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* logview = TerminalBindings::UnwrapLogView(ctx, this_val);
    if (!logview) return JS_EXCEPTION;
    return JS_NewInt32(ctx, logview->GetCurrentMatch());
}

// LogView 属性和方法定义
static const JSCFunctionListEntry js_logview_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("maxEntries", js_logview_get_max_entries, js_logview_set_max_entries, 0),
    JS_CGETSET_MAGIC_DEF("autoScroll", js_logview_get_auto_scroll, js_logview_set_auto_scroll, 0),
    JS_CFUNC_DEF("append", 3, js_logview_append),
    JS_CFUNC_DEF("clear", 0, js_logview_clear),
    JS_CFUNC_DEF("scrollTo", 1, js_logview_scroll_to),
    JS_CFUNC_DEF("setLevelFilter", 1, js_logview_set_level_filter),
    JS_CFUNC_DEF("setSourceFilter", 1, js_logview_set_source_filter),
    JS_CFUNC_DEF("clearFilter", 0, js_logview_clear_filter),
    JS_CFUNC_DEF("search", 2, js_logview_search),
    JS_CFUNC_DEF("nextMatch", 0, js_logview_next_match),
    JS_CFUNC_DEF("prevMatch", 0, js_logview_prev_match),
    JS_CFUNC_DEF("clearSearch", 0, js_logview_clear_search),
    JS_CFUNC_DEF("export", 1, js_logview_export),
    JS_CFUNC_DEF("getMatchCount", 0, js_logview_get_match_count),
    JS_CFUNC_DEF("getCurrentMatch", 0, js_logview_get_current_match),
};

// ========== 初始化 ==========

void TerminalBindings::Init(JSContext* ctx) {
    if (initialized_) return;

    InitTerminalClass(ctx);
    InitLogViewClass(ctx);

    initialized_ = true;
}

void TerminalBindings::Cleanup(JSContext* ctx) {
    initialized_ = false;
}

void TerminalBindings::InitTerminalClass(JSContext* ctx) {
    JSRuntime* rt = JS_GetRuntime(ctx);

    // 注册 class ID
    JS_NewClassID(rt, &terminal_class_id);

    // 定义 class
    JSClassDef terminal_class_def = {
        .class_name = "HTMLTerminalElement",
        .finalizer = js_terminal_finalizer,
    };
    JS_NewClass(rt, terminal_class_id, &terminal_class_def);

    // 创建 prototype
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_terminal_proto_funcs,
                               sizeof(js_terminal_proto_funcs) / sizeof(js_terminal_proto_funcs[0]));
    JS_SetClassProto(ctx, terminal_class_id, proto);
}

void TerminalBindings::InitLogViewClass(JSContext* ctx) {
    JSRuntime* rt = JS_GetRuntime(ctx);

    JS_NewClassID(rt, &logview_class_id);

    JSClassDef logview_class_def = {
        .class_name = "HTMLLogViewElement",
        .finalizer = js_logview_finalizer,
    };
    JS_NewClass(rt, logview_class_id, &logview_class_def);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_logview_proto_funcs,
                               sizeof(js_logview_proto_funcs) / sizeof(js_logview_proto_funcs[0]));
    JS_SetClassProto(ctx, logview_class_id, proto);
}

JSValue TerminalBindings::WrapTerminal(JSContext* ctx, mbink::HTMLTerminalElement* terminal) {
    JSValue obj = JS_NewObjectClass(ctx, terminal_class_id);
    JS_SetOpaque(obj, terminal);
    return obj;
}

mbink::HTMLTerminalElement* TerminalBindings::UnwrapTerminal(JSContext* ctx, JSValue obj) {
    return static_cast<mbink::HTMLTerminalElement*>(JS_GetOpaque(obj, terminal_class_id));
}

JSValue TerminalBindings::WrapLogView(JSContext* ctx, mbink::HTMLLogViewElement* logview) {
    JSValue obj = JS_NewObjectClass(ctx, logview_class_id);
    JS_SetOpaque(obj, logview);
    return obj;
}

mbink::HTMLLogViewElement* TerminalBindings::UnwrapLogView(JSContext* ctx, JSValue obj) {
    return static_cast<mbink::HTMLLogViewElement*>(JS_GetOpaque(obj, logview_class_id));
}

}  // namespace mbink
