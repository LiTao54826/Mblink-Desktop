/**
 * @file contenteditable_handler.cpp
 * @brief ContentEditable 输入处理器实现
 * 
 * @note 大文件说明 (2505 行)
 * 本文件包含 contentEditable 功能的完整实现。
 * 文件较大的原因：
 * 1. 实现完整的富文本编辑功能
 * 2. 包含复杂的光标和选区管理
 * 3. 包含文本插入、删除、格式化逻辑
 * 4. 包含键盘快捷键处理
 * 5. 包含撤销/重做支持
 * 6. 需要处理各种边界情况
 *
 * 计划重构：
 * - 提取命令模式实现到独立文件
 * - 提取选区操作到独立文件
 */

#include "contenteditable_handler.h"
#include "core/editing/selection_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/selection.h"
#include "core/dom/range.h"
#include "core/dom/event.h"
#include <algorithm>
#include <cctype>
#include <iostream>

namespace lightui {

// ========== 构造函数/析构函数 ==========

ContentEditableHandler::ContentEditableHandler(SelectionManager* selection_manager)
    : selection_manager_(selection_manager) {
}

ContentEditableHandler::~ContentEditableHandler() = default;

// ========== 可编辑性检查 ==========

bool ContentEditableHandler::IsEditable(std::shared_ptr<Element> element) const {
    return IsContentEditable(element);
}

bool ContentEditableHandler::IsContentEditable(std::shared_ptr<Element> element) const {
    if (!element) {
        return false;
    }
    return element->IsContentEditable();
}

// ========== 输入处理 ==========

bool ContentEditableHandler::HandleTextInput(
    std::shared_ptr<Element> target,
    const std::string& text) {

    if (!target || !IsEditable(target)) {
        return false;
    }

    // 分发 beforeinput 事件
    if (!DispatchBeforeInputEvent(target, "insertText", text)) {
        return false;  // 事件被取消
    }

    // 获取文档
    auto document = target->GetOwnerDocument();
    if (!document) {
        return false;
    }

    // 插入文本
    if (!InsertText(document, text)) {
        return false;
    }

    // 分发 input 事件
    DispatchInputEvent(target, "insertText", text);

    return true;
}

bool ContentEditableHandler::HandleKeyDown(
    std::shared_ptr<Element> target,
    int key_code,
    bool ctrl_key,
    bool shift_key,
    bool alt_key) {

    if (!target || !IsEditable(target)) {
        return false;
    }

    auto document = target->GetOwnerDocument();
    if (!document) {
        return false;
    }

    // 处理特殊键
    switch (key_code) {
        case 8:  // Backspace
            if (!DispatchBeforeInputEvent(target, "deleteContentBackward", "")) {
                return false;
            }
            if (DeleteCharacter(document, false)) {
                DispatchInputEvent(target, "deleteContentBackward", "");
                return true;
            }
            break;

        case 46:  // Delete
            if (!DispatchBeforeInputEvent(target, "deleteContentForward", "")) {
                return false;
            }
            if (DeleteCharacter(document, true)) {
                DispatchInputEvent(target, "deleteContentForward", "");
                return true;
            }
            break;

        case 13:  // Enter
            if (!DispatchBeforeInputEvent(target, "insertLineBreak", "")) {
                return false;
            }
            if (InsertLineBreak(document)) {
                DispatchInputEvent(target, "insertLineBreak", "");
                return true;
            }
            break;

        // 方向键
        case 37:  // Left Arrow
            if (ctrl_key) {
                return MoveCursorToPreviousWord(document, shift_key);
            }
            return MoveCursorLeft(document, shift_key);

        case 39:  // Right Arrow
            if (ctrl_key) {
                return MoveCursorToNextWord(document, shift_key);
            }
            return MoveCursorRight(document, shift_key);

        case 38:  // Up Arrow
            return MoveCursorUp(document, shift_key);

        case 40:  // Down Arrow
            return MoveCursorDown(document, shift_key);

        case 36:  // Home
            return MoveCursorToLineStart(document, shift_key);

        case 35:  // End
            return MoveCursorToLineEnd(document, shift_key);

        default:
            // 处理 Ctrl 组合键
            if (ctrl_key) {
                switch (key_code) {
                    case 66:  // Ctrl+B (Bold)
                        return ExecCommand(document, "bold");
                    case 73:  // Ctrl+I (Italic)
                        return ExecCommand(document, "italic");
                    case 85:  // Ctrl+U (Underline)
                        return ExecCommand(document, "underline");
                    case 65:  // Ctrl+A (Select All)
                        return ExecCommand(document, "selectAll");
                    case 67:  // Ctrl+C (Copy) - 由系统处理
                        return false;
                    case 86:  // Ctrl+V (Paste) - 由系统处理
                        return false;
                    case 88:  // Ctrl+X (Cut) - 由系统处理
                        return false;
                    case 90:  // Ctrl+Z (Undo)
                        return Undo(document);
                    case 89:  // Ctrl+Y (Redo)
                        return Redo(document);
                }
            }
            break;
    }

    return false;
}

// ========== 编辑操作 ==========

bool ContentEditableHandler::InsertText(
    std::shared_ptr<Document> document,
    const std::string& text) {

    if (!document || text.empty()) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return false;
    }

    // 保存撤销状态
    auto anchor_for_undo = selection->GetAnchorNode();
    auto editable_for_undo = FindEditableElement(anchor_for_undo);
    if (editable_for_undo) {
        SaveUndoState(editable_for_undo);
    }

    // 如果有选中内容，先删除
    if (!selection->IsCollapsed()) {
        DeleteSelection(document);
    }

    // 获取光标位置
    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();

    if (!anchor_node) {
        return false;
    }

    // 如果是文本节点，直接插入
    if (anchor_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(anchor_node);
        if (text_node) {
            std::string content = text_node->GetTextContent();
            content.insert(anchor_offset, text);
            text_node->SetTextContent(content);

            // 更新光标位置
            selection->Collapse(anchor_node, anchor_offset + static_cast<int>(text.length()));
            return true;
        }
    }

    // 如果是元素节点，创建文本节点
    auto element = std::dynamic_pointer_cast<Element>(anchor_node);
    if (element) {
        auto new_text = std::make_shared<Text>(text);

        // 在指定位置插入（AppendChild/InsertBefore 会自动设置 owner_document_）
        auto children = element->GetChildNodes();
        if (anchor_offset >= static_cast<int>(children.size())) {
            element->AppendChild(new_text);
        } else if (anchor_offset > 0) {
            auto ref_node = children[anchor_offset];
            element->InsertBefore(new_text, ref_node);
        } else {
            if (!children.empty()) {
                element->InsertBefore(new_text, children[0]);
            } else {
                element->AppendChild(new_text);
            }
        }

        // 更新光标位置
        selection->Collapse(new_text, static_cast<int>(text.length()));
        return true;
    }

    return false;
}

