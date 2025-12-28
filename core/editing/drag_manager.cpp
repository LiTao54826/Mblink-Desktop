/**
 * @file drag_manager.cpp
 * @brief 拖拽管理器实现
 */

#include "drag_manager.h"
#include "core/event/types/data_transfer.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include "core/dom/event.h"
#include "core/dom/drag_event.h"
#include "core/render/objects/render_object.h"
#include "core/event/input/hit_testing.h"
#include "core/event/types/mouse_event.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace lightui {

DragManager::DragManager()
    : drag_mode_(DragMode::None)
    , drag_state_(DragState::None)
    , drag_started_(false)
    , drag_verbose_(false)
    , detect_start_x_(0.0f)
    , detect_start_y_(0.0f)
    , drag_start_x_(0.0f)
    , drag_start_y_(0.0f)
    , data_transfer_(std::make_shared<DataTransfer>())
    , drop_allowed_(false) {
}

DragManager::~DragManager() {
    ReleaseDragClone();
}

bool DragManager::StartDragDetection(std::shared_ptr<Element> element, float mouse_x, float mouse_y) {
    if (!element) {
        return false;
    }

    // 查找可拖拽的元素
    auto draggable = FindDraggableElement(element);
    if (!draggable) {
        return false;
    }

    // 设置拖拽元素
    drag_element_ = draggable;
    drag_mode_ = GetDragMode(draggable);
    drag_state_ = DragState::Detecting;  // 进入检测状态
    drag_started_ = false;
    drag_verbose_ = (drag_mode_ == DragMode::DragDrop || drag_mode_ == DragMode::Clone);

    // 记录检测起始位置
    detect_start_x_ = mouse_x;
    detect_start_y_ = mouse_y;

    // 初始化DataTransfer对象
    data_transfer_ = std::make_shared<DataTransfer>();
    data_transfer_->SetEffectAllowed(DragEffect::All);  // 默认允许所有效果

    return true;
}

bool DragManager::CheckDragThreshold(float mouse_x, float mouse_y) const {
    if (drag_state_ != DragState::Detecting) {
        return false;
    }

    // 计算鼠标移动距离
    float dx = mouse_x - detect_start_x_;
    float dy = mouse_y - detect_start_y_;
    float distance = std::sqrt(dx * dx + dy * dy);

    return distance >= DRAG_THRESHOLD;
}

bool DragManager::UpdateDrag(float mouse_x, float mouse_y, std::shared_ptr<Document> document,
                             std::shared_ptr<RenderObject> root_render) {
    auto drag_element = drag_element_.lock();
    if (!drag_element) {
        return false;
    }

    // 如果在检测状态，检查是否超过阈值
    if (drag_state_ == DragState::Detecting) {
        if (!CheckDragThreshold(mouse_x, mouse_y)) {
            // 还没超过阈值，不开始拖拽
            return false;
        }
        // 超过阈值，转换到拖拽状态
        drag_state_ = DragState::Dragging;
    }

    // 如果还没开始拖拽，发送dragstart事件
    if (!drag_started_) {
        drag_start_x_ = mouse_x;
        drag_start_y_ = mouse_y;

        // 发送dragstart事件（使用 DragEvent）
        auto dragstart_event = std::make_shared<DragEvent>(
            DragEvent::DRAG_START,
            static_cast<int>(mouse_x),
            static_cast<int>(mouse_y),
            0,
            data_transfer_
        );
        drag_element->DispatchEvent(dragstart_event);
        drag_started_ = true;

        // 如果是克隆模式，创建拖拽克隆
        if (drag_mode_ == DragMode::Clone) {
            CreateDragClone(drag_element, mouse_x, mouse_y);
        }

        // 设置:drag伪类
        drag_element->SetPseudoClass("drag", true);
    }

    // 发送drag事件
    auto drag_event = std::make_shared<DragEvent>(
        DragEvent::DRAG,
        static_cast<int>(mouse_x),
        static_cast<int>(mouse_y),
        0,
        data_transfer_
    );
    drag_element->DispatchEvent(drag_event);

    // 更新拖拽hover链
    UpdateDragHoverChain(mouse_x, mouse_y, document, root_render);

    return true;
}

