/**
 * @file mouse_event_dispatcher.cpp
 * @brief 鼠标事件分发器实现
 *
 * 从 event_loop.cpp 提取的鼠标事件处理逻辑。
 * 负责处理 hover 链管理、鼠标光标更新等。
 */

#include "mouse_event_dispatcher.h"

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/elements/html_input_element.h"
#include "core/event/hit_testing.h"
#include "core/event/mouse_event.h"
#include "core/render/render_object.h"
#include "core/window/window.h"
#include "core/window/window_manager.h"

namespace lightui {

MouseEventDispatcher::MouseEventDispatcher() = default;

MouseEventDispatcher::~MouseEventDispatcher() = default;

void MouseEventDispatcher::SetManagers(DragManager* drag_manager,
                                        SelectionManager* selection_manager,
                                        ContentEditableHandler* contenteditable_handler,
                                        FocusManager* focus_manager) {
    drag_manager_ = drag_manager;
    selection_manager_ = selection_manager;
    contenteditable_handler_ = contenteditable_handler;
    focus_manager_ = focus_manager;
}

void MouseEventDispatcher::SetCursorCallback(std::function<void(SDL_SystemCursor)> callback) {
    cursor_callback_ = std::move(callback);
}

bool MouseEventDispatcher::HandleMouseEvent(const SDL_Event& event,
                                             std::shared_ptr<Window> window,
                                             std::shared_ptr<Document> document,
                                             std::shared_ptr<RenderObject> root_render) {
    // 当前为占位实现
    // 完整的事件处理逻辑仍在 EventLoop::HandleMouseEventForDOM 中
    // 未来可以逐步迁移到这里
    (void)event;
    (void)window;
    (void)document;
    (void)root_render;
    return false;
}

void MouseEventDispatcher::UpdateHoverChain(Uint32 window_id,
                                             float mouse_x,
                                             float mouse_y,
                                             const HitTestResult& hit_result) {
    // 获取窗口（用于触发重绘）
    auto& window_manager = WindowManager::Instance();
    auto window = window_manager.FindWindowByID(window_id);
    if (!window) {
        return;
    }

    // 获取新的 hover 目标元素
    std::shared_ptr<Element> new_hover = hit_result.IsValid() ? hit_result.element : nullptr;
    std::shared_ptr<Element> old_hover = hover_element_.lock();

    // 快速路径：如果 hover 元素没有变化，直接返回
    if (new_hover == old_hover) {
        return;
    }

    // 构建新的 hover 链（从目标元素到根元素）
    std::vector<std::weak_ptr<Element>> new_hover_chain;

    if (new_hover) {
        auto current = new_hover;
        while (current) {
            new_hover_chain.push_back(current);

            auto parent_node = current->GetParentNode();
            if (parent_node && parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                current = std::static_pointer_cast<Element>(parent_node);
            } else {
                current = nullptr;
            }
        }
    }

    // 发送 mouseout 事件到离开的元素
    bool changed = SendEvents(hover_chain_, new_hover_chain, "mouseout", mouse_x, mouse_y);

    // 发送 mouseover 事件到进入的元素
    changed |= SendEvents(new_hover_chain, hover_chain_, "mouseover", mouse_x, mouse_y);

    // 如果有伪类变化，触发重绘
    if (changed) {
        window->SetNeedsRepaint();
    }

    // 发送 mouseleave 到旧的 hover 元素
    if (old_hover) {
        auto leave_event = std::make_shared<MouseEvent>(
            "mouseleave",
            static_cast<int>(mouse_x),
            static_cast<int>(mouse_y),
            0
        );
        old_hover->DispatchEvent(leave_event);
    }

    // 发送 mouseenter 到新的 hover 元素
    if (new_hover) {
        auto enter_event = std::make_shared<MouseEvent>(
            "mouseenter",
            static_cast<int>(mouse_x),
            static_cast<int>(mouse_y),
            0
        );
        new_hover->DispatchEvent(enter_event);
    }

    // 更新 hover 链
    hover_chain_ = std::move(new_hover_chain);
    hover_element_ = new_hover;
}

std::shared_ptr<Element> MouseEventDispatcher::GetHoverElement() const {
    return hover_element_.lock();
}

const std::vector<std::weak_ptr<Element>>& MouseEventDispatcher::GetHoverChain() const {
    return hover_chain_;
}

bool MouseEventDispatcher::SendEvents(const std::vector<std::weak_ptr<Element>>& old_items,
                                       const std::vector<std::weak_ptr<Element>>& new_items,
                                       const std::string& event_type,
                                       float mouse_x,
                                       float mouse_y) {
    // 参考：RmlUi/Source/Core/Context.cpp - SendEvents
    // 找出在 old_items 中但不在 new_items 中的元素
    bool has_changes = false;

    for (const auto& weak_elem : old_items) {
        auto element = weak_elem.lock();
        if (!element) {
            continue;
        }

        // 检查是否在新集合中
        bool found = false;
        for (const auto& new_weak : new_items) {
            auto new_elem = new_weak.lock();
            if (new_elem && new_elem == element) {
                found = true;
                break;
            }
        }

        if (!found) {
            // 创建鼠标事件
            auto mouse_event = std::make_shared<MouseEvent>(
                event_type,
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0
            );

            // 分发事件
            element->DispatchEvent(mouse_event);

            // 检查元素当前的 hover 状态
            bool current_hover = element->HasPseudoClass("hover");
            bool new_hover_state = (event_type == "mouseover");

            if (current_hover != new_hover_state) {
                element->SetPseudoClass("hover", new_hover_state);

                // 只有具有内置 hover 样式的元素才需要触发重绘
                std::string tag = element->GetTagName();
                if (tag == "button" || tag == "a" || tag == "input" ||
                    tag == "textarea" || tag == "select") {
                    has_changes = true;
                }
            }
        }
    }

    return has_changes;
}

void MouseEventDispatcher::UpdateMouseCursor(const HitTestResult& hit_result, Uint32 window_id) {
    (void)window_id;  // 暂未使用

    SDL_SystemCursor target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;

    if (hit_result.IsValid() && hit_result.element) {
        std::string tag_name = hit_result.element->GetTagName();

        if (tag_name == "input") {
            auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(hit_result.element);
            if (input_element) {
                InputType input_type = input_element->GetInputType();

                // 对于 number 类型，检查是否在 spinner 区域
                if (input_type == InputType::Number && hit_result.render_object) {
                    const auto& style = hit_result.render_object->GetComputedStyle();
                    const auto& layout = hit_result.render_object->GetLayoutInfo();
                    float padding_left = style.padding.left.ToPx();
                    float padding_right = style.padding.right.ToPx();
                    float border_width = style.border.width.ToPx();

                    const float spinner_width = 16.0f;
                    float content_width = layout.width - padding_left - padding_right - border_width * 2;
                    float spinner_x = padding_left + border_width + content_width - spinner_width;

                    if (hit_result.local_x >= spinner_x) {
                        target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
                    } else {
                        target_cursor = SDL_SYSTEM_CURSOR_TEXT;
                    }
                }
                else if (input_type == InputType::Text ||
                         input_type == InputType::Password ||
                         input_type == InputType::Email ||
                         input_type == InputType::Tel ||
                         input_type == InputType::Url ||
                         input_type == InputType::Search) {
                    target_cursor = SDL_SYSTEM_CURSOR_TEXT;
                }
                else if (input_type == InputType::Button ||
                         input_type == InputType::Submit ||
                         input_type == InputType::Reset ||
                         input_type == InputType::Checkbox ||
                         input_type == InputType::Radio) {
                    target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
                }
            }
        }
        else if (tag_name == "textarea") {
            target_cursor = SDL_SYSTEM_CURSOR_TEXT;
        }
        else if (tag_name == "button" || tag_name == "select") {
            target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
        }
        else if (tag_name == "a") {
            target_cursor = SDL_SYSTEM_CURSOR_POINTER;
        }
    }

    // 通过回调设置光标
    if (cursor_callback_) {
        cursor_callback_(target_cursor);
    }
}

int MouseEventDispatcher::SDLButtonToMouseButton(Uint8 sdl_button) {
    switch (sdl_button) {
        case SDL_BUTTON_LEFT:   return 1;
        case SDL_BUTTON_MIDDLE: return 2;
        case SDL_BUTTON_RIGHT:  return 3;
        default:                return 0;
    }
}

} // namespace lightui