bool ContentEditableHandler::DeleteSelection(std::shared_ptr<Document> document) {
    if (!document) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection || selection->IsCollapsed()) {
        return false;
    }

    auto range = selection->GetRangeAt(0);
    if (!range) {
        return false;
    }

    // 获取范围边界
    auto start_container = range->GetStartContainer();
    auto end_container = range->GetEndContainer();
    int start_offset = range->GetStartOffset();
    int end_offset = range->GetEndOffset();

    // 简单情况：同一个文本节点内
    if (start_container == end_container &&
        start_container->GetNodeType() == NodeType::TEXT_NODE) {

        auto text_node = std::dynamic_pointer_cast<Text>(start_container);
        if (text_node) {
            std::string content = text_node->GetTextContent();
            content.erase(start_offset, end_offset - start_offset);
            text_node->SetTextContent(content);

            // 折叠选择到删除位置
            selection->Collapse(start_container, start_offset);
            return true;
        }
    }

    // 跨节点删除
    // 先收集所有需要处理的文本节点（按文档顺序）
    auto editable_root = FindEditableElement(start_container);
    if (!editable_root) {
        return false;
    }

    std::vector<std::shared_ptr<Text>> all_text_nodes;
    std::function<void(std::shared_ptr<Node>)> collectAllTextNodes;
    collectAllTextNodes = [&](std::shared_ptr<Node> node) {
        if (!node) return;
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                all_text_nodes.push_back(text_node);
            }
        }
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                for (auto& child : elem->GetChildNodes()) {
                    collectAllTextNodes(child);
                }
            }
        }
    };
    collectAllTextNodes(editable_root);

    // 找到起始和结束节点在列表中的位置
    int start_idx = -1, end_idx = -1;
    for (size_t i = 0; i < all_text_nodes.size(); i++) {
        if (all_text_nodes[i] == start_container) start_idx = static_cast<int>(i);
        if (all_text_nodes[i] == end_container) end_idx = static_cast<int>(i);
    }

    if (start_idx == -1 || end_idx == -1 || start_idx > end_idx) {
        return false;
    }

    // 1. 处理起始文本节点：保留 0 到 start_offset 的内容
    if (start_container->GetNodeType() == NodeType::TEXT_NODE) {
        auto start_text = std::dynamic_pointer_cast<Text>(start_container);
        if (start_text) {
            std::string content = start_text->GetTextContent();
            content.erase(start_offset);
            start_text->SetTextContent(content);
        }
    }

    // 2. 处理结束文本节点：保留 end_offset 到末尾的内容
    if (end_container->GetNodeType() == NodeType::TEXT_NODE && start_idx != end_idx) {
        auto end_text = std::dynamic_pointer_cast<Text>(end_container);
        if (end_text) {
            std::string content = end_text->GetTextContent();
            content.erase(0, end_offset);
            end_text->SetTextContent(content);
        }
    }

    // 3. 清空中间所有文本节点的内容
    for (int i = start_idx + 1; i < end_idx; i++) {
        all_text_nodes[i]->SetTextContent("");
    }

    // 4. 删除内容为空的元素（从后向前删除，避免索引问题）
    // 收集需要删除的空元素
    std::vector<std::shared_ptr<Element>> elements_to_remove;
    for (int i = end_idx; i >= start_idx; i--) {
        auto text_node = all_text_nodes[i];
        if (text_node->GetTextContent().empty()) {
            // 找到文本节点的父元素
            auto parent = text_node->GetParentNode();
            if (parent && parent->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto parent_elem = std::dynamic_pointer_cast<Element>(parent);
                // 不删除 contentEditable 根元素
                if (parent_elem && parent_elem != editable_root) {
                    // 检查父元素是否只有这一个空文本节点
                    bool only_empty_text = true;
                    for (auto& child : parent_elem->GetChildNodes()) {
                        if (child != text_node) {
                            if (child->GetNodeType() == NodeType::TEXT_NODE) {
                                auto t = std::dynamic_pointer_cast<Text>(child);
                                if (t && !t->GetTextContent().empty()) {
                                    only_empty_text = false;
                                    break;
                                }
                            } else {
                                only_empty_text = false;
                                break;
                            }
                        }
                    }
                    if (only_empty_text) {
                        elements_to_remove.push_back(parent_elem);
                    }
                }
            }
        }
    }

    // 删除空元素
    for (auto& elem : elements_to_remove) {
        auto parent = elem->GetParentNode();
        if (parent && parent->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto parent_elem = std::dynamic_pointer_cast<Element>(parent);
            if (parent_elem) {
                parent_elem->RemoveChild(elem);
            }
        }
    }

    // 5. 折叠选择到起始位置
    selection->Collapse(start_container, start_offset);
    return true;
}

bool ContentEditableHandler::DeleteCharacter(
    std::shared_ptr<Document> document,
    bool forward) {

    if (!document) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return false;
    }

    // 保存撤销状态
    auto anchor_for_undo = selection->GetAnchorNode();
    auto editable_for_undo = FindEditableElement(anchor_for_undo);
    if (editable_for_undo) {
        SaveUndoState(editable_for_undo);
    }

    // 如果有选中内容，删除选中内容
    if (!selection->IsCollapsed()) {
        return DeleteSelection(document);
    }

    // 获取光标位置
    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();

    if (!anchor_node) {
        return false;
    }

    // 处理文本节点
    if (anchor_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(anchor_node);
        if (!text_node) {
            return false;
        }

        std::string content = text_node->GetTextContent();

        if (forward) {
            // Delete: 删除光标后的字符
            if (anchor_offset >= static_cast<int>(content.length())) {
                // 已在末尾，尝试合并下一个节点
                return MergeToNextNode(document, anchor_node);
            }
            
            // 计算要删除的字符长度（处理 UTF-8 多字节字符）
            size_t char_len = 1;
            if (anchor_offset < static_cast<int>(content.length())) {
                unsigned char c = content[anchor_offset];
                if ((c & 0x80) == 0) char_len = 1;
                else if ((c & 0xE0) == 0xC0) char_len = 2;
                else if ((c & 0xF0) == 0xE0) char_len = 3;
                else if ((c & 0xF8) == 0xF0) char_len = 4;
            }
            
            content.erase(anchor_offset, char_len);
            text_node->SetTextContent(content);
            // 光标位置不变
        } else {
            // Backspace: 删除光标前的字符
            if (anchor_offset <= 0) {
                // 已在开头，尝试合并到前一个节点或删除空元素
                return MergeToPreviousNode(document, anchor_node);
            }
            
            // 计算要删除的字符的起始位置和长度（处理 UTF-8 多字节字符）
            // 需要找到 anchor_offset 前一个字符的起始位置
            size_t delete_start = anchor_offset - 1;
            size_t char_len = 1;
            
            // 向前查找 UTF-8 字符的起始位置
            while (delete_start > 0 && (static_cast<unsigned char>(content[delete_start]) & 0xC0) == 0x80) {
                delete_start--;
            }
            char_len = anchor_offset - delete_start;
            
            content.erase(delete_start, char_len);
            text_node->SetTextContent(content);
            // 光标前移
            selection->Collapse(anchor_node, static_cast<int>(delete_start));
            
            // 如果删除后文本节点变空，立即触发合并到上一行
            if (content.empty()) {
                return MergeToPreviousNode(document, anchor_node);
            }
        }

        return true;
    }

    return false;
}

bool ContentEditableHandler::InsertLineBreak(std::shared_ptr<Document> document) {
    if (!document) {
        return false;
    }

    // 插入换行符
    return InsertText(document, "\n");
}


// ========== execCommand 支持 ==========

bool ContentEditableHandler::ExecCommand(
    std::shared_ptr<Document> document,
    const std::string& command,
    const std::string& value) {

    if (!document) {
        return false;
    }

    // 检查是否有焦点在可编辑元素上
    auto selection = GetSelection(document);
    if (!selection) {
        return false;
    }

    auto anchor_node = selection->GetAnchorNode();
    if (!anchor_node) {
        return false;
    }

    auto editable = FindEditableElement(anchor_node);
    if (!editable) {
        return false;
    }

    // 执行命令
    if (command == "bold") {
        return ApplyBold(document);
    } else if (command == "italic") {
        return ApplyItalic(document);
    } else if (command == "underline") {
        return ApplyUnderline(document);
    } else if (command == "insertText") {
        return InsertText(document, value);
    } else if (command == "delete") {
        if (!selection->IsCollapsed()) {
            return DeleteSelection(document);
        }
        return DeleteCharacter(document, false);
    } else if (command == "selectAll") {
        // 选择可编辑元素的所有内容
        selection->SelectAllChildren(editable);
        return true;
    }

    return false;
}

bool ContentEditableHandler::QueryCommandState(
    std::shared_ptr<Document> document,
    const std::string& command) {

    if (!document) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return false;
    }

    auto anchor_node = selection->GetAnchorNode();
    if (!anchor_node) {
        return false;
    }

    // 检查当前节点或其祖先是否有对应的格式标签
    auto current = anchor_node;
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(current);
            if (element) {
                std::string tag_name = element->GetTagName();
                // 转换为小写进行比较
                for (auto& c : tag_name) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }

                if (command == "bold" && (tag_name == "b" || tag_name == "strong")) {
                    return true;
                }
                if (command == "italic" && (tag_name == "i" || tag_name == "em")) {
                    return true;
                }
                if (command == "underline" && tag_name == "u") {
                    return true;
                }
            }
        }
        current = current->GetParentNode();
    }

    return false;
}

bool ContentEditableHandler::QueryCommandEnabled(
    std::shared_ptr<Document> document,
    const std::string& command) {

    if (!document) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return false;
    }

    auto anchor_node = selection->GetAnchorNode();
    if (!anchor_node) {
        return false;
    }

    // 检查是否在可编辑区域
    auto editable = FindEditableElement(anchor_node);
    if (!editable) {
        return false;
    }

    // 大多数命令在可编辑区域都可用
    if (command == "bold" || command == "italic" || command == "underline" ||
        command == "insertText" || command == "delete" || command == "selectAll") {
        return true;
    }

    return false;
}

// ========== 事件分发 ==========

bool ContentEditableHandler::DispatchBeforeInputEvent(
    std::shared_ptr<Element> target,
    const std::string& input_type,
    const std::string& data) {

    if (!target) {
        return true;
    }

    // 创建 beforeinput 事件（使用 InputEvent 类）
    auto event = std::make_shared<InputEvent>("beforeinput", input_type, data);

    // 分发事件
    target->DispatchEvent(event);

    // 返回事件是否被取消
    return !event->IsDefaultPrevented();
}

