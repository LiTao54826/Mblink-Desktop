/**
 * @file drag_manager.h
 * @brief 拖拽管理器
 *
 * 功能：
 * - 支持 HTML5 标准 draggable 属性
 * - 支持 RmlUi 风格 drag 属性（向后兼容）
 * - 拖拽阈值检测（防止误触发）
 * - 标准 HTML5 拖拽事件分发
 *
 * 参考：
 * - W3C HTML5 - Drag and Drop
 * - RmlUi/Source/Core/Context.cpp - CreateDragClone, UpdateHoverChain
 */

#pragma once

#include <memory>
#include <unordered_set>
#include <string>
#include "core/event/types/data_transfer.h"  // 需要 DragEffect 枚举

namespace lightui {

// 前向声明
class Element;
class Document;
class Event;
class DataTransfer;
class RenderObject;

/**
 * @brief 拖拽模式
 * 
 * 参考：RmlUi的Style::Drag枚举
 */
enum class DragMode {
    None = 0,       // 不可拖拽
    Drag,           // 可拖拽（简单模式，不发送详细事件）
    DragDrop,       // 可拖拽（详细模式，发送 HTML5 标准事件）
    Clone,          // 拖拽时克隆元素
    Block           // 阻止拖拽
};

/**
 * @brief 拖拽检测状态
 */
enum class DragState {
    None,           // 无拖拽
    Detecting,      // 检测中（等待超过阈值）
    Dragging        // 拖拽中
};

/**
 * @brief 拖拽管理器
 * 
 * 负责管理元素拖拽状态，支持：
 * - HTML5 标准 draggable 属性
 * - 拖拽事件（dragstart, drag, dragend, dragenter, dragleave, dragover, drop）
 * - 拖拽克隆（drag: clone）
 * - :drag伪类自动设置
 * - 拖拽hover链管理
 * - 拖拽阈值检测
 * 
 * 参考：
 * - W3C HTML5 - Drag and Drop
 * - RmlUi的拖拽管理机制
 */
class DragManager {
public:
    // 拖拽阈值（像素）- 鼠标移动超过此距离才开始拖拽
    static constexpr float DRAG_THRESHOLD = 4.0f;

    DragManager();
    ~DragManager();

    /**
     * @brief 开始拖拽检测（在mousedown时调用）
     * @param element 被点击的元素
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     * @return true表示可以开始拖拽检测
     */
    bool StartDragDetection(std::shared_ptr<Element> element, float mouse_x, float mouse_y);

    /**
     * @brief 检查是否超过拖拽阈值
     * @param mouse_x 当前鼠标X坐标
     * @param mouse_y 当前鼠标Y坐标
     * @return true表示超过阈值，应该开始拖拽
     */
    bool CheckDragThreshold(float mouse_x, float mouse_y) const;

    /**
     * @brief 更新拖拽状态（在mousemove时调用）
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     * @param document 当前文档
     * @param root_render 渲染树根节点（用于精确 hit testing）
     * @return true表示正在拖拽
     */
    bool UpdateDrag(float mouse_x, float mouse_y, std::shared_ptr<Document> document, 
                    std::shared_ptr<RenderObject> root_render = nullptr);

    /**
     * @brief 结束拖拽（在mouseup时调用）
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     */
    void EndDrag(float mouse_x, float mouse_y);

    /**
     * @brief 取消拖拽
     */
    void CancelDrag();

    /**
     * @brief 获取当前拖拽元素
     * @return 当前拖拽元素，如果没有则返回nullptr
     */
    std::shared_ptr<Element> GetDragElement() const;

    /**
     * @brief 获取当前拖拽悬停元素
     * @return 当前拖拽悬停元素，如果没有则返回nullptr
     */
    std::shared_ptr<Element> GetDragHoverElement() const;

    /**
     * @brief 是否正在拖拽
     * @return true表示正在拖拽
     */
    bool IsDragging() const;

    /**
     * @brief 获取拖拽克隆元素
     * @return 拖拽克隆元素，如果没有则返回nullptr
     */
    std::shared_ptr<Element> GetDragClone() const;

    /**
     * @brief 获取DataTransfer对象
     * @return DataTransfer对象
     */
    std::shared_ptr<DataTransfer> GetDataTransfer() const;

