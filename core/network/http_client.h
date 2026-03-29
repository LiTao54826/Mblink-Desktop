/**
 * @file http_client.h
 * @brief HTTP 客户端 - 提供异步 HTTP 请求功能
 */

#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <future>
#include <vector>

namespace mbink {

/**
 * @brief HTTP 响应结构
 */
struct HttpResponse {
    int status_code = 0;                              // HTTP 状态码
    std::string status_text;                          // 状态文本
    std::map<std::string, std::string> headers;       // 响应头
    std::string body;                                 // 响应体
    std::string error;                                // 错误信息（如果有）
    bool ok = false;                                  // 是否成功 (status 200-299)
    
    /**
     * @brief 检查响应是否成功
     */
    bool IsOk() const { return ok && status_code >= 200 && status_code < 300; }
};

/**
 * @brief HTTP 请求选项
 */
struct HttpRequestOptions {
    std::string method = "GET";                        // 请求方法
    std::map<std::string, std::string> headers;        // 请求头
    std::string body;                                  // 请求体
    int timeout_ms = 30000;                            // 超时时间（毫秒）
    bool follow_redirects = true;                      // 是否跟随重定向
};

/**
 * @brief HTTP 客户端类
 * 
 * 提供同步和异步的 HTTP 请求功能
 */
class HttpClient {
public:
    using ResponseCallback = std::function<void(const HttpResponse&)>;
    
    /**
     * @brief 构造函数
     */
    HttpClient();
    
    /**
     * @brief 析构函数
     */
    ~HttpClient();
    
    // 禁止拷贝
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    
    /**
     * @brief 同步 GET 请求
     * @param url 请求 URL
     * @param options 请求选项
     * @return HTTP 响应
     */
    HttpResponse Get(const std::string& url, const HttpRequestOptions& options = {});
    
    /**
     * @brief 同步 POST 请求
     * @param url 请求 URL
     * @param body 请求体
     * @param options 请求选项
     * @return HTTP 响应
     */
    HttpResponse Post(const std::string& url, const std::string& body, 
                      const HttpRequestOptions& options = {});
    
    /**
     * @brief 通用同步请求
     * @param url 请求 URL
     * @param options 请求选项
     * @return HTTP 响应
     */
    HttpResponse Request(const std::string& url, const HttpRequestOptions& options);
    
    /**
     * @brief 异步 GET 请求
     * @param url 请求 URL
     * @param callback 响应回调
     * @param options 请求选项
     */
    void GetAsync(const std::string& url, ResponseCallback callback,
                  const HttpRequestOptions& options = {});
    
    /**
     * @brief 异步 POST 请求
     * @param url 请求 URL
     * @param body 请求体
     * @param callback 响应回调
     * @param options 请求选项
     */
    void PostAsync(const std::string& url, const std::string& body,
                   ResponseCallback callback, const HttpRequestOptions& options = {});
    
    /**
     * @brief 通用异步请求
     * @param url 请求 URL
     * @param options 请求选项
     * @param callback 响应回调
     */
    void RequestAsync(const std::string& url, const HttpRequestOptions& options,
                      ResponseCallback callback);
    
    /**
     * @brief 获取异步请求的 future
     * @param url 请求 URL
     * @param options 请求选项
     * @return future 对象
     */
    std::future<HttpResponse> RequestFuture(const std::string& url, 
                                            const HttpRequestOptions& options);

private:
    /**
     * @brief 解析 URL
     * @param url 完整 URL
     * @param scheme 输出协议
     * @param host 输出主机
     * @param port 输出端口
     * @param path 输出路径
     * @return 是否解析成功
     */
    bool ParseUrl(const std::string& url, std::string& scheme, 
                  std::string& host, int& port, std::string& path);
    
    /**
     * @brief 执行 HTTP 请求（平台相关实现）
     */
    HttpResponse DoRequest(const std::string& url, const HttpRequestOptions& options);
};

} // namespace mbink

