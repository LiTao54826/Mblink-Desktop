#include "common.h"
#include "daemon_ipc.h"
#include "daemon_server.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using mbink::ui_dev::DaemonState;
using mbink::ui_dev::ErrorResponse;
using mbink::ui_dev::EnsureProjectIdentity;
using mbink::ui_dev::FindProjectIdentityUpwards;
using mbink::ui_dev::GetDaemonPipeName;
using mbink::ui_dev::IsProcessRunning;
using mbink::ui_dev::ListManagedProjects;
using mbink::ui_dev::LoadState;
using mbink::ui_dev::ProjectIdentity;
using mbink::ui_dev::RemoveState;
using mbink::ui_dev::RunDaemonServer;
using mbink::ui_dev::SendDaemonRequest;
using mbink::ui_dev::StartDetachedDaemon;
using mbink::ui_dev::WaitForDaemonReady;
using mbink::ui_dev::InitProject;
using mbink::ui_dev::InitProjectResult;
using mbink::ui_dev::ListSupportedInitTemplates;


namespace {

constexpr int kDaemonStartupTimeoutMs = 10000;
constexpr int kDaemonRequestTimeoutMs = 10000;
constexpr int kBuildRequestTimeoutMs = 45000;

void ConfigureConsoleForUtf8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void PrintJson(const nlohmann::json& j) {
    std::cout << j.dump(2) << std::endl;
}

void PrintJsonLine(const nlohmann::json& j) {
    std::cout << j.dump() << std::endl;
}

std::string GetOptionValue(const std::vector<std::string>& args, const std::string& key) {
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == key) return args[i + 1];
    }
    return "";
}

bool TryParseNumber(const std::string& value, double* out) {
    if (!out || value.empty()) return false;
    try {
        size_t parsed = 0;
        const double number = std::stod(value, &parsed);
        if (parsed != value.size()) return false;
        *out = number;
        return true;
    } catch (...) {
        return false;
    }
}

bool ReadFileBytes(const std::filesystem::path& path, std::string* content) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    *content = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    return ifs.good() || ifs.eof();
}

