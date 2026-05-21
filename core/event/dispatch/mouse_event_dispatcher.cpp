/**
 * @file mouse_event_dispatcher.cpp
 * @brief 鼠标事件分发器实现
 *
 * 从 event_loop.cpp 提取的鼠标事件处理逻辑。
 * 负责处理 hover 链管理、鼠标光标更新、表单元素交互等。
 */

#include "mouse_event_dispatcher.h"

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/dom/text.h"
#include "core/dom/selection/selection.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/elements/html_button_element.h"
#include "core/dom/elements/html_form_element.h"
#include "core/dom/elements/html_select_element.h"
#include "core/dom/elements/terminal/html_terminal_element.h"
#include "core/dom/elements/logview/html_logview_element.h"
#include "core/editing/drag_manager.h"
#include "core/editing/selection_manager.h"
#include "core/editing/contenteditable_controller.h"
#include "core/event/input/focus_manager.h"
#include "core/event/input/hit_test_controller.h"
#include "core/event/types/mouse_event.h"
#include "core/render/layer/paint_layer.h"
#include "core/render/objects/render_object.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/render/objects/select_dropdown.h"
#include "core/render/text/font_manager.h"
#include "core/render/input/text_edit_metrics.h"
#include "core/utils/utf8_utils.h"
#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>


#include "core/window/window.h"

#include <cmath>
#include "core/window/window_manager.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>

namespace mbink {

namespace {

bool IsPrimaryEditorElement(const std::shared_ptr<Element>& element) {
    if (!element) {
        return false;
    }
    const std::string tag_name = element->GetTagName();
    return tag_name == "input" || tag_name == "textarea" || tag_name == "terminal" || tag_name == "logview";
}

bool ContainsElement(const std::shared_ptr<Element>& ancestor, const std::shared_ptr<Element>& element) {
    if (!ancestor || !element) {
        return false;
    }

    std::shared_ptr<Node> current = element;
    while (current) {
        if (current.get() == ancestor.get()) {
            return true;
        }
        current = current->GetParentNode();
    }
    return false;
}

bool IsDisabledFormControlElement(const std::shared_ptr<Element>& element) {
    if (!element || !element->HasAttribute("disabled")) {
        return false;
    }

    const std::string tag_name = element->GetTagName();
    return tag_name == "button" ||
           tag_name == "input" ||
           tag_name == "select" ||
           tag_name == "textarea" ||
           tag_name == "option" ||
           tag_name == "optgroup" ||
           tag_name == "fieldset";
}

std::shared_ptr<Element> ResolveHoverTargetForDisabledFormControl(const std::shared_ptr<Element>& element) {
    if (!element) {
        return nullptr;
    }

    std::shared_ptr<Node> current = element;
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto current_element = std::static_pointer_cast<Element>(current);
            if (IsDisabledFormControlElement(current_element)) {
                current = current->GetParentNode();
                while (current) {
                    if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
                        auto hover_element = std::static_pointer_cast<Element>(current);
                        if (!IsDisabledFormControlElement(hover_element)) {
                            return hover_element;
                        }
                    }
                    current = current->GetParentNode();
                }
                return nullptr;
            }
        }
        current = current->GetParentNode();
    }

    return element;
}

} // namespace

MouseEventDispatcher::MouseEventDispatcher() = default;

MouseEventDispatcher::~MouseEventDispatcher() = default;

void MouseEventDispatcher::SetManagers(DragManager* drag_manager,
                                        SelectionManager* selection_manager,
                                        ContentEditableController* contenteditable_controller,
                                        FocusManager* focus_manager) {
    drag_manager_ = drag_manager;
    selection_manager_ = selection_manager;
    contenteditable_controller_ = contenteditable_controller;
    focus_manager_ = focus_manager;
}

void MouseEventDispatcher::SetCursorCallback(std::function<void(SDL_SystemCursor)> callback) {
    cursor_callback_ = std::move(callback);
}

void MouseEventDispatcher::CancelPendingFocusClear() {
    pending_focus_clear_.element.reset();
    pending_focus_clear_.target.reset();
    pending_focus_clear_.focus_serial = 0;
    pending_focus_clear_.active = false;
}

void MouseEventDispatcher::PrepareFocusClearOnMouseDown(const std::shared_ptr<Element>& hit_element, int button) {
    CancelPendingFocusClear();

    if (button != 1 || !focus_manager_ || !hit_element) {
        return;
    }

    auto current_focus = focus_manager_->GetFocusElement();
    if (!current_focus || ContainsElement(current_focus, hit_element)) {
        return;
    }

    pending_focus_clear_.element = current_focus;
    pending_focus_clear_.target = focus_manager_->FindFocusableElement(hit_element);
    pending_focus_clear_.focus_serial = focus_manager_->GetFocusChangeSerial();
    pending_focus_clear_.active = true;
}

void MouseEventDispatcher::ResolveFocusClearAfterClick(std::shared_ptr<Window> window) {
    if (!pending_focus_clear_.active || !focus_manager_) {
        return;
    }

    auto pending_element = pending_focus_clear_.element.lock();
    auto target_element = pending_focus_clear_.target.lock();
    const uint64_t pending_serial = pending_focus_clear_.focus_serial;
    CancelPendingFocusClear();

    if (!pending_element) {
        return;
    }

    focus_manager_->SetWindow(window.get());
    if (focus_manager_->GetFocusElement() != pending_element) {
        return;
    }

    if (focus_manager_->GetFocusChangeSerial() != pending_serial) {
        return;
    }

    if (target_element) {
        focus_manager_->SetFocus(target_element, false);
        return;
    }

    focus_manager_->Blur(pending_element);
}

bool MouseEventDispatcher::ShouldSuppressMouseEventsForDisabledFormControl(
    const std::shared_ptr<Element>& element) {
    std::shared_ptr<Node> current = element;
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto current_element = std::static_pointer_cast<Element>(current);
            if (IsDisabledFormControlElement(current_element)) {
                return true;
            }
        }
        current = current->GetParentNode();
    }
    return false;
}

void MouseEventDispatcher::HandleSuppressedDisabledMouseTarget(std::shared_ptr<Window> window,
                                                               const SDL_Event& event,
                                                               float logical_x,
                                                               float logical_y) {
    if (!window) {
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        int button = SDLButtonToMouseButton(event.button.button);
        if (button == 1) mouse_buttons_state_ |= 1;
        else if (button == 3) mouse_buttons_state_ |= 2;
        else if (button == 2) mouse_buttons_state_ |= 4;

        CancelPendingFocusClear();
        if (auto last_mousedown = last_mousedown_element_.lock()) {
            last_mousedown->SetPseudoClass("active", false);
        }
        last_mousedown_element_.reset();
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        int button = SDLButtonToMouseButton(event.button.button);
        if (button == 1) mouse_buttons_state_ &= ~1;
        else if (button == 3) mouse_buttons_state_ &= ~2;
        else if (button == 2) mouse_buttons_state_ &= ~4;

        auto last_mousedown = last_mousedown_element_.lock();
        if (last_mousedown) {
            HandleNoHitMouseUp(window, last_mousedown, event, logical_x, logical_y);
            last_mousedown->SetPseudoClass("active", false);
            last_mousedown_element_.reset();
        }

        ResolveFocusClearAfterClick(window);
        if (event.button.button == SDL_BUTTON_LEFT && drag_manager_) {
            drag_manager_->EndDrag(logical_x, logical_y);
        }
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        auto last_mousedown = last_mousedown_element_.lock();
        if (last_mousedown) {
            HandleNoHitMouseMotion(window, last_mousedown, event, logical_x, logical_y);
        }
    }
}

