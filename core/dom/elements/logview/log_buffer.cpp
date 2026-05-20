/**
 * @file log_buffer.cpp
 * @brief 日志专用缓冲区实现
 */

#include "log_buffer.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace mbink {

LogBuffer::LogBuffer(size_t max_entries)
    : max_entries_(max_entries),
      start_time_(Clock::now()) {
    constexpr size_t kInitialEntryReserve = 256;
    constexpr size_t kEstimatedBytesPerEntry = 64;
    const size_t initial_entries = std::min(max_entries, kInitialEntryReserve);
    data_.reserve(initial_entries * kEstimatedBytesPerEntry);
    entry_offsets_.reserve(initial_entries);
    source_names_.reserve(16);
}

void LogBuffer::Append(LogLevel level, uint8_t source_id,
                       std::string_view message, uint32_t timestamp) {
    // 限制消息长度
    size_t msg_len = std::min(message.size(), static_cast<size_t>(65535));

    // 计算时间戳
    if (timestamp == 0) {
        timestamp = GetCurrentTimestamp();
    }

    // 记录偏移
    size_t offset = data_.size();
    entry_offsets_.push_back(offset);

    // 写入头部
    data_.resize(data_.size() + LogEntryHeader::SIZE + msg_len);
    uint8_t* ptr = data_.data() + offset;

    // timestamp (4 bytes)
    std::memcpy(ptr, &timestamp, 4);
    ptr += 4;

    // level (1 byte)
    *ptr++ = static_cast<uint8_t>(level);

    // source_id (1 byte)
    *ptr++ = source_id;

    // message_length (2 bytes)
    uint16_t len16 = static_cast<uint16_t>(msg_len);
    std::memcpy(ptr, &len16, 2);
    ptr += 2;

    // message
    std::memcpy(ptr, message.data(), msg_len);

    // 检查容量
    TrimToCapacity();
}

void LogBuffer::Append(LogLevel level, const std::string& source,
                       std::string_view message) {
    uint8_t source_id = RegisterSource(source);
    Append(level, source_id, message);
}

void LogBuffer::Clear() {
    data_.clear();
    entry_offsets_.clear();
    start_time_ = Clock::now();
}

LogEntryView LogBuffer::GetEntry(size_t index) const {
    return LogEntryView(data_.data() + entry_offsets_[index]);
}

LogLevel LogBuffer::GetLevel(size_t index) const {
    return GetEntry(index).level();
}

uint8_t LogBuffer::GetSourceId(size_t index) const {
    return GetEntry(index).source_id();
}

uint32_t LogBuffer::GetTimestamp(size_t index) const {
    return GetEntry(index).timestamp();
}

std::string_view LogBuffer::GetMessage(size_t index) const {
    return GetEntry(index).message();
}

uint8_t LogBuffer::RegisterSource(const std::string& name) {
    // 查找已存在的源名
    auto it = std::find(source_names_.begin(), source_names_.end(), name);
    if (it != source_names_.end()) {
        return static_cast<uint8_t>(it - source_names_.begin());
    }

    // 添加新源名（最多 256 个）
    if (source_names_.size() >= 256) {
        return 0;  // 超出限制，使用第一个
    }

    source_names_.push_back(name);
    return static_cast<uint8_t>(source_names_.size() - 1);
}

const std::string& LogBuffer::GetSourceName(uint8_t id) const {
    static const std::string empty;
    if (id < source_names_.size()) {
        return source_names_[id];
    }
    return empty;
}

std::string LogBuffer::ExportAsText() const {
    std::ostringstream oss;

    for (size_t i = 0; i < size(); ++i) {
        auto entry = GetEntry(i);
        oss << FormatTimestamp(entry.timestamp()) << " "
            << "[" << LogLevelToString(entry.level()) << "] "
            << "[" << GetSourceName(entry.source_id()) << "] "
            << entry.message() << "\n";
    }

    return oss.str();
}

std::string LogBuffer::ExportAsJson() const {
    std::ostringstream oss;
    oss << "[\n";

    for (size_t i = 0; i < size(); ++i) {
        auto entry = GetEntry(i);

        if (i > 0) oss << ",\n";
        oss << "  {"
            << "\"timestamp\":" << entry.timestamp() << ","
            << "\"level\":\"" << LogLevelToString(entry.level()) << "\","
            << "\"source\":\"" << GetSourceName(entry.source_id()) << "\","
            << "\"message\":\"";

        // 转义 JSON 字符串
        for (char c : entry.message()) {
            switch (c) {
                case '"':  oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 32) {
                        oss << "\\u" << std::hex << std::setfill('0')
                            << std::setw(4) << static_cast<int>(c);
                    } else {
                        oss << c;
                    }
            }
        }
        oss << "\"}";
    }

    oss << "\n]";
    return oss.str();
}

void LogBuffer::TrimToCapacity() {
    if (entry_offsets_.size() <= max_entries_) {
        return;
    }

    // 计算要删除的条目数
    size_t to_remove = entry_offsets_.size() - max_entries_;

    // 计算新的数据起始偏移
    size_t new_start = entry_offsets_[to_remove];

    // 移动数据
    data_.erase(data_.begin(), data_.begin() + new_start);

    // 更新偏移量
    entry_offsets_.erase(entry_offsets_.begin(),
                         entry_offsets_.begin() + to_remove);

    // 调整所有偏移量
    for (auto& offset : entry_offsets_) {
        offset -= new_start;
    }
}

uint32_t LogBuffer::GetCurrentTimestamp() const {
    auto now = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time_);
    return static_cast<uint32_t>(duration.count());
}

std::string LogBuffer::FormatTimestamp(uint32_t timestamp) {
    uint32_t ms = timestamp % 1000;
    uint32_t total_sec = timestamp / 1000;
    uint32_t sec = total_sec % 60;
    uint32_t min = (total_sec / 60) % 60;
    uint32_t hour = total_sec / 3600;

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << hour << ":"
        << std::setw(2) << min << ":"
        << std::setw(2) << sec << "."
        << std::setw(3) << ms;
    return oss.str();
}

}  // namespace mbink
