/**
 * @file log_entry.h
 * @brief 日志条目结构定义
 *
 * 紧凑的日志条目结构，8字节头部 + 消息内容。
 */

#ifndef MBINK_DOM_ELEMENTS_LOGVIEW_LOG_ENTRY_H_
#define MBINK_DOM_ELEMENTS_LOGVIEW_LOG_ENTRY_H_

#include <cstdint>
#include <string>
#include <string_view>

namespace lightui {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

/**
 * @brief 日志级别转字符串
 */
inline const char* LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 字符串转日志级别
 */
inline LogLevel StringToLogLevel(std::string_view str) {
    if (str == "DEBUG") return LogLevel::DEBUG;
    if (str == "INFO")  return LogLevel::INFO;
    if (str == "WARN")  return LogLevel::WARN;
    if (str == "ERROR") return LogLevel::ERROR;
    if (str == "FATAL") return LogLevel::FATAL;
    return LogLevel::INFO;
}

/**
 * @brief 日志条目头部结构
 *
 * 内存布局 (8 bytes):
 *   - timestamp: 4 bytes (相对时间戳，毫秒)
 *   - level: 1 byte
 *   - source_id: 1 byte (预注册源名索引)
 *   - message_length: 2 bytes
 */
struct LogEntryHeader {
    uint32_t timestamp;       ///< 相对时间戳（毫秒）
    LogLevel level;           ///< 日志级别
    uint8_t source_id;        ///< 源名索引
    uint16_t message_length;  ///< 消息长度

    static constexpr size_t SIZE = 8;
};

static_assert(sizeof(LogEntryHeader) == LogEntryHeader::SIZE,
              "LogEntryHeader must be 8 bytes");

/**
 * @brief 日志条目视图（只读）
 *
 * 提供对紧凑存储中日志条目的只读访问。
 */
class LogEntryView {
public:
    LogEntryView(const uint8_t* data) : data_(data) {}

    uint32_t timestamp() const {
        return *reinterpret_cast<const uint32_t*>(data_);
    }

    LogLevel level() const {
        return static_cast<LogLevel>(data_[4]);
    }

    uint8_t source_id() const {
        return data_[5];
    }

    uint16_t message_length() const {
        return *reinterpret_cast<const uint16_t*>(data_ + 6);
    }

    std::string_view message() const {
        return std::string_view(
            reinterpret_cast<const char*>(data_ + LogEntryHeader::SIZE),
            message_length());
    }

    /// 整个条目的总大小
    size_t total_size() const {
        return LogEntryHeader::SIZE + message_length();
    }

private:
    const uint8_t* data_;
};

}  // namespace lightui

#endif  // MBINK_DOM_ELEMENTS_LOGVIEW_LOG_ENTRY_H_