void ContentEditableHandler::DispatchInputEvent(
    std::shared_ptr<Element> target,
    const std::string& input_type,
    const std::string& data) {

    if (!target) {
        return;
    }

    // 创建 input 事件（使用 InputEvent 类）
    auto event = std::make_shared<InputEvent>("input", input_type, data);

    // 分发事件
    target->DispatchEvent(event);
}

// ========== 格式化命令 ==========

bool ContentEditableHandler::ApplyFormatting(
    std::shared_ptr<Document> document,
    const std::string& tag_name) {

    if (!document || tag_name.empty()) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection || selection->IsCollapsed()) {
        return false;
    }

    auto range = selection->GetRangeAt(0);
    if (!range) {
        return false;
    }

    return ApplyFormattingToRange(document, range, tag_name);
}

bool ContentEditableHandler::RemoveFormatting(
    std::shared_ptr<Document> document,
    const std::string& tag_name) {

    if (!document || tag_name.empty()) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection || selection->IsCollapsed()) {
        return false;
    }

    auto anchor_node = selection->GetAnchorNode();
    if (!anchor_node) {
        return false;
    }

    // 查找包含选择的格式化元素
    auto current = anchor_node;
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(current);
            if (element) {
                std::string current_tag = element->GetTagName();
                for (auto& c : current_tag) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                
                std::string target_tag = tag_name;
                for (auto& c : target_tag) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                
                // 检查是否匹配（考虑别名：b/strong, i/em）
                bool matches = (current_tag == target_tag) ||
                               (target_tag == "strong" && current_tag == "b") ||
                               (target_tag == "b" && current_tag == "strong") ||
                               (target_tag == "em" && current_tag == "i") ||
                               (target_tag == "i" && current_tag == "em");
                
                if (matches) {
                    // 找到格式化元素，将其内容提升到父元素
                    auto parent = element->GetParentNode();
                    if (parent && parent->GetNodeType() == NodeType::ELEMENT_NODE) {
                        auto parent_elem = std::dynamic_pointer_cast<Element>(parent);
                        if (parent_elem) {
                            // 将格式化元素的所有子节点移动到父元素
                            auto children = element->GetChildNodes();
                            for (auto& child : children) {
                                parent_elem->InsertBefore(child, element);
                            }
                            // 删除空的格式化元素
                            parent_elem->RemoveChild(element);
                            return true;
                        }
                    }
                }
            }
        }
        current = current->GetParentNode();
    }

    return false;
}

bool ContentEditableHandler::ToggleFormatting(
    std::shared_ptr<Document> document,
    const std::string& tag_name) {

    if (!document || tag_name.empty()) {
        return false;
    }

    // 检查当前选择是否已有该格式
    std::string command;
    std::string lower_tag = tag_name;
    for (auto& c : lower_tag) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    
    if (lower_tag == "strong" || lower_tag == "b") {
        command = "bold";
    } else if (lower_tag == "em" || lower_tag == "i") {
        command = "italic";
    } else if (lower_tag == "u") {
        command = "underline";
    } else {
        command = lower_tag;
    }

    if (QueryCommandState(document, command)) {
        // 已有格式，移除
        return RemoveFormatting(document, tag_name);
    } else {
        // 没有格式，添加
        return ApplyFormatting(document, tag_name);
    }
}

bool ContentEditableHandler::ApplyFormattingToRange(
    std::shared_ptr<Document> document,
    std::shared_ptr<Range> range,
    const std::string& tag_name) {

    if (!document || !range || tag_name.empty()) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return false;
    }

    // 获取选中范围的起始和结束
    auto start_container = range->GetStartContainer();
    auto end_container = range->GetEndContainer();
    int start_offset = range->GetStartOffset();
    int end_offset = range->GetEndOffset();

    // 简单情况：同一个文本节点内
    if (start_container == end_container &&
        start_container->GetNodeType() == NodeType::TEXT_NODE) {

        auto text_node = std::dynamic_pointer_cast<Text>(start_container);
        if (!text_node) {
            return false;
        }

        std::string content = text_node->GetTextContent();
        std::string before = content.substr(0, start_offset);
        std::string selected = content.substr(start_offset, end_offset - start_offset);
        std::string after = content.substr(end_offset);

        // 获取父元素
        auto parent = text_node->GetParentNode();
        if (!parent) {
            return false;
        }

        auto parent_element = std::dynamic_pointer_cast<Element>(parent);
        if (!parent_element) {
            return false;
        }

        // 创建新的结构：before_text + <tag>selected</tag> + after_text
        if (!before.empty()) {
            auto before_text = std::make_shared<Text>(before);
            parent_element->InsertBefore(before_text, text_node);
        }

        // 创建格式化元素
        auto format_element = document->CreateElement(tag_name);
        auto selected_text = std::make_shared<Text>(selected);
        format_element->AppendChild(selected_text);
        parent_element->InsertBefore(format_element, text_node);

        if (!after.empty()) {
            auto after_text = std::make_shared<Text>(after);
            parent_element->InsertBefore(after_text, text_node);
        }

        // 移除原文本节点
        parent_element->RemoveChild(text_node);

        // 更新选择到新的格式化文本
        selection->Collapse(selected_text, static_cast<int>(selected.length()));

        return true;
    }

    // 跨节点格式化
    // 收集所有需要格式化的文本节点
    auto editable_root = FindEditableElement(start_container);
    if (!editable_root) {
        return false;
    }

    std::vector<std::shared_ptr<Text>> all_text_nodes;
    std::function<void(std::shared_ptr<Node>)> collectAllTextNodes;
    collectAllTextNodes = [&](std::shared_ptr<Node> node) {
        if (!node) return;
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                all_text_nodes.push_back(text_node);
            }
        }
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                for (auto& child : elem->GetChildNodes()) {
                    collectAllTextNodes(child);
                }
            }
        }
    };
    collectAllTextNodes(editable_root);

    // 找到起始和结束节点在列表中的位置
    int start_idx = -1, end_idx = -1;
    for (size_t i = 0; i < all_text_nodes.size(); i++) {
        if (all_text_nodes[i] == start_container) start_idx = static_cast<int>(i);
        if (all_text_nodes[i] == end_container) end_idx = static_cast<int>(i);
    }

    if (start_idx == -1 || end_idx == -1 || start_idx > end_idx) {
        return false;
    }

    // 对每个涉及的文本节点应用格式化
    for (int i = start_idx; i <= end_idx; i++) {
        auto text_node = all_text_nodes[i];
        std::string content = text_node->GetTextContent();
        
        int node_start = (i == start_idx) ? start_offset : 0;
        int node_end = (i == end_idx) ? end_offset : static_cast<int>(content.length());
        
        if (node_start >= node_end) continue;
        
        std::string before = content.substr(0, node_start);
        std::string selected = content.substr(node_start, node_end - node_start);
        std::string after = content.substr(node_end);
        
        auto parent = text_node->GetParentNode();
        if (!parent || parent->GetNodeType() != NodeType::ELEMENT_NODE) continue;
        
        auto parent_element = std::dynamic_pointer_cast<Element>(parent);
        if (!parent_element) continue;
        
        // 创建新结构
        if (!before.empty()) {
            auto before_text = std::make_shared<Text>(before);
            parent_element->InsertBefore(before_text, text_node);
        }
        
        auto format_element = document->CreateElement(tag_name);
        auto selected_text = std::make_shared<Text>(selected);
        format_element->AppendChild(selected_text);
        parent_element->InsertBefore(format_element, text_node);
        
        if (!after.empty()) {
            auto after_text = std::make_shared<Text>(after);
            parent_element->InsertBefore(after_text, text_node);
        }
        
        parent_element->RemoveChild(text_node);
    }

    return true;
}

bool ContentEditableHandler::ApplyBold(std::shared_ptr<Document> document) {
    return ApplyFormatting(document, "strong");
}

bool ContentEditableHandler::ApplyItalic(std::shared_ptr<Document> document) {
    return ApplyFormatting(document, "em");
}

bool ContentEditableHandler::ApplyUnderline(std::shared_ptr<Document> document) {
    return ApplyFormatting(document, "u");
}

// ========== 辅助方法 ==========

std::shared_ptr<Selection> ContentEditableHandler::GetSelection(
    std::shared_ptr<Document> document) {

    if (!selection_manager_ || !document) {
        return nullptr;
    }

    return selection_manager_->GetSelection(document);
}