bool MouseEventDispatcher::HandleMouseEvent(const SDL_Event& event,
                                             std::shared_ptr<Window> window,
                                             std::shared_ptr<Document> document,
                                             std::shared_ptr<RenderObject> root_render) {
    if (!window || !document) {
        return false;
    }

    // 获取窗口 ID
    Uint32 window_id = 0;
    float mouse_x = 0, mouse_y = 0;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        window_id = event.button.windowID;
        mouse_x = event.button.x;
        mouse_y = event.button.y;
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        window_id = event.motion.windowID;
        mouse_x = event.motion.x;
        mouse_y = event.motion.y;
    } else {
        return false;
    }

    // 将物理像素坐标转换为逻辑像素坐标（CSS 像素）
    float dpi_scale = window->GetDisplayScale();
    float logical_x = mouse_x / dpi_scale;
    float logical_y = mouse_y / dpi_scale;

    // ===== 处理滚动条拖动 =====
    auto dragging_element = GetScrollbarDraggingElement();
    if (dragging_element && dragging_element->IsDraggingScrollbar()) {
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            float old_x = dragging_element->GetScrollX();
            float old_y = dragging_element->GetScrollY();
            float target_x = old_x;
            float target_y = old_y;

            if (dragging_element->UpdateScrollbarDrag(logical_x, logical_y, target_x, target_y) &&
                (target_x != old_x || target_y != old_y)) {
                auto render_pipeline = window->GetRenderPipeline();
                bool scrolled = false;
                if (render_pipeline) {
                    scrolled = render_pipeline->ScrollTo(dragging_element.get(), target_x, target_y);
                }
                if (!scrolled) {
                    dragging_element->ScrollTo(target_x, target_y);
                }
            }

            window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            return true;
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            dragging_element->EndScrollbarDrag();
            ClearScrollbarDragging();
            window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            return true;
        }
    }

    // ===== 处理 select 下拉菜单 =====
    auto& dropdown_manager = SelectDropdownManager::Instance();
    if (dropdown_manager.IsDropdownOpen()) {
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            if (dropdown_manager.HandleMouseMove(logical_x, logical_y)) {
                window->SetNeedsRepaintFor(RepaintReason::MouseHover);
            }
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            if (dropdown_manager.HitTest(logical_x, logical_y)) {
                dropdown_manager.HandleClick(logical_x, logical_y);
                window->SetNeedsRepaintFor(RepaintReason::MouseButton);
                return true;
            } else {
                dropdown_manager.CloseDropdown();
                window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            }
        }
    }

    // 对于鼠标移动事件，只在位置真正改变时才更新 hover
    static float last_mouse_x = -1, last_mouse_y = -1;
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        if (mouse_x == last_mouse_x && mouse_y == last_mouse_y) {
            return false;
        }
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
    }

    // ===== 执行 Hit Testing =====
    HitTestResult hit_result;

    if (root_render) {
        // 使用 HitTestController（基于 ViewportBounds 缓存）
        HitTestController hit_controller;
        HitTestRequest request;
        auto result_ex = hit_controller.HitTest(root_render, logical_x, logical_y, request);

        // 转换为 HitTestResult 格式
        if (result_ex.IsValid()) {
            hit_result.element = result_ex.element;
            hit_result.render_object = result_ex.render_object;
            hit_result.local_x = result_ex.local_x;
            hit_result.local_y = result_ex.local_y;
        }
    }

    // 更新 hover 链
    if (hit_result.IsValid() &&
        ShouldSuppressMouseEventsForDisabledFormControl(hit_result.element)) {
        HitTestResult hover_hit;
        hover_hit.element = ResolveHoverTargetForDisabledFormControl(hit_result.element);
        UpdateHoverChain(window, logical_x, logical_y, hover_hit);
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            UpdateMouseCursor(hover_hit, window_id);
        }
        HandleSuppressedDisabledMouseTarget(window, event, logical_x, logical_y);
        return true;
    }

    UpdateHoverChain(window, logical_x, logical_y, hit_result);

    // 更新鼠标光标样式
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        UpdateMouseCursor(hit_result, window_id);
    }

    // ===== 检测滚动条点击 =====
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT && root_render) {
        std::function<std::shared_ptr<RenderObject>(std::shared_ptr<RenderObject>, float, float)> findScrollableAtPoint;
        findScrollableAtPoint = [&](std::shared_ptr<RenderObject> obj, float abs_x, float abs_y) -> std::shared_ptr<RenderObject> {
            const auto& layout = obj->GetLayoutInfo();
            float local_x = abs_x - layout.x;
            float local_y = abs_y - layout.y;

            float effective_width = obj->GetEffectiveVisibleWidth();
            float effective_height = obj->GetEffectiveVisibleHeight();
            if (effective_width <= 0) effective_width = layout.width;
            if (effective_height <= 0) effective_height = layout.height;

            if (local_x >= 0 && local_x <= effective_width && local_y >= 0 && local_y <= effective_height) {
                const auto& style = obj->GetComputedStyle();
                std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
                std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
                bool allow_scroll = (overflow_x == "scroll" || overflow_x == "auto" ||
                                     overflow_y == "scroll" || overflow_y == "auto");
                if (allow_scroll) {
                    auto scrollbar_area = obj->HitTestScrollbar(local_x, local_y);
                    if (scrollbar_area != RenderObject::ScrollbarHitArea::None) {
                        return obj;
                    }
                }

                float child_offset_x = local_x + obj->GetScrollX();
                float child_offset_y = local_y + obj->GetScrollY();
                for (const auto& child : obj->GetChildren()) {
                    auto result = findScrollableAtPoint(child, child_offset_x, child_offset_y);
                    if (result) {
                        return result;
                    }
                }
            }
            return nullptr;
        };

        auto scrollable = findScrollableAtPoint(root_render, logical_x, logical_y);
        if (scrollable) {
            float elem_abs_x = 0, elem_abs_y = 0;
            std::vector<std::shared_ptr<RenderObject>> ancestors;
            auto current = scrollable;
            while (current) {
                ancestors.push_back(current);
                current = current->GetParent();
            }
            // 计算元素的绝对位置时，需要考虑父元素的滚动偏移
            // 滚动条是固定在容器视口内的，不随内容滚动
            // 所以计算 local 坐标时，需要减去父元素的滚动偏移
            for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
                const auto& l = (*it)->GetLayoutInfo();
                elem_abs_x += l.x;
                elem_abs_y += l.y;
                // 对于非目标元素的祖先，需要减去其滚动偏移
                // 因为子元素的视觉位置会随父元素滚动而移动
                if ((*it) != scrollable) {
                    elem_abs_x -= (*it)->GetScrollX();
                    elem_abs_y -= (*it)->GetScrollY();
                }
            }

            float local_x = logical_x - elem_abs_x;
            float local_y = logical_y - elem_abs_y;

            auto scrollbar_area = scrollable->HitTestScrollbar(local_x, local_y);
            if (scrollbar_area != RenderObject::ScrollbarHitArea::None) {
                scrollable->StartScrollbarDrag(scrollbar_area, logical_x, logical_y);
                SetScrollbarDraggingElement(scrollable, window_id);
                window->SetNeedsRepaintFor(RepaintReason::MouseButton);
                return true;
            }
        }
    }

    // 获取按钮编号
    int button = 0;
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        button = SDLButtonToMouseButton(event.button.button);
    }

    // 如果没有命中任何元素
    if (!hit_result.IsValid()) {
        auto last_mousedown = last_mousedown_element_.lock();

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (event.button.button == SDL_BUTTON_LEFT && focus_manager_) {
                auto current_focus = focus_manager_->GetFocusElement();
                if (current_focus) {
                    pending_focus_clear_.element = current_focus;
                    pending_focus_clear_.focus_serial = focus_manager_->GetFocusChangeSerial();
                    pending_focus_clear_.active = true;
                }
            } else {
                CancelPendingFocusClear();
            }
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && last_mousedown) {
            HandleNoHitMouseUp(window, last_mousedown, event, logical_x, logical_y);
            last_mousedown->SetPseudoClass("active", false);
            last_mousedown_element_.reset();
            ResolveFocusClearAfterClick(window);

            if (event.button.button == SDL_BUTTON_LEFT && drag_manager_) {
                drag_manager_->EndDrag(mouse_x, mouse_y);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION && last_mousedown) {
            HandleNoHitMouseMotion(window, last_mousedown, event, logical_x, logical_y);
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            ResolveFocusClearAfterClick(window);
        }

        return false;
    }

    // 处理 Range 滑块拖动
    auto last_mousedown = last_mousedown_element_.lock();
    if (event.type == SDL_EVENT_MOUSE_MOTION && last_mousedown) {
        std::string tag_name = last_mousedown->GetTagName();
        if (tag_name == "input") {
            auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown);
            if (input_element && input_element->IsDraggingRange()) {
                HandleRangeDrag(window, input_element, logical_x, root_render);
                return true;
            }
        }
    }

    // ===== 处理 mousedown 事件 =====
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        PrepareFocusClearOnMouseDown(hit_result.element, button);
        HandleMouseDown(window, document, hit_result, event, logical_x, logical_y, button, root_render);
    }
    // ===== 处理 mouseup 事件 =====
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        HandleMouseUp(window, document, hit_result, event, logical_x, logical_y, button, root_render);
    }
    // ===== 处理 mousemove 事件 =====
    else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        HandleMouseMove(window, document, hit_result, logical_x, logical_y, root_render);
    }

    return true;
}