std::string ReadStdinBytes() {
    return std::string(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
}

std::string Base64Encode(const std::string& input) {
    static constexpr char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);
    for (size_t i = 0; i < input.size(); i += 3) {
        const unsigned char b0 = static_cast<unsigned char>(input[i]);
        const unsigned char b1 = i + 1 < input.size() ? static_cast<unsigned char>(input[i + 1]) : 0;
        const unsigned char b2 = i + 2 < input.size() ? static_cast<unsigned char>(input[i + 2]) : 0;
        output.push_back(kTable[(b0 >> 2) & 0x3F]);
        output.push_back(kTable[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)]);
        output.push_back(i + 1 < input.size() ? kTable[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=');
        output.push_back(i + 2 < input.size() ? kTable[b2 & 0x3F] : '=');
    }
    return output;
}

std::filesystem::path GetExecutablePath() {
#ifdef _WIN32
    char buf[MAX_PATH] = {0};
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return std::filesystem::path(std::string(buf, buf + n));
#else
    return std::filesystem::current_path();
#endif
}

std::optional<std::filesystem::path> NormalizeProjectRoot(const std::string& project_hint, std::string* error) {
    if (error) error->clear();
    if (project_hint.empty()) return std::nullopt;

    auto root = std::filesystem::absolute(project_hint).lexically_normal();
    if (std::filesystem::exists(root) && !std::filesystem::is_directory(root)) root = root.parent_path();
    if (root.empty() || !std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        if (error) *error = "项目目录不存在: " + project_hint;
        return std::nullopt;
    }
    return root;
}

std::optional<std::filesystem::path> FindProjectConfigRootUpwards(const std::filesystem::path& start_path) {
    std::error_code ec;
    auto current = std::filesystem::absolute(start_path.empty() ? std::filesystem::current_path() : start_path, ec).lexically_normal();
    if (ec) current = std::filesystem::current_path();
    if (std::filesystem::exists(current, ec) && !std::filesystem::is_directory(current, ec)) current = current.parent_path();
    while (!current.empty()) {
        if (std::filesystem::exists(current / "mbink.config.json", ec)) return current;
        const auto parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return std::nullopt;
}

std::optional<ProjectIdentity> ResolveProjectIdentity(const std::string& project_hint,
                                                      bool allow_auto_create,
                                                      std::string* error) {
    if (error) error->clear();

    const auto resolve_root = [&](const std::filesystem::path& root) -> std::optional<ProjectIdentity> {
        ProjectIdentity identity;
        if (!EnsureProjectIdentity(root, &identity, error)) return std::nullopt;
        return identity;
    };

    if (!project_hint.empty()) {
        const auto root = NormalizeProjectRoot(project_hint, error);
        if (!root.has_value()) return std::nullopt;
        return resolve_root(*root);
    }

    std::string lookup_error;
    if (const auto found = FindProjectIdentityUpwards(std::filesystem::current_path(), &lookup_error); found.has_value()) {
        return found;
    }
    if (!lookup_error.empty()) {
        if (error) *error = lookup_error;
        return std::nullopt;
    }

    if (allow_auto_create) {
        if (const auto config_root = FindProjectConfigRootUpwards(std::filesystem::current_path()); config_root.has_value()) {
            return resolve_root(*config_root);
        }
    }

    std::string list_error;
    auto managed_projects = ListManagedProjects(&list_error);
    if (!list_error.empty()) {
        if (error) *error = list_error;
        return std::nullopt;
    }
    if (managed_projects.size() == 1) return managed_projects.front();
    if (managed_projects.size() > 1) {
        if (error) *error = "检测到多个项目，请使用 --project <path> 指定";
        return std::nullopt;
    }

    if (error) *error = "未找到可用项目，请在项目目录下执行，或使用 --project <path> / open <path>";
    return std::nullopt;
}

bool HasRunningDaemon(const ProjectIdentity& identity, bool cleanup_stale_state) {
    const auto state = LoadState(identity.project_id, nullptr);
    if (state.has_value() && state->running && state->pid > 0 && IsProcessRunning(state->pid)) return true;
    if (cleanup_stale_state && state.has_value()) {
        std::string rm_err;
        RemoveState(identity.project_id, &rm_err);
    }
    return false;
}

bool EnsureDaemonRunning(const ProjectIdentity& identity, std::string* error) {
    if (HasRunningDaemon(identity, true)) return true;

    int pid = 0;
    if (!StartDetachedDaemon(GetExecutablePath(), identity.project_root.string(), identity.project_id, &pid, error)) {
        return false;
    }
    return WaitForDaemonReady(GetDaemonPipeName(identity.project_id), kDaemonStartupTimeoutMs, error);
}

nlohmann::json CallDaemon(const ProjectIdentity& identity, const nlohmann::json& req) {
    std::string error;
    nlohmann::json resp;
    if (!SendDaemonRequest(GetDaemonPipeName(identity.project_id), req, &resp, &error, kDaemonRequestTimeoutMs)) {
        return ErrorResponse("ipc_failed", error);
    }
    return resp;
}

nlohmann::json CallDaemonWithTimeout(const ProjectIdentity& identity,
                                    const nlohmann::json& req,
                                    int timeout_ms) {
    std::string error;
    nlohmann::json resp;
    if (!SendDaemonRequest(GetDaemonPipeName(identity.project_id), req, &resp, &error, timeout_ms)) {
        return ErrorResponse("ipc_failed", error);
    }
    return resp;
}

std::string GetProjectHint(const std::vector<std::string>& args, size_t positional_index) {
    const auto explicit_project = GetOptionValue(args, "--project");
    if (!explicit_project.empty()) return explicit_project;
    if (positional_index < args.size() && args[positional_index].rfind("--", 0) != 0) return args[positional_index];
    return "";
}

std::optional<ProjectIdentity> ResolveCommandProject(const std::vector<std::string>& args,
                                                    size_t positional_index,
                                                    bool allow_auto_create,
                                                    std::string* error) {
    return ResolveProjectIdentity(GetProjectHint(args, positional_index), allow_auto_create, error);
}

std::optional<ProjectIdentity> RequireRunningDaemonProject(const std::vector<std::string>& args,
                                                          size_t positional_index,
                                                          bool allow_auto_create,
                                                          std::string* error) {
    auto identity = ResolveCommandProject(args, positional_index, allow_auto_create, error);
    if (!identity.has_value()) return std::nullopt;
    if (!HasRunningDaemon(*identity, true)) {
        if (error) *error = "请先执行 open 或 daemon start";
        return std::nullopt;
    }
    return identity;
}

std::optional<ProjectIdentity> ResolveMcpProject(const std::optional<ProjectIdentity>& active_project,
                                                 std::string* error) {
    if (active_project.has_value()) return active_project;
    return ResolveProjectIdentity("", true, error);
}

nlohmann::json MakeJsonRpcResult(const nlohmann::json& id, const nlohmann::json& result) {
    return nlohmann::json{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
}

nlohmann::json MakeJsonRpcError(const nlohmann::json& id, int code, const std::string& message) {
    return nlohmann::json{{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", code}, {"message", message}}}};
}

std::string DecodeUriComponent(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());
    auto hex = [](char ch) -> int {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const int hi = hex(value[i + 1]);
            const int lo = hex(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                decoded.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        decoded.push_back(value[i] == '+' ? ' ' : value[i]);
    }
    return decoded;
}

nlohmann::json BuildMcpTools() {
    return nlohmann::json::array({
        {{"name", "open_project"}, {"description", "打开项目并启动运行时"}, {"inputSchema", {{"type", "object"}, {"properties", {{"path", {{"type", "string"}}}, {"project_root", {{"type", "string"}}}}}}}},
        {{"name", "init_project"}, {"description", "初始化新项目模板"}, {"inputSchema", {{"type", "object"}, {"properties", {{"path", {{"type", "string"}}}, {"template", {{"type", "string"}, {"enum", nlohmann::json(ListSupportedInitTemplates())}}}}}, {"required", nlohmann::json::array({"path", "template"})}}}},
        {{"name", "get_project_info"}, {"description", "获取当前项目信息"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "read_file"}, {"description", "读取项目文件"}, {"inputSchema", {{"type", "object"}, {"properties", {{"path", {{"type", "string"}}}, {"encoding", {{"type", "string"}, {"enum", nlohmann::json::array({"utf8", "base64"})}}}}}, {"required", nlohmann::json::array({"path"})}}}},
        {{"name", "write_file"}, {"description", "写入项目文件"}, {"inputSchema", {{"type", "object"}, {"properties", {{"path", {{"type", "string"}}}, {"content", {{"type", "string"}}}, {"encoding", {{"type", "string"}, {"enum", nlohmann::json::array({"utf8", "base64"})}}}}}, {"required", nlohmann::json::array({"path", "content"})}}}},
        {{"name", "build"}, {"description", "执行构建，可选 watch"}, {"inputSchema", {{"type", "object"}, {"properties", {{"watch", {{"type", "boolean"}}}}}}}},
        {{"name", "get_build_status"}, {"description", "获取最近一次构建状态"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "reload"}, {"description", "重启当前 runtime"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "eval_js"}, {"description", "在运行时执行 JS 代码"}, {"inputSchema", {{"type", "object"}, {"properties", {{"code", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"code"})}}}},
        {{"name", "query_element"}, {"description", "按 selector 查询元素列表"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "inspect"}, {"description", "检查单个元素详情"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "click"}, {"description", "点击目标元素"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "input_text"}, {"description", "向目标元素输入文本"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}, {"text", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"selector", "text"})}}}},
        {{"name", "scroll"}, {"description", "滚动目标元素"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}, {"x", {{"type", "number"}}}, {"y", {{"type", "number"}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "highlight"}, {"description", "高亮目标元素"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}, {"color", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "snapshot_ui"}, {"description", "获取运行时快照"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "get_console_logs"}, {"description", "获取运行时日志"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "get_js_errors"}, {"description", "获取运行时错误"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "stop_daemon"}, {"description", "停止 daemon"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}}
    });
}

nlohmann::json BuildMcpResources() {
    return nlohmann::json::array({
        {{"uri", "ui://snapshot"}, {"name", "ui snapshot"}, {"mimeType", "application/json"}},
        {{"uri", "ui://console_logs"}, {"name", "ui console logs"}, {"mimeType", "application/json"}},
        {{"uri", "ui://build_status"}, {"name", "build status"}, {"mimeType", "application/json"}},
        {{"uri", "project://file_tree"}, {"name", "project file tree"}, {"mimeType", "application/json"}},
        {{"uri", "project://config"}, {"name", "project config"}, {"mimeType", "application/json"}}
    });
}

nlohmann::json ReadMcpResource(const std::optional<ProjectIdentity>& active_project, const std::string& uri) {
    std::string err;
    const auto project = ResolveMcpProject(active_project, &err);
    if (!project.has_value()) return ErrorResponse("project_not_resolved", err);

    nlohmann::json payload;
    std::string mime_type = "application/json";
    if (uri == "ui://snapshot") {
        payload = CallDaemon(*project, nlohmann::json{{"cmd", "snapshot"}});
    } else if (uri == "ui://console_logs") {
        payload = CallDaemon(*project, nlohmann::json{{"cmd", "logs"}});
    } else if (uri == "ui://build_status") {
        payload = CallDaemon(*project, nlohmann::json{{"cmd", "build_status"}});
    } else if (uri == "project://file_tree") {
        payload = CallDaemon(*project, nlohmann::json{{"cmd", "info"}});
        if (payload.is_object() && payload.contains("file_tree")) payload = payload["file_tree"];
    } else if (uri == "project://config") {
        payload = CallDaemon(*project, nlohmann::json{{"cmd", "info"}});
        if (payload.is_object() && payload.contains("project") && payload["project"].contains("config")) payload = payload["project"]["config"];
    } else if (uri.rfind("project://file/", 0) == 0) {
        mime_type = "text/plain";
        const auto rel_path = DecodeUriComponent(uri.substr(std::string("project://file/").size()));
        payload = CallDaemon(*project, nlohmann::json{{"cmd", "read"}, {"path", rel_path}, {"encoding", "utf8"}});
        if (!payload.value("ok", false)) return payload;
        return nlohmann::json{{"contents", nlohmann::json::array({{{"uri", uri}, {"mimeType", mime_type}, {"text", payload.value("content", std::string{})}}})}};
    } else {
        return ErrorResponse("resource_not_found", "未知 resource: " + uri);
    }

    if (!payload.value("ok", true) && payload.contains("error")) return payload;
    return nlohmann::json{{"contents", nlohmann::json::array({{{"uri", uri}, {"mimeType", mime_type}, {"text", payload.dump(2)}}})}};
}

nlohmann::json CallMcpTool(const std::string& tool_name,
                          const nlohmann::json& arguments,
                          std::optional<ProjectIdentity>* active_project) {
    nlohmann::json tool_result;
    if (tool_name == "init_project") {
        const auto target_dir = arguments.value("path", arguments.value("target_dir", std::string{}));
        const auto template_name = arguments.value("template", std::string{"preact-jsx"});
        if (target_dir.empty()) return ErrorResponse("invalid_args", "path 不能为空");
        InitProjectResult result;
        std::string err;
        if (!InitProject(std::filesystem::path(target_dir), template_name, &result, &err)) {
            return ErrorResponse("init_failed", err);
        }
        tool_result = nlohmann::json{{"ok", true},
                                     {"project_root", result.project_root.string()},
                                     {"project_name", result.project_name},
                                     {"template", result.template_name},
                                     {"files_created", result.files_created},
                                     {"next_step", result.next_step}};
    } else if (tool_name == "open_project") {
        const auto root = arguments.value("path", arguments.value("project_root", std::string{}));
        std::string err;
        auto identity = ResolveProjectIdentity(root, true, &err);
        if (!identity.has_value()) return ErrorResponse("project_not_found", err);
        if (!EnsureDaemonRunning(*identity, &err)) return ErrorResponse("daemon_start_failed", err);
        tool_result = CallDaemon(*identity, nlohmann::json{{"cmd", "open"}, {"project_root", identity->project_root.string()}});
        if (tool_result.value("ok", false) && active_project) *active_project = *identity;
    } else if (tool_name == "read_file") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        const auto path = arguments.value("path", std::string{});
        if (path.empty()) return ErrorResponse("invalid_args", "path 不能为空");
        nlohmann::json req{{"cmd", "read"}, {"path", path}};
        const auto encoding = arguments.value("encoding", std::string{});
        if (!encoding.empty()) req["encoding"] = encoding;
        tool_result = CallDaemon(*project, req);
    } else if (tool_name == "write_file") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        const auto path = arguments.value("path", std::string{});
        if (path.empty()) return ErrorResponse("invalid_args", "path 不能为空");
        if (!arguments.contains("content") || !arguments.at("content").is_string()) return ErrorResponse("invalid_args", "content 不能为空");
        nlohmann::json req{{"cmd", "write"}, {"path", path}, {"content", arguments.at("content")}};
        const auto encoding = arguments.value("encoding", std::string{});
        if (!encoding.empty()) req["encoding"] = encoding;
        tool_result = CallDaemon(*project, req);
    } else if (tool_name == "get_project_info") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", "info"}});
    } else if (tool_name == "build" || tool_name == "build_project") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        nlohmann::json req{{"cmd", "build"}};
        if (arguments.value("watch", false)) req["watch"] = true;
        tool_result = CallDaemonWithTimeout(*project, req, kBuildRequestTimeoutMs);
    } else if (tool_name == "get_build_status") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", "build_status"}});
    } else if (tool_name == "query_element" || tool_name == "inspect" || tool_name == "click") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        const auto selector = arguments.value("selector", std::string{});
        if (selector.empty()) return ErrorResponse("invalid_args", "selector 不能为空");
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", tool_name}, {"selector", selector}});
    } else if (tool_name == "input_text") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        const auto selector = arguments.value("selector", std::string{});
        const auto text = arguments.value("text", std::string{});
        if (selector.empty()) return ErrorResponse("invalid_args", "selector 不能为空");
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", "input_text"}, {"selector", selector}, {"text", text}});
    } else if (tool_name == "scroll") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        const auto selector = arguments.value("selector", std::string{});
        if (selector.empty()) return ErrorResponse("invalid_args", "selector 不能为空");
        nlohmann::json req{{"cmd", "scroll"}, {"selector", selector}};
        if (arguments.contains("x")) req["x"] = arguments["x"];
        if (arguments.contains("y")) req["y"] = arguments["y"];
        if (!req.contains("x") && !req.contains("y")) return ErrorResponse("invalid_args", "scroll 至少需要 x 或 y");
        tool_result = CallDaemon(*project, req);
    } else if (tool_name == "highlight") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        const auto selector = arguments.value("selector", std::string{});
        if (selector.empty()) return ErrorResponse("invalid_args", "selector 不能为空");
        nlohmann::json req{{"cmd", "highlight"}, {"selector", selector}};
        const auto color = arguments.value("color", std::string{});
        if (!color.empty()) req["color"] = color;
        tool_result = CallDaemon(*project, req);
    } else if (tool_name == "snapshot_ui" || tool_name == "snapshot") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", "snapshot"}});
    } else if (tool_name == "get_console_logs" || tool_name == "logs") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", "logs"}});
    } else if (tool_name == "get_js_errors" || tool_name == "errors") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", "errors"}});
    } else if (tool_name == "eval_js") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        const auto code = arguments.value("code", std::string{});
        if (code.empty()) return ErrorResponse("invalid_args", "code 不能为空");
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", "eval"}, {"code", code}});
    } else if (tool_name == "reload" || tool_name == "reload_runtime") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", "reload"}});
    } else if (tool_name == "stop_daemon") {
        std::string err;
        const auto project = ResolveMcpProject(active_project ? *active_project : std::optional<ProjectIdentity>{}, &err);
        if (!project.has_value()) return ErrorResponse("project_not_resolved", err);
        tool_result = CallDaemon(*project, nlohmann::json{{"cmd", "stop"}});
        if (tool_result.value("ok", false) && active_project) active_project->reset();
    } else {
        return ErrorResponse("tool_not_found", "未知工具: " + tool_name);
    }

    return nlohmann::json{{"content", nlohmann::json::array({{{"type", "text"}, {"text", tool_result.dump(2)}}})},
                          {"structuredContent", tool_result},
                          {"isError", !tool_result.value("ok", false)}};
}


}  // namespace

