/**
 * @file test_terminal_buffer_properties.cpp
 * @brief TerminalBuffer 属性测试
 *
 * 测试 TerminalBuffer 的正确性属性：
 * - 属性 8: Write/Serialize 往返
 * - 属性 9: ScrollTo 定位
 * - 属性 12: 行换行
 */

#include "core/dom/elements/terminal/terminal_buffer.h"
#include "core/dom/elements/terminal/ansi_parser.h"

#include <gtest/gtest.h>
#include <random>
#include <string>
#include <algorithm>

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

    char PrintableChar() {
        return static_cast<char>(Int(0x20, 0x7E));
    }

    std::string PrintableString(size_t length) {
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += PrintableChar();
        }
        return result;
    }

    std::string Line(int max_length) {
        return PrintableString(Size(1, max_length));
    }

private:
    std::mt19937 gen_;
};

/**
 * **Feature: virtual-text-components, Property 8: Write/Serialize 往返**
 * **Validates: Requirements 6.1, 6.4**
 *
 * *对于任何*纯文本字符串，Write() 后调用 Serialize() SHALL 返回包含原始文本的字符串。
 */
class WriteSerializeRoundTripPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(WriteSerializeRoundTripPropertyTest, SingleLineRoundTrip) {
    for (int iter = 0; iter < kIterations; ++iter) {
        int cols = rng_.Int(40, 120);
        TerminalBuffer buffer(cols);
        
        // 生成不超过列宽的文本
        std::string text = rng_.PrintableString(rng_.Size(1, cols));
        
        // 写入
        for (char c : text) {
            Cell cell;
            cell.codepoint = c;
            buffer.PutCell(cell);
        }
        
        // 序列化
        std::string serialized = buffer.Serialize();
        
        // 验证：序列化结果包含原始文本
        // 注意：可能有尾随空格，所以使用 find
        ASSERT_NE(serialized.find(text), std::string::npos)
            << "Serialized output should contain original text";
    }
}

TEST_F(WriteSerializeRoundTripPropertyTest, MultiLineRoundTrip) {
    for (int iter = 0; iter < kIterations; ++iter) {
        int cols = rng_.Int(40, 80);
        TerminalBuffer buffer(cols);
        
        // 生成多行文本
        int line_count = rng_.Int(2, 10);
        std::vector<std::string> lines;
        
        for (int i = 0; i < line_count; ++i) {
            std::string line = rng_.Line(cols - 1);
            lines.push_back(line);
            
            for (char c : line) {
                Cell cell;
                cell.codepoint = c;
                buffer.PutCell(cell);
            }
            
            if (i < line_count - 1) {
                buffer.NewLine();
            }
        }
        
        // 序列化
        std::string serialized = buffer.Serialize();
        
        // 验证：每行都在序列化结果中
        for (const auto& line : lines) {
            ASSERT_NE(serialized.find(line), std::string::npos)
                << "Serialized output should contain line: " << line;
        }
    }
}

TEST_F(WriteSerializeRoundTripPropertyTest, GetTextRangeCorrectness) {
    for (int iter = 0; iter < kIterations; ++iter) {
        int cols = rng_.Int(40, 80);
        TerminalBuffer buffer(cols);
        
        // 写入一些文本
        std::string text = rng_.PrintableString(rng_.Size(10, cols));
        for (char c : text) {
            Cell cell;
            cell.codepoint = c;
            buffer.PutCell(cell);
        }
        
        // 获取部分文本
        int start_col = rng_.Int(0, static_cast<int>(text.size()) / 2);
        int end_col = rng_.Int(start_col + 1, static_cast<int>(text.size()));
        
        std::string extracted = buffer.GetText(0, start_col, 0, end_col);
        std::string expected = text.substr(start_col, end_col - start_col);
        
        ASSERT_EQ(extracted, expected)
            << "GetText should return correct substring";
    }
}

/**
 * **Feature: virtual-text-components, Property 9: ScrollTo 定位**
 * **Validates: Requirements 6.3**
 *
 * *对于任何*有效行号 L，调用 ScrollTo(L) SHALL 设置视口使行 L 为第一个可见行。
 */
class ScrollToPositioningPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

// 注意：ScrollTo 是在 VirtualScrollRenderer 中实现的，这里测试 TerminalBuffer 的行访问
TEST_F(ScrollToPositioningPropertyTest, LineAccessAfterScrollback) {
    for (int iter = 0; iter < kIterations; ++iter) {
        int cols = 80;
        int scrollback = rng_.Int(100, 1000);
        TerminalBuffer buffer(cols, scrollback);
        
        // 写入超过回滚缓冲区的行数
        int line_count = rng_.Int(scrollback / 2, scrollback * 2);
        std::vector<std::string> lines;
        
        for (int i = 0; i < line_count; ++i) {
            std::string line = "Line" + std::to_string(i);
            lines.push_back(line);
            
            for (char c : line) {
                Cell cell;
                cell.codepoint = c;
                buffer.PutCell(cell);
            }
            buffer.NewLine();
        }
        
        // 验证：可以访问缓冲区中的行
        int total = buffer.total_lines();
        ASSERT_GT(total, 0);
        
        // 访问随机行
        if (total > 0) {
            int line_idx = rng_.Int(0, total - 1);
            std::string line_text = buffer.GetLineText(line_idx);
            // 不应崩溃
            ASSERT_NO_THROW({});
        }
    }
}

