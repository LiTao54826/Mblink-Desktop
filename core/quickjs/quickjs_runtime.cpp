/**
 * @file quickjs_runtime.cpp
 * @brief QuickJS运行时实现
 *
 * 实现内容：
 * - QuickJS运行时和上下文管理
 * - JavaScript代码执行
 * - C++和JavaScript互操作
 * - 数据类型转换
 */

#include "quickjs_runtime.h"
#include "quickjs-libc.h"
#include "core/utils/encoding_utils.h"
#include "tools/app_bundler/bytecode_compiler.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

namespace mbink {

namespace {

std::filesystem::path Utf8PathToFsPath(const std::string& path) {
#ifdef _WIN32
    return std::filesystem::path(utils::UTF8ToWide(path));
#else
    return std::filesystem::path(path);
#endif
}

std::string FsPathToUtf8String(const std::filesystem::path& path) {
#ifdef _WIN32
    return utils::WideToUTF8(path.wstring());
#else
    return path.string();
#endif
}

std::ifstream OpenUtf8FileForRead(const std::string& path) {
#ifdef _WIN32
    return std::ifstream(Utf8PathToFsPath(path), std::ios::binary);
#else
    return std::ifstream(path, std::ios::binary);
#endif
}

std::string NormalizeFsPath(const std::filesystem::path& path) {
    std::string result = FsPathToUtf8String(path.lexically_normal());
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

bool IsNativeModulePath(const std::string& path) {
    std::filesystem::path fs_path = Utf8PathToFsPath(path);
    std::string ext = FsPathToUtf8String(fs_path.extension());
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
#ifdef _WIN32
    return ext == ".dll";
#elif defined(__APPLE__)
    return ext == ".dylib" || ext == ".so";
#else
    return ext == ".so";
#endif
}

std::vector<std::string> GetNativeModuleExtensions() {
#ifdef _WIN32
    return {".dll"};
#elif defined(__APPLE__)
    return {".dylib", ".so"};
#else
    return {".so"};
#endif
}

}  // namespace

QuickJSRuntime::QuickJSRuntime() {
    InitRuntime();
    InitStdLib();
    InitConsole();
    InitModuleLoader();
    InitTimers();
}

QuickJSRuntime::~QuickJSRuntime() {
    // 清理所有待处理的任务和定时器
    // JSValueWrapper会在Task被销毁时自动释放JSValue
    active_timers_.clear();
    while (!task_queue_.empty()) {
        task_queue_.pop();
    }
    // Clear timer_queue_ (multimap)
    timer_queue_.clear();
    active_timers_.clear();

    // 先释放 context，再进行 runtime 级 GC，避免 context root 持有对象
    if (ctx_) {
        JS_FreeContext(ctx_);
        ctx_ = nullptr;
    }

    if (rt_) {
        // 多次 GC 尝试清理循环引用/延迟可回收对象
        for (int i = 0; i < 8; i++) {
            JS_RunGC(rt_);
        }

        JS_FreeRuntime(rt_);
        rt_ = nullptr;
    }
}

namespace {

constexpr uint32_t kMergedBytecodeMagic = 0x4342424d;  // MBBC

uint32_t ReadLe32(const std::string& data) {
    if (data.size() < 4) return 0;
    return static_cast<uint32_t>(static_cast<unsigned char>(data[0])) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[1])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[2])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[3])) << 24);
}

JSModuleDef* LoadMergedBytecodeModule(JSContext* ctx,
                                      const std::string& resolved_path,
                                      const std::string& module_data) {
    if (ReadLe32(module_data) != kMergedBytecodeMagic) {
        return nullptr;
    }

    std::vector<uint8_t> merged(module_data.begin() + 4, module_data.end());
    auto modules = mbink::BytecodeCompiler::ParseMergedBytecode(merged);
    if (modules.empty()) {
        JS_ThrowInternalError(ctx, "Empty merged bytecode: %s", resolved_path.c_str());
        return nullptr;
    }

    const mbink::CompiledModule* selected = nullptr;
    for (const auto& module : modules) {
        if (module.is_entry) {
            selected = &module;
            break;
        }
    }
    if (!selected) {
        selected = &modules.front();
    }

    JSValue obj = JS_ReadObject(ctx,
                                selected->bytecode.data(),
                                selected->bytecode.size(),
                                JS_READ_OBJ_BYTECODE);
    if (JS_IsException(obj)) {
        return nullptr;
    }
    if (JS_VALUE_GET_TAG(obj) != JS_TAG_MODULE) {
        JS_FreeValue(ctx, obj);
        JS_ThrowInternalError(ctx, "Bytecode is not a module: %s", resolved_path.c_str());
        return nullptr;
    }
    if (JS_ResolveModule(ctx, obj) < 0) {
        JS_FreeValue(ctx, obj);
        return nullptr;
    }

    JSModuleDef* module_def = reinterpret_cast<JSModuleDef*>(JS_VALUE_GET_PTR(obj));
    JS_FreeValue(ctx, obj);
    return module_def;
}