int main(int argc, char** argv) {
    ConfigureConsoleForUtf8();

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    if (args.empty()) {
        PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev <daemon|stop|init|open|info|read|write|build|build-status|snapshot|logs|errors|eval <code>|query <selector>|inspect <selector>|click <selector>|input-text <selector> <text>|scroll <selector> [--x <num>] [--y <num>]|highlight <selector> [--color <css-color>]|reload|serve> ..."));
        return 1;
    }

    const auto& cmd = args[0];
    if (cmd == "init") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev init <target-dir> [--template preact-jsx|preact-ts|vanilla-js|python-host|rust-host]"));
            return 1;
        }
        const std::string template_name = [&args]() {
            const auto value = GetOptionValue(args, "--template");
            return value.empty() ? std::string("preact-jsx") : value;
        }();
        InitProjectResult result;
        std::string err;
        if (!InitProject(std::filesystem::path(args[1]), template_name, &result, &err)) {
            PrintJson(ErrorResponse("init_failed", err));
            return 1;
        }
        PrintJson(nlohmann::json{{"ok", true},
                                 {"project_root", result.project_root.string()},
                                 {"project_name", result.project_name},
                                 {"template", result.template_name},
                                 {"supported_templates", ListSupportedInitTemplates()},
                                 {"files_created", result.files_created},
                                 {"next_step", result.next_step}});
        return 0;
    }
    if (cmd == "daemon") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev daemon <start|stop|status|run>"));
            return 1;
        }
        const auto action = args[1];
        if (action == "run") return RunDaemonServer(args);
        if (action == "start") {
            std::string err;
            const auto identity = ResolveCommandProject(args, args.size(), true, &err);
            if (!identity.has_value()) {
                PrintJson(ErrorResponse("project_not_found", err));
                return 1;
            }
            if (!EnsureDaemonRunning(*identity, &err)) {
                PrintJson(ErrorResponse("daemon_start_failed", err));
                return 1;
            }
            PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "status"}}));
            return 0;
        }
        if (action == "stop") {
            std::string err;
            const auto identity = ResolveCommandProject(args, args.size(), false, &err);
            if (!identity.has_value()) {
                PrintJson(ErrorResponse("project_not_found", err));
                return 1;
            }
            PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "stop"}}));
            return 0;
        }
        if (action == "status") {
            std::string err;
            const auto identity = ResolveCommandProject(args, args.size(), false, &err);
            if (!identity.has_value()) {
                PrintJson(ErrorResponse("project_not_found", err));
                return 1;
            }
            const auto state = LoadState(identity->project_id, nullptr);
            if (state.has_value() && state->running && state->pid > 0 && IsProcessRunning(state->pid)) {
                PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "status"}}));
            } else {
                PrintJson(nlohmann::json{{"ok", true},
                                         {"project_id", identity->project_id},
                                         {"project_root", identity->project_root.string()},
                                         {"daemon", nlohmann::json{{"running", false}}}});
            }
            return 0;
        }
        PrintJson(ErrorResponse("invalid_args", "未知 daemon 子命令；`eval` 应直接使用 `mbink-ui-dev eval <code>`"));
        return 1;
    }

    if (cmd == "stop") {
        std::string err;
        const auto identity = ResolveCommandProject(args, args.size(), false, &err);
        if (!identity.has_value()) {
            PrintJson(ErrorResponse("project_not_found", err));
            return 1;
        }
        const auto state = LoadState(identity->project_id, nullptr);
        if (!state.has_value() || !state->running || state->pid <= 0 || !IsProcessRunning(state->pid)) {
            std::string rm_err;
            RemoveState(identity->project_id, &rm_err);
            PrintJson(nlohmann::json{{"ok", true},
                                     {"project_id", identity->project_id},
                                     {"project_root", identity->project_root.string()},
                                     {"stopped", false},
                                     {"already_stopped", true}});
            return 0;
        }
        const auto response = CallDaemon(*identity, nlohmann::json{{"cmd", "stop"}});
        PrintJson(response);
        return response.value("ok", false) ? 0 : 1;
    }

    if (cmd == "open") {
        std::string err;
        const auto identity = ResolveCommandProject(args, 1, true, &err);
        if (!identity.has_value()) {
            PrintJson(ErrorResponse("project_not_found", err));
            return 1;
        }
        if (!EnsureDaemonRunning(*identity, &err)) {
            PrintJson(ErrorResponse("daemon_start_failed", err));
            return 1;
        }
        PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "open"}, {"project_root", identity->project_root.string()}}));
        return 0;
    }

    if (cmd == "serve") {
        std::optional<ProjectIdentity> active_project;
        std::string serve_project_error;
        if (const auto initial_project = ResolveCommandProject(args, args.size(), true, &serve_project_error); initial_project.has_value()) {
            if (!EnsureDaemonRunning(*initial_project, &serve_project_error)) {
                PrintJsonLine(MakeJsonRpcError(nullptr, -32000, serve_project_error));
                return 1;
            }
            active_project = *initial_project;
        }
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            const auto req = nlohmann::json::parse(line, nullptr, false);
            if (req.is_discarded()) {
                PrintJsonLine(MakeJsonRpcError(nullptr, -32700, "parse error"));
                continue;
            }

            const auto id = req.contains("id") ? req.at("id") : nlohmann::json(nullptr);
            const auto method = req.value("method", std::string{});
            const auto params = req.value("params", nlohmann::json::object());

            if (method == "initialize") {
                PrintJsonLine(MakeJsonRpcResult(id,
                                                nlohmann::json{{"protocolVersion", "2024-11-05"},
                                                               {"capabilities", {{"tools", nlohmann::json::object()}, {"resources", nlohmann::json::object()}}},
                                                               {"serverInfo", {{"name", "mbink-ui-dev"}, {"version", "0.1.0"}}}}));
                continue;
            }
            if (method == "tools/list") {
                PrintJsonLine(MakeJsonRpcResult(id, nlohmann::json{{"tools", BuildMcpTools()}}));
                continue;
            }
            if (method == "resources/list") {
                PrintJsonLine(MakeJsonRpcResult(id, nlohmann::json{{"resources", BuildMcpResources()}}));
                continue;
            }
            if (method == "resources/read") {
                const auto uri = params.value("uri", std::string{});
                if (uri.empty()) {
                    PrintJsonLine(MakeJsonRpcError(id, -32602, "resource uri is required"));
                    continue;
                }
                const auto resource = ReadMcpResource(active_project, uri);
                if (resource.contains("error")) {
                    PrintJsonLine(MakeJsonRpcError(id, -32002, resource["error"].value("message", std::string{"resource read failed"})));
                    continue;
                }
                PrintJsonLine(MakeJsonRpcResult(id, resource));
                continue;
            }
            if (method == "tools/call") {
                const auto tool_name = params.value("name", std::string{});
                const auto arguments = params.value("arguments", nlohmann::json::object());
                if (tool_name.empty()) {
                    PrintJsonLine(MakeJsonRpcError(id, -32602, "tool name is required"));
                    continue;
                }
                PrintJsonLine(MakeJsonRpcResult(id, CallMcpTool(tool_name, arguments, &active_project)));
                continue;
            }
            if (method == "notifications/initialized") continue;
            PrintJsonLine(MakeJsonRpcError(id, -32601, "method not found"));
        }
        return 0;
    }


    // 其余命令：要求 daemon 已运行
    std::string command_error;
    const auto identity = RequireRunningDaemonProject(args, args.size(), true, &command_error);
    if (!identity.has_value()) {
        PrintJson(ErrorResponse("daemon_not_running", command_error));
        return 1;
    }

    if (cmd == "snapshot") PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "snapshot"}}));
    else if (cmd == "info") PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "info"}}));
    else if (cmd == "read") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev read <path> [--encoding utf8|base64]"));
            return 1;
        }
        const auto encoding = GetOptionValue(args, "--encoding");
        nlohmann::json req{{"cmd", "read"}, {"path", args[1]}};
        if (!encoding.empty()) req["encoding"] = encoding;
        PrintJson(CallDaemon(*identity, req));
    }
    else if (cmd == "write") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev write <path> [--content <text> | --from <file>] [--encoding utf8|base64]"));
            return 1;
        }
        const auto from = GetOptionValue(args, "--from");
        const auto inline_content = GetOptionValue(args, "--content");
        const auto encoding = GetOptionValue(args, "--encoding");
        if (!from.empty() && !inline_content.empty()) {
            PrintJson(ErrorResponse("invalid_args", "--content 与 --from 不能同时指定"));
            return 1;
        }
        std::string content;
        if (!from.empty()) {
            if (!ReadFileBytes(std::filesystem::absolute(from), &content)) {
                PrintJson(ErrorResponse("file_read_failed", "读取 --from 文件失败"));
                return 1;
            }
        } else if (!inline_content.empty()) {
            content = inline_content;
        } else {
            content = ReadStdinBytes();
        }
        const std::string resolved_encoding = encoding.empty() ? "utf8" : encoding;
        nlohmann::json req{{"cmd", "write"}, {"path", args[1]}, {"encoding", resolved_encoding}};
        req["content"] = resolved_encoding == "base64" ? Base64Encode(content) : content;
        PrintJson(CallDaemon(*identity, req));
    }
    else if (cmd == "build") {
        nlohmann::json req{{"cmd", "build"}};
        if (std::find(args.begin() + 1, args.end(), std::string("--watch")) != args.end()) req["watch"] = true;
        PrintJson(CallDaemonWithTimeout(*identity, req, kBuildRequestTimeoutMs));
    }
    else if (cmd == "build-status") PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "build_status"}}));
    else if (cmd == "logs") PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "logs"}}));
    else if (cmd == "errors") PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "errors"}}));
    else if (cmd == "query" || cmd == "query-element") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev query <selector>"));
            return 1;
        }
        PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "query_element"}, {"selector", args[1]}}));
    }
    else if (cmd == "inspect") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev inspect <selector>"));
            return 1;
        }
        PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "inspect"}, {"selector", args[1]}}));
    }
    else if (cmd == "click") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev click <selector>"));
            return 1;
        }
        PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "click"}, {"selector", args[1]}}));
    }
    else if (cmd == "input-text") {
        if (args.size() < 3) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev input-text <selector> <text>"));
            return 1;
        }
        std::string text = args[2];
        for (size_t i = 3; i < args.size(); ++i) {
            if (args[i].rfind("--", 0) == 0) break;
            text += " " + args[i];
        }
        PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "input_text"}, {"selector", args[1]}, {"text", text}}));
    }
    else if (cmd == "scroll") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev scroll <selector> [--x <num>] [--y <num>]"));
            return 1;
        }
        nlohmann::json req{{"cmd", "scroll"}, {"selector", args[1]}};
        const auto x = GetOptionValue(args, "--x");
        const auto y = GetOptionValue(args, "--y");
        double number = 0.0;
        if (!x.empty()) {
            if (!TryParseNumber(x, &number)) {
                PrintJson(ErrorResponse("invalid_args", "--x 必须是数字"));
                return 1;
            }
            req["x"] = number;
        }
        if (!y.empty()) {
            if (!TryParseNumber(y, &number)) {
                PrintJson(ErrorResponse("invalid_args", "--y 必须是数字"));
                return 1;
            }
            req["y"] = number;
        }
        if (!req.contains("x") && !req.contains("y")) {
            PrintJson(ErrorResponse("invalid_args", "scroll 至少需要 --x 或 --y"));
            return 1;
        }
        PrintJson(CallDaemon(*identity, req));
    }
    else if (cmd == "highlight") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev highlight <selector> [--color <css-color>]"));
            return 1;
        }
        nlohmann::json req{{"cmd", "highlight"}, {"selector", args[1]}};
        const auto color = GetOptionValue(args, "--color");
        if (!color.empty()) req["color"] = color;
        PrintJson(CallDaemon(*identity, req));
    }
    else if (cmd == "eval") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev eval <code>"));
            return 1;
        }
        std::string code = args[1];
        for (size_t i = 2; i < args.size(); ++i) code += " " + args[i];
        PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "eval"}, {"code", code}}));
    }
    else if (cmd == "reload") PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "reload"}}));
    else {
        PrintJson(ErrorResponse("invalid_args", "未知命令"));
        return 1;
    }

    return 0;
}