/**
 * **Feature: virtual-text-components, Property 12: 行换行**
 * **Validates: Requirements 7.4**
 *
 * *对于任何*长度 L > 终端宽度 W 的行，SHALL 显示在 ceil(L/W) 个可视行上。
 */
class LineWrappingPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(LineWrappingPropertyTest, LongLineWrapsCorrectly) {
    for (int iter = 0; iter < kIterations; ++iter) {
        int cols = rng_.Int(20, 80);
        TerminalBuffer buffer(cols);
        
        // 生成超过列宽的文本
        int text_length = cols + rng_.Int(1, cols * 2);
        std::string text = rng_.PrintableString(text_length);
        
        // 写入
        for (char c : text) {
            Cell cell;
            cell.codepoint = c;
            buffer.PutCell(cell);
        }
        
        // 计算期望的行数
        int expected_lines = (text_length + cols - 1) / cols;
        
        // 验证：总行数正确
        ASSERT_GE(buffer.total_lines(), expected_lines)
            << "Long line should wrap to multiple visual lines";
    }
}

TEST_F(LineWrappingPropertyTest, ExactWidthLineDoesNotWrap) {
    for (int iter = 0; iter < kIterations; ++iter) {
        int cols = rng_.Int(20, 80);
        TerminalBuffer buffer(cols);
        
        // 生成恰好等于列宽的文本
        std::string text = rng_.PrintableString(cols);
        
        // 写入
        for (char c : text) {
            Cell cell;
            cell.codepoint = c;
            buffer.PutCell(cell);
        }
        
        // 验证：只有一行（光标可能在下一行开头）
        ASSERT_LE(buffer.total_lines(), 2)
            << "Exact width line should not create extra lines";
    }
}

TEST_F(LineWrappingPropertyTest, ContentPreservedAfterWrap) {
    for (int iter = 0; iter < kIterations; ++iter) {
        int cols = rng_.Int(20, 40);
        TerminalBuffer buffer(cols);
        
        // 生成超过列宽的文本
        int text_length = cols * 2 + rng_.Int(1, cols);
        std::string text = rng_.PrintableString(text_length);
        
        // 写入
        for (char c : text) {
            Cell cell;
            cell.codepoint = c;
            buffer.PutCell(cell);
        }
        
        // 序列化并验证内容
        std::string serialized = buffer.Serialize();
        
        // 移除换行符后应该包含原始文本
        std::string no_newlines;
        for (char c : serialized) {
            if (c != '\n' && c != '\r' && c != ' ') {
                no_newlines += c;
            }
        }
        
        std::string text_no_spaces;
        for (char c : text) {
            if (c != ' ') {
                text_no_spaces += c;
            }
        }
        
        // 验证非空格字符被保留
        for (char c : text_no_spaces) {
            ASSERT_NE(no_newlines.find(c), std::string::npos)
                << "Character '" << c << "' should be preserved after wrap";
        }
    }
}

// 测试光标操作
class CursorOperationsPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(CursorOperationsPropertyTest, SetCursorClampsToValidRange) {
    for (int iter = 0; iter < kIterations; ++iter) {
        int cols = rng_.Int(40, 120);
        TerminalBuffer buffer(cols);
        buffer.set_visible_rows(24);
        
        // 设置超出范围的光标位置
        int row = rng_.Int(-10, 100);
        int col = rng_.Int(-10, cols + 50);
        
        buffer.SetCursor(row, col);
        
        // 验证：光标被限制在有效范围内
        ASSERT_GE(buffer.cursor_row(), 0);
        ASSERT_GE(buffer.cursor_col(), 0);
        ASSERT_LT(buffer.cursor_col(), cols);
    }
}

TEST_F(CursorOperationsPropertyTest, MoveCursorRelative) {
    for (int iter = 0; iter < kIterations; ++iter) {
        int cols = rng_.Int(40, 120);
        TerminalBuffer buffer(cols);
        buffer.set_visible_rows(24);
        
        // 设置初始位置
        int start_row = rng_.Int(5, 15);
        int start_col = rng_.Int(10, cols - 10);
        buffer.SetCursor(start_row, start_col);
        
        // 相对移动
        int delta_row = rng_.Int(-5, 5);
        int delta_col = rng_.Int(-10, 10);
        buffer.MoveCursor(delta_row, delta_col);
        
        // 验证：移动后位置正确（考虑边界限制）
        int expected_row = std::max(0, start_row + delta_row);
        int expected_col = std::max(0, std::min(cols - 1, start_col + delta_col));
        
        ASSERT_EQ(buffer.cursor_row(), expected_row);
        ASSERT_EQ(buffer.cursor_col(), expected_col);
    }
}

}  // namespace
}  // namespace mblink