std::shared_ptr<Element> ContentEditableHandler::FindEditableElement(
    std::shared_ptr<Node> node) {

    if (!node) {
        return nullptr;
    }

    // 向上遍历查找最外层的可编辑元素（设置了 contenteditable="true" 属性的元素）
    auto current = node;
    std::shared_ptr<Element> editable_root = nullptr;
    
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(current);
            if (element) {
                // 检查是否显式设置了 contenteditable="true" 属性
                std::string attr = element->GetAttribute("contenteditable");
                if (attr == "true" || attr == "") {
                    // 如果属性为 "true" 或空字符串（表示 contenteditable 属性存在但无值）
                    // 需要进一步检查是否真的设置了该属性
                    if (element->HasAttribute("contenteditable")) {
                        editable_root = element;
                        // 继续向上查找，以找到最外层的 contentEditable 元素
                    }
                }
            }
        }
        current = current->GetParentNode();
    }

    return editable_root;
}

bool ContentEditableHandler::MergeToPreviousNode(
    std::shared_ptr<Document> document,
    std::shared_ptr<Node> current_node) {

    if (!document || !current_node) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return false;
    }

    // 辅助函数：查找节点中最后一个文本节点
    std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findLastTextNode;
    findLastTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Text> {
        if (!node) return nullptr;
        
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            return std::dynamic_pointer_cast<Text>(node);
        }
        
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                auto children = elem->GetChildNodes();
                for (auto it = children.rbegin(); it != children.rend(); ++it) {
                    auto result = findLastTextNode(*it);
                    if (result) return result;
                }
            }
        }
        return nullptr;
    };

    // 辅助函数：将光标移动到节点末尾（确保光标在文本节点上）
    auto moveCursorToEndOfNode = [&](std::shared_ptr<Node> node) -> bool {
        if (!node) return false;
        
        // 先尝试找到最后一个文本节点
        auto last_text = findLastTextNode(node);
        if (last_text) {
            selection->Collapse(last_text, static_cast<int>(last_text->GetTextContent().length()));
            return true;
        }
        
        // 如果没有文本节点，在元素中创建一个空文本节点
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                auto empty_text = std::make_shared<Text>("");
                elem->AppendChild(empty_text);
                selection->Collapse(empty_text, 0);
                return true;
            }
        }
        
        return false;
    };

    // 辅助函数：检查元素是否为空（没有文本内容）
    std::function<bool(std::shared_ptr<Node>)> isNodeEmpty;
    isNodeEmpty = [&](std::shared_ptr<Node> node) -> bool {
        if (!node) return true;
        
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text = std::dynamic_pointer_cast<Text>(node);
            return !text || text->GetTextContent().empty();
        }
        
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                for (auto& child : elem->GetChildNodes()) {
                    if (!isNodeEmpty(child)) return false;
                }
            }
        }
        return true;
    };

    // 辅助函数：检查是否是块级元素
    auto isBlockElement = [](const std::string& tag) -> bool {
        return tag == "p" || tag == "div" || tag == "h1" || tag == "h2" || 
               tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6" ||
               tag == "li" || tag == "blockquote" || tag == "pre";
    };

    // 辅助函数：检查是否是格式化元素
    auto isFormattingElement = [](const std::string& tag) -> bool {
        return tag == "em" || tag == "strong" || tag == "b" || 
               tag == "i" || tag == "u" || tag == "s" || 
               tag == "span" || tag == "a" || tag == "code";
    };

    // 获取当前节点的父元素
    auto parent = current_node->GetParentNode();
    if (!parent) {
        return false;
    }

    // 查找前一个兄弟节点
    auto prev_sibling = current_node->GetPreviousSibling();
    
    // 如果当前文本节点为空，检查是否应该删除父元素
    if (current_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(current_node);
        if (text_node && text_node->GetTextContent().empty()) {
            // 文本节点为空，检查父元素
            if (parent->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto parent_elem = std::dynamic_pointer_cast<Element>(parent);
                if (parent_elem) {
                    std::string tag = parent_elem->GetTagName();
                    // 转换为小写
                    for (auto& c : tag) {
                        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    }
                    
                    // 检查父元素是否为空
                    bool parent_is_empty = isNodeEmpty(parent_elem);
                    
                    // 情况1：空的格式化元素（em, strong 等）
                    if (isFormattingElement(tag) && parent_is_empty) {
                        
                        auto format_prev = parent_elem->GetPreviousSibling();
                        auto grandparent = parent_elem->GetParentNode();
                        
                        if (grandparent) {
                            auto grandparent_elem = std::dynamic_pointer_cast<Element>(grandparent);
                            if (grandparent_elem) {
                                // 删除空的格式化元素
                                grandparent_elem->RemoveChild(parent_elem);
                                
                                // 将光标移动到前一个节点的末尾
                                if (format_prev) {
                                    auto last_text = findLastTextNode(format_prev);
                                    if (last_text) {
                                        selection->Collapse(last_text, static_cast<int>(last_text->GetTextContent().length()));
                                        return true;
                                    }
                                    // 使用辅助函数确保光标在文本节点上
                                    if (moveCursorToEndOfNode(format_prev)) {
                                        return true;
                                    }
                                }
                                
                                // 没有前一个兄弟，检查祖父元素是否为空
                                if (isNodeEmpty(grandparent_elem)) {
                                    // 祖父元素为空，递归处理
                                    return MergeToPreviousNode(document, grandparent_elem);
                                }
                                
                                // 祖父元素不为空，但当前格式化元素没有前一个兄弟
                                // 尝试在祖父元素中找到其他文本节点
                                auto last_text_in_grandparent = findLastTextNode(grandparent_elem);
                                if (last_text_in_grandparent) {
                                    selection->Collapse(last_text_in_grandparent, static_cast<int>(last_text_in_grandparent->GetTextContent().length()));
                                    return true;
                                }
                                
                                // 使用辅助函数确保光标在文本节点上
                                if (moveCursorToEndOfNode(grandparent)) {
                                    return true;
                                }
                                return false;
                            }
                        }
                    }
                    
                    // 情况2：空的块级元素（p, div 等）- 删除整行并移动到上一行
                    if (isBlockElement(tag) && parent_is_empty) {
                        
                        auto block_prev = parent_elem->GetPreviousSibling();
                        auto grandparent = parent_elem->GetParentNode();
                        
                        if (grandparent) {
                            auto grandparent_elem = std::dynamic_pointer_cast<Element>(grandparent);
                            if (grandparent_elem) {
                                // 删除空的块级元素
                                grandparent_elem->RemoveChild(parent_elem);
                                
                                // 将光标移动到前一个块级元素的末尾
                                if (block_prev) {
                                    auto last_text = findLastTextNode(block_prev);
                                    if (last_text) {
                                        selection->Collapse(last_text, static_cast<int>(last_text->GetTextContent().length()));
                                        return true;
                                    }
                                    
                                    // 如果前一个块级元素没有文本，使用辅助函数确保光标在文本节点上
                                    if (moveCursorToEndOfNode(block_prev)) {
                                        return true;
                                    }
                                    return false;
                                }
                                
                                // 没有前一个兄弟，使用辅助函数确保光标在文本节点上
                                if (moveCursorToEndOfNode(grandparent)) {
                                    return true;
                                }
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 情况3：当前节点是元素节点且为空（可能是递归调用）
    if (current_node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto elem = std::dynamic_pointer_cast<Element>(current_node);
        if (elem && isNodeEmpty(elem)) {
            // 不要删除 contentEditable 的根元素
            if (elem->IsContentEditable()) {
                // 检查父元素是否也是 contentEditable
                auto parent_node = elem->GetParentNode();
                if (!parent_node || parent_node->GetNodeType() != NodeType::ELEMENT_NODE) {
                    return false;
                }
                auto parent_elem_check = std::dynamic_pointer_cast<Element>(parent_node);
                if (!parent_elem_check || !parent_elem_check->IsContentEditable()) {
                    return false;
                }
            }
            
            std::string tag = elem->GetTagName();
            for (auto& c : tag) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            
            if (isBlockElement(tag) || isFormattingElement(tag)) {
                auto elem_prev = elem->GetPreviousSibling();
                auto elem_parent = elem->GetParentNode();
                
                if (elem_parent) {
                    auto parent_elem = std::dynamic_pointer_cast<Element>(elem_parent);
                    if (parent_elem) {
                        parent_elem->RemoveChild(elem);
                        
                        if (elem_prev) {
                            auto last_text = findLastTextNode(elem_prev);
                            if (last_text) {
                                int new_offset = static_cast<int>(last_text->GetTextContent().length());
                                selection->Collapse(last_text, new_offset);
                                return true;
                            }
                            // 使用辅助函数确保光标在文本节点上
                            if (moveCursorToEndOfNode(elem_prev)) {
                                return true;
                            }
                        }
                        
                        // 检查父元素是否也为空了
                        if (isNodeEmpty(parent_elem)) {
                            std::string parent_tag = parent_elem->GetTagName();
                            for (auto& c : parent_tag) {
                                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                            }
                            if (isBlockElement(parent_tag) || isFormattingElement(parent_tag)) {
                                return MergeToPreviousNode(document, parent_elem);
                            }
                        }
                        
                        // 使用辅助函数确保光标在文本节点上
                        if (moveCursorToEndOfNode(elem_parent)) {
                            return true;
                        }
                        return false;
                    }
                }
            }
        }
    }

    // 情况4：有前一个兄弟节点，删除其最后一个字符
    if (prev_sibling) {
        auto prev_text = findLastTextNode(prev_sibling);
        if (prev_text) {
            std::string content = prev_text->GetTextContent();
            if (!content.empty()) {
                // 删除前一个文本节点的最后一个字符
                size_t delete_start = content.length() - 1;
                while (delete_start > 0 && (static_cast<unsigned char>(content[delete_start]) & 0xC0) == 0x80) {
                    delete_start--;
                }
                
                content.erase(delete_start);
                prev_text->SetTextContent(content);
                selection->Collapse(prev_text, static_cast<int>(content.length()));
                return true;
            } else {
                // 前一个文本节点也为空，递归处理
                return MergeToPreviousNode(document, prev_text);
            }
        }
    }
    
    // 情况5：没有前一个兄弟，检查父元素是否需要处理
    if (!prev_sibling && parent->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto parent_elem = std::dynamic_pointer_cast<Element>(parent);
        if (parent_elem) {
            std::string tag = parent_elem->GetTagName();
            for (auto& c : tag) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            
            // 如果父元素是格式化或块级元素，且当前节点是第一个子节点
            if (isFormattingElement(tag) || isBlockElement(tag)) {
                // 检查父元素的前一个兄弟
                auto parent_prev = parent_elem->GetPreviousSibling();
                if (parent_prev) {
                    auto last_text = findLastTextNode(parent_prev);
                    if (last_text) {
                        std::string content = last_text->GetTextContent();
                        if (!content.empty()) {
                            // 删除前一个元素最后一个字符
                            size_t delete_start = content.length() - 1;
                            while (delete_start > 0 && (static_cast<unsigned char>(content[delete_start]) & 0xC0) == 0x80) {
                                delete_start--;
                            }
                            content.erase(delete_start);
                            last_text->SetTextContent(content);
                            selection->Collapse(last_text, static_cast<int>(content.length()));
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool ContentEditableHandler::MergeToNextNode(
    std::shared_ptr<Document> document,
    std::shared_ptr<Node> current_node) {

    if (!document || !current_node) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return false;
    }

    // 辅助函数：查找节点中第一个文本节点
    std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findFirstTextNode;
    findFirstTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Text> {
        if (!node) return nullptr;
        
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            return std::dynamic_pointer_cast<Text>(node);
        }
        
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                for (auto& child : elem->GetChildNodes()) {
                    auto result = findFirstTextNode(child);
                    if (result) return result;
                }
            }
        }
        return nullptr;
    };

    // 辅助函数：检查元素是否为空（没有文本内容）
    std::function<bool(std::shared_ptr<Node>)> isNodeEmpty;
    isNodeEmpty = [&](std::shared_ptr<Node> node) -> bool {
        if (!node) return true;
        
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text = std::dynamic_pointer_cast<Text>(node);
            return !text || text->GetTextContent().empty();
        }
        
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                for (auto& child : elem->GetChildNodes()) {
                    if (!isNodeEmpty(child)) return false;
                }
            }
        }
        return true;
    };

    // 辅助函数：检查是否是块级元素
    auto isBlockElement = [](const std::string& tag) -> bool {
        return tag == "p" || tag == "div" || tag == "h1" || tag == "h2" || 
               tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6" ||
               tag == "li" || tag == "blockquote" || tag == "pre";
    };

    // 辅助函数：检查是否是格式化元素
    auto isFormattingElement = [](const std::string& tag) -> bool {
        return tag == "em" || tag == "strong" || tag == "b" || 
               tag == "i" || tag == "u" || tag == "s" || 
               tag == "span" || tag == "a" || tag == "code";
    };

    // 获取当前节点的父元素
    auto parent = current_node->GetParentNode();
    if (!parent) {
        return false;
    }

    // 查找下一个兄弟节点
    auto next_sibling = current_node->GetNextSibling();
    
    // 情况1：有下一个兄弟节点，删除其第一个字符
    if (next_sibling) {
        auto next_text = findFirstTextNode(next_sibling);
        if (next_text) {
            std::string content = next_text->GetTextContent();
            if (!content.empty()) {
                // 删除下一个文本节点的第一个字符
                size_t char_len = 1;
                unsigned char c = content[0];
                if ((c & 0x80) == 0) char_len = 1;
                else if ((c & 0xE0) == 0xC0) char_len = 2;
                else if ((c & 0xF0) == 0xE0) char_len = 3;
                else if ((c & 0xF8) == 0xF0) char_len = 4;
                
                content.erase(0, char_len);
                next_text->SetTextContent(content);
                
                // 如果删除后文本为空，检查是否需要删除父元素
                if (content.empty()) {
                    auto next_parent = next_text->GetParentNode();
                    if (next_parent && next_parent->GetNodeType() == NodeType::ELEMENT_NODE) {
                        auto next_parent_elem = std::dynamic_pointer_cast<Element>(next_parent);
                        if (next_parent_elem && isNodeEmpty(next_parent_elem)) {
                            std::string tag = next_parent_elem->GetTagName();
                            for (auto& c : tag) {
                                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                            }
                            if (isFormattingElement(tag)) {
                                // 删除空的格式化元素
                                auto grandparent = next_parent_elem->GetParentNode();
                                if (grandparent && grandparent->GetNodeType() == NodeType::ELEMENT_NODE) {
                                    auto grandparent_elem = std::dynamic_pointer_cast<Element>(grandparent);
                                    if (grandparent_elem) {
                                        grandparent_elem->RemoveChild(next_parent_elem);
                                    }
                                }
                            }
                        }
                    }
                }
                
                // 光标位置不变
                return true;
            } else {
                // 下一个文本节点为空，递归处理
                return MergeToNextNode(document, next_text);
            }
        }
    }
    
    // 情况2：当前节点是文本节点且在块级元素末尾，合并下一个块级元素
    if (current_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(current_node);
        if (text_node) {
            // 查找当前节点所在的块级元素
            auto current_block = parent;
            while (current_block) {
                if (current_block->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto elem = std::dynamic_pointer_cast<Element>(current_block);
                    if (elem) {
                        std::string tag = elem->GetTagName();
                        for (auto& c : tag) {
                            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                        }
                        if (isBlockElement(tag)) {
                            // 找到块级元素，查找下一个块级兄弟
                            auto next_block = elem->GetNextSibling();
                            while (next_block) {
                                if (next_block->GetNodeType() == NodeType::ELEMENT_NODE) {
                                    auto next_elem = std::dynamic_pointer_cast<Element>(next_block);
                                    if (next_elem) {
                                        std::string next_tag = next_elem->GetTagName();
                                        for (auto& c : next_tag) {
                                            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                                        }
                                        if (isBlockElement(next_tag)) {
                                            // 找到下一个块级元素，将其内容合并到当前块
                                            auto first_text = findFirstTextNode(next_elem);
                                            if (first_text) {
                                                std::string next_content = first_text->GetTextContent();
                                                if (!next_content.empty()) {
                                                    // 删除下一个块的第一个字符
                                                    size_t char_len = 1;
                                                    unsigned char c = next_content[0];
                                                    if ((c & 0x80) == 0) char_len = 1;
                                                    else if ((c & 0xE0) == 0xC0) char_len = 2;
                                                    else if ((c & 0xF0) == 0xE0) char_len = 3;
                                                    else if ((c & 0xF8) == 0xF0) char_len = 4;
                                                    
                                                    next_content.erase(0, char_len);
                                                    first_text->SetTextContent(next_content);
                                                    
                                                    return true;
                                                }
                                            }
                                            
                                            // 下一个块为空，删除它
                                            if (isNodeEmpty(next_elem)) {
                                                auto block_parent = next_elem->GetParentNode();
                                                if (block_parent && block_parent->GetNodeType() == NodeType::ELEMENT_NODE) {
                                                    auto block_parent_elem = std::dynamic_pointer_cast<Element>(block_parent);
                                                    if (block_parent_elem) {
                                                        block_parent_elem->RemoveChild(next_elem);
                                                        return true;
                                                    }
                                                }
                                            }
                                            break;
                                        }
                                    }
                                }
                                next_block = next_block->GetNextSibling();
                            }
                            break;
                        }
                    }
                }
                current_block = current_block->GetParentNode();
            }
        }
    }

    return false;
}

// ========== 光标移动实现 ==========

bool ContentEditableHandler::MoveCursorLeft(
    std::shared_ptr<Document> document,
    bool extend_selection) {

    if (!document) return false;

    auto selection = GetSelection(document);
    if (!selection) return false;

    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();

    if (!anchor_node) return false;

    // 如果有选中内容且不是扩展选择，折叠到选择起点
    if (!selection->IsCollapsed() && !extend_selection) {
        selection->CollapseToStart();
        return true;
    }

    // 处理文本节点
    if (anchor_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(anchor_node);
        if (!text_node) return false;

        std::string content = text_node->GetTextContent();

        if (anchor_offset > 0) {
            // 向左移动一个字符（处理 UTF-8）
            int new_offset = anchor_offset - 1;
            while (new_offset > 0 && (static_cast<unsigned char>(content[new_offset]) & 0xC0) == 0x80) {
                new_offset--;
            }

            if (extend_selection) {
                selection->Extend(anchor_node, new_offset);
            } else {
                selection->Collapse(anchor_node, new_offset);
            }
            return true;
        } else {
            // 已在文本节点开头，移动到前一个文本节点
            auto prev_text = FindPreviousTextNode(anchor_node);
            if (prev_text) {
                auto prev_text_node = std::dynamic_pointer_cast<Text>(prev_text);
                if (prev_text_node) {
                    int new_offset = static_cast<int>(prev_text_node->GetTextContent().length());
                    if (extend_selection) {
                        selection->Extend(prev_text, new_offset);
                    } else {
                        selection->Collapse(prev_text, new_offset);
                    }
                    return true;
                }
            }
        }
    }

    return false;
}

bool ContentEditableHandler::MoveCursorRight(
    std::shared_ptr<Document> document,
    bool extend_selection) {

    if (!document) return false;

    auto selection = GetSelection(document);
    if (!selection) return false;

    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();

    if (!anchor_node) return false;

    // 如果有选中内容且不是扩展选择，折叠到选择终点
    if (!selection->IsCollapsed() && !extend_selection) {
        selection->CollapseToEnd();
        return true;
    }

    // 处理文本节点
    if (anchor_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(anchor_node);
        if (!text_node) return false;

        std::string content = text_node->GetTextContent();
        int content_length = static_cast<int>(content.length());

        if (anchor_offset < content_length) {
            // 向右移动一个字符（处理 UTF-8）
            int new_offset = anchor_offset;
            unsigned char c = content[new_offset];
            if ((c & 0x80) == 0) new_offset += 1;
            else if ((c & 0xE0) == 0xC0) new_offset += 2;
            else if ((c & 0xF0) == 0xE0) new_offset += 3;
            else if ((c & 0xF8) == 0xF0) new_offset += 4;
            else new_offset += 1;

            if (new_offset > content_length) new_offset = content_length;

            if (extend_selection) {
                selection->Extend(anchor_node, new_offset);
            } else {
                selection->Collapse(anchor_node, new_offset);
            }
            return true;
        } else {
            // 已在文本节点末尾，移动到下一个文本节点
            auto next_text = FindNextTextNode(anchor_node);
            if (next_text) {
                if (extend_selection) {
                    selection->Extend(next_text, 0);
                } else {
                    selection->Collapse(next_text, 0);
                }
                return true;
            }
        }
    }

    return false;
}

bool ContentEditableHandler::MoveCursorUp(
    std::shared_ptr<Document> document,
    bool extend_selection) {

    if (!document) return false;

    auto selection = GetSelection(document);
    if (!selection) return false;

    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();
    if (!anchor_node) return false;

    // 查找当前节点所在的块级元素
    auto current = anchor_node;
    std::shared_ptr<Element> current_block = nullptr;
    
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(current);
            if (elem) {
                std::string tag = elem->GetTagName();
                for (auto& c : tag) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                if (tag == "p" || tag == "div" || tag == "li" || tag == "h1" ||
                    tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6" ||
                    tag == "blockquote" || tag == "pre") {
                    current_block = elem;
                    break;
                }
            }
        }
        current = current->GetParentNode();
    }

    if (!current_block) {
        // 没有找到块级元素，移动到行首
        return MoveCursorToLineStart(document, extend_selection);
    }

    // 查找前一个块级元素
    auto prev_sibling = current_block->GetPreviousSibling();
    while (prev_sibling) {
        if (prev_sibling->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto prev_elem = std::dynamic_pointer_cast<Element>(prev_sibling);
            if (prev_elem) {
                std::string tag = prev_elem->GetTagName();
                for (auto& c : tag) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                if (tag == "p" || tag == "div" || tag == "li" || tag == "h1" ||
                    tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6" ||
                    tag == "blockquote" || tag == "pre") {
                    // 找到前一个块级元素，移动到其末尾
                    std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findLastTextNode;
                    findLastTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Text> {
                        if (!node) return nullptr;
                        if (node->GetNodeType() == NodeType::TEXT_NODE) {
                            return std::dynamic_pointer_cast<Text>(node);
                        }
                        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                            auto elem = std::dynamic_pointer_cast<Element>(node);
                            if (elem) {
                                auto children = elem->GetChildNodes();
                                for (auto it = children.rbegin(); it != children.rend(); ++it) {
                                    auto result = findLastTextNode(*it);
                                    if (result) return result;
                                }
                            }
                        }
                        return nullptr;
                    };

                    auto last_text = findLastTextNode(prev_elem);
                    if (last_text) {
                        int end_offset = static_cast<int>(last_text->GetTextContent().length());
                        if (extend_selection) {
                            selection->Extend(last_text, end_offset);
                        } else {
                            selection->Collapse(last_text, end_offset);
                        }
                        return true;
                    }
                    break;
                }
            }
        }
        prev_sibling = prev_sibling->GetPreviousSibling();
    }

    // 没有前一个块级元素，移动到当前块的开头
    return MoveCursorToLineStart(document, extend_selection);
}

bool ContentEditableHandler::MoveCursorDown(
    std::shared_ptr<Document> document,
    bool extend_selection) {

    if (!document) return false;

    auto selection = GetSelection(document);
    if (!selection) return false;

    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();
    if (!anchor_node) return false;

    // 查找当前节点所在的块级元素
    auto current = anchor_node;
    std::shared_ptr<Element> current_block = nullptr;
    
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(current);
            if (elem) {
                std::string tag = elem->GetTagName();
                for (auto& c : tag) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                if (tag == "p" || tag == "div" || tag == "li" || tag == "h1" ||
                    tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6" ||
                    tag == "blockquote" || tag == "pre") {
                    current_block = elem;
                    break;
                }
            }
        }
        current = current->GetParentNode();
    }

    if (!current_block) {
        // 没有找到块级元素，移动到行尾
        return MoveCursorToLineEnd(document, extend_selection);
    }

    // 查找下一个块级元素
    auto next_sibling = current_block->GetNextSibling();
    while (next_sibling) {
        if (next_sibling->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto next_elem = std::dynamic_pointer_cast<Element>(next_sibling);
            if (next_elem) {
                std::string tag = next_elem->GetTagName();
                for (auto& c : tag) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                if (tag == "p" || tag == "div" || tag == "li" || tag == "h1" ||
                    tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6" ||
                    tag == "blockquote" || tag == "pre") {
                    // 找到下一个块级元素，移动到其开头
                    std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findFirstTextNode;
                    findFirstTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Text> {
                        if (!node) return nullptr;
                        if (node->GetNodeType() == NodeType::TEXT_NODE) {
                            return std::dynamic_pointer_cast<Text>(node);
                        }
                        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                            auto elem = std::dynamic_pointer_cast<Element>(node);
                            if (elem) {
                                for (auto& child : elem->GetChildNodes()) {
                                    auto result = findFirstTextNode(child);
                                    if (result) return result;
                                }
                            }
                        }
                        return nullptr;
                    };

                    auto first_text = findFirstTextNode(next_elem);
                    if (first_text) {
                        if (extend_selection) {
                            selection->Extend(first_text, 0);
                        } else {
                            selection->Collapse(first_text, 0);
                        }
                        return true;
                    }
                    break;
                }
            }
        }
        next_sibling = next_sibling->GetNextSibling();
    }

    // 没有下一个块级元素，移动到当前块的末尾
    return MoveCursorToLineEnd(document, extend_selection);
}

