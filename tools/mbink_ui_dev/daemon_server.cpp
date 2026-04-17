#include "daemon_server.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace mbink::ui_dev {

bool IsProcessRunning(int pid) {
#ifdef _WIN32
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (!process) return false;
    const DWORD code = WaitForSingleObject(process, 0);
    CloseHandle(process);
    return code == WAIT_TIMEOUT;
#else
    (void)pid; return false;
#endif
}

#ifdef _WIN32
namespace {

std::filesystem::path GetCurrentExecutablePath() {
    char buf[MAX_PATH] = {0};
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return std::filesystem::path(std::string(buf, buf + n));
}

std::filesystem::path GetRuntimeStdoutLogPath(const DaemonState& state) {
    return GetRuntimeFilePath(state.project_id, "runtime.stdout.log");
}

std::filesystem::path GetRuntimeStderrLogPath(const DaemonState& state) {
    return GetRuntimeFilePath(state.project_id, "runtime.stderr.log");
}

std::filesystem::path GetRuntimeSnapshotPath(const DaemonState& state) {
    return GetRuntimeFilePath(state.project_id, "runtime.snapshot.json");
}

std::filesystem::path GetRuntimeCommandPath(const DaemonState& state) {
    return GetRuntimeFilePath(state.project_id, "runtime.command.json");
}

std::filesystem::path GetRuntimeResponsePath(const DaemonState& state) {
    return GetRuntimeFilePath(state.project_id, "runtime.response.json");
}

std::filesystem::path GetRuntimeConsolePath(const DaemonState& state) {
    return GetRuntimeFilePath(state.project_id, "runtime.console.json");
}

std::filesystem::path GetRuntimeErrorsPath(const DaemonState& state) {
    return GetRuntimeFilePath(state.project_id, "runtime.errors.json");
}

std::filesystem::path GetRuntimeLifecyclePath(const DaemonState& state) {
    return GetRuntimeFilePath(state.project_id, "runtime.lifecycle.json");
}

std::filesystem::path GetProjectBuildLogPath(const std::filesystem::path& project_root, const ProjectConfig& config) {
    return std::filesystem::absolute(project_root / config.out_dir / "build.log");
}

void RemoveFileIfExists(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

struct FileFingerprint {
    std::filesystem::file_time_type mtime{};
    uintmax_t size = 0;
};

struct WatchContext {
    std::atomic<bool> stop_requested{false};
    bool enabled = false;
    bool initialized = false;
    bool pending = false;
    int debounce_ms = 100;
    std::filesystem::path project_root;
    std::string entry;
    std::string src_dir;
    std::string out_dir;
    std::map<std::string, FileFingerprint> snapshot;
    std::vector<std::string> changed_paths;
    std::chrono::steady_clock::time_point last_change_at{};
    std::thread worker;
};

std::mutex g_daemon_mutex;
WatchContext g_watch;

nlohmann::json DefaultWatchState() {
    return nlohmann::json{{"enabled", false}, {"status", "idle"}, {"debounce_ms", 100}, {"changed_paths", nlohmann::json::array()}};
}

std::string ToLowerAscii(std::string value);


bool ShouldWatchFile(const std::filesystem::path& project_root,
                     const std::filesystem::path& path,
                     const WatchContext& watch) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) return false;
    const auto rel = std::filesystem::relative(path, project_root, ec).generic_string();
    if (ec || rel.empty()) return false;
    const auto lower = ToLowerAscii(rel);
    if (lower.rfind(".git/", 0) == 0 || lower.rfind("node_modules/", 0) == 0) return false;
    if (!watch.out_dir.empty()) {
        const auto out_prefix = ToLowerAscii(std::filesystem::path(watch.out_dir).generic_string());
        if (!out_prefix.empty() && (lower == out_prefix || lower.rfind(out_prefix + "/", 0) == 0)) return false;
    }
    if (lower == "mbink.config.json") return true;
    if (lower == ToLowerAscii(std::filesystem::path(watch.entry).generic_string())) return true;
    const auto ext = ToLowerAscii(path.extension().string());
    return ext == ".js" || ext == ".mjs" || ext == ".jsx" || ext == ".ts" || ext == ".tsx" || ext == ".css" || ext == ".json" || ext == ".html";
}

std::map<std::string, FileFingerprint> CaptureWatchSnapshot(const WatchContext& watch) {
    std::map<std::string, FileFingerprint> files;
    if (watch.project_root.empty() || !std::filesystem::exists(watch.project_root)) return files;
    std::error_code ec;
    for (std::filesystem::recursive_directory_iterator it(watch.project_root, ec), end; !ec && it != end; it.increment(ec)) {
        const auto current = it->path();
        const auto rel = std::filesystem::relative(current, watch.project_root, ec).generic_string();
        if (ec) break;
        if (it->is_directory(ec)) {
            const auto lower = ToLowerAscii(rel);
            const auto out_dir = ToLowerAscii(std::filesystem::path(watch.out_dir).generic_string());
            if (lower == ".git" || lower == "node_modules" || (!out_dir.empty() && lower == out_dir)) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!ShouldWatchFile(watch.project_root, current, watch)) continue;
        std::error_code info_ec;
        files[rel] = FileFingerprint{std::filesystem::last_write_time(current, info_ec), std::filesystem::file_size(current, info_ec)};
    }
    return files;
}

std::vector<std::string> CollectChangedPaths(const std::map<std::string, FileFingerprint>& before,
                                             const std::map<std::string, FileFingerprint>& after) {
    std::vector<std::string> changed;
    auto push = [&changed](const std::string& value) {
        if (std::find(changed.begin(), changed.end(), value) == changed.end()) changed.push_back(value);
    };
    for (const auto& [path, now] : after) {
        const auto it = before.find(path);
        if (it == before.end() || it->second.size != now.size || it->second.mtime != now.mtime) push(path);
    }
    for (const auto& [path, _] : before) {
        if (after.find(path) == after.end()) push(path);
    }
    if (changed.size() > 20) changed.resize(20);
    return changed;
}

void ResetWatchUnlocked() {
    g_watch.enabled = false;
    g_watch.initialized = false;
    g_watch.pending = false;
    g_watch.project_root.clear();
    g_watch.entry.clear();
    g_watch.src_dir.clear();
    g_watch.out_dir.clear();
    g_watch.snapshot.clear();
    g_watch.changed_paths.clear();
}

void ConfigureWatchUnlocked(const DaemonState& state, bool enabled) {
    ResetWatchUnlocked();
    g_watch.enabled = enabled;
    g_watch.debounce_ms = 100;
    g_watch.project_root = state.project_root;
    g_watch.entry = state.project.entry;
    g_watch.src_dir = state.project.src_dir;
    g_watch.out_dir = state.project.out_dir;
}

std::string TrimAscii(std::string value) {
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }
    size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t')) ++start;
    return value.substr(start);
}

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string SanitizeJsonText(const std::string& value) {
    std::string sanitized;
    sanitized.reserve(value.size());
    for (unsigned char ch : value) {
        if (ch == '\t' || ch == '\r' || ch == '\n' || (ch >= 0x20 && ch < 0x80)) {
            sanitized.push_back(static_cast<char>(ch));
        } else {
            sanitized.push_back('?');
        }
    }
    return sanitized;
}


