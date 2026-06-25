/**
 * @file styles_panel.h
 * @brief 样式面板容器
 */

#pragma once

#include <memory>
#include <functional>
#include "core/dom/element.h"

class SkCanvas;

namespace mblink {

class InlineStylesView;
class BoxModelView;
class Event;

/**
 * @brief 样式面板标签页
 */
enum class StylesTab {
    Styles,
    BoxModel
};

/**
 * @brief 样式面板
 */
class StylesPanel {
public:
    StylesPanel();
    ~StylesPanel();

    /**
     * @brief 设置目标元素
     */
    void SetElement(std::shared_ptr<Element> element);

    /**
     * @brief 刷新面板内容
     */
    void Refresh();

    /**
     * @brief 设置活动标签页
     */
    void SetActiveTab(StylesTab tab) { active_tab_ = tab; }

    /**
     * @brief 获取活动标签页
     */
    StylesTab GetActiveTab() const { return active_tab_; }

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
     */
    bool HandleMouseEvent(int x, int y, int button, bool pressed);
    
    /**
     * @brief 处理鼠标移动事件
     * @return 是否需要重绘
     */
    bool HandleMouseMove(int x, int y);

    /**
     * @brief 样式变更回调
     */
    using StyleChangeCallback = std::function<void(
        const std::string& property, const std::string& value)>;
    void SetOnStyleChanged(StyleChangeCallback callback) {
        on_style_changed_ = callback;
    }

    /**
     * @brief Box Model 悬停回调类型
     * @param element 悬停的元素
     * @param area 悬停的区域 (0=None, 1=Margin, 2=Border, 3=Padding, 4=Content)
     */
    using BoxModelHoverCallback = std::function<void(std::shared_ptr<Element>, int)>;
    void SetOnBoxModelHover(BoxModelHoverCallback callback);

    /**
     * @brief 清除 Box Model 悬停状态
     */
    void ClearBoxModelHover();

private:
    std::weak_ptr<Element> element_;
    StylesTab active_tab_ = StylesTab::Styles;

    std::unique_ptr<InlineStylesView> inline_styles_view_;
    std::unique_ptr<BoxModelView> box_model_view_;

    StyleChangeCallback on_style_changed_;
    BoxModelHoverCallback on_box_model_hover_;
    
    // 保存最后的渲染位置和尺寸
    float last_x_ = 0;
    float last_y_ = 0;
    float last_width_ = 0;
    float last_height_ = 0;
    
    // 滚动偏移量
    float scroll_offset_ = 0;
    float content_height_ = 0;  // 内容总高度

    void RenderTabs(SkCanvas* canvas, float x, float y, float width, float height);
    
public:
    /**
     * @brief 处理鼠标滚轮事件
     * @param delta_y 滚轮垂直滚动量
     * @return 是否处理了事件
     */
    bool HandleMouseWheel(float delta_y);
};

} // namespace mblink
