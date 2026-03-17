/**
 * @file contenteditable_handler.h
 * @brief ContentEditable 输入处理器
 *
 * 功能：
 * - 处理可编辑元素的文本输入
 * - 处理删除操作（Backspace/Delete）
 * - 处理换行（Enter）
 * - 支持 execCommand API
 * - 分发 beforeinput/input 事件
 * - 撤销/重做支持
 *
 * 参考：https://w3c.github.io/editing/docs/execCommand/
 */

#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <deque>
#include <unordered_map>

#include "core/editing/input_edit_state.h"

namespace lightui {

// 前向声明
class Document;
class Element;
class Node;
class Range;
class SelectionManager;
class Selection;
struct KeyEvent;

/**
 * @brief 撤销状态结构
 */
struct UndoState {
    std::string innerHTML;                    // 元素的 innerHTML
    std::weak_ptr<Element> element;           // 可编辑元素
    std::weak_ptr<Node> anchor_node;          // 光标锚点节点
    int anchor_offset = 0;                    // 光标锚点偏移量
    std::weak_ptr<Node> focus_node;           // 光标焦点节点
    int focus_offset = 0;                     // 光标焦点偏移量
    bool is_collapsed = true;                 // 选择是否折叠
};

/**
 * @brief ContentEditable 输入处理器
 *
 * 处理可编辑元素的所有输入操作
 */
class ContentEditableHandler {
public:
    /**
     * @brief 构造函数
     * @param selection_manager 选择管理器
     */
    explicit ContentEditableHandler(SelectionManager* selection_manager);

    /**
     * @brief 析构函数
     */
    ~ContentEditableHandler();

    // ========== 可编辑性检查 ==========

    /**
     * @brief 检查元素是否可编辑
     * @param element 要检查的元素
     * @return true 如果元素可编辑
     */
    bool IsEditable(std::shared_ptr<Element> element) const;

    /**
     * @brief 检查元素的 isContentEditable 属性
     * @param element 要检查的元素
     * @return true 如果元素可编辑
     */
    bool IsContentEditable(std::shared_ptr<Element> element) const;

    // ========== 输入处理 ==========

    /**
     * @brief 处理文本输入
     * @param target 目标元素
     * @param text 输入的文本
     * @return true 如果输入被处理
     */
    bool HandleTextInput(std::shared_ptr<Element> target, const std::string& text);

    /**
     * @brief 处理键盘按下事件
     * @param target 目标元素
     * @param key_code 键码
     * @param ctrl_key 是否按下 Ctrl
     * @param shift_key 是否按下 Shift
     * @param alt_key 是否按下 Alt
     * @return true 如果事件被处理
     */
    bool HandleKeyDown(std::shared_ptr<Element> target,
                       int key_code,
                       bool ctrl_key,
                       bool shift_key,
                       bool alt_key);

    // ========== 编辑操作 ==========

    /**
     * @brief 在光标位置插入文本
     * @param document 文档
     * @param text 要插入的文本
     * @return true 如果插入成功
     */
    bool InsertText(std::shared_ptr<Document> document, const std::string& text);

    /**
     * @brief 删除选中的内容
     * @param document 文档
     * @return true 如果删除成功
     */
    bool DeleteSelection(std::shared_ptr<Document> document);

    /**
     * @brief 删除字符
     * @param document 文档
     * @param forward true 删除光标后的字符（Delete），false 删除光标前的字符（Backspace）
     * @return true 如果删除成功
     */
    bool DeleteCharacter(std::shared_ptr<Document> document, bool forward);

    /**
     * @brief 插入换行
     * @param document 文档
     * @return true 如果插入成功
     */
    bool InsertLineBreak(std::shared_ptr<Document> document);

    // ========== IME Composition ==========

    bool StartComposition(std::shared_ptr<Document> document,
                          const std::string& text,
                          int start,
                          int end);
    bool UpdateComposition(std::shared_ptr<Document> document,
                           const std::string& text,
                           int start,
                           int end);
    bool CommitComposition(std::shared_ptr<Document> document, const std::string& text);
    bool CancelComposition(std::shared_ptr<Document> document);
    bool HasActiveComposition(std::shared_ptr<Document> document) const;
    CompositionState GetCompositionState(std::shared_ptr<Document> document) const;

