#include "daemon_server.h"
#include "core/api/resource_package.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <optional>
#include <random>
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

std::filesystem::path JoinProjectPath(const std::filesystem::path& project_root, const std::string& relative_path) {
    return project_root / PathFromUtf8(relative_path);
}

std::string PathToGenericUtf8(const std::filesystem::path& path) {
    auto result = PathToUtf8(path);
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

std::string RelativePathToUtf8(const std::filesystem::path& path, const std::filesystem::path& root) {
    std::error_code ec;
    const auto relative = std::filesystem::relative(path, root, ec);
    return PathToGenericUtf8(ec ? path.filename() : relative);
}

std::filesystem::path GetProjectBuildLogPath(const std::filesystem::path& project_root, const ProjectConfig& config) {
    return std::filesystem::absolute(JoinProjectPath(project_root, config.out_dir) / "build.log");
}

constexpr uintmax_t kInlineSnapshotMaxBytes = 256 * 1024;

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
std::mutex g_runtime_command_mutex;
std::atomic<bool> g_daemon_stopping{false};
WatchContext g_watch;

std::string GenerateRuntimeEpoch() {
    const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::ostringstream oss;
    oss << "rt-" << std::hex << now << "-" << rng();
    return oss.str();
}

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
        const auto runtime_epoch = lifecycle.value("runtime_epoch", std::string{});
        if (!status.empty()) state->runtime_status = status;
        if (!reason.empty()) state->runtime_stop_reason = reason;
        if (!runtime_epoch.empty()) state->runtime_epoch = runtime_epoch;
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

bool MaterialConfigEquals(const ProjectConfig& a, const ProjectConfig& b) {
    return a.entry == b.entry &&
           a.src_dir == b.src_dir &&
           a.out_dir == b.out_dir &&
           a.template_name == b.template_name &&
           a.purpose == b.purpose &&
           a.runtime == b.runtime &&
           a.build_builder == b.build_builder &&
           a.build_jsx_factory == b.build_jsx_factory &&
           a.build_jsx_fragment == b.build_jsx_fragment &&
           a.build_external == b.build_external &&
           a.build_sourcemap == b.build_sourcemap &&
           a.build_minify == b.build_minify &&
           a.build_hide_console == b.build_hide_console &&
           a.width == b.width &&
           a.height == b.height &&
           a.title == b.title &&
           a.borderless == b.borderless &&
           a.resizable == b.resizable;
}

void MergeRuntimeFields(DaemonState* state, const DaemonState& source) {
    if (!state) return;
    state->runtime_pid = source.runtime_pid;
    state->runtime_epoch = source.runtime_epoch;
    state->runtime_status = source.runtime_status;
    state->runtime_stop_reason = source.runtime_stop_reason;
    if (source.stopping) state->stopping = true;
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
    const std::filesystem::path input_path = PathFromUtf8(relative_path);
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

uintmax_t FileSizeOrZero(const std::filesystem::path& path) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    return ec ? 0 : size;
}

std::wstring QuoteWindowsArg(const std::wstring& value) {
    std::wstring quoted = L"\"";
    size_t backslashes = 0;
    for (wchar_t ch : value) {
        if (ch == L'\\') {
            ++backslashes;
            continue;
        }
        if (ch == L'"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(ch);
            backslashes = 0;
            continue;
        }
        if (backslashes > 0) {
            quoted.append(backslashes, L'\\');
            backslashes = 0;
        }
        quoted.push_back(ch);
    }
    if (backslashes > 0) quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

std::wstring BuildWindowsCommandLine(const std::filesystem::path& exe, const std::string& args) {
    std::wstring command = QuoteWindowsArg(exe.wstring());
    if (!args.empty()) {
        command.push_back(L' ');
        command += Utf8ToWide(args);
    }
    return command;
}

bool SnapshotFileLooksComplete(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || std::filesystem::is_directory(path, ec)) return false;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size == 0) return false;

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    const auto probe = static_cast<std::streamoff>(std::min<uintmax_t>(size, 4096));
    ifs.seekg(-probe, std::ios::end);
    std::string tail(static_cast<size_t>(probe), '\0');
    ifs.read(tail.data(), probe);
    if (!ifs.good() && !ifs.eof()) return false;
    for (auto it = tail.rbegin(); it != tail.rend(); ++it) {
        const unsigned char ch = static_cast<unsigned char>(*it);
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
        return ch == '}';
    }
    return false;
}

nlohmann::json SnapshotFileMetadata(const std::filesystem::path& snapshot_path, uintmax_t bytes) {
    return nlohmann::json{{"path", PathToUtf8(snapshot_path)},
                          {"encoding", "utf8"},
                          {"mime_type", "application/json"},
                          {"bytes", bytes}};
}

bool FileHasPngSignature(const std::filesystem::path& path) {
    static constexpr unsigned char kPngSignature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    unsigned char signature[8] = {};
    ifs.read(reinterpret_cast<char*>(signature), sizeof(signature));
    return ifs.gcount() == static_cast<std::streamsize>(sizeof(signature)) &&
           std::memcmp(signature, kPngSignature, sizeof(signature)) == 0;
}

nlohmann::json ScreenshotFileMetadata(const nlohmann::json& screenshot, bool include_inline) {
    if (!screenshot.is_object() || !screenshot.value("included", false)) {
        return nlohmann::json{{"included", false}};
    }
    auto result = screenshot;
    if (!include_inline) {
        result.erase("base64");
        result["encoding"] = "file";
    }
    return result;
}

bool SnapshotScreenshotLooksComplete(const nlohmann::json& snapshot) {
    if (!snapshot.is_object()) return false;
    const auto& screenshot = snapshot.contains("screenshot") ? snapshot["screenshot"] : nlohmann::json();
    if (!screenshot.is_object() || !screenshot.value("included", false)) return false;
    const auto path = screenshot.value("path", std::string{});
    const auto expected_bytes = screenshot.value("bytes", uintmax_t{0});
    if (path.empty() || expected_bytes == 0) return false;
    const auto screenshot_path = PathFromUtf8(path);
    const auto actual_bytes = FileSizeOrZero(screenshot_path);
    return actual_bytes == expected_bytes && actual_bytes > 0 && FileHasPngSignature(screenshot_path);
}

bool SnapshotMatchesRuntimeEpoch(const nlohmann::json& snapshot, const DaemonState& state) {
    if (!snapshot.is_object() || !snapshot.value("ok", false)) return false;
    const auto snapshot_epoch = snapshot.value("runtime_epoch", std::string{});
    return !snapshot_epoch.empty() && !state.runtime_epoch.empty() && snapshot_epoch == state.runtime_epoch;
}

nlohmann::json BuildSnapshotFileResponse(const DaemonState& state,
                                         const std::filesystem::path& snapshot_path,
                                         const nlohmann::json& snapshot,
                                         uintmax_t bytes,
                                         const std::string& note) {
    const nlohmann::json viewport = snapshot.contains("viewport")
                                        ? snapshot["viewport"]
                                        : nlohmann::json{{"width", state.project.width},
                                                         {"height", state.project.height},
                                                         {"dpr", 1.0}};
    return nlohmann::json{{"ok", true},
                          {"timestamp", CurrentTimestampIso8601()},
                          {"response_mode", "file"},
                          {"viewport", viewport},
                          {"snapshot", SnapshotFileMetadata(snapshot_path, bytes)},
                          {"screenshot", ScreenshotFileMetadata(snapshot.value("screenshot", nlohmann::json{{"included", false}}),
                                                                 snapshot.value("screenshot_base64", std::string{}).size() > 0)},
                          {"screenshot_base64", snapshot.value("screenshot_base64", std::string{})},
                          {"runtime_epoch", state.runtime_epoch},
                          {"source", PathToUtf8(snapshot_path)},
                          {"inline_limit_bytes", kInlineSnapshotMaxBytes},
                          {"note", note}};
}

nlohmann::json BuildSnapshotResponse(const DaemonState& state,
                                     const std::filesystem::path& snapshot_path,
                                     const nlohmann::json& snapshot,
                                     uintmax_t snapshot_bytes,
                                     const std::string& response_mode) {
    if (response_mode == "file") {
        return BuildSnapshotFileResponse(state,
                                         snapshot_path,
                                         snapshot,
                                         snapshot_bytes,
                                         "snapshot stored on disk; read snapshot.path to avoid large IPC/stdout payloads");
    }
    if (response_mode == "auto" && snapshot_bytes > kInlineSnapshotMaxBytes) {
        return BuildSnapshotFileResponse(state,
                                         snapshot_path,
                                         snapshot,
                                         snapshot_bytes,
                                         "snapshot is large, so response was returned by file path");
    }

    auto inline_snapshot = snapshot;
    inline_snapshot["timestamp"] = CurrentTimestampIso8601();
    inline_snapshot["source"] = PathToUtf8(snapshot_path);
    inline_snapshot["response_mode"] = "inline";
    inline_snapshot["snapshot"] = SnapshotFileMetadata(snapshot_path, snapshot_bytes);
    inline_snapshot["inline_limit_bytes"] = kInlineSnapshotMaxBytes;
    return inline_snapshot;
}

bool SnapshotRequestHasCustomOptions(const nlohmann::json& request) {
    return request.contains("max_nodes") ||
           request.contains("max_depth") ||
           request.contains("root_selector") ||
           request.value("include_screenshot", false) ||
           request.value("inline_screenshot", false);
}

std::filesystem::path BuildRequestSnapshotPath(const DaemonState& state) {
    auto path = GetRuntimeSnapshotPath(state);
    path += "." + GenerateRuntimeEpoch() + ".request.json";
    return path;
}

std::filesystem::path BuildRequestScreenshotPath(const std::filesystem::path& snapshot_path) {
    auto path = snapshot_path;
    if (path.has_extension()) {
        path.replace_extension(".png");
    } else {
        path += ".png";
    }
    return path;
}

