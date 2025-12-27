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
        return false;
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
                return false;  // 已在末尾
            }
            content.erase(anchor_offset, 1);
            text_node->SetTextContent(content);
            // 光标位置不变
        } else {
            // Backspace: 删除光标前的字符
            if (anchor_offset <= 0) {
                return false;  // 已在开头
            }
            content.erase(anchor_offset - 1, 1);
            text_node->SetTextContent(content);
            // 光标前移一位
            selection->Collapse(anchor_node, anchor_offset - 1);
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

} // namespace lightui