void DragManager::EndDrag(float mouse_x, float mouse_y) {
    auto drag_element = drag_element_.lock();
    if (!drag_element) {
        // 清理DataTransfer
        if (data_transfer_) {
            data_transfer_->ClearData();
        }
        drag_state_ = DragState::None;
        drop_allowed_ = false;
        return;
    }

    if (drag_started_) {
        auto drag_hover = drag_hover_element_.lock();
        
        // 检查是否允许 drop
        // 1. 必须有目标元素
        // 2. 必须是详细模式
        // 3. dropEffect 必须与 effectAllowed 兼容
        // 4. dragover 必须调用了 preventDefault
        bool can_drop = drag_hover && drag_verbose_ && drop_allowed_;
        
        if (can_drop && data_transfer_) {
            // 检查效果兼容性
            DragEffect effect_allowed = data_transfer_->GetEffectAllowed();
            DragEffect drop_effect = data_transfer_->GetDropEffect();
            
            if (!IsEffectCompatible(effect_allowed, drop_effect)) {
                // 不兼容，设置为 none 并取消 drop
                data_transfer_->SetDropEffect(DragEffect::None);
                can_drop = false;
            }
            
            // 如果 dropEffect 是 none，也取消 drop
            if (data_transfer_->GetDropEffect() == DragEffect::None) {
                can_drop = false;
            }
        }
        
        if (can_drop && drag_hover) {
            // 发送 drop 事件
            auto drop_event = std::make_shared<DragEvent>(
                DragEvent::DROP,
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0,
                data_transfer_
            );
            drag_hover->DispatchEvent(drop_event);
        }
        
        // 发送 dragleave 事件（如果有悬停元素）
        if (drag_hover && drag_verbose_) {
            auto dragleave_event = std::make_shared<DragEvent>(
                DragEvent::DRAG_LEAVE,
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0,
                data_transfer_
            );
            drag_hover->DispatchEvent(dragleave_event);
        }

        // 发送dragend事件（总是发送）
        auto dragend_event = std::make_shared<DragEvent>(
            DragEvent::DRAG_END,
            static_cast<int>(mouse_x),
            static_cast<int>(mouse_y),
            0,
            data_transfer_
        );
        drag_element->DispatchEvent(dragend_event);

        // 移除:drag伪类
        drag_element->SetPseudoClass("drag", false);

        // 释放拖拽克隆
        ReleaseDragClone();
    }

    // 清除拖拽状态
    drag_element_.reset();
    drag_hover_element_.reset();
    drag_hover_chain_.clear();
    drag_mode_ = DragMode::None;
    drag_state_ = DragState::None;
    drag_started_ = false;
    drag_verbose_ = false;
    drop_allowed_ = false;

    // 清理DataTransfer
    if (data_transfer_) {
        data_transfer_->ClearData();
    }
}

void DragManager::CancelDrag() {
    auto drag_element = drag_element_.lock();
    if (drag_element && drag_started_) {
        // 移除:drag伪类
        drag_element->SetPseudoClass("drag", false);
        
        // 释放拖拽克隆
        ReleaseDragClone();
    }

    // 清除拖拽状态
    drag_element_.reset();
    drag_hover_element_.reset();
    drag_hover_chain_.clear();
    drag_mode_ = DragMode::None;
    drag_state_ = DragState::None;
    drag_started_ = false;
    drag_verbose_ = false;
    drop_allowed_ = false;

    // 清理DataTransfer
    if (data_transfer_) {
        data_transfer_->ClearData();
    }
}

std::shared_ptr<Element> DragManager::GetDragElement() const {
    return drag_element_.lock();
}

std::shared_ptr<Element> DragManager::GetDragHoverElement() const {
    return drag_hover_element_.lock();
}

bool DragManager::IsDragging() const {
    return drag_started_ && !drag_element_.expired();
}

std::shared_ptr<Element> DragManager::GetDragClone() const {
    return drag_clone_;
}

std::shared_ptr<DataTransfer> DragManager::GetDataTransfer() const {
    return data_transfer_;
}

bool DragManager::IsElementDraggable(std::shared_ptr<Element> element) const {
    if (!element) {
        return false;
    }

    DragMode mode = GetDragMode(element);
    return mode == DragMode::Drag || mode == DragMode::DragDrop || mode == DragMode::Clone;
}

