/**
 * @file http_client.cpp
 * @brief HTTP 客户端实现 - 使用 WinHTTP (Windows)
 */

#include "http_client.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace lightui {

HttpClient::HttpClient() {
}

HttpClient::~HttpClient() {
}

bool HttpClient::ParseUrl(const std::string& url, std::string& scheme,
                          std::string& host, int& port, std::string& path) {
    // 简单的 URL 解析
    std::regex url_regex(R"(^(https?):\/\/([^:\/\s]+)(?::(\d+))?(\/.*)?$)", std::regex::icase);
    std::smatch match;
    
    if (!std::regex_match(url, match, url_regex)) {
        return false;
    }
    
    scheme = match[1].str();
    host = match[2].str();
    
    if (match[3].matched) {
        port = std::stoi(match[3].str());
    } else {
        port = (scheme == "https") ? 443 : 80;
    }
    
    path = match[4].matched ? match[4].str() : "/";
    
    return true;
}

HttpResponse HttpClient::Get(const std::string& url, const HttpRequestOptions& options) {
    HttpRequestOptions opts = options;
    opts.method = "GET";
    return Request(url, opts);
}

HttpResponse HttpClient::Post(const std::string& url, const std::string& body,
                              const HttpRequestOptions& options) {
    HttpRequestOptions opts = options;
    opts.method = "POST";
    opts.body = body;
    return Request(url, opts);
}

HttpResponse HttpClient::Request(const std::string& url, const HttpRequestOptions& options) {
    return DoRequest(url, options);
}

void HttpClient::GetAsync(const std::string& url, ResponseCallback callback,
                          const HttpRequestOptions& options) {
    HttpRequestOptions opts = options;
    opts.method = "GET";
    RequestAsync(url, opts, callback);
}

void HttpClient::PostAsync(const std::string& url, const std::string& body,
                           ResponseCallback callback, const HttpRequestOptions& options) {
    HttpRequestOptions opts = options;
    opts.method = "POST";
    opts.body = body;
    RequestAsync(url, opts, callback);
}

void HttpClient::RequestAsync(const std::string& url, const HttpRequestOptions& options,
                              ResponseCallback callback) {
    // 在新线程中执行请求
    std::thread([this, url, options, callback]() {
        HttpResponse response = DoRequest(url, options);
        if (callback) {
            callback(response);
        }
    }).detach();
}

std::future<HttpResponse> HttpClient::RequestFuture(const std::string& url,
                                                    const HttpRequestOptions& options) {
    return std::async(std::launch::async, [this, url, options]() {
        return DoRequest(url, options);
    });
}

#ifdef _WIN32
// Windows WinHTTP 实现
HttpResponse HttpClient::DoRequest(const std::string& url, const HttpRequestOptions& options) {
    HttpResponse response;
    
    std::string scheme, host, path;
    int port;
    
    if (!ParseUrl(url, scheme, host, port, path)) {
        response.error = "Invalid URL: " + url;
        return response;
    }
    
    // 转换为宽字符
    std::wstring whost(host.begin(), host.end());
    std::wstring wpath(path.begin(), path.end());
    std::wstring wmethod(options.method.begin(), options.method.end());
    
    // 初始化 WinHTTP
    HINTERNET hSession = WinHttpOpen(L"MBink/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) {
        response.error = "WinHttpOpen failed: " + std::to_string(GetLastError());
        return response;
    }
    
    // 设置超时
    WinHttpSetTimeouts(hSession, options.timeout_ms, options.timeout_ms,
                       options.timeout_ms, options.timeout_ms);
    
    // 连接到服务器
    HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(),
                                        static_cast<INTERNET_PORT>(port), 0);
    
    if (!hConnect) {
        response.error = "WinHttpConnect failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hSession);
        return response;
    }
    
    // 创建请求
    DWORD flags = (scheme == "https") ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wmethod.c_str(),
                                            wpath.c_str(), NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            flags);
    
    if (!hRequest) {
        response.error = "WinHttpOpenRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }
    
    // 添加请求头
    for (const auto& header : options.headers) {
        std::wstring wheader = std::wstring(header.first.begin(), header.first.end()) +
                               L": " + std::wstring(header.second.begin(), header.second.end());
        WinHttpAddRequestHeaders(hRequest, wheader.c_str(), -1,
                                 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }
    
    // 发送请求
    BOOL result;
    if (!options.body.empty()) {
        result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    (LPVOID)options.body.c_str(),
                                    static_cast<DWORD>(options.body.length()),
                                    static_cast<DWORD>(options.body.length()), 0);
    } else {
        result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    }
    
    if (!result) {
        response.error = "WinHttpSendRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }
    
    // 接收响应
    result = WinHttpReceiveResponse(hRequest, NULL);
    
    if (!result) {
        response.error = "WinHttpReceiveResponse failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }
    
    // 获取状态码
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize,
                        WINHTTP_NO_HEADER_INDEX);
    
    response.status_code = static_cast<int>(statusCode);
    response.ok = (statusCode >= 200 && statusCode < 300);
    
    // 获取响应头
    DWORD headerSize = 0;
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX, NULL, &headerSize,
                        WINHTTP_NO_HEADER_INDEX);
    
    if (headerSize > 0) {
        std::vector<wchar_t> headerBuffer(headerSize / sizeof(wchar_t));
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                            WINHTTP_HEADER_NAME_BY_INDEX, headerBuffer.data(),
                            &headerSize, WINHTTP_NO_HEADER_INDEX);
        // 解析响应头（简化版）
    }
    
    // 读取响应体
    std::stringstream bodyStream;
    DWORD bytesAvailable = 0;
    
    do {
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            break;
        }
        
        if (bytesAvailable > 0) {
            std::vector<char> buffer(bytesAvailable + 1);
            DWORD bytesRead = 0;
            
            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                buffer[bytesRead] = '\0';
                bodyStream.write(buffer.data(), bytesRead);
            }
        }
    } while (bytesAvailable > 0);
    
    response.body = bodyStream.str();
    
    // 清理
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return response;
}

#else
// 非 Windows 平台的占位实现
HttpResponse HttpClient::DoRequest(const std::string& url, const HttpRequestOptions& options) {
    HttpResponse response;
    response.error = "HTTP client not implemented for this platform";
    return response;
}
#endif

} // namespace lightui

