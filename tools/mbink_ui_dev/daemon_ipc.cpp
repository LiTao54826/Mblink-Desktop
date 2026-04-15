#include "daemon_ipc.h"

#include <chrono>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mbink::ui_dev {

#ifdef _WIN32
namespace {

bool ReadPipeMessage(HANDLE pipe, std::string* out, std::string* error) {
    std::vector<char> buffer(4096);
    out->clear();

    for (;;) {
        DWORD read = 0;
        const BOOL ok = ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr);
        if (read > 0) out->append(buffer.data(), buffer.data() + read);
        if (ok) return true;
        if (GetLastError() != ERROR_MORE_DATA) {
            if (error) *error = "读取 daemon 响应失败";
            return false;
        }
    }
}

}  // namespace
#endif

bool SendDaemonRequest(const std::string& pipe_name,
                       const nlohmann::json& request,
                       nlohmann::json* response,
                       std::string* error,
                       int timeout_ms) {
#ifdef _WIN32
    if (!WaitNamedPipeA(pipe_name.c_str(), timeout_ms)) {
        if (error) *error = "等待 daemon IPC 超时";
        return false;
    }

    HANDLE pipe = CreateFileA(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                              OPEN_EXISTING, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
        if (error) *error = "连接 daemon IPC 失败";
        return false;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr);

    const auto payload = request.dump();
    DWORD written = 0;
    if (!WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr)) {
        CloseHandle(pipe);
        if (error) *error = "发送 daemon 请求失败";
        return false;
    }

    std::string raw_response;
    const bool ok = ReadPipeMessage(pipe, &raw_response, error);
    CloseHandle(pipe);
    if (!ok) return false;

    try {
        *response = nlohmann::json::parse(raw_response);
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
#else
    (void)pipe_name;
    (void)request;
    (void)response;
    (void)error;
    (void)timeout_ms;
    return false;
#endif
}

bool WaitForDaemonReady(const std::string& pipe_name, int timeout_ms, std::string* error) {
#ifdef _WIN32
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        nlohmann::json response;
        if (SendDaemonRequest(pipe_name, nlohmann::json{{"cmd", "ping"}}, &response, nullptr, 200)) {
            return response.value("ok", false);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (error) *error = "daemon 启动超时";
    return false;
#else
    (void)pipe_name;
    (void)timeout_ms;
    if (error) *error = "当前平台未实现 daemon IPC";
    return false;
#endif
}

}  // namespace mbink::ui_dev
