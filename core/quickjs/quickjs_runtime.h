/**
 * @file quickjs_runtime.h
 * @brief QuickJS运行时封装
 * 
 * 功能：
 * - 封装QuickJS运行时和上下文
 * - 执行JavaScript代码
 * - 注册C++函数到JavaScript
 * - JavaScript和C++数据类型转换
 * - 模块系统支持
 * 
 * 依赖：
 * - QuickJS
 * - nlohmann/json（用于数据转换）
 * 
 * 实现要点：
 * - RAII管理QuickJS资源
 * - 异常安全的JavaScript执行
 * - 高效的数据类型转换
 * - 支持ES模块导入
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include "quickjs.h"
#include "nlohmann/json.hpp"

namespace lightui {

using json = nlohmann::json;

/**
 * @brief C++函数类型（可以从JavaScript调用）
 */
using NativeFunction = std::function<json(const json&)>;

/**
 * @brief QuickJS运行时类
 * 
 * 封装QuickJS运行时和上下文，提供JavaScript执行环境
 */
class QuickJSRuntime {
public:
    /**
     * @brief 构造函数
     */
    QuickJSRuntime();
    
    /**
     * @brief 析构函数
     */
    ~QuickJSRuntime();
    
    // 禁止拷贝
    QuickJSRuntime(const QuickJSRuntime&) = delete;
    QuickJSRuntime& operator=(const QuickJSRuntime&) = delete;
    
    /**
     * @brief 执行JavaScript代码
     * @param code JavaScript代码
     * @param filename 文件名（用于错误报告）
     * @return 执行结果（JSON格式）
     * @throws std::runtime_error 如果执行失败
     */
    json Eval(const std::string& code, const std::string& filename = "<eval>");
    
    /**
     * @brief 执行JavaScript文件
     * @param filepath 文件路径
     * @return 执行结果（JSON格式）
     * @throws std::runtime_error 如果执行失败
     */
    json EvalFile(const std::string& filepath);
    
    /**
     * @brief 注册C++函数到JavaScript全局对象
     * @param name 函数名
     * @param func C++函数
     */
    void RegisterFunction(const std::string& name, NativeFunction func);
    
    /**
     * @brief 调用JavaScript函数
     * @param func_name 函数名
     * @param args 参数（JSON格式）
     * @return 返回值（JSON格式）
     * @throws std::runtime_error 如果调用失败
     */
    json CallFunction(const std::string& func_name, const json& args);
    
    /**
     * @brief 获取全局对象的属性
     * @param name 属性名
     * @return 属性值（JSON格式）
     */
    json GetGlobalProperty(const std::string& name);
    
    /**
     * @brief 设置全局对象的属性
     * @param name 属性名
     * @param value 属性值（JSON格式）
     */
    void SetGlobalProperty(const std::string& name, const json& value);
    
    /**
     * @brief 获取QuickJS上下文
     * @return QuickJS上下文指针
     */
    JSContext* GetContext() const { return ctx_; }
    
    /**
     * @brief 获取QuickJS运行时
     * @return QuickJS运行时指针
     */
    JSRuntime* GetRuntime() const { return rt_; }

public:
    /**
     * @brief 将JSValue转换为JSON
     * @param value JSValue
     * @return JSON对象
     */
    json JSValueToJSON(JSValue value);
    
    /**
     * @brief 将JSON转换为JSValue
     * @param value JSON对象
     * @return JSValue
     */
    JSValue JSONToJSValue(const json& value);

private:
    /**
     * @brief 初始化运行时
     */
    void InitRuntime();
    
    /**
     * @brief 初始化标准库
     */
    void InitStdLib();
    
    /**
     * @brief 获取并格式化JavaScript错误
     * @return 错误信息
     */
    std::string GetJSError();
    
    /**
     * @brief 静态C函数包装器（用于QuickJS）
     */
    static JSValue NativeFunctionWrapper(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv, int magic,
                                        JSValue* func_data);

private:
    JSRuntime* rt_ = nullptr;
    JSContext* ctx_ = nullptr;
    std::unordered_map<std::string, NativeFunction> native_functions_;
};

} // namespace lightui

