/**
 * @file fetch_bindings.h
 * @brief fetch API 的 JavaScript 绑定
 *
 * 提供类似浏览器 fetch() 的异步 HTTP 请求 API
 */

#pragma once

#include "http_client.h"
#include "core/event/loop/task_scheduler.h"
#include <quickjs.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <queue>
#include <mutex>
#include <vector>
#include <functional>

namespace mbink {

/**
 * @brief 待处理的 fetch 响应
 */
struct PendingFetchResponse {
    int request_id;
    HttpResponse response;
};

// 前向声明静态回调函数
static JSValue js_fetch_request(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
static JSValue js_fetch_check_response(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);

/**
 * @brief Fetch API 绑定类
 *
 * 将 fetch API 暴露给 JavaScript
 */
class FetchBindings {
    // 允许静态回调函数访问私有成员
    friend JSValue js_fetch_request(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
    friend JSValue js_fetch_check_response(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);

public:
    using AssetProvider = std::function<bool(const std::string&, std::vector<uint8_t>&)>;

    /**
     * @brief 构造函数
     * @param ctx QuickJS 上下文
     * @param task_scheduler 任务调度器
     */
    FetchBindings(JSContext* ctx, std::shared_ptr<TaskScheduler> task_scheduler);

    /**
     * @brief 析构函数
     */
    ~FetchBindings();

    /**
     * @brief 初始化绑定
     */
    void InitBindings();

    /**
     * @brief 处理待处理的响应
     * 在主线程中调用，用于处理异步请求完成后的回调
     */
    void ProcessPendingResponses();

    /**
     * @brief 检查是否有待处理的响应
     */
    bool HasPendingResponses() const;

    static void SetAssetProvider(AssetProvider provider);
    static AssetProvider GetAssetProvider();
    static void SetBasePath(const std::string& path);
    static const std::string& GetBasePath();

    // 静态实例指针（供静态回调使用）
    static FetchBindings* instance_;

private:
    /**
     * @brief 注册原生函数
     */
    void RegisterNativeFunctions();
    
    /**
     * @brief 注册 JavaScript polyfill
     */
    void RegisterJSPolyfill();
    
    /**
     * @brief 执行 fetch 请求
     * @param url 请求 URL
     * @param options 请求选项（JSON）
     * @return 请求 ID
     */
    int DoFetch(const std::string& url, const nlohmann::json& options);
    
    /**
     * @brief 将 HttpResponse 转换为 JSON
     */
    nlohmann::json ResponseToJson(const HttpResponse& response);

    /**
     * @brief 辅助函数：将 JSON 转换为 JSValue
     */
    JSValue JsonToJSValue(const nlohmann::json& j);

    /**
     * @brief 辅助函数：将 JSValue 转换为 JSON
     */
    nlohmann::json JSValueToJson(JSValue val);

    /**
     * @brief 执行 JS 代码
     */
    void EvalJS(const std::string& code, const std::string& filename);

private:
    JSContext* ctx_;
    std::shared_ptr<TaskScheduler> task_scheduler_;
    std::unique_ptr<HttpClient> http_client_;

    int next_request_id_;

    // 待处理的响应队列（线程安全）
    mutable std::mutex pending_mutex_;
    std::queue<PendingFetchResponse> pending_responses_;

    static AssetProvider asset_provider_;
    static std::string base_path_;
};

} // namespace mbink

