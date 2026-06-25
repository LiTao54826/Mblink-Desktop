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
#include <thread>
#include <chrono>


#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

using mblink::ui_dev::DaemonState;
using mblink::ui_dev::ErrorResponse;
using mblink::ui_dev::EnsureProjectIdentity;
using mblink::ui_dev::FindProjectIdentityUpwards;
using mblink::ui_dev::GetDaemonPipeName;
using mblink::ui_dev::IsProcessRunning;
using mblink::ui_dev::ListManagedProjects;
using mblink::ui_dev::LoadState;
using mblink::ui_dev::ProjectIdentity;
using mblink::ui_dev::RemoveState;
using mblink::ui_dev::RunDaemonServer;
using mblink::ui_dev::SendDaemonRequest;
using mblink::ui_dev::StartDetachedDaemon;
using mblink::ui_dev::WaitForDaemonReady;
using mblink::ui_dev::InitProject;
using mblink::ui_dev::InitProjectOptions;
using mblink::ui_dev::InitProjectResult;
using mblink::ui_dev::ListSupportedInitCombinations;
using mblink::ui_dev::ListSupportedInitTemplates;
using mblink::ui_dev::PathFromUtf8;
using mblink::ui_dev::PathToUtf8;


namespace {

constexpr int kDaemonStartupTimeoutMs = 10000;
constexpr int kDaemonRequestTimeoutMs = 10000;
constexpr int kBuildRequestTimeoutMs = 300000;
constexpr int kDaemonOpenRetryCount = 3;
constexpr int kDaemonOpenRetryDelayMs = 250;
constexpr size_t kMcpInlineTextMaxBytes = 128 * 1024;

std::string GetOptionValue(const std::vector<std::string>& args, const std::string& key);

bool HasOption(const std::vector<std::string>& args, const std::string& key) {
    return std::find(args.begin(), args.end(), key) != args.end();
}

void ConfigureConsoleForUtf8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

std::vector<std::string> CollectCommandLineArgsUtf8(int argc, char** argv) {
    std::vector<std::string> args;
#ifdef _WIN32
    int wide_argc = 0;
    LPWSTR* wide_argv = CommandLineToArgvW(GetCommandLineW(), &wide_argc);
    if (wide_argv) {
        args.reserve(static_cast<size_t>(wide_argc > 1 ? wide_argc - 1 : 0));
        for (int i = 1; i < wide_argc; ++i) {
            args.push_back(mblink::ui_dev::WideToUtf8(wide_argv[i]));
        }
        LocalFree(wide_argv);
        return args;
    }
#endif
    args.reserve(static_cast<size_t>(argc > 1 ? argc - 1 : 0));
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
    return args;
}

void PrintJson(const nlohmann::json& j) {
    std::cout << j.dump(2) << std::endl;
}

nlohmann::json SnapshotRequestFromResponseMode(const std::string& response_mode) {
    nlohmann::json req{{"cmd", "snapshot"}};
    if (!response_mode.empty()) req["response_mode"] = response_mode;
    return req;
}

void AddScreenshotOptionsIfPresent(const std::vector<std::string>& args,
                                   nlohmann::json* req) {
    if (!req) return;
    if (HasOption(args, "--include-screenshot")) {
        (*req)["include_screenshot"] = true;
    }
    if (HasOption(args, "--inline-screenshot")) {
        (*req)["include_screenshot"] = true;
        (*req)["inline_screenshot"] = true;
    }
}

void AddIntOptionIfPresent(const std::vector<std::string>& args,
                           const std::string& option,
                           const std::string& field,
                           nlohmann::json* req) {
    const auto value = GetOptionValue(args, option);
    if (value.empty() || !req) return;
    try {
        (*req)[field] = std::stoi(value);
    } catch (...) {
    }
}

std::string SummarizeToolResultForText(const nlohmann::json& value) {
    auto text_value = value;
    if (text_value.is_object()) {
        text_value.erase("screenshot_base64");
        if (text_value.contains("screenshot") && text_value["screenshot"].is_object()) {
            text_value["screenshot"].erase("base64");
        }
    }
    const auto rendered = text_value.dump(2);
    if (rendered.size() <= kMcpInlineTextMaxBytes) return rendered;
    nlohmann::json summary{{"ok", value.value("ok", true)},
                           {"truncated", true},
                           {"bytes", rendered.size()},
                           {"message", "structuredContent is large; use structuredContent or snapshot.path instead of text"}};
    if (value.is_object()) {
        for (const auto& key : {"response_mode", "source", "snapshot", "screenshot", "viewport", "timestamp", "inline_limit_bytes", "note"}) {
            if (value.contains(key)) summary[key] = value.at(key);
        }
        if (summary.contains("screenshot") && summary["screenshot"].is_object()) {
            summary["screenshot"].erase("base64");
        }
    }
    return summary.dump(2);
}

nlohmann::json InitProjectResultToJson(const InitProjectResult& result) {
    nlohmann::json warnings = nlohmann::json::array();
    for (const auto& warning : result.warnings) {
        warnings.push_back({{"code", "template_selection_warning"}, {"message", warning}});
    }
    return nlohmann::json{{"ok", true},
                          {"project_root", PathToUtf8(result.project_root)},
                          {"project_name", result.project_name},
                          {"template", result.template_name},
                          {"purpose", result.purpose},
                          {"runtime", result.runtime},
                          {"canonical_key", result.canonical_key},
                          {"requested", {{"purpose", result.requested_purpose}, {"runtime", result.requested_runtime}, {"legacy_template", result.legacy_template}, {"used_default", result.used_default}}},
                          {"resolved", {{"purpose", result.purpose}, {"runtime", result.runtime}, {"canonical_key", result.canonical_key}, {"layers", result.layers}}},
                          {"legacy_template", result.legacy_template},
                          {"used_default", result.used_default},
                          {"layers", result.layers},
                          {"warnings", warnings},
                          {"supported_purposes", nlohmann::json::array({"minimal", "showcase", "desktop-app"})},
                          {"supported_runtimes", nlohmann::json::array({"tool", "python", "rust", "go"})},
                          {"supported_combinations", ListSupportedInitCombinations()},
                          {"default_combination", "minimal/tool"},
                          {"legacy_templates", ListSupportedInitTemplates()},
                          {"files_created", result.files_created},
                          {"next_step", result.next_step}};
}

std::string InitErrorCode(const std::string& message) {
    if (message.find("must be provided together") != std::string::npos) return "invalid_args.partial_canonical_input";
    if (message.find("conflicts") != std::string::npos) return "invalid_args.legacy_canonical_conflict";
    if (message.find("unsupported purpose") != std::string::npos) return "invalid_args.unsupported_purpose";
    if (message.find("unsupported runtime") != std::string::npos) return "invalid_args.unsupported_runtime";
    if (message.find("unsupported legacy template") != std::string::npos) return "invalid_args.unsupported_legacy_template";
    return "init_failed";
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

bool Base64Decode(const std::string& input, std::string* output, std::string* error) {
    if (error) error->clear();
    if (!output) {
        if (error) *error = "Base64Decode output 不能为空";
        return false;
    }
    output->clear();
    auto decode = [](char ch) -> int {
        if (ch >= 'A' && ch <= 'Z') return ch - 'A';
        if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
        if (ch >= '0' && ch <= '9') return ch - '0' + 52;
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        return -1;
    };
    int value = 0;
    int bits = -8;
    for (char ch : input) {
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
        if (ch == '=') break;
        const int decoded = decode(ch);
        if (decoded < 0) {
            if (error) *error = "--code-base64 不是有效 Base64";
            return false;
        }
        value = (value << 6) | decoded;
        bits += 6;
        if (bits >= 0) {
            output->push_back(static_cast<char>((value >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return true;
}


std::filesystem::path GetExecutablePath() {
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (n == 0) return {};
        if (n < buffer.size()) {
            buffer.resize(n);
            return std::filesystem::path(buffer);
        }
        buffer.resize(buffer.size() * 2);
    }
#else
    return std::filesystem::current_path();
#endif
}

std::optional<std::filesystem::path> NormalizeProjectRoot(const std::string& project_hint, std::string* error) {
    if (error) error->clear();
    if (project_hint.empty()) return std::nullopt;

    auto root = std::filesystem::absolute(PathFromUtf8(project_hint)).lexically_normal();
    if (std::filesystem::exists(root) && !std::filesystem::is_directory(root)) root = root.parent_path();
    if (root.empty() || !std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        if (error) *error = "project directory does not exist: " + project_hint;
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
        if (std::filesystem::exists(current / "mblink.config.json", ec)) return current;
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

    if (error) *error = "No project found. Run in a project directory or pass --project <path> / open <path>";
    return std::nullopt;
}

std::optional<ProjectIdentity> FindManagedProjectByRoot(const std::filesystem::path& root,
                                                        std::string* error) {
    std::string list_error;
    auto managed_projects = ListManagedProjects(&list_error);
    if (!list_error.empty()) {
        if (error) *error = list_error;
        return std::nullopt;
    }

    const auto normalized_root = std::filesystem::absolute(root).lexically_normal();
    for (const auto& identity : managed_projects) {
        if (identity.project_root.empty()) continue;
        const auto candidate = std::filesystem::absolute(identity.project_root).lexically_normal();
        if (candidate == normalized_root) return identity;
    }
    return std::nullopt;
}

std::optional<ProjectIdentity> ResolveExistingProjectIdentity(const std::string& project_hint,
                                                             std::string* error) {
    if (error) error->clear();
    if (project_hint.empty()) return ResolveProjectIdentity("", false, error);

    const auto root = NormalizeProjectRoot(project_hint, error);
    if (!root.has_value()) return std::nullopt;
    std::string lookup_error;
    if (const auto identity = FindManagedProjectByRoot(*root, &lookup_error); identity.has_value()) {
        return identity;
    }
    if (!lookup_error.empty()) {
        if (error) *error = lookup_error;
        return std::nullopt;
    }
    return ResolveProjectIdentity(project_hint, false, error);
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

bool EnsureDaemonRunning(const ProjectIdentity& identity, std::string* error, bool* started_now = nullptr) {
    if (started_now) *started_now = false;
    if (HasRunningDaemon(identity, true)) return true;

    int pid = 0;
    if (!StartDetachedDaemon(GetExecutablePath(), PathToUtf8(identity.project_root), identity.project_id, &pid, error)) {
        return false;
    }
    if (started_now) *started_now = true;
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

int PrintDaemonResponse(nlohmann::json response) {
    const bool ok = response.value("ok", false);
    PrintJson(response);
    return ok ? 0 : 1;
}

nlohmann::json OpenProjectViaDaemon(const ProjectIdentity& identity, bool started_now, bool force = false) {
    auto request = nlohmann::json{{"cmd", "open"}, {"project_root", PathToUtf8(identity.project_root)}};
    if (force) request["force"] = true;
    auto response = CallDaemon(identity, request);
    if (!started_now) return response;
    for (int attempt = 1; attempt < kDaemonOpenRetryCount; ++attempt) {
        if (response.value("ok", false)) return response;
        if (!response.contains("error") || response["error"].value("code", std::string{}) != "ipc_failed") return response;
        std::this_thread::sleep_for(std::chrono::milliseconds(kDaemonOpenRetryDelayMs));
        response = CallDaemon(identity, request);
    }
    return response;
}

bool ResolveEvalCode(const std::vector<std::string>& args, std::string* code, std::string* error) {
    if (error) error->clear();
    if (!code) {
        if (error) *error = "code 不能为空";
        return false;
    }
    const auto code_base64 = GetOptionValue(args, "--code-base64");
    const auto from = GetOptionValue(args, "--from");
    if (!code_base64.empty()) return Base64Decode(code_base64, code, error);
    if (!from.empty()) {
        if (from == "-") {
            *code = ReadStdinBytes();
            return true;
        }
        if (!ReadFileBytes(std::filesystem::absolute(PathFromUtf8(from)), code)) {
            if (error) *error = "读取 --from 文件失败";
            return false;
        }
        return true;
    }
    code->clear();
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--project" || args[i] == "--code-base64" || args[i] == "--from") {
            ++i;
            continue;
        }
        if (args[i].rfind("--", 0) == 0) continue;
        if (!code->empty()) *code += " ";
        *code += args[i];
    }
    if (!code->empty()) return true;
    if (error) *error = "用法: mblink-ui-dev eval <code> | --code-base64 <base64> | --from <file|->";
    return false;
}

bool OptionConsumesNextValue(const std::string& arg) {
    return arg == "--project" || arg == "--encoding" || arg == "--content" || arg == "--from" ||
           arg == "--x" || arg == "--y" || arg == "--color" || arg == "--code-base64" ||
           arg == "--response" || arg == "--response-mode" ||
           arg == "--max-nodes" || arg == "--max-depth" || arg == "--root-selector" || arg == "--limit";
}

std::vector<std::string> CollectPositionalArgs(const std::vector<std::string>& args) {
    std::vector<std::string> positional;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i].rfind("--", 0) == 0) {
            if (OptionConsumesNextValue(args[i])) ++i;
            continue;
        }
        positional.push_back(args[i]);
    }
    return positional;
}

std::string GetProjectHint(const std::vector<std::string>& args, size_t positional_index) {
    const auto explicit_project = GetOptionValue(args, "--project");
    if (!explicit_project.empty()) return explicit_project;
    const auto positional = CollectPositionalArgs(args);
    if (positional_index > 0 && positional_index - 1 < positional.size()) return positional[positional_index - 1];
    return "";
}


std::optional<ProjectIdentity> ResolveCommandProject(const std::vector<std::string>& args,
                                                    size_t positional_index,
                                                    bool allow_auto_create,
                                                    std::string* error) {
    return ResolveProjectIdentity(GetProjectHint(args, positional_index), allow_auto_create, error);
}

std::optional<ProjectIdentity> ResolveExistingCommandProject(const std::vector<std::string>& args,
                                                            size_t positional_index,
                                                            std::string* error) {
    return ResolveExistingProjectIdentity(GetProjectHint(args, positional_index), error);
}

std::optional<ProjectIdentity> RequireRunningDaemonProject(const std::vector<std::string>& args,
                                                          size_t positional_index,
                                                          bool allow_auto_create,
                                                          std::string* error) {
    auto identity = ResolveCommandProject(args, positional_index, allow_auto_create, error);
    if (!identity.has_value()) return std::nullopt;
    if (!HasRunningDaemon(*identity, true)) {
        if (error) *error = "Run open or daemon start first";
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
    auto tools = nlohmann::json::array({
        {{"name", "open_project"},
         {"description", "Open a project and start or reuse its runtime"},
         {"inputSchema", {{"type", "object"},
                          {"properties", {{"path", {{"type", "string"}}},
                                          {"project_root", {{"type", "string"}}},
                                          {"force", {{"type", "boolean"}}}}}}}},
        {{"name", "init_project"}, {"description", "Initialize a new MBlink UI Dev project"}, {"inputSchema", {{"type", "object"}, {"additionalProperties", false}, {"properties", {{"path", {{"type", "string"}}}, {"purpose", {{"type", "string"}, {"enum", nlohmann::json::array({"minimal", "showcase", "desktop-app"})}}}, {"runtime", {{"type", "string"}, {"enum", nlohmann::json::array({"tool", "python", "rust", "go"})}}}, {"template", {{"type", "string"}, {"deprecated", true}, {"enum", nlohmann::json(ListSupportedInitTemplates())}}}}}, {"required", nlohmann::json::array({"path"})}}}},
        {{"name", "get_project_info"}, {"description", "Get current project information"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "read_file"}, {"description", "Read a project file"}, {"inputSchema", {{"type", "object"}, {"properties", {{"path", {{"type", "string"}}}, {"encoding", {{"type", "string"}, {"enum", nlohmann::json::array({"utf8", "base64"})}}}}}, {"required", nlohmann::json::array({"path"})}}}},
        {{"name", "write_file"}, {"description", "Write a project file"}, {"inputSchema", {{"type", "object"}, {"properties", {{"path", {{"type", "string"}}}, {"content", {{"type", "string"}}}, {"encoding", {{"type", "string"}, {"enum", nlohmann::json::array({"utf8", "base64"})}}}}}, {"required", nlohmann::json::array({"path", "content"})}}}},
        {{"name", "build"}, {"description", "Build the project"}, {"inputSchema", {{"type", "object"}, {"properties", {{"watch", {{"type", "boolean"}}}}}}}},
        {{"name", "get_build_status"}, {"description", "Get the latest build status"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "reload"}, {"description", "Reload the current runtime"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "eval_js"}, {"description", "Evaluate JavaScript in the runtime"}, {"inputSchema", {{"type", "object"}, {"properties", {{"code", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"code"})}}}},
        {{"name", "query_element"}, {"description", "Query elements by selector"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}, {"limit", {{"type", "integer"}, {"minimum", 0}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "inspect"}, {"description", "Inspect one element"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "click"}, {"description", "Click an element"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "input_text"}, {"description", "Input text into an element"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}, {"text", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"selector", "text"})}}}},
        {{"name", "scroll"}, {"description", "Scroll an element"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}, {"x", {{"type", "number"}}}, {"y", {{"type", "number"}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "highlight"}, {"description", "Highlight an element"}, {"inputSchema", {{"type", "object"}, {"properties", {{"selector", {{"type", "string"}}}, {"color", {{"type", "string"}}}}}, {"required", nlohmann::json::array({"selector"})}}}},
        {{"name", "snapshot_ui"}, {"description", "Get a UI snapshot"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "get_console_logs"}, {"description", "Get runtime console logs"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "get_js_errors"}, {"description", "Get runtime JavaScript errors"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}},
        {{"name", "stop_daemon"}, {"description", "Stop the daemon"}, {"inputSchema", {{"type", "object"}, {"properties", nlohmann::json::object()}}}}
    });
    for (auto& tool : tools) {
        if (tool.value("name", std::string{}) == "snapshot_ui") {
            tool["inputSchema"] = nlohmann::json{{"type", "object"},
                                                 {"properties", {{"response_mode", {{"type", "string"}, {"enum", nlohmann::json::array({"auto", "inline", "file"})}}},
                                                                 {"max_nodes", {{"type", "integer"}, {"minimum", 1}}},
                                                                 {"max_depth", {{"type", "integer"}, {"minimum", 1}}},
                                                                 {"root_selector", {{"type", "string"}}},
                                                                 {"include_screenshot", {{"type", "boolean"}}},
                                                                 {"inline_screenshot", {{"type", "boolean"}}}}}};
        } else if (tool.value("name", std::string{}) == "query_element") {
            tool["inputSchema"] = nlohmann::json{{"type", "object"},
                                                 {"properties", {{"selector", {{"type", "string"}}},
                                                                 {"limit", {{"type", "integer"}, {"minimum", 0}}}}},
                                                 {"required", nlohmann::json::array({"selector"})}};
        }
    }
    return tools;
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
        payload = CallDaemon(*project, nlohmann::json{{"cmd", "snapshot"}, {"response_mode", "auto"}});
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
        if (target_dir.empty()) return ErrorResponse("invalid_args", "path cannot be empty");
        InitProjectOptions options;
        options.purpose = arguments.value("purpose", std::string{});
        options.runtime = arguments.value("runtime", std::string{});
        options.legacy_template = arguments.value("template", std::string{});
        InitProjectResult result;
        std::string err;
        if (!InitProject(PathFromUtf8(target_dir), options, &result, &err)) {
            return ErrorResponse(InitErrorCode(err), err);
        }
        tool_result = InitProjectResultToJson(result);
    } else if (tool_name == "open_project") {
        const auto root = arguments.value("path", arguments.value("project_root", std::string{}));
        std::string err;
        bool started_now = false;
        auto identity = ResolveProjectIdentity(root, true, &err);
        if (!identity.has_value()) return ErrorResponse("project_not_found", err);
        if (!EnsureDaemonRunning(*identity, &err, &started_now)) return ErrorResponse("daemon_start_failed", err);
        tool_result = OpenProjectViaDaemon(*identity, started_now, arguments.value("force", false));
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
        nlohmann::json req{{"cmd", tool_name}, {"selector", selector}};
        if (arguments.contains("limit")) req["limit"] = arguments["limit"];
        tool_result = CallDaemon(*project, req);
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
        if (!req.contains("x") && !req.contains("y")) return ErrorResponse("invalid_args", "scroll requires at least x or y");
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
        auto response_mode = arguments.value("response_mode", arguments.value("mode", std::string{}));
        auto req = SnapshotRequestFromResponseMode(response_mode);
        if (arguments.contains("max_nodes")) req["max_nodes"] = arguments["max_nodes"];
        if (arguments.contains("max_depth")) req["max_depth"] = arguments["max_depth"];
        if (arguments.contains("root_selector")) req["root_selector"] = arguments["root_selector"];
        if (arguments.contains("include_screenshot")) req["include_screenshot"] = arguments["include_screenshot"];
        if (arguments.contains("inline_screenshot")) req["inline_screenshot"] = arguments["inline_screenshot"];
        tool_result = CallDaemon(*project, req);
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

    return nlohmann::json{{"content", nlohmann::json::array({{{"type", "text"}, {"text", SummarizeToolResultForText(tool_result)}}})},
                          {"structuredContent", tool_result},
                          {"isError", !tool_result.value("ok", false)}};
}


}  // namespace

int main(int argc, char** argv) {
    ConfigureConsoleForUtf8();

    const auto args = CollectCommandLineArgsUtf8(argc, argv);

    if (args.empty()) {
        PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev <daemon|stop|init|open|info|read|write|build|build-status|snapshot|logs|errors|eval <code>|query <selector>|inspect <selector>|click <selector>|input-text <selector> <text>|scroll <selector> [--x <num>] [--y <num>]|highlight <selector> [--color <css-color>]|reload|serve> ..."));
        return 1;
    }

    const auto& cmd = args[0];
    if (cmd == "init") {
        InitProjectOptions options;
        options.legacy_template = GetOptionValue(args, "--template");
        options.purpose = GetOptionValue(args, "--purpose");
        options.runtime = GetOptionValue(args, "--runtime");

        std::vector<std::string> positional;
        bool skip_next = false;
        for (size_t i = 1; i < args.size(); ++i) {
            if (skip_next) {
                skip_next = false;
                continue;
            }
            if (args[i] == "--template" || args[i] == "--purpose" || args[i] == "--runtime") {
                skip_next = true;
                continue;
            }
            if (args[i].rfind("--", 0) == 0) continue;
            positional.push_back(args[i]);
        }
        if (positional.size() > 1) {
            PrintJson(ErrorResponse("invalid_args", "usage: mblink-ui-dev init [target-dir] [--purpose minimal|showcase|desktop-app --runtime tool|python|rust|go] [--template legacy-name]"));
            return 1;
        }
        const auto target_dir = positional.empty() ? std::filesystem::current_path() : PathFromUtf8(positional.front());
        InitProjectResult result;
        std::string err;
        if (!InitProject(target_dir, options, &result, &err)) {
            PrintJson(ErrorResponse(InitErrorCode(err), err));
            return 1;
        }
        PrintJson(InitProjectResultToJson(result));
        return 0;
    }
    if (cmd == "daemon") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev daemon <start|stop|status|run>"));
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
            const auto identity = ResolveExistingCommandProject(args, args.size(), &err);
            if (!identity.has_value()) {
                PrintJson(ErrorResponse("project_not_found", err));
                return 1;
            }
            PrintJson(CallDaemon(*identity, nlohmann::json{{"cmd", "stop"}}));
            return 0;
        }
        if (action == "status") {
            std::string err;
            const auto identity = ResolveExistingCommandProject(args, args.size(), &err);
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
                                         {"project_root", PathToUtf8(identity->project_root)},
                                         {"daemon", nlohmann::json{{"running", false}}}});
            }
            return 0;
        }
        PrintJson(ErrorResponse("invalid_args", "Unknown daemon subcommand; use `mblink-ui-dev eval <code>` directly"));
        return 1;
    }

    if (cmd == "stop") {
        std::string err;
        const auto identity = ResolveExistingCommandProject(args, args.size(), &err);
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
                                     {"project_root", PathToUtf8(identity->project_root)},
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
        bool started_now = false;
        const bool force = std::find(args.begin() + 1, args.end(), std::string("--force")) != args.end();
        const auto identity = ResolveCommandProject(args, 1, true, &err);
        if (!identity.has_value()) {
            PrintJson(ErrorResponse("project_not_found", err));
            return 1;
        }
        if (!EnsureDaemonRunning(*identity, &err, &started_now)) {
            PrintJson(ErrorResponse("daemon_start_failed", err));
            return 1;
        }
        PrintJson(OpenProjectViaDaemon(*identity, started_now, force));
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
                                                               {"serverInfo", {{"name", "mblink-ui-dev"}, {"version", "0.1.0"}}}}));
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


    // Other commands require a running daemon
    if (cmd == "build") {
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
        nlohmann::json req{{"cmd", "build"}};
        if (std::find(args.begin() + 1, args.end(), std::string("--watch")) != args.end()) req["watch"] = true;
        const auto response = CallDaemonWithTimeout(*identity, req, kBuildRequestTimeoutMs);
        PrintJson(response);
        return response.value("ok", false) ? 0 : 1;
    }

    std::string command_error;
    const auto identity = RequireRunningDaemonProject(args, args.size(), true, &command_error);
    if (!identity.has_value()) {
        PrintJson(ErrorResponse("daemon_not_running", command_error));
        return 1;
    }
    const auto positional = CollectPositionalArgs(args);

    if (cmd == "snapshot") {
        auto response_mode = GetOptionValue(args, "--response-mode");
        if (response_mode.empty()) response_mode = GetOptionValue(args, "--response");
        auto req = SnapshotRequestFromResponseMode(response_mode);
        AddIntOptionIfPresent(args, "--max-nodes", "max_nodes", &req);
        AddIntOptionIfPresent(args, "--max-depth", "max_depth", &req);
        const auto root_selector = GetOptionValue(args, "--root-selector");
        if (!root_selector.empty()) req["root_selector"] = root_selector;
        AddScreenshotOptionsIfPresent(args, &req);
        PrintJson(CallDaemon(*identity, req));
        return 0;
    }
    else if (cmd == "info") return PrintDaemonResponse(CallDaemon(*identity, nlohmann::json{{"cmd", "info"}}));
    else if (cmd == "read") {
        if (positional.empty()) {
            PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev read <path> [--encoding utf8|base64]"));
            return 1;
        }
        const auto encoding = GetOptionValue(args, "--encoding");
        nlohmann::json req{{"cmd", "read"}, {"path", positional[0]}};
        if (!encoding.empty()) req["encoding"] = encoding;
        PrintJson(CallDaemon(*identity, req));
    }
    else if (cmd == "write") {
        if (positional.empty()) {
            PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev write <path> [--content <text> | --from <file>] [--encoding utf8|base64]"));
            return 1;
        }
        const auto from = GetOptionValue(args, "--from");
        const auto inline_content = GetOptionValue(args, "--content");
        const auto encoding = GetOptionValue(args, "--encoding");
        if (!from.empty() && !inline_content.empty()) {
            PrintJson(ErrorResponse("invalid_args", "--content and --from cannot both be specified"));
            return 1;
        }
        std::string content;
        if (!from.empty()) {
            if (!ReadFileBytes(std::filesystem::absolute(PathFromUtf8(from)), &content)) {
                PrintJson(ErrorResponse("file_read_failed", "读取 --from 文件失败"));
                return 1;
            }
        } else if (!inline_content.empty()) {
            content = inline_content;
        } else {
            content = ReadStdinBytes();
        }
        const std::string resolved_encoding = encoding.empty() ? "utf8" : encoding;
        nlohmann::json req{{"cmd", "write"}, {"path", positional[0]}, {"encoding", resolved_encoding}};
        req["content"] = resolved_encoding == "base64" ? Base64Encode(content) : content;
        return PrintDaemonResponse(CallDaemon(*identity, req));
    }
    else if (cmd == "build-status") return PrintDaemonResponse(CallDaemon(*identity, nlohmann::json{{"cmd", "build_status"}}));
    else if (cmd == "logs") return PrintDaemonResponse(CallDaemon(*identity, nlohmann::json{{"cmd", "logs"}}));
    else if (cmd == "errors") return PrintDaemonResponse(CallDaemon(*identity, nlohmann::json{{"cmd", "errors"}}));
    else if (cmd == "query" || cmd == "query-element") {
        if (positional.empty()) {
            PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev query <selector>"));
            return 1;
        }
        nlohmann::json req{{"cmd", "query_element"}, {"selector", positional[0]}};
        AddIntOptionIfPresent(args, "--limit", "limit", &req);
        return PrintDaemonResponse(CallDaemon(*identity, req));
    }
    else if (cmd == "inspect") {
        if (positional.empty()) {
            PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev inspect <selector>"));
            return 1;
        }
        return PrintDaemonResponse(CallDaemon(*identity, nlohmann::json{{"cmd", "inspect"}, {"selector", positional[0]}}));
    }
    else if (cmd == "click") {
        if (positional.empty()) {
            PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev click <selector>"));
            return 1;
        }
        return PrintDaemonResponse(CallDaemon(*identity, nlohmann::json{{"cmd", "click"}, {"selector", positional[0]}}));
    }
    else if (cmd == "input-text") {
        if (positional.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev input-text <selector> <text>"));
            return 1;
        }
        std::string text = positional[1];
        for (size_t i = 2; i < positional.size(); ++i) text += " " + positional[i];
        return PrintDaemonResponse(CallDaemon(*identity, nlohmann::json{{"cmd", "input_text"}, {"selector", positional[0]}, {"text", text}}));
    }
    else if (cmd == "scroll") {
        if (positional.empty()) {
            PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev scroll <selector> [--x <num>] [--y <num>]"));
            return 1;
        }
        nlohmann::json req{{"cmd", "scroll"}, {"selector", positional[0]}};
        const auto x = GetOptionValue(args, "--x");
        const auto y = GetOptionValue(args, "--y");
        double number = 0.0;
        if (!x.empty()) {
            if (!TryParseNumber(x, &number)) {
                PrintJson(ErrorResponse("invalid_args", "--x must be a number"));
                return 1;
            }
            req["x"] = number;
        }
        if (!y.empty()) {
            if (!TryParseNumber(y, &number)) {
                PrintJson(ErrorResponse("invalid_args", "--y must be a number"));
                return 1;
            }
            req["y"] = number;
        }
        if (!req.contains("x") && !req.contains("y")) {
            PrintJson(ErrorResponse("invalid_args", "scroll requires at least --x or --y"));
            return 1;
        }
        return PrintDaemonResponse(CallDaemon(*identity, req));
    }
    else if (cmd == "highlight") {
        if (positional.empty()) {
            PrintJson(ErrorResponse("invalid_args", "用法: mblink-ui-dev highlight <selector> [--color <css-color>]"));
            return 1;
        }
        nlohmann::json req{{"cmd", "highlight"}, {"selector", positional[0]}};
        const auto color = GetOptionValue(args, "--color");
        if (!color.empty()) req["color"] = color;
        return PrintDaemonResponse(CallDaemon(*identity, req));
    }
    else if (cmd == "eval") {
        std::string code;
        std::string error;
        if (!ResolveEvalCode(args, &code, &error)) {
            PrintJson(ErrorResponse("invalid_args", error));
            return 1;
        }
        return PrintDaemonResponse(CallDaemon(*identity, nlohmann::json{{"cmd", "eval"}, {"code", code}}));
    }
    else if (cmd == "reload") return PrintDaemonResponse(CallDaemon(*identity, nlohmann::json{{"cmd", "reload"}}));
    else {
        PrintJson(ErrorResponse("invalid_args", "未知命令"));
        return 1;
    }

    return 0;
}
