/**
 * @file focus_manager.cpp
 * @brief 焦点管理器实现
 */

#include "focus_manager.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include "core/dom/event.h"
#include "core/dom/node.h"
#include "core/dom/html_input_element.h"
#include "core/dom/html_textarea_element.h"
#include "core/window/window.h"
#include <SDL3/SDL.h>

namespace lightui {

FocusManager::FocusManager() {
}

FocusManager::~FocusManager() {
    // 析构时从文档注销
    UnregisterFromDocument();
}

void FocusManager::RegisterWithDocument(std::shared_ptr<Document> document) {
    if (!document) {
        return;
    }

    // 如果已经注册到同一个文档，不需要重复注册
    auto current_doc = registered_document_.lock();
    if (current_doc == document) {
        return;
    }

    // 先从旧文档注销
    UnregisterFromDocument();

    // 注册到新文档
    document->AddObserver(this);
    registered_document_ = document;
}

void FocusManager::UnregisterFromDocument() {
    auto doc = registered_document_.lock();
    if (doc) {
        doc->RemoveObserver(this);
    }
    registered_document_.reset();
}

bool FocusManager::SetFocus(std::shared_ptr<Element> element, bool focus_visible) {
    if (!element) {
        return false;
    }

    // 检查元素是否可聚焦
    if (!IsFocusable(element)) {
        return false;
    }

    // 获取当前焦点元素
    auto old_focus = focus_element_.lock();

    // 如果已经是焦点元素，不需要重复设置
    if (old_focus == element) {
        return true;
    }

    // 注册到元素所属文档的观察者管理器
    // 这样当元素被移除时，我们会收到通知并清除焦点
    // 参考 Chrome/Blink: 焦点元素被移除时自动清除焦点
    auto doc = std::dynamic_pointer_cast<Document>(element->GetOwnerDocument());
    if (doc) {
        RegisterWithDocument(doc);
    }

    // 发送焦点变化事件
    SendFocusEvents(old_focus, element, focus_visible);

    // 更新焦点元素
    focus_element_ = element;

    // 如果是输入元素，启用SDL文本输入
    std::string tag_name = element->GetTagName();
    if (tag_name == "input" || tag_name == "textarea") {
        if (window_) {
            SDL_StartTextInput(window_->GetSDLWindow());
        }
    }

    // 触发重绘以显示光标
    if (window_) {
        window_->SetNeedsRepaint();
    }

    return true;
}

void FocusManager::Blur(std::shared_ptr<Element> element) {
    auto current_focus = focus_element_.lock();

    if (current_focus == element) {
        // 如果是输入元素，停止SDL文本输入
        std::string tag_name = element->GetTagName();
        if (tag_name == "input" || tag_name == "textarea") {
            if (window_) {
                SDL_StopTextInput(window_->GetSDLWindow());
            }
        }

        // 发送blur事件
        SendFocusEvents(current_focus, nullptr, false);

        // 清除焦点
        focus_element_.reset();

        // 触发重绘以隐藏光标
        if (window_) {
            window_->SetNeedsRepaint();
        }
    }
}

std::shared_ptr<Element> FocusManager::GetFocusElement() const {
    // 参考 Chrome/Blink: document.activeElement 返回当前焦点元素
    // 如果焦点元素已被销毁（weak_ptr 过期），则返回 nullptr
    // 不在这里自动清除焦点，焦点应该通过 ClearFocus() 或元素移除事件来清除
    auto element = focus_element_.lock();
    return element;
}

bool FocusManager::TabToNextFocusableElement(std::shared_ptr<Document> current_document, bool reverse) {
    if (!current_document) {
        return false;
    }

    // 收集所有可聚焦元素
    std::vector<std::shared_ptr<Element>> focusable_elements;
    auto root = current_document->GetDocumentElement();
    if (root) {
        CollectFocusableElements(root, focusable_elements);
    }

    if (focusable_elements.empty()) {
        return false;
    }

    // 按tabindex排序
    std::sort(focusable_elements.begin(), focusable_elements.end(),
        [this](const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b) {
            int tab_a = GetTabIndex(a);
            int tab_b = GetTabIndex(b);
            
            // tabindex > 0 的元素优先
            if (tab_a > 0 && tab_b > 0) {
                return tab_a < tab_b;
            }
            if (tab_a > 0) {
                return true;
            }
            if (tab_b > 0) {
                return false;
            }
            
            // tabindex = 0 的元素按DOM顺序
            return false;
        });

    // 查找当前焦点元素的位置
    auto current_focus = focus_element_.lock();
    int current_index = -1;
    
    if (current_focus) {
        for (size_t i = 0; i < focusable_elements.size(); ++i) {
            if (focusable_elements[i] == current_focus) {
                current_index = static_cast<int>(i);
                break;
            }
        }
    }

    // 计算下一个焦点元素的索引
    int next_index;
    if (current_index == -1) {
        // 没有当前焦点，选择第一个或最后一个
        next_index = reverse ? static_cast<int>(focusable_elements.size()) - 1 : 0;
    } else {
        // 移动到下一个或上一个
        next_index = reverse ? current_index - 1 : current_index + 1;
        
        // 循环
        if (next_index < 0) {
            next_index = static_cast<int>(focusable_elements.size()) - 1;
        } else if (next_index >= static_cast<int>(focusable_elements.size())) {
            next_index = 0;
        }
    }

    // 设置焦点到下一个元素
    return SetFocus(focusable_elements[next_index], true);  // focus_visible = true for keyboard navigation
}

void FocusManager::ClearFocus() {
    auto current_focus = focus_element_.lock();
    if (current_focus) {
        // 如果是输入元素，停止SDL文本输入
        std::string tag_name = current_focus->GetTagName();
        if (tag_name == "input" || tag_name == "textarea") {
            if (window_) {
                SDL_StopTextInput(window_->GetSDLWindow());
            }
        }

        SendFocusEvents(current_focus, nullptr, false);

        // 触发重绘
        if (window_) {
            window_->SetNeedsRepaint();
        }
    }
    focus_element_.reset();
}

void FocusManager::OnNodeRemoved(Node* node, Node* parent) {
    // 参考 Chrome/Blink: 当焦点元素被从 DOM 移除时，自动清除焦点
    // 这是正确的焦点管理行为，而不是在 GetFocusElement 中检查

    auto current_focus = focus_element_.lock();
    if (!current_focus || !node) {
        return;
    }

    // 检查被移除的节点是否是焦点元素本身
    if (current_focus.get() == node) {
        ClearFocus();
        return;
    }

    // 检查被移除的节点是否包含焦点元素（通过检查焦点元素的祖先链）
    // 注意：此时节点可能已经从 DOM 断开，所以需要检查焦点元素的父节点链
    std::shared_ptr<Node> current = current_focus;
    while (current) {
        if (current.get() == node) {
            ClearFocus();
            return;
        }
        current = current->GetParentNode();
    }
}

bool FocusManager::ProcessAutofocus(std::shared_ptr<Document> document) {
    if (!document) {
        return false;
    }

    // 查找第一个有autofocus属性的可聚焦元素
    // 参考：W3C HTML5 - autofocus属性
    auto root = document->GetDocumentElement();
    if (!root) {
        return false;
    }

    // 递归查找autofocus元素
    std::function<std::shared_ptr<Element>(std::shared_ptr<Element>)> find_autofocus;
    find_autofocus = [&](std::shared_ptr<Element> element) -> std::shared_ptr<Element> {
        if (!element) {
            return nullptr;
        }

        // 检查当前元素是否有autofocus属性
        if (element->HasAttribute("autofocus") && IsFocusable(element)) {
            return element;
        }

        // 递归检查子元素
        auto children = element->GetChildNodes();
        for (const auto& child : children) {
            if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto child_element = std::static_pointer_cast<Element>(child);
                auto result = find_autofocus(child_element);
                if (result) {
                    return result;
                }
            }
        }

        return nullptr;
    };

