/**
 * @file conpty_backend.cpp
 * @brief Windows ConPTY 后端实现
 *
 * 使用 Windows 10 1809+ 的 CreatePseudoConsole API 实现伪终端。
 */

#include "pty_backend.h"

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <iostream>
#include <thread>

namespace mblink {

/**
 * @brief Windows ConPTY 后端
 *
 * 使用 ConPTY API 提供完整的伪终端功能。
 * 需要 Windows 10 版本 1809 或更高版本。
 */
class ConPtyBackend : public PtyBackend {
public:
    ConPtyBackend() = default;
    ~ConPtyBackend() override { Stop(); }

    bool Start(const std::string& shell, int rows, int cols) override {
        if (running_.load()) {
            return false;
        }

        // 创建管道
        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;

        // 输入管道：我们写入 -> PTY 读取
        if (!CreatePipe(&input_read_, &input_write_, &sa, 0)) {
            return false;
        }

        // 输出管道：PTY 写入 -> 我们读取
        if (!CreatePipe(&output_read_, &output_write_, &sa, 0)) {
            CloseHandle(input_read_);
            CloseHandle(input_write_);
            return false;
        }

        // 创建伪控制台
        COORD size;
        size.X = static_cast<SHORT>(cols);
        size.Y = static_cast<SHORT>(rows);

        HRESULT hr = CreatePseudoConsole(
            size,
            input_read_,
            output_write_,
            0,
            &pty_handle_
        );

        if (FAILED(hr)) {
            Cleanup();
            return false;
        }

        // 确定 shell 路径
        std::string shell_path = shell;
        if (shell_path.empty()) {
            char* comspec = nullptr;
            size_t len = 0;
            if (_dupenv_s(&comspec, &len, "COMSPEC") == 0 && comspec) {
                shell_path = comspec;
                free(comspec);
            } else {
                shell_path = "cmd.exe";
            }
        }

        // 初始化启动信息
        STARTUPINFOEXW si = {};
        si.StartupInfo.cb = sizeof(STARTUPINFOEXW);

        // 分配属性列表
        SIZE_T attr_size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &attr_size);

        si.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            HeapAlloc(GetProcessHeap(), 0, attr_size)
        );

        if (!si.lpAttributeList) {
            ClosePseudoConsole(pty_handle_);
            pty_handle_ = nullptr;
            Cleanup();
            return false;
        }

        if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attr_size)) {
            HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
            ClosePseudoConsole(pty_handle_);
            pty_handle_ = nullptr;
            Cleanup();
            return false;
        }

        // 设置伪控制台属性
        if (!UpdateProcThreadAttribute(
                si.lpAttributeList,
                0,
                PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                pty_handle_,
                sizeof(HPCON),
                nullptr,
                nullptr)) {
            DeleteProcThreadAttributeList(si.lpAttributeList);
            HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
            ClosePseudoConsole(pty_handle_);
            pty_handle_ = nullptr;
            Cleanup();
            return false;
        }

        // 转换为宽字符
        int wide_len = MultiByteToWideChar(CP_UTF8, 0, shell_path.c_str(), -1, nullptr, 0);
        std::wstring wide_shell(wide_len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, shell_path.c_str(), -1, &wide_shell[0], wide_len);

        // 创建进程
        PROCESS_INFORMATION pi = {};
        BOOL success = CreateProcessW(
            nullptr,
            &wide_shell[0],
            nullptr,
            nullptr,
            FALSE,
            EXTENDED_STARTUPINFO_PRESENT,
            nullptr,
            nullptr,
            &si.StartupInfo,
            &pi
        );

        DeleteProcThreadAttributeList(si.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, si.lpAttributeList);

        if (!success) {
            ClosePseudoConsole(pty_handle_);
            pty_handle_ = nullptr;
            Cleanup();
            return false;
        }

        process_handle_ = pi.hProcess;
        CloseHandle(pi.hThread);

        // 关闭不需要的管道端
        CloseHandle(input_read_);
        input_read_ = nullptr;
        CloseHandle(output_write_);
        output_write_ = nullptr;

        running_.store(true);

        // 启动读取线程
        read_thread_ = std::thread([this]() {
            ReadLoop();
        });

        return true;
    }

    void Stop() override {
        if (!running_.load()) {
            return;
        }

        running_.store(false);

        // 关闭伪控制台（这会导致进程终止）
        if (pty_handle_) {
            ClosePseudoConsole(pty_handle_);
            pty_handle_ = nullptr;
        }

        // 等待进程结束
        if (process_handle_) {
            WaitForSingleObject(process_handle_, 1000);
            CloseHandle(process_handle_);
            process_handle_ = nullptr;
        }

        // 等待读取线程结束
        if (read_thread_.joinable()) {
            read_thread_.join();
        }

        Cleanup();
    }

    bool IsRunning() const override {
        return running_.load();
    }

    void Write(const char* data, size_t len) override {
        if (input_write_ && running_.load()) {
            DWORD written;
            WriteFile(input_write_, data, static_cast<DWORD>(len), &written, nullptr);
        }
    }

    void Resize(int rows, int cols) override {
        if (pty_handle_) {
            COORD size;
            size.X = static_cast<SHORT>(cols);
            size.Y = static_cast<SHORT>(rows);
            ResizePseudoConsole(pty_handle_, size);
        }
    }

private:
    std::atomic<bool> running_{false};
    HPCON pty_handle_ = nullptr;
    HANDLE input_read_ = nullptr;
    HANDLE input_write_ = nullptr;
    HANDLE output_read_ = nullptr;
    HANDLE output_write_ = nullptr;
    HANDLE process_handle_ = nullptr;
    std::thread read_thread_;

    void ReadLoop() {
        char buffer[4096];
        DWORD bytes_read;

        while (running_.load()) {
            BOOL success = ReadFile(
                output_read_,
                buffer,
                sizeof(buffer),
                &bytes_read,
                nullptr
            );

            if (success && bytes_read > 0) {
                if (data_cb_) {
                    // ConPTY 在 Windows 10 1903+ 默认输出 UTF-8 (CP 65001)
                    // 直接传递原始数据
                    data_cb_(buffer, bytes_read);
                }
            } else {
                // 读取失败或 PTY 关闭
                break;
            }
        }

        // 获取退出码
        if (process_handle_) {
            DWORD exit_code = 0;
            if (GetExitCodeProcess(process_handle_, &exit_code)) {
                if (exit_code != STILL_ACTIVE && exit_cb_) {
                    exit_cb_(static_cast<int>(exit_code));
                }
            }
        }

        running_.store(false);
    }

    void Cleanup() {
        if (input_read_) {
            CloseHandle(input_read_);
            input_read_ = nullptr;
        }
        if (input_write_) {
            CloseHandle(input_write_);
            input_write_ = nullptr;
        }
        if (output_read_) {
            CloseHandle(output_read_);
            output_read_ = nullptr;
        }
        if (output_write_) {
            CloseHandle(output_write_);
            output_write_ = nullptr;
        }
    }
};

std::unique_ptr<PtyBackend> PtyBackend::Create() {
    return std::make_unique<ConPtyBackend>();
}

}  // namespace mblink

#endif  // _WIN32