bool ContentEditableHandler::MoveCursorToLineStart(
    std::shared_ptr<Document> document,
    bool extend_selection) {

    if (!document) return false;

    auto selection = GetSelection(document);
    if (!selection) return false;

    auto anchor_node = selection->GetAnchorNode();
    if (!anchor_node) return false;

    // 查找当前行的块级元素
    auto current = anchor_node;
    std::shared_ptr<Element> block_element = nullptr;

    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(current);
            if (elem) {
                std::string tag = elem->GetTagName();
                for (auto& c : tag) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                if (tag == "p" || tag == "div" || tag == "li" || tag == "h1" ||
                    tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6") {
                    block_element = elem;
                    break;
                }
            }
        }
        current = current->GetParentNode();
    }

    if (block_element) {
        // 找到块级元素中的第一个文本节点
        std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findFirstTextNode;
        findFirstTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Text> {
            if (!node) return nullptr;
            if (node->GetNodeType() == NodeType::TEXT_NODE) {
                return std::dynamic_pointer_cast<Text>(node);
            }
            if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto elem = std::dynamic_pointer_cast<Element>(node);
                if (elem) {
                    for (auto& child : elem->GetChildNodes()) {
                        auto result = findFirstTextNode(child);
                        if (result) return result;
                    }
                }
            }
            return nullptr;
        };

        auto first_text = findFirstTextNode(block_element);
        if (first_text) {
            if (extend_selection) {
                selection->Extend(first_text, 0);
            } else {
                selection->Collapse(first_text, 0);
            }
            return true;
        }
    }

    // 回退：移动到当前文本节点开头
    if (anchor_node->GetNodeType() == NodeType::TEXT_NODE) {
        if (extend_selection) {
            selection->Extend(anchor_node, 0);
        } else {
            selection->Collapse(anchor_node, 0);
        }
        return true;
    }

    return false;
}

