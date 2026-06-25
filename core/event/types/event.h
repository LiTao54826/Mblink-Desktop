/**
 * @file event.h
 * @brief 事件基类
 */

#pragma once

#include <string>
#include <memory>

namespace mblink {

class Element;

/**
 * @brief 事件基类
 */
class Event {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型
     */
    explicit Event(const std::string& type)
        : type_(type)
        , bubbles_(true)
        , cancelable_(true)
        , default_prevented_(false)
        , propagation_stopped_(false) {
    }

    virtual ~Event() = default;

    /**
     * @brief 获取事件类型
     */
    const std::string& GetType() const { return type_; }

    /**
     * @brief 获取目标元素
     */
    std::shared_ptr<Element> GetTarget() const { return target_; }

    /**
     * @brief 设置目标元素
     */
    void SetTarget(std::shared_ptr<Element> target) { target_ = target; }

    /**
     * @brief 获取当前目标元素
     */
    std::shared_ptr<Element> GetCurrentTarget() const { return current_target_; }

    /**
     * @brief 设置当前目标元素
     */
    void SetCurrentTarget(std::shared_ptr<Element> target) { current_target_ = target; }

    /**
     * @brief 是否冒泡
     */
    bool GetBubbles() const { return bubbles_; }

    /**
     * @brief 是否可取消
     */
    bool GetCancelable() const { return cancelable_; }

    /**
     * @brief 阻止默认行为
     */
    void PreventDefault() {
        if (cancelable_) {
            default_prevented_ = true;
        }
    }

    /**
     * @brief 是否已阻止默认行为
     */
    bool IsDefaultPrevented() const { return default_prevented_; }

    /**
     * @brief 停止传播
     */
    void StopPropagation() { propagation_stopped_ = true; }

    /**
     * @brief 是否已停止传播
     */
    bool IsPropagationStopped() const { return propagation_stopped_; }

private:
    std::string type_;
    std::shared_ptr<Element> target_;
    std::shared_ptr<Element> current_target_;
    bool bubbles_;
    bool cancelable_;
    bool default_prevented_;
    bool propagation_stopped_;
};

} // namespace mblink
