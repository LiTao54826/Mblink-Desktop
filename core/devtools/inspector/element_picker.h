/**
 * @file element_picker.h
 * @brief 元素拾取工具
 */

#pragma once

#include <memory>
#include "core/dom/document.h"
#include "core/dom/element.h"

class SkCanvas;

namespace lightui {

class Event;
class RenderObject;

/**
 * @brief 元素拾取工具
 */
class ElementPicker {
public:
    explicit ElementPicker(Document* document);
    ~ElementPicker();

    /**
     * @brief 启动拾取模式
     */
    void Start();

    /**
     * @brief 停止拾取模式
     */
    void Stop();

    /**
     * @brief 检查是否处于拾取模式
     */
    bool IsActive() const { return active_; }

    /**
     * @brief 处理事件
     * @return 是否消费了事件
     */
    bool HandleEvent(const Event& event);

    /**
     * @brief 处理鼠标移动
     */
    void HandleMouseMove(int x, int y);

    /**
     * @brief 处理鼠标点击
     * @return 是否拾取了元素
     */
    bool HandleMouseClick(int x, int y);

    /**
     * @brief 获取拾取的元素
     */
    std::shared_ptr<Element> GetPickedElement() const { return picked_element_; }

    /**
     * @brief 获取当前悬停的元素
     */
    std::shared_ptr<Element> GetHoveredElement() const { return hovered_element_; }

    /**
     * @brief 设置当前悬停的元素（用于event_loop调用）
     */
    void SetHoverElement(std::shared_ptr<Element> element) { 
        hovered_element_ = element; 
        hovered_render_object_.reset();  // 清除旧的render_object
    }
    
    /**
     * @brief 设置当前悬停的元素和对应的RenderObject
     */
    void SetHoverElement(std::shared_ptr<Element> element, std::shared_ptr<RenderObject> render_obj);

    /**
     * @brief 渲染悬停高亮
     */
    void RenderHoverHighlight(SkCanvas* canvas);

private:
    Document* document_;
    bool active_ = false;

    std::shared_ptr<Element> hovered_element_;
    std::shared_ptr<RenderObject> hovered_render_object_;  // HitTest返回的RenderObject
    std::shared_ptr<Element> picked_element_;

    int last_mouse_x_ = 0;
    int last_mouse_y_ = 0;

    /**
     * @brief 命中测试 - 找到指定坐标下的元素
     */
    std::shared_ptr<Element> HitTest(int x, int y);
};

} // namespace lightui