bool ContentEditableHandler::MoveCursorToLineEnd(
    std::shared_ptr<Document> document,
    bool extend_selection) {

    if (!document) return false;

    auto selection = GetSelection(document);
    if (!selection) return false;

    auto anchor_node = selection->GetAnchorNode();
    if (!anchor_node) return false;

    // 查找当前行的块级元素
    auto current = anchor_node;
    std::shared_ptr<Element> block_element = nullptr;

    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(current);
            if (elem) {
                std::string tag = elem->GetTagName();
                for (auto& c : tag) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                if (tag == "p" || tag == "div" || tag == "li" || tag == "h1" ||
                    tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6") {
                    block_element = elem;
                    break;
                }
            }
        }
        current = current->GetParentNode();
    }

    if (block_element) {
        // 找到块级元素中的最后一个文本节点
        std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findLastTextNode;
        findLastTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Text> {
            if (!node) return nullptr;
            if (node->GetNodeType() == NodeType::TEXT_NODE) {
                return std::dynamic_pointer_cast<Text>(node);
            }
            if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto elem = std::dynamic_pointer_cast<Element>(node);
                if (elem) {
                    auto children = elem->GetChildNodes();
                    for (auto it = children.rbegin(); it != children.rend(); ++it) {
                        auto result = findLastTextNode(*it);
                        if (result) return result;
                    }
                }
            }
            return nullptr;
        };

        auto last_text = findLastTextNode(block_element);
        if (last_text) {
            int end_offset = static_cast<int>(last_text->GetTextContent().length());
            if (extend_selection) {
                selection->Extend(last_text, end_offset);
            } else {
                selection->Collapse(last_text, end_offset);
            }
            return true;
        }
    }

    // 回退：移动到当前文本节点末尾
    if (anchor_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(anchor_node);
        if (text_node) {
            int end_offset = static_cast<int>(text_node->GetTextContent().length());
            if (extend_selection) {
                selection->Extend(anchor_node, end_offset);
            } else {
                selection->Collapse(anchor_node, end_offset);
            }
            return true;
        }
    }

    return false;
}

