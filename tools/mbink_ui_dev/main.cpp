#include "common.h"
#include "daemon_ipc.h"
#include "daemon_server.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using mbink::ui_dev::DaemonState;
using mbink::ui_dev::ErrorResponse;
using mbink::ui_dev::IsProcessRunning;
using mbink::ui_dev::LoadState;
using mbink::ui_dev::RemoveState;
using mbink::ui_dev::RunDaemonServer;
using mbink::ui_dev::SendDaemonRequest;
using mbink::ui_dev::StartDetachedDaemon;
using mbink::ui_dev::WaitForDaemonReady;
using mbink::ui_dev::kDaemonPipeName;

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

std::string GetOptionValue(const std::vector<std::string>& args, const std::string& key) {
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == key) return args[i + 1];
    }
    return "";
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

bool EnsureDaemonRunning(const std::string& project_root, std::string* error) {
    std::string load_error;
    const auto state = LoadState(&load_error);
    if (state.has_value() && state->running && state->pid > 0 && IsProcessRunning(state->pid)) return true;
    if (state.has_value()) {
        std::string rm_err;
        RemoveState(&rm_err);
    }

    int pid = 0;
    if (!StartDetachedDaemon(GetExecutablePath(), project_root, &pid, error)) return false;
    return WaitForDaemonReady(kDaemonPipeName, kDaemonStartupTimeoutMs, error);
}

nlohmann::json CallDaemon(const nlohmann::json& req) {
    std::string error;
    nlohmann::json resp;
    if (!SendDaemonRequest(kDaemonPipeName, req, &resp, &error, kDaemonRequestTimeoutMs)) {
        return ErrorResponse("ipc_failed", error);
    }
    return resp;
}

nlohmann::json CallDaemonWithTimeout(const nlohmann::json& req, int timeout_ms) {
    std::string error;
    nlohmann::json resp;
    if (!SendDaemonRequest(kDaemonPipeName, req, &resp, &error, timeout_ms)) {
        return ErrorResponse("ipc_failed", error);
    }
    return resp;
}

}  // namespace

int main(int argc, char** argv) {
    ConfigureConsoleForUtf8();

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    if (args.empty()) {
        PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev <daemon|open|info|read|write|build|build-status|snapshot|logs|errors|eval <code>|reload> ..."));
        return 1;
    }

    const auto& cmd = args[0];
    if (cmd == "daemon") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev daemon <start|stop|status|run>"));
            return 1;
        }
        const auto action = args[1];
        if (action == "run") return RunDaemonServer(args);
        if (action == "start") {
            std::string err;
            const auto project = GetOptionValue(args, "--project");
            if (!EnsureDaemonRunning(project, &err)) {
                PrintJson(ErrorResponse("daemon_start_failed", err));
                return 1;
            }
            PrintJson(CallDaemon(nlohmann::json{{"cmd", "status"}}));
            return 0;
        }
        if (action == "stop") {
            PrintJson(CallDaemon(nlohmann::json{{"cmd", "stop"}}));
            return 0;
        }
        if (action == "status") {
            const auto state = LoadState(nullptr);
            if (state.has_value() && state->running && state->pid > 0 && IsProcessRunning(state->pid)) {
                PrintJson(CallDaemon(nlohmann::json{{"cmd", "status"}}));
            } else {
                PrintJson(nlohmann::json{{"ok", true}, {"daemon", nlohmann::json{{"running", false}}}});
            }
            return 0;
        }
        PrintJson(ErrorResponse("invalid_args", "未知 daemon 子命令；`eval` 应直接使用 `mbink-ui-dev eval <code>`"));
        return 1;
    }

    if (cmd == "open") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev open <project-path>"));
            return 1;
        }
        const auto root = std::filesystem::absolute(args[1]);
        if (!std::filesystem::exists(root)) {
            PrintJson(ErrorResponse("project_not_found", "项目目录不存在"));
            return 1;
        }
        std::string err;
        if (!EnsureDaemonRunning(root.string(), &err)) {
            PrintJson(ErrorResponse("daemon_start_failed", err));
            return 1;
        }
        PrintJson(CallDaemon(nlohmann::json{{"cmd", "open"}, {"project_root", root.string()}}));
        return 0;
    }

    // 其余命令：要求 daemon 已运行
    {
        const auto state = LoadState(nullptr);
        if (!state.has_value() || !state->running || state->pid <= 0 || !IsProcessRunning(state->pid)) {
            PrintJson(ErrorResponse("daemon_not_running", "请先执行 daemon start 或 open"));
            return 1;
        }
    }

    if (cmd == "snapshot") PrintJson(CallDaemon(nlohmann::json{{"cmd", "snapshot"}}));
    else if (cmd == "info") PrintJson(CallDaemon(nlohmann::json{{"cmd", "info"}}));
    else if (cmd == "read") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev read <path> [--encoding utf8|base64]"));
            return 1;
        }
        const auto encoding = GetOptionValue(args, "--encoding");
        nlohmann::json req{{"cmd", "read"}, {"path", args[1]}};
        if (!encoding.empty()) req["encoding"] = encoding;
        PrintJson(CallDaemon(req));
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
        PrintJson(CallDaemon(req));
    }
    else if (cmd == "build") PrintJson(CallDaemonWithTimeout(nlohmann::json{{"cmd", "build"}}, kBuildRequestTimeoutMs));
    else if (cmd == "build-status") PrintJson(CallDaemon(nlohmann::json{{"cmd", "build_status"}}));
    else if (cmd == "logs") PrintJson(CallDaemon(nlohmann::json{{"cmd", "logs"}}));
    else if (cmd == "errors") PrintJson(CallDaemon(nlohmann::json{{"cmd", "errors"}}));
    else if (cmd == "eval") {
        if (args.size() < 2) {
            PrintJson(ErrorResponse("invalid_args", "用法: mbink-ui-dev eval <code>"));
            return 1;
        }
        std::string code = args[1];
        for (size_t i = 2; i < args.size(); ++i) code += " " + args[i];
        PrintJson(CallDaemon(nlohmann::json{{"cmd", "eval"}, {"code", code}}));
    }
    else if (cmd == "reload") PrintJson(CallDaemon(nlohmann::json{{"cmd", "reload"}}));
    else {
        PrintJson(ErrorResponse("invalid_args", "未知命令"));
        return 1;
    }

    return 0;
}
