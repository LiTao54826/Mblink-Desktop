/**
 * @file log_filter.h
 * @brief 日志过滤器
 *
 * 支持按级别和源名过滤日志。
 */

#ifndef MBINK_DOM_ELEMENTS_LOGVIEW_LOG_FILTER_H_
#define MBINK_DOM_ELEMENTS_LOGVIEW_LOG_FILTER_H_

#include "log_buffer.h"

#include <set>
#include <vector>

namespace mbink {

/**
 * @brief 日志过滤器
 *
 * 特点：
 * - 位掩码级别过滤：高效的级别筛选
 * - 源名过滤：按源名白名单过滤
 * - 索引维护：过滤不复制数据，只维护索引
 */
class LogFilter {
public:
    LogFilter();

    // === 级别过滤 ===

    /**
     * @brief 设置级别过滤掩码
     * @param mask 位掩码，每位对应一个级别
     *
     * 例如：0x1F 显示所有级别，0x1C 只显示 WARN/ERROR/FATAL
     */
    void SetLevelMask(uint8_t mask);

    /**
     * @brief 获取当前级别掩码
     */
    uint8_t level_mask() const { return level_mask_; }

    /**
     * @brief 启用指定级别
     */
    void EnableLevel(LogLevel level);

    /**
     * @brief 禁用指定级别
     */
    void DisableLevel(LogLevel level);

    /**
     * @brief 检查级别是否启用
     */
    bool IsLevelEnabled(LogLevel level) const;

    // === 源名过滤 ===

    /**
     * @brief 设置源名过滤（白名单模式）
     * @param sources 允许的源名列表，空表示允许所有
     */
    void SetSourceFilter(const std::vector<std::string>& sources);

    /**
     * @brief 清除源名过滤
     */
    void ClearSourceFilter();

    /**
     * @brief 检查源名是否允许
     */
    bool IsSourceAllowed(const std::string& source) const;

    // === 过滤操作 ===

    /**
     * @brief 检查日志条目是否通过过滤
     */
    bool Matches(LogLevel level, const std::string& source) const;

    /**
     * @brief 更新过滤后的索引列表
     * @param buffer 日志缓冲区
     * @param[out] indices 输出的过滤后索引
     */
    void UpdateFilteredIndices(const LogBuffer& buffer,
                               std::vector<size_t>& indices) const;

    /**
     * @brief 检查是否有任何过滤条件
     */
    bool HasFilter() const;

private:
    uint8_t level_mask_;                  ///< 级别过滤掩码
    std::set<std::string> source_filter_; ///< 源名白名单
    bool filter_by_source_;               ///< 是否启用源名过滤
};

}  // namespace mbink

#endif  // MBINK_DOM_ELEMENTS_LOGVIEW_LOG_FILTER_H_
