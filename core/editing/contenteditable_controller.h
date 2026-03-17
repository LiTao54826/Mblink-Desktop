/**
 * @file contenteditable_controller.h
 * @brief ContentEditable 控制器 - 协调 contentEditable 相关的所有事件处理
 *
 * 功能：
 * - 处理 contentEditable 元素的鼠标事件（mousedown, mousemove, mouseup）
 * - 管理拖拽选择状态
 * - 协调 SelectionManager 和 ContentEditableHandler
 * - 提供 GetContentEditableRoot 辅助函数
 */

#pragma once

#include <memory>
#include <string>

namespace lightui {

// 前向声明
class Document;
class Element;
class Node;
struct ContentEditableResolvedPosition;
class SelectionManager;
class ContentEditableHandler;
class RenderObject;
class Window;

/**
 * @brief 拖拽选择状态
 */
struct DragSelectionState {
    bool is_active = false;
    std::shared_ptr<Element> editable_root;
    std::shared_ptr<Node> start_node;
    int start_offset = 0;
    
    void Reset() {
        is_active = false;
        editable_root = nullptr;
        start_node = nullptr;
        start_offset = 0;
    }
};

/**
 * @brief ContentEditable 控制器
 *
 * 负责协调 contentEditable 相关的所有事件处理，
 * 集中管理拖拽选择状态，避免分散的 static 变量。
 */
class ContentEditableController {
public:
    /**
     * @brief 构造函数
     * @param selection_manager 选择管理器
     * @param editable_handler 可编辑内容处理器
     */
    ContentEditableController(SelectionManager* selection_manager,
                              ContentEditableHandler* editable_handler);

    /**
     * @brief 析构函数
     */
    ~ContentEditableController();

    // ========== 鼠标事件处理 ==========

    /**
     * @brief 处理鼠标按下事件
     * @param target 目标元素
     * @param x 鼠标 X 坐标（逻辑坐标）
     * @param y 鼠标 Y 坐标（逻辑坐标）
     * @param shift_key 是否按下 Shift 键
     * @param render_object 目标元素的渲染对象
     * @param root_render 渲染树根节点
     * @return true 如果事件被处理
     */
    bool HandleMouseDown(
        std::shared_ptr<Element> target,
        float x, float y,
        bool shift_key,
        std::shared_ptr<RenderObject> render_object,
        std::shared_ptr<RenderObject> root_render
    );

    /**
     * @brief 处理鼠标移动事件
     * @param document 文档
     * @param x 鼠标 X 坐标（逻辑坐标）
     * @param y 鼠标 Y 坐标（逻辑坐标）
     * @param root_render 渲染树根节点
     * @param window 窗口对象
     * @return true 如果事件被处理
     */
    bool HandleMouseMove(
        std::shared_ptr<Document> document,
        float x, float y,
        std::shared_ptr<RenderObject> root_render,
        Window* window
    );

    /**
     * @brief 处理鼠标释放事件
     * @param target 目标元素
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @return true 如果事件被处理
     */
    bool HandleMouseUp(
        std::shared_ptr<Element> target,
        float x, float y
    );

    // ========== 键盘事件处理 ==========

    /**
     * @brief 处理键盘按下事件
     * @param target 目标元素
     * @param key_code 键码
     * @param ctrl_key 是否按下 Ctrl
     * @param shift_key 是否按下 Shift
     * @param alt_key 是否按下 Alt
     * @return true 如果事件被处理
     */
    bool HandleKeyDown(
        std::shared_ptr<Element> target,
        int key_code,
        bool ctrl_key,
        bool shift_key,
        bool alt_key
    );

    /**
     * @brief 处理文本输入
     * @param target 目标元素
     * @param text 输入的文本
     * @return true 如果输入被处理
     */
    bool HandleTextInput(
        std::shared_ptr<Element> target,
        const std::string& text
    );

    // ========== 状态查询 ==========

    /**
     * @brief 检查是否正在拖拽选择
     * @return true 如果正在拖拽选择
     */
    bool IsDragging() const { return drag_state_.is_active; }

    /**
     * @brief 获取拖拽选择状态
     * @return 拖拽选择状态
     */
    const DragSelectionState& GetDragState() const { return drag_state_; }

    /**
     * @brief 获取最后一次 mousedown 的元素
     * @return 最后一次 mousedown 的元素
     */
    std::shared_ptr<Element> GetLastMouseDownElement() const { return last_mousedown_element_; }

    /**
     * @brief 设置最后一次 mousedown 的元素
     * @param element 元素
     */
    void SetLastMouseDownElement(std::shared_ptr<Element> element) { last_mousedown_element_ = element; }

    // ========== 辅助方法 ==========

    /**
     * @brief 获取 contentEditable 根元素
     * @param node 起始节点
     * @return contentEditable 根元素，如果没有返回 nullptr
     */
    static std::shared_ptr<Element> GetContentEditableRoot(std::shared_ptr<Node> node);

    static ContentEditableResolvedPosition ResolveFallbackCaretPosition(
        const std::shared_ptr<Element>& contenteditable_root,
        const std::shared_ptr<Node>& fallback_node,
        float click_x,
        float host_width);

private:
    /**
     * @brief 查找文本节点在指定位置
     * @param render_obj 渲染对象
     * @param click_x 点击 X 坐标（相对于渲染对象）
     * @param click_y 点击 Y 坐标（相对于渲染对象）
     * @param out_node 输出：找到的文本节点
     * @param out_offset 输出：文本偏移量
     * @return true 如果找到文本节点
     */
    bool FindTextNodeAtPosition(
        std::shared_ptr<RenderObject> render_obj,
        float click_x, float click_y,
        std::shared_ptr<Node>& out_node,
        int& out_offset
    );

    /**
     * @brief 查找 contentEditable 根元素的渲染对象
     * @param root_render 渲染树根节点
     * @param contenteditable_root contentEditable 根元素
     * @param out_abs_x 输出：绝对 X 坐标
     * @param out_abs_y 输出：绝对 Y 坐标
     * @return 渲染对象，如果没有返回 nullptr
     */
    std::shared_ptr<RenderObject> FindContentEditableRenderObject(
        std::shared_ptr<RenderObject> root_render,
        std::shared_ptr<Element> contenteditable_root,
        float& out_abs_x,
        float& out_abs_y
    );

private:
    SelectionManager* selection_manager_;
    ContentEditableHandler* editable_handler_;
    
    // 拖拽选择状态
    DragSelectionState drag_state_;
    
    // 最后一次 mousedown 的元素
    std::shared_ptr<Element> last_mousedown_element_;
};

} // namespace lightui
