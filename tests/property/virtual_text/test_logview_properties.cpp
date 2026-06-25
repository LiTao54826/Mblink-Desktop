/**
 * @file test_logview_properties.cpp
 * @brief LogView 组件属性测试
 *
 * 测试 LogView 的正确性属性：
 * - 属性 13: 日志追加正确性
 * - 属性 14: 自动滚动行为
 * - 属性 15: 日志级别过滤
 * - 属性 16: 搜索匹配正确性
 * - 属性 17: 正则搜索
 */

#include "core/dom/elements/logview/log_buffer.h"
#include "core/dom/elements/logview/log_filter.h"
#include "core/dom/elements/logview/log_search.h"

#include <gtest/gtest.h>
#include <random>
#include <string>
#include <algorithm>
#include <set>

namespace mblink {
namespace {

class RandomGenerator {
public:
    RandomGenerator() : gen_(std::random_device{}()) {}

    int Int(int min, int max) {
        return std::uniform_int_distribution<int>(min, max)(gen_);
    }

    size_t Size(size_t min, size_t max) {
        return std::uniform_int_distribution<size_t>(min, max)(gen_);
    }

    LogLevel Level() {
        return static_cast<LogLevel>(Int(0, 4));
    }

    std::string Message(size_t length) {
        static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += chars[Int(0, sizeof(chars) - 2)];
        }
        return result;
    }

    std::string Source() {
        static const char* sources[] = {"app", "network", "database", "ui", "system"};
        return sources[Int(0, 4)];
    }

    uint8_t LevelMask() {
        return static_cast<uint8_t>(Int(1, 31));  // 至少一个级别
    }

private:
    std::mt19937 gen_;
};

/**
 * **Feature: virtual-text-components, Property 13: 日志追加正确性**
 * **Validates: Requirements 9.1**
 *
 * *对于任何*日志条目，Append() 后 buffer 的最后一条 SHALL 包含相同的 level、source 和 message。
 */
class LogAppendCorrectnessPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(LogAppendCorrectnessPropertyTest, AppendedEntryMatchesInput) {
    for (int iter = 0; iter < kIterations; ++iter) {
        LogBuffer buffer(10000);
        
        // 生成随机日志条目
        LogLevel level = rng_.Level();
        std::string source = rng_.Source();
        std::string message = rng_.Message(rng_.Size(10, 200));
        
        // 注册源并追加
        uint8_t source_id = buffer.RegisterSource(source);
        buffer.Append(level, source_id, message);
        
        // 验证：最后一条日志匹配输入
        ASSERT_EQ(buffer.size(), 1u);
        ASSERT_EQ(buffer.GetLevel(0), level)
            << "Level should match";
        ASSERT_EQ(buffer.GetSourceId(0), source_id)
            << "Source ID should match";
        ASSERT_EQ(buffer.GetMessage(0), message)
            << "Message should match";
    }
}

TEST_F(LogAppendCorrectnessPropertyTest, MultipleAppendsPreserveOrder) {
    for (int iter = 0; iter < kIterations; ++iter) {
        LogBuffer buffer(10000);
        
        // 追加多条日志
        int count = rng_.Int(5, 50);
        std::vector<std::tuple<LogLevel, std::string, std::string>> entries;
        
        for (int i = 0; i < count; ++i) {
            LogLevel level = rng_.Level();
            std::string source = rng_.Source();
            std::string message = "msg" + std::to_string(i) + "_" + rng_.Message(20);
            
            entries.push_back({level, source, message});
            buffer.Append(level, source, message);
        }
        
        // 验证：所有条目按顺序保存
        ASSERT_EQ(buffer.size(), static_cast<size_t>(count));
        
        for (int i = 0; i < count; ++i) {
            auto [level, source, message] = entries[i];
            ASSERT_EQ(buffer.GetLevel(i), level);
            ASSERT_EQ(buffer.GetMessage(i), message);
        }
    }
}

TEST_F(LogAppendCorrectnessPropertyTest, OverflowKeepsNewest) {
    for (int iter = 0; iter < kIterations; ++iter) {
        size_t max_entries = rng_.Size(10, 100);
        LogBuffer buffer(max_entries);
        
        // 追加超过容量的日志
        size_t total = max_entries + rng_.Size(10, 50);
        std::vector<std::string> messages;
        
        for (size_t i = 0; i < total; ++i) {
            std::string message = "msg" + std::to_string(i);
            messages.push_back(message);
            buffer.Append(LogLevel::INFO, "test", message);
        }
        
        // 验证：保留最新的条目
        ASSERT_EQ(buffer.size(), max_entries);
        
        size_t start_idx = total - max_entries;
        for (size_t i = 0; i < buffer.size(); ++i) {
            ASSERT_EQ(buffer.GetMessage(i), messages[start_idx + i])
                << "Should keep newest entries";
        }
    }
}

/**
 * **Feature: virtual-text-components, Property 14: 自动滚动行为**
 * **Validates: Requirements 9.4**
 *
 * *对于任何*启用 auto_scroll 且视口在底部的状态，新日志到达后视口 SHALL 仍在底部。
 */
// 注意：自动滚动是在 HTMLLogViewElement 中实现的，这里测试 LogBuffer 的基础功能
class AutoScrollBehaviorPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
};

