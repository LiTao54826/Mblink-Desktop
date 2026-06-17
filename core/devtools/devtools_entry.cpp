#include "devtools_bridge.h"

#include "core/devtools/devtools_manager.h"
#include "core/devtools/inspector/element_picker.h"
#include "core/dom/element.h"
#include "core/render/objects/render_object.h"
#include "tools/esm_loader/ui_dev_control.h"
#include "tools/esm_loader/ui_dev_snapshot.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <random>
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
constexpr size_t kDefaultSnapshotMaxNodes = 2000;
constexpr size_t kHttpSnapshotMaxNodes = 10000;
constexpr int kDefaultSnapshotMaxDepth = 64;
constexpr int kHttpSnapshotMaxDepth = 256;
DevToolsHostServices g_host_services{};

template <typename Fn>
int RunWithHostMainThread(const DevToolsHostContext* context, Fn&& fn) {
    if (!context || !g_host_services.is_main_thread ||
        g_host_services.is_main_thread(context) ||
        !g_host_services.run_on_main_thread_sync) {
        fn();
        return kMbinkOk;
    }

    struct TaskState {
        Fn* fn = nullptr;
    } state{&fn};
    return g_host_services.run_on_main_thread_sync(
        context,
        [](void* user_data) {
            auto* task = static_cast<TaskState*>(user_data);
            if (task && task->fn) {
                (*task->fn)();
            }
        },
        &state);
}

template <typename Result, typename Fn>
Result RunWithHostMainThreadResult(const DevToolsHostContext* context, Result fallback, Fn&& fn) {
    Result result = fallback;
    const int rc = RunWithHostMainThread(context, [&]() {
        result = fn();
    });
    return rc == kMbinkOk ? result : fallback;
}

char* CopyHostString(const std::string& value) {
    if (!g_host_services.copy_string) {
        return nullptr;
    }
    return g_host_services.copy_string(value.c_str());
}

void SetHostString(char** out, const std::string& value) {
    if (!out) {
        return;
    }
    *out = CopyHostString(value);
}

std::shared_ptr<Element> SharedElement(Element* element) {
    if (!element) {
        return nullptr;
    }
    try {
        return std::static_pointer_cast<Element>(element->shared_from_this());
    } catch (...) {
        return nullptr;
    }
}

std::shared_ptr<RenderObject> SharedRenderObject(RenderObject* render_object) {
    if (!render_object) {
        return nullptr;
    }
    try {
        return render_object->shared_from_this();
    } catch (...) {
        return nullptr;
    }
}

DevToolsDockPosition ToBridgeDockPosition(DockPosition position) {
    return position == DockPosition::Right ? DevToolsDockPosition::Right
                                           : DevToolsDockPosition::Bottom;
}

DevToolsBounds MakeBounds(float x, float y, float width, float height) {
    DevToolsBounds bounds;
    bounds.x = x;
    bounds.y = y;
    bounds.width = width;
    bounds.height = height;
    return bounds;
}

std::string NewCommandToken() {
    const auto tick = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return "devtools-" + std::to_string(tick);
}

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int BridgeSnapshotFile(const DevToolsHostContext* context,
                       const char* output_path,
                       const UiDevSnapshotOptionsBridge* options,
                       char** out_error);
int BridgeCommandJson(const DevToolsHostContext* context,
                      const char* command_json,
                      char** out_response_json);

std::string JsonTextResult(const nlohmann::json& value) {
    auto text_value = value;
    if (text_value.is_object()) {
        text_value.erase("screenshot_base64");
        if (text_value.contains("screenshot") && text_value["screenshot"].is_object()) {
            text_value["screenshot"].erase("base64");
        }
    }
    return text_value.dump(2);
}

nlohmann::json McpToolContent(const nlohmann::json& value) {
    return nlohmann::json{{"content", nlohmann::json::array({{{"type", "text"},
                                                              {"text", JsonTextResult(value)}}})},
                          {"structuredContent", value},
                          {"isError", !value.value("ok", false)}};
}

