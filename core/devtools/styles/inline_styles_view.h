/**
 * @file inline_styles_view.h
 * @brief 内联样式视图
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include "core/dom/element.h"

class SkCanvas;

namespace mbink {

/**
 * @brief 内联样式视图
 */
class InlineStylesView {
public:
    InlineStylesView();
    ~InlineStylesView();

    /**
     * @brief 设置目标元素
     */
    void SetElement(std::shared_ptr<Element> element);

    /**
     * @brief 渲染视图
     */
    void Render(SkCanvas* canvas, float x, float y, float width, float height);

    /**
     * @brief 获取样式属性列表
     */
    struct StyleProperty {
        std::string name;
        std::string value;
    };
    std::vector<StyleProperty> GetStyleProperties() const;

    /**
     * @brief 获取内容总高度（用于滚动计算）
     */
    float GetContentHeight() const { return content_height_; }

private:
    std::weak_ptr<Element> element_;
    mutable float content_height_ = 0;  // 内容总高度

    void RenderEmptyState(SkCanvas* canvas, float x, float y, float width, float height);
    void RenderStyleList(SkCanvas* canvas, float x, float y, float width, float height,
                         const std::vector<StyleProperty>& properties);
};

} // namespace mbink