bool ReadPipeMessage(HANDLE pipe, std::string* out, std::string* error) {
    std::vector<char> buffer(4096);
    out->clear();
    for (;;) {
        DWORD read = 0;
        const BOOL ok = ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr);
        if (read > 0) out->append(buffer.data(), buffer.data() + read);
        if (ok) return true;
        if (GetLastError() != ERROR_MORE_DATA) {
            if (error) *error = "读取 daemon 请求失败";
            return false;
        }
    }
}

bool WriteJsonFile(const std::filesystem::path& path, const nlohmann::json& value) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs << value.dump(2);
    return ofs.good();
}

nlohmann::json ReadJsonFile(const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return nlohmann::json();
    try {
        return nlohmann::json::parse(ifs);
    } catch (...) {
        return nlohmann::json();
    }
}

void RemoveRuntimeArtifacts(const DaemonState& state) {
    RemoveFileIfExists(GetRuntimeSnapshotPath(state));
    RemoveFileIfExists(GetRuntimeCommandPath(state));
    RemoveFileIfExists(GetRuntimeResponsePath(state));
    RemoveFileIfExists(GetRuntimeConsolePath(state));
    RemoveFileIfExists(GetRuntimeErrorsPath(state));
    RemoveFileIfExists(GetRuntimeLifecyclePath(state));
    RemoveFileIfExists(GetRuntimeStdoutLogPath(state));
    RemoveFileIfExists(GetRuntimeStderrLogPath(state));
}

void RefreshRuntimeStateUnlocked(DaemonState* state) {
    if (!state) return;
    const bool process_running = state->runtime_pid > 0 && IsProcessRunning(state->runtime_pid);
    auto lifecycle = ReadJsonFile(GetRuntimeLifecyclePath(*state));
    if (lifecycle.is_object()) {
        const auto status = lifecycle.value("status", std::string{});
        const auto reason = lifecycle.value("reason", std::string{});
        if (!status.empty()) state->runtime_status = status;
        if (!reason.empty()) state->runtime_stop_reason = reason;
    }
    if (process_running) {
        state->runtime_status = "running";
        if (state->runtime_stop_reason.empty() || state->runtime_stop_reason == "unknown") {
            state->runtime_stop_reason = "running";
        }
        return;
    }
    if (state->runtime_pid > 0) state->runtime_pid = 0;
    if (state->runtime_status.empty() || state->runtime_status == "running" || state->runtime_status == "starting") {
        state->runtime_status = "stopped";
    }
    if (state->runtime_stop_reason.empty() || state->runtime_stop_reason == "running") {
        state->runtime_stop_reason = lifecycle.is_object()
            ? lifecycle.value("reason", std::string("process_exited"))
            : std::string("process_exited");
    }
}

bool RuntimeWasUserClosed(const DaemonState& state) {
    return state.runtime_status == "stopped" && state.runtime_stop_reason == "user_closed";
}

bool IsPathInsideRoot(const std::filesystem::path& root, const std::filesystem::path& target) {
    const auto normalized_root = std::filesystem::absolute(root).lexically_normal();
    const auto normalized_target = std::filesystem::absolute(target).lexically_normal();
    auto root_it = normalized_root.begin();
    auto target_it = normalized_target.begin();
    for (; root_it != normalized_root.end() && target_it != normalized_target.end(); ++root_it, ++target_it) {
        if (*root_it != *target_it) return false;
    }
    return root_it == normalized_root.end();
}

bool ResolveProjectPath(const std::filesystem::path& project_root,
                        const std::string& relative_path,
                        std::filesystem::path* resolved,
                        std::string* error) {
    if (relative_path.empty()) {
        if (error) *error = "缺少 path";
        return false;
    }
    const std::filesystem::path input_path(relative_path);
    if (input_path.is_absolute()) {
        if (error) *error = "path 必须是项目根目录下的相对路径";
        return false;
    }
    const auto candidate = std::filesystem::absolute(project_root / input_path).lexically_normal();
    if (!IsPathInsideRoot(project_root, candidate)) {
        if (error) *error = "path 超出项目根目录";
        return false;
    }
    if (resolved) *resolved = candidate;
    return true;
}

bool ReadFileBytes(const std::filesystem::path& path, std::string* content, std::string* error) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        if (error) *error = "读取文件失败";
        return false;
    }
    std::string data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (!ifs.good() && !ifs.eof()) {
        if (error) *error = "读取文件内容失败";
        return false;
    }
    if (content) *content = std::move(data);
    return true;
}

bool WriteFileBytes(const std::filesystem::path& path, const std::string& content, std::string* error) {
    std::error_code ec;
    const auto parent = path.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent, ec);
    if (ec) {
        if (error) *error = "创建目录失败";
        return false;
    }
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        if (error) *error = "打开目标文件失败";
        return false;
    }
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!ofs.good()) {
        if (error) *error = "写入文件失败";
        return false;
    }
    return true;
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
    auto DecodeChar = [](unsigned char ch) -> int {
        if (ch >= 'A' && ch <= 'Z') return ch - 'A';
        if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
        if (ch >= '0' && ch <= '9') return ch - '0' + 52;
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        if (ch == '=') return -2;
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') return -3;
        return -1;
    };
    std::string normalized;
    normalized.reserve(input.size());
    for (unsigned char ch : input) {
        const int v = DecodeChar(ch);
        if (v == -3) continue;
        if (v == -1) {
            if (error) *error = "base64 内容非法";
            return false;
        }
        normalized.push_back(static_cast<char>(ch));
    }
    if (normalized.size() % 4 != 0) {
        if (error) *error = "base64 长度非法";
        return false;
    }
    std::string decoded;
    decoded.reserve((normalized.size() / 4) * 3);
    for (size_t i = 0; i < normalized.size(); i += 4) {
        const int c0 = DecodeChar(static_cast<unsigned char>(normalized[i]));
        const int c1 = DecodeChar(static_cast<unsigned char>(normalized[i + 1]));
        const int c2 = DecodeChar(static_cast<unsigned char>(normalized[i + 2]));
        const int c3 = DecodeChar(static_cast<unsigned char>(normalized[i + 3]));
        if (c0 < 0 || c1 < 0 || c2 == -1 || c3 == -1) {
            if (error) *error = "base64 内容非法";
            return false;
        }
        decoded.push_back(static_cast<char>((c0 << 2) | (c1 >> 4)));
        if (c2 != -2) {
            decoded.push_back(static_cast<char>(((c1 & 0x0F) << 4) | (c2 >> 2)));
            if (c3 != -2) {
                decoded.push_back(static_cast<char>(((c2 & 0x03) << 6) | c3));
            }
        } else if (c3 != -2) {
            if (error) *error = "base64 padding 非法";
            return false;
        }
    }
    if (output) *output = std::move(decoded);
    return true;
}