    /**
     * @brief 检查元素是否可拖拽（支持 HTML5 draggable 和 RmlUi drag 属性）
     * @param element 要检查的元素
     * @return true表示元素可拖拽
     *
     * 优先级：
     * 1. draggable 属性（HTML5 标准）
     * 2. drag 属性（RmlUi 风格，向后兼容）
     * 3. 默认行为（基于元素类型）
     */
    bool IsElementDraggable(std::shared_ptr<Element> element) const;

    /**
     * @brief 获取当前拖拽状态
     * @return 拖拽状态
     */
    DragState GetDragState() const { return drag_state_; }

    /**
     * @brief 检查是否正在检测拖拽（等待超过阈值）
     * @return true表示正在检测
     */
    bool IsDetecting() const { return drag_state_ == DragState::Detecting; }

    /**
     * @brief 检查 dropEffect 是否与 effectAllowed 兼容
     * @param effect_allowed 允许的效果
     * @param drop_effect 当前效果
     * @return true表示兼容
     *
     * 参考：W3C HTML5 - Drag and Drop - The drag data store mode
     */
    static bool IsEffectCompatible(DragEffect effect_allowed, DragEffect drop_effect);

    /**
     * @brief 检查是否允许 drop（基于 dragover 的 preventDefault）
     * @return true表示允许 drop
     */
    bool IsDropAllowed() const { return drop_allowed_; }

    /**
     * @brief 设置是否允许 drop
     * @param allowed 是否允许
     */
    void SetDropAllowed(bool allowed) { drop_allowed_ = allowed; }

private:
    /**
     * @brief 查找可拖拽的元素
     * @param element 起始元素
     * @return 可拖拽的元素，如果没有则返回nullptr
     */
    std::shared_ptr<Element> FindDraggableElement(std::shared_ptr<Element> element);

    /**
     * @brief 获取元素的拖拽模式
     * @param element 元素
     * @return 拖拽模式
     *
     * 优先级：
     * 1. draggable 属性（HTML5 标准）
     * 2. drag 属性（RmlUi 风格）
     */
    DragMode GetDragMode(std::shared_ptr<Element> element) const;

    /**
     * @brief 创建拖拽克隆
     * @param element 要克隆的元素
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     */
    void CreateDragClone(std::shared_ptr<Element> element, float mouse_x, float mouse_y);

    /**
     * @brief 释放拖拽克隆
     */
    void ReleaseDragClone();

    /**
     * @brief 更新拖拽hover链
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     * @param document 当前文档
     * @param root_render 渲染树根节点（用于精确 hit testing）
     */
    void UpdateDragHoverChain(float mouse_x, float mouse_y, std::shared_ptr<Document> document,
                              std::shared_ptr<RenderObject> root_render);

    /**
     * @brief 发送拖拽事件到元素集合差集
     * @param old_items 旧集合
     * @param new_items 新集合
     * @param event_type 事件类型
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     */
    void SendDragEvents(const std::unordered_set<Element*>& old_items,
                       const std::unordered_set<Element*>& new_items,
                       const std::string& event_type,
                       float mouse_x,
                       float mouse_y);

private:
    // 当前拖拽元素（弱引用）
    std::weak_ptr<Element> drag_element_;

    // 当前拖拽悬停元素（弱引用）
    std::weak_ptr<Element> drag_hover_element_;

    // 拖拽hover链（原始指针，用于快速查找）
    std::unordered_set<Element*> drag_hover_chain_;

    // 拖拽克隆元素（强引用）
    std::shared_ptr<Element> drag_clone_;

    // 拖拽模式
    DragMode drag_mode_;

    // 拖拽状态
    DragState drag_state_;

    // 是否已开始拖拽（已发送 dragstart）
    bool drag_started_;

    // 是否是详细模式（发送 HTML5 标准事件）
    bool drag_verbose_;

    // 拖拽检测起始位置
    float detect_start_x_;
    float detect_start_y_;

    // 拖拽开始时的鼠标位置
    float drag_start_x_;
    float drag_start_y_;

    // DataTransfer对象
    std::shared_ptr<DataTransfer> data_transfer_;

    // 是否允许 drop（基于 dragover 的 preventDefault）
    bool drop_allowed_;
};

} // namespace lightui