std::string ReadFileTextOrEmpty(const std::filesystem::path& path) {
    std::string content;
    std::string error;
    return ReadFileBytes(path, &content, &error) ? content : std::string{};
}

std::vector<std::string> SplitProcessLines(const std::string& raw_output) {
    std::vector<std::string> lines;
    std::stringstream ss(raw_output);
    std::string line;
    while (std::getline(ss, line)) {
        line = TrimAscii(line);
        line = SanitizeJsonText(line);
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

struct ProcessRunResult {
    DWORD exit_code = 1;
    bool timed_out = false;
    int duration_ms = 0;
    std::string raw_output;
};

bool RunCapturedCommand(const std::string& command,
                        const std::filesystem::path& cwd,
                        int timeout_ms,
                        const std::filesystem::path& capture_path,
                        ProcessRunResult* result,
                        std::string* error) {
    if (result) *result = ProcessRunResult{};
    std::error_code ec;
    const auto parent = capture_path.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent, ec);
    if (ec) {
        if (error) *error = "failed to create build log directory";
        return false;
    }

    SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE output_handle = CreateFileW(capture_path.wstring().c_str(),
                                       GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       &sa,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL,
                                       nullptr);
    if (output_handle == INVALID_HANDLE_VALUE) {
        if (error) *error = "failed to create build log file";
        return false;
    }

    HANDLE input_handle = CreateFileW(L"NUL",
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      &sa,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      nullptr);
    if (input_handle == INVALID_HANDLE_VALUE) {
        CloseHandle(output_handle);
        if (error) *error = "failed to open NUL input";
        return false;
    }

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = input_handle;
    si.hStdOutput = output_handle;
    si.hStdError = output_handle;

    std::wstring mutable_cmd = Utf8ToWide(command);
    mutable_cmd.push_back(L'\0');
    const auto working_dir = cwd.wstring();

    const auto started = std::chrono::steady_clock::now();
    const BOOL created = CreateProcessW(nullptr,
                                        mutable_cmd.data(),
                                        nullptr,
                                        nullptr,
                                        TRUE,
                                        CREATE_NO_WINDOW,
                                        nullptr,
                                        working_dir.empty() ? nullptr : working_dir.c_str(),
                                        &si,
                                        &pi);
    CloseHandle(input_handle);
    CloseHandle(output_handle);

    if (!created) {
        if (error) *error = "failed to start build command";
        return false;
    }

    const DWORD wait_result = WaitForSingleObject(pi.hProcess, static_cast<DWORD>(timeout_ms));
    DWORD exit_code = 1;
    bool timed_out = false;
    if (wait_result == WAIT_TIMEOUT) {
        timed_out = true;
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 5000);
    }
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (result) {
        result->exit_code = timed_out ? 1 : exit_code;
        result->timed_out = timed_out;
        result->duration_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count());
        result->raw_output = ReadFileTextOrEmpty(capture_path);
        if (timed_out) {
            result->raw_output += "\nbuild timed out after " + std::to_string(timeout_ms) + "ms";
        }
    }
    return true;
}

std::string SanitizeArtifactStem(std::string value) {
    value = TrimAscii(value);
    for (char& ch : value) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if ((uch >= 'a' && uch <= 'z') || (uch >= 'A' && uch <= 'Z') || (uch >= '0' && uch <= '9') || ch == '-' || ch == '_') continue;
        ch = '-';
    }
    while (!value.empty() && (value.front() == '-' || value.front() == '_')) value.erase(value.begin());
    while (!value.empty() && (value.back() == '-' || value.back() == '_')) value.pop_back();
    return value.empty() ? std::string{"mbink-app"} : value;
}

std::string DetectProjectRuntime(const ProjectConfig& config) {
    const auto lower = ToLowerAscii(config.template_name);
    if (lower == "rust" || lower.size() >= 5 && lower.rfind("/rust") == lower.size() - 5) return "rust";
    if (lower == "go" || lower.size() >= 3 && lower.rfind("/go") == lower.size() - 3) return "go";
    if (lower == "python" || lower.size() >= 7 && lower.rfind("/python") == lower.size() - 7) return "python";
    if (!config.runtime.empty() && config.runtime != "unknown") return config.runtime;
    return "tool";
}

std::filesystem::path ResourcePackagePath(const std::filesystem::path& project_root, const ProjectConfig& config) {
    return std::filesystem::absolute(JoinProjectPath(project_root, config.out_dir) / "app.mbrp").lexically_normal();
}

std::filesystem::path ResourceInputDir(const std::filesystem::path& project_root, const ProjectConfig& config) {
    return std::filesystem::absolute(JoinProjectPath(project_root, config.out_dir) / "app").lexically_normal();
}

bool CopyFileOverwrite(const std::filesystem::path& from, const std::filesystem::path& to, std::string* error) {
    std::error_code ec;
    const auto parent = to.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent, ec);
    if (ec) {
        if (error) *error = "failed to create output directory";
        return false;
    }
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        if (error) *error = "failed to copy " + PathToUtf8(from) + " to " + PathToUtf8(to);
        return false;
    }
    return true;
}

bool SetWindowsGuiSubsystem(const std::filesystem::path& exe_path, bool hide_console, std::string* error) {
    std::string data;
    if (!ReadFileBytes(exe_path, &data, error)) return false;
    if (data.size() < 64) {
        if (error) *error = "invalid PE file: too small";
        return false;
    }
    auto* bytes = reinterpret_cast<unsigned char*>(data.data());
    if (bytes[0] != 'M' || bytes[1] != 'Z') {
        if (error) *error = "invalid PE file: missing MZ signature";
        return false;
    }
    uint32_t pe_offset = 0;
    std::memcpy(&pe_offset, bytes + 0x3C, sizeof(pe_offset));
    if (pe_offset + 24 + 70 > data.size()) {
        if (error) *error = "invalid PE file: PE header out of bounds";
        return false;
    }
    if (bytes[pe_offset] != 'P' || bytes[pe_offset + 1] != 'E' ||
        bytes[pe_offset + 2] != 0 || bytes[pe_offset + 3] != 0) {
        if (error) *error = "invalid PE file: missing PE signature";
        return false;
    }
    const size_t subsystem_offset = static_cast<size_t>(pe_offset) + 24 + 68;
    if (subsystem_offset + 2 > data.size()) {
        if (error) *error = "invalid PE file: subsystem field out of bounds";
        return false;
    }
    const uint16_t subsystem = hide_console ? 2 : 3;
    std::memcpy(bytes + subsystem_offset, &subsystem, sizeof(subsystem));
    if (!WriteFileBytes(exe_path, data, error)) return false;
    return true;
}