nlohmann::json BuildFileTreeNode(const std::filesystem::path& node_path,
                                 const std::filesystem::path& project_root,
                                 int remaining_depth) {
    std::error_code ec;
    const auto relative = std::filesystem::relative(node_path, project_root, ec);
    const std::string relative_path = ec ? node_path.filename().generic_string() : relative.generic_string();
    if (std::filesystem::is_directory(node_path, ec)) {
        nlohmann::json node{{"path", relative_path}, {"type", "dir"}, {"children", nlohmann::json::array()}};
        if (remaining_depth > 0) {
            std::vector<std::filesystem::directory_entry> entries;
            for (std::filesystem::directory_iterator it(node_path, ec); !ec && it != std::filesystem::directory_iterator(); ++it) {
                entries.push_back(*it);
            }
            std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
                const bool a_dir = a.is_directory();
                const bool b_dir = b.is_directory();
                if (a_dir != b_dir) return a_dir > b_dir;
                return a.path().filename().generic_string() < b.path().filename().generic_string();
            });
            for (const auto& entry : entries) {
                node["children"].push_back(BuildFileTreeNode(entry.path(), project_root, remaining_depth - 1));
            }
        }
        return node;
    }
    uintmax_t size = 0;
    if (std::filesystem::is_regular_file(node_path, ec)) size = std::filesystem::file_size(node_path, ec);
    return nlohmann::json{{"path", relative_path}, {"type", "file"}, {"size", size}};
}

nlohmann::json BuildProjectFileTree(const std::filesystem::path& project_root, int depth) {
    nlohmann::json tree = nlohmann::json::array();
    std::error_code ec;
    std::vector<std::filesystem::directory_entry> entries;
    for (std::filesystem::directory_iterator it(project_root, ec); !ec && it != std::filesystem::directory_iterator(); ++it) {
        entries.push_back(*it);
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        const bool a_dir = a.is_directory();
        const bool b_dir = b.is_directory();
        if (a_dir != b_dir) return a_dir > b_dir;
        return a.path().filename().generic_string() < b.path().filename().generic_string();
    });
    const int child_depth = depth > 0 ? depth - 1 : 0;
    for (const auto& entry : entries) {
        tree.push_back(BuildFileTreeNode(entry.path(), project_root, child_depth));
    }
    return tree;
}


bool StartRuntime(const std::filesystem::path& root, DaemonState* state, std::string* error);


bool RequestRuntimeCommand(const DaemonState& state,
                           int runtime_pid,
                           nlohmann::json command,
                           nlohmann::json* response,
                           std::string* error) {
    const auto command_path = GetRuntimeCommandPath(state);
    const auto response_path = GetRuntimeResponsePath(state);
    const std::string type = command.value("type", "command");
    const auto request_id = type + "-" + std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    command["id"] = request_id;
    RemoveFileIfExists(command_path);
    RemoveFileIfExists(response_path);

    if (!WriteJsonFile(command_path, command)) {
        if (error) *error = "写入 runtime 请求失败";
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!IsProcessRunning(runtime_pid)) {
            if (error) *error = "runtime 已退出，请求中断";
            return false;
        }

        auto resp = ReadJsonFile(response_path);
        if (resp.is_object() && resp.value("id", std::string()) == request_id) {
            if (response) *response = resp;
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (error) *error = "等待 runtime 响应超时";
    return false;
}

bool RequestRuntimeEval(const DaemonState& state,
                        int runtime_pid,
                        const std::string& code,
                        nlohmann::json* response,
                        std::string* error) {
    return RequestRuntimeCommand(state,
                                 runtime_pid,
                                 nlohmann::json{{"type", "eval"}, {"code", code}},
                                 response,
                                 error);
}

bool RequestRuntimeReloadBundle(const DaemonState& state,
                                int runtime_pid,
                                const std::filesystem::path& bundle_path,
                                nlohmann::json* response,
                                std::string* error) {
    return RequestRuntimeCommand(state,
                                 runtime_pid,
                                 nlohmann::json{{"type", "reload_bundle"}, {"bundle_path", bundle_path.string()}},
                                 response,
                                 error);
}

bool RequestRuntimeUiCommand(const DaemonState& state,
                             int runtime_pid,
                             nlohmann::json command,
                             nlohmann::json* response,
                             std::string* error) {
    return RequestRuntimeCommand(state, runtime_pid, std::move(command), response, error);
}

std::string ExtractRuntimeCommandMessage(const nlohmann::json& response, const std::string& fallback) {
    if (response.is_object()) {
        if (response.contains("error") && response["error"].is_object()) {
            const auto message = response["error"].value("message", "");
            if (!message.empty()) return message;
        }
        const auto message = response.value("message", "");
        if (!message.empty()) return message;
    }
    return fallback;
}

std::filesystem::path ResolveReloadBundlePath(const std::filesystem::path& root, const ProjectConfig& project) {
    const auto built_bundle = std::filesystem::absolute(root / project.out_dir / "App.js").lexically_normal();
    std::error_code ec;
    if (std::filesystem::exists(built_bundle, ec) && std::filesystem::is_regular_file(built_bundle, ec)) return built_bundle;
    return std::filesystem::absolute(root / project.entry).lexically_normal();
}

nlohmann::json ReloadRuntimeBundleOrRestart(const std::filesystem::path& root, DaemonState* state) {
    RefreshRuntimeStateUnlocked(state);
    const auto bundle_path = ResolveReloadBundlePath(root, state->project);
    nlohmann::json runtime_response;
    std::string reload_error;

    if (state->runtime_pid > 0 && IsProcessRunning(state->runtime_pid)) {
        if (RequestRuntimeReloadBundle(*state, state->runtime_pid, bundle_path, &runtime_response, &reload_error) &&
            runtime_response.value("ok", false)) {
            state->runtime_status = "running";
            state->runtime_stop_reason = "running";
            return nlohmann::json{{"ok", true},
                                  {"mode", "reload_bundle"},
                                  {"fallback", false},
                                  {"bundle_path", bundle_path.string()},
                                  {"response", runtime_response}};
        }
        reload_error = ExtractRuntimeCommandMessage(runtime_response,
                                                    reload_error.empty() ? "runtime 内 reload_bundle 失败" : reload_error);
    } else {
        reload_error = "runtime 未运行，改为 restart_runtime";
    }

    if (RuntimeWasUserClosed(*state)) {
        return nlohmann::json{{"ok", false},
                              {"mode", "user_closed"},
                              {"fallback", false},
                              {"bundle_path", bundle_path.string()},
                              {"message", "runtime 已被用户手动关闭，当前不会自动拉起"},
                              {"response", runtime_response}};
    }

    std::string restart_error;
    if (StartRuntime(root, state, &restart_error)) {
        return nlohmann::json{{"ok", true},
                              {"mode", "restart_runtime"},
                              {"fallback", true},
                              {"bundle_path", bundle_path.string()},
                              {"message", reload_error.empty() ? "in-place reload 失败，已回退 restart_runtime" : reload_error},
                              {"response", runtime_response}};
    }

    return nlohmann::json{{"ok", false},
                          {"mode", "restart_runtime"},
                          {"fallback", true},
                          {"bundle_path", bundle_path.string()},
                          {"message", restart_error},
                          {"previous_error", reload_error},
                          {"response", runtime_response}};
}

nlohmann::json BuildStructuredLogTail(const std::filesystem::path& path,
                                    const std::string& stream,
                                    size_t max_lines,
                                    const char* array_field_name) {
    auto ToLowerAscii = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    };
    auto TrimAscii = [](std::string value) {
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }
        size_t start = 0;
        while (start < value.size() && (value[start] == ' ' || value[start] == '\t')) ++start;
        return value.substr(start);
    };
    auto InferLevel = [&](const std::string& message) {
        const std::string lower = ToLowerAscii(message);
        if (lower.find("fatal") != std::string::npos || lower.find("exception") != std::string::npos ||
            lower.find(" error") != std::string::npos || lower.rfind("error", 0) == 0 ||
            lower.find("failed") != std::string::npos || lower.find("✗") != std::string::npos ||
            stream == "stderr") {
            return std::string("error");
        }
        if (lower.find("warn") != std::string::npos) return std::string("warning");
        if (lower.find("debug") != std::string::npos || lower.find("trace") != std::string::npos) {
            return std::string("debug");
        }
        return std::string("info");
    };

    std::ifstream ifs(path, std::ios::binary);
    nlohmann::json result{{"ok", true},
                          {"stream", stream},
                          {"source", path.string()},
                          {"timestamp", CurrentTimestampIso8601()},
                          {"count", 0},
                          {"truncated", false},
                          {array_field_name, nlohmann::json::array()}};
    if (!ifs) return result;

    std::deque<std::pair<size_t, std::string>> lines;
    std::string line;
    size_t line_no = 0;
    while (std::getline(ifs, line)) {
        ++line_no;
        const std::string trimmed = TrimAscii(line);
        if (trimmed.empty()) continue;
        if (lines.size() >= max_lines) lines.pop_front();
        lines.emplace_back(line_no, trimmed);
    }

    auto& entries = result[array_field_name];
    result["count"] = lines.size();
    result["truncated"] = line_no > lines.size();
    result["line_count"] = line_no;
    for (const auto& item : lines) {
        entries.push_back({{"line", item.first},
                           {"stream", stream},
                           {"level", InferLevel(item.second)},
                           {"message", item.second}});
    }
    return result;
}

