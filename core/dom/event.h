/**
 * @file event.h
 * @brief DOM 事件类
 * 
 * 功能：
 * - 表示 DOM 事件对象
 * - 支持事件冒泡和捕获
 * - 事件阶段管理
 * - 事件传播控制
 * 
 * 实现要点：
 * - 符合 W3C DOM Events 标准
 * - 支持三个事件阶段：捕获、目标、冒泡
 * - 支持 stopPropagation 和 preventDefault
 */

#pragma once

#include <string>
#include <memory>

namespace lightui {

// 前向声明
class Node;

/**
 * @brief 事件阶段枚举
 */
enum class EventPhase {
    NONE = 0,
    CAPTURING_PHASE = 1,
    AT_TARGET = 2,
    BUBBLING_PHASE = 3
};

/**
 * @brief DOM 事件类
 */
class Event {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型（如 "click", "mousemove"）
     * @param bubbles 是否冒泡
     * @param cancelable 是否可取消
     */
    explicit Event(const std::string& type, bool bubbles = true, bool cancelable = true);
    
    /**
     * @brief 析构函数
     */
    virtual ~Event() = default;
    
    // ========== 事件属性 ==========
    
    /**
     * @brief 获取事件类型
     * @return 事件类型
     */
    std::string GetType() const { return type_; }
    
    /**
     * @brief 获取事件目标（触发事件的节点）
     * @return 事件目标
     */
    std::shared_ptr<Node> GetTarget() const { return target_.lock(); }
    
    /**
     * @brief 获取当前目标（当前处理事件的节点）
     * @return 当前目标
     */
    std::shared_ptr<Node> GetCurrentTarget() const { return current_target_.lock(); }
    
    /**
     * @brief 获取事件阶段
     * @return 事件阶段
     */
    EventPhase GetEventPhase() const { return event_phase_; }
    
    /**
     * @brief 检查事件是否冒泡
     * @return true 表示冒泡
     */
    bool GetBubbles() const { return bubbles_; }
    
    /**
     * @brief 检查事件是否可取消
     * @return true 表示可取消
     */
    bool GetCancelable() const { return cancelable_; }
    
    /**
     * @brief 获取时间戳
     * @return 时间戳（毫秒）
     */
    double GetTimeStamp() const { return time_stamp_; }
    
    // ========== 事件控制 ==========
    
    /**
     * @brief 停止事件传播
     */
    void StopPropagation();
    
    /**
     * @brief 立即停止事件传播（包括当前节点的其他监听器）
     */
    void StopImmediatePropagation();
    
    /**
     * @brief 阻止默认行为
     */
    void PreventDefault();
    
    /**
     * @brief 检查事件传播是否已停止
     * @return true 表示已停止
     */
    bool IsPropagationStopped() const { return propagation_stopped_; }
    
    /**
     * @brief 检查事件传播是否立即停止
     * @return true 表示立即停止
     */
    bool IsImmediatePropagationStopped() const { return immediate_propagation_stopped_; }
    
    /**
     * @brief 检查默认行为是否已阻止
     * @return true 表示已阻止
     */
    bool IsDefaultPrevented() const { return default_prevented_; }
    
    // ========== 内部方法（由事件分发系统使用） ==========
    
    /**
     * @brief 设置事件目标
     * @param target 事件目标
     */
    void SetTarget(std::shared_ptr<Node> target);
    
    /**
     * @brief 设置当前目标
     * @param current_target 当前目标
     */
    void SetCurrentTarget(std::shared_ptr<Node> current_target);
    
    /**
     * @brief 设置事件阶段
     * @param phase 事件阶段
     */
    void SetEventPhase(EventPhase phase);
    
    /**
     * @brief 重置事件状态（用于事件对象复用）
     */
    void Reset();

protected:
    std::string type_;
    std::weak_ptr<Node> target_;
    std::weak_ptr<Node> current_target_;
    EventPhase event_phase_;
    bool bubbles_;
    bool cancelable_;
    double time_stamp_;
    
    bool propagation_stopped_;
    bool immediate_propagation_stopped_;
    bool default_prevented_;
};

/**
 * @brief 鼠标事件类
 */
class MouseEvent : public Event {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @param button 鼠标按钮
     */
    MouseEvent(const std::string& type, int x, int y, int button = 0);
    
    /**
     * @brief 获取鼠标 X 坐标
     */
    int GetClientX() const { return client_x_; }
    
    /**
     * @brief 获取鼠标 Y 坐标
     */
    int GetClientY() const { return client_y_; }
    
    /**
     * @brief 获取鼠标按钮
     */
    int GetButton() const { return button_; }

private:
    int client_x_;
    int client_y_;
    int button_;
};

/**
 * @brief 键盘事件类
 */
class KeyboardEvent : public Event {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型
     * @param key 按键名称
     * @param code 按键代码
     */
    KeyboardEvent(const std::string& type, const std::string& key, const std::string& code);
    
    /**
     * @brief 获取按键名称
     */
    std::string GetKey() const { return key_; }
    
    /**
     * @brief 获取按键代码
     */
    std::string GetCode() const { return code_; }

private:
    std::string key_;
    std::string code_;
};

} // namespace lightui

