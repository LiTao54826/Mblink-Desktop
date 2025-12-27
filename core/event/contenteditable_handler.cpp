/**
 * @file contenteditable_handler.cpp
 * @brief ContentEditable 输入处理器实现
 */

#include "contenteditable_handler.h"
#include "selection_manager.h"
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

    std::cout << "[ContentEditableHandler::HandleTextInput] text='" << text << "'" << std::endl;

    if (!target || !IsEditable(target)) {
        std::cout << "[ContentEditableHandler::HandleTextInput] target not editable" << std::endl;
        return false;
    }

    // 分发 beforeinput 事件
    if (!DispatchBeforeInputEvent(target, "insertText", text)) {
        std::cout << "[ContentEditableHandler::HandleTextInput] beforeinput cancelled" << std::endl;
        return false;  // 事件被取消
    }

    // 获取文档
    auto document = target->GetOwnerDocument();
    if (!document) {
        std::cout << "[ContentEditableHandler::HandleTextInput] no document" << std::endl;
        return false;
    }

    // 插入文本
    if (!InsertText(document, text)) {
        std::cout << "[ContentEditableHandler::HandleTextInput] InsertText failed" << std::endl;
        return false;
    }

    std::cout << "[ContentEditableHandler::HandleTextInput] success" << std::endl;

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

    std::cout << "[ContentEditableHandler::HandleKeyDown] key_code=" << key_code << std::endl;

    if (!target || !IsEditable(target)) {
        std::cout << "[ContentEditableHandler::HandleKeyDown] target not editable" << std::endl;
        return false;
    }

    auto document = target->GetOwnerDocument();
    if (!document) {
        std::cout << "[ContentEditableHandler::HandleKeyDown] no document" << std::endl;
        return false;
    }

    // 处理特殊键
    switch (key_code) {
        case 8:  // Backspace
            std::cout << "[ContentEditableHandler::HandleKeyDown] Backspace pressed" << std::endl;
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
                    case 90:  // Ctrl+Z (Undo) - TODO: 实现撤销
                        return false;
                    case 89:  // Ctrl+Y (Redo) - TODO: 实现重做
                        return false;
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

    std::cout << "[ContentEditableHandler::InsertText] text='" << text << "'" << std::endl;

    if (!document || text.empty()) {
        std::cout << "[ContentEditableHandler::InsertText] no document or empty text" << std::endl;
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        std::cout << "[ContentEditableHandler::InsertText] no selection" << std::endl;
        return false;
    }

    std::cout << "[ContentEditableHandler::InsertText] selection isCollapsed=" << selection->IsCollapsed() << std::endl;

    // 如果有选中内容，先删除
    if (!selection->IsCollapsed()) {
        DeleteSelection(document);
    }

    // 获取光标位置
    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();

    if (!anchor_node) {
        std::cout << "[ContentEditableHandler::InsertText] no anchor node" << std::endl;
        return false;
    }

    std::cout << "[ContentEditableHandler::InsertText] anchor_node type=" << static_cast<int>(anchor_node->GetNodeType()) 
              << " offset=" << anchor_offset << std::endl;

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

    // TODO: 处理跨节点删除的复杂情况

    return false;
}