nlohmann::json BuildStructuredRuntimeBufferResponse(const std::filesystem::path& path,
                                                    const std::string& stream,
                                                    size_t max_entries,
                                                    const char* array_field_name) {
    nlohmann::json result{{"ok", true},
                          {"stream", stream},
                          {"source", path.string()},
                          {"timestamp", CurrentTimestampIso8601()},
                          {"count", 0},
                          {"truncated", false},
                          {array_field_name, nlohmann::json::array()}};
    auto payload = ReadJsonFile(path);
    if (!payload.is_object() || !payload.contains(array_field_name) || !payload[array_field_name].is_array()) {
        return result;
    }

    const auto& source_entries = payload[array_field_name];
    const size_t total = source_entries.size();
    const size_t start = total > max_entries ? (total - max_entries) : 0;
    auto& entries = result[array_field_name];
    for (size_t i = start; i < total; ++i) {
        entries.push_back(source_entries[i]);
    }
    result["count"] = entries.size();
    result["line_count"] = total;
    result["truncated"] = total > entries.size();
    return result;
}

std::filesystem::path FindEsbuildExecutable(const std::filesystem::path& project_root) {
    const char* path_env = std::getenv("PATH");
    std::vector<std::filesystem::path> candidates = {
        project_root / "node_modules/.bin/esbuild.cmd",
        project_root / "node_modules/.bin/esbuild",
        project_root / "node_modules/@esbuild/win32-x64/esbuild.exe",
        "esbuild.cmd",
        "esbuild.exe",
        "esbuild"
    };
    if (path_env && *path_env) {
        std::stringstream ss(path_env);
        std::string segment;
        while (std::getline(ss, segment, ';')) {
            segment = TrimAscii(segment);
            if (segment.empty()) continue;
            candidates.push_back(std::filesystem::path(segment) / "esbuild.cmd");
            candidates.push_back(std::filesystem::path(segment) / "esbuild.exe");
            candidates.push_back(std::filesystem::path(segment) / "esbuild");
        }
    }
    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !std::filesystem::is_directory(candidate, ec)) return candidate;
    }
    return {};
}

std::filesystem::path DetectBuildEntryPoint(const std::filesystem::path& project_root, const ProjectConfig& config) {
    const std::vector<std::filesystem::path> preferred_candidates = {
        project_root / config.src_dir / "App.jsx",
        project_root / config.src_dir / "app.jsx",
        project_root / config.src_dir / "main.jsx",
        project_root / config.src_dir / "index.jsx",
        project_root / config.src_dir / "App.tsx",
        project_root / config.src_dir / "main.tsx",
        project_root / config.src_dir / "index.tsx"
    };
    if (config.template_name == "preact-jsx" || config.template_name == "preact-ts") {
        for (const auto& candidate : preferred_candidates) {
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
                return std::filesystem::absolute(candidate).lexically_normal();
            }
        }
    }

    const auto entry_path = std::filesystem::absolute(project_root / config.entry).lexically_normal();
    if (entry_path.extension() == ".jsx" || entry_path.extension() == ".tsx") return entry_path;
    if (entry_path.extension() == ".js" || entry_path.extension() == ".mjs" || entry_path.extension() == ".ts") {
        for (const auto& candidate : preferred_candidates) {
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
                return std::filesystem::absolute(candidate).lexically_normal();
            }
        }
        return entry_path;
    }
    const std::vector<std::filesystem::path> candidates = {
        project_root / config.src_dir / "App.jsx",
        project_root / config.src_dir / "app.jsx",
        project_root / config.src_dir / "main.jsx",
        project_root / config.src_dir / "index.jsx",
        project_root / config.src_dir / "App.tsx",
        project_root / config.src_dir / "main.tsx",
        project_root / config.src_dir / "index.tsx",
        project_root / config.src_dir / "app.js",
        project_root / config.src_dir / "main.js",
        project_root / config.src_dir / "index.js"
    };
    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
            return std::filesystem::absolute(candidate).lexically_normal();
        }
    }
    return {};
}

std::string QuoteForCmd(const std::string& value) {
    return std::string{"\""} + value + "\"";
}

bool IsCmdScript(const std::filesystem::path& path) {
    const auto ext = ToLowerAscii(path.extension().string());
    return ext == ".cmd" || ext == ".bat";
}