    // ========== execCommand 支持 ==========

    /**
     * @brief 执行编辑命令
     * @param document 文档
     * @param command 命令名称
     * @param value 命令参数（可选）
     * @return true 如果命令执行成功
     */
    bool ExecCommand(std::shared_ptr<Document> document,
                     const std::string& command,
                     const std::string& value = "");

    /**
     * @brief 查询命令状态
     * @param document 文档
     * @param command 命令名称
     * @return true 如果命令处于激活状态
     */
    bool QueryCommandState(std::shared_ptr<Document> document,
                           const std::string& command);

    /**
     * @brief 查询命令是否可用
     * @param document 文档
     * @param command 命令名称
     * @return true 如果命令可以执行
     */
    bool QueryCommandEnabled(std::shared_ptr<Document> document,
                             const std::string& command);

private:
    // ========== 事件分发 ==========

    /**
     * @brief 分发 beforeinput 事件
     * @param target 目标元素
     * @param input_type 输入类型
     * @param data 输入数据
     * @return true 如果事件未被取消
     */
    bool DispatchBeforeInputEvent(std::shared_ptr<Element> target,
                                  const std::string& input_type,
                                  const std::string& data);

    /**
     * @brief 分发 input 事件
     * @param target 目标元素
     * @param input_type 输入类型
     * @param data 输入数据
     */
    void DispatchInputEvent(std::shared_ptr<Element> target,
                            const std::string& input_type,
                            const std::string& data);

    // ========== 格式化命令 ==========

    /**
     * @brief 应用格式化（统一方法）
     * @param document 文档
     * @param tag_name 标签名（如 "strong", "em", "u"）
     * @return true 如果成功
     */
    bool ApplyFormatting(std::shared_ptr<Document> document, const std::string& tag_name);

    /**
     * @brief 移除格式化
     * @param document 文档
     * @param tag_name 标签名（如 "strong", "em", "u"）
     * @return true 如果成功
     */
    bool RemoveFormatting(std::shared_ptr<Document> document, const std::string& tag_name);

    /**
     * @brief 切换格式化（如果已有则移除，否则添加）
     * @param document 文档
     * @param tag_name 标签名（如 "strong", "em", "u"）
     * @return true 如果成功
     */
    bool ToggleFormatting(std::shared_ptr<Document> document, const std::string& tag_name);

    /**
     * @brief 应用格式化到范围（支持跨节点）
     * @param document 文档
     * @param range 范围
     * @param tag_name 标签名
     * @return true 如果成功
     */
    bool ApplyFormattingToRange(std::shared_ptr<Document> document,
                                std::shared_ptr<Range> range,
                                const std::string& tag_name);

    /**
     * @brief 应用粗体格式
     * @param document 文档
     * @return true 如果成功
     */
    bool ApplyBold(std::shared_ptr<Document> document);

    /**
     * @brief 应用斜体格式
     * @param document 文档
     * @return true 如果成功
     */
    bool ApplyItalic(std::shared_ptr<Document> document);

    /**
     * @brief 应用下划线格式
     * @param document 文档
     * @return true 如果成功
     */
    bool ApplyUnderline(std::shared_ptr<Document> document);

    // ========== 辅助方法 ==========

    /**
     * @brief 获取当前选择
     * @param document 文档
     * @return Selection 对象
     */
    std::shared_ptr<Selection> GetSelection(std::shared_ptr<Document> document);

    /**
     * @brief 查找包含节点的可编辑元素
     * @param node 节点
     * @return 可编辑元素，如果没有返回 nullptr
     */
    std::shared_ptr<Element> FindEditableElement(std::shared_ptr<Node> node) const;

    /**
     * @brief 合并到前一个节点（处理跨节点删除）
     * @param document 文档
     * @param current_node 当前节点
     * @return true 如果合并成功
     */
    bool MergeToPreviousNode(std::shared_ptr<Document> document, std::shared_ptr<Node> current_node);

    /**
     * @brief 合并到下一个节点（处理 Delete 键跨节点删除）
     * @param document 文档
     * @param current_node 当前节点
     * @return true 如果合并成功
     */
    bool MergeToNextNode(std::shared_ptr<Document> document, std::shared_ptr<Node> current_node);