void MouseEventDispatcher::UpdateHoverChain(std::shared_ptr<Window> window,
                                             float mouse_x,
                                             float mouse_y,
                                             const HitTestResult& hit_result) {
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
        window->SetNeedsRepaintFor(RepaintReason::MouseHover);
    }

    // 发送 mouseleave 到旧的 hover 元素
    if (old_hover && !ShouldSuppressMouseEventsForDisabledFormControl(old_hover)) {
        auto leave_event = std::make_shared<MouseEvent>(
            "mouseleave",
            static_cast<int>(mouse_x),
            static_cast<int>(mouse_y),
            0,
            1,
            0
        );
        old_hover->DispatchEvent(leave_event);
    }

    // 发送 mouseenter 到新的 hover 元素
    if (new_hover && !ShouldSuppressMouseEventsForDisabledFormControl(new_hover)) {
        auto enter_event = std::make_shared<MouseEvent>(
            "mouseenter",
            static_cast<int>(mouse_x),
            static_cast<int>(mouse_y),
            0,
            1,
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
            if (ShouldSuppressMouseEventsForDisabledFormControl(element)) {
                if (element->HasPseudoClass("hover")) {
                    element->SetPseudoClass("hover", false);
                }
                continue;
            }

            auto mouse_event = std::make_shared<MouseEvent>(
                event_type,
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0,
                1,
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
                if (tag == "button" || tag == "a") {
                    has_changes = true;
                }
            }
        }
    }

    return has_changes;
}

void MouseEventDispatcher::UpdateMouseCursor(const HitTestResult& hit_result, Uint32 window_id) {
    (void)window_id;

    SDL_SystemCursor target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;

    if (hit_result.IsValid() && hit_result.element) {
        std::string tag_name = hit_result.element->GetTagName();

        if (tag_name == "input") {
            auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(hit_result.element);
            if (input_element) {
                InputType input_type = input_element->GetInputType();

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

void MouseEventDispatcher::HandleInputMouseInteraction(
    std::shared_ptr<HTMLInputElement> input_element,
    float local_x,
    Uint32 event_type,
    float font_size,
    const std::string& font_family) {

    if (!input_element) {
        return;
    }

    std::string value = input_element->GetValue();
    InputType type = input_element->GetInputType();

    if (type != InputType::Text && type != InputType::Password &&
        type != InputType::Email && type != InputType::Tel &&
        type != InputType::Url && type != InputType::Search &&
        type != InputType::Number) {
        return;
    }

    FontDescriptor desc;
    desc.family = font_family.empty() ? "sans-serif" : font_family;
    desc.size = font_size > 0 ? font_size : 14.0f;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);

    const bool mask_as_password = type == InputType::Password;
    int char_pos = text_edit_metrics::HitTestTextPosition(value, local_x, font, mask_as_password);

    if (event_type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        input_element->SetCursorPosition(char_pos);
        input_element->SetDragStartPos(char_pos);
        input_element->HandleMouseDown(local_x, 0);
    } else if (event_type == SDL_EVENT_MOUSE_MOTION) {
        if (input_element->IsDraggingSelection()) {
            int drag_start = input_element->GetDragStartPos();
            input_element->SetSelection(drag_start, char_pos);
        }
    } else if (event_type == SDL_EVENT_MOUSE_BUTTON_UP) {
        input_element->HandleMouseUp();
    }
}

std::shared_ptr<RenderObject> MouseEventDispatcher::GetScrollbarDraggingElement() const {
    return scrollbar_dragging_element_.lock();
}

void MouseEventDispatcher::SetScrollbarDraggingElement(std::shared_ptr<RenderObject> element, Uint32 window_id) {
    scrollbar_dragging_element_ = element;
    scrollbar_dragging_window_id_ = window_id;
}

void MouseEventDispatcher::ClearScrollbarDragging() {
    scrollbar_dragging_element_.reset();
    scrollbar_dragging_window_id_ = 0;
}

void MouseEventDispatcher::HandleTextAreaMouseInteraction(
    std::shared_ptr<HTMLTextAreaElement> textarea_element,
    float local_x,
    float local_y,
    Uint32 event_type,
    float font_size,
    const std::string& font_family,
    bool shift_key,
    float visible_width,
    float visible_height) {

    if (!textarea_element) {
        return;
    }

    FontDescriptor desc;
    desc.family = font_family.empty() ? "sans-serif" : font_family;
    desc.size = font_size > 0 ? font_size : 14.0f;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);

    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);
    float line_height = -font_metrics.fAscent + font_metrics.fDescent;
    if (font_metrics.fLeading > 0) {
        line_height += font_metrics.fLeading;
    } else {
        line_height += font_size * 0.2f;
    }

    if (event_type == SDL_EVENT_MOUSE_MOTION && textarea_element->IsDraggingSelection() &&
        visible_width > 0 && visible_height > 0) {
        float scroll_speed = line_height;
        float scroll_top = textarea_element->GetScrollTop();
        float scroll_left = textarea_element->GetScrollLeft();
        float content_height = textarea_element->GetContentHeight(line_height);
        float max_scroll_y = std::max(0.0f, content_height - visible_height);
        float max_scroll_x = std::max(0.0f, textarea_element->GetMaxLineWidth(font) - visible_width);

        if (local_y < 0) {
            textarea_element->SetScrollTop(std::max(0.0f, scroll_top - scroll_speed));
        } else if (local_y > visible_height) {
            textarea_element->SetScrollTop(std::min(max_scroll_y, scroll_top + scroll_speed));
        }

        if (local_x < 0) {
            textarea_element->SetScrollLeft(std::max(0.0f, scroll_left - scroll_speed));
        } else if (local_x > visible_width) {
            textarea_element->SetScrollLeft(std::min(max_scroll_x, scroll_left + scroll_speed));
        }
    }

    float scroll_top = textarea_element->GetScrollTop();
    float scroll_left = textarea_element->GetScrollLeft();

    float clamped_local_x = std::max(0.0f, local_x);
    float clamped_local_y = std::max(0.0f, local_y);
    if (visible_width > 0) clamped_local_x = std::min(clamped_local_x, visible_width);
    if (visible_height > 0) clamped_local_y = std::min(clamped_local_y, visible_height);

    float actual_x = clamped_local_x + scroll_left;
    float actual_y = clamped_local_y + scroll_top;
    int clicked_line = std::max(0, static_cast<int>(actual_y / line_height));

    std::string value = textarea_element->GetValue();
    std::vector<std::string> lines;
    std::istringstream stream(value);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    if (value.empty() || (!value.empty() && value.back() == '\n')) {
        lines.push_back("");
    }
    if (lines.empty()) {
        lines.push_back("");
    }

    clicked_line = std::min(clicked_line, static_cast<int>(lines.size()) - 1);

    int char_offset = 0;
    for (int i = 0; i < clicked_line; ++i) {
        char_offset += static_cast<int>(utf8::CharCount(lines[i])) + 1;
    }

    const std::string& current_line = lines[clicked_line];
    int char_pos_in_line = text_edit_metrics::HitTestTextPosition(current_line, actual_x, font, false);
    int char_pos = char_offset + char_pos_in_line;

    if (event_type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (shift_key) {
            int current_start = textarea_element->GetSelectionStart();
            textarea_element->SetSelection(current_start, char_pos);
        } else {
            textarea_element->SetCursorPosition(char_pos);
        }
        textarea_element->SetDragStartPos(char_pos);
        textarea_element->HandleMouseDown(local_x, local_y);
    } else if (event_type == SDL_EVENT_MOUSE_MOTION) {
        if (textarea_element->IsDraggingSelection()) {
            int drag_start = textarea_element->GetDragStartPos();
            textarea_element->SetSelection(drag_start, char_pos);
        }
    } else if (event_type == SDL_EVENT_MOUSE_BUTTON_UP) {
        textarea_element->HandleMouseUp();
    }
}

void MouseEventDispatcher::ProcessFormElementDefaultAction(std::shared_ptr<Element> element, const HitTestResult& hit_result) {
    if (!element) {
        return;
    }

    std::string tag_name = element->GetTagName();

    // 处理 input 元素
    if (tag_name == "input") {
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (!input_element) {
            return;
        }

        if (input_element->IsDisabled()) {
            return;
        }

        InputType type = input_element->GetInputType();

        // Checkbox: 切换 checked 状态
        if (type == InputType::Checkbox) {
            bool checked = input_element->GetChecked();
            input_element->SetChecked(!checked, true);

            auto& window_manager = WindowManager::Instance();
            for (auto& window : window_manager.GetAllWindows()) {
                window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            }
        }
        // Radio: 选中
        else if (type == InputType::Radio) {
            if (!input_element->GetChecked()) {
                std::string name = input_element->GetAttribute("name");
                if (!name.empty()) {
                    auto document = element->GetOwnerDocument();
                    if (document) {
                        auto body = document->GetBody();
                        if (body) {
                            UncheckRadioGroup(body, name, input_element);
                        }
                    }
                }

                input_element->SetChecked(true, true);

                auto& window_manager = WindowManager::Instance();
                for (auto& window : window_manager.GetAllWindows()) {
                    window->SetNeedsRepaintFor(RepaintReason::MouseButton);
                }
            }
        }
        // Submit 按钮
        else if (type == InputType::Submit) {
            auto form = FindParentForm(element);
            if (form) {
                form->Submit();
            }
        }
    }
    // 处理 button 元素
    else if (tag_name == "button") {
        auto button_element = std::dynamic_pointer_cast<HTMLButtonElement>(element);
        if (button_element) {
            if (button_element->GetDisabled()) {
                return;
            }

            std::string button_type = button_element->GetAttribute("type");
            if (button_type.empty()) {
                button_type = "submit";
            }

            if (button_type == "submit") {
                auto form = FindParentForm(element);
                if (form) {
                    form->Submit();
                }
            } else if (button_type == "reset") {
                auto form = FindParentForm(element);
                if (form) {
                    form->Reset();
                }
            }
        }
    }
    // 处理 select 元素
    else if (tag_name == "select") {
        auto select_element = std::dynamic_pointer_cast<HTMLSelectElement>(element);
        if (select_element) {
            if (select_element->GetDisabled()) {
                return;
            }

            auto& dropdown_manager = SelectDropdownManager::Instance();

            if (select_element->IsDropdownOpen()) {
                dropdown_manager.CloseDropdown();
            } else {
                auto render_obj = hit_result.render_object;
                if (render_obj) {
                    float abs_x = 0, abs_y = 0;
                    auto current = render_obj;
                    while (current) {
                        const auto& layout = current->GetLayoutInfo();
                        abs_x += layout.x;
                        abs_y += layout.y;

                        auto parent = current->GetParent();
                        if (parent) {
                            abs_x -= parent->GetScrollX();
                            abs_y -= parent->GetScrollY();
                        }

                        current = parent;
                    }

                    const auto& layout = render_obj->GetLayoutInfo();
                    SkRect trigger_rect = SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);

                    select_element->SetDropdownOpen(true);
                    dropdown_manager.OpenDropdown(select_element, trigger_rect);
                }
            }

            auto& window_manager = WindowManager::Instance();
            for (auto& window : window_manager.GetAllWindows()) {
                window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            }
        }
    }
}