bool EvalMergedOrSingleBytecode(JSContext* ctx,
                                const std::string& resolved_path,
                                const std::string& module_data,
                                std::string& error) {
    if (ReadLe32(module_data) == kMergedBytecodeMagic) {
        std::vector<uint8_t> merged(module_data.begin() + 4, module_data.end());
        auto modules = mbink::BytecodeCompiler::ParseMergedBytecode(merged);
        if (modules.empty()) {
            error = "Empty merged bytecode: " + resolved_path;
            return false;
        }
        for (auto& module : modules) {
            JSValue obj = JS_ReadObject(ctx,
                                        module.bytecode.data(),
                                        module.bytecode.size(),
                                        JS_READ_OBJ_BYTECODE);
            if (JS_IsException(obj)) {
                error = "Failed to read merged bytecode: " + resolved_path;
                return false;
            }
            JSValue result = JS_EvalFunction(ctx, obj);
            if (JS_IsException(result)) {
                JSValue exc = JS_GetException(ctx);
                const char* err = JS_ToCString(ctx, exc);
                if (err) {
                    error = err;
                    JS_FreeCString(ctx, err);
                } else {
                    error = "Failed to eval merged bytecode: " + resolved_path;
                }
                JS_FreeValue(ctx, exc);
                JS_FreeValue(ctx, result);
                return false;
            }
            JS_FreeValue(ctx, result);
        }
        return true;
    }

    JSValue obj = JS_ReadObject(ctx,
                                reinterpret_cast<const uint8_t*>(module_data.data()),
                                module_data.size(),
                                JS_READ_OBJ_BYTECODE);
    if (JS_IsException(obj)) {
        error = "Failed to read bytecode: " + resolved_path;
        return false;
    }
    JSValue result = JS_EvalFunction(ctx, obj);
    if (JS_IsException(result)) {
        JSValue exc = JS_GetException(ctx);
        const char* err = JS_ToCString(ctx, exc);
        if (err) {
            error = err;
            JS_FreeCString(ctx, err);
        } else {
            error = "Failed to eval bytecode: " + resolved_path;
        }
        JS_FreeValue(ctx, exc);
        JS_FreeValue(ctx, result);
        return false;
    }
    JS_FreeValue(ctx, result);
    return true;
}
}

json QuickJSRuntime::Eval(const std::string& code, const std::string& filename) {
    JSValue result = JS_Eval(ctx_, code.c_str(), code.length(),
                             filename.c_str(), JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        std::string error = GetJSError();
        JS_FreeValue(ctx_, result);
        throw std::runtime_error("JavaScript error: " + error);
    }
    json ret = JSValueToJSON(result);
    JS_FreeValue(ctx_, result);
    return ret;
}

json QuickJSRuntime::EvalModule(const std::string& code, const std::string& filename) {
    JSValue result = JS_Eval(ctx_, code.c_str(), code.length(),
                             filename.c_str(), JS_EVAL_TYPE_MODULE);
    if (JS_IsException(result)) {
        std::string error = GetJSError();
        JS_FreeValue(ctx_, result);
        throw std::runtime_error("JavaScript module error: " + error);
    }
    json ret = JSValueToJSON(result);
    JS_FreeValue(ctx_, result);
    return ret;
}

json QuickJSRuntime::EvalFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return Eval(buffer.str(), filepath);
}

void QuickJSRuntime::RegisterFunction(const std::string& name, NativeFunction func) {
    native_functions_[name] = func;

    // Create function name as JSValue for func_data
    JSValue func_name = JS_NewString(ctx_, name.c_str());

    JSValue func_obj = JS_NewCFunctionData(
        ctx_, NativeFunctionWrapper, 0, 0, 1, &func_name
    );

    // JS_NewCFunctionData internally duplicates func_data values,
    // so we must release our local reference to avoid leaks.
    JS_FreeValue(ctx_, func_name);

    JSValue global = JS_GetGlobalObject(ctx_);
    JS_SetPropertyStr(ctx_, global, name.c_str(), func_obj);
    JS_FreeValue(ctx_, global);
}

json QuickJSRuntime::CallFunction(const std::string& func_name, const json& args) {
    JSValue global = JS_GetGlobalObject(ctx_);
    JSValue func = JS_GetPropertyStr(ctx_, global, func_name.c_str());

    if (!JS_IsFunction(ctx_, func)) {
        JS_FreeValue(ctx_, func);
        JS_FreeValue(ctx_, global);
        throw std::runtime_error("Not a function: " + func_name);
    }

    // Convert args array to JSValue array
    std::vector<JSValue> js_args_vec;
    if (args.is_array()) {
        for (const auto& arg : args) {
            js_args_vec.push_back(JSONToJSValue(arg));
        }
    }

    JSValue result = JS_Call(ctx_, func, global, js_args_vec.size(),
                            js_args_vec.empty() ? nullptr : js_args_vec.data());

    // Free arguments
    for (auto& arg : js_args_vec) {
        JS_FreeValue(ctx_, arg);
    }
    JS_FreeValue(ctx_, func);
    JS_FreeValue(ctx_, global);

    if (JS_IsException(result)) {
        std::string error = GetJSError();
        JS_FreeValue(ctx_, result);
        throw std::runtime_error("Function call error: " + error);
    }

    json ret = JSValueToJSON(result);
    JS_FreeValue(ctx_, result);
    return ret;
}

json QuickJSRuntime::GetGlobalProperty(const std::string& name) {
    JSValue global = JS_GetGlobalObject(ctx_);
    JSValue value = JS_GetPropertyStr(ctx_, global, name.c_str());
    JS_FreeValue(ctx_, global);

    json ret = JSValueToJSON(value);
    JS_FreeValue(ctx_, value);
    return ret;
}

void QuickJSRuntime::SetGlobalProperty(const std::string& name, const json& value) {
    JSValue global = JS_GetGlobalObject(ctx_);
    JSValue js_value = JSONToJSValue(value);
    JS_SetPropertyStr(ctx_, global, name.c_str(), js_value);
    JS_FreeValue(ctx_, global);
}