nlohmann::json BuildMcpToolsJson() {
    return nlohmann::json::array({
        {{"name", "snapshot_ui"}, {"description", "Get a UI snapshot"},
         {"inputSchema", {{"type", "object"},
                          {"properties", {{"max_nodes", {{"type", "integer"}, {"minimum", 1}}},
                                          {"max_depth", {{"type", "integer"}, {"minimum", 1}}},
                                          {"root_selector", {{"type", "string"}}},
                                          {"include_screenshot", {{"type", "boolean"}}},
                                          {"inline_screenshot", {{"type", "boolean"}}}}}}}},
        {{"name", "query_element"}, {"description", "Query elements by selector"},
         {"inputSchema", {{"type", "object"},
                          {"properties", {{"selector", {{"type", "string"}}},
                                          {"limit", {{"type", "integer"}, {"minimum", 0}}}}},
                          {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "inspect"}, {"description", "Inspect one element"},
         {"inputSchema", {{"type", "object"},
                          {"properties", {{"selector", {{"type", "string"}}}}},
                          {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "click"}, {"description", "Click an element"},
         {"inputSchema", {{"type", "object"},
                          {"properties", {{"selector", {{"type", "string"}}}}},
                          {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "input_text"}, {"description", "Input text into an element"},
         {"inputSchema", {{"type", "object"},
                          {"properties", {{"selector", {{"type", "string"}}}, {"text", {{"type", "string"}}}}},
                          {"required", nlohmann::json::array({"selector", "text"})}}}},
        {{"name", "scroll"}, {"description", "Scroll an element"},
         {"inputSchema", {{"type", "object"},
                          {"properties", {{"selector", {{"type", "string"}}},
                                          {"x", {{"type", "number"}}},
                                          {"y", {{"type", "number"}}}}},
                          {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "highlight"}, {"description", "Highlight an element"},
         {"inputSchema", {{"type", "object"},
                          {"properties", {{"selector", {{"type", "string"}}}, {"color", {{"type", "string"}}}}},
                          {"required", nlohmann::json::array({"selector"})}}}}
    });
}

nlohmann::json MakeJsonRpcResult(const nlohmann::json& id, const nlohmann::json& result) {
    return nlohmann::json{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
}

nlohmann::json MakeJsonRpcError(const nlohmann::json& id,
                                int code,
                                const std::string& message) {
    return nlohmann::json{{"jsonrpc", "2.0"},
                          {"id", id},
                          {"error", {{"code", code}, {"message", message}}}};
}

nlohmann::json DevToolsCommandToJson(const DevToolsHostContext* context,
                                     nlohmann::json command) {
    if (!command.contains("id")) {
        command["id"] = NewCommandToken();
    }
    char* response = nullptr;
    const auto payload = command.dump();
    const int rc = BridgeCommandJson(context, payload.c_str(), &response);
    if (rc != kMbinkOk || !response) {
        nlohmann::json error{{"ok", false},
                             {"error", {{"code", "ui_dev_command_failed"},
                                         {"message", response ? response : "ui-dev command failed"}}}};
        if (response && g_host_services.free_string) {
            g_host_services.free_string(response);
        }
        return error;
    }
    auto parsed = nlohmann::json::parse(response, nullptr, false);
    if (g_host_services.free_string) {
        g_host_services.free_string(response);
    }
    if (parsed.is_discarded()) {
        return nlohmann::json{{"ok", false},
                              {"error", {{"code", "invalid_devtools_response"},
                                          {"message", "devtools command returned invalid JSON"}}}};
    }
    return parsed;
}

nlohmann::json SnapshotToJson(const DevToolsHostContext* context,
                              const nlohmann::json& arguments) {
    const size_t requested_max_nodes =
        arguments.value("max_nodes", kDefaultSnapshotMaxNodes);
    const int requested_max_depth =
        arguments.value("max_depth", kDefaultSnapshotMaxDepth);
    if (requested_max_nodes == 0 || requested_max_nodes > kHttpSnapshotMaxNodes) {
        return nlohmann::json{{"ok", false},
                              {"error",
                               {{"code", "invalid_args"},
                                {"message", "max_nodes must be between 1 and 10000"}}}};
    }
    if (requested_max_depth <= 0 || requested_max_depth > kHttpSnapshotMaxDepth) {
        return nlohmann::json{{"ok", false},
                              {"error",
                               {{"code", "invalid_args"},
                                {"message", "max_depth must be between 1 and 256"}}}};
    }
    if (arguments.value("include_screenshot", false) &&
        arguments.value("inline_screenshot", false)) {
        return nlohmann::json{{"ok", false},
                              {"error",
                               {{"code", "invalid_args"},
                                {"message", "inline screenshots are not supported over HTTP MCP"}}}};
    }

    const auto token = NewCommandToken();
    const auto snapshot_path = std::filesystem::temp_directory_path() /
        ("mbink-devtools-http-snapshot-" + token + ".json");
    const auto screenshot_path = std::filesystem::temp_directory_path() /
        ("mbink-devtools-http-screenshot-" + token + ".png");

    UiDevSnapshotOptionsBridge options;
    options.runtime_epoch = context && context->runtime_epoch ? context->runtime_epoch : nullptr;
    options.max_nodes = requested_max_nodes;
    options.max_depth = requested_max_depth;
    const auto root_selector = arguments.value("root_selector", std::string{});
    options.root_selector = root_selector.empty() ? nullptr : root_selector.c_str();
    options.include_screenshot = arguments.value("include_screenshot", false);
    options.inline_screenshot = arguments.value("inline_screenshot", false);
    const auto screenshot_file = arguments.value("screenshot_file", std::string{});
    const auto actual_screenshot = screenshot_file.empty() ? screenshot_path.string() : screenshot_file;
    options.screenshot_file = options.include_screenshot ? actual_screenshot.c_str() : nullptr;
    options.shutdown_requested = context && context->shutdown_requested;

    char* error = nullptr;
    const int rc = BridgeSnapshotFile(context, snapshot_path.string().c_str(), &options, &error);
    if (rc != kMbinkOk) {
        const std::string message = error ? error : "snapshot export failed";
        if (error && g_host_services.free_string) {
            g_host_services.free_string(error);
        }
        return nlohmann::json{{"ok", false},
                              {"error", {{"code", "snapshot_failed"}, {"message", message}}}};
    }
    if (error && g_host_services.free_string) {
        g_host_services.free_string(error);
    }

    auto parsed = nlohmann::json::parse(ReadFile(snapshot_path), nullptr, false);
    std::error_code ec;
    std::filesystem::remove(snapshot_path, ec);
    if (parsed.is_discarded()) {
        return nlohmann::json{{"ok", false},
                              {"error", {{"code", "snapshot_parse_failed"},
                                          {"message", "snapshot JSON parse failed"}}}};
    }
    return parsed;
}

nlohmann::json CallDevToolsMcpTool(const DevToolsHostContext* context,
                                   const std::string& tool_name,
                                   const nlohmann::json& arguments) {
    if (tool_name == "snapshot_ui" || tool_name == "snapshot") {
        return McpToolContent(SnapshotToJson(context, arguments));
    }

    if (tool_name == "query_element" || tool_name == "inspect" || tool_name == "click" ||
        tool_name == "input_text" || tool_name == "scroll" || tool_name == "highlight") {
        const auto selector = arguments.value("selector", std::string{});
        if (selector.empty()) {
            return McpToolContent(nlohmann::json{{"ok", false},
                                                {"error", {{"code", "invalid_args"},
                                                            {"message", "selector is required"}}}});
        }
        nlohmann::json command = arguments;
        command["type"] = tool_name;
        return McpToolContent(DevToolsCommandToJson(context, std::move(command)));
    }

    return McpToolContent(nlohmann::json{{"ok", false},
                                        {"error", {{"code", "tool_not_found"},
                                                    {"message", "unknown devtools MCP tool: " + tool_name}}}});
}

nlohmann::json HandleMcpJsonRpc(const DevToolsHostContext* context,
                                const nlohmann::json& request) {
    const auto id = request.contains("id") ? request.at("id") : nlohmann::json(nullptr);
    const auto method = request.value("method", std::string{});
    const auto params = request.value("params", nlohmann::json::object());

    if (method == "initialize") {
        return MakeJsonRpcResult(id,
                                 nlohmann::json{{"protocolVersion", "2025-03-26"},
                                                {"capabilities", {{"tools", nlohmann::json::object()}}},
                                                {"serverInfo", {{"name", "mbink-devtools"},
                                                                {"version", "0.1.0"}}}});
    }
    if (method == "tools/list") {
        return MakeJsonRpcResult(id, nlohmann::json{{"tools", BuildMcpToolsJson()}});
    }
    if (method == "tools/call") {
        const auto tool_name = params.value("name", std::string{});
        if (tool_name.empty()) {
            return MakeJsonRpcError(id, -32602, "tool name is required");
        }
        const auto arguments = params.value("arguments", nlohmann::json::object());
        return MakeJsonRpcResult(id, CallDevToolsMcpTool(context, tool_name, arguments));
    }
    if (method == "notifications/initialized") {
        return nlohmann::json{{"jsonrpc", "2.0"}};
    }
    return MakeJsonRpcError(id, -32601, "method not found");
}

std::string RandomToken() {
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::uniform_int_distribution<unsigned long long> dist;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << dist(rng)
        << std::setw(16) << dist(rng);
    return oss.str();
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
    const std::string bearer_prefix = "Bearer ";
    return it->second.size() == bearer_prefix.size() + token.size() &&
           it->second.rfind(bearer_prefix, 0) == 0 &&
           it->second.substr(bearer_prefix.size()) == token;
}

#ifdef _WIN32
class HttpMcpServer {
public:
    int Start(const DevToolsHostContext* context,
              const DevToolsHttpServerOptionsBridge* options,
              DevToolsHttpServerInfoBridge* info,
              char** out_error) {
        if (!context || !context->host_user_data ||
            !g_host_services.with_current_context_sync || !info) {
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
        running_ = true;
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
        if (!info) {
            return;
        }
        info->port = port_;
        info->url = CopyHostString(url_);
        info->auth_token = CopyHostString(token_);
    }

    void StopLocked() {
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
        {
            std::lock_guard<std::mutex> lock(mutex_);
            StopLocked();
        }
        if (worker_.joinable()) {
            worker_.join();
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
        setsockopt(client,
                   SOL_SOCKET,
                   SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&timeout_ms),
                   sizeof(timeout_ms));
        setsockopt(client,
                   SOL_SOCKET,
                   SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&timeout_ms),
                   sizeof(timeout_ms));
    }

    std::string HandleRequest(const std::string& raw) {
        void* host_user_data = nullptr;
        std::string token;
        bool require_auth = true;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_ || !host_user_data_) {
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
        auto request = nlohmann::json::parse(body, nullptr, false);
        if (request.is_discarded()) {
            return JsonErrorResponse(400, "parse_error", "request body is not valid JSON");
        }

        struct RequestState {
            const nlohmann::json* request = nullptr;
            nlohmann::json response;
        } state{&request, {}};
        const int rc = g_host_services.with_current_context_sync(
            host_user_data,
            [](const DevToolsHostContext* current_context, void* raw_state) {
                auto* state = static_cast<RequestState*>(raw_state);
                if (!state || !state->request) {
                    return;
                }
                state->response = HandleMcpJsonRpc(current_context, *state->request);
            },
            &state);
        if (rc != kMbinkOk) {
            return JsonErrorResponse(500, "host_unavailable", "devtools host is unavailable");
        }
        return HttpResponse(200, "application/json", state.response.dump());
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

void BridgeInitialize(const DevToolsHostContext* context) {
    if (!context || !context->document || !context->window) {
        return;
    }
    RunWithHostMainThread(context, [&]() {
        DevToolsManager::GetInstance().Initialize(context->document, context->window);
    });
}

void BridgeShutdown(const DevToolsHostContext* context) {
    g_http_mcp_server.Stop();
    RunWithHostMainThread(context, [&]() {
        auto& devtools = DevToolsManager::GetInstance();
        devtools.Close();
        devtools.Shutdown();
    });
}

int BridgeOpen(const DevToolsHostContext* context) {
    if (!context || !context->document || !context->window) {
        return kMbinkErrorInvalidHandle;
    }
    return RunWithHostMainThread(context, [&]() {
        auto& devtools = DevToolsManager::GetInstance();
        devtools.Initialize(context->document, context->window);
        devtools.Open();
    });
}

int BridgeClose(const DevToolsHostContext* context) {
    return RunWithHostMainThread(context, [&]() {
        DevToolsManager::GetInstance().Close();
    });
}

bool BridgeIsOpen() {
    return DevToolsManager::GetInstance().IsOpen();
}

DevToolsDockPosition BridgeDockPosition() {
    return ToBridgeDockPosition(DevToolsManager::GetInstance().GetDockPosition());
}

DevToolsBounds BridgeMainAppBounds(float width, float height) {
    float x = 0.0f;
    float y = 0.0f;
    float out_width = width;
    float out_height = height;
    DevToolsManager::GetInstance().GetMainAppBounds(width, height, x, y, out_width, out_height);
    return MakeBounds(x, y, out_width, out_height);
}

DevToolsBounds BridgePanelBounds(float width, float height) {
    float x = 0.0f;
    float y = 0.0f;
    float out_width = 0.0f;
    float out_height = 0.0f;
    DevToolsManager::GetInstance().GetPanelBounds(width, height, x, y, out_width, out_height);
    return MakeBounds(x, y, out_width, out_height);
}

bool BridgeHandleKeyboardShortcut(int key, bool ctrl, bool shift, bool alt) {
    return DevToolsManager::GetInstance().HandleKeyboardShortcut(key, ctrl, shift, alt);
}

bool BridgeHandleMouseWheel(int x, int y, float delta_x, float delta_y) {
    return DevToolsManager::GetInstance().HandleMouseWheel(x, y, delta_x, delta_y);
}

bool BridgeIsDraggingPanelBorder() {
    return DevToolsManager::GetInstance().IsDraggingPanelBorder();
}

bool BridgeIsMouseOnPanelBorder(float x, float y, float width, float height) {
    return DevToolsManager::GetInstance().IsMouseOnPanelBorder(x, y, width, height);
}

bool BridgeHandlePanelBorderDrag(float x, float y, float width, float height, bool pressed) {
    return DevToolsManager::GetInstance().HandlePanelBorderDrag(x, y, width, height, pressed);
}

bool BridgeUpdatePanelBorderDrag(float x, float y, float width, float height) {
    return DevToolsManager::GetInstance().UpdatePanelBorderDrag(x, y, width, height);
}

bool BridgeIsDraggingSplitter() {
    return DevToolsManager::GetInstance().IsDraggingSplitter();
}

bool BridgeHandleMouseEvent(int x, int y, int button, bool pressed) {
    return DevToolsManager::GetInstance().HandleMouseEvent(x, y, button, pressed);
}

bool BridgeHandleMouseMove(int x, int y) {
    return DevToolsManager::GetInstance().HandleMouseMove(x, y);
}

bool BridgeIsMouseOnSplitter(int x, int y) {
    return DevToolsManager::GetInstance().IsMouseOnSplitter(x, y);
}

void BridgeClearBoxModelHover() {
    DevToolsManager::GetInstance().ClearBoxModelHover();
}

bool BridgeIsPickerActive() {
    return DevToolsManager::GetInstance().IsPickerActive();
}

void BridgeSetPickerHover(Element* element, RenderObject* render_object) {
    auto* picker = DevToolsManager::GetInstance().GetElementPicker();
    if (!picker) {
        return;
    }
    picker->SetHoverElement(SharedElement(element), SharedRenderObject(render_object));
}

void BridgeSelectElement(Element* element) {
    DevToolsManager::GetInstance().SelectElement(SharedElement(element));
}

void BridgeStopPicker() {
    DevToolsManager::GetInstance().StopElementPicker();
}

void BridgeRenderHighlight(SkCanvas* canvas) {
    DevToolsManager::GetInstance().RenderHighlight(canvas);
}

void BridgeRenderPanel(SkCanvas* canvas, float width, float height) {
    DevToolsManager::GetInstance().Render(canvas, width, height);
}

int BridgeSnapshotFile(const DevToolsHostContext* context,
                       const char* output_path,
                       const UiDevSnapshotOptionsBridge* options,
                       char** out_error) {
    if (out_error) {
        *out_error = nullptr;
    }
    if (!context || !context->window || !context->document || !output_path) {
        SetHostString(out_error, "invalid snapshot host");
        return kMbinkErrorInvalidHandle;
    }
    if (context->shutdown_requested || (options && options->shutdown_requested)) {
        SetHostString(out_error, "shutdown_in_progress");
        return kMbinkErrorInvalidHandle;
    }

    ui_dev::SnapshotExportOptions snapshot_options;
    snapshot_options.runtime_epoch = options && options->runtime_epoch
                                         ? options->runtime_epoch
                                         : (context->runtime_epoch ? context->runtime_epoch : "");
    const size_t requested_max_nodes =
        options && options->max_nodes > 0 ? options->max_nodes : kDefaultSnapshotMaxNodes;
    if (requested_max_nodes > kHttpSnapshotMaxNodes) {
        SetHostString(out_error, "max_nodes must be between 1 and 10000");
        return kMbinkErrorInvalidParam;
    }
    const int requested_max_depth =
        options && options->max_depth > 0 ? options->max_depth : kDefaultSnapshotMaxDepth;
    if (requested_max_depth > kHttpSnapshotMaxDepth) {
        SetHostString(out_error, "max_depth must be between 1 and 256");
        return kMbinkErrorInvalidParam;
    }
    snapshot_options.max_nodes = requested_max_nodes;
    snapshot_options.max_depth = requested_max_depth;
    snapshot_options.root_selector = options && options->root_selector ? options->root_selector : "";
    snapshot_options.include_screenshot = options && options->include_screenshot;
    snapshot_options.inline_screenshot = options && options->inline_screenshot;
    snapshot_options.screenshot_path = options && options->screenshot_file ? options->screenshot_file : "";

    bool ok = false;
    std::string error;
    const int rc = RunWithHostMainThread(context, [&]() {
        ok = ui_dev::ExportUiDevSnapshot(context->window,
                                         context->document,
                                         output_path,
                                         snapshot_options,
                                         &error);
    });
    if (rc != kMbinkOk || !ok) {
        SetHostString(out_error, error.empty() ? "snapshot export failed" : error);
        return rc == kMbinkOk ? kMbinkErrorUnknown : rc;
    }
    return kMbinkOk;
}

int BridgeCommandJson(const DevToolsHostContext* context,
                      const char* command_json,
                      char** out_response_json) {
    if (out_response_json) {
        *out_response_json = nullptr;
    }
    if (!context || !context->runtime || !context->window || !context->document ||
        !command_json || !out_response_json) {
        return kMbinkErrorInvalidParam;
    }
    if (context->shutdown_requested) {
        SetHostString(out_response_json, R"({"ok":false,"error":{"code":"shutdown_in_progress","message":"runtime is shutting down"}})");
        return kMbinkErrorInvalidHandle;
    }

    const auto token = NewCommandToken();
    const auto base = std::filesystem::temp_directory_path();
    const auto command_path = base / ("mbink-devtools-command-" + token + ".json");
    const auto response_path = base / ("mbink-devtools-response-" + token + ".json");

    try {
        {
            std::ofstream file(command_path, std::ios::binary | std::ios::trunc);
            if (!file) {
                return kMbinkErrorUnknown;
            }
            file << command_json;
        }

        bool ok = false;
        bool handled = false;
        std::string error;
        std::string last_command_id;
        std::string runtime_epoch = context->runtime_epoch ? context->runtime_epoch : "";
        const int marshal_rc = RunWithHostMainThread(context, [&]() {
            ok = ui_dev::TryHandleUiDevCommand(context->runtime,
                                               context->window,
                                               context->document,
                                               command_path.string(),
                                               response_path.string(),
                                               &last_command_id,
                                               &runtime_epoch,
                                               nullptr,
                                               &handled,
                                               &error);
        });
        if (marshal_rc != kMbinkOk || !ok || !handled) {
            SetHostString(out_response_json, error.empty() ? "ui-dev command not handled" : error);
            return marshal_rc == kMbinkOk ? kMbinkErrorUnknown : marshal_rc;
        }

        const auto response = ReadFile(response_path);
        {
            std::error_code ec;
            std::filesystem::remove(command_path, ec);
            std::filesystem::remove(response_path, ec);
        }
        if (response.empty()) {
            SetHostString(out_response_json, "empty ui-dev response");
            return kMbinkErrorUnknown;
        }
        SetHostString(out_response_json, response);
        return *out_response_json ? kMbinkOk : kMbinkErrorUnknown;
    } catch (const std::exception& e) {
        SetHostString(out_response_json, e.what());
        return kMbinkErrorUnknown;
    } catch (...) {
        SetHostString(out_response_json, "unknown ui-dev command error");
        return kMbinkErrorUnknown;
    }
}

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
    BridgeInitialize,
    BridgeShutdown,
    BridgeOpen,
    BridgeClose,
    BridgeIsOpen,
    BridgeDockPosition,
    BridgeMainAppBounds,
    BridgePanelBounds,
    BridgeHandleKeyboardShortcut,
    BridgeHandleMouseWheel,
    BridgeIsDraggingPanelBorder,
    BridgeIsMouseOnPanelBorder,
    BridgeHandlePanelBorderDrag,
    BridgeUpdatePanelBorderDrag,
    BridgeIsDraggingSplitter,
    BridgeHandleMouseEvent,
    BridgeHandleMouseMove,
    BridgeIsMouseOnSplitter,
    BridgeClearBoxModelHover,
    BridgeIsPickerActive,
    BridgeSetPickerHover,
    BridgeSelectElement,
    BridgeStopPicker,
    BridgeRenderHighlight,
    BridgeRenderPanel,
    BridgeSnapshotFile,
    BridgeCommandJson,
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
        !register_bridge) {
        return kMbinkErrorInvalidParam;
    }

    g_host_services = *host_services;
    return register_bridge(version, &kBridgeApi) ? kMbinkOk : kMbinkErrorUnknown;
}

}  // namespace mbink
