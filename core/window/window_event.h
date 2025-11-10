/**
 * @file window_event.h
 * @brief 窗口事件类
 * 
 * 功能：
 * - 表示窗口事件（resize, move, focus, blur, close等）
 * - 提供事件数据访问
 */

#pragma once

#include <string>
#include <memory>

namespace lightui {

/**
 * @brief 窗口事件类型枚举
 */
enum class WindowEventType {
    NONE = 0,
    RESIZE,         // 窗口大小改变
    MOVE,           // 窗口位置改变
    FOCUS,          // 窗口获得焦点
    BLUR,           // 窗口失去焦点
    MINIMIZE,       // 窗口最小化
    MAXIMIZE,       // 窗口最大化
    RESTORE,        // 窗口恢复
    CLOSE,          // 窗口关闭请求
    SHOWN,          // 窗口显示
    HIDDEN,         // 窗口隐藏
    EXPOSED,        // 窗口需要重绘
    ENTER,          // 鼠标进入窗口
    LEAVE           // 鼠标离开窗口
};

/**
 * @brief 窗口事件类
 */
class WindowEvent {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型
     */
    explicit WindowEvent(WindowEventType type)
        : type_(type)
        , data1_(0)
        , data2_(0) {}
    
    /**
     * @brief 构造函数（带数据）
     * @param type 事件类型
     * @param data1 数据1（如宽度、X坐标）
     * @param data2 数据2（如高度、Y坐标）
     */
    WindowEvent(WindowEventType type, int data1, int data2)
        : type_(type)
        , data1_(data1)
        , data2_(data2) {}
    
    /**
     * @brief 析构函数
     */
    virtual ~WindowEvent() = default;
    
    /**
     * @brief 获取事件类型
     * @return 事件类型
     */
    WindowEventType GetType() const { return type_; }
    
    /**
     * @brief 获取数据1
     * @return 数据1
     */
    int GetData1() const { return data1_; }
    
    /**
     * @brief 获取数据2
     * @return 数据2
     */
    int GetData2() const { return data2_; }
    
    /**
     * @brief 获取事件类型名称
     * @return 事件类型名称
     */
    std::string GetTypeName() const {
        switch (type_) {
            case WindowEventType::RESIZE: return "resize";
            case WindowEventType::MOVE: return "move";
            case WindowEventType::FOCUS: return "focus";
            case WindowEventType::BLUR: return "blur";
            case WindowEventType::MINIMIZE: return "minimize";
            case WindowEventType::MAXIMIZE: return "maximize";
            case WindowEventType::RESTORE: return "restore";
            case WindowEventType::CLOSE: return "close";
            case WindowEventType::SHOWN: return "shown";
            case WindowEventType::HIDDEN: return "hidden";
            case WindowEventType::EXPOSED: return "exposed";
            case WindowEventType::ENTER: return "enter";
            case WindowEventType::LEAVE: return "leave";
            default: return "none";
        }
    }

private:
    WindowEventType type_;
    int data1_;
    int data2_;
};

} // namespace lightui

// Hash function for WindowEventType to use in unordered_map
namespace std {
    template<>
    struct hash<lightui::WindowEventType> {
        size_t operator()(const lightui::WindowEventType& type) const {
            return static_cast<size_t>(type);
        }
    };
}

