#include "devtools_bridge.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace mbink {
namespace {

constexpr unsigned int kDevToolsAttachVersion = kDevToolsBridgeVersion;
constexpr int kMbinkOk = 0;
constexpr int kMbinkErrorInvalidHandle = -1;
constexpr int kMbinkErrorInvalidParam = -2;
constexpr int kMbinkErrorUnknown = -99;

DevToolsHostServices g_host_services{};

char* CopyHostString(const std::string& value) {
    if (!g_host_services.copy_string) {
        return nullptr;
    }
    return g_host_services.copy_string(value.c_str());
}

void SetHostString(char** out, const std::string& value) {
    if (out) {
        *out = CopyHostString(value);
    }
}

std::string ToLowerAscii(std::string value) {
    for (char& ch : value) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return value;
}

std::string TrimAscii(std::string value) {
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n' ||
                              value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }
    size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t')) {
        ++start;
    }
    return value.substr(start);
}

std::unordered_map<std::string, std::string> ParseHttpHeaders(const std::string& header_text) {
    std::unordered_map<std::string, std::string> headers;
    std::istringstream stream(header_text);
    std::string line;
    std::getline(stream, line);
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        headers[ToLowerAscii(TrimAscii(line.substr(0, colon)))] = TrimAscii(line.substr(colon + 1));
    }
    return headers;
}

bool IsAllowedLocalOrigin(const std::string& origin) {
    if (origin.empty()) {
        return true;
    }
    const auto lower = ToLowerAscii(origin);
    constexpr const char* loopback_origin = "http://127.0.0.1";
    return lower == loopback_origin ||
           (lower.rfind(loopback_origin, 0) == 0 &&
            lower.size() > std::char_traits<char>::length(loopback_origin) &&
            lower[std::char_traits<char>::length(loopback_origin)] == ':');
}

std::string HttpStatusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "Error";
    }
}

std::string HttpResponse(int status,
                         const std::string& content_type,
                         const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << ' ' << HttpStatusText(status) << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Content-Type: " << content_type << "\r\n"
             << "Connection: close\r\n"
             << "Access-Control-Allow-Origin: http://127.0.0.1\r\n"
             << "Access-Control-Allow-Headers: content-type, authorization, x-mbink-devtools-token\r\n"
             << "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
             << "\r\n"
             << body;
    return response.str();
}

std::string JsonErrorResponse(int status, const std::string& code, const std::string& message) {
    return HttpResponse(status,
                        "application/json",
                        nlohmann::json{{"ok", false},
                                       {"error", {{"code", code}, {"message", message}}}}.dump());
}

bool HasValidAuth(const std::unordered_map<std::string, std::string>& headers,
                  const std::string& token,
                  bool require_auth) {
    if (!require_auth) {
        return true;
    }
    if (token.empty()) {
        return false;
    }
    auto it = headers.find("x-mbink-devtools-token");
    if (it != headers.end() && it->second == token) {
        return true;
    }
    it = headers.find("authorization");
    if (it == headers.end()) {
        return false;
    }
    constexpr const char* bearer_prefix = "Bearer ";
    constexpr size_t bearer_prefix_len = std::char_traits<char>::length(bearer_prefix);
    return it->second.size() == bearer_prefix_len + token.size() &&
           it->second.rfind(bearer_prefix, 0) == 0 &&
           it->second.substr(bearer_prefix_len) == token;
}

std::string RandomToken() {
    unsigned char bytes[16] = {};
#ifdef _WIN32
    if (BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        return {};
    }
#else
    return {};
#endif
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char byte : bytes) {
        oss << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return oss.str();
}

#ifdef _WIN32
class HttpMcpServer {
public:
    int Start(const DevToolsHostContext* context,
              const DevToolsHttpServerOptionsBridge* options,
              DevToolsHttpServerInfoBridge* info,
              char** out_error) {
        if (!context || !context->host_user_data ||
            !g_host_services.handle_http_mcp_json || !info) {
            SetHostString(out_error, "invalid HTTP devtools host");
            return kMbinkErrorInvalidParam;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            if (host_user_data_ != context->host_user_data) {
                SetHostString(out_error, "mbink devtools HTTP MCP is already running for another host");
                return kMbinkErrorInvalidHandle;
            }
            FillInfoLocked(info);
            return kMbinkOk;
        }

        bind_host_ = options && options->bind_host && options->bind_host[0]
                         ? options->bind_host
                         : "127.0.0.1";
        if (bind_host_ != "127.0.0.1") {
            SetHostString(out_error, "mbink devtools HTTP currently binds only to 127.0.0.1");
            return kMbinkErrorInvalidParam;
        }
        require_auth_ = !options || options->require_auth;
        token_ = options && options->auth_token && options->auth_token[0]
                     ? options->auth_token
                     : (require_auth_ ? RandomToken() : std::string{});
        if (require_auth_ && token_.empty()) {
            SetHostString(out_error, "failed to generate devtools auth token");
            return kMbinkErrorUnknown;
        }
        host_user_data_ = context->host_user_data;

        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            SetHostString(out_error, "WSAStartup failed");
            return kMbinkErrorUnknown;
        }
        winsock_started_ = true;

        listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket_ == INVALID_SOCKET) {
            AbortStartLocked();
            SetHostString(out_error, "socket creation failed");
            return kMbinkErrorUnknown;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(options ? options->port : 0);
        if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            AbortStartLocked();
            SetHostString(out_error, "bind failed");
            return kMbinkErrorUnknown;
        }
        if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
            AbortStartLocked();
            SetHostString(out_error, "listen failed");
            return kMbinkErrorUnknown;
        }
        sockaddr_in bound{};
        int bound_len = sizeof(bound);
        if (getsockname(listen_socket_, reinterpret_cast<sockaddr*>(&bound), &bound_len) == SOCKET_ERROR) {
            AbortStartLocked();
            SetHostString(out_error, "getsockname failed");
            return kMbinkErrorUnknown;
        }

        port_ = ntohs(bound.sin_port);
        url_ = "http://127.0.0.1:" + std::to_string(port_) + "/mcp";
        stopping_ = false;
        running_ = true;
        if (g_host_services.set_main_thread_sync_cancelled) {
            g_host_services.set_main_thread_sync_cancelled(host_user_data_, false);
        }
        worker_ = std::thread([this]() { ServeLoop(); });
        FillInfoLocked(info);
        return kMbinkOk;
    }

    int Stop() {
        StopServer();
        return kMbinkOk;
    }

private:
    void FillInfoLocked(DevToolsHttpServerInfoBridge* info) const {
        info->port = port_;
        info->url = CopyHostString(url_);
        info->auth_token = CopyHostString(token_);
    }

    void StopLocked() {
        stopping_ = true;
        if (listen_socket_ != INVALID_SOCKET) {
            closesocket(listen_socket_);
            listen_socket_ = INVALID_SOCKET;
        }
        for (SOCKET client : active_clients_) {
            shutdown(client, SD_BOTH);
            closesocket(client);
        }
        active_clients_.clear();
        running_ = false;
    }

    void CleanupStoppedLocked() {
        if (winsock_started_) {
            WSACleanup();
            winsock_started_ = false;
        }
        host_user_data_ = nullptr;
        port_ = 0;
        url_.clear();
        token_.clear();
    }

    void AbortStartLocked() {
        StopLocked();
        CleanupStoppedLocked();
    }

    void StopServer() {
        void* host_user_data = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            host_user_data = host_user_data_;
            StopLocked();
        }
        if (host_user_data && g_host_services.set_main_thread_sync_cancelled) {
            g_host_services.set_main_thread_sync_cancelled(host_user_data, true);
        }
        if (worker_.joinable()) {
            worker_.join();
        }
        if (host_user_data && g_host_services.set_main_thread_sync_cancelled) {
            g_host_services.set_main_thread_sync_cancelled(host_user_data, false);
        }
        std::lock_guard<std::mutex> lock(mutex_);
        CleanupStoppedLocked();
    }

    std::string ReadHttpRequest(SOCKET client) {
        std::string data;
        char buffer[4096];
        for (;;) {
            const int n = recv(client, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                break;
            }
            data.append(buffer, buffer + n);
            const auto header_end = data.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                const auto headers = ParseHttpHeaders(data.substr(0, header_end));
                size_t content_length = 0;
                if (auto it = headers.find("content-length"); it != headers.end()) {
                    try {
                        content_length = static_cast<size_t>(std::stoul(it->second));
                    } catch (...) {
                        content_length = 0;
                    }
                }
                if (data.size() >= header_end + 4 + content_length) {
                    break;
                }
            }
            if (data.size() > 16 * 1024 * 1024) {
                break;
            }
        }
        return data;
    }

    bool AddClient(SOCKET client) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            active_clients_.insert(client);
            return true;
        }
        return false;
    }

    bool TakeClient(SOCKET client) {
        std::lock_guard<std::mutex> lock(mutex_);
        return active_clients_.erase(client) > 0;
    }

    void CloseClient(SOCKET client) {
        shutdown(client, SD_BOTH);
        closesocket(client);
    }

    void ConfigureClientSocket(SOCKET client) {
        DWORD timeout_ms = 1000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
    }

    std::string HandleRequest(const std::string& raw) {
        void* host_user_data = nullptr;
        std::string token;
        bool require_auth = true;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_ || stopping_ || !host_user_data_) {
                return JsonErrorResponse(500, "host_unavailable", "devtools host is unavailable");
            }
            host_user_data = host_user_data_;
            token = token_;
            require_auth = require_auth_;
        }

        const auto header_end = raw.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            return JsonErrorResponse(400, "bad_request", "missing HTTP headers");
        }
        const std::string header_text = raw.substr(0, header_end);
        std::istringstream first_line_stream(header_text);
        std::string method;
        std::string path;
        first_line_stream >> method >> path;
        const auto headers = ParseHttpHeaders(header_text);
        if (method == "OPTIONS") {
            return HttpResponse(204, "text/plain", "");
        }
        if (method != "POST") {
            return JsonErrorResponse(405, "method_not_allowed", "use POST");
        }
        if (path != "/mcp" && path != "/") {
            return JsonErrorResponse(404, "not_found", "unknown devtools HTTP path");
        }
        const auto origin_it = headers.find("origin");
        if (origin_it != headers.end() && !IsAllowedLocalOrigin(origin_it->second)) {
            return JsonErrorResponse(403, "forbidden_origin", "origin is not allowed");
        }
        if (!HasValidAuth(headers, token, require_auth)) {
            return JsonErrorResponse(401, "unauthorized", "missing or invalid devtools token");
        }

        size_t content_length = 0;
        if (auto it = headers.find("content-length"); it != headers.end()) {
            try {
                content_length = static_cast<size_t>(std::stoul(it->second));
            } catch (...) {
                return JsonErrorResponse(400, "bad_request", "invalid content-length");
            }
        }
        const auto body_offset = header_end + 4;
        const auto body = raw.substr(body_offset, (std::min)(content_length, raw.size() - body_offset));
        if (body.empty()) {
            return JsonErrorResponse(400, "bad_request", "request body is empty");
        }

        char* response_json = nullptr;
        const int rc = g_host_services.handle_http_mcp_json(host_user_data, body.c_str(), &response_json);
        if (rc != kMbinkOk || !response_json) {
            const std::string message = response_json ? response_json : "devtools host is unavailable";
            if (response_json && g_host_services.free_string) {
                g_host_services.free_string(response_json);
            }
            return JsonErrorResponse(500, "host_unavailable", message);
        }
        const std::string response(response_json);
        if (g_host_services.free_string) {
            g_host_services.free_string(response_json);
        }
        return HttpResponse(200, "application/json", response);
    }

    void ServeLoop() {
        for (;;) {
            SOCKET current = INVALID_SOCKET;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!running_ || listen_socket_ == INVALID_SOCKET) {
                    break;
                }
                current = listen_socket_;
            }
            SOCKET client = accept(current, nullptr, nullptr);
            if (client == INVALID_SOCKET) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!running_) {
                    break;
                }
                continue;
            }
            if (!AddClient(client)) {
                CloseClient(client);
                break;
            }
            ConfigureClientSocket(client);
            const auto request = ReadHttpRequest(client);
            const auto response = HandleRequest(request);
            send(client, response.data(), static_cast<int>(response.size()), 0);
            if (TakeClient(client)) {
                CloseClient(client);
            }
        }
    }

    mutable std::mutex mutex_;
    bool running_ = false;
    bool stopping_ = false;
    bool winsock_started_ = false;
    SOCKET listen_socket_ = INVALID_SOCKET;
    std::set<SOCKET> active_clients_;
    std::thread worker_;
    void* host_user_data_ = nullptr;
    std::string bind_host_;
    unsigned short port_ = 0;
    std::string url_;
    std::string token_;
    bool require_auth_ = true;
};
#else
class HttpMcpServer {
public:
    int Start(const DevToolsHostContext*,
              const DevToolsHttpServerOptionsBridge*,
              DevToolsHttpServerInfoBridge*,
              char** out_error) {
        SetHostString(out_error, "devtools HTTP MCP is not implemented on this platform");
        return kMbinkErrorUnknown;
    }

