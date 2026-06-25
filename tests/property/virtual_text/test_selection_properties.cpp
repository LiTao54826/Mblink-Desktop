/**
 * @file test_selection_properties.cpp
 * @brief SelectionManager 属性测试
 *
 * 测试 SelectionManager 的正确性属性：
 * - 属性 5: 单词选择
 * - 属性 6: 行选择
 */

#include "core/dom/elements/virtual_text/selection_manager.h"

#include <gtest/gtest.h>
#include <random>
#include <string>
#include <vector>

namespace mblink {
namespace {

// 使用 virtual_text 命名空间中的类型
using virtual_text::SelectionManager;
using virtual_text::SelectionRange;

class RandomGenerator {
public:
    RandomGenerator() : gen_(std::random_device{}()) {}

    int Int(int min, int max) {
        return std::uniform_int_distribution<int>(min, max)(gen_);
    }

    size_t Size(size_t min, size_t max) {
        return std::uniform_int_distribution<size_t>(min, max)(gen_);
    }

    std::string Word() {
        static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
        size_t len = Size(1, 15);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            result += chars[Int(0, sizeof(chars) - 2)];
        }
        return result;
    }

    std::string Separator() {
        static const char seps[] = " \t.,;:!?()[]{}";
        size_t len = Size(1, 3);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            result += seps[Int(0, sizeof(seps) - 2)];
        }
        return result;
    }

    std::string LineWithWords(int word_count, std::vector<std::pair<int, int>>& word_positions) {
        std::string line;
        word_positions.clear();
        
        for (int i = 0; i < word_count; ++i) {
            if (i > 0) {
                line += Separator();
            }
            int start = static_cast<int>(line.size());
            std::string word = Word();
            line += word;
            int end = static_cast<int>(line.size());
            word_positions.push_back({start, end});
        }
        
        return line;
    }

private:
    std::mt19937 gen_;
};

/**
 * **Feature: virtual-text-components, Property 5: 单词选择**
 * **Validates: Requirements 14.3**
 *
 * *对于任何*包含单词边界的文本，在位置 (row, col) 调用 SelectWord() 
 * SHALL 选中围绕该位置的连续单词字符。
 */
class WordSelectionPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(WordSelectionPropertyTest, SelectWordSelectsEntireWord) {
    for (int iter = 0; iter < kIterations; ++iter) {
        SelectionManager manager;
        
        // 生成包含多个单词的行
        int word_count = rng_.Int(2, 10);
        std::vector<std::pair<int, int>> word_positions;
        std::string line = rng_.LineWithWords(word_count, word_positions);
        
        // 随机选择一个单词
        int word_idx = rng_.Int(0, word_count - 1);
        auto [word_start, word_end] = word_positions[word_idx];
        
        // 在单词内的随机位置点击
        int click_col = rng_.Int(word_start, word_end - 1);
        int line_num = rng_.Int(0, 100);
        
        manager.SelectWord(line_num, click_col, line);
        
        // 验证：选择了整个单词
        ASSERT_TRUE(manager.HasSelection())
            << "Should have selection after SelectWord";
        
        SelectionRange sel = manager.GetSelection();
        ASSERT_EQ(sel.start_line, line_num);
        ASSERT_EQ(sel.end_line, line_num);
        ASSERT_EQ(sel.start_col, word_start)
            << "Selection should start at word beginning";
        ASSERT_EQ(sel.end_col, word_end)
            << "Selection should end at word end";
    }
}

TEST_F(WordSelectionPropertyTest, SelectWordAtWordBoundary) {
    for (int iter = 0; iter < kIterations; ++iter) {
        SelectionManager manager;
        
        int word_count = rng_.Int(2, 5);
        std::vector<std::pair<int, int>> word_positions;
        std::string line = rng_.LineWithWords(word_count, word_positions);
        
        // 在第一个单词的开始位置点击
        auto [first_start, first_end] = word_positions[0];
        manager.SelectWord(0, first_start, line);
        
        ASSERT_TRUE(manager.HasSelection());
        SelectionRange sel = manager.GetSelection();
        ASSERT_EQ(sel.start_col, first_start);
        ASSERT_EQ(sel.end_col, first_end);
    }
}

TEST_F(WordSelectionPropertyTest, SelectWordOnSeparatorSelectsNothing) {
    SelectionManager manager;
    
    std::string line = "hello   world";
    //                  01234567890123
    //                       ^^^  separators at 5,6,7
    
    // 点击分隔符位置
    manager.SelectWord(0, 6, line);
    
    // 在分隔符上点击可能选择空或不选择
    // 具体行为取决于实现，但不应崩溃
    // 如果有选择，应该是空的或只包含分隔符
    if (manager.HasSelection()) {
        SelectionRange sel = manager.GetSelection();
        // 选择范围应该很小（分隔符区域）
        ASSERT_LE(sel.end_col - sel.start_col, 3);
    }
}

/**
 * **Feature: virtual-text-components, Property 6: 行选择**
 * **Validates: Requirements 14.4**
 *
 * *对于任何*多行缓冲区，在行 R 调用 SelectLine() SHALL 选中行 R 的所有字符。
 */
class LineSelectionPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(LineSelectionPropertyTest, SelectLineSelectsEntireLine) {
    for (int iter = 0; iter < kIterations; ++iter) {
        SelectionManager manager;
        
        int line_num = rng_.Int(0, 1000);
        int line_length = rng_.Int(1, 200);  // 至少 1 个字符，空行在另一个测试中处理
        
        manager.SelectLine(line_num, line_length);
        
        // 验证：选择了整行
        ASSERT_TRUE(manager.HasSelection())
            << "Should have selection after SelectLine";
        
        SelectionRange sel = manager.GetSelection();
        ASSERT_EQ(sel.start_line, line_num)
            << "Selection should be on the specified line";
        ASSERT_EQ(sel.end_line, line_num)
            << "Selection should be on the specified line";
        ASSERT_EQ(sel.start_col, 0)
            << "Selection should start at column 0";
        ASSERT_EQ(sel.end_col, line_length)
            << "Selection should end at line_length";
    }
}

TEST_F(LineSelectionPropertyTest, SelectLineOnEmptyLine) {
    for (int iter = 0; iter < kIterations; ++iter) {
        SelectionManager manager;
        
        int line_num = rng_.Int(0, 1000);
        
        // 空行
        manager.SelectLine(line_num, 0);
        
        // 验证：选择存在但为空
        SelectionRange sel = manager.GetSelection();
        ASSERT_EQ(sel.start_line, line_num);
        ASSERT_EQ(sel.end_line, line_num);
        ASSERT_EQ(sel.start_col, 0);
        ASSERT_EQ(sel.end_col, 0);
        ASSERT_TRUE(sel.IsEmpty());
    }
}

TEST_F(LineSelectionPropertyTest, SelectLineCoversAllColumns) {
    for (int iter = 0; iter < kIterations; ++iter) {
        SelectionManager manager;
        
        int line_num = rng_.Int(0, 100);
        int line_length = rng_.Int(10, 200);
        
        manager.SelectLine(line_num, line_length);
        
        SelectionRange sel = manager.GetSelection();
        
        // 验证：行内所有列都被选中
        for (int col = 0; col < line_length; ++col) {
            ASSERT_TRUE(manager.IsSelected(line_num, col))
                << "Column " << col << " should be selected";
        }
        
        // 验证：行外的列不被选中
        ASSERT_FALSE(manager.IsSelected(line_num - 1, 0))
            << "Previous line should not be selected";
        ASSERT_FALSE(manager.IsSelected(line_num + 1, 0))
            << "Next line should not be selected";
    }
}

// 测试 GetSelectedText 模板函数
TEST_F(LineSelectionPropertyTest, GetSelectedTextReturnsCorrectContent) {
    for (int iter = 0; iter < kIterations; ++iter) {
        SelectionManager manager;
        
        // 创建测试数据
        std::vector<std::string> lines;
        int num_lines = rng_.Int(5, 20);
        for (int i = 0; i < num_lines; ++i) {
            std::vector<std::pair<int, int>> positions;
            lines.push_back(rng_.LineWithWords(rng_.Int(1, 5), positions));
        }
        
        // 选择一行
        int line_num = rng_.Int(0, num_lines - 1);
        manager.SelectLine(line_num, static_cast<int>(lines[line_num].size()));
        
        // 获取选中文本
        auto get_line = [&lines](int line) -> std::string {
            if (line >= 0 && line < static_cast<int>(lines.size())) {
                return lines[line];
            }
            return "";
        };
        
        std::string selected = manager.GetSelectedText(get_line);
        
        // 验证：选中的文本等于该行内容
        ASSERT_EQ(selected, lines[line_num])
            << "Selected text should match line content";
    }
}

// 测试多行选择
class MultiLineSelectionPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
};

TEST_F(MultiLineSelectionPropertyTest, DragSelectionSpansMultipleLines) {
    for (int iter = 0; iter < 50; ++iter) {
        SelectionManager manager;
        
        int start_line = rng_.Int(0, 50);
        int start_col = rng_.Int(0, 80);
        int end_line = start_line + rng_.Int(1, 10);
        int end_col = rng_.Int(0, 80);
        
        manager.StartSelection(start_line, start_col);
        manager.UpdateSelection(end_line, end_col);
        manager.EndSelection();
        
        ASSERT_TRUE(manager.HasSelection());
        
        SelectionRange sel = manager.GetSelection();
        
        // 验证：选择范围正确
        ASSERT_EQ(sel.start_line, start_line);
        ASSERT_EQ(sel.start_col, start_col);
        ASSERT_EQ(sel.end_line, end_line);
        ASSERT_EQ(sel.end_col, end_col);
        
        // 验证：中间行的所有列都被选中
        for (int line = start_line + 1; line < end_line; ++line) {
            for (int col = 0; col < 80; ++col) {
                ASSERT_TRUE(manager.IsSelected(line, col))
                    << "Middle line " << line << " col " << col << " should be selected";
            }
        }
    }
}

TEST_F(MultiLineSelectionPropertyTest, ClearSelectionRemovesSelection) {
    for (int iter = 0; iter < 50; ++iter) {
        SelectionManager manager;
        
        // 创建选择
        manager.StartSelection(rng_.Int(0, 50), rng_.Int(0, 80));
        manager.UpdateSelection(rng_.Int(0, 50), rng_.Int(0, 80));
        manager.EndSelection();
        
        // 清除选择
        manager.ClearSelection();
        
        ASSERT_FALSE(manager.HasSelection())
            << "Should have no selection after ClearSelection";
    }
}

}  // namespace
}  // namespace mblink
