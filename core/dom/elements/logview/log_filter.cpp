/**
 * @file log_filter.cpp
 * @brief 日志过滤器实现
 */

#include "log_filter.h"

namespace mblink {

LogFilter::LogFilter()
    : level_mask_(0xFF),  // 默认显示所有级别
      filter_by_source_(false) {
}

void LogFilter::SetLevelMask(uint8_t mask) {
    level_mask_ = mask;
}

void LogFilter::EnableLevel(LogLevel level) {
    level_mask_ |= (1 << static_cast<int>(level));
}

void LogFilter::DisableLevel(LogLevel level) {
    level_mask_ &= ~(1 << static_cast<int>(level));
}

bool LogFilter::IsLevelEnabled(LogLevel level) const {
    return (level_mask_ & (1 << static_cast<int>(level))) != 0;
}

void LogFilter::SetSourceFilter(const std::vector<std::string>& sources) {
    source_filter_.clear();
    for (const auto& s : sources) {
        source_filter_.insert(s);
    }
    filter_by_source_ = !source_filter_.empty();
}

void LogFilter::ClearSourceFilter() {
    source_filter_.clear();
    filter_by_source_ = false;
}

bool LogFilter::IsSourceAllowed(const std::string& source) const {
    if (!filter_by_source_) {
        return true;
    }
    return source_filter_.find(source) != source_filter_.end();
}

bool LogFilter::Matches(LogLevel level, const std::string& source) const {
    // 检查级别
    if (!IsLevelEnabled(level)) {
        return false;
    }

    // 检查源名
    if (!IsSourceAllowed(source)) {
        return false;
    }

    return true;
}

void LogFilter::UpdateFilteredIndices(const LogBuffer& buffer,
                                      std::vector<size_t>& indices) const {
    indices.clear();
    indices.reserve(buffer.size());

    for (size_t i = 0; i < buffer.size(); ++i) {
        LogLevel level = buffer.GetLevel(i);

        // 检查级别
        if (!IsLevelEnabled(level)) {
            continue;
        }

        // 检查源名
        if (filter_by_source_) {
            uint8_t source_id = buffer.GetSourceId(i);
            const std::string& source = buffer.GetSourceName(source_id);
            if (!IsSourceAllowed(source)) {
                continue;
            }
        }

        indices.push_back(i);
    }
}

bool LogFilter::HasFilter() const {
    // 如果级别掩码不是全部启用，或者有源名过滤
    return level_mask_ != 0xFF || filter_by_source_;
}

}  // namespace mblink