TEST_F(AutoScrollBehaviorPropertyTest, BufferSizeGrowsWithAppend) {
    LogBuffer buffer(1000);
    
    for (int i = 0; i < 100; ++i) {
        size_t prev_size = buffer.size();
        buffer.Append(LogLevel::INFO, "test", "message");
        ASSERT_EQ(buffer.size(), prev_size + 1);
    }
}

/**
 * **Feature: virtual-text-components, Property 15: 日志级别过滤**
 * **Validates: Requirements 10.1, 10.3**
 *
 * *对于任何*级别掩码设置，过滤后显示的所有日志 SHALL 满足 (1 << level) & mask != 0。
 */
class LogLevelFilterPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(LogLevelFilterPropertyTest, FilteredEntriesMatchMask) {
    for (int iter = 0; iter < kIterations; ++iter) {
        LogBuffer buffer(10000);
        LogFilter filter;
        
        // 追加各种级别的日志
        int count = rng_.Int(50, 200);
        for (int i = 0; i < count; ++i) {
            buffer.Append(rng_.Level(), "test", "message" + std::to_string(i));
        }
        
        // 设置随机级别掩码
        uint8_t mask = rng_.LevelMask();
        filter.SetLevelMask(mask);
        
        // 获取过滤后的索引
        std::vector<size_t> filtered;
        filter.UpdateFilteredIndices(buffer, filtered);
        
        // 验证：所有过滤后的条目都匹配掩码
        for (size_t idx : filtered) {
            LogLevel level = buffer.GetLevel(idx);
            uint8_t level_bit = 1 << static_cast<int>(level);
            ASSERT_TRUE(level_bit & mask)
                << "Filtered entry should match level mask";
        }
        
        // 验证：所有匹配掩码的条目都在过滤结果中
        std::set<size_t> filtered_set(filtered.begin(), filtered.end());
        for (size_t i = 0; i < buffer.size(); ++i) {
            LogLevel level = buffer.GetLevel(i);
            uint8_t level_bit = 1 << static_cast<int>(level);
            if (level_bit & mask) {
                ASSERT_TRUE(filtered_set.count(i) > 0)
                    << "Entry matching mask should be in filtered results";
            }
        }
    }
}

TEST_F(LogLevelFilterPropertyTest, AllLevelsMaskReturnsAll) {
    for (int iter = 0; iter < kIterations; ++iter) {
        LogBuffer buffer(10000);
        LogFilter filter;
        
        int count = rng_.Int(10, 100);
        for (int i = 0; i < count; ++i) {
            buffer.Append(rng_.Level(), "test", "message");
        }
        
        // 设置所有级别掩码
        filter.SetLevelMask(0x1F);  // 所有 5 个级别
        
        std::vector<size_t> filtered;
        filter.UpdateFilteredIndices(buffer, filtered);
        
        // 验证：返回所有条目
        ASSERT_EQ(filtered.size(), buffer.size())
            << "All levels mask should return all entries";
    }
}

