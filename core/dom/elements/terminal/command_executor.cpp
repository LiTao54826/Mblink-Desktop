/**
 * @file command_executor.cpp
 * @brief 命令执行器实现
 */

#include "command_executor.h"

#include <algorithm>
#include <thread>

#ifdef _WIN32
// Windows 实现
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace mbink {

CommandExecutor::CommandExecutor() = default;

CommandExecutor::~CommandExecutor() {
    Kill();
    Cleanup();
}

bool CommandExecutor::Execute(const std::string& command) {
    if (running_.load()) {
        return false;  // 已有命令在执行
    }

#ifdef _WIN32
    // Windows 实现
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE stdout_write = nullptr;
    HANDLE stderr_write = nullptr;

    // 创建 stdout 管道
    if (!CreatePipe(&stdout_read_, &stdout_write, &sa, 0)) {
        return false;
    }
    SetHandleInformation(stdout_read_, HANDLE_FLAG_INHERIT, 0);

    // 创建 stderr 管道
    if (!CreatePipe(&stderr_read_, &stderr_write, &sa, 0)) {
        CloseHandle(stdout_read_);
        CloseHandle(stdout_write);
        return false;
    }
    SetHandleInformation(stderr_read_, HANDLE_FLAG_INHERIT, 0);

    // 创建进程
    STARTUPINFOA si = {};
    si.cb = sizeof(STARTUPINFOA);
    si.hStdOutput = stdout_write;
    si.hStdError = stderr_write;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi = {};

    std::string cmd = "cmd /c " + command;
    if (!CreateProcessA(
            nullptr,
            const_cast<char*>(cmd.c_str()),
            nullptr,
            nullptr,
            TRUE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi)) {
        CloseHandle(stdout_read_);
        CloseHandle(stdout_write);
        CloseHandle(stderr_read_);
        CloseHandle(stderr_write);
        return false;
    }

    process_handle_ = pi.hProcess;
    CloseHandle(pi.hThread);
    CloseHandle(stdout_write);
    CloseHandle(stderr_write);

#else
    // Unix 实现
    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) < 0) {
        return false;
    }
    if (pipe(stderr_pipe) < 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return false;
    }

    child_pid_ = fork();
    if (child_pid_ < 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        return false;
    }

    if (child_pid_ == 0) {
        // 子进程
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(127);
    }

    // 父进程
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    stdout_fd_ = stdout_pipe[0];
    stderr_fd_ = stderr_pipe[0];

    // 设置非阻塞
    fcntl(stdout_fd_, F_SETFL, O_NONBLOCK);
    fcntl(stderr_fd_, F_SETFL, O_NONBLOCK);
#endif

    running_.store(true);

    // 启动读取线程
    std::thread([this]() {
        ReadOutput();
    }).detach();

    return true;
}

void CommandExecutor::Kill() {
    if (!running_.load()) {
        return;
    }

#ifdef _WIN32
    if (process_handle_) {
        TerminateProcess(process_handle_, 1);
    }
#else
    if (child_pid_ > 0) {
        kill(child_pid_, SIGTERM);
    }
#endif
}