void MouseEventDispatcher::UncheckRadioGroup(const std::shared_ptr<Node>& node,
                                              const std::string& group_name,
                                              const std::shared_ptr<HTMLInputElement>& except) {
    if (!node) {
        return;
    }

    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element && element->GetTagName() == "input") {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (input && input != except &&
            input->GetInputType() == InputType::Radio &&
            input->GetAttribute("name") == group_name) {
            input->SetChecked(false, false);
        }
    }

    auto children = node->GetChildNodes();
    for (auto& child : children) {
        UncheckRadioGroup(child, group_name, except);
    }
}

std::shared_ptr<HTMLFormElement> MouseEventDispatcher::FindParentForm(std::shared_ptr<Element> element) {
    if (!element) {
        return nullptr;
    }

    auto parent = element->GetParentNode();
    while (parent) {
        auto parent_element = std::dynamic_pointer_cast<Element>(parent);
        if (parent_element && parent_element->GetTagName() == "form") {
            return std::dynamic_pointer_cast<HTMLFormElement>(parent_element);
        }
        parent = parent->GetParentNode();
    }
    return nullptr;
}

void MouseEventDispatcher::HandleNoHitMouseUp(std::shared_ptr<Window> window,
                                               std::shared_ptr<Element> last_mousedown,
                                               const SDL_Event& event,
                                               float logical_x,
                                               float logical_y) {
    // 分发 mouseup 事件到 last_mousedown 元素（即使鼠标不在元素上）
    int button = SDLButtonToMouseButton(event.button.button);
    auto mouseup_event = std::make_shared<MouseEvent>(
        "mouseup",
        static_cast<int>(logical_x),
        static_cast<int>(logical_y),
        button - 1,  // button: 0=左键, 1=中键, 2=右键 (W3C标准)
        1,
        0  // buttons: 按钮已释放
    );
    last_mousedown->DispatchEvent(mouseup_event);

    std::string tag_name = last_mousedown->GetTagName();
    if (tag_name == "textarea") {
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(last_mousedown);
        if (textarea_element) {
            if (textarea_element->IsDraggingScrollbar()) {
                textarea_element->EndScrollbarDrag();
            }
            if (textarea_element->IsDraggingSelection()) {
                textarea_element->HandleMouseUp();
            }
        }
    } else if (tag_name == "terminal") {
        auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(last_mousedown);
        if (terminal_element) {
            terminal_element->HandleMouseUp(logical_x, logical_y, 0);
            window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            if (auto pipeline = window->GetRenderPipeline()) {
                pipeline->ForceRasterize();
            }
        }
    } else if (tag_name == "logview") {
        auto logview_element = std::dynamic_pointer_cast<HTMLLogViewElement>(last_mousedown);
        if (logview_element) {
            logview_element->OnMouseUp(logical_x, logical_y, 0);
            window->SetNeedsRepaintFor(RepaintReason::MouseButton);
        }
    } else if (tag_name == "input") {
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown);
        if (input_element) {
            if (input_element->IsDraggingRange()) {
                input_element->EndRangeDrag();
                window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            }
            else if (input_element->IsDraggingSelection()) {
                HandleInputMouseInteraction(input_element, 0, event.type, 14.0f, "");
            }
        }
    } else if (last_mousedown->IsContentEditable()) {
        if (contenteditable_controller_ && contenteditable_controller_->IsDragging()) {
            contenteditable_controller_->HandleMouseUp(last_mousedown, logical_x, logical_y);
        }
        window->SetNeedsRepaintFor(RepaintReason::MouseButton);
    }
}

