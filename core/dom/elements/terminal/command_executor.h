/**
 * @file command_executor.h
 * @brief 命令执行器
 *
 * 提供单次命令执行功能（不需要 PTY）。
 */

#pragma once

#include <atomic>
#include <functional>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace lightui {

/**
 * @brief 命令执行器
 *
 * 执行单次命令并捕获输出，不需要完整的 PTY 支持。
 * 适用于简单的命令执行场景。
 */
class CommandExecutor {
public:
    /// 输出回调
    using OutputCallback = std::function<void(const std::string& data, bool is_stderr)>;
    /// 退出回调
    using ExitCallback = std::function<void(int exit_code)>;

    CommandExecutor();
    ~CommandExecutor();

    /**
     * @brief 设置输出回调
     */
    void SetOutputCallback(OutputCallback cb) { output_cb_ = std::move(cb); }

    /**
     * @brief 设置退出回调
     */
    void SetExitCallback(ExitCallback cb) { exit_cb_ = std::move(cb); }

    /**
     * @brief 执行命令（异步）
     * @param command 命令字符串
     * @return 成功启动返回 true
     */
    bool Execute(const std::string& command);

    /**
     * @brief 终止当前命令
     */
    void Kill();

    /**
     * @brief 检查是否正在执行
     */
    bool IsRunning() const { return running_.load(); }

private:
    OutputCallback output_cb_;
    ExitCallback exit_cb_;
    std::atomic<bool> running_{false};

#ifdef _WIN32
    HANDLE process_handle_ = nullptr;
    HANDLE stdout_read_ = nullptr;
    HANDLE stderr_read_ = nullptr;
#else
    int child_pid_ = -1;
    int stdout_fd_ = -1;
    int stderr_fd_ = -1;
#endif

    /**
     * @brief 读取输出（在后台线程中调用）
     */
    void ReadOutput();

    /**
     * @brief 清理资源
     */
    void Cleanup();
};

}  // namespace lightui
