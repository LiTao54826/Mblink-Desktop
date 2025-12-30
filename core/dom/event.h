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
     * @param button 鼠标按钮（触发事件的按钮：0=左键，1=中键，2=右键）
     * @param detail 点击次数（1=单击，2=双击，3=三击）
     * @param buttons 当前按下的按钮位掩码（1=左键，2=右键，4=中键）
     */
    MouseEvent(const std::string& type, int x, int y, int button = 0, int detail = 1, int buttons = 0);
    
    /**
     * @brief 获取鼠标 X 坐标
     */
    int GetClientX() const { return client_x_; }
    
    /**
     * @brief 获取鼠标 Y 坐标
     */
    int GetClientY() const { return client_y_; }
    
    /**
     * @brief 获取鼠标按钮（触发事件的按钮）
     * @return 0=左键，1=中键，2=右键
     */
    int GetButton() const { return button_; }
    
    /**
     * @brief 获取当前按下的按钮位掩码
     * @return 位掩码：1=左键，2=右键，4=中键
     */
    int GetButtons() const { return buttons_; }
    
    /**
     * @brief 获取点击次数
     * @return 点击次数（1=单击，2=双击，3=三击）
     */
    int GetDetail() const { return detail_; }

private:
    int client_x_;
    int client_y_;
    int button_;
    int buttons_;
    int detail_;
};

/**
 * @brief 键盘事件类
 *
 * 参考：
 * - W3C UI Events - KeyboardEvent
 * - MDN Web Docs - KeyboardEvent
 */
class KeyboardEvent : public Event {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型（keydown, keyup, keypress）
     * @param key 按键名称（如"a", "Enter", "ArrowUp"）
     * @param code 按键代码（如"KeyA", "Enter", "ArrowUp"）
     * @param key_code 按键码（已废弃但保留兼容性）
     * @param ctrl_key Ctrl键是否按下
     * @param shift_key Shift键是否按下
     * @param alt_key Alt键是否按下
     * @param meta_key Meta键（Windows键/Command键）是否按下
     * @param repeat 是否是重复按键
     */
    KeyboardEvent(const std::string& type,
                  const std::string& key,
                  const std::string& code,
                  int key_code = 0,
                  bool ctrl_key = false,
                  bool shift_key = false,
                  bool alt_key = false,
                  bool meta_key = false,
                  bool repeat = false);

    /**
     * @brief 获取按键名称
     * @return 按键名称（如"a", "Enter", "ArrowUp"）
     */
    std::string GetKey() const { return key_; }

    /**
     * @brief 获取按键代码
     * @return 按键代码（如"KeyA", "Enter", "ArrowUp"）
     */
    std::string GetCode() const { return code_; }

    /**
     * @brief 获取按键码（已废弃但保留兼容性）
     * @return 按键码
     */
    int GetKeyCode() const { return key_code_; }

    /**
     * @brief 获取字符码（已废弃但保留兼容性）
     * @return 字符码（与keyCode相同）
     */
    int GetCharCode() const { return key_code_; }

    /**
     * @brief 检查Ctrl键是否按下
     * @return true表示Ctrl键按下
     */
    bool GetCtrlKey() const { return ctrl_key_; }

    /**
     * @brief 检查Shift键是否按下
     * @return true表示Shift键按下
     */
    bool GetShiftKey() const { return shift_key_; }

    /**
     * @brief 检查Alt键是否按下
     * @return true表示Alt键按下
     */
    bool GetAltKey() const { return alt_key_; }

    /**
     * @brief 检查Meta键是否按下
     * @return true表示Meta键按下（Windows键/Command键）
     */
    bool GetMetaKey() const { return meta_key_; }

    /**
     * @brief 检查是否是重复按键
     * @return true表示是重复按键（按住不放）
     */
    bool GetRepeat() const { return repeat_; }

    /**
     * @brief 检查修饰键是否按下
     * @param key_arg 修饰键名称（"Control", "Shift", "Alt", "Meta"）
     * @return true表示指定的修饰键按下
     */
    bool GetModifierState(const std::string& key_arg) const;

private:
    std::string key_;       // 按键名称
    std::string code_;      // 按键代码
    int key_code_;          // 按键码（已废弃）
    bool ctrl_key_;         // Ctrl键状态
    bool shift_key_;        // Shift键状态
    bool alt_key_;          // Alt键状态
    bool meta_key_;         // Meta键状态
    bool repeat_;           // 是否重复
};