void MouseEventDispatcher::HandleNoHitMouseMotion(std::shared_ptr<Window> window,
                                                   std::shared_ptr<Element> last_mousedown,
                                                   const SDL_Event& event,
                                                   float logical_x,
                                                   float logical_y) {
    std::string tag_name = last_mousedown->GetTagName();
    if (tag_name == "textarea") {
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(last_mousedown);
        if (textarea_element && (textarea_element->IsDraggingSelection() || textarea_element->IsDraggingScrollbar())) {
            auto root_render = window->GetCachedRenderTree();
            if (root_render) {
                struct FindResult {
                    std::shared_ptr<RenderObject> render_obj;
                    float abs_x = 0;
                    float abs_y = 0;
                };
                std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
                findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
                    if (!obj) return {};
                    const auto& layout = obj->GetLayoutInfo();
                    float current_x = offset_x + layout.x;
                    float current_y = offset_y + layout.y;

                    auto node = obj->GetNode();
                    if (node && std::dynamic_pointer_cast<HTMLTextAreaElement>(node) == textarea_element) {
                        return {obj, current_x, current_y};
                    }
                    float child_offset_x = current_x - obj->GetScrollX();
                    float child_offset_y = current_y - obj->GetScrollY();
                    for (auto& child : obj->GetChildren()) {
                        auto result = findRenderObj(child, child_offset_x, child_offset_y);
                        if (result.render_obj) return result;
                    }
                    return {};
                };

                auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
                if (find_result.render_obj) {
                    const auto& layout = find_result.render_obj->GetLayoutInfo();
                    const auto& style = find_result.render_obj->GetComputedStyle();
                    float padding_left = style.padding.left.ToPx();
                    float padding_top = style.padding.top.ToPx();
                    float padding_right = style.padding.right.ToPx();
                    float padding_bottom = style.padding.bottom.ToPx();

                    const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
                    SkFont font;
                    font.setSize(style.font_size);
                    float line_height = style.font_size * 1.2f;
                    float content_height = textarea_element->GetContentHeight(line_height);
                    float max_line_width = textarea_element->GetMaxLineWidth(font);
                    float base_visible_width = layout.width - padding_left - padding_right;
                    float base_visible_height = layout.height - padding_top - padding_bottom;
                    bool need_v_scrollbar = content_height > base_visible_height;
                    bool need_h_scrollbar = max_line_width > base_visible_width;
                    float visible_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
                    float visible_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);

                    float text_local_x = logical_x - find_result.abs_x - padding_left;
                    float text_local_y = logical_y - find_result.abs_y - padding_top;
                    HandleTextAreaMouseInteraction(textarea_element, text_local_x, text_local_y, event.type,
                                                   style.font_size, style.font_family, false,
                                                   visible_width, visible_height);
                }
            }
        }
    } else if (tag_name == "input") {
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown);
        if (input_element) {
            if (input_element->IsDraggingRange()) {
                HandleRangeDrag(window, input_element, logical_x, window->GetCachedRenderTree());
            }
            else if (input_element->IsDraggingSelection()) {
                auto root_render = window->GetCachedRenderTree();
                if (root_render) {
                    struct FindResult {
                        std::shared_ptr<RenderObject> render_obj;
                        float abs_x = 0;
                        float abs_y = 0;
                    };
                    std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
                    findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
                        if (!obj) return {};
                        const auto& layout = obj->GetLayoutInfo();
                        float current_x = offset_x + layout.x;
                        float current_y = offset_y + layout.y;

                        auto node = obj->GetNode();
                        if (node && std::dynamic_pointer_cast<HTMLInputElement>(node) == input_element) {
                            return {obj, current_x, current_y};
                        }
                        float child_offset_x = current_x - obj->GetScrollX();
                        float child_offset_y = current_y - obj->GetScrollY();
                        for (auto& child : obj->GetChildren()) {
                            auto result = findRenderObj(child, child_offset_x, child_offset_y);
                            if (result.render_obj) return result;
                        }
                        return {};
                    };

                    auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
                    if (find_result.render_obj) {
                        const auto& style = find_result.render_obj->GetComputedStyle();
                        float padding_left = style.padding.left.ToPx();
                        float text_local_x = logical_x - find_result.abs_x - padding_left;
                        HandleInputMouseInteraction(input_element, text_local_x, event.type,
                                                   style.font_size, style.font_family);
                    }
                }
            }
        }
    } else if (last_mousedown->IsContentEditable()) {
        HandleContentEditableDragSelection(window,
                                           std::dynamic_pointer_cast<Document>(last_mousedown->GetOwnerDocument()),
                                           logical_x,
                                           logical_y,
                                           window->GetCachedRenderTree(),
                                           event.type);
    }
    // 处理 terminal 的滚动条拖动
    else if (tag_name == "terminal") {
        auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(last_mousedown);
        if (terminal_element) {
            // 需要找到 terminal 元素的渲染对象来计算本地坐标
            auto root_render = window->GetCachedRenderTree();
            if (root_render) {
                struct FindResult {
                    std::shared_ptr<RenderObject> render_obj;
                    float abs_x = 0;
                    float abs_y = 0;
                };
                std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
                findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
                    if (!obj) return {};
                    const auto& layout = obj->GetLayoutInfo();
                    float current_x = offset_x + layout.x;
                    float current_y = offset_y + layout.y;

                    auto node = obj->GetNode();
                    if (node && std::dynamic_pointer_cast<HTMLTerminalElement>(node) == terminal_element) {
                        return {obj, current_x, current_y};
                    }
                    float child_offset_x = current_x - obj->GetScrollX();
                    float child_offset_y = current_y - obj->GetScrollY();
                    for (auto& child : obj->GetChildren()) {
                        auto result = findRenderObj(child, child_offset_x, child_offset_y);
                        if (result.render_obj) return result;
                    }
                    return {};
                };

                auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
                if (find_result.render_obj) {
                    float local_x = logical_x - find_result.abs_x;
                    float local_y = logical_y - find_result.abs_y;
                    terminal_element->HandleMouseMove(local_x, local_y);
                    window->SetNeedsRepaintFor(RepaintReason::MouseHover);
                    if (auto pipeline = window->GetRenderPipeline()) {
                        pipeline->ForceRasterize();
                    }
                }
            }
        }
    }
    // 处理 logview 的拖动选择
    else if (tag_name == "logview") {
        auto logview_element = std::dynamic_pointer_cast<HTMLLogViewElement>(last_mousedown);
        if (logview_element) {
            auto root_render = window->GetCachedRenderTree();
            if (root_render) {
                struct FindResult {
                    std::shared_ptr<RenderObject> render_obj;
                    float abs_x = 0;
                    float abs_y = 0;
                };
                std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
                findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
                    if (!obj) return {};
                    const auto& layout = obj->GetLayoutInfo();
                    float current_x = offset_x + layout.x;
                    float current_y = offset_y + layout.y;

                    auto node = obj->GetNode();
                    if (node && std::dynamic_pointer_cast<HTMLLogViewElement>(node) == logview_element) {
                        return {obj, current_x, current_y};
                    }
                    float child_offset_x = current_x - obj->GetScrollX();
                    float child_offset_y = current_y - obj->GetScrollY();
                    for (auto& child : obj->GetChildren()) {
                        auto result = findRenderObj(child, child_offset_x, child_offset_y);
                        if (result.render_obj) return result;
                    }
                    return {};
                };

                auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
                if (find_result.render_obj) {
                    float local_x = logical_x - find_result.abs_x;
                    float local_y = logical_y - find_result.abs_y;
                    logview_element->OnMouseMove(local_x, local_y);
                    window->SetNeedsRepaintFor(RepaintReason::MouseHover);
                }
            }
        }
    }
    // 处理 contentEditable 的拖动选择
    else if (last_mousedown->IsContentEditable() &&
             contenteditable_controller_ &&
             contenteditable_controller_->IsDragging()) {
        HandleContentEditableDragSelection(window,
                                           std::dynamic_pointer_cast<Document>(last_mousedown->GetOwnerDocument()),
                                           logical_x,
                                           logical_y,
                                           window->GetCachedRenderTree(),
                                           event.type);
    }
}

