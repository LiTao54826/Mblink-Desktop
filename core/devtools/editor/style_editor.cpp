/**
 * @file style_editor.cpp
 * @brief 样式编辑器实现
 */

#include "style_editor.h"
#include <algorithm>
#include <unordered_set>

namespace lightui {

namespace {
    // 有效的 CSS 属性列表（部分）
    const std::unordered_set<std::string> VALID_PROPERTIES = {
        "display", "position", "top", "right", "bottom", "left",
        "width", "height", "min-width", "max-width", "min-height", "max-height",
        "margin", "margin-top", "margin-right", "margin-bottom", "margin-left",
        "padding", "padding-top", "padding-right", "padding-bottom", "padding-left",
        "border", "border-width", "border-style", "border-color",
        "border-top", "border-right", "border-bottom", "border-left",
        "color", "background", "background-color", "background-image",
        "font-family", "font-size", "font-weight", "font-style",
        "text-align", "text-decoration", "line-height", "letter-spacing",
        "flex", "flex-direction", "flex-wrap", "justify-content", "align-items",
        "grid-template-columns", "grid-template-rows", "grid-gap",
        "overflow", "visibility", "opacity", "z-index",
        "transform", "transition", "animation"
    };
}

StyleEditor::StyleEditor() = default;

StyleEditor::~StyleEditor() = default;

void StyleEditor::SetElement(std::shared_ptr<Element> element) {
    element_ = element;
}

bool StyleEditor::SetStyleProperty(const std::string& property, const std::string& value) {
    auto element = element_.lock();
    if (!element) return false;

    // 验证
    if (!ValidateStyleValue(property, value)) {
        return false;
    }

    // 应用样式
    element->SetStyle(property, value);

    // 触发回调
    if (on_style_changed_) {
        on_style_changed_(property, value);
    }

    return true;
}

bool StyleEditor::AddStyleProperty(const std::string& property, const std::string& value) {
    return SetStyleProperty(property, value);
}

bool StyleEditor::RemoveStyleProperty(const std::string& property) {
    auto element = element_.lock();
    if (!element) return false;

    // 设置为空字符串来移除样式
    element->SetStyle(property, "");

    if (on_style_changed_) {
        on_style_changed_(property, "");
    }

    return true;
}

std::string StyleEditor::GetStyleProperty(const std::string& property) const {
    auto element = element_.lock();
    if (!element) return "";

    return element->GetStyle(property);
}

bool StyleEditor::ValidateStyleValue(const std::string& property, const std::string& value) {
    // 检查属性名是否有效
    std::string lower_prop = property;
    std::transform(lower_prop.begin(), lower_prop.end(), lower_prop.begin(), ::tolower);

    // 允许自定义属性（以 -- 开头）
    if (lower_prop.substr(0, 2) == "--") {
        return true;
    }

    // 检查是否为已知属性
    if (VALID_PROPERTIES.find(lower_prop) == VALID_PROPERTIES.end()) {
        // 未知属性，但仍然允许（可能是新的 CSS 属性）
        // 只是发出警告
    }

    // 值不能为空（除非是删除操作）
    if (value.empty()) {
        return false;
    }

    // TODO: 更详细的值验证
    // 例如：检查颜色格式、长度单位等

    return true;
}

} // namespace lightui