bool DragManager::IsEffectCompatible(DragEffect effect_allowed, DragEffect drop_effect) {
    // 参考：W3C HTML5 - Drag and Drop
    // https://html.spec.whatwg.org/multipage/dnd.html#dom-datatransfer-dropeffect
    
    // "none" 总是兼容的（表示不允许 drop）
    if (drop_effect == DragEffect::None) {
        return true;
    }

    // "uninitialized" 允许所有效果
    if (effect_allowed == DragEffect::Uninitialized || effect_allowed == DragEffect::All) {
        return true;
    }

    // 检查具体的兼容性
    switch (drop_effect) {
        case DragEffect::Copy:
            return effect_allowed == DragEffect::Copy ||
                   effect_allowed == DragEffect::CopyMove ||
                   effect_allowed == DragEffect::CopyLink;
        case DragEffect::Move:
            return effect_allowed == DragEffect::Move ||
                   effect_allowed == DragEffect::CopyMove ||
                   effect_allowed == DragEffect::LinkMove;
        case DragEffect::Link:
            return effect_allowed == DragEffect::Link ||
                   effect_allowed == DragEffect::CopyLink ||
                   effect_allowed == DragEffect::LinkMove;
        default:
            return false;
    }
}

std::shared_ptr<Element> DragManager::FindDraggableElement(std::shared_ptr<Element> element) {
    if (!element) {
        return nullptr;
    }

    // 参考：RmlUi/Source/Core/Context.cpp - ProcessMouseButtonDown (lines 707-719)
    // 向上遍历DOM树，查找第一个可拖拽的元素
    
    auto current = element;
    while (current) {
        DragMode mode = GetDragMode(current);
        
        switch (mode) {
        case DragMode::None:
            // 继续向上查找
            break;
        case DragMode::Block:
            // 阻止拖拽
            return nullptr;
        case DragMode::Drag:
        case DragMode::DragDrop:
        case DragMode::Clone:
            // 找到可拖拽元素
            return current;
        }

        // 向上查找父元素
        auto parent = current->GetParentNode();
        if (parent && parent->GetNodeType() == NodeType::ELEMENT_NODE) {
            current = std::static_pointer_cast<Element>(parent);
        } else {
            current = nullptr;
        }
    }

    return nullptr;
}

DragMode DragManager::GetDragMode(std::shared_ptr<Element> element) const {
    if (!element) {
        return DragMode::None;
    }

    // 优先检查 HTML5 标准 draggable 属性
    std::string draggable_attr = element->GetAttribute("draggable");
    if (!draggable_attr.empty()) {
        // draggable 属性存在，优先使用
        if (draggable_attr == "true" || draggable_attr.empty()) {
            // draggable="true" 或 draggable（空值）表示可拖拽
            // 使用 DragDrop 模式以发送完整的 HTML5 事件
            return DragMode::DragDrop;
        } else if (draggable_attr == "false") {
            return DragMode::None;
        }
        // 其他无效值视为 false
        return DragMode::None;
    }

    // 向后兼容：检查 RmlUi 风格的 drag 属性
    std::string drag_attr = element->GetAttribute("drag");
    if (drag_attr.empty()) {
        return DragMode::None;
    }

    // 解析drag属性值
    if (drag_attr == "none") {
        return DragMode::None;
    } else if (drag_attr == "drag") {
        return DragMode::Drag;
    } else if (drag_attr == "drag-drop") {
        return DragMode::DragDrop;
    } else if (drag_attr == "clone") {
        return DragMode::Clone;
    } else if (drag_attr == "block") {
        return DragMode::Block;
    }

    return DragMode::None;
}

void DragManager::CreateDragClone(std::shared_ptr<Element> element, float mouse_x, float mouse_y) {
    // 参考：RmlUi/Source/Core/Context.cpp - CreateDragClone (lines 1471-1511)

    if (!element) {
        return;
    }

    // 1. 克隆元素（深度克隆）
    auto cloned_node = element->CloneNode(true);
    drag_clone_ = std::dynamic_pointer_cast<Element>(cloned_node);

    if (!drag_clone_) {
        return;
    }

    // 2. 设置克隆元素的位置（跟随鼠标）
    // 注意：这里简化处理，实际应该设置position: absolute和left/top
    // 在完整实现中，应该将克隆元素添加到一个特殊的"cursor proxy"文档
    drag_clone_->SetAttribute("style",
        "position: absolute; left: " + std::to_string(static_cast<int>(mouse_x)) +
        "px; top: " + std::to_string(static_cast<int>(mouse_y)) + "px;");

    // 3. 设置:drag伪类
    drag_clone_->SetPseudoClass("drag", true);

    // 4. 将克隆元素添加到cursor proxy文档
    // TODO: 在完整实现中，应该有一个专门的cursor proxy文档
    // 现在暂时不添加到DOM树，只保存引用

    (void)mouse_x;
    (void)mouse_y;
}

