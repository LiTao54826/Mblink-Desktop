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
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <chrono>

namespace lightui {

QuickJSRuntime::QuickJSRuntime() {
    InitRuntime();
    InitStdLib();
    InitConsole();
    InitModuleLoader();
    InitTimers();
}

QuickJSRuntime::~QuickJSRuntime() {
    if (ctx_) {
        JS_FreeContext(ctx_);
        ctx_ = nullptr;
    }
    if (rt_) {
        JS_FreeRuntime(rt_);
        rt_ = nullptr;
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
        if (args.is_array() && !args.empty()) {
            for (const auto& arg : args) {
                if (arg.is_string()) {
                    std::cout << arg.get<std::string>();
                } else {
                    std::cout << arg.dump();
                }
                std::cout << " ";
            }
            std::cout << std::endl;
        }
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
    // Determine log level prefix based on magic value
    const char* prefix = "";
    switch (magic) {
        case 0: prefix = ""; break;           // log
        case 1: prefix = "[ERROR] "; break;   // error
        case 2: prefix = "[WARN] "; break;    // warn
        case 3: prefix = "[INFO] "; break;    // info
        default: prefix = ""; break;
    }

    // Print prefix
    if (prefix[0] != '\0') {
        printf("%s", prefix);
    }

    // Print all arguments separated by space
    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            printf(" ");
        }

        // Convert JSValue to string
        const char* str = JS_ToCString(ctx, argv[i]);
        if (str) {
            printf("%s", str);
            JS_FreeCString(ctx, str);
        } else {
            printf("[Error converting to string]");
        }
    }

    printf("\n");
    fflush(stdout);  // Ensure output is flushed

    return JS_UNDEFINED;
}

void QuickJSRuntime::InitModuleLoader() {
    // Set module loader function
    JS_SetModuleLoaderFunc(rt_, nullptr, ModuleLoader, this);
}

JSModuleDef* QuickJSRuntime::ModuleLoader(JSContext* ctx, const char* module_name, void* opaque) {
    QuickJSRuntime* runtime = static_cast<QuickJSRuntime*>(opaque);
    if (!runtime) {
        JS_ThrowInternalError(ctx, "Runtime not found in module loader");
        return nullptr;
    }

    // Check if module is registered
    auto it = runtime->module_registry_.find(module_name);
    if (it == runtime->module_registry_.end()) {
        JS_ThrowReferenceError(ctx, "Module not found: %s", module_name);
        return nullptr;
    }

    const std::string& module_code = it->second;

    // Compile the module
    JSValue module_val = JS_Eval(ctx, module_code.c_str(), module_code.length(),
                                 module_name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(module_val)) {
        return nullptr;
    }

    // Get the module definition
    JSModuleDef* m = (JSModuleDef*)JS_VALUE_GET_PTR(module_val);
    JS_FreeValue(ctx, module_val);

    return m;
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

    // Evaluate the module
    JSValue result = JS_Eval(ctx_, module_code.c_str(), module_code.length(),
                            module_name.c_str(), JS_EVAL_TYPE_MODULE);

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
    // Read file content
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open module file: " + filepath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string module_code = buffer.str();

    // Register and load the module
    RegisterModule(filepath, module_code);
    return LoadModule(filepath);
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

    // Store in active_timers first (this is the authoritative copy)
    active_timers_[task.id] = task;

    // Add to timer queue (this is just for scheduling)
    timer_queue_.push(task);

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

    // Remove timer from active_timers
    // JSValueWrapper destructors will automatically free callback and arguments
    auto it = runtime->active_timers_.find(timer_id);
    if (it != runtime->active_timers_.end()) {
        runtime->active_timers_.erase(it);
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
                fprintf(stderr, "[Timer Error] %s\n", error.c_str());
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
            fprintf(stderr, "[Microtask Error] %s\n", error.c_str());
            break;
        }
    }
}

void QuickJSRuntime::RunGC() {
    JS_RunGC(rt_);
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
            Task task = timer_queue_.top();

            if (task.execute_time > now) {
                break;  // No more expired timers
            }

            timer_queue_.pop();

            // Check if timer was cancelled
            auto it = active_timers_.find(task.id);
            if (it == active_timers_.end()) {
                // Timer was cancelled, skip it
                continue;
            }

            // Get the actual task from active_timers
            Task& active_task = it->second;

            // Prepare arguments array for JS_Call
            std::vector<JSValue> js_args;
            for (const auto& arg_wrapper : active_task.args) {
                js_args.push_back(arg_wrapper->Get());
            }

            // Execute the timer callback
            JSValue result = JS_Call(ctx_, active_task.callback->Get(), JS_UNDEFINED,
                                    js_args.size(), js_args.data());

            if (JS_IsException(result)) {
                std::string error = GetJSError();
                fprintf(stderr, "[Timer Error] %s\n", error.c_str());
            }

            JS_FreeValue(ctx_, result);

            // If it's an interval, reschedule it
            if (active_task.repeat) {
                active_task.execute_time = now + active_task.interval;
                timer_queue_.push(active_task);
            } else {
                // One-time timer, clean up
                // JSValueWrapper destructors will automatically free callback and arguments
                active_timers_.erase(it);
            }
        }

        // 5. Process microtasks again
        ProcessMicrotasks();

        iterations++;

        // 6. If no immediate tasks but have timers, sleep briefly
        if (task_queue_.empty() && !timer_queue_.empty()) {
            int64_t next_time = timer_queue_.top().execute_time;
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

} // namespace lightui

