/**
 * @file pty_backend.h
 * @brief PTY 后端抽象接口
 *
 * 提供跨平台的伪终端抽象。
 */

#pragma once

#include <functional>
#include <memory>
#include <string>

namespace lightui {

/**
 * @brief PTY 后端抽象基类
 *
 * 提供跨平台的伪终端接口：
 * - Windows: ConPTY
 * - Unix: forkpty
 */
class PtyBackend {
public:
    /// 数据回调
    using DataCallback = std::function<void(const char* data, size_t len)>;
    /// 退出回调
    using ExitCallback = std::function<void(int exit_code)>;

    virtual ~PtyBackend() = default;

    // === 生命周期 ===

    /**
     * @brief 启动 PTY
     * @param shell Shell 路径（空则使用默认）
     * @param rows 行数
     * @param cols 列数
     * @return 成功返回 true
     */
    virtual bool Start(const std::string& shell, int rows, int cols) = 0;

    /**
     * @brief 停止 PTY
     */
    virtual void Stop() = 0;

    /**
     * @brief 检查是否正在运行
     */
    virtual bool IsRunning() const = 0;

    // === I/O ===

    /**
     * @brief 写入数据到 PTY
     * @param data 数据
     * @param len 长度
     */
    virtual void Write(const char* data, size_t len) = 0;

    /**
     * @brief 调整 PTY 大小
     * @param rows 行数
     * @param cols 列数
     */
    virtual void Resize(int rows, int cols) = 0;

    // === 回调 ===

    /**
     * @brief 设置数据回调
     */
    void SetDataCallback(DataCallback cb) { data_cb_ = std::move(cb); }

    /**
     * @brief 设置退出回调
     */
    void SetExitCallback(ExitCallback cb) { exit_cb_ = std::move(cb); }

    // === 工厂方法 ===

    /**
     * @brief 创建平台特定的 PTY 后端
     * @return PTY 后端实例
     */
    static std::unique_ptr<PtyBackend> Create();

protected:
    DataCallback data_cb_;
    ExitCallback exit_cb_;
};

}  // namespace lightui
