/**
 * @file drag_manager.h
 * @brief 拖拽管理器
 *
 * 参考：RmlUi/Source/Core/Context.cpp - CreateDragClone, UpdateHoverChain
 */

#pragma once

#include <memory>
#include <unordered_set>
#include <string>

namespace lightui {

// 前向声明
class Element;
class Document;
class Event;
class DataTransfer;

/**
 * @brief 拖拽模式
 * 
 * 参考：RmlUi的Style::Drag枚举
 */
enum class DragMode {
    None = 0,       // 不可拖拽
    Drag,           // 可拖拽（简单模式，不发送详细事件）
    DragDrop,       // 可拖拽（详细模式，发送dragover/dragout/dragdrop事件）
    Clone,          // 拖拽时克隆元素
    Block           // 阻止拖拽
};

/**
 * @brief 拖拽管理器
 * 
 * 负责管理元素拖拽状态，支持：
 * - 拖拽事件（dragstart, drag, dragend, dragover, dragout, dragdrop）
 * - 拖拽克隆（drag: clone）
 * - :drag伪类自动设置
 * - 拖拽hover链管理
 * 
 * 参考：RmlUi的拖拽管理机制
 */
class DragManager {
public:
    DragManager();
    ~DragManager();

    /**
     * @brief 开始拖拽检测（在mousedown时调用）
     * @param element 被点击的元素
     * @return true表示可以开始拖拽
     */
    bool StartDragDetection(std::shared_ptr<Element> element);

    /**
     * @brief 更新拖拽状态（在mousemove时调用）
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     * @param document 当前文档
     * @return true表示正在拖拽
     */
    bool UpdateDrag(float mouse_x, float mouse_y, std::shared_ptr<Document> document);

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
     */
    DragMode GetDragMode(std::shared_ptr<Element> element);

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
     */
    void UpdateDragHoverChain(float mouse_x, float mouse_y, std::shared_ptr<Document> document);

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

    // 是否已开始拖拽
    bool drag_started_;

    // 是否是详细模式（发送dragover/dragout/dragdrop事件）
    bool drag_verbose_;

    // 拖拽开始时的鼠标位置
    float drag_start_x_;
    float drag_start_y_;

    // DataTransfer对象
    std::shared_ptr<DataTransfer> data_transfer_;
};

} // namespace lightui