json QuickJSRuntime::JSValueToJSON(JSValue value) {
    int tag = JS_VALUE_GET_TAG(value);

    // Handle undefined
    if (JS_IsUndefined(value)) {
        return nullptr;
    }

    // Handle null
    if (JS_IsNull(value)) {
        return nullptr;
    }

    // Handle boolean
    if (JS_IsBool(value)) {
        return JS_VALUE_GET_BOOL(value) != 0;
    }

    // Handle number
    if (JS_IsNumber(value)) {
        if (tag == JS_TAG_INT) {
            return JS_VALUE_GET_INT(value);
        } else {
            double d;
            JS_ToFloat64(ctx_, &d, value);
            return d;
        }
    }

    // Handle string
    if (JS_IsString(value)) {
        const char* str = JS_ToCString(ctx_, value);
        if (str) {
            std::string result(str);
            JS_FreeCString(ctx_, str);
            return result;
        }
        return "";
    }

    // Handle array
    if (JS_IsArray(value)) {
        json arr = json::array();
        JSValue length_val = JS_GetPropertyStr(ctx_, value, "length");
        int32_t length = 0;
        JS_ToInt32(ctx_, &length, length_val);
        JS_FreeValue(ctx_, length_val);

        for (int32_t i = 0; i < length; i++) {
            JSValue item = JS_GetPropertyUint32(ctx_, value, i);
            arr.push_back(JSValueToJSON(item));
            JS_FreeValue(ctx_, item);
        }
        return arr;
    }

    // Handle object
    if (JS_IsObject(value)) {
        json obj = json::object();
        JSPropertyEnum* props = nullptr;
        uint32_t prop_count = 0;

        if (JS_GetOwnPropertyNames(ctx_, &props, &prop_count, value,
                                   JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
            for (uint32_t i = 0; i < prop_count; i++) {
                const char* key = JS_AtomToCString(ctx_, props[i].atom);
                if (key) {
                    JSValue prop_val = JS_GetProperty(ctx_, value, props[i].atom);
                    obj[key] = JSValueToJSON(prop_val);
                    JS_FreeValue(ctx_, prop_val);
                    JS_FreeCString(ctx_, key);
                }
                JS_FreeAtom(ctx_, props[i].atom);
            }
            js_free(ctx_, props);
        }
        return obj;
    }

    return nullptr;
}

JSValue QuickJSRuntime::JSONToJSValue(const json& value) {
    // Handle null
    if (value.is_null()) {
        return JS_NULL;
    }

    // Handle boolean
    if (value.is_boolean()) {
        return JS_NewBool(ctx_, value.get<bool>());
    }

    // Handle number
    if (value.is_number_integer()) {
        return JS_NewInt64(ctx_, value.get<int64_t>());
    }
    if (value.is_number_float()) {
        return JS_NewFloat64(ctx_, value.get<double>());
    }

    // Handle string
    if (value.is_string()) {
        std::string str = value.get<std::string>();
        return JS_NewStringLen(ctx_, str.c_str(), str.length());
    }

    // Handle array
    if (value.is_array()) {
        JSValue arr = JS_NewArray(ctx_);
        uint32_t idx = 0;
        for (const auto& item : value) {
            JSValue js_item = JSONToJSValue(item);
            JS_SetPropertyUint32(ctx_, arr, idx++, js_item);
        }
        return arr;
    }

    // Handle object
    if (value.is_object()) {
        JSValue obj = JS_NewObject(ctx_);
        for (auto it = value.begin(); it != value.end(); ++it) {
            JSValue js_val = JSONToJSValue(it.value());
            JS_SetPropertyStr(ctx_, obj, it.key().c_str(), js_val);
        }
        return obj;
    }

    return JS_UNDEFINED;
}

void QuickJSRuntime::InitRuntime() {
    rt_ = JS_NewRuntime();
    if (!rt_) {
        throw std::runtime_error("Failed to create QuickJS runtime");
    }

    // Set memory limit (256MB)
    JS_SetMemoryLimit(rt_, 256 * 1024 * 1024);

    // Set max stack size (1MB)
    JS_SetMaxStackSize(rt_, 1024 * 1024);

    ctx_ = JS_NewContext(rt_);
    if (!ctx_) {
        JS_FreeRuntime(rt_);
        rt_ = nullptr;
        throw std::runtime_error("Failed to create QuickJS context");
    }

    // Set context opaque to this runtime instance for native function wrapper
    JS_SetContextOpaque(ctx_, this);
}

void QuickJSRuntime::InitStdLib() {
    // Note: QuickJS libc functions can cause issues on Windows
    // For now, we'll skip them and implement our own standard library later
    // TODO: Implement custom standard library functions

    // js_std_init_handlers(rt_);
    // js_std_add_helpers(ctx_, 0, nullptr);
    // js_init_module_std(ctx_, "std");
    // js_init_module_os(ctx_, "os");

    // Add console API
    RegisterFunction("print", [](const json& args) -> json {
        return nullptr;
    });
}

std::string QuickJSRuntime::GetJSError() {
    JSValue exception = JS_GetException(ctx_);

    std::string error_msg;

    // Get error message
    const char* str = JS_ToCString(ctx_, exception);
    if (str) {
        error_msg = str;
        JS_FreeCString(ctx_, str);
    }

    // Try to get stack trace
    if (JS_IsObject(exception)) {
        JSValue stack = JS_GetPropertyStr(ctx_, exception, "stack");
        if (JS_IsString(stack)) {
            const char* stack_str = JS_ToCString(ctx_, stack);
            if (stack_str) {
                error_msg += "\n" + std::string(stack_str);
                JS_FreeCString(ctx_, stack_str);
            }
        }
        JS_FreeValue(ctx_, stack);
    }

    JS_FreeValue(ctx_, exception);
    return error_msg.empty() ? "Unknown error" : error_msg;
}

void QuickJSRuntime::InitConsole() {
    // Create console object
    JSValue global = JS_GetGlobalObject(ctx_);
    JSValue console = JS_NewObject(ctx_);

    // Add console.log (magic = 0)
    JSValue log_func = JS_NewCFunctionMagic(ctx_, ConsoleLog, "log", 0, JS_CFUNC_generic_magic, 0);
    JS_SetPropertyStr(ctx_, console, "log", log_func);

    // Add console.error (magic = 1)
    JSValue error_func = JS_NewCFunctionMagic(ctx_, ConsoleLog, "error", 0, JS_CFUNC_generic_magic, 1);
    JS_SetPropertyStr(ctx_, console, "error", error_func);

    // Add console.warn (magic = 2)
    JSValue warn_func = JS_NewCFunctionMagic(ctx_, ConsoleLog, "warn", 0, JS_CFUNC_generic_magic, 2);
    JS_SetPropertyStr(ctx_, console, "warn", warn_func);

    // Add console.info (magic = 3)
    JSValue info_func = JS_NewCFunctionMagic(ctx_, ConsoleLog, "info", 0, JS_CFUNC_generic_magic, 3);
    JS_SetPropertyStr(ctx_, console, "info", info_func);

    // Set console as global property
    JS_SetPropertyStr(ctx_, global, "console", console);
    JS_FreeValue(ctx_, global);
}

JSValue QuickJSRuntime::ConsoleLog(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv, int magic) {
    // magic: 0=log, 1=error, 2=warn, 3=info
    FILE* out = (magic == 1) ? stderr : stdout;

    for (int i = 0; i < argc; i++) {
        if (i > 0) fputc(' ', out);
        const char* str = JS_ToCString(ctx, argv[i]);
        if (str) {
            fputs(str, out);
            JS_FreeCString(ctx, str);
        }
    }
    fputc('\n', out);
    fflush(out);

    return JS_UNDEFINED;
}

void QuickJSRuntime::InitModuleLoader() {
    // Set module loader function with normalize function
    JS_SetModuleLoaderFunc(rt_, ModuleNormalize, ModuleLoader, this);
}

JSModuleDef* QuickJSRuntime::ModuleLoader(JSContext* ctx, const char* module_name, void* opaque) {
    QuickJSRuntime* runtime = static_cast<QuickJSRuntime*>(opaque);
    if (!runtime) {
        JS_ThrowInternalError(ctx, "Runtime not found in module loader");
        return nullptr;
    }

    // 1. 优先从注册表查找（精确匹配）
    auto it = runtime->module_registry_.find(module_name);
    if (it != runtime->module_registry_.end()) {
        const std::string& module_code = it->second;
        JSValue module_val = JS_Eval(ctx, module_code.c_str(), module_code.length(),
                                     module_name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (JS_IsException(module_val)) {
            return nullptr;
        }
        JSModuleDef* m = (JSModuleDef*)JS_VALUE_GET_PTR(module_val);
        JS_FreeValue(ctx, module_val);
        return m;
    }

    // 2. 解析路径并尝试从文件系统加载
    std::string resolved_path;
    if (!runtime->ResolveModulePath(module_name, resolved_path)) {
        JS_ThrowReferenceError(ctx, "Cannot resolve module: %s", module_name);
        return nullptr;
    }

    // 3. 检查解析后的路径是否已注册（避免重复加载）
    it = runtime->module_registry_.find(resolved_path);
    if (it != runtime->module_registry_.end()) {
        const std::string& module_code = it->second;
        JSValue module_val = JS_Eval(ctx, module_code.c_str(), module_code.length(),
                                     resolved_path.c_str(), JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (JS_IsException(module_val)) {
            return nullptr;
        }
        JSModuleDef* m = (JSModuleDef*)JS_VALUE_GET_PTR(module_val);
        JS_FreeValue(ctx, module_val);
        return m;
    }

    // 4. native module 走 quickjs-ng 官方 loader，其余保持现有 JS 模块逻辑
    if (IsNativeModulePath(resolved_path)) {
        return js_module_loader(ctx, resolved_path.c_str(), nullptr);
    }

    // 5. 优先通过 file loader 读取，其次回退文件系统
    std::string module_code;
    if (runtime->file_loader_) {
        std::string loader_error;
        if (!runtime->file_loader_(resolved_path, module_code, &loader_error)) {
            std::ifstream file = OpenUtf8FileForRead(resolved_path);
            if (!file.is_open()) {
                JS_ThrowReferenceError(ctx, "Module file not found: %s", resolved_path.c_str());
                return nullptr;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            module_code = buffer.str();
        }
    } else {
        std::ifstream file = OpenUtf8FileForRead(resolved_path);
        if (!file.is_open()) {
            JS_ThrowReferenceError(ctx, "Module file not found: %s", resolved_path.c_str());
            return nullptr;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        module_code = buffer.str();
    }

    if (JSModuleDef* bytecode_module = LoadMergedBytecodeModule(ctx, resolved_path, module_code)) {
        return bytecode_module;
    }

    // 5. 注册并编译
    runtime->module_registry_[resolved_path] = module_code;
    
    // 更新基础路径为当前加载的模块路径（供其依赖项使用）
    std::string old_base_path = runtime->base_module_path_;
    runtime->base_module_path_ = resolved_path;
    
    JSValue module_val = JS_Eval(ctx, module_code.c_str(), module_code.length(),
                                 resolved_path.c_str(), JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    
    // 恢复旧的基础路径
    runtime->base_module_path_ = old_base_path;
    
    if (JS_IsException(module_val)) {
        return nullptr;
    }

    JSModuleDef* m = (JSModuleDef*)JS_VALUE_GET_PTR(module_val);
    JS_FreeValue(ctx, module_val);
    return m;
}

char* QuickJSRuntime::ModuleNormalize(JSContext* ctx, const char* module_base,
                                       const char* module_name, void* opaque) {
    QuickJSRuntime* runtime = static_cast<QuickJSRuntime*>(opaque);
    if (!runtime) {
        return nullptr;
    }

    std::string name(module_name);
    std::string base(module_base ? module_base : "");
    std::string resolved_path;
    
    // 如果是绝对路径，解析为文件夹或文件
    if ((name.length() > 1 && name[1] == ':') || (name.length() > 0 && name[0] == '/')) {
        resolved_path = runtime->ResolveFolderOrFileWithLoader(name);
        return js_strdup(ctx, resolved_path.c_str());
    }
    
    // 如果是相对路径，基于 base 解析
    if (name.rfind("./", 0) == 0 || name.rfind("../", 0) == 0) {
        if (base.empty()) {
            // 如果没有 base，使用当前设置的 base_module_path_
            if (runtime->base_module_path_.empty()) {
                return js_strdup(ctx, module_name);
            }
            base = runtime->base_module_path_;
        }
        
        std::filesystem::path base_path = Utf8PathToFsPath(base);
        std::filesystem::path relative_path = Utf8PathToFsPath(name);
        std::filesystem::path resolved = (base_path.parent_path() / relative_path).lexically_normal();
        
        // 转换为字符串并标准化为正斜杠
        std::string resolved_str = NormalizeFsPath(resolved);
        
        // 检查是否是文件夹，如果是则尝试解析 package.json 或 index.js
        resolved_path = runtime->ResolveFolderOrFileWithLoader(resolved_str);
        return js_strdup(ctx, resolved_path.c_str());
    }
    
    // 裸模块名，尝试 node_modules 查找
    if (!name.empty() && name[0] != '.' && name[0] != '/') {
        std::string base_for_search = base.empty() ? runtime->base_module_path_ : base;
        if (!base_for_search.empty()) {
            resolved_path = ResolveNodeModules(name, base_for_search);
            if (!resolved_path.empty()) {
                return js_strdup(ctx, resolved_path.c_str());
            }
        }
    }
    
    // 未能解析，返回原名称
    return js_strdup(ctx, module_name);
}

std::string QuickJSRuntime::ResolveFolderOrFile(const std::string& path) {
    namespace fs = std::filesystem;
    fs::path fs_path = Utf8PathToFsPath(path);

    // 1. 如果路径已经是文件，直接返回
    if (fs::is_regular_file(fs_path)) {
        return NormalizeFsPath(fs_path);
    }

    // 2. 尝试补常见扩展名
    if (!path.empty() && path.find('.') == std::string::npos) {
        fs::path with_js = fs_path;
        with_js += ".js";
        if (fs::is_regular_file(with_js)) {
            return NormalizeFsPath(with_js);
        }
        for (const auto& ext : GetNativeModuleExtensions()) {
            fs::path with_native = fs_path;
            with_native += ext;
            if (fs::is_regular_file(with_native)) {
                return NormalizeFsPath(with_native);
            }
        }
    }

    // 3. 如果路径不存在，尝试作为文件或目录处理
    if (!fs::exists(fs_path)) {
        return path;  // 返回原路径，稍后会报错
    }

    // 4. 如果是目录，尝试解析 package
    if (fs::is_directory(fs_path)) {
        return ResolvePackageDirectory(NormalizeFsPath(fs_path));
    }

    return NormalizeFsPath(fs_path);
}

std::string QuickJSRuntime::ResolveFolderOrFileWithLoader(const std::string& path) const {
    if (file_loader_) {
        std::string data;
        if (file_loader_(path, data, nullptr)) {
            return path;
        }
        if (!path.empty() && path.find('.') == std::string::npos) {
            const std::string with_js = path + ".js";
            if (file_loader_(with_js, data, nullptr)) {
                return with_js;
            }
            for (const auto& ext : GetNativeModuleExtensions()) {
                const std::string with_native = path + ext;
                if (file_loader_(with_native, data, nullptr)) {
                    return with_native;
                }
            }
        }
        return ResolvePackageDirectoryWithLoader(path);
    }
    return ResolveFolderOrFile(path);
}

std::string QuickJSRuntime::ResolvePackageDirectory(const std::string& dir_path) {
    namespace fs = std::filesystem;
    fs::path dir_fs_path = Utf8PathToFsPath(dir_path);

    fs::path package_json = dir_fs_path / "package.json";
    if (!fs::exists(package_json)) {
        // 没有 package.json，尝试默认入口
        fs::path index_js = dir_fs_path / "index.js";
        if (fs::exists(index_js)) {
            return NormalizeFsPath(index_js);
        }
        for (const auto& ext : GetNativeModuleExtensions()) {
            fs::path index_native = dir_fs_path / ("index" + ext);
            if (fs::exists(index_native)) {
                return NormalizeFsPath(index_native);
            }
        }
        return NormalizeFsPath(dir_fs_path);
    }

    // 读取并解析 package.json
    std::ifstream file(Utf8PathToFsPath(FsPathToUtf8String(package_json)), std::ios::binary);
    if (!file.is_open()) {
        return dir_path;
    }

    try {
        json pkg = json::parse(file);

        // 优先使用 exports 字段（新标准）
        if (pkg.contains("exports")) {
            std::string exports_result = ResolvePackageExports(pkg["exports"], dir_path, ".");
            if (!exports_result.empty()) {
                return ResolveFolderOrFile(exports_result);
            }
        }

        // 回退到 main 字段
        if (pkg.contains("main") && pkg["main"].is_string()) {
            std::string main_file = pkg["main"].get<std::string>();
            fs::path main_path = dir_fs_path / Utf8PathToFsPath(main_file);
            return ResolveFolderOrFile(NormalizeFsPath(main_path));
        }
    } catch (...) {
        // JSON 解析失败
    }

    // 最后尝试默认入口
    fs::path index_js = dir_fs_path / "index.js";
    if (fs::exists(index_js)) {
        return NormalizeFsPath(index_js);
    }
    for (const auto& ext : GetNativeModuleExtensions()) {
        fs::path index_native = dir_fs_path / ("index" + ext);
        if (fs::exists(index_native)) {
            return NormalizeFsPath(index_native);
        }
    }

    return NormalizeFsPath(dir_fs_path);
}

std::string QuickJSRuntime::ResolvePackageDirectoryWithLoader(const std::string& dir_path) const {
    if (!file_loader_) {
        return ResolvePackageDirectory(dir_path);
    }

    std::string data;
    std::string normalized = dir_path;
    if (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }

    const std::string package_json = normalized + "/package.json";
    if (file_loader_(package_json, data, nullptr)) {
        try {
            json pkg = json::parse(data);
            if (pkg.contains("exports")) {
                std::string exports_result = ResolvePackageExports(pkg["exports"], normalized, ".");
                if (!exports_result.empty()) {
                    return ResolveFolderOrFileWithLoader(exports_result);
                }
            }
            if (pkg.contains("main") && pkg["main"].is_string()) {
                std::string main_file = pkg["main"].get<std::string>();
                if (main_file.rfind("./", 0) == 0) {
                    main_file = main_file.substr(2);
                }
                return ResolveFolderOrFileWithLoader(normalized + "/" + main_file);
            }
        } catch (...) {
        }
    }

    const std::string index_js = normalized + "/index.js";
    if (file_loader_(index_js, data, nullptr)) {
        return index_js;
    }
    for (const auto& ext : GetNativeModuleExtensions()) {
        const std::string index_native = normalized + "/index" + ext;
        if (file_loader_(index_native, data, nullptr)) {
            return index_native;
        }
    }
    return dir_path;
}

std::string QuickJSRuntime::ResolvePackageExports(const json& exports, 
                                                   const std::string& package_dir,
                                                   const std::string& subpath) {
    namespace fs = std::filesystem;
    
    // exports 可以是字符串、对象或null
    if (exports.is_string()) {
        // 简单情况: "exports": "./dist/index.js"
        std::string export_path = exports.get<std::string>();
        if (export_path.rfind("./", 0) == 0) {
            export_path = export_path.substr(2);  // 去掉 "./"
        }
        fs::path full_path = Utf8PathToFsPath(package_dir) / Utf8PathToFsPath(export_path);
        return NormalizeFsPath(full_path);
    }
    
    if (exports.is_object()) {
        // 对象情况: { ".": "./dist/index.js", "./utils": "./dist/utils.js" }
        if (exports.contains(subpath)) {
            const json& target = exports[subpath];
            if (target.is_string()) {
                std::string export_path = target.get<std::string>();
                if (export_path.rfind("./", 0) == 0) {
                    export_path = export_path.substr(2);
                }
                fs::path full_path = Utf8PathToFsPath(package_dir) / Utf8PathToFsPath(export_path);
                return NormalizeFsPath(full_path);
            }
        }
        
        // 条件导出: { "import": "...", "require": "..." }
        // 优先使用 "import" (ES6)
        if (exports.contains("import") && exports["import"].is_string()) {
            std::string export_path = exports["import"].get<std::string>();
            if (export_path.rfind("./", 0) == 0) {
                export_path = export_path.substr(2);
            }
            fs::path full_path = Utf8PathToFsPath(package_dir) / Utf8PathToFsPath(export_path);
            return NormalizeFsPath(full_path);
        }
    }
    
    return "";
}

std::string QuickJSRuntime::ResolveNodeModules(const std::string& module_name,
                                                const std::string& start_path) {
    namespace fs = std::filesystem;

    // 从 start_path 开始向上遍历，查找 node_modules
    fs::path current_dir = Utf8PathToFsPath(start_path).parent_path();

    while (!current_dir.empty() && current_dir.has_parent_path()) {
        // 检查当前目录的 node_modules
        fs::path node_modules = current_dir / "node_modules" / module_name;

        if (fs::exists(node_modules)) {
            if (fs::is_directory(node_modules)) {
                // 如果是目录，尝试解析为 package
                std::string resolved = ResolvePackageDirectory(FsPathToUtf8String(node_modules));
                return resolved;
            } else if (fs::is_regular_file(node_modules)) {
                // 如果是文件，直接返回
                return NormalizeFsPath(node_modules);
            }
        }

        // 尝试添加常见扩展名
        fs::path node_modules_js = current_dir / "node_modules" / (module_name + ".js");
        if (fs::is_regular_file(node_modules_js)) {
            return NormalizeFsPath(node_modules_js);
        }
        for (const auto& ext : GetNativeModuleExtensions()) {
            fs::path node_modules_native = current_dir / "node_modules" / (module_name + ext);
            if (fs::is_regular_file(node_modules_native)) {
                return NormalizeFsPath(node_modules_native);
            }
        }

        // 向上一级目录
        fs::path parent = current_dir.parent_path();
        if (parent == current_dir) {
            break;  // 已到达根目录
        }
        current_dir = parent;
    }

    return "";  // 未找到
}

void QuickJSRuntime::RegisterModule(const std::string& module_name, const std::string& module_code) {
    module_registry_[module_name] = module_code;
}

json QuickJSRuntime::LoadModule(const std::string& module_name) {
    // Check if module is registered
    auto it = module_registry_.find(module_name);
    if (it == module_registry_.end()) {
        throw std::runtime_error("Module not found: " + module_name);
    }

    const std::string& module_code = it->second;

    // 设置基础路径为当前模块路径（供其依赖项使用）
    std::string old_base_path = base_module_path_;
    base_module_path_ = module_name;

    // Evaluate the module
    JSValue result = JS_Eval(ctx_, module_code.c_str(), module_code.length(),
                            module_name.c_str(), JS_EVAL_TYPE_MODULE);

    // 恢复旧的基础路径
    base_module_path_ = old_base_path;

    if (JS_IsException(result)) {
        std::string error = GetJSError();
        JS_FreeValue(ctx_, result);
        throw std::runtime_error("Module load error: " + error);
    }

    // Get module namespace
    json ret = JSValueToJSON(result);
    JS_FreeValue(ctx_, result);

    return ret;
}

json QuickJSRuntime::LoadModuleFile(const std::string& filepath) {
    std::string normalized_abs_path = ResolveFolderOrFileWithLoader(filepath);
    std::string module_code;
    if (file_loader_ && file_loader_(normalized_abs_path, module_code, nullptr)) {
    } else {
        std::ifstream file = OpenUtf8FileForRead(normalized_abs_path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open module file: " + filepath);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        module_code = buffer.str();
        std::filesystem::path abs_path = std::filesystem::absolute(Utf8PathToFsPath(normalized_abs_path));
        normalized_abs_path = NormalizeFsPath(abs_path);
    }

    SetBaseModulePath(normalized_abs_path);

    if (ReadLe32(module_code) == kMergedBytecodeMagic) {
        std::string error;
        if (!EvalMergedOrSingleBytecode(ctx_, normalized_abs_path, module_code, error)) {
            throw std::runtime_error(error.empty() ? ("Failed to eval bytecode module: " + normalized_abs_path) : error);
        }
        return json();
    }

    // Register and load the module
    RegisterModule(normalized_abs_path, module_code);
    return LoadModule(normalized_abs_path);
}

void QuickJSRuntime::SetBaseModulePath(const std::string& path) {
    base_module_path_ = path;
}

void QuickJSRuntime::SetFileLoader(FileLoader loader) {
    file_loader_ = std::move(loader);
}

bool QuickJSRuntime::ResolveModulePath(const char* module_name, std::string& resolved_path) {
    std::string name(module_name);
    
    // 已是绝对路径（Windows: C:/... 或 Linux: /...）
    if ((name.length() > 1 && name[1] == ':') || (name.length() > 0 && name[0] == '/')) {
        resolved_path = ResolveFolderOrFileWithLoader(name);
        return true;
    }
    
    // 相对路径：基于 base_module_path_ 解析
    if (name.rfind("./", 0) == 0 || name.rfind("../", 0) == 0) {
        if (base_module_path_.empty()) {
            // 没有基础路径，无法解析相对路径
            return false;
        }
        
        std::filesystem::path base(Utf8PathToFsPath(base_module_path_));
        std::filesystem::path relative(Utf8PathToFsPath(name));
        std::filesystem::path resolved = (base.parent_path() / relative).lexically_normal();

        // 转换路径为字符串并确保使用正斜杠（跨平台兼容）
        resolved_path = ResolveFolderOrFileWithLoader(NormalizeFsPath(resolved));
        return true;
    }
    
    // 裸模块名（如 "preact"）- 暂不支持 node_modules 查找
    return false;
}

// ============================================================================
// Async Task Queue Implementation
// ============================================================================

void QuickJSRuntime::InitTimers() {
    JSValue global = JS_GetGlobalObject(ctx_);

    // Add setTimeout
    JSValue setTimeout_func = JS_NewCFunction(ctx_, SetTimeout, "setTimeout", 2);
    JS_SetPropertyStr(ctx_, global, "setTimeout", setTimeout_func);

    // Add setInterval
    JSValue setInterval_func = JS_NewCFunction(ctx_, SetInterval, "setInterval", 2);
    JS_SetPropertyStr(ctx_, global, "setInterval", setInterval_func);

    // Add clearTimeout
    JSValue clearTimeout_func = JS_NewCFunction(ctx_, ClearTimer, "clearTimeout", 1);
    JS_SetPropertyStr(ctx_, global, "clearTimeout", clearTimeout_func);

    // Add clearInterval
    JSValue clearInterval_func = JS_NewCFunction(ctx_, ClearTimer, "clearInterval", 1);
    JS_SetPropertyStr(ctx_, global, "clearInterval", clearInterval_func);

    JS_FreeValue(ctx_, global);
}

int64_t QuickJSRuntime::GetCurrentTimeMs() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return duration.count();
}

int QuickJSRuntime::CreateTimer(JSValue callback, int64_t delay, bool repeat,
                                const std::vector<JSValue>& args) {
    Task task;
    task.id = next_task_id_++;
    task.callback = std::make_shared<JSValueWrapper>(ctx_, callback);
    task.execute_time = GetCurrentTimeMs() + delay;
    task.repeat = repeat;
    task.interval = delay;
    task.cancelled = false;

    // Wrap arguments in JSValueWrapper
    for (const auto& arg : args) {
        task.args.push_back(std::make_shared<JSValueWrapper>(ctx_, arg));
    }

    // Add to timer queue (multimap allows efficient insertion and deletion)
    auto it = timer_queue_.insert(std::make_pair(task.execute_time, task));

    // Store iterator in active_timers for O(1) deletion
    active_timers_[task.id] = it;

    return task.id;
}

JSValue QuickJSRuntime::SetTimeout(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    QuickJSRuntime* runtime = static_cast<QuickJSRuntime*>(JS_GetContextOpaque(ctx));
    if (!runtime) {
        return JS_ThrowInternalError(ctx, "Runtime not found");
    }

    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "First argument must be a function");
    }

    // Get delay (default 0)
    int64_t delay = 0;
    if (argc >= 2) {
        int32_t delay_int;
        if (JS_ToInt32(ctx, &delay_int, argv[1]) == 0) {
            delay = std::max(0, delay_int);
        }
    }

    // Get additional arguments
    std::vector<JSValue> args;
    for (int i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }

    int timer_id = runtime->CreateTimer(argv[0], delay, false, args);

    return JS_NewInt32(ctx, timer_id);
}

JSValue QuickJSRuntime::SetInterval(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv) {
    QuickJSRuntime* runtime = static_cast<QuickJSRuntime*>(JS_GetContextOpaque(ctx));
    if (!runtime) {
        return JS_ThrowInternalError(ctx, "Runtime not found");
    }

    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "First argument must be a function");
    }

    // Get interval (default 0)
    int64_t interval = 0;
    if (argc >= 2) {
        int32_t interval_int;
        if (JS_ToInt32(ctx, &interval_int, argv[1]) == 0) {
            interval = std::max(0, interval_int);
        }
    }

    // Get additional arguments
    std::vector<JSValue> args;
    for (int i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }

    int timer_id = runtime->CreateTimer(argv[0], interval, true, args);

    return JS_NewInt32(ctx, timer_id);
}

JSValue QuickJSRuntime::ClearTimer(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    QuickJSRuntime* runtime = static_cast<QuickJSRuntime*>(JS_GetContextOpaque(ctx));
    if (!runtime) {
        return JS_ThrowInternalError(ctx, "Runtime not found");
    }

    if (argc < 1) {
        return JS_UNDEFINED;
    }

    int32_t timer_id;
    if (JS_ToInt32(ctx, &timer_id, argv[0]) != 0) {
        return JS_UNDEFINED;
    }

    // Find timer in active_timers
    auto it = runtime->active_timers_.find(timer_id);
    if (it != runtime->active_timers_.end()) {
        // Get iterator to timer_queue_ entry
        auto timer_it = it->second;

        // Remove from timer_queue_ immediately (O(1) operation with iterator)
        runtime->timer_queue_.erase(timer_it);

        // Remove from active_timers
        runtime->active_timers_.erase(it);

        // JSValueWrapper destructors will automatically free callback and arguments
    }

    return JS_UNDEFINED;
}

void QuickJSRuntime::ProcessTasks() {
    while (!task_queue_.empty()) {
        Task task = task_queue_.front();
        task_queue_.pop();

        if (!task.cancelled) {
            // Prepare arguments array for JS_Call
            std::vector<JSValue> js_args;
            for (const auto& arg_wrapper : task.args) {
                js_args.push_back(arg_wrapper->Get());
            }

            // Call the callback
            JSValue result = JS_Call(ctx_, task.callback->Get(), JS_UNDEFINED,
                                    js_args.size(), js_args.data());

            if (JS_IsException(result)) {
                // Log error but continue
                std::string error = GetJSError();
            }

            JS_FreeValue(ctx_, result);
        }

        // JSValueWrapper destructors will automatically free callback and arguments
        // when task goes out of scope
    }
}

void QuickJSRuntime::ProcessMicrotasks() {
    JSContext* ctx;
    int ret;

    // Execute all pending jobs (Promise callbacks, etc.)
    while (true) {
        ret = JS_ExecutePendingJob(rt_, &ctx);
        if (ret == 0) {
            break;  // No more jobs
        }
        if (ret < 0) {
            // Error occurred
            std::string error = GetJSError();
            break;
        }
    }
}

void QuickJSRuntime::RunGC() {
    JS_RunGC(rt_);
}

json QuickJSRuntime::GetMemoryUsageStats(bool run_gc_first) {
    if (!rt_) {
        return json::object();
    }

    if (run_gc_first) {
        JS_RunGC(rt_);
    }

    JSMemoryUsage usage{};
    JS_ComputeMemoryUsage(rt_, &usage);

    return json{
        {"malloc_size", usage.malloc_size},
        {"memory_used_size", usage.memory_used_size},
        {"malloc_count", usage.malloc_count},
        {"memory_used_count", usage.memory_used_count},
        {"atom_count", usage.atom_count},
        {"atom_size", usage.atom_size},
        {"str_count", usage.str_count},
        {"str_size", usage.str_size},
        {"obj_count", usage.obj_count},
        {"obj_size", usage.obj_size},
        {"prop_count", usage.prop_count},
        {"prop_size", usage.prop_size},
        {"shape_count", usage.shape_count},
        {"shape_size", usage.shape_size},
        {"js_func_count", usage.js_func_count},
        {"js_func_size", usage.js_func_size},
        {"js_func_code_size", usage.js_func_code_size},
        {"c_func_count", usage.c_func_count},
        {"array_count", usage.array_count},
        {"fast_array_count", usage.fast_array_count},
        {"fast_array_elements", usage.fast_array_elements},
        {"binary_object_count", usage.binary_object_count},
        {"binary_object_size", usage.binary_object_size}
    };
}

bool QuickJSRuntime::HasPendingJobs() {
    return JS_IsJobPending(rt_);
}

void QuickJSRuntime::RunEventLoop(int max_iterations) {
    int iterations = 0;

    while (max_iterations < 0 || iterations < max_iterations) {
        // 1. Process all microtasks first
        ProcessMicrotasks();

        // 2. Check if there are any tasks to process
        bool has_tasks = !task_queue_.empty() || !timer_queue_.empty() || HasPendingJobs();

        if (!has_tasks) {
            break;  // No more tasks, exit loop
        }

        // 3. Process immediate tasks
        ProcessTasks();

        // 4. Process expired timers
        int64_t now = GetCurrentTimeMs();

        while (!timer_queue_.empty()) {
            // Get the first timer (earliest execute_time)
            auto timer_it = timer_queue_.begin();

            if (timer_it->first > now) {
                break;  // No more expired timers
            }

            // Extract task and remove from queue
            Task task = timer_it->second;
            int timer_id = task.id;
            timer_queue_.erase(timer_it);

            // Remove from active_timers (we'll re-add if it's an interval)
            active_timers_.erase(timer_id);

            // Prepare arguments array for JS_Call
            std::vector<JSValue> js_args;
            for (const auto& arg_wrapper : task.args) {
                js_args.push_back(arg_wrapper->Get());
            }

            // Execute the timer callback
            JSValue result = JS_Call(ctx_, task.callback->Get(), JS_UNDEFINED,
                                    js_args.size(), js_args.data());

            if (JS_IsException(result)) {
                std::string error = GetJSError();
            }

            JS_FreeValue(ctx_, result);

            // If it's an interval, reschedule it
            if (task.repeat) {
                task.execute_time = now + task.interval;
                auto new_it = timer_queue_.insert(std::make_pair(task.execute_time, task));
                active_timers_[timer_id] = new_it;
            }
            // else: One-time timer, already cleaned up (removed from both maps)
        }

        // 5. Process microtasks again
        ProcessMicrotasks();

        iterations++;

        // 6. If no immediate tasks but have timers, sleep briefly
        if (task_queue_.empty() && !timer_queue_.empty()) {
            // Get the earliest timer (first element in multimap)
            int64_t next_time = timer_queue_.begin()->first;
            int64_t sleep_ms = std::max(0LL, next_time - GetCurrentTimeMs());
            if (sleep_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    std::min(sleep_ms, 10LL)
                ));
            }
        }
    }
}