nlohmann::json CompileResourcePackageStatus(const std::filesystem::path& project_root,
                                            const ProjectConfig& config,
                                            const std::filesystem::path& app_bundle,
                                            std::string* error) {
    const auto input_dir = ResourceInputDir(project_root, config);
    const auto output_file = ResourcePackagePath(project_root, config);

    std::error_code ec;
    if (std::filesystem::exists(input_dir, ec)) {
        const auto out_dir = std::filesystem::absolute(JoinProjectPath(project_root, config.out_dir)).lexically_normal();
        if (IsPathInsideRoot(out_dir, input_dir)) {
            std::filesystem::remove_all(input_dir, ec);
        }
    }
    std::filesystem::create_directories(input_dir, ec);
    if (ec) {
        if (error) *error = "failed to create resource input directory";
        return nlohmann::json{{"ok", false}, {"error", error ? *error : ""}};
    }

    if (!CopyFileOverwrite(app_bundle, input_dir / "app.js", error)) {
        return nlohmann::json{{"ok", false}, {"error", error ? *error : ""}};
    }
    const auto config_path = project_root / "mbink.config.json";
    if (std::filesystem::exists(config_path, ec)) {
        std::string ignored;
        CopyFileOverwrite(config_path, input_dir / "mbink.config.json", &ignored);
    }

    const auto input_dir_text = PathToUtf8(input_dir);
    const auto output_file_text = PathToUtf8(output_file);
    std::string compile_error;
    if (!mbink::resourcepkg::CompileResources(input_dir_text.c_str(), output_file_text.c_str(), "", compile_error)) {
        if (compile_error.empty()) compile_error = "resource package build failed";
        if (error) *error = compile_error;
        return nlohmann::json{{"ok", false},
                              {"input_dir", RelativePathToUtf8(input_dir, project_root)},
                              {"output", RelativePathToUtf8(output_file, project_root)},
                              {"error", error ? *error : ""}};
    }

    return nlohmann::json{{"ok", true},
                          {"format", "mbrp"},
                          {"input_dir", RelativePathToUtf8(input_dir, project_root)},
                          {"output", RelativePathToUtf8(output_file, project_root)}};
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
    if (g_daemon_stopping.load()) {
        if (error) *error = "shutdown_in_progress";
        return false;
    }
    std::unique_lock<std::mutex> runtime_lock(g_runtime_command_mutex);
    if (g_daemon_stopping.load()) {
        if (error) *error = "shutdown_in_progress";
        return false;
    }
    const auto command_path = GetRuntimeCommandPath(state);
    const auto response_path = GetRuntimeResponsePath(state);
    const std::string type = command.value("type", "command");
    const auto request_id = type + "-" + std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    command["id"] = request_id;
    if (!state.runtime_epoch.empty()) command["runtime_epoch"] = state.runtime_epoch;
    RemoveFileIfExists(command_path);
    RemoveFileIfExists(response_path);

    if (!WriteJsonFile(command_path, command)) {
        if (error) *error = "写入 runtime 请求失败";
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    while (std::chrono::steady_clock::now() < deadline) {
        if (g_daemon_stopping.load()) {
            if (error) *error = "shutdown_in_progress";
            return false;
        }
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
                                 nlohmann::json{{"type", "reload_bundle"}, {"bundle_path", PathToUtf8(bundle_path)}},
                                 response,
                                 error);
}

void WriteRuntimeLifecycleUnlocked(const DaemonState& state,
                                   const std::string& status,
                                   const std::string& reason) {
    if (state.runtime_epoch.empty()) return;
    WriteJsonFile(GetRuntimeLifecyclePath(state),
                  nlohmann::json{{"ok", true},
                                 {"status", status},
                                 {"reason", reason},
                                 {"runtime_epoch", state.runtime_epoch},
                                 {"pid", state.runtime_pid},
                                 {"timestamp", CurrentTimestampIso8601()}});
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

nlohmann::json RuntimeCommandErrorResponse(const std::string& fallback_code, const std::string& message) {
    if (message == "runtime_command_timeout" || message.find("超时") != std::string::npos ||
        message.find("timeout") != std::string::npos) {
        return ErrorResponse("runtime_command_timeout", message.empty() ? "runtime command timed out" : message);
    }
    if (message == "shutdown_in_progress") {
        return ErrorResponse("shutdown_in_progress", "runtime is shutting down");
    }
    return ErrorResponse(fallback_code, message);
}

std::filesystem::path ResolveReloadBundlePath(const std::filesystem::path& root, const ProjectConfig& project) {
    const auto built_bundle = std::filesystem::absolute(JoinProjectPath(root, project.out_dir) / "App.js").lexically_normal();
    std::error_code ec;
    if (std::filesystem::exists(built_bundle, ec) && std::filesystem::is_regular_file(built_bundle, ec)) return built_bundle;
    return std::filesystem::absolute(JoinProjectPath(root, project.entry)).lexically_normal();
}

nlohmann::json ReloadRuntimeBundleOrRestart(const std::filesystem::path& root, DaemonState* state) {
    RefreshRuntimeStateUnlocked(state);
    const auto bundle_path = ResolveReloadBundlePath(root, state->project);
    nlohmann::json runtime_response;
    std::string reload_error;
    RemoveFileIfExists(GetRuntimeSnapshotPath(*state));

    if (state->runtime_pid > 0 && IsProcessRunning(state->runtime_pid)) {
        if (RequestRuntimeReloadBundle(*state, state->runtime_pid, bundle_path, &runtime_response, &reload_error) &&
            runtime_response.value("ok", false)) {
            const auto next_epoch = runtime_response.value("runtime_epoch", std::string{});
            if (!next_epoch.empty()) state->runtime_epoch = next_epoch;
            state->runtime_status = "running";
            state->runtime_stop_reason = "running";
            WriteRuntimeLifecycleUnlocked(*state, state->runtime_status, state->runtime_stop_reason);
            return nlohmann::json{{"ok", true},
                                  {"mode", "reload_bundle"},
                                  {"fallback", false},
                                  {"runtime_epoch", state->runtime_epoch},
                                  {"bundle_path", PathToUtf8(bundle_path)},
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
                              {"bundle_path", PathToUtf8(bundle_path)},
                              {"message", "runtime 已被用户手动关闭，当前不会自动拉起"},
                              {"response", runtime_response}};
    }

    std::string restart_error;
    if (StartRuntime(root, state, &restart_error)) {
        return nlohmann::json{{"ok", true},
                              {"mode", "restart_runtime"},
                              {"fallback", true},
                              {"runtime_epoch", state->runtime_epoch},
                              {"bundle_path", PathToUtf8(bundle_path)},
                              {"message", reload_error.empty() ? "in-place reload 失败，已回退 restart_runtime" : reload_error},
                              {"response", runtime_response}};
    }

    return nlohmann::json{{"ok", false},
                          {"mode", "restart_runtime"},
                          {"fallback", true},
                          {"bundle_path", PathToUtf8(bundle_path)},
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
                          {"source", PathToUtf8(path)},
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
                          {"source", PathToUtf8(path)},
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

bool SourceLooksLikeJsx(const std::filesystem::path& file_path) {
    const auto lower_ext = ToLowerAscii(file_path.extension().string());
    if (lower_ext == ".jsx" || lower_ext == ".tsx") return true;
    if (lower_ext != ".js" && lower_ext != ".mjs" && lower_ext != ".ts") return false;

    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs) return false;
    std::string source((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (source.find("</") != std::string::npos) return true;
    for (size_t i = 0; i + 1 < source.size(); ++i) {
        if (source[i] != '<') continue;
        const char next = source[i + 1];
        if ((next >= 'A' && next <= 'Z') || (next >= 'a' && next <= 'z') || next == '>') {
            return true;
        }
    }
    return false;
}

bool ProjectRequiresJsxBuild(const std::filesystem::path& project_root, const ProjectConfig& config) {
    if (config.template_name == "preact-jsx" || config.template_name == "preact-ts") return true;
    if (config.template_name == "minimal/tool" || config.template_name == "minimal/python" ||
        config.template_name == "minimal/rust" || config.template_name == "minimal/go" ||
        config.template_name == "showcase/tool" || config.template_name == "showcase/python" ||
        config.template_name == "showcase/rust" || config.template_name == "showcase/go" ||
        config.template_name == "desktop-app/tool" || config.template_name == "desktop-app/python" ||
        config.template_name == "desktop-app/rust" || config.template_name == "desktop-app/go") return true;

    const auto entry_path = std::filesystem::absolute(JoinProjectPath(project_root, config.entry)).lexically_normal();
    if (SourceLooksLikeJsx(entry_path)) return true;

    const auto scan_root = std::filesystem::absolute(JoinProjectPath(project_root, config.src_dir)).lexically_normal();
    std::error_code ec;
    if (!std::filesystem::exists(scan_root, ec) || !std::filesystem::is_directory(scan_root, ec)) return false;

    size_t scanned = 0;
    for (std::filesystem::recursive_directory_iterator it(scan_root, ec), end; !ec && it != end && scanned < 64; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        const auto ext = ToLowerAscii(it->path().extension().string());
        if (ext != ".js" && ext != ".mjs" && ext != ".jsx" && ext != ".ts" && ext != ".tsx") continue;
        ++scanned;
        if (SourceLooksLikeJsx(it->path())) return true;
    }
    return false;
}

bool ShouldBuildBeforeRun(const std::filesystem::path& project_root, const ProjectConfig& config) {
    const auto entry_path = std::filesystem::absolute(JoinProjectPath(project_root, config.entry)).lexically_normal();
    const auto ext = ToLowerAscii(entry_path.extension().string());
    if (ext == ".jsx" || ext == ".tsx" || ext == ".ts") return true;
    return ProjectRequiresJsxBuild(project_root, config);
}


std::filesystem::path DetectBuildEntryPoint(const std::filesystem::path& project_root, const ProjectConfig& config) {
    const std::vector<std::filesystem::path> preferred_candidates = {
        JoinProjectPath(project_root, config.src_dir) / "App.jsx",
        JoinProjectPath(project_root, config.src_dir) / "app.jsx",
        JoinProjectPath(project_root, config.src_dir) / "main.jsx",
        JoinProjectPath(project_root, config.src_dir) / "index.jsx",
        JoinProjectPath(project_root, config.src_dir) / "App.tsx",
        JoinProjectPath(project_root, config.src_dir) / "main.tsx",
        JoinProjectPath(project_root, config.src_dir) / "index.tsx"
    };
    if (config.template_name == "preact-jsx" || config.template_name == "preact-ts" ||
        config.template_name == "minimal/tool" || config.template_name == "minimal/python" ||
        config.template_name == "minimal/rust" || config.template_name == "minimal/go" ||
        config.template_name == "showcase/tool" || config.template_name == "showcase/python" ||
        config.template_name == "showcase/rust" || config.template_name == "showcase/go" ||
        config.template_name == "desktop-app/tool" || config.template_name == "desktop-app/python" ||
        config.template_name == "desktop-app/rust" || config.template_name == "desktop-app/go") {
        for (const auto& candidate : preferred_candidates) {
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
                return std::filesystem::absolute(candidate).lexically_normal();
            }
        }
    }

    const auto entry_path = std::filesystem::absolute(JoinProjectPath(project_root, config.entry)).lexically_normal();
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
        JoinProjectPath(project_root, config.src_dir) / "App.jsx",
        JoinProjectPath(project_root, config.src_dir) / "app.jsx",
        JoinProjectPath(project_root, config.src_dir) / "main.jsx",
        JoinProjectPath(project_root, config.src_dir) / "index.jsx",
        JoinProjectPath(project_root, config.src_dir) / "App.tsx",
        JoinProjectPath(project_root, config.src_dir) / "main.tsx",
        JoinProjectPath(project_root, config.src_dir) / "index.tsx",
        JoinProjectPath(project_root, config.src_dir) / "app.js",
        JoinProjectPath(project_root, config.src_dir) / "main.js",
        JoinProjectPath(project_root, config.src_dir) / "index.js"
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
    const auto out_dir = std::filesystem::absolute(JoinProjectPath(project_root, config.out_dir)).lexically_normal();
    std::error_code ec;
    std::filesystem::create_directories(out_dir, ec);
    if (ec) {
        if (error) *error = "创建 out_dir 失败";
        return {};
    }
    const auto output_file = out_dir / "App.js";
    std::string esbuild_args = QuoteForCmd(PathToUtf8(entry_point)) +
                               " --bundle --format=esm --outfile=" + QuoteForCmd(PathToUtf8(output_file)) +
                               " --jsx=transform --jsx-factory=" + QuoteForCmd(config.build_jsx_factory) +
                               " --jsx-fragment=" + QuoteForCmd(config.build_jsx_fragment) +
                               " --loader:.js=jsx --loader:.mjs=jsx" +
                               (config.build_sourcemap ? " --sourcemap=inline" : " --sourcemap=false") +
                               (config.build_minify ? " --minify" : "") +
                               " --color=false --log-level=info --log-limit=0";
    for (const auto& ext : config.build_external) {
        if (ext.empty()) continue;
        esbuild_args += " --external:" + QuoteForCmd(ext);
    }
    std::string command;
    if (IsCmdScript(esbuild)) {
        command = "cmd.exe /d /c call " + QuoteForCmd(PathToUtf8(esbuild)) + " " + esbuild_args;
    } else {
        command = QuoteForCmd(PathToUtf8(esbuild)) + " " + esbuild_args;
    }

    SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE read_pipe = INVALID_HANDLE_VALUE;
    HANDLE write_pipe = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        if (error) *error = "创建构建输出管道失败";
        return {};
    }
    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = write_pipe;
    si.hStdError = write_pipe;

    std::wstring mutable_cmd = Utf8ToWide(command);
    mutable_cmd.push_back(L'\0');
    const auto working_dir = project_root.wstring();
    const BOOL created = CreateProcessW(nullptr, mutable_cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                        nullptr, working_dir.empty() ? nullptr : working_dir.c_str(), &si, &pi);
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
                          {"pipeline", "mbink-ui-dev"},
                          {"started_at", started_iso},
                          {"finished_at", finished_iso},
                          {"duration_ms", duration_ms},
                          {"entry_point", RelativePathToUtf8(entry_point, project_root)},
                          {"out_dir", config.out_dir},
                          {"outputs", nlohmann::json::array({RelativePathToUtf8(output_file, project_root)})},
                          {"warnings", warnings},
                          {"errors", errors},
                          {"raw_output", lines},
                          {"build_log", PathToUtf8(build_log_path)}};
    if (exit_code == 0) {
        std::string package_error;
        auto package_status = CompileResourcePackageStatus(project_root, config, output_file, &package_error);
        status["resource_package"] = package_status;
        if (package_status.value("ok", false)) {
            const auto package_rel = package_status.value("output", std::string{});
            if (!package_rel.empty()) status["outputs"].push_back(package_rel);
        } else {
            status["ok"] = false;
            status["status"] = "failed";
            status["errors"].push_back({{"message", package_error.empty() ? "resource package build failed" : package_error}});
            if (error) *error = package_error.empty() ? "resource package build failed" : package_error;
        }
    }
    WriteJsonFile(build_log_path, status);
    return status;
}

std::filesystem::path FindExecutableOnPath(const std::vector<std::string>& names) {
    std::vector<std::filesystem::path> candidates;
    for (const auto& name : names) candidates.emplace_back(name);
    const char* path_env = std::getenv("PATH");
    if (path_env && *path_env) {
        std::stringstream ss(path_env);
        std::string segment;
        while (std::getline(ss, segment, ';')) {
            segment = TrimAscii(segment);
            if (segment.empty()) continue;
            for (const auto& name : names) candidates.emplace_back(std::filesystem::path(segment) / name);
        }
    }
    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !std::filesystem::is_directory(candidate, ec)) {
            return std::filesystem::absolute(candidate, ec).lexically_normal();
        }
    }
    return {};
}

bool IsPythonNativeBinary(const std::filesystem::path& path) {
    const auto ext = ToLowerAscii(path.extension().string());
    return ext == ".pyd" || ext == ".dll" || ext == ".so" || ext == ".dylib";
}

bool IsIgnoredPythonPackageDir(const std::filesystem::path& path) {
    const auto name = ToLowerAscii(path.filename().string());
    return name.empty() ||
           name == "__pycache__" ||
           name == ".git" ||
           name == ".hg" ||
           name == ".svn" ||
           name == ".venv" ||
           name == "venv" ||
           name == "env" ||
           name == "node_modules" ||
           name == ".dist" ||
           name == "build" ||
           name == "dist" ||
           name == "pyinstaller-work" ||
           name == "pyinstaller-spec";
}

std::optional<std::string> PythonPackageNameFromDir(const std::filesystem::path& package_dir,
                                                    const std::filesystem::path& search_root) {
    std::error_code ec;
    auto rel = std::filesystem::relative(package_dir, search_root, ec);
    if (ec || rel.empty()) return std::nullopt;

    std::vector<std::string> parts;
    for (const auto& part : rel) {
        const auto item = part.string();
        if (item.empty() || item == "." || item == "..") return std::nullopt;
        parts.push_back(item);
    }
    if (parts.empty()) return std::nullopt;

    std::filesystem::path current = search_root;
    for (const auto& part : parts) {
        current /= part;
        if (!std::filesystem::exists(current / "__init__.py")) return std::nullopt;
    }

    std::string name;
    for (const auto& part : parts) {
        if (!name.empty()) name += ".";
        name += part;
    }
    return name.empty() ? std::nullopt : std::optional<std::string>{name};
}

std::vector<std::filesystem::path> PythonImportSearchRoots(const std::filesystem::path& project_root,
                                                           const ProjectConfig& config) {
    std::vector<std::filesystem::path> roots = {
        project_root,
        project_root / "src",
        JoinProjectPath(project_root, config.src_dir),
        project_root / "host",
        project_root / "vendor"
    };
    std::vector<std::filesystem::path> result;
    for (const auto& root : roots) {
        std::error_code ec;
        const auto absolute = std::filesystem::absolute(root, ec).lexically_normal();
        if (ec || absolute.empty() || !std::filesystem::exists(absolute, ec) || !std::filesystem::is_directory(absolute, ec)) continue;
        if (std::find(result.begin(), result.end(), absolute) == result.end()) result.push_back(absolute);
    }
    return result;
}

std::optional<std::string> PythonVersionFromAbiTag(const std::filesystem::path& path) {
    const auto name = ToLowerAscii(path.filename().string());
    const std::string marker = ".cp";
    const auto pos = name.find(marker);
    if (pos == std::string::npos || pos + marker.size() + 2 > name.size()) return std::nullopt;

    size_t i = pos + marker.size();
    std::string digits;
    while (i < name.size() && std::isdigit(static_cast<unsigned char>(name[i]))) {
        digits.push_back(name[i]);
        ++i;
    }
    if (digits.size() == 2) {
        return digits.substr(0, 1) + "." + digits.substr(1, 1);
    }
    if (digits.size() == 3) {
        return digits.substr(0, 1) + "." + digits.substr(1, 2);
    }
    return std::nullopt;
}

struct PythonPackageScan {
    std::vector<std::string> collect_all;
    std::vector<std::string> native_binaries;
    std::optional<std::string> required_python_version;
};

PythonPackageScan ScanProjectPythonPackages(const std::filesystem::path& project_root,
                                            const ProjectConfig& config,
                                            const std::vector<std::filesystem::path>& search_roots) {
    PythonPackageScan scan;
    for (const auto& search_root : search_roots) {
        std::error_code ec;
        std::filesystem::recursive_directory_iterator it(search_root, std::filesystem::directory_options::skip_permission_denied, ec);
        const std::filesystem::recursive_directory_iterator end;
        for (; !ec && it != end; it.increment(ec)) {
            const auto& entry = *it;
            const auto path = entry.path();
            if (entry.is_directory(ec)) {
                if (IsIgnoredPythonPackageDir(path)) it.disable_recursion_pending();
                continue;
            }
            if (!entry.is_regular_file(ec) || !IsPythonNativeBinary(path)) continue;
            if (IsPathInsideRoot(JoinProjectPath(project_root, config.out_dir), path)) continue;

            scan.native_binaries.push_back(RelativePathToUtf8(path, project_root));
            if (!scan.required_python_version.has_value()) {
                scan.required_python_version = PythonVersionFromAbiTag(path);
            }

            auto dir = path.parent_path();
            while (!dir.empty() && IsPathInsideRoot(search_root, dir)) {
                if (std::filesystem::exists(dir / "__init__.py")) {
                    if (auto package_name = PythonPackageNameFromDir(dir, search_root)) {
                        if (*package_name != "mbink" &&
                            std::find(scan.collect_all.begin(), scan.collect_all.end(), *package_name) == scan.collect_all.end()) {
                            scan.collect_all.push_back(*package_name);
                        }
                        break;
                    }
                }
                if (dir == search_root) break;
                dir = dir.parent_path();
            }
        }
    }

    std::sort(scan.collect_all.begin(), scan.collect_all.end());
    std::sort(scan.native_binaries.begin(), scan.native_binaries.end());
    scan.native_binaries.erase(std::unique(scan.native_binaries.begin(), scan.native_binaries.end()), scan.native_binaries.end());
    return scan;
}

std::filesystem::path FindPythonLauncher(const std::optional<std::string>& required_version) {
    if (required_version.has_value()) {
        const auto launcher = FindExecutableOnPath({"py.exe"});
        if (!launcher.empty()) return launcher;
    }
    return FindExecutableOnPath({"python.exe", "py.exe", "python"});
}

std::string BuildPythonCommand(const std::filesystem::path& python,
                               const std::optional<std::string>& required_version) {
    const auto executable = ToLowerAscii(python.filename().string());
    if (executable == "py.exe") {
        return QuoteForCmd(PathToUtf8(python)) + (required_version.has_value() ? " -" + *required_version : " -3");
    }
    return QuoteForCmd(PathToUtf8(python));
}

std::filesystem::path FindMbinkRuntimeLibrary(const std::filesystem::path& project_root) {
    const auto exe_dir = GetCurrentExecutablePath().parent_path();
    const std::vector<std::filesystem::path> candidates = {
        exe_dir / "mbink.dll",
        project_root / "rust_host" / "vendor" / "mbink-sys" / "runtime" / "mbink.dll",
        project_root / ".mbink" / "mbink-go" / "mbink.dll",
        project_root / "vendor" / "mbink-go" / "mbink.dll",
        project_root / "vendor" / "mbink" / "bin" / "mbink.dll"
    };
    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !std::filesystem::is_directory(candidate, ec)) {
            return std::filesystem::absolute(candidate, ec).lexically_normal();
        }
    }
    return {};
}

