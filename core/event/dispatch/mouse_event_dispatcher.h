/**
 * @file mouse_event_dispatcher.h
 * @brief 鼠标事件分发器
 *
 * 从 event_loop.cpp 提取的鼠标事件处理逻辑。
 * 负责处理鼠标点击、移动、hover 链管理等。
 *
 * 参考：
 * - W3C UI Events - MouseEvent
 * - RmlUi/Source/Core/Context.cpp - UpdateHoverChain
 */

#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace lightui {

// 前向声明
class Element;
class Document;
class Window;
class RenderObject;
class DragManager;
class SelectionManager;
class ContentEditableHandler;
class FocusManager;
struct HitTestResult;

/**
 * @brief 鼠标事件分发器
 *
 * 负责：
 * 1. 处理鼠标按下/抬起/移动事件
 * 2. 管理 hover 链（mouseover/mouseout 事件）
 * 3. 处理 click/dblclick 事件
 * 4. 更新鼠标光标样式
 * 5. 处理滚动条拖动
 */
class MouseEventDispatcher {
public:
    /**
     * @brief 构造函数
     */
    MouseEventDispatcher();

    /**
     * @brief 析构函数
     */
    ~MouseEventDispatcher();

    /**
     * @brief 设置依赖的管理器
     * @param drag_manager 拖拽管理器
     * @param selection_manager 选择管理器
     * @param contenteditable_handler 可编辑内容处理器
     * @param focus_manager 焦点管理器
     */
    void SetManagers(DragManager* drag_manager,
                     SelectionManager* selection_manager,
                     ContentEditableHandler* contenteditable_handler,
                     FocusManager* focus_manager);

    /**
     * @brief 设置光标更新回调
     * @param callback 光标更新回调函数
     */
    void SetCursorCallback(std::function<void(SDL_SystemCursor)> callback);

    /**
     * @brief 处理鼠标事件
     * @param event SDL 鼠标事件
     * @param window 目标窗口
     * @param document 目标文档
     * @param root_render 渲染树根节点
     * @return true 如果事件被处理
     */
    bool HandleMouseEvent(const SDL_Event& event,
                          std::shared_ptr<Window> window,
                          std::shared_ptr<Document> document,
                          std::shared_ptr<RenderObject> root_render);

    /**
     * @brief 更新 hover 链
     * @param window_id 窗口 ID
     * @param mouse_x 鼠标 X 坐标
     * @param mouse_y 鼠标 Y 坐标
     * @param hit_result Hit Testing 结果
     */
    void UpdateHoverChain(Uint32 window_id,
                          float mouse_x,
                          float mouse_y,
                          const HitTestResult& hit_result);

    /**
     * @brief 获取当前 hover 元素
     * @return 当前 hover 元素
     */
    std::shared_ptr<Element> GetHoverElement() const;

    /**
     * @brief 获取 hover 链
     * @return hover 链（从目标元素到根元素）
     */
    const std::vector<std::weak_ptr<Element>>& GetHoverChain() const;

private:
    /**
     * @brief 发送事件到元素集合的差集
     * @param old_items 旧元素集合
     * @param new_items 新元素集合
     * @param event_type 事件类型
     * @param mouse_x 鼠标 X 坐标
     * @param mouse_y 鼠标 Y 坐标
     * @return 是否有伪类变化
     */
    bool SendEvents(const std::vector<std::weak_ptr<Element>>& old_items,
                    const std::vector<std::weak_ptr<Element>>& new_items,
                    const std::string& event_type,
                    float mouse_x,
                    float mouse_y);

    /**
     * @brief 更新鼠标光标样式
     * @param hit_result Hit Testing 结果
     * @param window_id 窗口 ID
     */
    void UpdateMouseCursor(const HitTestResult& hit_result, Uint32 window_id);

    /**
     * @brief 将 SDL 鼠标按钮转换为鼠标按钮编号
     * @param sdl_button SDL 鼠标按钮
     * @return 鼠标按钮编号 (0=无, 1=左, 2=中, 3=右)
     */
    static int SDLButtonToMouseButton(Uint8 sdl_button);

private:
    // 依赖的管理器（不拥有所有权）
    DragManager* drag_manager_ = nullptr;
    SelectionManager* selection_manager_ = nullptr;
    ContentEditableHandler* contenteditable_handler_ = nullptr;
    FocusManager* focus_manager_ = nullptr;

    // 光标更新回调
    std::function<void(SDL_SystemCursor)> cursor_callback_;

    // Hover 链追踪
    std::vector<std::weak_ptr<Element>> hover_chain_;
    std::weak_ptr<Element> hover_element_;

    // 点击状态追踪（用于 click/dblclick）
    std::weak_ptr<Element> last_mousedown_element_;
    std::weak_ptr<Element> last_click_element_;
    Uint64 last_click_time_ = 0;
    static constexpr Uint64 DOUBLE_CLICK_TIME_MS = 500;

    // 滚动条拖动状态
    std::weak_ptr<RenderObject> scrollbar_dragging_element_;
    Uint32 scrollbar_dragging_window_id_ = 0;
};

} // namespace lightui
