#include "daemon_server.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <deque>
#include <fstream>
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

std::filesystem::path GetRuntimeStdoutLogPath() {
    return std::filesystem::temp_directory_path() / "mbink-ui-dev.runtime.stdout.log";
}

std::filesystem::path GetRuntimeStderrLogPath() {
    return std::filesystem::temp_directory_path() / "mbink-ui-dev.runtime.stderr.log";
}

std::filesystem::path GetRuntimeSnapshotPath() {
    return std::filesystem::temp_directory_path() / "mbink-ui-dev.runtime.snapshot.json";
}

std::filesystem::path GetRuntimeCommandPath() {
    return std::filesystem::temp_directory_path() / "mbink-ui-dev.runtime.command.json";
}

std::filesystem::path GetRuntimeResponsePath() {
    return std::filesystem::temp_directory_path() / "mbink-ui-dev.runtime.response.json";
}

std::filesystem::path GetRuntimeConsolePath() {
    return std::filesystem::temp_directory_path() / "mbink-ui-dev.runtime.console.json";
}

std::filesystem::path GetRuntimeErrorsPath() {
    return std::filesystem::temp_directory_path() / "mbink-ui-dev.runtime.errors.json";
}

std::filesystem::path GetProjectBuildLogPath(const std::filesystem::path& project_root, const ProjectConfig& config) {
    return std::filesystem::absolute(project_root / config.out_dir / "build.log");
}

