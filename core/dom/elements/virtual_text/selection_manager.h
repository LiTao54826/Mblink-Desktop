/**
 * @file selection_manager.h
 * @brief 文本选择管理器
 *
 * 提供文本选择功能，支持拖动选择、单词选择和行选择。
 */

#pragma once

#include <functional>
#include <string>

namespace mblink {
namespace virtual_text {

/**
 * @brief 选择范围
 */
struct SelectionRange {
    int start_line = 0;
    int start_col = 0;
    int end_line = 0;
    int end_col = 0;

    /**
     * @brief 检查选择是否为空
     * @return 为空返回 true
     */
    bool IsEmpty() const {
        return start_line == end_line && start_col == end_col;
    }

    /**
     * @brief 检查指定位置是否在选择范围内
     * @param line 行号
     * @param col 列号
     * @return 在范围内返回 true
     */
    bool Contains(int line, int col) const;

    /**
     * @brief 规范化选择范围
     *
     * 确保 start 在 end 之前。
     */
    void Normalize();

    /**
     * @brief 获取规范化后的选择范围
     * @return 规范化的选择范围
     */
    SelectionRange Normalized() const;
};

/**
 * @brief 文本选择管理器
 *
 * SelectionManager 管理文本选择状态，支持：
 * - 鼠标拖动选择
 * - 双击选择单词
 * - 三击选择整行
 * - 全选
 */
class SelectionManager {
public:
    SelectionManager() = default;

    // === 选择操作 ===

    /**
     * @brief 开始选择
     * @param line 起始行
     * @param col 起始列
     */
    void StartSelection(int line, int col);

    /**
     * @brief 更新选择（拖动时调用）
     * @param line 当前行
     * @param col 当前列
     */
    void UpdateSelection(int line, int col);

    /**
     * @brief 结束选择
     */
    void EndSelection();

    /**
     * @brief 清除选择
     */
    void ClearSelection();

    // === 快捷选择 ===

    /**
     * @brief 选择单词（双击）
     * @param line 行号
     * @param col 列号
     * @param line_text 该行的文本内容
     */
    void SelectWord(int line, int col, const std::string& line_text);

    /**
     * @brief 选择整行（三击）
     * @param line 行号
     * @param line_length 该行的长度
     */
    void SelectLine(int line, int line_length);

    /**
     * @brief 全选
     * @param total_lines 总行数
     * @param last_line_length 最后一行的长度
     */
    void SelectAll(int total_lines, int last_line_length);

    // === 查询 ===

    /**
     * @brief 检查是否有选择
     * @return 有选择返回 true
     */
    bool HasSelection() const;

    /**
     * @brief 获取选择范围
     * @return 选择范围（已规范化）
     */
    SelectionRange GetSelection() const;

    /**
     * @brief 检查指定位置是否被选中
     * @param line 行号
     * @param col 列号
     * @return 被选中返回 true
     */
    bool IsSelected(int line, int col) const;

    /**
     * @brief 检查是否正在选择中
     * @return 正在选择返回 true
     */
    bool IsSelecting() const { return is_selecting_; }

    // === 文本提取 ===

    /**
     * @brief 获取选中的文本
     * @tparam GetLineFunc 获取行文本的函数类型
     * @param get_line 获取指定行文本的函数 (int line) -> std::string
     * @return 选中的文本
     */
    template <typename GetLineFunc>
    std::string GetSelectedText(GetLineFunc get_line) const {
        if (!HasSelection()) {
            return "";
        }

        SelectionRange sel = GetSelection();
        std::string result;

        for (int line = sel.start_line; line <= sel.end_line; ++line) {
            std::string line_text = get_line(line);
            
            int start_col = (line == sel.start_line) ? sel.start_col : 0;
            int end_col = (line == sel.end_line) ? sel.end_col : static_cast<int>(line_text.size());
            
            // 确保范围有效
            start_col = std::max(0, std::min(start_col, static_cast<int>(line_text.size())));
            end_col = std::max(start_col, std::min(end_col, static_cast<int>(line_text.size())));
            
            if (start_col < end_col) {
                result += line_text.substr(start_col, end_col - start_col);
            }
            
            // 添加换行符（除了最后一行）
            if (line < sel.end_line) {
                result += '\n';
            }
        }

        return result;
    }

private:
    SelectionRange selection_;
    bool is_selecting_ = false;
    bool has_selection_ = false;

    /**
     * @brief 检查字符是否为单词字符
     * @param c 字符
     * @return 是单词字符返回 true
     */
    static bool IsWordChar(char c);
};

}  // namespace virtual_text
}  // namespace mblink
