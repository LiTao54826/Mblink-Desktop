/**
 * @file log_buffer.h
 * @brief 日志专用缓冲区
 *
 * 紧凑存储实现，支持源名注册和导出功能。
 */

#ifndef MBLINK_DOM_ELEMENTS_LOGVIEW_LOG_BUFFER_H_
#define MBLINK_DOM_ELEMENTS_LOGVIEW_LOG_BUFFER_H_

#include "log_entry.h"

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

namespace mblink {

/**
 * @brief 日志专用缓冲区
 *
 * 特点：
 * - 紧凑存储：所有日志数据连续存放
 * - 固定容量：超过最大条目数自动淘汰旧日志
 * - 源名注册：预注册源名，用索引引用
 * - 高效导出：支持文本和 JSON 格式
 */
class LogBuffer {
public:
    /**
     * @brief 构造函数
     * @param max_entries 最大日志条目数
     */
    explicit LogBuffer(size_t max_entries = 100000);

    // === 写入操作 ===

    /**
     * @brief 追加日志条目
     * @param level 日志级别
     * @param source_id 源名索引（通过 RegisterSource 获取）
     * @param message 日志消息
     * @param timestamp 时间戳（0 表示使用当前时间）
     */
    void Append(LogLevel level, uint8_t source_id,
                std::string_view message, uint32_t timestamp = 0);

    /**
     * @brief 追加日志条目（使用源名字符串）
     * @param level 日志级别
     * @param source 源名字符串
     * @param message 日志消息
     */
    void Append(LogLevel level, const std::string& source,
                std::string_view message);

    /**
     * @brief 清空所有日志
     */
    void Clear();

    // === 读取操作 ===

    /**
     * @brief 获取日志条目数
     */
    size_t size() const { return entry_offsets_.size(); }

    /**
     * @brief 是否为空
     */
    bool empty() const { return entry_offsets_.empty(); }

    /**
     * @brief 获取指定索引的日志条目视图
     */
    LogEntryView GetEntry(size_t index) const;

    /**
     * @brief 获取日志级别
     */
    LogLevel GetLevel(size_t index) const;

    /**
     * @brief 获取源名索引
     */
    uint8_t GetSourceId(size_t index) const;

    /**
     * @brief 获取时间戳
     */
    uint32_t GetTimestamp(size_t index) const;

    /**
     * @brief 获取消息内容
     */
    std::string_view GetMessage(size_t index) const;

    // === 源名管理 ===

    /**
     * @brief 注册源名
     * @param name 源名字符串
     * @return 源名索引（0-255）
     */
    uint8_t RegisterSource(const std::string& name);

    /**
     * @brief 获取源名
     * @param id 源名索引
     * @return 源名字符串
     */
    const std::string& GetSourceName(uint8_t id) const;

    /**
     * @brief 获取所有已注册的源名
     */
    const std::vector<std::string>& GetSourceNames() const {
        return source_names_;
    }

    // === 导出功能 ===

    /**
     * @brief 导出为纯文本格式
     */
    std::string ExportAsText() const;

    /**
     * @brief 导出为 JSON 格式
     */
    std::string ExportAsJson() const;

    // === 容量信息 ===

    size_t max_entries() const { return max_entries_; }
    size_t data_size() const { return data_.size(); }

private:
    std::vector<uint8_t> data_;           ///< 紧凑存储的日志数据
    std::vector<size_t> entry_offsets_;   ///< 每条日志的起始偏移
    std::vector<std::string> source_names_; ///< 源名列表
    size_t max_entries_;                  ///< 最大条目数

    using Clock = std::chrono::steady_clock;
    Clock::time_point start_time_;        ///< 起始时间（用于计算相对时间戳）

    /**
     * @brief 裁剪到容量限制
     */
    void TrimToCapacity();

    /**
     * @brief 获取当前相对时间戳
     */
    uint32_t GetCurrentTimestamp() const;

    /**
     * @brief 格式化时间戳
     */
    static std::string FormatTimestamp(uint32_t timestamp);
};

}  // namespace mblink

#endif  // MBLINK_DOM_ELEMENTS_LOGVIEW_LOG_BUFFER_H_
