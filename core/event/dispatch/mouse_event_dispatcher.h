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

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace lightui {

// 前向声明
class Element;
class Node;
class Text;
class Document;
class Window;
class RenderObject;
class DragManager;
class SelectionManager;
class ContentEditableHandler;
class FocusManager;
class HTMLInputElement;
class HTMLTextAreaElement;
class HTMLButtonElement;
class HTMLFormElement;
class HTMLSelectElement;
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
     * @brief 更新 hover 链（使用已计算的 HitTestResult）
     * @param window 目标窗口
     * @param mouse_x 鼠标 X 坐标
     * @param mouse_y 鼠标 Y 坐标
     * @param hit_result Hit Testing 结果
     */
    void UpdateHoverChain(std::shared_ptr<Window> window,
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

    /**
     * @brief 处理输入框的鼠标交互（点击定位光标、拖动选择）
     * @param input_element 输入元素
     * @param local_x 相对于输入框内容区域的X坐标
     * @param event_type 事件类型
     * @param font_size 字体大小
     * @param font_family 字体族
     */
    void HandleInputMouseInteraction(std::shared_ptr<HTMLInputElement> input_element,
                                     float local_x,
                                     Uint32 event_type,
                                     float font_size,
                                     const std::string& font_family);

    /**
     * @brief 处理 TextArea 元素的鼠标交互
     * @param textarea_element TextArea 元素
     * @param local_x 相对于文本内容区域的 X 坐标
     * @param local_y 相对于文本内容区域的 Y 坐标
     * @param event_type 事件类型
     * @param font_size 字体大小
     * @param font_family 字体族
     * @param shift_key 是否按住Shift键
     * @param visible_width 可见区域宽度
     * @param visible_height 可见区域高度
     */
    void HandleTextAreaMouseInteraction(std::shared_ptr<HTMLTextAreaElement> textarea_element,
                                        float local_x,
                                        float local_y,
                                        Uint32 event_type,
                                        float font_size,
                                        const std::string& font_family,
                                        bool shift_key = false,
                                        float visible_width = 0.0f,
                                        float visible_height = 0.0f);

    /**
     * @brief 获取正在拖动滚动条的元素
     * @return 正在拖动滚动条的 RenderObject
     */
    std::shared_ptr<RenderObject> GetScrollbarDraggingElement() const;

    /**
     * @brief 设置正在拖动滚动条的元素
     * @param element RenderObject
     * @param window_id 窗口 ID
     */
    void SetScrollbarDraggingElement(std::shared_ptr<RenderObject> element, Uint32 window_id);

    /**
     * @brief 清除滚动条拖动状态
     */
    void ClearScrollbarDragging();

    /**
     * @brief 获取滚动条拖动所在窗口 ID
     * @return 窗口 ID
     */
    Uint32 GetScrollbarDraggingWindowId() const { return scrollbar_dragging_window_id_; }

    /**
     * @brief 处理表单元素的默认行为
     * @param element 被点击的元素
     * @param hit_result Hit Testing 结果
     */
    void ProcessFormElementDefaultAction(std::shared_ptr<Element> element, const HitTestResult& hit_result);

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

    /**
     * @brief 取消同组 radio 的选中状态
     * @param node 起始节点
     * @param group_name radio 组名
     * @param except 排除的元素
     */
    void UncheckRadioGroup(const std::shared_ptr<Node>& node,
                           const std::string& group_name,
                           const std::shared_ptr<HTMLInputElement>& except);

    /**
     * @brief 查找父级表单元素
     * @param element 起始元素
     * @return 父级表单元素
     */
    std::shared_ptr<HTMLFormElement> FindParentForm(std::shared_ptr<Element> element);

    /**
     * @brief 处理 contentEditable 拖动选择
     * @param window 窗口
     * @param logical_x 逻辑 X 坐标
     * @param logical_y 逻辑 Y 坐标
     * @param event_type 事件类型
     */
    void HandleContentEditableDragSelection(std::shared_ptr<Window> window,
                                            float logical_x,
                                            float logical_y,
                                            Uint32 event_type);

    /**
     * @brief 处理没有命中元素时的 mouseup 事件
     */
    void HandleNoHitMouseUp(std::shared_ptr<Window> window,
                            std::shared_ptr<Element> last_mousedown,
                            const SDL_Event& event,
                            float logical_x,
                            float logical_y);

    /**
     * @brief 处理没有命中元素时的 mousemove 事件
     */
    void HandleNoHitMouseMotion(std::shared_ptr<Window> window,
                                std::shared_ptr<Element> last_mousedown,
                                const SDL_Event& event,
                                float logical_x,
                                float logical_y);

    /**
     * @brief 处理 Range 滑块拖动
     */
    void HandleRangeDrag(std::shared_ptr<Window> window,
                         std::shared_ptr<HTMLInputElement> input_element,
                         float logical_x,
                         std::shared_ptr<RenderObject> root_render);

    /**
     * @brief 处理 mousedown 事件
     */
    void HandleMouseDown(std::shared_ptr<Window> window,
                         std::shared_ptr<Document> document,
                         const HitTestResult& hit_result,
                         const SDL_Event& event,
                         float logical_x,
                         float logical_y,
                         int button,
                         std::shared_ptr<RenderObject> root_render);

    /**
     * @brief 处理 mouseup 事件
     */
    void HandleMouseUp(std::shared_ptr<Window> window,
                       std::shared_ptr<Document> document,
                       const HitTestResult& hit_result,
                       const SDL_Event& event,
                       float logical_x,
                       float logical_y,
                       int button,
                       std::shared_ptr<RenderObject> root_render);

    /**
     * @brief 处理 mousemove 事件
     */
    void HandleMouseMove(std::shared_ptr<Window> window,
                         std::shared_ptr<Document> document,
                         const HitTestResult& hit_result,
                         float logical_x,
                         float logical_y,
                         std::shared_ptr<RenderObject> root_render);

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

    // contentEditable 拖动选择状态
    bool contenteditable_dragging_ = false;
    std::weak_ptr<Node> contenteditable_drag_start_node_;
    int contenteditable_drag_start_offset_ = 0;
};

} // namespace lightui
