/**
 * @file input_handler.h
 * @brief 输入事件处理器
 * 
 * 负责处理鼠标、键盘和触摸输入事件
 */

#pragma once

#include <SDL3/SDL.h>
#include <functional>
#include <string>

namespace mblink {

/**
 * @brief 鼠标事件类型
 */
enum class MouseEventType {
    MOVE,       // 鼠标移动
    DOWN,       // 鼠标按下
    UP,         // 鼠标释放
    WHEEL,      // 鼠标滚轮
    ENTER,      // 鼠标进入窗口
    LEAVE       // 鼠标离开窗口
};

/**
 * @brief 鼠标按钮
 */
enum class MouseButton {
    LEFT = 1,
    MIDDLE = 2,
    RIGHT = 3,
    X1 = 4,
    X2 = 5
};

/**
 * @brief 输入处理器的鼠标事件（内部使用）
 * 注意：与 DOM MouseEvent 不同，这是用于输入处理器的内部事件
 */
struct InputMouseEvent {
    MouseEventType type;    // 事件类型
    int x;                  // X 坐标
    int y;                  // Y 坐标
    int button;             // 按钮（0=无，1=左，2=中，3=右）
    int wheel_x;            // 滚轮 X 方向
    int wheel_y;            // 滚轮 Y 方向
    Uint32 window_id;       // 窗口 ID
};

/**
 * @brief 键盘事件类型
 */
enum class KeyEventType {
    DOWN,       // 按键按下
    UP,         // 按键释放
    TEXT        // 文本输入
};

/**
 * @brief 键盘事件
 */
struct KeyEvent {
    KeyEventType type;      // 事件类型
    SDL_Keycode key;        // 按键代码
    SDL_Scancode scancode;  // 扫描码
    std::string text;       // 文本输入（仅 TEXT 类型）
    bool ctrl;              // Ctrl 键是否按下
    bool shift;             // Shift 键是否按下
    bool alt;               // Alt 键是否按下
    bool repeat;            // 是否是重复按键
    Uint32 window_id;       // 窗口 ID
};

/**
 * @brief 输入处理器类
 * 
 * 功能：
 * 1. 处理 SDL 输入事件
 * 2. 转换为统一的输入事件格式
 * 3. 分发到回调函数
 */
class InputHandler {
public:
    /**
     * @brief 构造函数
     */
    InputHandler();
    
    /**
     * @brief 析构函数
     */
    ~InputHandler() = default;

    /**
     * @brief 处理 SDL 事件
     * 
     * @param event SDL 事件
     * @return true 如果事件被处理
     */
    bool HandleSDLEvent(const SDL_Event& event);
    
    /**
     * @brief 设置鼠标事件回调
     *
     * @param callback 鼠标事件回调函数
     */
    void SetMouseCallback(std::function<void(const InputMouseEvent&)> callback);
    
    /**
     * @brief 设置键盘事件回调
     * 
     * @param callback 键盘事件回调函数
     */
    void SetKeyboardCallback(std::function<void(const KeyEvent&)> callback);
    
    /**
     * @brief 获取鼠标位置
     * 
     * @param x 输出 X 坐标
     * @param y 输出 Y 坐标
     */
    void GetMousePosition(int* x, int* y) const;
    
    /**
     * @brief 检查鼠标按钮是否按下
     * 
     * @param button 鼠标按钮
     * @return true 如果按下
     */
    bool IsMouseButtonDown(MouseButton button) const;
    
    /**
     * @brief 检查按键是否按下
     * 
     * @param key 按键代码
     * @return true 如果按下
     */
    bool IsKeyDown(SDL_Keycode key) const;
    
    /**
     * @brief 检查扫描码是否按下
     * 
     * @param scancode 扫描码
     * @return true 如果按下
     */
    bool IsScancodeDown(SDL_Scancode scancode) const;

private:
    /**
     * @brief 处理鼠标事件
     * 
     * @param event SDL 事件
     * @return true 如果事件被处理
     */
    bool HandleMouseEvent(const SDL_Event& event);
    
    /**
     * @brief 处理键盘事件
     * 
     * @param event SDL 事件
     * @return true 如果事件被处理
     */
    bool HandleKeyboardEvent(const SDL_Event& event);
    
    /**
     * @brief 获取修饰键状态
     * 
     * @param ctrl 输出 Ctrl 键状态
     * @param shift 输出 Shift 键状态
     * @param alt 输出 Alt 键状态
     */
    void GetModifierKeys(bool& ctrl, bool& shift, bool& alt) const;

private:
    std::function<void(const InputMouseEvent&)> mouse_callback_;
    std::function<void(const KeyEvent&)> keyboard_callback_;
    
    int mouse_x_ = 0;
    int mouse_y_ = 0;
    Uint32 mouse_button_state_ = 0;
    const bool* keyboard_state_ = nullptr;
};

} // namespace mblink