JSValue QuickJSRuntime::NativeFunctionWrapper(JSContext* ctx, JSValueConst this_val,
                                              int argc, JSValueConst* argv, int magic,
                                              JSValue* func_data) {
    // Get the runtime instance from context opaque
    QuickJSRuntime* runtime = static_cast<QuickJSRuntime*>(JS_GetContextOpaque(ctx));
    if (!runtime) {
        return JS_ThrowInternalError(ctx, "Runtime not found");
    }

    // Get function name from func_data
    const char* func_name_str = JS_ToCString(ctx, func_data[0]);
    if (!func_name_str) {
        return JS_ThrowInternalError(ctx, "Function name not found");
    }
    std::string func_name(func_name_str);
    JS_FreeCString(ctx, func_name_str);

    // Find the native function
    auto it = runtime->native_functions_.find(func_name);
    if (it == runtime->native_functions_.end()) {
        return JS_ThrowInternalError(ctx, "Native function not found: %s", func_name.c_str());
    }

    // Convert arguments to JSON array
    json args = json::array();
    for (int i = 0; i < argc; i++) {
        args.push_back(runtime->JSValueToJSON(argv[i]));
    }

    try {
        // Call the native function
        json result = it->second(args);

        // Convert result back to JSValue
        return runtime->JSONToJSValue(result);
    } catch (const std::exception& e) {
        return JS_ThrowInternalError(ctx, "Native function error: %s", e.what());
    }
}

} // namespace mbink

