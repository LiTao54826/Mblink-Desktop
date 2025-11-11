/**
 * @file event_types.cpp
 * @brief 事件类型注册表实现
 */

#include "event_types.h"
#include <cstring>

namespace lightui {

// ========== EventTypeRegistry 实现 ==========

EventTypeRegistry::EventTypeRegistry() 
    : next_custom_id_(static_cast<uint32_t>(EventId::Custom)) {
    // 初始化数组为nullptr
    std::memset(specifications_, 0, sizeof(specifications_));
    
    // 注册所有内置事件
    RegisterBuiltinEvents();
}

EventTypeRegistry& EventTypeRegistry::GetInstance() {
    static EventTypeRegistry instance;
    return instance;
}

void EventTypeRegistry::RegisterBuiltinEvents() {
    // ========== 鼠标事件 ==========
    // 参考：RmlUi/Source/Core/EventSpecification.cpp
    
    RegisterEvent(EventId::Click, "click", true, true);
    RegisterEvent(EventId::Dblclick, "dblclick", true, true);
    RegisterEvent(EventId::Mousedown, "mousedown", true, true);
    RegisterEvent(EventId::Mouseup, "mouseup", true, true);
    RegisterEvent(EventId::Mousemove, "mousemove", true, true);
    RegisterEvent(EventId::Mouseover, "mouseover", true, true);
    RegisterEvent(EventId::Mouseout, "mouseout", true, true);
    RegisterEvent(EventId::Mouseenter, "mouseenter", false, false);  // 不冒泡
    RegisterEvent(EventId::Mouseleave, "mouseleave", false, false);  // 不冒泡
    RegisterEvent(EventId::Contextmenu, "contextmenu", true, true);
    
    // ========== 键盘事件 ==========
    
    RegisterEvent(EventId::Keydown, "keydown", true, true);
    RegisterEvent(EventId::Keyup, "keyup", true, true);
    RegisterEvent(EventId::Keypress, "keypress", true, true);  // 已废弃但保留
    RegisterEvent(EventId::Textinput, "textinput", true, true);
    
    // ========== 焦点事件 ==========
    
    RegisterEvent(EventId::Focus, "focus", false, false);      // 不冒泡
    RegisterEvent(EventId::Blur, "blur", false, false);        // 不冒泡
    RegisterEvent(EventId::Focusin, "focusin", true, false);   // 冒泡版本
    RegisterEvent(EventId::Focusout, "focusout", true, false); // 冒泡版本
    
    // ========== 拖拽事件 ==========
    
    RegisterEvent(EventId::Dragstart, "dragstart", true, true);
    RegisterEvent(EventId::Drag, "drag", true, true);
    RegisterEvent(EventId::Dragend, "dragend", true, false);
    RegisterEvent(EventId::Dragover, "dragover", true, true);
    RegisterEvent(EventId::Dragenter, "dragenter", true, true);
    RegisterEvent(EventId::Dragleave, "dragleave", true, false);
    RegisterEvent(EventId::Drop, "drop", true, true);
    
    // ========== 表单事件 ==========
    
    RegisterEvent(EventId::Submit, "submit", true, true);
    RegisterEvent(EventId::Change, "change", true, false);
    RegisterEvent(EventId::Input, "input", true, false);
    
    // ========== 窗口事件 ==========
    
    RegisterEvent(EventId::Resize, "resize", false, false);
    RegisterEvent(EventId::Scroll, "scroll", false, false);  // 注意：某些元素的scroll可以冒泡
    RegisterEvent(EventId::Load, "load", false, false);
    RegisterEvent(EventId::Unload, "unload", false, false);
}

void EventTypeRegistry::RegisterEvent(EventId id, const std::string& type,
                                      bool bubbles, bool cancelable, bool interruptible) {
    size_t index = static_cast<size_t>(id);
    if (index >= static_cast<size_t>(EventId::MaxNumIds)) {
        return;  // 超出范围
    }
    
    // 创建事件规范
    specifications_[index] = new EventSpecification(id, type, bubbles, cancelable, interruptible);
    
    // 添加到映射表
    type_to_id_[type] = id;
}

EventId EventTypeRegistry::GetId(const std::string& type) const {
    auto it = type_to_id_.find(type);
    if (it != type_to_id_.end()) {
        return it->second;
    }
    return EventId::Invalid;
}

EventId EventTypeRegistry::GetIdOrInsert(const std::string& type) {
    // 先尝试查找
    EventId id = GetId(type);
    if (id != EventId::Invalid) {
        return id;
    }
    
    // 不存在，注册为自定义事件
    return RegisterCustomEvent(type);
}

std::string EventTypeRegistry::GetType(EventId id) const {
    size_t index = static_cast<size_t>(id);
    if (index >= static_cast<size_t>(EventId::MaxNumIds)) {
        return "";
    }
    
    const EventSpecification* spec = specifications_[index];
    if (spec) {
        return spec->type;
    }
    return "";
}

const EventSpecification* EventTypeRegistry::GetSpecification(EventId id) const {
    size_t index = static_cast<size_t>(id);
    if (index >= static_cast<size_t>(EventId::MaxNumIds)) {
        return nullptr;
    }
    return specifications_[index];
}

EventId EventTypeRegistry::RegisterCustomEvent(const std::string& type, 
                                               bool bubbles, bool cancelable) {
    // 检查是否已存在
    EventId existing_id = GetId(type);
    if (existing_id != EventId::Invalid) {
        return existing_id;
    }
    
    // 分配新ID
    if (next_custom_id_ >= static_cast<uint32_t>(EventId::MaxNumIds)) {
        // ID用完了
        return EventId::Invalid;
    }
    
    EventId new_id = static_cast<EventId>(next_custom_id_++);
    RegisterEvent(new_id, type, bubbles, cancelable);
    
    return new_id;
}

} // namespace lightui