/**
 * @brief 动画事件类
 *
 * 用于 CSS 动画事件：
 * - animationstart: 动画开始时触发
 * - animationend: 动画结束时触发
 * - animationiteration: 动画迭代时触发（除了最后一次）
 *
 * 参考：
 * - W3C CSS Animations Level 1 - AnimationEvent
 * - MDN Web Docs - AnimationEvent
 */
class AnimationEvent : public Event {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型（animationstart, animationend, animationiteration）
     * @param animation_name 动画名称（@keyframes 规则名称）
     * @param elapsed_time 动画已运行时间（秒）
     * @param pseudo_element 伪元素选择器（如 "::before"，可选）
     */
    AnimationEvent(const std::string& type,
                   const std::string& animation_name,
                   float elapsed_time,
                   const std::string& pseudo_element = "");

    /**
     * @brief 获取动画名称
     * @return 动画名称（@keyframes 规则名称）
     */
    std::string GetAnimationName() const { return animation_name_; }

    /**
     * @brief 获取动画已运行时间
     * @return 已运行时间（秒）
     */
    float GetElapsedTime() const { return elapsed_time_; }

    /**
     * @brief 获取伪元素选择器
     * @return 伪元素选择器（如 "::before"），如果不是伪元素则返回空字符串
     */
    std::string GetPseudoElement() const { return pseudo_element_; }

private:
    std::string animation_name_;    // 动画名称
    float elapsed_time_;            // 已运行时间（秒）
    std::string pseudo_element_;    // 伪元素选择器
};

/**
 * @brief 输入事件类
 *
 * 用于 contenteditable 元素的输入事件：
 * - beforeinput: 输入前触发（可取消）
 * - input: 输入后触发（不可取消）
 *
 * inputType 常见值：
 * - "insertText": 插入文本
 * - "insertLineBreak": 插入换行
 * - "deleteContentBackward": 向后删除（Backspace）
 * - "deleteContentForward": 向前删除（Delete）
 * - "deleteByCut": 剪切删除
 * - "insertFromPaste": 粘贴插入
 * - "formatBold": 加粗格式
 * - "formatItalic": 斜体格式
 * - "formatUnderline": 下划线格式
 *
 * 参考：
 * - W3C UI Events - InputEvent
 * - MDN Web Docs - InputEvent
 */
class InputEvent : public Event {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型（"beforeinput" 或 "input"）
     * @param input_type 输入类型（如 "insertText", "deleteContentBackward"）
     * @param data 输入的数据（如插入的文本），可为空
     * @param is_composing 是否在输入法组合中
     */
    InputEvent(const std::string& type,
               const std::string& input_type,
               const std::string& data = "",
               bool is_composing = false);

    /**
     * @brief 获取输入类型
     * @return 输入类型（如 "insertText", "deleteContentBackward"）
     */
    std::string GetInputType() const { return input_type_; }

    /**
     * @brief 获取输入数据
     * @return 输入的数据（如插入的文本），可能为空
     */
    std::string GetData() const { return data_; }

    /**
     * @brief 检查是否在输入法组合中
     * @return true 表示在输入法组合中
     */
    bool IsComposing() const { return is_composing_; }

private:
    std::string input_type_;    ///< 输入类型
    std::string data_;          ///< 输入数据
    bool is_composing_;         ///< 是否在输入法组合中
};

/**
 * @brief 剪贴板事件类
 *
 * 用于剪贴板操作事件：
 * - copy: 复制时触发
 * - cut: 剪切时触发
 * - paste: 粘贴时触发
 *
 * 参考：
 * - W3C Clipboard API - ClipboardEvent
 * - MDN Web Docs - ClipboardEvent
 */
class ClipboardEvent : public Event {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型（"copy", "cut", "paste"）
     * @param clipboard_data 剪贴板数据（文本内容）
     */
    ClipboardEvent(const std::string& type,
                   const std::string& clipboard_data = "");

    /**
     * @brief 获取剪贴板数据
     * @return 剪贴板中的文本数据
     */
    std::string GetClipboardData() const { return clipboard_data_; }

    /**
     * @brief 设置剪贴板数据
     * @param data 要设置的数据
     */
    void SetClipboardData(const std::string& data) { clipboard_data_ = data; }

private:
    std::string clipboard_data_;    ///< 剪贴板数据
};

} // namespace lightui