bool ContentEditableHandler::DeleteCharacter(
    std::shared_ptr<Document> document,
    bool forward) {

    if (!document) {
        return false;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        std::cout << "[ContentEditableHandler::DeleteCharacter] no selection" << std::endl;
        return false;
    }

    std::cout << "[ContentEditableHandler::DeleteCharacter] forward=" << forward 
              << " isCollapsed=" << selection->IsCollapsed() << std::endl;

    // 如果有选中内容，删除选中内容
    if (!selection->IsCollapsed()) {
        return DeleteSelection(document);
    }

    // 获取光标位置
    auto anchor_node = selection->GetAnchorNode();
    int anchor_offset = selection->GetAnchorOffset();

    if (!anchor_node) {
        std::cout << "[ContentEditableHandler::DeleteCharacter] no anchor node" << std::endl;
        return false;
    }

    std::cout << "[ContentEditableHandler::DeleteCharacter] anchor_node type=" 
              << static_cast<int>(anchor_node->GetNodeType()) 
              << " offset=" << anchor_offset << std::endl;

    // 处理文本节点
    if (anchor_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(anchor_node);
        if (!text_node) {
            return false;
        }

        std::string content = text_node->GetTextContent();
        std::cout << "[ContentEditableHandler::DeleteCharacter] content='" << content 
                  << "' length=" << content.length() << std::endl;

        if (forward) {
            // Delete: 删除光标后的字符
            if (anchor_offset >= static_cast<int>(content.length())) {
                // 已在末尾，尝试合并下一个节点
                // TODO: 实现向前合并
                std::cout << "[ContentEditableHandler::DeleteCharacter] already at end, try merge next" << std::endl;
                return false;
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
                std::cout << "[ContentEditableHandler::DeleteCharacter] at beginning, try merge previous" << std::endl;
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
        }

        std::cout << "[ContentEditableHandler::DeleteCharacter] success, new content='" << content << "'" << std::endl;
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

bool ContentEditableHandler::ApplyBold(std::shared_ptr<Document> document) {
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

        // 创建新的结构：before_text + <strong>selected</strong> + after_text
        if (!before.empty()) {
            auto before_text = std::make_shared<Text>(before);
            parent_element->InsertBefore(before_text, text_node);
        }

        // 创建 <strong> 元素
        auto strong_element = document->CreateElement("strong");
        auto selected_text = std::make_shared<Text>(selected);
        strong_element->AppendChild(selected_text);
        parent_element->InsertBefore(strong_element, text_node);

        if (!after.empty()) {
            auto after_text = std::make_shared<Text>(after);
            parent_element->InsertBefore(after_text, text_node);
        }

        // 移除原文本节点
        parent_element->RemoveChild(text_node);

        // 更新选择到新的粗体文本
        selection->Collapse(selected_text, static_cast<int>(selected.length()));

        return true;
    }

    // TODO: 处理跨节点的复杂情况
    return false;
}

bool ContentEditableHandler::ApplyItalic(std::shared_ptr<Document> document) {
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

        // 创建新的结构：before_text + <em>selected</em> + after_text
        if (!before.empty()) {
            auto before_text = std::make_shared<Text>(before);
            parent_element->InsertBefore(before_text, text_node);
        }

        // 创建 <em> 元素
        auto em_element = document->CreateElement("em");
        auto selected_text = std::make_shared<Text>(selected);
        em_element->AppendChild(selected_text);
        parent_element->InsertBefore(em_element, text_node);

        if (!after.empty()) {
            auto after_text = std::make_shared<Text>(after);
            parent_element->InsertBefore(after_text, text_node);
        }

        // 移除原文本节点
        parent_element->RemoveChild(text_node);

        // 更新选择
        selection->Collapse(selected_text, static_cast<int>(selected.length()));

        return true;
    }

    // TODO: 处理跨节点的复杂情况
    return false;
}

bool ContentEditableHandler::ApplyUnderline(std::shared_ptr<Document> document) {
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

        // 创建新的结构：before_text + <u>selected</u> + after_text
        if (!before.empty()) {
            auto before_text = std::make_shared<Text>(before);
            parent_element->InsertBefore(before_text, text_node);
        }

        // 创建 <u> 元素
        auto u_element = document->CreateElement("u");
        auto selected_text = std::make_shared<Text>(selected);
        u_element->AppendChild(selected_text);
        parent_element->InsertBefore(u_element, text_node);

        if (!after.empty()) {
            auto after_text = std::make_shared<Text>(after);
            parent_element->InsertBefore(after_text, text_node);
        }

        // 移除原文本节点
        parent_element->RemoveChild(text_node);

        // 更新选择
        selection->Collapse(selected_text, static_cast<int>(selected.length()));

        return true;
    }

    // TODO: 处理跨节点的复杂情况
    return false;
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

    // 向上遍历查找可编辑元素
    auto current = node;
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(current);
            if (element && element->IsContentEditable()) {
                return element;
            }
        }
        current = current->GetParentNode();
    }

    return nullptr;
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

    std::cout << "[MergeToPreviousNode] Starting merge" << std::endl;

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
            std::cout << "[MergeToPreviousNode] Cursor moved to end of text node" << std::endl;
            return true;
        }
        
        // 如果没有文本节点，在元素中创建一个空文本节点
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem) {
                auto empty_text = std::make_shared<Text>("");
                elem->AppendChild(empty_text);
                selection->Collapse(empty_text, 0);
                std::cout << "[MergeToPreviousNode] Created empty text node and set cursor" << std::endl;
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
        std::cout << "[MergeToPreviousNode] No parent" << std::endl;
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
                    
                    std::cout << "[MergeToPreviousNode] Parent tag=" << tag 
                              << " isEmpty=" << parent_is_empty << std::endl;
                    
                    // 情况1：空的格式化元素（em, strong 等）
                    if (isFormattingElement(tag) && parent_is_empty) {
                        std::cout << "[MergeToPreviousNode] Removing empty formatting element: " << tag << std::endl;
                        
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
                                        std::cout << "[MergeToPreviousNode] Cursor moved to previous text node" << std::endl;
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
                                    std::cout << "[MergeToPreviousNode] Cursor moved to last text in grandparent" << std::endl;
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
                        std::cout << "[MergeToPreviousNode] Removing empty block element: " << tag << std::endl;
                        
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
                                        std::cout << "[MergeToPreviousNode] Cursor moved to end of previous block" << std::endl;
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
                    std::cout << "[MergeToPreviousNode] Cannot delete contentEditable root" << std::endl;
                    return false;
                }
                auto parent_elem_check = std::dynamic_pointer_cast<Element>(parent_node);
                if (!parent_elem_check || !parent_elem_check->IsContentEditable()) {
                    std::cout << "[MergeToPreviousNode] Cannot delete contentEditable root" << std::endl;
                    return false;
                }
            }
            
            std::string tag = elem->GetTagName();
            for (auto& c : tag) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            
            if (isBlockElement(tag) || isFormattingElement(tag)) {
                std::cout << "[MergeToPreviousNode] Removing empty element (recursive): " << tag << std::endl;
                
                auto elem_prev = elem->GetPreviousSibling();
                auto elem_parent = elem->GetParentNode();
                
                std::cout << "[MergeToPreviousNode] elem_prev=" << (elem_prev ? "exists" : "null") 
                          << " elem_parent=" << (elem_parent ? "exists" : "null") << std::endl;
                
                if (elem_parent) {
                    auto parent_elem = std::dynamic_pointer_cast<Element>(elem_parent);
                    if (parent_elem) {
                        parent_elem->RemoveChild(elem);
                        
                        if (elem_prev) {
                            std::cout << "[MergeToPreviousNode] Looking for last text in prev sibling" << std::endl;
                            auto last_text = findLastTextNode(elem_prev);
                            if (last_text) {
                                std::cout << "[MergeToPreviousNode] Found last text: '" << last_text->GetTextContent() << "'" << std::endl;
                                selection->Collapse(last_text, static_cast<int>(last_text->GetTextContent().length()));
                                std::cout << "[MergeToPreviousNode] Cursor set to end of previous element" << std::endl;
                                return true;
                            }
                            std::cout << "[MergeToPreviousNode] No text found, using moveCursorToEndOfNode" << std::endl;
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
                std::cout << "[MergeToPreviousNode] Deleted last char from previous text node" << std::endl;
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
                            std::cout << "[MergeToPreviousNode] Deleted last char from parent's previous sibling" << std::endl;
                            return true;
                        }
                    }
                }
            }
        }
    }

    std::cout << "[MergeToPreviousNode] No previous node to merge with" << std::endl;
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

    // TODO: 实现上移光标（需要布局信息）
    // 暂时移动到行首
    return MoveCursorToLineStart(document, extend_selection);
}

