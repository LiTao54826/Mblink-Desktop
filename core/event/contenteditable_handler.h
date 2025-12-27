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
 *
 * 参考：https://w3c.github.io/editing/docs/execCommand/
 */

#pragma once

#include <memory>
#include <string>
#include <functional>

namespace lightui {

// 前向声明
class Document;
class Element;
class Node;
class SelectionManager;
class Selection;
struct KeyEvent;

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
    std::shared_ptr<Element> FindEditableElement(std::shared_ptr<Node> node);

    /**
     * @brief 合并到前一个节点（处理跨节点删除）
     * @param document 文档
     * @param current_node 当前节点
     * @return true 如果合并成功
     */
    bool MergeToPreviousNode(std::shared_ptr<Document> document, std::shared_ptr<Node> current_node);

private:
    SelectionManager* selection_manager_;
};

} // namespace lightui
