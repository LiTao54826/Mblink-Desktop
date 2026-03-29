/**
 * @file element_highlighter.h
 * @brief 元素高亮覆盖层
 */

#pragma once

#include <memory>
#include "core/dom/element.h"
#include "include/core/SkColor.h"

class SkCanvas;

namespace mbink {

/**
 * @brief 高亮区域类型（与 BoxAreaType 对应）
 */
enum class HighlightAreaType {
    None,
    Margin,
    Border,
    Padding,
    Content,
    All  // 显示所有区域
};

/**
 * @brief 高亮显示选项
 */
struct HighlightOptions {
    bool show_margin = true;
    bool show_border = true;
    bool show_padding = true;
    bool show_content = true;
    bool show_info_tooltip = true;
};

/**
 * @brief 元素高亮覆盖层
 */
class ElementHighlighter {
public:
    ElementHighlighter();
    ~ElementHighlighter();

    // 高亮控制
    void SetHighlightedElement(std::shared_ptr<Element> element);
    void SetHoveredElement(std::shared_ptr<Element> element);
    void ClearHighlight();
    void ClearHover();

    /**
     * @brief 设置 Box Model 悬停高亮
     * @param element 要高亮的元素
     * @param area 要高亮的区域
     */
    void SetBoxModelHover(std::shared_ptr<Element> element, HighlightAreaType area);

    /**
     * @brief 清除 Box Model 悬停高亮
     */
    void ClearBoxModelHover();

    // 显示选项
    void SetOptions(const HighlightOptions& options) { options_ = options; }
    const HighlightOptions& GetOptions() const { return options_; }

    // 颜色配置
    void SetMarginColor(SkColor color) { margin_color_ = color; }
    void SetBorderColor(SkColor color) { border_color_ = color; }
    void SetPaddingColor(SkColor color) { padding_color_ = color; }
    void SetContentColor(SkColor color) { content_color_ = color; }

    // 渲染（在主应用内容之上）
    void Render(SkCanvas* canvas);

private:
    std::weak_ptr<Element> highlighted_element_;
    std::weak_ptr<Element> hovered_element_;
    std::weak_ptr<Element> box_model_hover_element_;
    HighlightAreaType box_model_hover_area_ = HighlightAreaType::None;
    HighlightOptions options_;

    // 默认颜色（半透明）- Chrome DevTools 风格
    SkColor margin_color_ = 0x80F3B585;   // Orange/tan
    SkColor border_color_ = 0x80FDDD9B;   // Yellow
    SkColor padding_color_ = 0x80C2DDA6;  // Green
    SkColor content_color_ = 0x80A2C5E0;  // Blue

    void RenderElementHighlight(SkCanvas* canvas, std::shared_ptr<Element> element,
                                 bool is_hover);
    void RenderBoxModelHighlight(SkCanvas* canvas, std::shared_ptr<Element> element,
                                  HighlightAreaType area);
    void RenderBoxModel(SkCanvas* canvas, std::shared_ptr<Element> element);
    void RenderInfoTooltip(SkCanvas* canvas, std::shared_ptr<Element> element);
};

} // namespace mbink