bool ContentEditableHandler::MoveCursorToPreviousWord(
    std::shared_ptr<Document> document,
    bool extend_selection) {

    if (!document) return false;

    auto selection = GetSelection(document);
    if (!selection) return false;

    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();

    if (!anchor_node || anchor_node->GetNodeType() != NodeType::TEXT_NODE) {
        return MoveCursorLeft(document, extend_selection);
    }

    auto text_node = std::dynamic_pointer_cast<Text>(anchor_node);
    if (!text_node) return false;

    std::string content = text_node->GetTextContent();

    // 跳过当前位置前的空白
    int pos = anchor_offset;
    while (pos > 0 && std::isspace(static_cast<unsigned char>(content[pos - 1]))) {
        pos--;
    }

    // 跳过单词字符
    while (pos > 0 && !std::isspace(static_cast<unsigned char>(content[pos - 1]))) {
        pos--;
    }

    if (pos != anchor_offset) {
        if (extend_selection) {
            selection->Extend(anchor_node, pos);
        } else {
            selection->Collapse(anchor_node, pos);
        }
        return true;
    }

    // 如果已在开头，移动到前一个文本节点
    auto prev_text = FindPreviousTextNode(anchor_node);
    if (prev_text) {
        auto prev_text_node = std::dynamic_pointer_cast<Text>(prev_text);
        if (prev_text_node) {
            int new_offset = static_cast<int>(prev_text_node->GetTextContent().length());
            if (extend_selection) {
                selection->Extend(prev_text, new_offset);
            } else {
                selection->Collapse(prev_text, new_offset);
            }
            return true;
        }
    }

    return false;
}

bool ContentEditableHandler::MoveCursorToNextWord(
    std::shared_ptr<Document> document,
    bool extend_selection) {

    if (!document) return false;

    auto selection = GetSelection(document);
    if (!selection) return false;

    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();

    if (!anchor_node || anchor_node->GetNodeType() != NodeType::TEXT_NODE) {
        return MoveCursorRight(document, extend_selection);
    }

    auto text_node = std::dynamic_pointer_cast<Text>(anchor_node);
    if (!text_node) return false;

    std::string content = text_node->GetTextContent();
    int content_length = static_cast<int>(content.length());

    // 跳过当前位置后的单词字符
    int pos = anchor_offset;
    while (pos < content_length && !std::isspace(static_cast<unsigned char>(content[pos]))) {
        pos++;
    }

    // 跳过空白
    while (pos < content_length && std::isspace(static_cast<unsigned char>(content[pos]))) {
        pos++;
    }

    if (pos != anchor_offset) {
        if (extend_selection) {
            selection->Extend(anchor_node, pos);
        } else {
            selection->Collapse(anchor_node, pos);
        }
        return true;
    }

    // 如果已在末尾，移动到下一个文本节点
    auto next_text = FindNextTextNode(anchor_node);
    if (next_text) {
        if (extend_selection) {
            selection->Extend(next_text, 0);
        } else {
            selection->Collapse(next_text, 0);
        }
        return true;
    }

    return false;
}

std::shared_ptr<Node> ContentEditableHandler::FindPreviousTextNode(
    std::shared_ptr<Node> current_node) {

    if (!current_node) return nullptr;

    // 查找可编辑区域的根元素
    auto editable_root = FindEditableElement(current_node);
    if (!editable_root) {
        return nullptr;
    }

    // 辅助函数：在节点中查找最后一个文本节点
    std::function<std::shared_ptr<Node>(std::shared_ptr<Node>)> findLastTextNode;
    findLastTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Node> {
        if (!node) return nullptr;
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            return node;
        }
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                auto children = elem->GetChildNodes();
                for (auto it = children.rbegin(); it != children.rend(); ++it) {
                    auto result = findLastTextNode(*it);
                    if (result) return result;
                }
            }
        }
        return nullptr;
    };

    // 递归向上查找前一个文本节点
    std::function<std::shared_ptr<Node>(std::shared_ptr<Node>)> findPrevious;
    findPrevious = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Node> {
        if (!node || node == editable_root) {
            return nullptr;
        }
        
        // 检查前一个兄弟
        auto prev_sibling = node->GetPreviousSibling();
        if (prev_sibling) {
            auto result = findLastTextNode(prev_sibling);
            if (result) return result;
        }
        
        // 没有前一个兄弟，向上到父节点继续查找
        auto parent = node->GetParentNode();
        if (parent && parent != editable_root) {
            return findPrevious(parent);
        }
        
        return nullptr;
    };

    return findPrevious(current_node);
}

