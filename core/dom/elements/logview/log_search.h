/**
 * @file log_search.h
 * @brief 日志搜索功能
 *
 * 支持普通文本和正则表达式搜索。
 */

#ifndef MBLINK_DOM_ELEMENTS_LOGVIEW_LOG_SEARCH_H_
#define MBLINK_DOM_ELEMENTS_LOGVIEW_LOG_SEARCH_H_

#include "log_buffer.h"

#include <regex>
#include <string>
#include <vector>

namespace mblink {

/**
 * @brief 搜索匹配结果
 */
struct SearchMatch {
    size_t log_index;    ///< 日志条目索引
    size_t start_pos;    ///< 匹配起始位置
    size_t length;       ///< 匹配长度
};

/**
 * @brief 日志搜索器
 *
 * 特点：
 * - 普通文本搜索：大小写不敏感
 * - 正则表达式搜索：std::regex 支持
 * - 匹配导航：上一个/下一个匹配
 * - 高亮支持：返回匹配位置信息
 */
class LogSearch {
public:
    LogSearch();

    // === 搜索操作 ===

    /**
     * @brief 执行搜索
     * @param buffer 日志缓冲区
     * @param query 搜索关键词
     * @param use_regex 是否使用正则表达式
     * @param filtered_indices 过滤后的索引（nullptr 表示搜索全部）
     * @return 匹配数量
     */
    int Search(const LogBuffer& buffer, const std::string& query,
               bool use_regex = false,
               const std::vector<size_t>* filtered_indices = nullptr);

    /**
     * @brief 清除搜索
     */
    void Clear();

    // === 匹配导航 ===

    /**
     * @brief 跳转到下一个匹配
     * @return 当前匹配的日志索引，-1 表示无匹配
     */
    int NextMatch();

    /**
     * @brief 跳转到上一个匹配
     * @return 当前匹配的日志索引，-1 表示无匹配
     */
    int PrevMatch();

    /**
     * @brief 跳转到指定匹配
     * @param index 匹配索引
     * @return 当前匹配的日志索引，-1 表示无效索引
     */
    int GoToMatch(int index);

    // === 查询 ===

    /**
     * @brief 获取匹配数量
     */
    int match_count() const { return static_cast<int>(matches_.size()); }

    /**
     * @brief 获取当前匹配索引
     */
    int current_match() const { return current_match_; }

    /**
     * @brief 获取当前搜索关键词
     */
    const std::string& query() const { return query_; }

    /**
     * @brief 是否有活动搜索
     */
    bool HasSearch() const { return !query_.empty(); }

    /**
     * @brief 获取所有匹配
     */
    const std::vector<SearchMatch>& matches() const { return matches_; }

    /**
     * @brief 获取指定日志条目的匹配列表
     * @param log_index 日志索引
     * @return 该条目中的所有匹配
     */
    std::vector<SearchMatch> GetMatchesForEntry(size_t log_index) const;

    /**
     * @brief 检查日志条目是否包含匹配
     */
    bool HasMatchInEntry(size_t log_index) const;

    /**
     * @brief 检查是否是当前高亮的匹配
     */
    bool IsCurrentMatch(size_t log_index, size_t start_pos) const;

private:
    std::string query_;                ///< 当前搜索关键词
    bool use_regex_;                   ///< 是否使用正则
    std::vector<SearchMatch> matches_; ///< 所有匹配
    int current_match_;                ///< 当前匹配索引

    /**
     * @brief 普通文本搜索
     */
    void SearchPlainText(const LogBuffer& buffer,
                         const std::vector<size_t>* filtered_indices);

    /**
     * @brief 正则表达式搜索
     */
    void SearchRegex(const LogBuffer& buffer,
                     const std::vector<size_t>* filtered_indices);

    /**
     * @brief 在单条日志中搜索
     */
    void SearchInEntry(size_t log_index, std::string_view message);

    /**
     * @brief 在单条日志中正则搜索
     */
    void SearchRegexInEntry(size_t log_index, std::string_view message,
                            const std::regex& re);
};

}  // namespace mblink

#endif  // MBLINK_DOM_ELEMENTS_LOGVIEW_LOG_SEARCH_H_