    // ========== 光标移动 ==========

    /**
     * @brief 移动光标到左边一个字符
     * @param document 文档
     * @param extend_selection 是否扩展选择（Shift键）
     * @return true 如果移动成功
     */
    bool MoveCursorLeft(std::shared_ptr<Document> document, bool extend_selection);

    /**
     * @brief 移动光标到右边一个字符
     * @param document 文档
     * @param extend_selection 是否扩展选择（Shift键）
     * @return true 如果移动成功
     */
    bool MoveCursorRight(std::shared_ptr<Document> document, bool extend_selection);

    /**
     * @brief 移动光标到上一行
     * @param document 文档
     * @param extend_selection 是否扩展选择（Shift键）
     * @return true 如果移动成功
     */
    bool MoveCursorUp(std::shared_ptr<Document> document, bool extend_selection);

    /**
     * @brief 移动光标到下一行
     * @param document 文档
     * @param extend_selection 是否扩展选择（Shift键）
     * @return true 如果移动成功
     */
    bool MoveCursorDown(std::shared_ptr<Document> document, bool extend_selection);

    /**
     * @brief 移动光标到行首
     * @param document 文档
     * @param extend_selection 是否扩展选择（Shift键）
     * @return true 如果移动成功
     */
    bool MoveCursorToLineStart(std::shared_ptr<Document> document, bool extend_selection);

    /**
     * @brief 移动光标到行尾
     * @param document 文档
     * @param extend_selection 是否扩展选择（Shift键）
     * @return true 如果移动成功
     */
    bool MoveCursorToLineEnd(std::shared_ptr<Document> document, bool extend_selection);

    /**
     * @brief 移动光标到前一个单词
     * @param document 文档
     * @param extend_selection 是否扩展选择（Shift键）
     * @return true 如果移动成功
     */
    bool MoveCursorToPreviousWord(std::shared_ptr<Document> document, bool extend_selection);

    /**
     * @brief 移动光标到下一个单词
     * @param document 文档
     * @param extend_selection 是否扩展选择（Shift键）
     * @return true 如果移动成功
     */
    bool MoveCursorToNextWord(std::shared_ptr<Document> document, bool extend_selection);

    std::shared_ptr<Element> GetActiveEditableRoot(std::shared_ptr<Document> document) const;

    /**
     * @brief 查找前一个文本节点
     * @param current_node 当前节点
     * @return 前一个文本节点，如果没有返回 nullptr
     */
    std::shared_ptr<Node> FindPreviousTextNode(std::shared_ptr<Node> current_node);

    /**
     * @brief 查找下一个文本节点
     * @param current_node 当前节点
     * @return 下一个文本节点，如果没有返回 nullptr
     */
    std::shared_ptr<Node> FindNextTextNode(std::shared_ptr<Node> current_node);

    // ========== 撤销/重做 ==========

    /**
     * @brief 保存当前状态到撤销栈
     * @param element 可编辑元素
     */
    void SaveUndoState(std::shared_ptr<Element> element);

    /**
     * @brief 执行撤销操作
     * @param document 文档
     * @return true 如果撤销成功
     */
    bool Undo(std::shared_ptr<Document> document);

    /**
     * @brief 执行重做操作
     * @param document 文档
     * @return true 如果重做成功
     */
    bool Redo(std::shared_ptr<Document> document);

    /**
     * @brief 开始撤销组（连续操作合并为一个撤销操作）
     * @param element 可编辑元素
     */
    void BeginUndoGroup(std::shared_ptr<Element> element);

    /**
     * @brief 结束撤销组
     */
    void EndUndoGroup();

    /**
     * @brief 检查是否在撤销组中
     * @return true 如果在撤销组中
     */
    bool IsInUndoGroup() const { return in_undo_group_; }

private:
    SelectionManager* selection_manager_;

    // 撤销/重做栈
    std::deque<UndoState> undo_stack_;
    std::deque<UndoState> redo_stack_;
    static constexpr size_t MAX_UNDO_STACK_SIZE = 100;

    // 撤销组状态
    bool in_undo_group_ = false;
    UndoState undo_group_start_state_;

    std::unordered_map<Document*, CompositionState> composition_states_;
};

} // namespace lightui