nlohmann::json BuildStepFromProcess(const std::string& name,
                                    const std::string& command,
                                    const std::filesystem::path& cwd,
                                    int timeout_ms,
                                    const std::filesystem::path& log_path,
                                    std::string* error) {
    const auto started_iso = CurrentTimestampIso8601();
    ProcessRunResult run;
    std::string start_error;
    if (!RunCapturedCommand(command, cwd, timeout_ms, log_path, &run, &start_error)) {
        if (error) *error = start_error;
        return nlohmann::json{{"ok", false},
                              {"name", name},
                              {"started_at", started_iso},
                              {"finished_at", CurrentTimestampIso8601()},
                              {"duration_ms", 0},
                              {"exit_code", 1},
                              {"errors", nlohmann::json::array({{{"message", start_error}}})},
                              {"raw_output", nlohmann::json::array()},
                              {"log", PathToUtf8(log_path)}};
    }

    auto lines = SplitProcessLines(run.raw_output);
    nlohmann::json errors = nlohmann::json::array();
    nlohmann::json warnings = nlohmann::json::array();
    for (const auto& item : lines) {
        const auto lower = ToLowerAscii(item);
        if (lower.find("warning") != std::string::npos) warnings.push_back({{"message", item}});
        if (lower.find("error") != std::string::npos || lower.find("failed") != std::string::npos) errors.push_back({{"message", item}});
    }
    if (run.exit_code != 0 && errors.empty()) {
        errors.push_back({{"message", name + " failed with exit code " + std::to_string(run.exit_code)}});
    }
    if (run.timed_out) {
        errors.push_back({{"message", name + " timed out"}});
    }
    if (run.exit_code != 0 && error) {
        *error = errors.empty() ? name + " failed" : errors.front().value("message", name + " failed");
    }
    return nlohmann::json{{"ok", run.exit_code == 0},
                          {"name", name},
                          {"started_at", started_iso},
                          {"finished_at", CurrentTimestampIso8601()},
                          {"duration_ms", run.duration_ms},
                          {"exit_code", run.exit_code},
                          {"timed_out", run.timed_out},
                          {"warnings", warnings},
                          {"errors", errors},
                          {"raw_output", lines},
                          {"log", PathToUtf8(log_path)}};
}

