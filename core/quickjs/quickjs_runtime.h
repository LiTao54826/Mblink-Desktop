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
#include <queue>
#include <map>
#include <vector>
#include <chrono>
#include "quickjs.h"
#include "nlohmann/json.hpp"
#include "quickjs/js_value_wrapper.h"

namespace mbink {

using json = nlohmann::json;

/**
 * @brief 异步任务结构
 *
 * 使用shared_ptr<JSValueWrapper>来管理JavaScript值的生命周期，
 * 确保在Task被复制或移动时正确管理引用计数。
 */
struct Task {
    int id;                                              // 任务ID
    std::shared_ptr<JSValueWrapper> callback;            // JavaScript回调函数
    std::vector<std::shared_ptr<JSValueWrapper>> args;   // 回调参数
    int64_t execute_time;                                // 执行时间（毫秒时间戳）
    bool repeat;                                         // 是否重复执行
    int64_t interval;                                    // 重复间隔（毫秒）
    bool cancelled;                                      // 是否已取消

    // 用于优先队列排序（执行时间早的优先）
    bool operator>(const Task& other) const {
        return execute_time > other.execute_time;
    }
};

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
     * @brief 执行ES6模块代码
     * @param code JavaScript模块代码
     * @param filename 文件名（用于错误报告）
     * @return 执行结果（JSON格式）
     * @throws std::runtime_error 如果执行失败
     */
    json EvalModule(const std::string& code, const std::string& filename = "<eval>");

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
     * @brief 注册模块
     * @param module_name 模块名
     * @param module_code 模块代码
     */
    void RegisterModule(const std::string& module_name, const std::string& module_code);

    /**
     * @brief 加载并执行模块
     * @param module_name 模块名
     * @return 模块导出对象（JSON格式）
     * @throws std::runtime_error 如果加载失败
     */
    json LoadModule(const std::string& module_name);

    /**
     * @brief 从文件加载模块
     * @param filepath 文件路径
     * @return 模块导出对象（JSON格式）
     * @throws std::runtime_error 如果加载失败
     */
    json LoadModuleFile(const std::string& filepath);

    /**
     * @brief 设置模块基础路径（用于解析相对导入）
     * @param path 基础路径
     */
    void SetBaseModulePath(const std::string& path);

    using FileLoader = std::function<bool(const std::string&, std::string&, std::string*)>;

    void SetFileLoader(FileLoader loader);

    /**
     * @brief 运行事件循环
     * @param max_iterations 最大迭代次数，-1表示无限循环直到没有任务
     */
    void RunEventLoop(int max_iterations = -1);

    /**
     * @brief 处理所有待执行的微任务
     */
    void ProcessMicrotasks();

    /**
     * @brief 运行垃圾回收
     */
    void RunGC();

    /**
     * @brief 获取 QuickJS 内存统计
     * @param run_gc_first 是否先执行一次 GC
     * @return 内存统计（JSON）
     */
    json GetMemoryUsageStats(bool run_gc_first = false);

    /**
     * @brief 打印 QuickJS 内存统计
     * @param tag 日志标签
     * @param run_gc_first 是否先执行一次 GC
     */
    json DumpMemoryUsageStats(const std::string& tag = "QuickJSMemoryStats", bool run_gc_first = false);

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

    // ----------------------------
    // UI Dev: 可选日志/异常捕获回调
    // ----------------------------
    using ConsoleCallback = std::function<void(const json& entry)>;
    using ErrorCallback = std::function<void(const json& entry)>;

    void SetConsoleCallback(ConsoleCallback cb) { console_callback_ = std::move(cb); }
    void SetErrorCallback(ErrorCallback cb) { error_callback_ = std::move(cb); }

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
     * @brief 初始化Console API
     */
    void InitConsole();

    /**
     * @brief 初始化模块加载器
     */
    void InitModuleLoader();

    /**
     * @brief 初始化定时器API
     */
    void InitTimers();