bool ContentEditableHandler::MoveCursorDown(
    std::shared_ptr<Document> document,
    bool extend_selection) {

    // TODO: 实现下移光标（需要布局信息）
    // 暂时移动到行尾
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
    if (!editable_root) return nullptr;

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

    // 先检查前一个兄弟
    auto prev_sibling = current_node->GetPreviousSibling();
    if (prev_sibling) {
        auto result = findLastTextNode(prev_sibling);
        if (result) return result;
    }

    // 向上遍历父节点
    auto parent = current_node->GetParentNode();
    while (parent && parent != editable_root) {
        auto parent_prev = parent->GetPreviousSibling();
        if (parent_prev) {
            auto result = findLastTextNode(parent_prev);
            if (result) return result;
        }
        parent = parent->GetParentNode();
    }

    return nullptr;
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

    // 先检查下一个兄弟
    auto next_sibling = current_node->GetNextSibling();
    if (next_sibling) {
        auto result = findFirstTextNode(next_sibling);
        if (result) return result;
    }

    // 向上遍历父节点
    auto parent = current_node->GetParentNode();
    while (parent && parent != editable_root) {
        auto parent_next = parent->GetNextSibling();
        if (parent_next) {
            auto result = findFirstTextNode(parent_next);
            if (result) return result;
        }
        parent = parent->GetParentNode();
    }

    return nullptr;
}

} // namespace lightui
