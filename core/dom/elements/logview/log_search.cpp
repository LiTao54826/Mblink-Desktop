/**
 * @file log_search.cpp
 * @brief 日志搜索功能实现
 */

#include "log_search.h"

#include <algorithm>
#include <cctype>

namespace mbink {

LogSearch::LogSearch()
    : use_regex_(false),
      current_match_(-1) {
}

int LogSearch::Search(const LogBuffer& buffer, const std::string& query,
                      bool use_regex,
                      const std::vector<size_t>* filtered_indices) {
    Clear();

    if (query.empty()) {
        return 0;
    }

    query_ = query;
    use_regex_ = use_regex;

    if (use_regex) {
        SearchRegex(buffer, filtered_indices);
    } else {
        SearchPlainText(buffer, filtered_indices);
    }

    if (!matches_.empty()) {
        current_match_ = 0;
    }

    return static_cast<int>(matches_.size());
}

void LogSearch::Clear() {
    query_.clear();
    matches_.clear();
    current_match_ = -1;
    use_regex_ = false;
}

int LogSearch::NextMatch() {
    if (matches_.empty()) {
        return -1;
    }

    current_match_ = (current_match_ + 1) % static_cast<int>(matches_.size());
    return static_cast<int>(matches_[current_match_].log_index);
}

int LogSearch::PrevMatch() {
    if (matches_.empty()) {
        return -1;
    }

    current_match_--;
    if (current_match_ < 0) {
        current_match_ = static_cast<int>(matches_.size()) - 1;
    }
    return static_cast<int>(matches_[current_match_].log_index);
}

int LogSearch::GoToMatch(int index) {
    if (index < 0 || index >= static_cast<int>(matches_.size())) {
        return -1;
    }

    current_match_ = index;
    return static_cast<int>(matches_[current_match_].log_index);
}

std::vector<SearchMatch> LogSearch::GetMatchesForEntry(size_t log_index) const {
    std::vector<SearchMatch> result;
    for (const auto& match : matches_) {
        if (match.log_index == log_index) {
            result.push_back(match);
        }
    }
    return result;
}

bool LogSearch::HasMatchInEntry(size_t log_index) const {
    for (const auto& match : matches_) {
        if (match.log_index == log_index) {
            return true;
        }
    }
    return false;
}

bool LogSearch::IsCurrentMatch(size_t log_index, size_t start_pos) const {
    if (current_match_ < 0 ||
        current_match_ >= static_cast<int>(matches_.size())) {
        return false;
    }

    const auto& match = matches_[current_match_];
    return match.log_index == log_index && match.start_pos == start_pos;
}

void LogSearch::SearchPlainText(const LogBuffer& buffer,
                                const std::vector<size_t>* filtered_indices) {
    // 转换为小写进行大小写不敏感搜索
    std::string lower_query = query_;
    std::transform(lower_query.begin(), lower_query.end(),
                   lower_query.begin(), ::tolower);

    auto search_entry = [&](size_t idx) {
        std::string_view message = buffer.GetMessage(idx);
        std::string lower_msg(message);
        std::transform(lower_msg.begin(), lower_msg.end(),
                       lower_msg.begin(), ::tolower);

        size_t pos = 0;
        while ((pos = lower_msg.find(lower_query, pos)) != std::string::npos) {
            matches_.push_back({idx, pos, query_.size()});
            pos += query_.size();
        }
    };

    if (filtered_indices) {
        for (size_t idx : *filtered_indices) {
            search_entry(idx);
        }
    } else {
        for (size_t i = 0; i < buffer.size(); ++i) {
            search_entry(i);
        }
    }
}

void LogSearch::SearchRegex(const LogBuffer& buffer,
                            const std::vector<size_t>* filtered_indices) {
    std::regex re;
    try {
        re = std::regex(query_, std::regex::icase);
    } catch (const std::regex_error&) {
        // 无效正则，回退到普通搜索
        SearchPlainText(buffer, filtered_indices);
        return;
    }

    auto search_entry = [&](size_t idx) {
        std::string_view message = buffer.GetMessage(idx);
        std::string msg_str(message);

        auto begin = std::sregex_iterator(msg_str.begin(), msg_str.end(), re);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            matches_.push_back({
                idx,
                static_cast<size_t>(it->position()),
                static_cast<size_t>(it->length())
            });
        }
    };

    if (filtered_indices) {
        for (size_t idx : *filtered_indices) {
            search_entry(idx);
        }
    } else {
        for (size_t i = 0; i < buffer.size(); ++i) {
            search_entry(i);
        }
    }
}

}  // namespace mbink