bool CopyRuntimeDllToFinal(const std::filesystem::path& project_root,
                           const std::filesystem::path& final_dir,
                           nlohmann::json* outputs,
                           std::string* error) {
    const auto runtime = FindMbinkRuntimeLibrary(project_root);
    if (runtime.empty()) {
        if (error) *error = "mbink.dll not found for final artifact";
        return false;
    }
    const auto target = final_dir / runtime.filename();
    if (!CopyFileOverwrite(runtime, target, error)) return false;
    if (outputs) outputs->push_back(RelativePathToUtf8(target, project_root));
    return true;
}

bool PrepareEmbeddedResourceFile(const std::filesystem::path& resource_package,
                                 const std::filesystem::path& project_root,
                                 const std::filesystem::path& target,
                                 nlohmann::json* outputs,
                                 std::string* error) {
    if (resource_package.empty() || !std::filesystem::exists(resource_package)) {
        if (error) *error = "resource package is missing";
        return false;
    }
    if (!CopyFileOverwrite(resource_package, target, error)) return false;
    if (outputs) outputs->push_back(RelativePathToUtf8(target, project_root));
    return true;
}

nlohmann::json BuildRustHostArtifact(const std::filesystem::path& project_root,
                                     const ProjectConfig& config,
                                     const std::filesystem::path& resource_package,
                                     std::string* error) {
    const auto manifest = project_root / "rust_host" / "Cargo.toml";
    if (!std::filesystem::exists(manifest)) {
        if (error) *error = "rust_host/Cargo.toml not found";
        return nlohmann::json{{"ok", false}, {"runtime", "rust"}, {"error", error ? *error : ""}};
    }
    const auto cargo = FindExecutableOnPath({"cargo.exe", "cargo"});
    if (cargo.empty()) {
        if (error) *error = "cargo not found in PATH";
        return nlohmann::json{{"ok", false}, {"runtime", "rust"}, {"error", error ? *error : ""}};
    }

    const auto out_dir = std::filesystem::absolute(JoinProjectPath(project_root, config.out_dir)).lexically_normal();
    const auto final_dir = out_dir / "final";
    nlohmann::json outputs = nlohmann::json::array();
    if (!PrepareEmbeddedResourceFile(resource_package, project_root, project_root / "rust_host" / "resources" / "app.mbrp", &outputs, error)) {
        return nlohmann::json{{"ok", false}, {"runtime", "rust"}, {"error", error ? *error : ""}};
    }

    const std::string command = QuoteForCmd(PathToUtf8(cargo)) + " build --release --manifest-path " + QuoteForCmd(PathToUtf8(manifest));
    auto step = BuildStepFromProcess("cargo build --release", command, project_root, 180000, out_dir / "cargo-build.log", error);
    nlohmann::json status{{"ok", step.value("ok", false)},
                          {"runtime", "rust"},
                          {"builder", "cargo"},
                          {"embedded_resource", RelativePathToUtf8(project_root / "rust_host" / "resources" / "app.mbrp", project_root)},
                          {"steps", nlohmann::json::array({step})},
                          {"outputs", outputs}};
    if (!step.value("ok", false)) return status;

    const std::string stem = SanitizeArtifactStem(config.name);
    const auto built_exe = project_root / "rust_host" / "target" / "release" / "mbink-template-rust.exe";
    const auto final_exe = final_dir / (stem + ".exe");
    if (!CopyFileOverwrite(built_exe, final_exe, error)) {
        status["ok"] = false;
        status["error"] = error ? *error : "failed to copy rust artifact";
        return status;
    }
    if (!SetWindowsGuiSubsystem(final_exe, config.build_hide_console, error)) {
        status["ok"] = false;
        status["error"] = error ? *error : "failed to set rust artifact subsystem";
        return status;
    }
    status["outputs"].push_back(RelativePathToUtf8(final_exe, project_root));
    if (!CopyRuntimeDllToFinal(project_root, final_dir, &status["outputs"], error)) {
        status["ok"] = false;
        status["error"] = error ? *error : "failed to copy mbink.dll";
        return status;
    }
    status["host_artifact"] = RelativePathToUtf8(final_exe, project_root);
    status["window"] = {{"borderless", config.borderless}, {"resizable", config.resizable}};
    status["windows_subsystem"] = config.build_hide_console ? "windows" : "console";
    status["hide_console"] = config.build_hide_console;
    return status;
}

nlohmann::json BuildGoHostArtifact(const std::filesystem::path& project_root,
                                   const ProjectConfig& config,
                                   const std::filesystem::path& resource_package,
                                   std::string* error) {
    const auto main_go = project_root / "host" / "main.go";
    if (!std::filesystem::exists(main_go)) {
        if (error) *error = "host/main.go not found";
        return nlohmann::json{{"ok", false}, {"runtime", "go"}, {"error", error ? *error : ""}};
    }
    const auto go = FindExecutableOnPath({"go.exe", "go"});
    if (go.empty()) {
        if (error) *error = "go not found in PATH";
        return nlohmann::json{{"ok", false}, {"runtime", "go"}, {"error", error ? *error : ""}};
    }

    const auto out_dir = std::filesystem::absolute(JoinProjectPath(project_root, config.out_dir)).lexically_normal();
    const auto final_dir = out_dir / "final";
    const std::string stem = SanitizeArtifactStem(config.name);
    const auto final_exe = final_dir / (stem + ".exe");
    nlohmann::json outputs = nlohmann::json::array();
    if (!PrepareEmbeddedResourceFile(resource_package, project_root, project_root / "host" / "resources" / "app.mbrp", &outputs, error)) {
        return nlohmann::json{{"ok", false}, {"runtime", "go"}, {"error", error ? *error : ""}};
    }

    std::error_code ec;
    std::filesystem::create_directories(final_dir, ec);
    if (ec) {
        if (error) *error = "failed to create final output directory";
        return nlohmann::json{{"ok", false}, {"runtime", "go"}, {"error", error ? *error : ""}};
    }

    const std::string ldflags = config.build_hide_console ? " -ldflags " + QuoteForCmd("-H=windowsgui") : "";
    const std::string command = "cmd.exe /d /c set CGO_ENABLED=1&& " + QuoteForCmd(PathToUtf8(go)) +
                                " build -mod=mod" + ldflags + " -o " + QuoteForCmd(PathToUtf8(final_exe)) + " ./host";
    auto step = BuildStepFromProcess("go build", command, project_root, 180000, out_dir / "go-build.log", error);
    nlohmann::json status{{"ok", step.value("ok", false)},
                          {"runtime", "go"},
                          {"builder", "go"},
                          {"embedded_resource", RelativePathToUtf8(project_root / "host" / "resources" / "app.mbrp", project_root)},
                          {"steps", nlohmann::json::array({step})},
                          {"outputs", outputs}};
    if (!step.value("ok", false)) return status;
    if (!SetWindowsGuiSubsystem(final_exe, config.build_hide_console, error)) {
        status["ok"] = false;
        status["error"] = error ? *error : "failed to set go artifact subsystem";
        return status;
    }
    status["outputs"].push_back(RelativePathToUtf8(final_exe, project_root));
    if (!CopyRuntimeDllToFinal(project_root, final_dir, &status["outputs"], error)) {
        status["ok"] = false;
        status["error"] = error ? *error : "failed to copy mbink.dll";
        return status;
    }
    status["host_artifact"] = RelativePathToUtf8(final_exe, project_root);
    status["window"] = {{"borderless", config.borderless}, {"resizable", config.resizable}};
    status["windows_subsystem"] = config.build_hide_console ? "windows" : "console";
    status["hide_console"] = config.build_hide_console;
    return status;
}

