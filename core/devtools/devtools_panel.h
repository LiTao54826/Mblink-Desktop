/**
 * @file devtools_panel.h
 * @brief DevTools 面板容器
 */

#pragma once

#include <memory>
#include <functional>
#include "core/dom/document.h"
#include "core/dom/element.h"

class SkCanvas;

namespace mblink {

class DOMTreeView;
class StylesPanel;
class ElementSearch;
class Event;

/**
 * @brief DevTools 面板容器
 */
class DevToolsPanel {
public:
    explicit DevToolsPanel(Document* document);
    ~DevToolsPanel();

    /**
     * @brief 刷新面板内容
     */
    void Refresh();

    /**
     * @brief 刷新 DOM 树视图
     */
    void RefreshDOMTree();

    /**
     * @brief 刷新属性面板
     */
    void RefreshAttributes();

    /**
     * @brief 刷新样式面板
     */
    void RefreshStyles();

    /**
     * @brief 设置选中的元素
     */
    void SetSelectedElement(std::shared_ptr<Element> element);

    /**
     * @brief 渲染面板
     */
    void Render(SkCanvas* canvas, float x, float y, float width, float height);

    /**
     * @brief 处理事件
     */
    bool HandleEvent(const Event& event);

    /**
     * @brief 处理鼠标事件
     * @param x 鼠标 X 坐标（相对于面板）
     * @param y 鼠标 Y 坐标（相对于面板）
     * @param button 鼠标按钮（0=左键，1=右键）
     * @param pressed 是否按下
     * @return 是否消费了事件
     */
    bool HandleMouseEvent(int x, int y, int button, bool pressed);
    
    /**
     * @brief 处理鼠标移动事件
     * @param x 鼠标 X 坐标（相对于面板）
     * @param y 鼠标 Y 坐标（相对于面板）
     * @return 是否需要重绘
     */
    bool HandleMouseMove(int x, int y);

    /**
     * @brief Box Model 悬停回调类型
     */
    using BoxModelHoverCallback = std::function<void(std::shared_ptr<Element>, int)>;
    
    /**
     * @brief 设置 Box Model 悬停回调
     */
    void SetOnBoxModelHover(BoxModelHoverCallback callback);

    /**
     * @brief 处理鼠标滚轮事件
     */
    bool HandleMouseWheel(int x, int y, float delta_x, float delta_y);

    /**
     * @brief 设置元素选择器切换回调
     */
    void SetOnPickerToggled(std::function<void(bool)> callback) { on_picker_toggled_ = callback; }

    /**
     * @brief 设置停靠位置切换回调
     */
    void SetOnDockPositionToggled(std::function<void()> callback) { on_dock_toggled_ = callback; }

    /**
     * @brief 设置当前停靠位置（用于显示正确的图标）
     */
    void SetDockPosition(bool is_bottom) { is_dock_bottom_ = is_bottom; }

    /**
     * @brief 获取元素选择器状态
     */
    bool IsPickerActive() const { return picker_active_; }

    /**
     * @brief 切换元素选择器
     */
    void TogglePicker();

    /**
     * @brief 设置元素选择器状态
     */
    void SetPickerActive(bool active) { picker_active_ = active; }

    /**
     * @brief 检查是否正在拖动分隔线
     */
    bool IsDraggingSplitter() const { return dragging_splitter_; }

    /**
     * @brief 检查鼠标是否在分隔线上
     * @param x 鼠标 X 坐标（相对于面板）
     * @param y 鼠标 Y 坐标（相对于面板）
     * @return 是否在分隔线上
     */
    bool IsMouseOnSplitter(int x, int y) const;

    /**
     * @brief 清除 Box Model 悬停状态
     */
    void ClearBoxModelHover();

private:
    Document* document_;

    std::unique_ptr<DOMTreeView> dom_tree_view_;
    std::unique_ptr<StylesPanel> styles_panel_;
    std::unique_ptr<ElementSearch> element_search_;

    // 面板布局
    float splitter_position_ = 0.5f;  // DOM树和样式面板的分隔位置
    bool dragging_splitter_ = false;
    
    // 保存最后的渲染位置和尺寸（用于事件处理）
    float last_x_ = 0;
    float last_y_ = 0;
    float last_width_ = 0;
    float last_height_ = 0;
    
    // 元素选择器状态
    bool picker_active_ = false;
    std::function<void(bool)> on_picker_toggled_;
    
    // 停靠位置状态
    bool is_dock_bottom_ = true;
    std::function<void()> on_dock_toggled_;

    void RenderBackground(SkCanvas* canvas, float x, float y, float width, float height);
    void RenderToolbar(SkCanvas* canvas, float x, float y, float width, float height);
    
    // 检查是否在分隔线区域
    bool IsOnSplitter(int x) const;
};

} // namespace mblink