void RemoveFileIfExists(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
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


bool RequestRuntimeEval(int runtime_pid, const std::string& code, nlohmann::json* response, std::string* error) {
    const auto command_path = GetRuntimeCommandPath();
    const auto response_path = GetRuntimeResponsePath();
    const auto request_id = "eval-" + std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    RemoveFileIfExists(command_path);
    RemoveFileIfExists(response_path);

    if (!WriteJsonFile(command_path, nlohmann::json{{"id", request_id}, {"type", "eval"}, {"code", code}})) {
        if (error) *error = "写入 runtime eval 请求失败";
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!IsProcessRunning(runtime_pid)) {
            if (error) *error = "runtime 已退出，eval 中断";
            return false;
        }

        auto resp = ReadJsonFile(response_path);
        if (resp.is_object() && resp.value("id", std::string()) == request_id) {
            if (response) *response = resp;
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (error) *error = "等待 runtime eval 响应超时";
    return false;
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
    const auto entry_path = std::filesystem::absolute(project_root / config.entry).lexically_normal();
    if (entry_path.extension() == ".js" || entry_path.extension() == ".mjs" || entry_path.extension() == ".jsx" ||
        entry_path.extension() == ".ts" || entry_path.extension() == ".tsx") {
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
    const auto entry = std::filesystem::absolute(root / state->project.entry);
    if (!std::filesystem::exists(entry)) {
        if (error) *error = "入口文件不存在: " + entry.string();
        return false;
    }
    const auto stdout_path = GetRuntimeStdoutLogPath();
    const auto stderr_path = GetRuntimeStderrLogPath();
    const auto snapshot_path = GetRuntimeSnapshotPath();
    const auto command_path = GetRuntimeCommandPath();
    const auto response_path = GetRuntimeResponsePath();
    const auto console_path = GetRuntimeConsolePath();
    const auto errors_path = GetRuntimeErrorsPath();

    // 清理旧文件，避免串扰
    RemoveFileIfExists(snapshot_path);
    RemoveFileIfExists(command_path);
    RemoveFileIfExists(response_path);
    RemoveFileIfExists(console_path);
    RemoveFileIfExists(errors_path);

    std::string args = "\"" + entry.string() + "\" --width " + std::to_string(state->project.width) +
                       " --height " + std::to_string(state->project.height) +
                       " --title \"" + state->project.title + "\"" +
                       " --ui-dev-snapshot-file \"" + snapshot_path.string() + "\"" +
                       " --ui-dev-command-file \"" + command_path.string() + "\"" +
                       " --ui-dev-response-file \"" + response_path.string() + "\"" +
                       " --ui-dev-console-file \"" + console_path.string() + "\"" +
                       " --ui-dev-errors-file \"" + errors_path.string() + "\"";
    if (!StartDetachedProcess(runtime_exe, args, root, &stdout_path, &stderr_path, &state->runtime_pid, error)) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (!IsProcessRunning(state->runtime_pid)) {
        if (error) *error = "esm_loader 进程启动后立即退出";
        state->runtime_pid = 0;
        return false;
    }
    return true;
}

}  // namespace
#endif

bool StartDetachedDaemon(const std::filesystem::path& executable_path,
                         const std::string& project_root,
                         int* spawned_pid,
                         std::string* error) {
#ifdef _WIN32
    std::string args = "daemon run";
    if (!project_root.empty()) args += " --project \"" + project_root + "\"";
    return StartDetachedProcess(executable_path, args, executable_path.parent_path(), nullptr, nullptr, spawned_pid, error);
#else
    (void)executable_path; (void)project_root; (void)spawned_pid;
    if (error) *error = "当前平台未实现 daemon";
    return false;
#endif
}

nlohmann::json DispatchDaemonRequest(const nlohmann::json& request, DaemonState* state, bool* should_exit) {
    const auto cmd = request.value("cmd", "");
    if (cmd == "ping") return OkResponse();
    if (cmd == "status") return nlohmann::json{{"ok", true}, {"daemon", StateToJson(*state)}};
    if (cmd == "open") {
        std::string error; const auto root = std::filesystem::absolute(request.value("project_root", state->project_root));
        if (!std::filesystem::exists(root)) return ErrorResponse("project_not_found", "项目目录不存在");
        state->running = true; state->project_root = root.string(); state->project = LoadProjectConfig(root, &error);
        if (!error.empty()) return ErrorResponse("project_config_error", error);
        state->last_build = nlohmann::json{{"ok", true}, {"status", "not_built"}};
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
                              {"runtime_status", state->runtime_pid > 0 && IsProcessRunning(state->runtime_pid) ? "running" : "stopped"},
                              {"build_status", state->last_build.is_null() ? nlohmann::json{{"ok", true}, {"status", "not_built"}} : state->last_build},
                              {"file_tree", BuildProjectFileTree(root, 2)},
                              {"daemon", StateToJson(*state)}};
    }
    if (cmd == "read") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = std::filesystem::path(state->project_root);
        std::filesystem::path resolved_path;
        std::string error;
        if (!ResolveProjectPath(root, request.value("path", ""), &resolved_path, &error)) {
            return ErrorResponse("invalid_args", error);
        }
        if (!std::filesystem::exists(resolved_path)) return ErrorResponse("file_not_found", "文件不存在");
        if (std::filesystem::is_directory(resolved_path)) return ErrorResponse("invalid_args", "path 必须指向文件");
        std::string content;
        if (!ReadFileBytes(resolved_path, &content, &error)) return ErrorResponse("file_read_failed", error);
        const auto encoding = request.value("encoding", "utf8");
        if (encoding != "utf8" && encoding != "base64") {
            return ErrorResponse("invalid_args", "encoding 仅支持 utf8 或 base64");
        }
        return nlohmann::json{{"ok", true},
                              {"path", std::filesystem::relative(resolved_path, root).generic_string()},
                              {"encoding", encoding},
                              {"content", encoding == "base64" ? Base64Encode(content) : content},
                              {"bytes", content.size()}};
    }
    if (cmd == "write") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = std::filesystem::path(state->project_root);
        std::filesystem::path resolved_path;
        std::string error;
        if (!ResolveProjectPath(root, request.value("path", ""), &resolved_path, &error)) {
            return ErrorResponse("invalid_args", error);
        }
        const auto encoding = request.value("encoding", "utf8");
        if (encoding != "utf8" && encoding != "base64") {
            return ErrorResponse("invalid_args", "encoding 仅支持 utf8 或 base64");
        }
        if (!request.contains("content") || !request["content"].is_string()) {
            return ErrorResponse("invalid_args", "write 缺少 content");
        }
        std::string content = request["content"].get<std::string>();
        if (encoding == "base64" && !Base64Decode(content, &content, &error)) {
            return ErrorResponse("invalid_args", error);
        }
        if (!WriteFileBytes(resolved_path, content, &error)) {
            return ErrorResponse("file_write_failed", error);
        }
        return nlohmann::json{{"ok", true},
                              {"path", std::filesystem::relative(resolved_path, root).generic_string()},
                              {"bytes", content.size()},
                              {"encoding", encoding}};
    }
    if (cmd == "build") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        const auto root = std::filesystem::path(state->project_root);
        if (!std::filesystem::exists(root)) return ErrorResponse("project_not_found", "项目目录不存在");
        std::string error;
        auto build_status = BuildEsbuildStatus(root, state->project, &error);
        if (!error.empty()) return ErrorResponse("build_failed", error);
        state->last_build = build_status;
        if (!SaveState(*state, &error)) return ErrorResponse("state_write_failed", error);
        return build_status;
    }
    if (cmd == "build_status") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        return state->last_build.is_null()
                 ? nlohmann::json{{"ok", true}, {"status", "not_built"}}
                 : state->last_build;
    }
    if (cmd == "snapshot") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        auto snapshot = ReadJsonFile(GetRuntimeSnapshotPath());
        if (snapshot.is_object() && snapshot.value("ok", false)) {
            snapshot["timestamp"] = CurrentTimestampIso8601();
            snapshot["source"] = GetRuntimeSnapshotPath().string();
            return snapshot;
        }
        return nlohmann::json{{"ok", true},
                              {"timestamp", CurrentTimestampIso8601()},
                              {"viewport", {{"width", state->project.width}, {"height", state->project.height}, {"dpr", 1.0}}},
                              {"screenshot_base64", ""},
                              {"tree", {{"node_id", "stub-root"}, {"tag", "body"}, {"text", nullptr}, {"rect", {{"x", 0}, {"y", 0}, {"w", state->project.width}, {"h", state->project.height}}}, {"attrs", nlohmann::json::object()}, {"interactive", false}, {"visible", true}, {"scroll", {{"x", 0}, {"y", 0}, {"max_x", 0}, {"max_y", 0}}}, {"children", nlohmann::json::array()}}},
                              {"note", "P0 stub: snapshot 文件尚未就绪"}};
    }
    if (cmd == "logs") {
        const auto structured_path = GetRuntimeConsolePath();
        auto structured = BuildStructuredRuntimeBufferResponse(structured_path, "stdout", 200, "entries");
        if (structured.value("count", 0) > 0 || std::filesystem::exists(structured_path)) return structured;
        return BuildStructuredLogTail(GetRuntimeStdoutLogPath(), "stdout", 200, "entries");
    }
    if (cmd == "errors") {
        const auto structured_path = GetRuntimeErrorsPath();
        auto structured = BuildStructuredRuntimeBufferResponse(structured_path, "stderr", 200, "errors");
        if (structured.value("count", 0) > 0 || std::filesystem::exists(structured_path)) return structured;
        return BuildStructuredLogTail(GetRuntimeStderrLogPath(), "stderr", 200, "errors");
    }
    if (cmd == "eval") {
        if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        if (state->runtime_pid <= 0 || !IsProcessRunning(state->runtime_pid)) {
            return ErrorResponse("runtime_not_running", "runtime 未运行");
        }
        const auto code = request.value("code", "");
        if (code.empty()) return ErrorResponse("invalid_args", "eval 缺少 code");
        std::string error;
        nlohmann::json resp;
        if (!RequestRuntimeEval(state->runtime_pid, code, &resp, &error)) {
            return ErrorResponse("eval_failed", error);
        }
        resp["source"] = GetRuntimeResponsePath().string();
        return resp;
    }
    if (cmd == "reload") {
#ifdef _WIN32
        std::string error; if (state->project_root.empty()) return ErrorResponse("project_not_open", "请先执行 open");
        if (!StartRuntime(std::filesystem::path(state->project_root), state, &error)) return ErrorResponse("runtime_reload_failed", error);
        if (!SaveState(*state, &error)) return ErrorResponse("state_write_failed", error);
        return nlohmann::json{{"ok", true}, {"mode", "restart_runtime"}, {"daemon", StateToJson(*state)}};
#else
        return ErrorResponse("unsupported", "当前平台未实现 runtime reload");
#endif
    }
    if (cmd == "stop") {
        std::string error; *should_exit = true; state->running = false;
#ifdef _WIN32
        if (state->runtime_pid > 0 && IsProcessRunning(state->runtime_pid)) StopProcess(state->runtime_pid);
#endif
        state->runtime_pid = 0;
        RemoveFileIfExists(GetRuntimeSnapshotPath());
        RemoveFileIfExists(GetRuntimeCommandPath());
        RemoveFileIfExists(GetRuntimeResponsePath());
        RemoveFileIfExists(GetRuntimeConsolePath());
        RemoveFileIfExists(GetRuntimeErrorsPath());
        RemoveState(&error);
        return nlohmann::json{{"ok", true}, {"stopped", true}};
    }
    return ErrorResponse("invalid_args", "未知 daemon 请求");
}

int RunDaemonServer(const std::vector<std::string>& args) {
#ifdef _WIN32
    std::string error; DaemonState state{true, CurrentProcessId(), 0, CurrentTimestampIso8601(), "", {}};
    for (size_t i = 0; i + 1 < args.size(); ++i) if (args[i] == "--project") state.project_root = args[i + 1];
    if (!state.project_root.empty()) state.project = LoadProjectConfig(state.project_root, &error);
    if (!SaveState(state, &error)) return 1;
    bool should_exit = false;
    while (!should_exit) {
        HANDLE pipe = CreateNamedPipeA(kDaemonPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 65536, 65536, 0, nullptr);
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
    return 0;
#else
    (void)args; return 1;
#endif
}

}  // namespace mbink::ui_dev
