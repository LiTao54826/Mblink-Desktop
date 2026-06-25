/**
 * @file box_model_view.h
 * @brief 盒模型可视化视图
 */

#pragma once

#include <memory>
#include <functional>
#include "core/dom/element.h"

class SkCanvas;

namespace mblink {

/**
 * @brief 盒模型数据
 */
struct BoxModelData {
    // Content
    float content_width = 0;
    float content_height = 0;

    // Padding
    float padding_top = 0;
    float padding_right = 0;
    float padding_bottom = 0;
    float padding_left = 0;

    // Border
    float border_top = 0;
    float border_right = 0;
    float border_bottom = 0;
    float border_left = 0;

    // Margin
    float margin_top = 0;
    float margin_right = 0;
    float margin_bottom = 0;
    float margin_left = 0;
};

/**
 * @brief Box Model 悬停区域
 */
enum class BoxAreaType {
    None,
    Margin,
    Border,
    Padding,
    Content
};

/**
 * @brief 悬停区域变化回调类型
 */
using BoxAreaHoverCallback = std::function<void(std::shared_ptr<Element>, BoxAreaType)>;

/**
 * @brief 盒模型可视化视图
 */
class BoxModelView {
public:
    BoxModelView();
    ~BoxModelView();

    /**
     * @brief 设置目标元素
     */
    void SetElement(std::shared_ptr<Element> element);

    /**
     * @brief 获取盒模型数据
     */
    BoxModelData GetBoxModelData() const;

    /**
     * @brief 渲染视图
     */
    void Render(SkCanvas* canvas, float x, float y, float width, float height);

    /**
     * @brief 处理鼠标移动事件
     * @param x 鼠标 X 坐标（相对于视图）
     * @param y 鼠标 Y 坐标（相对于视图）
     * @return 悬停区域是否发生变化
     */
    bool HandleMouseMove(int x, int y);

    /**
     * @brief 获取当前悬停区域
     */
    BoxAreaType GetHoveredArea() const { return hovered_area_; }

    /**
     * @brief 获取当前元素
     */
    std::shared_ptr<Element> GetElement() const { return element_.lock(); }

    /**
     * @brief 重置悬停状态
     */
    void ResetHover() { hovered_area_ = BoxAreaType::None; }

    /**
     * @brief 设置悬停区域变化回调
     */
    void SetOnHoverChanged(BoxAreaHoverCallback callback) { on_hover_changed_ = callback; }

private:
    std::weak_ptr<Element> element_;
    BoxAreaType hovered_area_ = BoxAreaType::None;
    BoxAreaHoverCallback on_hover_changed_;

    // 保存渲染位置信息（用于判断悬停区域）
    // 这些是相对于视图原点的坐标
    float view_x_ = 0;      // 视图在画布上的 X 坐标
    float view_y_ = 0;      // 视图在画布上的 Y 坐标
    float diagram_x_ = 0;   // 图表相对于视图的 X 偏移
    float diagram_y_ = 0;   // 图表相对于视图的 Y 偏移
    float diagram_width_ = 300;
    float diagram_height_ = 200;
    float margin_inset_ = 0;
    float border_inset_ = 0;
    float padding_inset_ = 0;
    float content_inset_ = 0;

    void RenderBoxDiagram(SkCanvas* canvas, float x, float y, float width, float height,
                          const BoxModelData& data);
    void RenderLabel(SkCanvas* canvas, const std::string& text, float x, float y);
    BoxAreaType HitTest(int x, int y) const;
};

} // namespace mblink
