/**
 * @file unix_pty_backend.cpp
 * @brief Unix PTY 后端实现
 */

#include "pty_backend.h"

#ifndef _WIN32

#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <thread>

namespace mblink {

/**
 * @brief Unix PTY 后端
 */
class UnixPtyBackend : public PtyBackend {
public:
    UnixPtyBackend() = default;
    ~UnixPtyBackend() override { Stop(); }

    bool Start(const std::string& shell, int rows, int cols) override {
        if (running_) {
            return false;
        }

        // 确定 shell
        std::string shell_path = shell;
        if (shell_path.empty()) {
            const char* env_shell = getenv("SHELL");
            shell_path = env_shell ? env_shell : "/bin/sh";
        }

        // 设置窗口大小
        struct winsize ws;
        ws.ws_row = rows;
        ws.ws_col = cols;
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;

        // 创建 PTY
        child_pid_ = forkpty(&master_fd_, nullptr, nullptr, &ws);
        if (child_pid_ < 0) {
            return false;
        }

        if (child_pid_ == 0) {
            // 子进程
            setenv("TERM", "xterm-256color", 1);
            execlp(shell_path.c_str(), shell_path.c_str(), nullptr);
            _exit(127);
        }

        // 父进程
        running_ = true;

        // 启动读取线程
        read_thread_ = std::thread([this]() {
            ReadLoop();
        });

        return true;
    }

    void Stop() override {
        if (!running_) {
            return;
        }

        running_ = false;

        if (child_pid_ > 0) {
            kill(child_pid_, SIGHUP);
            waitpid(child_pid_, nullptr, 0);
            child_pid_ = -1;
        }

        if (master_fd_ >= 0) {
            close(master_fd_);
            master_fd_ = -1;
        }

        if (read_thread_.joinable()) {
            read_thread_.join();
        }
    }

    bool IsRunning() const override {
        return running_;
    }

    void Write(const char* data, size_t len) override {
        if (master_fd_ >= 0 && running_) {
            write(master_fd_, data, len);
        }
    }

    void Resize(int rows, int cols) override {
        if (master_fd_ >= 0) {
            struct winsize ws;
            ws.ws_row = rows;
            ws.ws_col = cols;
            ws.ws_xpixel = 0;
            ws.ws_ypixel = 0;
            ioctl(master_fd_, TIOCSWINSZ, &ws);
        }
    }

private:
    bool running_ = false;
    int master_fd_ = -1;
    pid_t child_pid_ = -1;
    std::thread read_thread_;

    void ReadLoop() {
        char buffer[4096];
        struct pollfd pfd;
        pfd.fd = master_fd_;
        pfd.events = POLLIN;

        while (running_) {
            int ret = poll(&pfd, 1, 100);  // 100ms 超时
            if (ret > 0 && (pfd.revents & POLLIN)) {
                ssize_t n = read(master_fd_, buffer, sizeof(buffer));
                if (n > 0) {
                    if (data_cb_) {
                        data_cb_(buffer, n);
                    }
                } else if (n <= 0) {
                    // PTY 关闭
                    break;
                }
            }

            // 检查子进程
            int status;
            pid_t result = waitpid(child_pid_, &status, WNOHANG);
            if (result == child_pid_) {
                running_ = false;
                int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                if (exit_cb_) {
                    exit_cb_(exit_code);
                }
                break;
            }
        }
    }
};

std::unique_ptr<PtyBackend> PtyBackend::Create() {
    return std::make_unique<UnixPtyBackend>();
}

}  // namespace mblink

#endif  // !_WIN32