nlohmann::json BuildPythonHostArtifact(const std::filesystem::path& project_root,
                                       const ProjectConfig& config,
                                       const std::filesystem::path& resource_package,
                                       std::string* error) {
    const auto main_py = project_root / "host" / "main.py";
    if (!std::filesystem::exists(main_py)) {
        if (error) *error = "host/main.py not found";
        return nlohmann::json{{"ok", false}, {"runtime", "python"}, {"error", error ? *error : ""}};
    }
    const auto out_dir = std::filesystem::absolute(JoinProjectPath(project_root, config.out_dir)).lexically_normal();
    const auto final_dir = out_dir / "final";
    const auto resource_copy = project_root / "host" / "resources" / "app.mbrp";
    nlohmann::json outputs = nlohmann::json::array();
    if (!PrepareEmbeddedResourceFile(resource_package, project_root, resource_copy, &outputs, error)) {
        return nlohmann::json{{"ok", false}, {"runtime", "python"}, {"error", error ? *error : ""}};
    }

    const auto dll_path = project_root / "vendor" / "mbink" / "bin" / "mbink.dll";
    if (!std::filesystem::exists(dll_path)) {
        if (error) *error = "vendor/mbink/bin/mbink.dll not found";
        return nlohmann::json{{"ok", false}, {"runtime", "python"}, {"error", error ? *error : ""}};
    }

    const std::string stem = SanitizeArtifactStem(config.name);
    const auto python_search_roots = PythonImportSearchRoots(project_root, config);
    const auto python_package_scan = ScanProjectPythonPackages(project_root, config, python_search_roots);
    const auto python = FindPythonLauncher(python_package_scan.required_python_version);
    if (python.empty()) {
        if (error) *error = "python not found in PATH";
        return nlohmann::json{{"ok", false}, {"runtime", "python"}, {"error", error ? *error : ""}};
    }
    const auto python_cmd = BuildPythonCommand(python, python_package_scan.required_python_version);
    const std::string console_mode = config.build_hide_console ? " --windowed" : " --console";
    std::string pyinstaller_args =
        " -m PyInstaller --noconfirm --clean --onefile --name " + QuoteForCmd(stem) +
        console_mode +
        " --distpath " + QuoteForCmd(PathToUtf8(final_dir)) +
        " --workpath " + QuoteForCmd(PathToUtf8(out_dir / "pyinstaller-work")) +
        " --specpath " + QuoteForCmd(PathToUtf8(out_dir / "pyinstaller-spec"));
    for (const auto& search_root : python_search_roots) {
        pyinstaller_args += " --paths " + QuoteForCmd(PathToUtf8(search_root));
    }
    for (const auto& package_name : python_package_scan.collect_all) {
        pyinstaller_args += " --collect-all " + QuoteForCmd(package_name);
    }
    pyinstaller_args +=
        " --add-data " + QuoteForCmd(PathToUtf8(resource_copy) + ";resources") +
        " --add-binary " + QuoteForCmd(PathToUtf8(dll_path) + ";mbink/bin") +
        " " + QuoteForCmd(PathToUtf8(main_py));
    const std::string command = python_cmd + pyinstaller_args;

    auto step = BuildStepFromProcess("python -m PyInstaller", command, project_root, 240000, out_dir / "pyinstaller.log", error);
    const auto final_exe = final_dir / (stem + ".exe");
    nlohmann::json status{{"ok", step.value("ok", false) && std::filesystem::exists(final_exe)},
                          {"runtime", "python"},
                          {"builder", "pyinstaller"},
                          {"python_executable", PathToUtf8(python)},
                          {"python_required_version", python_package_scan.required_python_version.value_or("")},
                          {"python_import_paths", nlohmann::json::array()},
                          {"python_collect_all", python_package_scan.collect_all},
                          {"python_native_binaries", python_package_scan.native_binaries},
                          {"embedded_resource", RelativePathToUtf8(resource_copy, project_root)},
                          {"steps", nlohmann::json::array({step})},
                          {"outputs", outputs}};
    for (const auto& search_root : python_search_roots) {
        status["python_import_paths"].push_back(RelativePathToUtf8(search_root, project_root));
    }
    if (!status.value("ok", false)) {
        if (error && error->empty()) *error = "PyInstaller did not produce the expected exe";
        status["error"] = error ? *error : "PyInstaller failed";
        return status;
    }
    if (!SetWindowsGuiSubsystem(final_exe, config.build_hide_console, error)) {
        status["ok"] = false;
        status["error"] = error ? *error : "failed to set python artifact subsystem";
        return status;
    }
    status["outputs"].push_back(RelativePathToUtf8(final_exe, project_root));
    status["host_artifact"] = RelativePathToUtf8(final_exe, project_root);
    status["window"] = {{"borderless", config.borderless}, {"resizable", config.resizable}};
    status["windows_subsystem"] = config.build_hide_console ? "windows" : "console";
    status["hide_console"] = config.build_hide_console;
    return status;
}

nlohmann::json BuildHostArtifactStatus(const std::filesystem::path& project_root,
                                       const ProjectConfig& config,
                                       std::string* error) {
    const auto runtime = DetectProjectRuntime(config);
    if (runtime == "tool") {
        return nlohmann::json{{"ok", true},
                              {"runtime", runtime},
                              {"status", "skipped"},
                              {"message", "tool runtime only needs the compiled resource package"}};
    }

    const auto package = ResourcePackagePath(project_root, config);
    if (runtime == "rust") return BuildRustHostArtifact(project_root, config, package, error);
    if (runtime == "go") return BuildGoHostArtifact(project_root, config, package, error);
    if (runtime == "python") return BuildPythonHostArtifact(project_root, config, package, error);

    if (error) *error = "unsupported host runtime: " + runtime;
    return nlohmann::json{{"ok", false}, {"runtime", runtime}, {"error", error ? *error : ""}};
}

