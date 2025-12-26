/**
 * @file drag_event.h
 * @brief HTML5 拖拽事件类
 *
 * 功能：
 * - 实现 W3C HTML5 DragEvent 接口
 * - 继承自 MouseEvent，添加 dataTransfer 属性
 * - 支持标准拖拽事件类型
 *
 * 参考：
 * - W3C HTML5 - Drag and Drop
 * - MDN Web Docs - DragEvent
 */

#pragma once

#include "event.h"
#include <memory>
#include <string>

namespace lightui {

// 前向声明
class DataTransfer;

/**
 * @brief HTML5 拖拽事件类
 *
 * 继承自 MouseEvent，添加 dataTransfer 属性用于在拖拽操作中传输数据。
 *
 * 支持的事件类型：
 * - dragstart: 拖拽开始时触发（在源元素上）
 * - drag: 拖拽过程中持续触发（在源元素上）
 * - dragend: 拖拽结束时触发（在源元素上）
 * - dragenter: 拖拽进入目标元素时触发
 * - dragleave: 拖拽离开目标元素时触发
 * - dragover: 拖拽在目标元素上方时持续触发
 * - drop: 在目标元素上释放时触发
 *
 * 参考：
 * - https://html.spec.whatwg.org/multipage/dnd.html#the-dragevent-interface
 * - https://developer.mozilla.org/en-US/docs/Web/API/DragEvent
 */
class DragEvent : public MouseEvent {
public:
    // ========== 事件类型常量 ==========
    static const std::string DRAG_START;   // "dragstart"
    static const std::string DRAG;         // "drag"
    static const std::string DRAG_END;     // "dragend"
    static const std::string DRAG_ENTER;   // "dragenter"
    static const std::string DRAG_LEAVE;   // "dragleave"
    static const std::string DRAG_OVER;    // "dragover"
    static const std::string DROP;         // "drop"

    /**
     * @brief 构造函数
     * @param type 事件类型（dragstart, drag, dragend, dragenter, dragleave, dragover, drop）
     * @param client_x 鼠标 X 坐标（相对于视口）
     * @param client_y 鼠标 Y 坐标（相对于视口）
     * @param button 鼠标按钮（0=左键, 1=中键, 2=右键）
     * @param data_transfer DataTransfer 对象
     * @param ctrl_key Ctrl 键是否按下
     * @param shift_key Shift 键是否按下
     * @param alt_key Alt 键是否按下
     * @param meta_key Meta 键是否按下
     */
    DragEvent(const std::string& type,
              int client_x,
              int client_y,
              int button,
              std::shared_ptr<DataTransfer> data_transfer,
              bool ctrl_key = false,
              bool shift_key = false,
              bool alt_key = false,
              bool meta_key = false);

    /**
     * @brief 析构函数
     */
    ~DragEvent() override = default;

    // ========== DataTransfer 访问 ==========

    /**
     * @brief 获取 DataTransfer 对象
     * @return DataTransfer 对象，用于在拖拽操作中传输数据
     *
     * 示例：
     * auto dt = event->GetDataTransfer();
     * dt->SetData("text/plain", "Hello World");
     */
    std::shared_ptr<DataTransfer> GetDataTransfer() const { return data_transfer_; }

    // ========== 修饰键状态 ==========

    /**
     * @brief 检查 Ctrl 键是否按下
     * @return true 表示 Ctrl 键按下
     */
    bool GetCtrlKey() const { return ctrl_key_; }

    /**
     * @brief 检查 Shift 键是否按下
     * @return true 表示 Shift 键按下
     */
    bool GetShiftKey() const { return shift_key_; }

    /**
     * @brief 检查 Alt 键是否按下
     * @return true 表示 Alt 键按下
     */
    bool GetAltKey() const { return alt_key_; }

    /**
     * @brief 检查 Meta 键是否按下
     * @return true 表示 Meta 键按下（Windows 键/Command 键）
     */
    bool GetMetaKey() const { return meta_key_; }

    // ========== 屏幕坐标 ==========

    /**
     * @brief 获取屏幕 X 坐标
     * @return 鼠标相对于屏幕的 X 坐标
     */
    int GetScreenX() const { return screen_x_; }

    /**
     * @brief 获取屏幕 Y 坐标
     * @return 鼠标相对于屏幕的 Y 坐标
     */
    int GetScreenY() const { return screen_y_; }

    /**
     * @brief 设置屏幕坐标
     * @param x 屏幕 X 坐标
     * @param y 屏幕 Y 坐标
     */
    void SetScreenPosition(int x, int y);

private:
    std::shared_ptr<DataTransfer> data_transfer_;  // DataTransfer 对象

    // 修饰键状态
    bool ctrl_key_ = false;
    bool shift_key_ = false;
    bool alt_key_ = false;
    bool meta_key_ = false;

    // 屏幕坐标
    int screen_x_ = 0;
    int screen_y_ = 0;
};

} // namespace lightui
