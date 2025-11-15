/**
 * @file event.cpp
 * @brief DOM 事件类实现
 */

#include "event.h"
#include "node.h"
#include <chrono>

namespace lightui {

// ========== Event 类实现 ==========

Event::Event(const std::string& type, bool bubbles, bool cancelable)
    : type_(type)
    , target_()
    , current_target_()
    , event_phase_(EventPhase::NONE)
    , bubbles_(bubbles)
    , cancelable_(cancelable)
    , time_stamp_(0.0)
    , propagation_stopped_(false)
    , immediate_propagation_stopped_(false)
    , default_prevented_(false) {
    
    // 获取当前时间戳（毫秒）
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    time_stamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void Event::StopPropagation() {
    propagation_stopped_ = true;
}

void Event::StopImmediatePropagation() {
    propagation_stopped_ = true;
    immediate_propagation_stopped_ = true;
}

void Event::PreventDefault() {
    if (cancelable_) {
        default_prevented_ = true;
    }
}

void Event::SetTarget(std::shared_ptr<Node> target) {
    target_ = target;
}

void Event::SetCurrentTarget(std::shared_ptr<Node> current_target) {
    current_target_ = current_target;
}

void Event::SetEventPhase(EventPhase phase) {
    event_phase_ = phase;
}

void Event::Reset() {
    target_.reset();
    current_target_.reset();
    event_phase_ = EventPhase::NONE;
    propagation_stopped_ = false;
    immediate_propagation_stopped_ = false;
    default_prevented_ = false;
}

// ========== MouseEvent 类实现 ==========

MouseEvent::MouseEvent(const std::string& type, int x, int y, int button)
    : Event(type, true, true)
    , client_x_(x)
    , client_y_(y)
    , button_(button) {
}

// ========== KeyboardEvent 类实现 ==========

KeyboardEvent::KeyboardEvent(const std::string& type,
                             const std::string& key,
                             const std::string& code,
                             int key_code,
                             bool ctrl_key,
                             bool shift_key,
                             bool alt_key,
                             bool meta_key,
                             bool repeat)
    : Event(type, true, true)
    , key_(key)
    , code_(code)
    , key_code_(key_code)
    , ctrl_key_(ctrl_key)
    , shift_key_(shift_key)
    , alt_key_(alt_key)
    , meta_key_(meta_key)
    , repeat_(repeat) {
}

bool KeyboardEvent::GetModifierState(const std::string& key_arg) const {
    // 参考：W3C UI Events - KeyboardEvent.getModifierState()
    if (key_arg == "Control" || key_arg == "Ctrl") {
        return ctrl_key_;
    } else if (key_arg == "Shift") {
        return shift_key_;
    } else if (key_arg == "Alt") {
        return alt_key_;
    } else if (key_arg == "Meta") {
        return meta_key_;
    }
    return false;
}

// ========== AnimationEvent 类实现 ==========

AnimationEvent::AnimationEvent(const std::string& type,
                               const std::string& animation_name,
                               float elapsed_time,
                               const std::string& pseudo_element)
    : Event(type, true, false)  // 动画事件冒泡但不可取消
    , animation_name_(animation_name)
    , elapsed_time_(elapsed_time)
    , pseudo_element_(pseudo_element) {
}

} // namespace lightui