bool StartDetachedProcess(const std::filesystem::path& exe, const std::string& args,
                          const std::filesystem::path& cwd,
                          const std::filesystem::path* stdout_path,
                          const std::filesystem::path* stderr_path,
                          int* pid, std::string* error) {
    std::wstring cmd = BuildWindowsCommandLine(exe, args);
    const bool inherit_handles = stdout_path != nullptr || stderr_path != nullptr;
    SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE out_handle = INVALID_HANDLE_VALUE;
    HANDLE err_handle = INVALID_HANDLE_VALUE;
    HANDLE in_handle = INVALID_HANDLE_VALUE;
    STARTUPINFOW si{}; PROCESS_INFORMATION pi{}; si.cb = sizeof(si);
    if (stdout_path) {
        out_handle = CreateFileW(stdout_path->wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (out_handle == INVALID_HANDLE_VALUE) {
            if (error) *error = "创建 stdout 日志失败";
            return false;
        }
    }
    if (stderr_path) {
        err_handle = CreateFileW(stderr_path->wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (err_handle == INVALID_HANDLE_VALUE) {
            if (out_handle != INVALID_HANDLE_VALUE) CloseHandle(out_handle);
            if (error) *error = "创建 stderr 日志失败";
            return false;
        }
    }
    if (inherit_handles) {
        in_handle = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
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
    cmd.push_back(L'\0');
    const auto working_dir = cwd.wstring();
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, inherit_handles,
                        DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                        nullptr, working_dir.empty() ? nullptr : working_dir.c_str(), &si, &pi)) {
        if (out_handle != INVALID_HANDLE_VALUE) CloseHandle(out_handle);
        if (err_handle != INVALID_HANDLE_VALUE) CloseHandle(err_handle);
        if (in_handle != INVALID_HANDLE_VALUE) CloseHandle(in_handle);
        if (error) *error = "启动进程失败: " + PathToUtf8(exe);
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
    std::filesystem::path entry = std::filesystem::absolute(JoinProjectPath(root, state->project.entry));
    if (ShouldBuildBeforeRun(root, state->project)) {
        std::string build_error;
        auto build_status = BuildEsbuildStatus(root, state->project, &build_error);
        if (!build_status.is_object()) {
            build_status = nlohmann::json{{"ok", false},
                                          {"status", "failed"},
                                          {"errors", nlohmann::json::array({{{"message", build_error.empty() ? "JSX/TS 运行前构建失败" : build_error}}})},
                                          {"warnings", nlohmann::json::array()},
                                          {"raw_output", nlohmann::json::array()}};
        }
        state->last_build = build_status;
        if (!build_status.value("ok", false)) {
            state->runtime_pid = 0;
            state->runtime_status = "stopped";
            state->runtime_stop_reason = "build_failed";
            if (error) *error = build_error.empty() ? "JSX/TS 运行前构建失败" : build_error;
            return false;
        }
        entry = std::filesystem::absolute(JoinProjectPath(root, state->project.out_dir) / "App.js");
    }
    if (!std::filesystem::exists(entry)) {
        state->runtime_pid = 0;
        state->runtime_status = "stopped";
        state->runtime_stop_reason = "entry_missing";
        if (error) *error = "入口文件不存在: " + PathToUtf8(entry);
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
    state->runtime_epoch = GenerateRuntimeEpoch();
    state->stopping = false;
    state->runtime_status = "starting";
    state->runtime_stop_reason = "starting";

    std::string args = QuoteForCmd(PathToUtf8(entry)) + " --width " + std::to_string(state->project.width) +
                       " --height " + std::to_string(state->project.height) +
                       " --title \"" + state->project.title + "\"" +
                       " --ui-dev-snapshot-file " + QuoteForCmd(PathToUtf8(snapshot_path)) +
                       " --ui-dev-command-file " + QuoteForCmd(PathToUtf8(command_path)) +
                       " --ui-dev-response-file " + QuoteForCmd(PathToUtf8(response_path)) +
                       " --ui-dev-console-file " + QuoteForCmd(PathToUtf8(console_path)) +
                       " --ui-dev-errors-file " + QuoteForCmd(PathToUtf8(errors_path)) +
                       " --ui-dev-lifecycle-file " + QuoteForCmd(PathToUtf8(lifecycle_path)) +
                       " --ui-dev-runtime-epoch \"" + state->runtime_epoch + "\"" +
                       " --ui-dev-snapshot-max-nodes 2000" +
                       " --ui-dev-snapshot-max-depth 64";
    if (state->project.borderless) args += " --borderless";
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
    state->stopping = false;
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
            std::unique_lock<std::mutex> lock(g_daemon_mutex);
            RefreshRuntimeStateUnlocked(state);
            if (build_status.value("ok", false) && !RuntimeWasUserClosed(*state)) {
                DaemonState command_state = *state;
                lock.unlock();
                auto reload = ReloadRuntimeBundleOrRestart(root, &command_state);
                lock.lock();
                if (state->project_id == command_state.project_id &&
                    state->project_root == command_state.project_root) {
                    MergeRuntimeFields(state, command_state);
                    build_status["reload"] = std::move(reload);
                } else {
                    build_status["reload"] = ErrorResponse("state_changed", "daemon state changed while watch reload was running");
                }
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
    std::unique_lock<std::mutex> lock(g_daemon_mutex);
    ResetWatchUnlocked();
}

}  // namespace
#endif

nlohmann::json HandleConnectedPipeRequest(HANDLE pipe, DaemonState* state, std::atomic<bool>* should_exit) {
    std::string raw_request;
    std::string read_error;
    nlohmann::json response = ErrorResponse("invalid_request", "request parse failed");
    bool request_should_exit = false;
    if (ReadPipeMessage(pipe, &raw_request, &read_error)) {
        try {
            response = DispatchDaemonRequest(nlohmann::json::parse(raw_request), state, &request_should_exit);
        } catch (const std::exception& e) {
            response = ErrorResponse("internal_error", e.what());
        } catch (...) {
            response = ErrorResponse("internal_error", "unknown internal exception");
        }
    } else if (!read_error.empty()) {
        response = ErrorResponse("invalid_request", read_error);
    }
    if (request_should_exit && should_exit) should_exit->store(true);
    return response;
}

void ServeConnectedPipe(HANDLE pipe, DaemonState* state, std::atomic<bool>* should_exit) {
    const auto response = HandleConnectedPipeRequest(pipe, state, should_exit);
    const auto payload = response.dump();
    DWORD written = 0;
    WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr);
    FlushFileBuffers(pipe);
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
}

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
        g_daemon_stopping.store(true);
        {
            std::lock_guard<std::mutex> lock(g_daemon_mutex);
            state->stopping = true;
            state->running = false;
            state->runtime_status = "stopping";
            if (state->runtime_stop_reason.empty() || state->runtime_stop_reason == "running" || state->runtime_stop_reason == "starting") {
                state->runtime_stop_reason = "daemon_stopping";
            }
            std::string save_error;
            SaveState(*state, &save_error);
        }
        StopWatchThread();
        std::lock_guard<std::mutex> lock(g_daemon_mutex);
        std::string error;
        *should_exit = true;
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

    std::unique_lock<std::mutex> lock(g_daemon_mutex);
    RefreshRuntimeStateUnlocked(state);
    if (cmd == "status") return nlohmann::json{{"ok", true}, {"daemon", StateToJson(*state)}};
    if (cmd == "open") {
        std::string error;
        const auto root = std::filesystem::absolute(PathFromUtf8(request.value("project_root", state->project_root))).lexically_normal();
        const bool force = request.value("force", false);
        if (!std::filesystem::exists(root)) return ErrorResponse("project_not_found", "项目目录不存在");
        ProjectIdentity identity;
        if (!EnsureProjectIdentity(root, &identity, &error)) return ErrorResponse("project_identity_failed", error);
        const auto loaded_project = LoadProjectConfig(root, &error);
        if (!error.empty()) return ErrorResponse("project_config_error", error);
        const auto normalized_current_root = state->project_root.empty()
            ? std::filesystem::path{}
            : std::filesystem::absolute(PathFromUtf8(state->project_root)).lexically_normal();
        const auto normalized_next_root = std::filesystem::absolute(root).lexically_normal();
        const bool same_project = state->project_id == identity.project_id &&
                                  normalized_current_root == normalized_next_root;
        const bool runtime_alive = state->runtime_pid > 0 && IsProcessRunning(state->runtime_pid);
        const bool same_config = MaterialConfigEquals(state->project, loaded_project);
        if (!force && same_project && runtime_alive && same_config) {
            state->running = true;
            state->runtime_id = identity.runtime_id;
            state->runtime_status = "running";
            state->runtime_stop_reason = "running";
            state->stopping = false;
            if (!SaveState(*state, &error)) return ErrorResponse("state_write_failed", error);
            return nlohmann::json{{"ok", true},
                                  {"project", ProjectToJson(root, state->project)},
                                  {"runtime_status", "runtime_reused"},
                                  {"reused", true},
                                  {"daemon", StateToJson(*state)}};
        }
        state->running = true;
        state->project_id = identity.project_id;
        state->runtime_id = identity.runtime_id;
        state->project_root = PathToUtf8(root);
        state->project = loaded_project;
        state->last_build = nlohmann::json{{"ok", true}, {"status", "not_built"}};
        ConfigureWatchUnlocked(*state, false);
        state->watch = DefaultWatchState();
        if (!StartRuntime(root, state, &error)) return ErrorResponse("runtime_start_failed", error);
        if (!SaveState(*state, &error)) return ErrorResponse("state_write_failed", error);
        return nlohmann::json{{"ok", true}, {"project", ProjectToJson(root, state->project)}, {"runtime_status", "runtime_started"}, {"reused", false}, {"daemon", StateToJson(*state)}};
    }
    if (cmd == "info") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = PathFromUtf8(state->project_root);
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
        const auto root = PathFromUtf8(state->project_root);
        std::filesystem::path resolved_path;
        std::string error;
        if (!ResolveProjectPath(root, request.value("path", ""), &resolved_path, &error)) return ErrorResponse("invalid_args", error);
        if (!std::filesystem::exists(resolved_path)) return ErrorResponse("file_not_found", "文件不存在");
        if (std::filesystem::is_directory(resolved_path)) return ErrorResponse("invalid_args", "path 必须指向文件");
        std::string content;
        if (!ReadFileBytes(resolved_path, &content, &error)) return ErrorResponse("file_read_failed", error);
        const auto encoding = request.value("encoding", "utf8");
        if (encoding != "utf8" && encoding != "base64") return ErrorResponse("invalid_args", "encoding 仅支持 utf8 或 base64");
        return nlohmann::json{{"ok", true}, {"path", RelativePathToUtf8(resolved_path, root)}, {"encoding", encoding}, {"content", encoding == "base64" ? Base64Encode(content) : content}, {"bytes", content.size()}};
    }
    if (cmd == "write") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = PathFromUtf8(state->project_root);
        std::filesystem::path resolved_path;
        std::string error;
        if (!ResolveProjectPath(root, request.value("path", ""), &resolved_path, &error)) return ErrorResponse("invalid_args", error);
        const auto encoding = request.value("encoding", "utf8");
        if (encoding != "utf8" && encoding != "base64") return ErrorResponse("invalid_args", "encoding 仅支持 utf8 或 base64");
        if (!request.contains("content") || !request["content"].is_string()) return ErrorResponse("invalid_args", "write 缺少 content");
        std::string content = request["content"].get<std::string>();
        if (encoding == "base64" && !Base64Decode(content, &content, &error)) return ErrorResponse("invalid_args", error);
        if (!WriteFileBytes(resolved_path, content, &error)) return ErrorResponse("file_write_failed", error);
        return nlohmann::json{{"ok", true}, {"path", RelativePathToUtf8(resolved_path, root)}, {"bytes", content.size()}, {"encoding", encoding}};
    }
    if (cmd == "build") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = PathFromUtf8(state->project_root);
        if (!std::filesystem::exists(root)) return ErrorResponse("project_not_found", "项目目录不存在");
        std::string error;
        state->project = LoadProjectConfig(root, &error);
        if (!error.empty()) return ErrorResponse("config_load_failed", error);
        auto build_status = BuildEsbuildStatus(root, state->project, &error);
        if (!error.empty()) {
            state->last_build = build_status.is_object() ? build_status : nlohmann::json{{"ok", false}, {"status", "failed"}, {"error", error}};
            return build_status.is_object() ? build_status : ErrorResponse("build_failed", error);
        }
        std::string host_error;
        auto host_status = BuildHostArtifactStatus(root, state->project, &host_error);
        build_status["host_build"] = host_status;
        if (host_status.is_object() && host_status.contains("outputs") && host_status["outputs"].is_array()) {
            for (const auto& output : host_status["outputs"]) {
                if (output.is_string()) build_status["outputs"].push_back(output.get<std::string>());
            }
        }
        if (!host_status.value("ok", false)) {
            build_status["ok"] = false;
            build_status["status"] = "failed";
            build_status["errors"].push_back({{"message", host_error.empty() ? "host build failed" : host_error}});
            error = host_error.empty() ? "host build failed" : host_error;
        }
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
        if (state->stopping) return ErrorResponse("shutdown_in_progress", "runtime is shutting down");
        const auto response_mode = request.value("response_mode", request.value("mode", std::string{"auto"}));
        if (response_mode != "auto" && response_mode != "inline" && response_mode != "file") {
            return ErrorResponse("invalid_args", "snapshot response_mode must be auto, inline, or file");
        }

        const bool include_screenshot = request.value("include_screenshot", false) ||
                                        request.value("inline_screenshot", false);
        const bool inline_screenshot = request.value("inline_screenshot", false);
        const bool custom_snapshot = SnapshotRequestHasCustomOptions(request);
        DaemonState snapshot_state = *state;
        auto snapshot_path = custom_snapshot ? BuildRequestSnapshotPath(snapshot_state)
                                             : GetRuntimeSnapshotPath(snapshot_state);
        const auto screenshot_path = include_screenshot ? BuildRequestScreenshotPath(snapshot_path)
                                                        : std::filesystem::path{};
        const int runtime_pid = snapshot_state.runtime_pid;
        lock.unlock();

        if (custom_snapshot) {
            nlohmann::json runtime_cmd{{"type", "snapshot"}, {"snapshot_file", PathToUtf8(snapshot_path)}};
            if (request.contains("max_nodes")) runtime_cmd["max_nodes"] = request["max_nodes"];
            if (request.contains("max_depth")) runtime_cmd["max_depth"] = request["max_depth"];
            if (request.contains("root_selector")) runtime_cmd["root_selector"] = request["root_selector"];
            if (include_screenshot) {
                runtime_cmd["include_screenshot"] = true;
                runtime_cmd["inline_screenshot"] = inline_screenshot;
                runtime_cmd["screenshot_file"] = PathToUtf8(screenshot_path);
            }
            std::string error;
            nlohmann::json resp;
            if (!RequestRuntimeUiCommand(snapshot_state, runtime_pid, std::move(runtime_cmd), &resp, &error)) {
                RemoveFileIfExists(snapshot_path);
                if (include_screenshot) RemoveFileIfExists(screenshot_path);
                return RuntimeCommandErrorResponse("snapshot_failed", error);
            }
            if (!resp.value("ok", false)) {
                RemoveFileIfExists(snapshot_path);
                if (include_screenshot) RemoveFileIfExists(screenshot_path);
                return ErrorResponse("snapshot_failed", ExtractRuntimeCommandMessage(resp, "snapshot export failed"));
            }
        }

        auto snapshot = SnapshotFileLooksComplete(snapshot_path) ? ReadJsonFile(snapshot_path) : nlohmann::json();
        bool snapshot_ready = SnapshotMatchesRuntimeEpoch(snapshot, snapshot_state);
        if (snapshot_ready && include_screenshot) snapshot_ready = SnapshotScreenshotLooksComplete(snapshot);
        bool stale_snapshot = snapshot.is_object() && snapshot.value("ok", false) && !snapshot_ready;
        if (!snapshot_ready &&
            !custom_snapshot &&
            runtime_pid > 0 && IsProcessRunning(runtime_pid)) {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
            while (std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                snapshot = SnapshotFileLooksComplete(snapshot_path) ? ReadJsonFile(snapshot_path) : nlohmann::json();
                snapshot_ready = SnapshotMatchesRuntimeEpoch(snapshot, snapshot_state);
                if (snapshot_ready && include_screenshot) snapshot_ready = SnapshotScreenshotLooksComplete(snapshot);
                stale_snapshot = snapshot.is_object() && snapshot.value("ok", false) && !snapshot_ready;
                if (snapshot_ready) break;
            }
        }
        const auto snapshot_bytes = snapshot_ready ? FileSizeOrZero(snapshot_path) : uintmax_t{0};
        if (snapshot_ready && snapshot.is_object() && snapshot.value("ok", false)) {
            lock.lock();
            const bool current_snapshot = !state->stopping &&
                                          state->project_id == snapshot_state.project_id &&
                                          state->project_root == snapshot_state.project_root &&
                                          state->runtime_epoch == snapshot_state.runtime_epoch;
            lock.unlock();
            if (!current_snapshot) return ErrorResponse("snapshot_stale", "snapshot epoch does not match current runtime");
            return BuildSnapshotResponse(snapshot_state, snapshot_path, snapshot, snapshot_bytes, response_mode);
        }
        if (stale_snapshot) {
            return ErrorResponse("snapshot_stale", "snapshot epoch does not match current runtime");
        }
        return ErrorResponse("snapshot_not_ready", "fresh snapshot is not ready");
    }
    if (cmd == "logs") return std::filesystem::exists(GetRuntimeConsolePath(*state)) ? BuildStructuredRuntimeBufferResponse(GetRuntimeConsolePath(*state), "stdout", 200, "entries") : BuildStructuredLogTail(GetRuntimeStdoutLogPath(*state), "stdout", 200, "entries");
    if (cmd == "errors") return std::filesystem::exists(GetRuntimeErrorsPath(*state)) ? BuildStructuredRuntimeBufferResponse(GetRuntimeErrorsPath(*state), "stderr", 200, "errors") : BuildStructuredLogTail(GetRuntimeStderrLogPath(*state), "stderr", 200, "errors");
    if (cmd == "eval") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        if (state->runtime_pid <= 0 || !IsProcessRunning(state->runtime_pid)) return ErrorResponse("runtime_not_running", "runtime 未运行");
        if (state->stopping) return ErrorResponse("shutdown_in_progress", "runtime is shutting down");
        const auto code = request.value("code", "");
        if (code.empty()) return ErrorResponse("invalid_args", "eval 缺少 code");
        DaemonState command_state = *state;
        const int runtime_pid = state->runtime_pid;
        lock.unlock();
        std::string error;
        nlohmann::json resp;
        if (!RequestRuntimeEval(command_state, runtime_pid, code, &resp, &error)) return RuntimeCommandErrorResponse("eval_failed", error);
        resp["source"] = PathToUtf8(GetRuntimeResponsePath(command_state));
        return resp;
    }
    if (cmd == "query_element" || cmd == "inspect" || cmd == "click" || cmd == "input_text" || cmd == "scroll" || cmd == "highlight") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        if (state->runtime_pid <= 0 || !IsProcessRunning(state->runtime_pid)) return ErrorResponse("runtime_not_running", "runtime 未运行");
        if (state->stopping) return ErrorResponse("shutdown_in_progress", "runtime is shutting down");
        const auto selector = request.value("selector", std::string{});
        if (selector.empty()) return ErrorResponse("invalid_args", cmd + " 缺少 selector");
        nlohmann::json runtime_cmd{{"type", cmd}, {"selector", selector}};
        if (request.contains("limit")) runtime_cmd["limit"] = request["limit"];
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
        DaemonState command_state = *state;
        const int runtime_pid = state->runtime_pid;
        lock.unlock();
        std::string error;
        nlohmann::json resp;
        if (!RequestRuntimeUiCommand(command_state, runtime_pid, std::move(runtime_cmd), &resp, &error)) {
            return RuntimeCommandErrorResponse(cmd + "_failed", error);
        }
        resp["source"] = PathToUtf8(GetRuntimeResponsePath(command_state));
        return resp;
    }
    if (cmd == "reload") {
#ifdef _WIN32
        std::string error;
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = PathFromUtf8(state->project_root);
        DaemonState command_state = *state;
        lock.unlock();
        auto reload = ReloadRuntimeBundleOrRestart(root, &command_state);
        lock.lock();
        if (state->project_id != command_state.project_id ||
            state->project_root != command_state.project_root) {
            return ErrorResponse("state_changed", "daemon state changed while reload was running");
        }
        MergeRuntimeFields(state, command_state);
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
    g_daemon_stopping.store(false);
    state.running = true;
    state.pid = CurrentProcessId();
    state.started_at = CurrentTimestampIso8601();
    state.last_build = nlohmann::json{{"ok", true}, {"status", "not_built"}};
    state.watch = DefaultWatchState();

    std::filesystem::path project_root;
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == "--project") project_root = std::filesystem::absolute(PathFromUtf8(args[i + 1])).lexically_normal();
        if (args[i] == "--project-id") state.project_id = args[i + 1];
    }
    if (!project_root.empty()) {
        ProjectIdentity identity;
        if (!EnsureProjectIdentity(project_root, &identity, &error)) return 1;
        state.project_id = identity.project_id;
        state.runtime_id = identity.runtime_id;
        state.project_root = PathToUtf8(project_root);
        state.project = LoadProjectConfig(project_root, &error);
    }
    if (state.runtime_id.empty()) state.runtime_id = state.project_id;
    if (!SaveState(state, &error)) return 1;

    const auto pipe_name = GetDaemonPipeName(state.project_id);
    std::atomic<bool> should_exit{false};
    while (!should_exit.load()) {
        const auto wide_pipe_name = Utf8ToWide(pipe_name);
        HANDLE pipe = CreateNamedPipeW(wide_pipe_name.c_str(),
                                       PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                       PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                       PIPE_UNLIMITED_INSTANCES,
                                       65536,
                                       65536,
                                       0,
                                       nullptr);
        if (pipe == INVALID_HANDLE_VALUE) return 1;
        OVERLAPPED connect_overlapped{};
        connect_overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!connect_overlapped.hEvent) { CloseHandle(pipe); return 1; }
        BOOL connected = ConnectNamedPipe(pipe, &connect_overlapped);
        DWORD connect_error = connected ? ERROR_SUCCESS : GetLastError();
        if (!connected && connect_error == ERROR_IO_PENDING) {
            while (!should_exit.load()) {
                const DWORD wait_result = WaitForSingleObject(connect_overlapped.hEvent, 100);
                if (wait_result == WAIT_OBJECT_0) break;
                if (wait_result != WAIT_TIMEOUT) break;
            }
            if (should_exit.load()) {
                CancelIoEx(pipe, &connect_overlapped);
                CloseHandle(connect_overlapped.hEvent);
                CloseHandle(pipe);
                break;
            }
            DWORD transferred = 0;
            connected = GetOverlappedResult(pipe, &connect_overlapped, &transferred, FALSE);
            connect_error = connected ? ERROR_SUCCESS : GetLastError();
        }
        CloseHandle(connect_overlapped.hEvent);
        if (!connected && connect_error != ERROR_PIPE_CONNECTED) { CloseHandle(pipe); continue; }
        std::thread(ServeConnectedPipe, pipe, &state, &should_exit).detach();
        continue;
    }
    StopWatchThread();
    return 0;
#else
    (void)args; return 1;
#endif
}

}  // namespace mbink::ui_dev