void MouseEventDispatcher::HandleRangeDrag(std::shared_ptr<Window> window,
                                            std::shared_ptr<HTMLInputElement> input_element,
                                            float logical_x,
                                            std::shared_ptr<RenderObject> root_render) {
    if (!root_render) return;

    struct FindResult {
        std::shared_ptr<RenderObject> render_obj;
        float abs_x = 0;
        float abs_y = 0;
    };
    std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
    findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
        if (!obj) return {};
        const auto& layout = obj->GetLayoutInfo();
        float current_x = offset_x + layout.x;
        float current_y = offset_y + layout.y;

        auto node = obj->GetNode();
        if (node && std::dynamic_pointer_cast<HTMLInputElement>(node) == input_element) {
            return {obj, current_x, current_y};
        }
        float child_offset_x = current_x - obj->GetScrollX();
        float child_offset_y = current_y - obj->GetScrollY();
        for (auto& child : obj->GetChildren()) {
            auto result = findRenderObj(child, child_offset_x, child_offset_y);
            if (result.render_obj) return result;
        }
        return {};
    };

    auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
    if (find_result.render_obj) {
        const auto& layout = find_result.render_obj->GetLayoutInfo();
        float local_x = logical_x - find_result.abs_x;
        input_element->UpdateRangeDrag(local_x, layout.width);
        window->SetNeedsRepaintFor(RepaintReason::MouseButton);
    }
}

void MouseEventDispatcher::HandleMouseDown(std::shared_ptr<Window> window,
                                            std::shared_ptr<Document> document,
                                            const HitTestResult& hit_result,
                                            const SDL_Event& event,
                                            float logical_x,
                                            float logical_y,
                                            int button,
                                            std::shared_ptr<RenderObject> root_render) {
    (void)root_render;

    // 更新鼠标按钮状态（用于 mousemove 事件的 buttons 属性）
    if (button == 1) mouse_buttons_state_ |= 1;       // 左键
    else if (button == 3) mouse_buttons_state_ |= 2;  // 右键
    else if (button == 2) mouse_buttons_state_ |= 4;  // 中键

    // 浏览器行为：mousedown 时自动更新 Selection 到点击位置
    // 这对于 CodeMirror 等库正确处理点击定位至关重要
    if (button == 1 && document) {  // 左键点击
        if (hit_result.element->IsContentEditable() && contenteditable_controller_) {
            SDL_Keymod mod_state = SDL_GetModState();
            bool shift_key = (mod_state & SDL_KMOD_SHIFT) != 0;
            contenteditable_controller_->HandleMouseDown(
                hit_result.element,
                logical_x,
                logical_y,
                shift_key,
                hit_result.render_object,
                root_render);
        } else if (selection_manager_ && !IsPrimaryEditorElement(hit_result.element)) {
            UpdateSelectionFromClick(document, hit_result, logical_x, logical_y);
        }
    }

    // 创建并分发 mousedown 事件
    // buttons: 1=左键, 2=右键, 4=中键
    int buttons = 0;
    if (button == 1) buttons = 1;       // 左键
    else if (button == 3) buttons = 2;  // 右键
    else if (button == 2) buttons = 4;  // 中键

    auto mousedown_event = std::make_shared<MouseEvent>(
        "mousedown",
        static_cast<int>(logical_x),
        static_cast<int>(logical_y),
        button - 1,  // button: 0=左键, 1=中键, 2=右键 (W3C标准)
        1,
        buttons
    );
    hit_result.element->DispatchEvent(mousedown_event);

    // 设置 :active 伪类
    hit_result.element->SetPseudoClass("active", true);
    last_mousedown_element_ = hit_result.element;

    // 处理表单元素的默认行为
    ProcessFormElementDefaultAction(hit_result.element, hit_result);

    // 处理输入框的鼠标交互
    std::string tag_name = hit_result.element->GetTagName();
    if (tag_name == "input") {
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(hit_result.element);
        if (input_element) {
            InputType input_type = input_element->GetInputType();

            // 处理 Range 滑块
            if (input_type == InputType::Range && hit_result.render_object) {
                const auto& layout = hit_result.render_object->GetLayoutInfo();
                input_element->StartRangeDrag(layout.width);
                // 立即更新位置
                input_element->UpdateRangeDrag(hit_result.local_x, layout.width);
                window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            }
            // 处理文本输入框
            else if (input_type == InputType::Text || input_type == InputType::Password ||
                     input_type == InputType::Email || input_type == InputType::Tel ||
                     input_type == InputType::Url || input_type == InputType::Search ||
                     input_type == InputType::Number) {
                if (hit_result.render_object) {
                    const auto& style = hit_result.render_object->GetComputedStyle();
                    float padding_left = style.padding.left.ToPx();
                    float text_local_x = hit_result.local_x - padding_left;
                    HandleInputMouseInteraction(input_element, text_local_x, event.type,
                                               style.font_size, style.font_family);
                }
            }

            // 设置焦点
            if (focus_manager_) {
                focus_manager_->SetWindow(window.get());
                focus_manager_->SetFocus(hit_result.element, false);
            }
        }
    }
    else if (tag_name == "textarea") {
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(hit_result.element);
        if (textarea_element && hit_result.render_object) {
            const auto& style = hit_result.render_object->GetComputedStyle();
            const auto& layout = hit_result.render_object->GetLayoutInfo();
            float padding_left = style.padding.left.ToPx();
            float padding_top = style.padding.top.ToPx();
            float padding_right = style.padding.right.ToPx();
            float padding_bottom = style.padding.bottom.ToPx();

            // 检查是否点击了滚动条
            const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
            SkFont font;
            font.setSize(style.font_size);
            float line_height = style.font_size * 1.2f;
            float content_height = textarea_element->GetContentHeight(line_height);
            float max_line_width = textarea_element->GetMaxLineWidth(font);
            float base_visible_width = layout.width - padding_left - padding_right;
            float base_visible_height = layout.height - padding_top - padding_bottom;
            bool need_v_scrollbar = content_height > base_visible_height;
            bool need_h_scrollbar = max_line_width > base_visible_width;
            float visible_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
            float visible_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);

            float text_local_x = hit_result.local_x - padding_left;
            float text_local_y = hit_result.local_y - padding_top;

            // 检查是否点击了滚动条
            if (need_v_scrollbar && text_local_x > visible_width) {
                textarea_element->StartScrollbarDrag(HTMLTextAreaElement::ScrollbarType::VERTICAL, text_local_y);
            } else if (need_h_scrollbar && text_local_y > visible_height) {
                textarea_element->StartScrollbarDrag(HTMLTextAreaElement::ScrollbarType::HORIZONTAL, text_local_x);
            } else {
                SDL_Keymod mod_state = SDL_GetModState();
                bool shift_key = (mod_state & SDL_KMOD_SHIFT) != 0;
                HandleTextAreaMouseInteraction(textarea_element, text_local_x, text_local_y, event.type,
                                               style.font_size, style.font_family, shift_key,
                                               visible_width, visible_height);
            }

            // 设置焦点
            if (focus_manager_) {
                focus_manager_->SetWindow(window.get());
                focus_manager_->SetFocus(hit_result.element, false);
            }
        }
    }
    // 处理 terminal 元素
    else if (tag_name == "terminal") {
        auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(hit_result.element);
        if (terminal_element) {
            // 计算点击次数
            Uint64 now = SDL_GetTicks();
            float dx = logical_x - last_click_x_;
            float dy = logical_y - last_click_y_;
            float distance = std::sqrt(dx * dx + dy * dy);

            if ((now - last_click_time_) < DOUBLE_CLICK_TIME_MS &&
                distance < CLICK_DISTANCE_THRESHOLD) {
                click_count_++;
                if (click_count_ > 3) click_count_ = 3;  // 最多三击
            } else {
                click_count_ = 1;
            }

            last_click_time_ = now;
            last_click_x_ = logical_x;
            last_click_y_ = logical_y;

            terminal_element->HandleMouseDown(hit_result.local_x, hit_result.local_y, 0, click_count_);

            // 设置焦点
            if (focus_manager_) {
                focus_manager_->SetWindow(window.get());
                focus_manager_->SetFocus(hit_result.element, false);
            }

            // 标记需要重绘
            window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            if (auto pipeline = window->GetRenderPipeline()) {
                pipeline->ForceRasterize();
            }
        }
    }
    // 处理 logview 元素
    else if (tag_name == "logview") {
        auto logview_element = std::dynamic_pointer_cast<HTMLLogViewElement>(hit_result.element);
        if (logview_element) {
            // 计算点击次数
            Uint64 now = SDL_GetTicks();
            float dx = logical_x - last_click_x_;
            float dy = logical_y - last_click_y_;
            float distance = std::sqrt(dx * dx + dy * dy);

            if ((now - last_click_time_) < DOUBLE_CLICK_TIME_MS &&
                distance < CLICK_DISTANCE_THRESHOLD) {
                click_count_++;
                if (click_count_ > 3) click_count_ = 3;
            } else {
                click_count_ = 1;
            }

            last_click_time_ = now;
            last_click_x_ = logical_x;
            last_click_y_ = logical_y;

            logview_element->OnMouseDown(hit_result.local_x, hit_result.local_y, 0, click_count_);

            // 设置焦点
            if (focus_manager_) {
                focus_manager_->SetWindow(window.get());
                focus_manager_->SetFocus(hit_result.element, false);
            }

            window->SetNeedsRepaintFor(RepaintReason::MouseButton);
            if (auto pipeline = window->GetRenderPipeline()) {
                pipeline->ForceRasterize();
            }
        }
    }
    // 处理 contentEditable 元素
    else if (hit_result.element->IsContentEditable()) {
        if (focus_manager_) {
            focus_manager_->SetWindow(window.get());
            focus_manager_->SetFocus(hit_result.element, false);
        }
    }
    // 参考 Blink/Chrome 的行为：
    // 点击非可聚焦元素时，不应该清除焦点
    // 这允许 JavaScript 在 click 事件中调用 element.focus() 来转移焦点
    // 焦点只在以下情况改变：
    // 1. 点击了另一个可聚焦元素（焦点转移到新元素）
    // 2. 调用了 element.focus() 或 element.blur()
    // 3. 按 Tab 键导航
    //
    // 注意：之前这里有 ClearFocus() 调用，这是错误的行为
    // 它会导致 Fluent Input 等组件无法正常工作（外层 div 点击后调用 input.focus()）

    // 拖拽检测
    if (event.button.button == SDL_BUTTON_LEFT && drag_manager_) {
        drag_manager_->StartDragDetection(hit_result.element, logical_x, logical_y);
    }
}