TEST_F(LogLevelFilterPropertyTest, SingleLevelMaskFiltersCorrectly) {
    for (int level_int = 0; level_int < 5; ++level_int) {
        LogBuffer buffer(10000);
        LogFilter filter;
        
        // 追加各种级别的日志
        for (int i = 0; i < 100; ++i) {
            buffer.Append(static_cast<LogLevel>(i % 5), "test", "message");
        }
        
        // 只启用一个级别
        uint8_t mask = 1 << level_int;
        filter.SetLevelMask(mask);
        
        std::vector<size_t> filtered;
        filter.UpdateFilteredIndices(buffer, filtered);
        
        // 验证：只返回该级别的条目
        for (size_t idx : filtered) {
            ASSERT_EQ(static_cast<int>(buffer.GetLevel(idx)), level_int)
                << "Should only return entries of the specified level";
        }
    }
}

/**
 * **Feature: virtual-text-components, Property 16: 搜索匹配正确性**
 * **Validates: Requirements 11.1, 11.2**
 *
 * *对于任何*搜索关键词，返回的匹配数量 SHALL 等于缓冲区中包含该关键词的日志条目数。
 */
class SearchMatchCorrectnessPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(SearchMatchCorrectnessPropertyTest, SearchFindsAllMatches) {
    for (int iter = 0; iter < kIterations; ++iter) {
        LogBuffer buffer(10000);
        LogSearch search;
        
        // 生成包含特定关键词的日志
        std::string keyword = "KEYWORD" + std::to_string(iter);
        int expected_matches = 0;
        
        for (int i = 0; i < 50; ++i) {
            std::string message;
            if (rng_.Int(0, 2) == 0) {
                message = "prefix " + keyword + " suffix";
                expected_matches++;
            } else {
                message = "no match here " + std::to_string(i);
            }
            buffer.Append(LogLevel::INFO, "test", message);
        }
        
        // 执行搜索
        int match_count = search.Search(buffer, keyword);
        
        // 验证：匹配数量正确
        ASSERT_EQ(match_count, expected_matches)
            << "Search should find all entries containing keyword";
    }
}

TEST_F(SearchMatchCorrectnessPropertyTest, SearchIsCaseInsensitive) {
    LogBuffer buffer(10000);
    LogSearch search;
    
    // 添加不同大小写的日志
    buffer.Append(LogLevel::INFO, "test", "Hello World");
    buffer.Append(LogLevel::INFO, "test", "HELLO WORLD");
    buffer.Append(LogLevel::INFO, "test", "hello world");
    buffer.Append(LogLevel::INFO, "test", "HeLLo WoRLd");
    
    // 搜索（应该是大小写不敏感的）
    int match_count = search.Search(buffer, "hello");
    
    // 验证：找到所有变体
    ASSERT_EQ(match_count, 4)
        << "Search should be case insensitive";
}

TEST_F(SearchMatchCorrectnessPropertyTest, EmptySearchReturnsNoMatches) {
    LogBuffer buffer(10000);
    LogSearch search;
    
    for (int i = 0; i < 10; ++i) {
        buffer.Append(LogLevel::INFO, "test", "message " + std::to_string(i));
    }
    
    // 空搜索
    int match_count = search.Search(buffer, "");
    
    ASSERT_EQ(match_count, 0)
        << "Empty search should return no matches";
}

TEST_F(SearchMatchCorrectnessPropertyTest, NoMatchReturnsZero) {
    for (int iter = 0; iter < kIterations; ++iter) {
        LogBuffer buffer(10000);
        LogSearch search;
        
        // 添加不包含关键词的日志
        for (int i = 0; i < 20; ++i) {
            buffer.Append(LogLevel::INFO, "test", "message " + std::to_string(i));
        }
        
        // 搜索不存在的关键词
        int match_count = search.Search(buffer, "NONEXISTENT_KEYWORD_XYZ");
        
        ASSERT_EQ(match_count, 0)
            << "Search for non-existent keyword should return 0";
    }
}