    auto autofocus_element = find_autofocus(root);
    if (autofocus_element) {
        // 设置焦点，focus_visible=false（autofocus不显示焦点指示器）
        return SetFocus(autofocus_element, false);
    }

    return false;
}

std::shared_ptr<Element> FocusManager::FindFocusableElement(std::shared_ptr<Element> element) {
    if (!element) {
        return nullptr;
    }

    // 如果元素本身可聚焦，返回它
    if (IsFocusable(element)) {
        return element;
    }

    // 否则，向上查找父元素
    auto parent = element->GetParentNode();
    if (parent && parent->GetNodeType() == NodeType::ELEMENT_NODE) {
        return FindFocusableElement(std::static_pointer_cast<Element>(parent));
    }

    return nullptr;
}

void FocusManager::CollectFocusableElements(std::shared_ptr<Element> root,
                                           std::vector<std::shared_ptr<Element>>& focusable_elements) {
    if (!root) {
        return;
    }

    // 检查当前元素是否可聚焦
    if (IsFocusable(root)) {
        int tab_index = GetTabIndex(root);
        // 只收集tabindex >= 0的元素（tabindex < 0表示不可通过Tab导航）
        if (tab_index >= 0) {
            focusable_elements.push_back(root);
        }
    }

    // 递归收集子元素
    auto children = root->GetChildNodes();
    for (const auto& child : children) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            CollectFocusableElements(std::static_pointer_cast<Element>(child), focusable_elements);
        }
    }
}