void MouseEventDispatcher::HandleMouseUp(std::shared_ptr<Window> window,
                                          std::shared_ptr<Document> /*document*/,
                                          const HitTestResult& hit_result,
                                          const SDL_Event& event,
                                          float logical_x,
                                          float logical_y,
                                          int button,
                                          std::shared_ptr<RenderObject> root_render) {
    (void)root_render;

    // 清除鼠标按钮状态
    if (button == 1) mouse_buttons_state_ &= ~1;       // 左键
    else if (button == 3) mouse_buttons_state_ &= ~2;  // 右键
    else if (button == 2) mouse_buttons_state_ &= ~4;  // 中键

    auto last_mousedown = last_mousedown_element_.lock();

    // 移除 :active 伪类
    if (last_mousedown) {
        last_mousedown->SetPseudoClass("active", false);
    }

    // 处理输入框的鼠标释放
    if (last_mousedown && event.button.button == SDL_BUTTON_LEFT) {
        std::string tag_name = last_mousedown->GetTagName();
        if (tag_name == "input") {
            auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown);
            if (input_element) {
                if (input_element->IsDraggingRange()) {
                    input_element->EndRangeDrag();
                    window->SetNeedsRepaintFor(RepaintReason::MouseButton);
                }
                else if (input_element->IsDraggingSelection()) {
                    HandleInputMouseInteraction(input_element, 0, event.type, 14.0f, "");
                }
            }
        } else if (tag_name == "textarea") {
            auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(last_mousedown);
            if (textarea_element) {
                if (textarea_element->IsDraggingScrollbar()) {
                    textarea_element->EndScrollbarDrag();
                }
                if (textarea_element->IsDraggingSelection()) {
                    textarea_element->HandleMouseUp();
                }
            }
        } else if (tag_name == "terminal") {
            auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(last_mousedown);
            if (terminal_element) {
                terminal_element->HandleMouseUp(logical_x, logical_y, 0);
                window->SetNeedsRepaintFor(RepaintReason::MouseButton);
                if (auto pipeline = window->GetRenderPipeline()) {
                    pipeline->ForceRasterize();
                }
            }
        } else if (tag_name == "logview") {
            auto logview_element = std::dynamic_pointer_cast<HTMLLogViewElement>(last_mousedown);
            if (logview_element) {
                logview_element->OnMouseUp(logical_x, logical_y, 0);
                window->SetNeedsRepaintFor(RepaintReason::MouseButton);
                if (auto pipeline = window->GetRenderPipeline()) {
                    pipeline->ForceRasterize();
                }
            }
        }

        // 结束 contentEditable 拖动选择
        if (last_mousedown->IsContentEditable() && contenteditable_controller_ && contenteditable_controller_->IsDragging()) {
            if (contenteditable_controller_) {
                contenteditable_controller_->HandleMouseUp(last_mousedown, logical_x, logical_y);
            }
        }
    }

    // 分发 mouseup 事件到当前命中的元素
    // mouseup 时按钮已释放，buttons 为 0
    auto mouseup_event = std::make_shared<MouseEvent>(
        "mouseup",
        static_cast<int>(logical_x),
        static_cast<int>(logical_y),
        button - 1,  // button: 0=左键, 1=中键, 2=右键 (W3C标准)
        1,
        0  // buttons: 按钮已释放
    );
    hit_result.element->DispatchEvent(mouseup_event);

    // 检查是否在同一元素上 mousedown 和 mouseup
    if (last_mousedown == hit_result.element) {
        // 触发 click 事件
        auto click_event = std::make_shared<MouseEvent>(
            "click",
            static_cast<int>(logical_x),
            static_cast<int>(logical_y),
            button - 1,  // button: 0=左键, 1=中键, 2=右键 (W3C标准)
            1,
            0  // buttons: 按钮已释放
        );
        hit_result.element->DispatchEvent(click_event);

        // 参考 Blink/Chrome 的行为：
        // click 事件分发后，不应该再尝试设置焦点或清除焦点
        // 因为 JavaScript 的 click 处理器可能已经调用了 element.focus()
        // 如果我们在这里清除焦点，会覆盖 JavaScript 设置的焦点状态
        //
        // 焦点管理应该完全由以下方式控制：
        // 1. mousedown 时点击可聚焦元素 → 焦点转移
        // 2. JavaScript 调用 element.focus() / element.blur()
        // 3. Tab 键导航

        // 检查双击
        Uint64 now = SDL_GetTicks();
        auto last_click = last_click_element_.lock();
        if (last_click == hit_result.element && (now - last_click_time_) < DOUBLE_CLICK_TIME_MS) {
            auto dblclick_event = std::make_shared<MouseEvent>(
                "dblclick",
                static_cast<int>(logical_x),
                static_cast<int>(logical_y),
                button - 1,  // button: 0=左键, 1=中键, 2=右键 (W3C标准)
                2,
                0  // buttons: 按钮已释放
            );
            hit_result.element->DispatchEvent(dblclick_event);
        }

        last_click_element_ = hit_result.element;
        last_click_time_ = now;
    }

    ResolveFocusClearAfterClick(window);
    last_mousedown_element_.reset();

    // 结束拖拽
    if (event.button.button == SDL_BUTTON_LEFT && drag_manager_) {
        drag_manager_->EndDrag(logical_x, logical_y);
    }
}

