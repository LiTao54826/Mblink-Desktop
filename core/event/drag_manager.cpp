/**
 * @file drag_manager.cpp
 * @brief 拖拽管理器实现
 */

#include "drag_manager.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include "core/dom/event.h"
#include "hit_testing.h"
#include "mouse_event.h"
#include <algorithm>

namespace lightui {

DragManager::DragManager()
    : drag_mode_(DragMode::None)
    , drag_started_(false)
    , drag_verbose_(false)
    , drag_start_x_(0.0f)
    , drag_start_y_(0.0f) {
}

DragManager::~DragManager() {
    ReleaseDragClone();
}

bool DragManager::StartDragDetection(std::shared_ptr<Element> element) {
    if (!element) {
        return false;
    }

    // 查找可拖拽的元素
    // 参考：RmlUi/Source/Core/Context.cpp - ProcessMouseButtonDown (lines 705-720)
    auto draggable = FindDraggableElement(element);
    if (!draggable) {
        return false;
    }

    // 设置拖拽元素
    drag_element_ = draggable;
    drag_mode_ = GetDragMode(draggable);
    drag_started_ = false;
    drag_verbose_ = (drag_mode_ == DragMode::DragDrop || drag_mode_ == DragMode::Clone);

    return true;
}

bool DragManager::UpdateDrag(float mouse_x, float mouse_y, std::shared_ptr<Document> document) {
    auto drag_element = drag_element_.lock();
    if (!drag_element) {
        return false;
    }

    // 如果还没开始拖拽，发送dragstart事件
    // 参考：RmlUi/Source/Core/Context.cpp - UpdateHoverChain (lines 1309-1322)
    if (!drag_started_) {
        drag_start_x_ = mouse_x;
        drag_start_y_ = mouse_y;

        // 发送dragstart事件
        auto dragstart_event = std::make_shared<MouseEvent>(
            "dragstart",
            static_cast<int>(mouse_x),
            static_cast<int>(mouse_y),
            0
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
    auto drag_event = std::make_shared<MouseEvent>(
        "drag",
        static_cast<int>(mouse_x),
        static_cast<int>(mouse_y),
        0
    );
    drag_element->DispatchEvent(drag_event);

    // 更新拖拽hover链
    UpdateDragHoverChain(mouse_x, mouse_y, document);

    // 发送dragmove事件（如果是详细模式）
    if (drag_verbose_) {
        auto drag_hover = drag_hover_element_.lock();
        if (drag_hover) {
            auto dragmove_event = std::make_shared<MouseEvent>(
                "dragmove",
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0
            );
            drag_hover->DispatchEvent(dragmove_event);
        }
    }

    return true;
}

void DragManager::EndDrag(float mouse_x, float mouse_y) {
    auto drag_element = drag_element_.lock();
    if (!drag_element) {
        return;
    }

    if (drag_started_) {
        // 参考：RmlUi/Source/Core/Context.cpp - ProcessMouseButtonUp (lines 779-803)
        
        // 发送dragdrop事件到拖拽悬停元素
        auto drag_hover = drag_hover_element_.lock();
        if (drag_hover && drag_verbose_) {
            auto dragdrop_event = std::make_shared<MouseEvent>(
                "dragdrop",
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0
            );
            drag_hover->DispatchEvent(dragdrop_event);

            // 发送dragout事件
            if (drag_hover) {  // 用户可能在dragdrop事件中移除了元素
                auto dragout_event = std::make_shared<MouseEvent>(
                    "dragout",
                    static_cast<int>(mouse_x),
                    static_cast<int>(mouse_y),
                    0
                );
                drag_hover->DispatchEvent(dragout_event);
            }
        }

        // 发送dragend事件
        if (drag_element) {  // 用户可能在dragdrop事件中移除了元素
            auto dragend_event = std::make_shared<MouseEvent>(
                "dragend",
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0
            );
            drag_element->DispatchEvent(dragend_event);

            // 移除:drag伪类
            drag_element->SetPseudoClass("drag", false);
        }

        // 释放拖拽克隆
        ReleaseDragClone();
    }

    // 清除拖拽状态
    drag_element_.reset();
    drag_hover_element_.reset();
    drag_hover_chain_.clear();
    drag_mode_ = DragMode::None;
    drag_started_ = false;
    drag_verbose_ = false;
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
    drag_started_ = false;
    drag_verbose_ = false;
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

DragMode DragManager::GetDragMode(std::shared_ptr<Element> element) {
    if (!element) {
        return DragMode::None;
    }

    // 检查drag属性
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
    
    // TODO: 实现元素克隆
    // 这需要Element::Clone()方法，暂时留空
    // 
    // 克隆步骤：
    // 1. 克隆元素
    // 2. 设置克隆元素的位置（跟随鼠标）
    // 3. 设置:drag伪类
    // 4. 将克隆元素添加到cursor proxy文档
    
    (void)element;
    (void)mouse_x;
    (void)mouse_y;
}

void DragManager::ReleaseDragClone() {
    if (drag_clone_) {
        // TODO: 从cursor proxy文档中移除克隆元素
        drag_clone_ = nullptr;
    }
}

void DragManager::UpdateDragHoverChain(float mouse_x, float mouse_y, std::shared_ptr<Document> document) {
    if (!document) {
        return;
    }

    // 参考：RmlUi/Source/Core/Context.cpp - UpdateHoverChain (lines 1361-1382)
    
    // 执行Hit Testing获取当前鼠标下的元素（忽略拖拽元素）
    HitTesting hit_testing;
    auto hit_result = hit_testing.HitTest(document, mouse_x, mouse_y);
    
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
    
    // 发送dragout/dragover事件（如果是详细模式）
    if (drag_started_ && drag_verbose_) {
        SendDragEvents(drag_hover_chain_, new_drag_hover_chain, "dragout", mouse_x, mouse_y);
        SendDragEvents(new_drag_hover_chain, drag_hover_chain_, "dragover", mouse_x, mouse_y);
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
    // 参考：RmlUi/Source/Core/Context.cpp - SendEvents
    // 找出在old_items中但不在new_items中的元素
    
    for (Element* element : old_items) {
        if (new_items.find(element) == new_items.end()) {
            // 这个元素在旧集合中但不在新集合中
            
            // 创建拖拽事件
            auto drag_event = std::make_shared<MouseEvent>(
                event_type,
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0
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