    int Stop() {
        return kMbinkOk;
    }
};
#endif

HttpMcpServer g_http_mcp_server;

int BridgeHttpStart(const DevToolsHostContext* context,
                    const DevToolsHttpServerOptionsBridge* options,
                    DevToolsHttpServerInfoBridge* info,
                    char** out_error) {
    if (out_error) {
        *out_error = nullptr;
    }
    return g_http_mcp_server.Start(context, options, info, out_error);
}

int BridgeHttpStop(const DevToolsHostContext*) {
    return g_http_mcp_server.Stop();
}

const DevToolsBridgeApi kBridgeApi{
    kDevToolsAttachVersion,
    BridgeHttpStart,
    BridgeHttpStop,
};

}  // namespace
}  // namespace mbink

#ifdef _WIN32
#define MBINK_DEVTOOLS_EXPORT __declspec(dllexport)
#else
#define MBINK_DEVTOOLS_EXPORT __attribute__((visibility("default")))
#endif

namespace mbink {

extern "C" MBINK_DEVTOOLS_EXPORT int mbink_devtools_attach(
    unsigned int version,
    const DevToolsHostServices* host_services,
    DevToolsRegisterBridgeFn register_bridge,
    DevToolsUnregisterBridgeFn) {
    if (version != kDevToolsAttachVersion ||
        !host_services ||
        host_services->version != kDevToolsAttachVersion ||
        !host_services->copy_string ||
        !host_services->free_string ||
        !host_services->handle_http_mcp_json ||
        !register_bridge) {
        return kMbinkErrorInvalidParam;
    }

    g_host_services = *host_services;
    return register_bridge(version, &kBridgeApi) ? kMbinkOk : kMbinkErrorUnknown;
}

}  // namespace mbink