nlohmann::json BuildEsbuildStatus(const std::filesystem::path& project_root, const ProjectConfig& config, std::string* error) {
    const auto started_at = std::chrono::steady_clock::now();
    const auto started_iso = CurrentTimestampIso8601();
    const auto esbuild = FindEsbuildExecutable(project_root);
    if (esbuild.empty()) {
        if (error) *error = "未找到 esbuild，可在 PATH 或项目 node_modules 中安装";
        return {};
    }
    const auto entry_point = DetectBuildEntryPoint(project_root, config);
    if (entry_point.empty()) {
        if (error) *error = "未找到可构建入口，请检查 entry/src_dir 配置";
        return {};
    }
    const auto out_dir = std::filesystem::absolute(project_root / config.out_dir).lexically_normal();
    std::error_code ec;
    std::filesystem::create_directories(out_dir, ec);
    if (ec) {
        if (error) *error = "创建 out_dir 失败";
        return {};
    }
    const auto output_file = out_dir / "App.js";
    std::string esbuild_args = QuoteForCmd(entry_point.string()) +
                               " --bundle --format=esm --outfile=" + QuoteForCmd(output_file.string()) +
                               " --jsx=transform --jsx-factory=" + QuoteForCmd(config.build_jsx_factory) +
                               " --jsx-fragment=" + QuoteForCmd(config.build_jsx_fragment) +
                               (config.build_sourcemap ? " --sourcemap=inline" : " --sourcemap=false") +
                               (config.build_minify ? " --minify" : "") +
                               " --color=false --log-level=info --log-limit=0";
    for (const auto& ext : config.build_external) {
        esbuild_args += " --external:" + QuoteForCmd(ext);
    }
    std::string command;
    if (IsCmdScript(esbuild)) {
        command = "cmd.exe /d /c call " + QuoteForCmd(esbuild.string()) + " " + esbuild_args;
    } else {
        command = QuoteForCmd(esbuild.string()) + " " + esbuild_args;
    }

    SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE read_pipe = INVALID_HANDLE_VALUE;
    HANDLE write_pipe = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        if (error) *error = "创建构建输出管道失败";
        return {};
    }
    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = write_pipe;
    si.hStdError = write_pipe;

    std::vector<char> mutable_cmd(command.begin(), command.end());
    mutable_cmd.push_back('\0');
    const std::string working_dir = project_root.string();
    const BOOL created = CreateProcessA(nullptr, mutable_cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                        nullptr, working_dir.c_str(), &si, &pi);
    CloseHandle(write_pipe);
    if (!created) {
        CloseHandle(read_pipe);
        if (error) *error = "启动 esbuild 失败";
        return {};
    }

    std::string raw_output;
    char buffer[4096];
    for (;;) {
        DWORD read = 0;
        const BOOL ok = ReadFile(read_pipe, buffer, sizeof(buffer), &read, nullptr);
        if (read > 0) raw_output.append(buffer, buffer + read);
        if (!ok || read == 0) break;
    }
    CloseHandle(read_pipe);

    const DWORD wait_result = WaitForSingleObject(pi.hProcess, 30000);
    DWORD exit_code = 1;
    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        raw_output += "\nbuild timed out after 30000ms";
    }
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    const auto finished_iso = CurrentTimestampIso8601();
    const auto duration_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started_at).count());
    const auto build_log_path = GetProjectBuildLogPath(project_root, config);

    std::vector<std::string> lines;
    std::stringstream ss(raw_output);
    std::string line;
    while (std::getline(ss, line)) {
        line = TrimAscii(line);
        line = SanitizeJsonText(line);
        if (!line.empty()) lines.push_back(line);
    }

    nlohmann::json warnings = nlohmann::json::array();
    nlohmann::json errors = nlohmann::json::array();
    for (const auto& item : lines) {
        const auto lower = ToLowerAscii(item);
        if (lower.find("warning") != std::string::npos) warnings.push_back({{"message", item}});
        if (lower.find("error") != std::string::npos) errors.push_back({{"message", item}});
    }

    nlohmann::json status{{"ok", exit_code == 0},
                          {"status", exit_code == 0 ? "success" : "failed"},
                          {"builder", "esbuild"},
                          {"started_at", started_iso},
                          {"finished_at", finished_iso},
                          {"duration_ms", duration_ms},
                          {"entry_point", std::filesystem::relative(entry_point, project_root).generic_string()},
                          {"out_dir", config.out_dir},
                          {"outputs", nlohmann::json::array({std::filesystem::relative(output_file, project_root).generic_string()})},
                          {"warnings", warnings},
                          {"errors", errors},
                          {"raw_output", lines},
                          {"build_log", build_log_path.string()}};
    WriteJsonFile(build_log_path, status);
    return status;
}