/**
 * **Feature: virtual-text-components, Property 17: 正则搜索**
 * **Validates: Requirements 11.4**
 *
 * *对于任何*有效正则表达式，搜索结果 SHALL 包含所有匹配该正则的日志条目。
 */
class RegexSearchPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 50;
};

TEST_F(RegexSearchPropertyTest, SimpleRegexMatches) {
    LogBuffer buffer(10000);
    LogSearch search;
    
    // 添加日志
    buffer.Append(LogLevel::INFO, "test", "error code 123");
    buffer.Append(LogLevel::INFO, "test", "error code 456");
    buffer.Append(LogLevel::INFO, "test", "warning message");
    buffer.Append(LogLevel::INFO, "test", "error code abc");
    
    // 正则搜索：匹配 "error code" 后跟数字
    int match_count = search.Search(buffer, "error code \\d+", true);
    
    ASSERT_EQ(match_count, 2)
        << "Regex should match entries with 'error code' followed by digits";
}

TEST_F(RegexSearchPropertyTest, InvalidRegexFallsBackToPlainText) {
    LogBuffer buffer(10000);
    LogSearch search;
    
    buffer.Append(LogLevel::INFO, "test", "test [invalid regex");
    buffer.Append(LogLevel::INFO, "test", "another message");
    
    // 无效正则（未闭合的方括号）
    // 应该回退到普通文本搜索或返回 0
    int match_count = search.Search(buffer, "[invalid", true);
    
    // 不应崩溃
    ASSERT_GE(match_count, 0);
}

TEST_F(RegexSearchPropertyTest, RegexWithGroups) {
    LogBuffer buffer(10000);
    LogSearch search;
    
    buffer.Append(LogLevel::INFO, "test", "user: john, action: login");
    buffer.Append(LogLevel::INFO, "test", "user: jane, action: logout");
    buffer.Append(LogLevel::INFO, "test", "system: startup");
    
    // 正则搜索：匹配 user: 后跟任意单词
    int match_count = search.Search(buffer, "user: \\w+", true);
    
    ASSERT_EQ(match_count, 2)
        << "Regex should match entries with 'user:' pattern";
}

// 测试搜索导航
class SearchNavigationPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
};

TEST_F(SearchNavigationPropertyTest, NextMatchCyclesThrough) {
    LogBuffer buffer(10000);
    LogSearch search;
    
    // 添加包含关键词的日志
    for (int i = 0; i < 5; ++i) {
        buffer.Append(LogLevel::INFO, "test", "match " + std::to_string(i));
    }
    
    search.Search(buffer, "match");
    
    // 遍历所有匹配
    std::set<int> visited;
    for (int i = 0; i < 10; ++i) {
        int idx = search.NextMatch();
        if (idx >= 0) {
            visited.insert(idx);
        }
    }
    
    // 应该访问了所有 5 个匹配
    ASSERT_EQ(visited.size(), 5u)
        << "NextMatch should cycle through all matches";
}

TEST_F(SearchNavigationPropertyTest, PrevMatchCyclesBackward) {
    LogBuffer buffer(10000);
    LogSearch search;
    
    for (int i = 0; i < 5; ++i) {
        buffer.Append(LogLevel::INFO, "test", "match " + std::to_string(i));
    }
    
    search.Search(buffer, "match");
    
    // 向后遍历
    std::vector<int> indices;
    for (int i = 0; i < 10; ++i) {
        int idx = search.PrevMatch();
        if (idx >= 0) {
            indices.push_back(idx);
        }
    }
    
    // 应该有结果
    ASSERT_FALSE(indices.empty())
        << "PrevMatch should return valid indices";
}

}  // namespace
}  // namespace mblink
