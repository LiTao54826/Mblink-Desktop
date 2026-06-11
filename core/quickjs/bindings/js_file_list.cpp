#include "js_file_list.h"

#include <cstdint>

namespace mbink {
namespace bindings {
namespace {

constexpr int kReadOnlyEnumerable = JS_PROP_ENUMERABLE;
constexpr int kReadOnlyHidden = 0;

struct JSFileListData {
    FileList files;
};

static JSClassID js_file_class_id = 0;
static JSClassID js_file_list_class_id = 0;

void JSFileListFinalizer(JSRuntime* rt, JSValue val) {
    (void)rt;
    auto* data = static_cast<JSFileListData*>(JS_GetOpaque(val, js_file_list_class_id));
    delete data;
}

static JSClassDef js_file_class = {
    /* class_name */ "File",
    /* finalizer */ nullptr,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr,
};

static JSClassDef js_file_list_class = {
    /* class_name */ "FileList",
    /* finalizer */ JSFileListFinalizer,
    /* gc_mark */ nullptr,
    /* call */ nullptr,
    /* exotic */ nullptr,
};

void DefineReadOnlyProperty(JSContext* ctx, JSValueConst obj, const char* name, JSValue value) {
    JS_DefinePropertyValueStr(ctx, obj, name, value, kReadOnlyEnumerable);
}

void DefineReadOnlyIndex(JSContext* ctx, JSValueConst obj, uint32_t index, JSValue value) {
    JS_DefinePropertyValueUint32(ctx, obj, index, value, kReadOnlyEnumerable);
}

void DefineToStringTag(JSContext* ctx, JSValueConst obj, const char* tag) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue symbol = JS_GetPropertyStr(ctx, global, "Symbol");
    JS_FreeValue(ctx, global);

    JSValue to_string_tag = JS_GetPropertyStr(ctx, symbol, "toStringTag");
    JS_FreeValue(ctx, symbol);

    JSAtom atom = JS_ValueToAtom(ctx, to_string_tag);
    JS_FreeValue(ctx, to_string_tag);
    if (atom != JS_ATOM_NULL) {
        JS_DefinePropertyValue(ctx, obj, atom, JS_NewString(ctx, tag), kReadOnlyHidden);
        JS_FreeAtom(ctx, atom);
    }
}

JSValue FileAt(JSContext* ctx, JSValueConst this_val, uint32_t index) {
    JSValue length_value = JS_GetPropertyStr(ctx, this_val, "length");
    uint32_t length = 0;
    JS_ToUint32(ctx, &length, length_value);
    JS_FreeValue(ctx, length_value);

    if (index >= length) {
        return JS_NULL;
    }

    return JS_GetPropertyUint32(ctx, this_val, index);
}

} // namespace

void InitFileListBinding(JSContext* ctx) {
    JSRuntime* rt = JS_GetRuntime(ctx);

    JS_NewClassID(rt, &js_file_class_id);
    JS_NewClass(rt, js_file_class_id, &js_file_class);
    JS_SetClassProto(ctx, js_file_class_id, JS_NewObject(ctx));

    JS_NewClassID(rt, &js_file_list_class_id);
    JS_NewClass(rt, js_file_list_class_id, &js_file_list_class);
    JS_SetClassProto(ctx, js_file_list_class_id, JS_NewObject(ctx));
}

JSValue WrapFile(JSContext* ctx, const FileInfo& file) {
    JSValue obj = JS_NewObjectClass(ctx, js_file_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    DefineReadOnlyProperty(ctx, obj, "name", JS_NewString(ctx, file.name.c_str()));
    DefineReadOnlyProperty(ctx, obj, "type", JS_NewString(ctx, file.type.c_str()));
    DefineReadOnlyProperty(ctx, obj, "size", JS_NewInt64(ctx, static_cast<int64_t>(file.size)));
    DefineReadOnlyProperty(ctx, obj, "lastModified", JS_NewInt64(ctx, file.last_modified));
    DefineReadOnlyProperty(ctx, obj, "webkitRelativePath",
                           JS_NewString(ctx, file.webkit_relative_path.c_str()));
    DefineToStringTag(ctx, obj, "File");
    return obj;
}

JSValue WrapFileList(JSContext* ctx, const FileList& files) {
    JSValue list = JS_NewObjectClass(ctx, js_file_list_class_id);
    if (JS_IsException(list)) {
        return list;
    }

    auto* data = new JSFileListData{files};
    JS_SetOpaque(list, data);

    for (uint32_t i = 0; i < files.size(); ++i) {
        DefineReadOnlyIndex(ctx, list, i, WrapFile(ctx, files[i]));
    }

    DefineReadOnlyProperty(ctx, list, "length", JS_NewUint32(ctx, static_cast<uint32_t>(files.size())));
    DefineReadOnlyProperty(ctx, list, "item",
        JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
            if (argc < 1) {
                return JS_NULL;
            }
            uint32_t index = 0;
            if (JS_ToUint32(ctx, &index, argv[0]) != 0) {
                return JS_NULL;
            }
            return FileAt(ctx, this_val, index);
        }, "item", 1));
    DefineToStringTag(ctx, list, "FileList");

    return list;
}

bool FileListFromJSValue(JSContext* ctx, JSValueConst value, FileList* files) {
    (void)ctx;
    if (!files || js_file_list_class_id == 0) {
        return false;
    }

    auto* data = static_cast<JSFileListData*>(JS_GetOpaque(value, js_file_list_class_id));
    if (!data) {
        return false;
    }

    *files = data->files;
    return true;
}

} // namespace bindings
} // namespace mbink