bool FocusManager::IsFocusable(std::shared_ptr<Element> element) {
    if (!element) {
        return false;
    }

    std::string tag_name = element->GetTagName();

    // 检查元素是否有tabindex属性
    std::string tabindex_str = element->GetAttribute("tabindex");
    if (!tabindex_str.empty()) {
        // 有tabindex属性的元素都可聚焦（即使tabindex=-1）
        return true;
    }

    // 检查是否是默认可聚焦的元素
    if (tag_name == "input" || tag_name == "button" ||
        tag_name == "select" || tag_name == "textarea" ||
        tag_name == "a") {
        // 检查是否被禁用
        std::string disabled = element->GetAttribute("disabled");
        if (disabled == "true" || disabled == "disabled") {
            return false;
        }
        return true;
    }

    return false;
}

int FocusManager::GetTabIndex(std::shared_ptr<Element> element) {
    if (!element) {
        return -1;
    }

    std::string tabindex_str = element->GetAttribute("tabindex");
    if (tabindex_str.empty()) {
        // 默认可聚焦元素的默认tabindex为0
        std::string tag_name = element->GetTagName();
        if (tag_name == "input" || tag_name == "button" || 
            tag_name == "select" || tag_name == "textarea" ||
            tag_name == "a") {
            return 0;
        }
        return -1;
    }

    // 解析tabindex
    try {
        return std::stoi(tabindex_str);
    } catch (...) {
        return -1;
    }
}

void FocusManager::SendFocusEvents(std::shared_ptr<Element> old_focus,
                                  std::shared_ptr<Element> new_focus,
                                  bool focus_visible) {
    // 参考：RmlUi/Source/Core/Context.cpp - OnFocusChange

    // 构建焦点链（从元素到根）
    // 使用 shared_ptr 而不是裸指针，避免访问已销毁的对象
    std::vector<std::shared_ptr<Element>> old_chain;
    std::vector<std::shared_ptr<Element>> new_chain;

    // 构建旧焦点链
    if (old_focus) {
        std::shared_ptr<Node> current = old_focus;
        while (current) {
            if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
                old_chain.push_back(std::static_pointer_cast<Element>(current));
            }
            current = current->GetParentNode();
        }
    }

    // 构建新焦点链
    if (new_focus) {
        std::shared_ptr<Node> current = new_focus;
        while (current) {
            if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
                new_chain.push_back(std::static_pointer_cast<Element>(current));
            }
            current = current->GetParentNode();
        }
    }

    // 发送blur/focusout事件到离开焦点链的元素
    // 参考：W3C UI Events - focusout是blur的冒泡版本
    for (const auto& element : old_chain) {
        // 检查元素是否在新焦点链中
        bool in_new_chain = false;
        for (const auto& new_elem : new_chain) {
            if (new_elem == element) {
                in_new_chain = true;
                break;
            }
        }

        if (!in_new_chain) {
            try {
                // 发送blur事件（不冒泡）
                auto blur_event = std::make_shared<Event>("blur");
                element->DispatchEvent(blur_event);

                // 发送focusout事件（冒泡）
                auto focusout_event = std::make_shared<Event>("focusout");
                element->DispatchEvent(focusout_event);

                // 移除:focus和:focus-visible伪类
                element->SetPseudoClass("focus", false);
                element->SetPseudoClass("focus-visible", false);
            } catch (const std::exception& e) {
                // 元素已被销毁或发生其他错误，忽略
            } catch (...) {
                // 元素已被销毁，忽略
            }
        }
    }

    // 发送focus/focusin事件到进入焦点链的元素
    // 参考：W3C UI Events - focusin是focus的冒泡版本
    for (const auto& element : new_chain) {
        // 检查元素是否在旧焦点链中
        bool in_old_chain = false;
        for (const auto& old_elem : old_chain) {
            if (old_elem == element) {
                in_old_chain = true;
                break;
            }
        }

        if (!in_old_chain) {
            try {
                // 发送focus事件（不冒泡）
                auto focus_event = std::make_shared<Event>("focus");
                element->DispatchEvent(focus_event);

                // 发送focusin事件（冒泡）
                auto focusin_event = std::make_shared<Event>("focusin");
                element->DispatchEvent(focusin_event);

                // 设置:focus伪类
                element->SetPseudoClass("focus", true);

                // 如果是键盘导航，设置:focus-visible伪类
                if (focus_visible) {
                    element->SetPseudoClass("focus-visible", true);
                }
            } catch (const std::exception& e) {
                // 元素已被销毁或发生其他错误，忽略
            } catch (...) {
                // 元素已被销毁，忽略
            }
        }
    }
}

} // namespace lightui

