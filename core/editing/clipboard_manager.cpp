/**
 * @file clipboard_manager.cpp
 * @brief 剪贴板管理器实现
 */

#include "clipboard_manager.h"

#include "contenteditable_handler.h"
#include "selection_manager.h"

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/event.h"
#include "core/dom/selection/selection.h"

#ifdef _WIN32
#include <SDL3/SDL.h>
#endif

namespace lightui {

// ========== 构造函数/析构函数 ==========

ClipboardManager::ClipboardManager(SelectionManager* selection_manager,
                                   ContentEditableHandler* editable_handler)
    : selection_manager_(selection_manager)
    , editable_handler_(editable_handler) {
}

ClipboardManager::~ClipboardManager() = default;

// ========== 剪贴板操作 ==========

bool ClipboardManager::Copy(std::shared_ptr<Document> document) {
    if (!document || !selection_manager_) {
        return false;
    }

    // 获取选中的文本
    std::string selected_text = GetSelectedText(document);
    if (selected_text.empty()) {
        return false;
    }

    // 获取焦点元素用于分发事件
    auto selection = selection_manager_->GetSelection(document);
    if (!selection) {
        return false;
    }

    auto anchor_node = selection->GetAnchorNode();
    auto target = FindEditableElement(anchor_node);

    // 分发 copy 事件
    if (target && !DispatchClipboardEvent(target, "copy")) {
        return false;  // 事件被取消
    }

    // 复制到剪贴板
    SetText(selected_text);

    return true;
}

bool ClipboardManager::Cut(std::shared_ptr<Document> document) {
    if (!document || !selection_manager_ || !editable_handler_) {
        return false;
    }

    // 获取选中的文本
    std::string selected_text = GetSelectedText(document);
    if (selected_text.empty()) {
        return false;
    }

    // 获取焦点元素
    auto selection = selection_manager_->GetSelection(document);
    if (!selection) {
        return false;
    }

    auto anchor_node = selection->GetAnchorNode();
    auto target = FindEditableElement(anchor_node);

    // 检查是否可编辑
    if (!target || !editable_handler_->IsEditable(target)) {
        return false;
    }

    // 分发 cut 事件
    if (!DispatchClipboardEvent(target, "cut")) {
        return false;  // 事件被取消
    }

    // 复制到剪贴板
    SetText(selected_text);

    // 删除选中内容
    editable_handler_->DeleteSelection(document);

    return true;
}

bool ClipboardManager::Paste(std::shared_ptr<Document> document) {
    if (!document || !selection_manager_ || !editable_handler_) {
        return false;
    }

    // 获取剪贴板内容
    std::string clipboard_content = GetText();
    if (clipboard_content.empty()) {
        return false;
    }

    // 获取焦点元素
    auto selection = selection_manager_->GetSelection(document);
    if (!selection) {
        return false;
    }

    auto anchor_node = selection->GetAnchorNode();
    auto target = FindEditableElement(anchor_node);

    // 检查是否可编辑
    if (!target || !editable_handler_->IsEditable(target)) {
        return false;
    }

    // 分发 paste 事件
    if (!DispatchClipboardEvent(target, "paste")) {
        return false;  // 事件被取消
    }

    // 插入文本
    return editable_handler_->InsertText(document, clipboard_content);
}

// ========== 系统剪贴板访问 ==========

std::string ClipboardManager::GetText() const {
#ifdef _WIN32
    // 使用 SDL3 获取系统剪贴板
    if (SDL_HasClipboardText()) {
        char* text = SDL_GetClipboardText();
        if (text) {
            std::string result(text);
            SDL_free(text);
            return result;
        }
    }
#endif
    // 回退到内部缓存
    return clipboard_text_;
}

void ClipboardManager::SetText(const std::string& text) {
#ifdef _WIN32
    // 使用 SDL3 设置系统剪贴板
    SDL_SetClipboardText(text.c_str());
#endif
    // 同时保存到内部缓存
    clipboard_text_ = text;
}

// ========== 事件处理 ==========

bool ClipboardManager::HandleCopyEvent(std::shared_ptr<Element> target) {
    if (!target) {
        return false;
    }

    auto document = target->GetOwnerDocument();
    return Copy(document);
}

bool ClipboardManager::HandleCutEvent(std::shared_ptr<Element> target) {
    if (!target) {
        return false;
    }

    auto document = target->GetOwnerDocument();
    return Cut(document);
}

bool ClipboardManager::HandlePasteEvent(std::shared_ptr<Element> target) {
    if (!target) {
        return false;
    }

    auto document = target->GetOwnerDocument();
    return Paste(document);
}

// ========== 键盘快捷键处理 ==========

bool ClipboardManager::HandleKeyboardShortcut(std::shared_ptr<Document> document,
                                              int key_code,
                                              bool ctrl_key,
                                              bool meta_key) {
    // 检查是否按下了 Ctrl 或 Meta (Cmd)
    if (!ctrl_key && !meta_key) {
        return false;
    }

    switch (key_code) {
        case 67:  // 'C' - Copy
            return Copy(document);

        case 88:  // 'X' - Cut
            return Cut(document);

        case 86:  // 'V' - Paste
            return Paste(document);

        default:
            return false;
    }
}

// ========== 私有方法 ==========

bool ClipboardManager::DispatchClipboardEvent(std::shared_ptr<Element> target,
                                              const std::string& type) {
    if (!target) {
        return true;
    }

    // 获取当前剪贴板内容（用于 paste 事件）
    std::string clipboard_data;
    if (type == "paste") {
        clipboard_data = GetText();
    }

    // 创建剪贴板事件（使用 ClipboardEvent 类）
    auto event = std::make_shared<ClipboardEvent>(type, clipboard_data);

    // 分发事件
    target->DispatchEvent(event);

    // 如果是 copy/cut 事件，检查是否有通过 setData 设置的数据
    if ((type == "copy" || type == "cut") && !event->IsDefaultPrevented()) {
        std::string modified_data = event->GetClipboardData();
        if (!modified_data.empty()) {
            // 使用事件处理器设置的数据
            SetText(modified_data);
        }
    }

    // 返回事件是否被取消
    return !event->IsDefaultPrevented();
}

std::string ClipboardManager::GetSelectedText(std::shared_ptr<Document> document) {
    if (!selection_manager_ || !document) {
        return "";
    }

    return selection_manager_->GetSelectedText(document);
}

std::shared_ptr<Element> ClipboardManager::FindEditableElement(std::shared_ptr<Node> node) {
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