bool StartDetachedProcess(const std::filesystem::path& exe, const std::string& args,
                          const std::filesystem::path& cwd,
                          const std::filesystem::path* stdout_path,
                          const std::filesystem::path* stderr_path,
                          int* pid, std::string* error) {
    std::string cmd = "\"" + exe.string() + "\"" + (args.empty() ? "" : " " + args);
    const bool inherit_handles = stdout_path != nullptr || stderr_path != nullptr;
    SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE out_handle = INVALID_HANDLE_VALUE;
    HANDLE err_handle = INVALID_HANDLE_VALUE;
    HANDLE in_handle = INVALID_HANDLE_VALUE;
    STARTUPINFOA si{}; PROCESS_INFORMATION pi{}; si.cb = sizeof(si);
    if (stdout_path) {
        out_handle = CreateFileA(stdout_path->string().c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (out_handle == INVALID_HANDLE_VALUE) {
            if (error) *error = "创建 stdout 日志失败";
            return false;
        }
    }
    if (stderr_path) {
        err_handle = CreateFileA(stderr_path->string().c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (err_handle == INVALID_HANDLE_VALUE) {
            if (out_handle != INVALID_HANDLE_VALUE) CloseHandle(out_handle);
            if (error) *error = "创建 stderr 日志失败";
            return false;
        }
    }
    if (inherit_handles) {
        in_handle = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (in_handle == INVALID_HANDLE_VALUE) {
            if (out_handle != INVALID_HANDLE_VALUE) CloseHandle(out_handle);
            if (err_handle != INVALID_HANDLE_VALUE) CloseHandle(err_handle);
            if (error) *error = "创建 stdin 重定向失败";
            return false;
        }
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = out_handle != INVALID_HANDLE_VALUE ? out_handle : GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = err_handle != INVALID_HANDLE_VALUE ? err_handle : GetStdHandle(STD_ERROR_HANDLE);
        si.hStdInput = in_handle;
    }
    std::vector<char> mutable_cmd(cmd.begin(), cmd.end()); mutable_cmd.push_back('\0');
    if (!CreateProcessA(nullptr, mutable_cmd.data(), nullptr, nullptr, inherit_handles,
                        DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                        nullptr, cwd.string().c_str(), &si, &pi)) {
        if (out_handle != INVALID_HANDLE_VALUE) CloseHandle(out_handle);
        if (err_handle != INVALID_HANDLE_VALUE) CloseHandle(err_handle);
        if (in_handle != INVALID_HANDLE_VALUE) CloseHandle(in_handle);
        if (error) *error = "启动进程失败: " + exe.string();
        return false;
    }
    if (out_handle != INVALID_HANDLE_VALUE) CloseHandle(out_handle);
    if (err_handle != INVALID_HANDLE_VALUE) CloseHandle(err_handle);
    if (in_handle != INVALID_HANDLE_VALUE) CloseHandle(in_handle);
    if (pid) *pid = static_cast<int>(pi.dwProcessId);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); return true;
}

void StopProcess(int pid) {
    HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (!process) return;
    TerminateProcess(process, 0);
    WaitForSingleObject(process, 2000);
    CloseHandle(process);
}

bool StartRuntime(const std::filesystem::path& root, DaemonState* state, std::string* error) {
    if (state->runtime_pid > 0 && IsProcessRunning(state->runtime_pid)) StopProcess(state->runtime_pid);
    const auto runtime_exe = GetCurrentExecutablePath().parent_path() / "esm_loader.exe";
    if (!std::filesystem::exists(runtime_exe)) {
        if (error) *error = "未找到 esm_loader.exe，请先编译 esm_loader";
        return false;
    }
    std::filesystem::path entry = std::filesystem::absolute(root / state->project.entry);
    if (state->project.template_name == "preact-ts") {
        std::string build_error;
        auto build_status = BuildEsbuildStatus(root, state->project, &build_error);
        if (!build_status.is_object()) {
            build_status = nlohmann::json{{"ok", false},
                                          {"status", "failed"},
                                          {"errors", nlohmann::json::array({{{"message", build_error.empty() ? "preact-ts 运行前构建失败" : build_error}}})},
                                          {"warnings", nlohmann::json::array()},
                                          {"raw_output", nlohmann::json::array()}};
        }
        state->last_build = build_status;
        if (!build_status.value("ok", false)) {
            state->runtime_pid = 0;
            state->runtime_status = "stopped";
            state->runtime_stop_reason = "build_failed";
            if (error) *error = build_error.empty() ? "preact-ts 运行前构建失败" : build_error;
            return false;
        }
        entry = std::filesystem::absolute(root / state->project.out_dir / "App.js");
    }
    if (!std::filesystem::exists(entry)) {
        state->runtime_pid = 0;
        state->runtime_status = "stopped";
        state->runtime_stop_reason = "entry_missing";
        if (error) *error = "入口文件不存在: " + entry.string();
        return false;
    }
    const auto stdout_path = GetRuntimeStdoutLogPath(*state);
    const auto stderr_path = GetRuntimeStderrLogPath(*state);
    const auto snapshot_path = GetRuntimeSnapshotPath(*state);
    const auto command_path = GetRuntimeCommandPath(*state);
    const auto response_path = GetRuntimeResponsePath(*state);
    const auto console_path = GetRuntimeConsolePath(*state);
    const auto errors_path = GetRuntimeErrorsPath(*state);
    const auto lifecycle_path = GetRuntimeLifecyclePath(*state);

    RemoveRuntimeArtifacts(*state);
    state->runtime_status = "starting";
    state->runtime_stop_reason = "starting";

    std::string args = "\"" + entry.string() + "\" --width " + std::to_string(state->project.width) +
                       " --height " + std::to_string(state->project.height) +
                       " --title \"" + state->project.title + "\"" +
                       " --ui-dev-snapshot-file \"" + snapshot_path.string() + "\"" +
                       " --ui-dev-command-file \"" + command_path.string() + "\"" +
                       " --ui-dev-response-file \"" + response_path.string() + "\"" +
                       " --ui-dev-console-file \"" + console_path.string() + "\"" +
                       " --ui-dev-errors-file \"" + errors_path.string() + "\"" +
                       " --ui-dev-lifecycle-file \"" + lifecycle_path.string() + "\"";
    if (!StartDetachedProcess(runtime_exe, args, root, &stdout_path, &stderr_path, &state->runtime_pid, error)) {
        state->runtime_pid = 0;
        state->runtime_status = "stopped";
        state->runtime_stop_reason = "spawn_failed";
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (!IsProcessRunning(state->runtime_pid)) {
        if (error) *error = "esm_loader 进程启动后立即退出";
        state->runtime_pid = 0;
        RefreshRuntimeStateUnlocked(state);
        return false;
    }
    state->runtime_status = "running";
    state->runtime_stop_reason = "running";
    return true;
}


void UpdateWatchStateUnlocked(DaemonState* state,
                              const std::string& status,
                              const std::vector<std::string>& changed_paths,
                              const nlohmann::json& last_event = nullptr) {
    auto watch = state->watch.is_object() ? state->watch : DefaultWatchState();
    watch["enabled"] = g_watch.enabled;
    watch["status"] = status;
    watch["debounce_ms"] = g_watch.debounce_ms;
    watch["changed_paths"] = changed_paths;
    if (!last_event.is_null()) watch["last_event"] = last_event;
    state->watch = watch;
}

void WatchLoop(DaemonState* state) {
    while (!g_watch.stop_requested.load()) {
        bool should_build = false;
        std::filesystem::path root;
        ProjectConfig project;
        std::vector<std::string> changed_paths;
        {
            std::lock_guard<std::mutex> lock(g_daemon_mutex);
            RefreshRuntimeStateUnlocked(state);
            if (g_watch.enabled && !g_watch.project_root.empty() && std::filesystem::exists(g_watch.project_root)) {
                if (!g_watch.initialized) {
                    g_watch.snapshot = CaptureWatchSnapshot(g_watch);
                    g_watch.initialized = true;
                    UpdateWatchStateUnlocked(state, "watching", {});
                    std::string save_error;
                    SaveState(*state, &save_error);
                } else {
                    auto next_snapshot = CaptureWatchSnapshot(g_watch);
                    auto changed = CollectChangedPaths(g_watch.snapshot, next_snapshot);
                    g_watch.snapshot = std::move(next_snapshot);
                    if (!changed.empty()) {
                        g_watch.pending = true;
                        g_watch.last_change_at = std::chrono::steady_clock::now();
                        g_watch.changed_paths = changed;
                        UpdateWatchStateUnlocked(state, "debouncing", g_watch.changed_paths, nlohmann::json{{"type", "change_detected"}, {"changed_paths", g_watch.changed_paths}, {"at", CurrentTimestampIso8601()}});
                        std::string save_error;
                        SaveState(*state, &save_error);
                    }
                }
                if (g_watch.pending) {
                    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_watch.last_change_at).count();
                    if (elapsed >= g_watch.debounce_ms) {
                        g_watch.pending = false;
                        root = g_watch.project_root;
                        project = state->project;
                        changed_paths = g_watch.changed_paths;
                        UpdateWatchStateUnlocked(state, "building", changed_paths, nlohmann::json{{"type", "build_started"}, {"changed_paths", changed_paths}, {"at", CurrentTimestampIso8601()}});
                        std::string save_error;
                        SaveState(*state, &save_error);
                        should_build = true;
                    }
                }
            }
        }
        if (should_build) {
            std::string build_error;
            auto build_status = BuildEsbuildStatus(root, project, &build_error);
            if (!build_status.is_object()) {
                build_status = nlohmann::json{{"ok", false}, {"status", "failed"}, {"errors", nlohmann::json::array({{{"message", build_error.empty() ? "watch build failed" : build_error}}})}, {"warnings", nlohmann::json::array()}, {"raw_output", nlohmann::json::array()}};
            }
            build_status["trigger"] = "watch";
            build_status["changed_paths"] = changed_paths;
            std::lock_guard<std::mutex> lock(g_daemon_mutex);
            RefreshRuntimeStateUnlocked(state);
            if (build_status.value("ok", false) && !RuntimeWasUserClosed(*state)) {
                build_status["reload"] = ReloadRuntimeBundleOrRestart(root, state);
            } else if (RuntimeWasUserClosed(*state)) {
                build_status["reload"] = nlohmann::json{{"ok", false}, {"mode", "user_closed"}, {"fallback", false}, {"message", "runtime 已被用户手动关闭，watch 不会自动拉起"}};
            }
            state->last_build = build_status;
            UpdateWatchStateUnlocked(state, g_watch.enabled ? "watching" : "idle", {}, nlohmann::json{{"type", "build_finished"}, {"ok", build_status.value("ok", false)}, {"changed_paths", changed_paths}, {"at", CurrentTimestampIso8601()}});
            std::string save_error;
            SaveState(*state, &save_error);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

void EnsureWatchThreadStarted(DaemonState* state) {
    if (g_watch.worker.joinable()) return;
    g_watch.stop_requested = false;
    g_watch.worker = std::thread([state]() { WatchLoop(state); });
}

void StopWatchThread() {
    g_watch.stop_requested = true;
    if (g_watch.worker.joinable()) g_watch.worker.join();
    std::lock_guard<std::mutex> lock(g_daemon_mutex);
    ResetWatchUnlocked();
}

}  // namespace
#endif

bool StartDetachedDaemon(const std::filesystem::path& executable_path,
                         const std::string& project_root,
                         const std::string& project_id,
                         int* spawned_pid,
                         std::string* error) {
#ifdef _WIN32
    std::string args = "daemon run";
    if (!project_root.empty()) args += " --project \"" + project_root + "\"";
    if (!project_id.empty()) args += " --project-id \"" + project_id + "\"";
    return StartDetachedProcess(executable_path, args, executable_path.parent_path(), nullptr, nullptr, spawned_pid, error);
#else
    (void)executable_path; (void)project_root; (void)project_id; (void)spawned_pid;
    if (error) *error = "当前平台未实现 daemon";
    return false;
#endif
}

nlohmann::json DispatchDaemonRequest(const nlohmann::json& request, DaemonState* state, bool* should_exit) {
    const auto cmd = request.value("cmd", "");
    if (cmd == "ping") return OkResponse();
    if (cmd == "stop") {
        StopWatchThread();
        std::lock_guard<std::mutex> lock(g_daemon_mutex);
        std::string error;
        *should_exit = true;
        state->running = false;
#ifdef _WIN32
        if (state->runtime_pid > 0 && IsProcessRunning(state->runtime_pid)) StopProcess(state->runtime_pid);
#endif
        state->runtime_pid = 0;
        state->runtime_status = "stopped";
        if (state->runtime_stop_reason.empty() || state->runtime_stop_reason == "running" || state->runtime_stop_reason == "starting") {
            state->runtime_stop_reason = "daemon_stopped";
        }
        state->watch = DefaultWatchState();
        RemoveRuntimeArtifacts(*state);
        RemoveState(state->project_id, &error);
        return nlohmann::json{{"ok", true}, {"stopped", true}};
    }

    std::lock_guard<std::mutex> lock(g_daemon_mutex);
    RefreshRuntimeStateUnlocked(state);
    if (cmd == "status") return nlohmann::json{{"ok", true}, {"daemon", StateToJson(*state)}};
    if (cmd == "open") {
        std::string error;
        const auto root = std::filesystem::absolute(request.value("project_root", state->project_root));
        if (!std::filesystem::exists(root)) return ErrorResponse("project_not_found", "项目目录不存在");
        ProjectIdentity identity;
        if (!EnsureProjectIdentity(root, &identity, &error)) return ErrorResponse("project_identity_failed", error);
        state->running = true;
        state->project_id = identity.project_id;
        state->runtime_id = identity.runtime_id;
        state->project_root = root.string();
        state->project = LoadProjectConfig(root, &error);
        if (!error.empty()) return ErrorResponse("project_config_error", error);
        state->last_build = nlohmann::json{{"ok", true}, {"status", "not_built"}};
        ConfigureWatchUnlocked(*state, false);
        state->watch = DefaultWatchState();
        if (!StartRuntime(root, state, &error)) return ErrorResponse("runtime_start_failed", error);
        if (!SaveState(*state, &error)) return ErrorResponse("state_write_failed", error);
        return nlohmann::json{{"ok", true}, {"project", ProjectToJson(root, state->project)}, {"runtime_status", "runtime_started"}, {"daemon", StateToJson(*state)}};
    }
    if (cmd == "info") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = std::filesystem::path(state->project_root);
        if (!std::filesystem::exists(root)) return ErrorResponse("project_not_found", "项目目录不存在");
        return nlohmann::json{{"ok", true},
                              {"project", ProjectToJson(root, state->project)},
                              {"runtime_status", state->runtime_status},
                              {"runtime_stop_reason", state->runtime_stop_reason},
                              {"build_status", state->last_build.is_null() ? nlohmann::json{{"ok", true}, {"status", "not_built"}} : state->last_build},
                              {"file_tree", BuildProjectFileTree(root, 2)},
                              {"daemon", StateToJson(*state)}};
    }
    if (cmd == "read") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = std::filesystem::path(state->project_root);
        std::filesystem::path resolved_path;
        std::string error;
        if (!ResolveProjectPath(root, request.value("path", ""), &resolved_path, &error)) return ErrorResponse("invalid_args", error);
        if (!std::filesystem::exists(resolved_path)) return ErrorResponse("file_not_found", "文件不存在");
        if (std::filesystem::is_directory(resolved_path)) return ErrorResponse("invalid_args", "path 必须指向文件");
        std::string content;
        if (!ReadFileBytes(resolved_path, &content, &error)) return ErrorResponse("file_read_failed", error);
        const auto encoding = request.value("encoding", "utf8");
        if (encoding != "utf8" && encoding != "base64") return ErrorResponse("invalid_args", "encoding 仅支持 utf8 或 base64");
        return nlohmann::json{{"ok", true}, {"path", std::filesystem::relative(resolved_path, root).generic_string()}, {"encoding", encoding}, {"content", encoding == "base64" ? Base64Encode(content) : content}, {"bytes", content.size()}};
    }
    if (cmd == "write") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = std::filesystem::path(state->project_root);
        std::filesystem::path resolved_path;
        std::string error;
        if (!ResolveProjectPath(root, request.value("path", ""), &resolved_path, &error)) return ErrorResponse("invalid_args", error);
        const auto encoding = request.value("encoding", "utf8");
        if (encoding != "utf8" && encoding != "base64") return ErrorResponse("invalid_args", "encoding 仅支持 utf8 或 base64");
        if (!request.contains("content") || !request["content"].is_string()) return ErrorResponse("invalid_args", "write 缺少 content");
        std::string content = request["content"].get<std::string>();
        if (encoding == "base64" && !Base64Decode(content, &content, &error)) return ErrorResponse("invalid_args", error);
        if (!WriteFileBytes(resolved_path, content, &error)) return ErrorResponse("file_write_failed", error);
        return nlohmann::json{{"ok", true}, {"path", std::filesystem::relative(resolved_path, root).generic_string()}, {"bytes", content.size()}, {"encoding", encoding}};
    }
    if (cmd == "build") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = std::filesystem::path(state->project_root);
        if (!std::filesystem::exists(root)) return ErrorResponse("project_not_found", "项目目录不存在");
        std::string error;
        auto build_status = BuildEsbuildStatus(root, state->project, &error);
        if (!error.empty()) return ErrorResponse("build_failed", error);
        state->last_build = build_status;
        if (request.value("watch", false)) {
            ConfigureWatchUnlocked(*state, true);
            EnsureWatchThreadStarted(state);
            UpdateWatchStateUnlocked(state, "watching", {});
        }
        if (!SaveState(*state, &error)) return ErrorResponse("state_write_failed", error);
        if (request.value("watch", false)) {
            build_status["watch"] = state->watch;
            build_status["daemon"] = StateToJson(*state);
        }
        return build_status;
    }
    if (cmd == "build_status") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        auto resp = state->last_build.is_null() ? nlohmann::json{{"ok", true}, {"status", "not_built"}} : state->last_build;
        resp["watch"] = state->watch.is_null() ? DefaultWatchState() : state->watch;
        return resp;
    }
    if (cmd == "snapshot") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        auto snapshot_path = GetRuntimeSnapshotPath(*state);
        auto snapshot = ReadJsonFile(snapshot_path);
        if ((!snapshot.is_object() || !snapshot.value("ok", false)) &&
            state->runtime_pid > 0 && IsProcessRunning(state->runtime_pid)) {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
            while (std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                snapshot = ReadJsonFile(snapshot_path);
                if (snapshot.is_object() && snapshot.value("ok", false)) break;
            }
        }
        if (snapshot.is_object() && snapshot.value("ok", false)) {
            snapshot["timestamp"] = CurrentTimestampIso8601();
            snapshot["source"] = snapshot_path.string();
            return snapshot;
        }
        return nlohmann::json{{"ok", true}, {"timestamp", CurrentTimestampIso8601()}, {"viewport", {{"width", state->project.width}, {"height", state->project.height}, {"dpr", 1.0}}}, {"screenshot_base64", ""}, {"tree", {{"node_id", "stub-root"}, {"tag", "body"}, {"text", nullptr}, {"rect", {{"x", 0}, {"y", 0}, {"w", state->project.width}, {"h", state->project.height}}}, {"attrs", nlohmann::json::object()}, {"interactive", false}, {"visible", true}, {"scroll", {{"x", 0}, {"y", 0}, {"max_x", 0}, {"max_y", 0}}}, {"children", nlohmann::json::array()}}}, {"note", "P0 stub: snapshot 文件尚未就绪"}};
    }
    if (cmd == "logs") return std::filesystem::exists(GetRuntimeConsolePath(*state)) ? BuildStructuredRuntimeBufferResponse(GetRuntimeConsolePath(*state), "stdout", 200, "entries") : BuildStructuredLogTail(GetRuntimeStdoutLogPath(*state), "stdout", 200, "entries");
    if (cmd == "errors") return std::filesystem::exists(GetRuntimeErrorsPath(*state)) ? BuildStructuredRuntimeBufferResponse(GetRuntimeErrorsPath(*state), "stderr", 200, "errors") : BuildStructuredLogTail(GetRuntimeStderrLogPath(*state), "stderr", 200, "errors");
    if (cmd == "eval") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        if (state->runtime_pid <= 0 || !IsProcessRunning(state->runtime_pid)) return ErrorResponse("runtime_not_running", "runtime 未运行");
        const auto code = request.value("code", "");
        if (code.empty()) return ErrorResponse("invalid_args", "eval 缺少 code");
        std::string error;
        nlohmann::json resp;
        if (!RequestRuntimeEval(*state, state->runtime_pid, code, &resp, &error)) return ErrorResponse("eval_failed", error);
        resp["source"] = GetRuntimeResponsePath(*state).string();
        return resp;
    }
    if (cmd == "query_element" || cmd == "inspect" || cmd == "click" || cmd == "input_text" || cmd == "scroll" || cmd == "highlight") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        if (state->runtime_pid <= 0 || !IsProcessRunning(state->runtime_pid)) return ErrorResponse("runtime_not_running", "runtime 未运行");
        const auto selector = request.value("selector", std::string{});
        if (selector.empty()) return ErrorResponse("invalid_args", cmd + " 缺少 selector");
        nlohmann::json runtime_cmd{{"type", cmd}, {"selector", selector}};
        if (cmd == "input_text") {
            if (!request.contains("text") || !request["text"].is_string()) return ErrorResponse("invalid_args", "input_text 缺少 text");
            runtime_cmd["text"] = request["text"];
        } else if (cmd == "scroll") {
            bool has_axis = false;
            if (request.contains("x")) {
                runtime_cmd["x"] = request["x"];
                has_axis = true;
            }
            if (request.contains("y")) {
                runtime_cmd["y"] = request["y"];
                has_axis = true;
            }
            if (!has_axis) return ErrorResponse("invalid_args", "scroll 至少需要 x 或 y");
        } else if (cmd == "highlight") {
            if (request.contains("color")) runtime_cmd["color"] = request["color"];
        }
        std::string error;
        nlohmann::json resp;
        if (!RequestRuntimeUiCommand(*state, state->runtime_pid, std::move(runtime_cmd), &resp, &error)) {
            return ErrorResponse(cmd + "_failed", error);
        }
        resp["source"] = GetRuntimeResponsePath(*state).string();
        return resp;
    }
    if (cmd == "reload") {
#ifdef _WIN32
        std::string error;
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        auto reload = ReloadRuntimeBundleOrRestart(std::filesystem::path(state->project_root), state);
        if (!reload.value("ok", false)) return ErrorResponse("runtime_reload_failed", reload.value("message", std::string("runtime reload 失败")));
        if (!SaveState(*state, &error)) return ErrorResponse("state_write_failed", error);
        reload["daemon"] = StateToJson(*state);
        return reload;
#else
        return ErrorResponse("unsupported", "当前平台未实现 runtime reload");
#endif
    }
    return ErrorResponse("invalid_args", "未知 daemon 请求");
}