    /**
     * @brief 获取并格式化JavaScript错误
     * @return 错误信息
     */
    std::string GetJSError();

    void ReportJSError(const std::string& where, const std::string& message);

    /**
     * @brief 获取当前时间（毫秒）
     */
    int64_t GetCurrentTimeMs();

    /**
     * @brief 处理所有待执行的任务
     */
    void ProcessTasks();

    /**
     * @brief 检查是否有待处理的微任务
     */
    bool HasPendingJobs();

    /**
     * @brief 静态C函数包装器（用于QuickJS）
     */
    static JSValue NativeFunctionWrapper(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv, int magic,
                                        JSValue* func_data);

    /**
     * @brief Console API 实现
     * @param magic 0=log, 1=error, 2=warn, 3=info
     */
    static JSValue ConsoleLog(JSContext* ctx, JSValueConst this_val,
                             int argc, JSValueConst* argv, int magic);

    /**
     * @brief 模块加载器回调
     */
    static JSModuleDef* ModuleLoader(JSContext* ctx, const char* module_name, void* opaque);

    /**
     * @brief 模块名标准化回调（解析相对路径）
     */
    static char* ModuleNormalize(JSContext* ctx, const char* module_base, 
                                 const char* module_name, void* opaque);

    /**
     * @brief setTimeout 实现
     */
    static JSValue SetTimeout(JSContext* ctx, JSValueConst this_val,
                             int argc, JSValueConst* argv);

    /**
     * @brief setInterval 实现
     */
    static JSValue SetInterval(JSContext* ctx, JSValueConst this_val,
                              int argc, JSValueConst* argv);

    /**
     * @brief clearTimeout/clearInterval 实现
     */
    static JSValue ClearTimer(JSContext* ctx, JSValueConst this_val,
                             int argc, JSValueConst* argv);

    /**
     * @brief 内部实现：创建定时器
     */
    int CreateTimer(JSValue callback, int64_t delay, bool repeat,
                   const std::vector<JSValue>& args);

    /**
     * @brief 解析模块路径（相对/绝对路径）
     * @param module_name 模块名称
     * @param resolved_path 解析后的路径（输出参数）
     * @return 是否成功解析
     */
    bool ResolveModulePath(const char* module_name, std::string& resolved_path);

    /**
     * @brief 解析文件夹或文件（支持 package.json 和 index.js）
     */
    static std::string ResolveFolderOrFile(const std::string& path);
    std::string ResolveFolderOrFileWithLoader(const std::string& path) const;

    /**
     * @brief 解析 package 目录（支持 exports 和 main 字段）
     */
    static std::string ResolvePackageDirectory(const std::string& dir_path);
    std::string ResolvePackageDirectoryWithLoader(const std::string& dir_path) const;

    /**
     * @brief 解析 package.json 的 exports 字段
     */
    static std::string ResolvePackageExports(const json& exports, 
                                              const std::string& package_dir,
                                              const std::string& subpath);

    /**
     * @brief 从 node_modules 查找模块
     */
    static std::string ResolveNodeModules(const std::string& module_name,
                                           const std::string& start_path);

private:
    JSRuntime* rt_ = nullptr;
    JSContext* ctx_ = nullptr;
    std::unordered_map<std::string, NativeFunction> native_functions_;
    std::unordered_map<std::string, std::string> module_registry_;
    std::string base_module_path_;  // 当前模块的基础路径
    FileLoader file_loader_;

    // 异步任务队列
    std::queue<Task> task_queue_;
    // Timer队列：使用multimap按执行时间排序，支持高效的插入和删除
    // Key: execute_time, Value: Task
    std::multimap<int64_t, Task> timer_queue_;
    // 活跃的timer映射：timer_id -> iterator to timer_queue_
    // 用于快速查找和删除特定的timer
    std::unordered_map<int, std::multimap<int64_t, Task>::iterator> active_timers_;
    int next_task_id_ = 1;

    ConsoleCallback console_callback_;
    ErrorCallback error_callback_;
};

} // namespace mbink