std::shared_ptr<Node> ContentEditableHandler::FindNextTextNode(
    std::shared_ptr<Node> current_node) {

    if (!current_node) return nullptr;

    // 查找可编辑区域的根元素
    auto editable_root = FindEditableElement(current_node);
    if (!editable_root) return nullptr;

    // 辅助函数：在节点中查找第一个文本节点
    std::function<std::shared_ptr<Node>(std::shared_ptr<Node>)> findFirstTextNode;
    findFirstTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Node> {
        if (!node) return nullptr;
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            return node;
        }
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                for (auto& child : elem->GetChildNodes()) {
                    auto result = findFirstTextNode(child);
                    if (result) return result;
                }
            }
        }
        return nullptr;
    };

    // 递归向上查找下一个文本节点
    std::function<std::shared_ptr<Node>(std::shared_ptr<Node>)> findNext;
    findNext = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Node> {
        if (!node || node == editable_root) return nullptr;
        
        // 检查下一个兄弟
        auto next_sibling = node->GetNextSibling();
        if (next_sibling) {
            auto result = findFirstTextNode(next_sibling);
            if (result) return result;
        }
        
        // 没有下一个兄弟，向上到父节点继续查找
        auto parent = node->GetParentNode();
        if (parent && parent != editable_root) {
            return findNext(parent);
        }
        
        return nullptr;
    };

    return findNext(current_node);
}

// ========== 撤销/重做 ==========

void ContentEditableHandler::SaveUndoState(std::shared_ptr<Element> element) {
    if (!element) return;

    UndoState state;
    state.innerHTML = element->GetInnerHTML();
    state.element = element;
    state.anchor_offset = 0;
    state.focus_offset = 0;
    state.is_collapsed = true;

    // 保存光标位置（包括节点引用）
    auto doc = element->GetOwnerDocument();
    if (doc && selection_manager_) {
        auto selection = selection_manager_->GetSelection(doc);
        if (selection) {
            state.anchor_node = selection->GetAnchorNode();
            state.anchor_offset = selection->GetAnchorOffset();
            state.focus_node = selection->GetFocusNode();
            state.focus_offset = selection->GetFocusOffset();
            state.is_collapsed = selection->IsCollapsed();
        }
    }

    // 检查是否与上一个状态相同（避免重复保存）
    if (!undo_stack_.empty()) {
        auto& last = undo_stack_.back();
        auto last_elem = last.element.lock();
        if (last_elem == element && last.innerHTML == state.innerHTML) {
            return;  // 状态相同，不保存
        }
    }

    // 添加到撤销栈
    undo_stack_.push_back(state);

    // 限制栈大小
    while (undo_stack_.size() > MAX_UNDO_STACK_SIZE) {
        undo_stack_.pop_front();
    }

    // 清空重做栈（新操作后重做历史失效）
    redo_stack_.clear();
}

bool ContentEditableHandler::Undo(std::shared_ptr<Document> document) {
    if (!document || undo_stack_.empty()) {
        return false;
    }

    // 从撤销栈获取要恢复的状态
    UndoState prev_state = undo_stack_.back();
    undo_stack_.pop_back();
    
    auto target_element = prev_state.element.lock();
    if (!target_element) {
        return false;
    }

    // 保存当前状态到重做栈（在修改之前获取当前内容）
    UndoState current_state;
    current_state.innerHTML = target_element->GetInnerHTML();
    current_state.element = target_element;
    
    // 保存当前光标位置
    if (selection_manager_) {
        auto selection = selection_manager_->GetSelection(document);
        if (selection) {
            current_state.anchor_node = selection->GetAnchorNode();
            current_state.anchor_offset = selection->GetAnchorOffset();
            current_state.focus_node = selection->GetFocusNode();
            current_state.focus_offset = selection->GetFocusOffset();
            current_state.is_collapsed = selection->IsCollapsed();
        }
    }

    // 只有当内容不同时才保存到重做栈
    if (current_state.innerHTML != prev_state.innerHTML) {
        redo_stack_.push_back(current_state);
    }

    // 恢复状态
    target_element->SetInnerHTML(prev_state.innerHTML);
    
    // 尝试恢复光标位置
    // 注意：由于 innerHTML 被替换，原来的节点引用已失效
    // 我们需要根据偏移量在新的 DOM 中找到对应位置
    if (selection_manager_) {
        auto selection = selection_manager_->GetSelection(document);
        if (selection) {
            // 简单恢复：折叠到元素开头的第一个文本节点
            std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findFirstTextNode;
            findFirstTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Text> {
                if (!node) return nullptr;
                if (node->GetNodeType() == NodeType::TEXT_NODE) {
                    return std::dynamic_pointer_cast<Text>(node);
                }
                if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto elem = std::dynamic_pointer_cast<Element>(node);
                    if (elem) {
                        for (auto& child : elem->GetChildNodes()) {
                            auto result = findFirstTextNode(child);
                            if (result) return result;
                        }
                    }
                }
                return nullptr;
            };
            
            auto first_text = findFirstTextNode(target_element);
            if (first_text) {
                // 尝试恢复到保存的偏移量位置
                int offset = std::min(prev_state.anchor_offset, 
                                     static_cast<int>(first_text->GetTextContent().length()));
                selection->Collapse(first_text, offset);
            }
        }
    }

    return true;
}

bool ContentEditableHandler::Redo(std::shared_ptr<Document> document) {
    if (!document || redo_stack_.empty()) {
        return false;
    }

    // 从重做栈获取要恢复的状态
    UndoState redo_state = redo_stack_.back();
    redo_stack_.pop_back();
    
    auto target_element = redo_state.element.lock();
    if (!target_element) {
        return false;
    }

    // 保存当前状态到撤销栈（在修改之前获取当前内容）
    UndoState current_state;
    current_state.innerHTML = target_element->GetInnerHTML();
    current_state.element = target_element;
    
    // 保存当前光标位置
    if (selection_manager_) {
        auto selection = selection_manager_->GetSelection(document);
        if (selection) {
            current_state.anchor_node = selection->GetAnchorNode();
            current_state.anchor_offset = selection->GetAnchorOffset();
            current_state.focus_node = selection->GetFocusNode();
            current_state.focus_offset = selection->GetFocusOffset();
            current_state.is_collapsed = selection->IsCollapsed();
        }
    }
    current_state.element = target_element;

    // 只有当内容不同时才保存到撤销栈
    if (current_state.innerHTML != redo_state.innerHTML) {
        undo_stack_.push_back(current_state);
    }

    // 恢复状态
    target_element->SetInnerHTML(redo_state.innerHTML);

    // 尝试恢复光标位置
    if (selection_manager_) {
        auto selection = selection_manager_->GetSelection(document);
        if (selection) {
            std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findFirstTextNode;
            findFirstTextNode = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Text> {
                if (!node) return nullptr;
                if (node->GetNodeType() == NodeType::TEXT_NODE) {
                    return std::dynamic_pointer_cast<Text>(node);
                }
                if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto elem = std::dynamic_pointer_cast<Element>(node);
                    if (elem) {
                        for (auto& child : elem->GetChildNodes()) {
                            auto result = findFirstTextNode(child);
                            if (result) return result;
                        }
                    }
                }
                return nullptr;
            };
            
            auto first_text = findFirstTextNode(target_element);
            if (first_text) {
                int offset = std::min(redo_state.anchor_offset, 
                                     static_cast<int>(first_text->GetTextContent().length()));
                selection->Collapse(first_text, offset);
            }
        }
    }

    return true;
}

void ContentEditableHandler::BeginUndoGroup(std::shared_ptr<Element> element) {
    if (!element || in_undo_group_) {
        return;
    }

    in_undo_group_ = true;
    
    // 保存组开始时的状态
    undo_group_start_state_.innerHTML = element->GetInnerHTML();
    undo_group_start_state_.element = element;
    undo_group_start_state_.anchor_offset = 0;
    undo_group_start_state_.focus_offset = 0;
    undo_group_start_state_.is_collapsed = true;

    auto doc = element->GetOwnerDocument();
    if (doc && selection_manager_) {
        auto selection = selection_manager_->GetSelection(doc);
        if (selection) {
            undo_group_start_state_.anchor_node = selection->GetAnchorNode();
            undo_group_start_state_.anchor_offset = selection->GetAnchorOffset();
            undo_group_start_state_.focus_node = selection->GetFocusNode();
            undo_group_start_state_.focus_offset = selection->GetFocusOffset();
            undo_group_start_state_.is_collapsed = selection->IsCollapsed();
        }
    }
}

void ContentEditableHandler::EndUndoGroup() {
    if (!in_undo_group_) {
        return;
    }

    in_undo_group_ = false;

    auto element = undo_group_start_state_.element.lock();
    if (!element) {
        return;
    }

    // 检查内容是否有变化
    std::string current_html = element->GetInnerHTML();
    if (current_html == undo_group_start_state_.innerHTML) {
        // 没有变化，不保存
        return;
    }

    // 保存组开始时的状态到撤销栈
    undo_stack_.push_back(undo_group_start_state_);

    // 限制栈大小
    while (undo_stack_.size() > MAX_UNDO_STACK_SIZE) {
        undo_stack_.pop_front();
    }

    // 清空重做栈
    redo_stack_.clear();
}

} // namespace lightui