int RunDaemonServer(const std::vector<std::string>& args) {
#ifdef _WIN32
    std::string error;
    DaemonState state;
    state.running = true;
    state.pid = CurrentProcessId();
    state.started_at = CurrentTimestampIso8601();
    state.last_build = nlohmann::json{{"ok", true}, {"status", "not_built"}};
    state.watch = DefaultWatchState();

    std::filesystem::path project_root;
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == "--project") project_root = std::filesystem::absolute(args[i + 1]).lexically_normal();
        if (args[i] == "--project-id") state.project_id = args[i + 1];
    }
    if (!project_root.empty()) {
        ProjectIdentity identity;
        if (!EnsureProjectIdentity(project_root, &identity, &error)) return 1;
        state.project_id = identity.project_id;
        state.runtime_id = identity.runtime_id;
        state.project_root = project_root.string();
        state.project = LoadProjectConfig(project_root, &error);
    }
    if (state.runtime_id.empty()) state.runtime_id = state.project_id;
    if (!SaveState(state, &error)) return 1;

    const auto pipe_name = GetDaemonPipeName(state.project_id);
    bool should_exit = false;
    while (!should_exit) {
        HANDLE pipe = CreateNamedPipeA(pipe_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 65536, 65536, 0, nullptr);
        if (pipe == INVALID_HANDLE_VALUE) return 1;
        if (!ConnectNamedPipe(pipe, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED) { CloseHandle(pipe); continue; }
        std::string raw_request;
        std::string read_error;
        nlohmann::json response = ErrorResponse("invalid_request", "请求解析失败");
        if (ReadPipeMessage(pipe, &raw_request, &read_error)) {
            try {
                response = DispatchDaemonRequest(nlohmann::json::parse(raw_request), &state, &should_exit);
            } catch (const std::exception& e) {
                response = ErrorResponse("internal_error", e.what());
            } catch (...) {
                response = ErrorResponse("internal_error", "未知内部异常");
            }
        } else if (!read_error.empty()) {
            response = ErrorResponse("invalid_request", read_error);
        }
        const auto payload = response.dump();
        DWORD written = 0;
        WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr);
        FlushFileBuffers(pipe); DisconnectNamedPipe(pipe); CloseHandle(pipe);
    }
    StopWatchThread();
    return 0;
#else
    (void)args; return 1;
#endif
}

}  // namespace mbink::ui_dev