void DragManager::ReleaseDragClone() {
    if (drag_clone_) {
        // 移除:drag伪类
        drag_clone_->SetPseudoClass("drag", false);

        // TODO: 从cursor proxy文档中移除克隆元素

        drag_clone_ = nullptr;
    }
}

void DragManager::UpdateDragHoverChain(float mouse_x, float mouse_y, std::shared_ptr<Document> document,
                                       std::shared_ptr<RenderObject> root_render) {
    if (!document) {
        return;
    }

    // 执行Hit Testing获取当前鼠标下的元素（忽略拖拽元素）
    HitTesting hit_testing;
    HitTestResult hit_result;
    
    // 优先使用渲染树进行精确 hit testing
    if (root_render) {
        hit_result = hit_testing.HitTestRenderObject(root_render, mouse_x, mouse_y, 0.0f, 0.0f);
    } else {
        // 回退到简化的 DOM 遍历（不推荐，可能不准确）
        hit_result = hit_testing.HitTest(document, mouse_x, mouse_y);
    }
    
    // 构建新的拖拽hover链
    std::unordered_set<Element*> new_drag_hover_chain;
    Element* new_drag_hover = nullptr;
    
    if (hit_result.IsValid()) {
        new_drag_hover = hit_result.element.get();
        
        // 从目标元素向上遍历到根元素
        Element* current = new_drag_hover;
        while (current) {
            new_drag_hover_chain.insert(current);
            
            // 获取父元素
            auto parent_node = current->GetParentNode();
            if (parent_node && parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                current = static_cast<Element*>(parent_node.get());
            } else {
                current = nullptr;
            }
        }
    }
    
    // 发送 HTML5 标准事件（如果是详细模式）
    if (drag_started_ && drag_verbose_) {
        // dragleave: 离开的元素（在旧链中但不在新链中）
        SendDragEvents(drag_hover_chain_, new_drag_hover_chain, DragEvent::DRAG_LEAVE, mouse_x, mouse_y);
        // dragenter: 进入的元素（在新链中但不在旧链中）
        SendDragEvents(new_drag_hover_chain, drag_hover_chain_, DragEvent::DRAG_ENTER, mouse_x, mouse_y);
        
        // dragover: 发送到当前悬停元素
        // 重置 drop_allowed_，等待 dragover 处理器调用 preventDefault
        drop_allowed_ = false;
        
        if (new_drag_hover) {
            try {
                auto element_ptr = std::static_pointer_cast<Element>(new_drag_hover->shared_from_this());
                auto dragover_event = std::make_shared<DragEvent>(
                    DragEvent::DRAG_OVER,
                    static_cast<int>(mouse_x),
                    static_cast<int>(mouse_y),
                    0,
                    data_transfer_
                );
                element_ptr->DispatchEvent(dragover_event);
                
                // 检查是否调用了 preventDefault
                if (dragover_event->IsDefaultPrevented()) {
                    drop_allowed_ = true;
                }
            } catch (...) {
                // 忽略
            }
        }
    }
    
    // 更新拖拽hover链
    drag_hover_chain_ = std::move(new_drag_hover_chain);
    
    // 更新拖拽hover元素
    if (new_drag_hover) {
        try {
            drag_hover_element_ = std::static_pointer_cast<Element>(new_drag_hover->shared_from_this());
        } catch (...) {
            drag_hover_element_.reset();
        }
    } else {
        drag_hover_element_.reset();
    }
}

void DragManager::SendDragEvents(const std::unordered_set<Element*>& old_items,
                                const std::unordered_set<Element*>& new_items,
                                const std::string& event_type,
                                float mouse_x,
                                float mouse_y) {
    // 找出在old_items中但不在new_items中的元素
    for (Element* element : old_items) {
        if (new_items.find(element) == new_items.end()) {
            // 这个元素在旧集合中但不在新集合中
            
            // 创建拖拽事件（使用 DragEvent）
            auto drag_event = std::make_shared<DragEvent>(
                event_type,
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0,
                data_transfer_
            );
            
            // 分发事件
            try {
                auto element_ptr = std::static_pointer_cast<Element>(element->shared_from_this());
                element_ptr->DispatchEvent(drag_event);
            } catch (...) {
                // 如果shared_from_this失败，说明元素已被销毁，忽略
            }
        }
    }
}

} // namespace lightui