void MouseEventDispatcher::HandleMouseMove(std::shared_ptr<Window> window,
                                            std::shared_ptr<Document> document,
                                            const HitTestResult& hit_result,
                                            float logical_x,
                                            float logical_y,
                                            std::shared_ptr<RenderObject> root_render) {
    // 使用跟踪的鼠标按钮状态（而不是 SDL_GetMouseState，因为它在某些情况下返回 0）
    int buttons = mouse_buttons_state_;

    // 创建并分发 mousemove 事件
    auto mousemove_event = std::make_shared<MouseEvent>(
        "mousemove",
        static_cast<int>(logical_x),
        static_cast<int>(logical_y),
        0,
        0,
        buttons
    );
    hit_result.element->DispatchEvent(mousemove_event);

    auto last_mousedown = last_mousedown_element_.lock();

    // 处理输入框的拖动选择
    if (last_mousedown) {
        std::string tag_name = last_mousedown->GetTagName();
        if (tag_name == "input") {
            auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown);
            if (input_element && input_element->IsDraggingSelection()) {
                if (hit_result.render_object) {
                    const auto& style = hit_result.render_object->GetComputedStyle();
                    float padding_left = style.padding.left.ToPx();
                    float text_local_x = hit_result.local_x - padding_left;
                    HandleInputMouseInteraction(input_element, text_local_x, SDL_EVENT_MOUSE_MOTION,
                                               style.font_size, style.font_family);
                }
            }
        } else if (tag_name == "textarea") {
            auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(last_mousedown);
            if (textarea_element && (textarea_element->IsDraggingSelection() || textarea_element->IsDraggingScrollbar())) {
                if (hit_result.render_object) {
                    const auto& style = hit_result.render_object->GetComputedStyle();
                    const auto& layout = hit_result.render_object->GetLayoutInfo();
                    float padding_left = style.padding.left.ToPx();
                    float padding_top = style.padding.top.ToPx();
                    float padding_right = style.padding.right.ToPx();
                    float padding_bottom = style.padding.bottom.ToPx();

                    const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
                    SkFont font;
                    font.setSize(style.font_size);
                    float line_height = style.font_size * 1.2f;
                    float content_height = textarea_element->GetContentHeight(line_height);
                    float max_line_width = textarea_element->GetMaxLineWidth(font);
                    float base_visible_width = layout.width - padding_left - padding_right;
                    float base_visible_height = layout.height - padding_top - padding_bottom;
                    bool need_v_scrollbar = content_height > base_visible_height;
                    bool need_h_scrollbar = max_line_width > base_visible_width;
                    float visible_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
                    float visible_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);

                    float text_local_x = hit_result.local_x - padding_left;
                    float text_local_y = hit_result.local_y - padding_top;
                    HandleTextAreaMouseInteraction(textarea_element, text_local_x, text_local_y, SDL_EVENT_MOUSE_MOTION,
                                                   style.font_size, style.font_family, false,
                                                   visible_width, visible_height);
                }
            }
        } else if (tag_name == "terminal") {
            auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(last_mousedown);
            if (terminal_element) {
                terminal_element->HandleMouseMove(hit_result.local_x, hit_result.local_y);
                window->SetNeedsRepaintFor(RepaintReason::MouseHover);
                if (auto pipeline = window->GetRenderPipeline()) {
                    pipeline->ForceRasterize();
                }
            }
        } else if (tag_name == "logview") {
            auto logview_element = std::dynamic_pointer_cast<HTMLLogViewElement>(last_mousedown);
            if (logview_element) {
                logview_element->OnMouseMove(hit_result.local_x, hit_result.local_y);
                window->SetNeedsRepaintFor(RepaintReason::MouseHover);
                if (auto pipeline = window->GetRenderPipeline()) {
                    pipeline->ForceRasterize();
                }
            }
        }
        // 处理 contentEditable 拖动选择
        else if (last_mousedown->IsContentEditable() && contenteditable_controller_ && contenteditable_controller_->IsDragging()) {
            HandleContentEditableDragSelection(window, document, logical_x, logical_y, root_render, SDL_EVENT_MOUSE_MOTION);
        }
    }

    // 更新拖拽状态
    if (drag_manager_ && (drag_manager_->IsDragging() || drag_manager_->IsDetecting())) {
        drag_manager_->UpdateDrag(logical_x, logical_y, document, root_render);
    }
}

void MouseEventDispatcher::HandleContentEditableDragSelection(std::shared_ptr<Window> window,
                                                               std::shared_ptr<Document> document,
                                                               float logical_x,
                                                               float logical_y,
                                                               std::shared_ptr<RenderObject> root_render,
                                                               Uint32 event_type) {
    (void)event_type;

    auto last_mousedown = last_mousedown_element_.lock();
    if (!window || !document || !last_mousedown || !last_mousedown->IsContentEditable()) {
        return;
    }

    if (!contenteditable_controller_) {
        return;
    }

    if (contenteditable_controller_->HandleMouseMove(document, logical_x, logical_y, root_render, window.get())) {
        window->SetNeedsRepaintFor(RepaintReason::MouseHover);
    }
}

void MouseEventDispatcher::UpdateSelectionFromClick(
    std::shared_ptr<Document> document,
    const HitTestResult& hit_result,
    float logical_x,
    float logical_y) {

    (void)logical_y;

    if (!document || !hit_result.IsValid() || !selection_manager_) {
        return;
    }

    auto selection = selection_manager_->GetSelection(document);
    if (!selection) {
        return;
    }

    // 查找点击位置的文本节点
    std::shared_ptr<Text> text_node = nullptr;
    std::shared_ptr<RenderObject> text_render = nullptr;

    if (hit_result.render_object) {
        auto node = hit_result.render_object->GetNode();
        if (node && node->GetNodeType() == NodeType::TEXT_NODE) {
            text_node = std::dynamic_pointer_cast<Text>(node);
            text_render = hit_result.render_object;
        } else {
            // 遍历子 RenderObject 查找文本节点
            for (const auto& child : hit_result.render_object->GetChildren()) {
                auto child_node = child->GetNode();
                if (child_node && child_node->GetNodeType() == NodeType::TEXT_NODE) {
                    text_node = std::dynamic_pointer_cast<Text>(child_node);
                    text_render = child;
                    break;
                }
            }
        }
    }

    // 如果没有找到文本节点，尝试从元素的子节点中查找
    if (!text_node) {
        for (const auto& child : hit_result.element->GetChildNodes()) {
            if (child->GetNodeType() == NodeType::TEXT_NODE) {
                text_node = std::dynamic_pointer_cast<Text>(child);
                break;
            }
        }
    }

    if (!text_node) {
        // 没有文本节点，将 Selection 折叠到元素开头
        selection->Collapse(hit_result.element, 0);
        return;
    }

    // 计算字符偏移量
    std::string text = text_node->GetTextContent();
    if (text.empty()) {
        selection->Collapse(text_node, 0);
        return;
    }

    // 获取文本渲染的样式信息
    float font_size = 16.0f;
    std::string font_family = "sans-serif";

    if (text_render) {
        const auto& style = text_render->GetComputedStyle();
        font_size = style.font_size;
        font_family = style.font_family.empty() ? "sans-serif" : style.font_family;
    } else if (hit_result.render_object) {
        const auto& style = hit_result.render_object->GetComputedStyle();
        font_size = style.font_size;
        font_family = style.font_family.empty() ? "sans-serif" : style.font_family;
    }

    // 创建字体
    FontDescriptor desc;
    desc.family = font_family;
    desc.size = font_size;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);

    float local_x = logical_x;
    if (text_render) {
        local_x -= text_render->GetLayoutInfo().x;
    } else if (hit_result.render_object) {
        local_x = hit_result.local_x - hit_result.render_object->GetComputedStyle().padding.left.ToPx();
    }

    CaretPosition caret_pos = selection_manager_->HitTestToCaretPosition(hit_result.element,
                                                                         static_cast<int>(std::round(local_x)),
                                                                         static_cast<int>(std::round(logical_y)),
                                                                         &font);
    if (!caret_pos.IsValid()) {
        selection->Collapse(text_node, 0);
        return;
    }

    selection->Collapse(caret_pos.node, caret_pos.offset);
}

} // namespace mbink