void CommandExecutor::ReadOutput() {
    char buffer[4096];

#ifdef _WIN32
    DWORD bytes_read;
    
    while (running_.load()) {
        // 读取 stdout
        DWORD stdout_available = 0;
        if (PeekNamedPipe(stdout_read_, nullptr, 0, nullptr, &stdout_available, nullptr) && stdout_available > 0 &&
            ReadFile(stdout_read_, buffer, std::min<DWORD>(stdout_available, sizeof(buffer) - 1), &bytes_read, nullptr)) {
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                if (output_cb_) {
                    output_cb_(std::string(buffer, bytes_read), false);
                }
            }
        }

        // 读取 stderr
        DWORD stderr_available = 0;
        if (PeekNamedPipe(stderr_read_, nullptr, 0, nullptr, &stderr_available, nullptr) && stderr_available > 0 &&
            ReadFile(stderr_read_, buffer, std::min<DWORD>(stderr_available, sizeof(buffer) - 1), &bytes_read, nullptr)) {
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                if (output_cb_) {
                    output_cb_(std::string(buffer, bytes_read), true);
                }
            }
        }

        // 检查进程是否结束
        DWORD exit_code;
        if (GetExitCodeProcess(process_handle_, &exit_code)) {
            if (exit_code != STILL_ACTIVE) {
                DWORD stdout_available = 0;
                while (PeekNamedPipe(stdout_read_, nullptr, 0, nullptr, &stdout_available, nullptr) && stdout_available > 0 &&
                       ReadFile(stdout_read_, buffer, std::min<DWORD>(stdout_available, sizeof(buffer) - 1), &bytes_read, nullptr) &&
                       bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    if (output_cb_) {
                        output_cb_(std::string(buffer, bytes_read), false);
                    }
                }

                DWORD stderr_available = 0;
                while (PeekNamedPipe(stderr_read_, nullptr, 0, nullptr, &stderr_available, nullptr) && stderr_available > 0 &&
                       ReadFile(stderr_read_, buffer, std::min<DWORD>(stderr_available, sizeof(buffer) - 1), &bytes_read, nullptr) &&
                       bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    if (output_cb_) {
                        output_cb_(std::string(buffer, bytes_read), true);
                    }
                }

                running_.store(false);
                if (exit_cb_) {
                    exit_cb_(static_cast<int>(exit_code));
                }
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

#else
    fd_set read_fds;
    struct timeval tv;

    while (running_.load()) {
        FD_ZERO(&read_fds);
        if (stdout_fd_ >= 0) FD_SET(stdout_fd_, &read_fds);
        if (stderr_fd_ >= 0) FD_SET(stderr_fd_, &read_fds);

        tv.tv_sec = 0;
        tv.tv_usec = 10000;  // 10ms

        int max_fd = std::max(stdout_fd_, stderr_fd_);
        int ret = select(max_fd + 1, &read_fds, nullptr, nullptr, &tv);

        if (ret > 0) {
            // 读取 stdout
            if (stdout_fd_ >= 0 && FD_ISSET(stdout_fd_, &read_fds)) {
                ssize_t n = read(stdout_fd_, buffer, sizeof(buffer) - 1);
                if (n > 0) {
                    buffer[n] = '\0';
                    if (output_cb_) {
                        output_cb_(std::string(buffer, n), false);
                    }
                } else if (n == 0) {
                    close(stdout_fd_);
                    stdout_fd_ = -1;
                }
            }

            // 读取 stderr
            if (stderr_fd_ >= 0 && FD_ISSET(stderr_fd_, &read_fds)) {
                ssize_t n = read(stderr_fd_, buffer, sizeof(buffer) - 1);
                if (n > 0) {
                    buffer[n] = '\0';
                    if (output_cb_) {
                        output_cb_(std::string(buffer, n), true);
                    }
                } else if (n == 0) {
                    close(stderr_fd_);
                    stderr_fd_ = -1;
                }
            }
        }

        // 检查子进程是否结束
        int status;
        pid_t result = waitpid(child_pid_, &status, WNOHANG);
        if (result == child_pid_) {
            running_.store(false);
            int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            if (exit_cb_) {
                exit_cb_(exit_code);
            }
            break;
        }
    }
#endif

    Cleanup();
}

void CommandExecutor::Cleanup() {
#ifdef _WIN32
    if (stdout_read_) {
        CloseHandle(stdout_read_);
        stdout_read_ = nullptr;
    }
    if (stderr_read_) {
        CloseHandle(stderr_read_);
        stderr_read_ = nullptr;
    }
    if (process_handle_) {
        CloseHandle(process_handle_);
        process_handle_ = nullptr;
    }
#else
    if (stdout_fd_ >= 0) {
        close(stdout_fd_);
        stdout_fd_ = -1;
    }
    if (stderr_fd_ >= 0) {
        close(stderr_fd_);
        stderr_fd_ = -1;
    }
    child_pid_ = -1;
#endif
}

}  // namespace mbink
