/**
 * @file wheel_event_dispatcher.h
 * @brief 滚轮事件分发器
 *
 * 从 event_loop.cpp 提取的滚轮事件处理逻辑。
 * 负责处理鼠标滚轮事件，包括垂直和水平滚动。
 */

#pragma once

#include <SDL3/SDL.h>
#include <memory>

namespace mblink {

// 前向声明
class Document;
class Element;
class Window;
class RenderObject;

/**
 * @brief 滚轮事件分发器
 *
 * 负责：
 * 1. 处理鼠标滚轮事件
 * 2. 查找可滚动的元素
 * 3. 处理 textarea 元素的滚动
 * 4. 处理普通元素的滚动
 */
class WheelEventDispatcher {
public:
    /**
     * @brief 构造函数
     */
    WheelEventDispatcher();

    /**
     * @brief 析构函数
     */
    ~WheelEventDispatcher();

    /**
     * @brief 处理滚轮事件
     * @param event SDL 滚轮事件
     * @param window 目标窗口
     * @param document 目标文档
     * @return true 如果事件被处理
     */
    bool HandleWheelEvent(const SDL_Event& event,
                          std::shared_ptr<Window> window,
                          std::shared_ptr<Document> document);

private:
    /**
     * @brief 处理 terminal 元素的滚轮事件
     * @return true 如果事件被处理
     */
    bool HandleTerminalWheel(std::shared_ptr<Window> window,
                             std::shared_ptr<Element> element,
                             float wheel_x, float wheel_y,
                             bool shift_pressed);

    /**
     * @brief 处理 logview 元素的滚轮事件
     * @return true 如果事件被处理
     */
    bool HandleLogViewWheel(std::shared_ptr<Window> window,
                            std::shared_ptr<Element> element,
                            float wheel_x, float wheel_y,
                            bool shift_pressed);

    /**
     * @brief 处理 textarea 元素的滚轮事件
     * @return true 如果事件被处理
     */
    bool HandleTextAreaWheel(std::shared_ptr<Window> window,
                             std::shared_ptr<Element> element,
                             std::shared_ptr<RenderObject> render_object,
                             float wheel_x, float wheel_y,
                             bool shift_pressed);

    /**
     * @brief 处理普通可滚动元素的滚轮事件
     * @return true 如果事件被处理
     */
    bool HandleScrollableElementWheel(std::shared_ptr<Window> window,
                                      std::shared_ptr<RenderObject> render_obj,
                                      float logical_x, float logical_y,
                                      float wheel_x, float wheel_y,
                                      bool shift_pressed);
};

} // namespace mblink
