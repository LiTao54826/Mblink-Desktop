#include "daemon_ipc.h"
#include "common.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mbink::ui_dev {

#ifdef _WIN32
namespace {

bool ReadPipeMessage(HANDLE pipe, std::string* out, std::string* error, int timeout_ms) {
    std::vector<char> buffer(4096);
    out->clear();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    for (;;) {
        DWORD read = 0;
        OVERLAPPED overlapped{};
        overlapped.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (!overlapped.hEvent) {
            if (error) *error = "read daemon response failed";
            return false;
        }

        BOOL ok = ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size()), &read, &overlapped);
        DWORD read_error = ok ? ERROR_SUCCESS : GetLastError();
        if (!ok && read_error == ERROR_IO_PENDING) {
            const auto now = std::chrono::steady_clock::now();
            const auto remaining_ms = now >= deadline
                ? 0
                : static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count());
            const DWORD wait_result = WaitForSingleObject(overlapped.hEvent, remaining_ms);
            if (wait_result == WAIT_TIMEOUT) {
                CancelIoEx(pipe, &overlapped);
                CloseHandle(overlapped.hEvent);
                if (error) *error = "daemon_response_timeout";
                return false;
            }
            if (wait_result != WAIT_OBJECT_0) {
                CancelIoEx(pipe, &overlapped);
                CloseHandle(overlapped.hEvent);
                if (error) *error = "read daemon response failed";
                return false;
            }
            ok = GetOverlappedResult(pipe, &overlapped, &read, FALSE);
            read_error = ok ? ERROR_SUCCESS : GetLastError();
        } else {
            DWORD transferred = 0;
            if (GetOverlappedResult(pipe, &overlapped, &transferred, FALSE)) {
                if (read == 0) read = transferred;
            } else if (read_error == ERROR_MORE_DATA && transferred > 0 && read == 0) {
                read = transferred;
            }
        }

        CloseHandle(overlapped.hEvent);
        if (read > 0) out->append(buffer.data(), buffer.data() + read);
        if (ok) return true;
        if (read_error != ERROR_MORE_DATA) {
            if (error) *error = "read daemon response failed";
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
    const auto wide_pipe_name = Utf8ToWide(pipe_name);
    if (!WaitNamedPipeW(wide_pipe_name.c_str(), timeout_ms)) {
        if (error) *error = "daemon_queue_timeout";
        return false;
    }

    HANDLE pipe = CreateFileW(wide_pipe_name.c_str(),
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              nullptr,
                              OPEN_EXISTING,
                              FILE_FLAG_OVERLAPPED,
                              nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
        if (error) *error = "connect daemon IPC failed";
        return false;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr);

    const auto payload = request.dump();
    DWORD written = 0;
    if (!WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr)) {
        CloseHandle(pipe);
        if (error) *error = "write daemon request failed";
        return false;
    }

    std::string raw_response;
    const int response_timeout_ms = timeout_ms > 0 ? timeout_ms : 1;
    const bool ok = ReadPipeMessage(pipe, &raw_response, error, response_timeout_ms);
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
    if (error) *error = "daemon startup timed out";
    return false;
#else
    (void)pipe_name;
    (void)timeout_ms;
    if (error) *error = "daemon IPC is not implemented on this platform";
    return false;
#endif
}

}  // namespace mbink::ui_dev
