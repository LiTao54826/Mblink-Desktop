/**
 * @file quickjs_runtime.cpp
 * @brief QuickJS运行时实现
 * 
 * 实现内容：
 * - QuickJS运行时和上下文管理
 * - JavaScript代码执行
 * - C++和JavaScript互操作
 * - 数据类型转换
 * 
 * TODO:
 * - [ ] 实现运行时初始化
 * - [ ] 实现代码执行（Eval）
 * - [ ] 实现函数注册
 * - [ ] 实现函数调用
 * - [ ] 实现JSON<->JSValue转换
 * - [ ] 添加模块系统支持
 * - [ ] 添加console API
 * - [ ] 添加setTimeout/setInterval
 */

#include "quickjs_runtime.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace lightui {

QuickJSRuntime::QuickJSRuntime() {
    // TODO: 实现构造函数
    // InitRuntime();
    // InitStdLib();
}

QuickJSRuntime::~QuickJSRuntime() {
    // TODO: 实现析构函数
    // if (ctx_) {
    //     JS_FreeContext(ctx_);
    // }
    // if (rt_) {
    //     JS_FreeRuntime(rt_);
    // }
}

json QuickJSRuntime::Eval(const std::string& code, const std::string& filename) {
    // TODO: 实现JavaScript代码执行
    // JSValue result = JS_Eval(ctx_, code.c_str(), code.length(),
    //                          filename.c_str(), JS_EVAL_TYPE_GLOBAL);
    // if (JS_IsException(result)) {
    //     std::string error = GetJSError();
    //     JS_FreeValue(ctx_, result);
    //     throw std::runtime_error("JavaScript error: " + error);
    // }
    // json ret = JSValueToJSON(result);
    // JS_FreeValue(ctx_, result);
    // return ret;
    return json();
}

json QuickJSRuntime::EvalFile(const std::string& filepath) {
    // TODO: 实现文件执行
    // std::ifstream file(filepath);
    // if (!file.is_open()) {
    //     throw std::runtime_error("Failed to open file: " + filepath);
    // }
    // std::stringstream buffer;
    // buffer << file.rdbuf();
    // return Eval(buffer.str(), filepath);
    return json();
}

void QuickJSRuntime::RegisterFunction(const std::string& name, NativeFunction func) {
    // TODO: 实现函数注册
    // native_functions_[name] = func;
    //
    // JSValue func_obj = JS_NewCFunctionData(
    //     ctx_, NativeFunctionWrapper, 0, 0, 1,
    //     &JS_NewString(ctx_, name.c_str())
    // );
    //
    // JSValue global = JS_GetGlobalObject(ctx_);
    // JS_SetPropertyStr(ctx_, global, name.c_str(), func_obj);
    // JS_FreeValue(ctx_, global);
}

json QuickJSRuntime::CallFunction(const std::string& func_name, const json& args) {
    // TODO: 实现JavaScript函数调用
    // JSValue global = JS_GetGlobalObject(ctx_);
    // JSValue func = JS_GetPropertyStr(ctx_, global, func_name.c_str());
    //
    // if (!JS_IsFunction(ctx_, func)) {
    //     JS_FreeValue(ctx_, func);
    //     JS_FreeValue(ctx_, global);
    //     throw std::runtime_error("Not a function: " + func_name);
    // }
    //
    // JSValue js_args = JSONToJSValue(args);
    // JSValue result = JS_Call(ctx_, func, global, 1, &js_args);
    //
    // JS_FreeValue(ctx_, js_args);
    // JS_FreeValue(ctx_, func);
    // JS_FreeValue(ctx_, global);
    //
    // if (JS_IsException(result)) {
    //     std::string error = GetJSError();
    //     JS_FreeValue(ctx_, result);
    //     throw std::runtime_error("Function call error: " + error);
    // }
    //
    // json ret = JSValueToJSON(result);
    // JS_FreeValue(ctx_, result);
    // return ret;
    return json();
}

json QuickJSRuntime::GetGlobalProperty(const std::string& name) {
    // TODO: 实现获取全局属性
    // JSValue global = JS_GetGlobalObject(ctx_);
    // JSValue value = JS_GetPropertyStr(ctx_, global, name.c_str());
    // JS_FreeValue(ctx_, global);
    //
    // json ret = JSValueToJSON(value);
    // JS_FreeValue(ctx_, value);
    // return ret;
    return json();
}

void QuickJSRuntime::SetGlobalProperty(const std::string& name, const json& value) {
    // TODO: 实现设置全局属性
    // JSValue global = JS_GetGlobalObject(ctx_);
    // JSValue js_value = JSONToJSValue(value);
    // JS_SetPropertyStr(ctx_, global, name.c_str(), js_value);
    // JS_FreeValue(ctx_, global);
}

json QuickJSRuntime::JSValueToJSON(JSValue value) {
    // TODO: 实现JSValue到JSON的转换
    // 处理各种类型：
    // - undefined/null
    // - boolean
    // - number
    // - string
    // - array
    // - object
    return json();
}

JSValue QuickJSRuntime::JSONToJSValue(const json& value) {
    // TODO: 实现JSON到JSValue的转换
    // 处理各种类型：
    // - null
    // - boolean
    // - number
    // - string
    // - array
    // - object
    return JS_UNDEFINED;
}

void QuickJSRuntime::InitRuntime() {
    // TODO: 初始化QuickJS运行时
    // rt_ = JS_NewRuntime();
    // if (!rt_) {
    //     throw std::runtime_error("Failed to create QuickJS runtime");
    // }
    //
    // ctx_ = JS_NewContext(rt_);
    // if (!ctx_) {
    //     JS_FreeRuntime(rt_);
    //     throw std::runtime_error("Failed to create QuickJS context");
    // }
}

void QuickJSRuntime::InitStdLib() {
    // TODO: 初始化标准库
    // js_std_add_helpers(ctx_, 0, nullptr);
    // js_init_module_std(ctx_, "std");
    // js_init_module_os(ctx_, "os");
}

std::string QuickJSRuntime::GetJSError() {
    // TODO: 获取JavaScript错误信息
    // JSValue exception = JS_GetException(ctx_);
    // const char* str = JS_ToCString(ctx_, exception);
    // std::string error(str);
    // JS_FreeCString(ctx_, str);
    // JS_FreeValue(ctx_, exception);
    // return error;
    return "Unknown error";
}

JSValue QuickJSRuntime::NativeFunctionWrapper(JSContext* ctx, JSValueConst this_val,
                                              int argc, JSValueConst* argv, int magic,
                                              JSValue* func_data) {
    // TODO: 实现C++函数包装器
    // 1. 从func_data获取函数名
    // 2. 从native_functions_查找对应的C++函数
    // 3. 转换参数（JSValue -> JSON）
    // 4. 调用C++函数
    // 5. 转换返回值（JSON -> JSValue）
    return JS_UNDEFINED;
}

} // namespace lightui

