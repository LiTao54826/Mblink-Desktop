/**
 * @file html_style_element.cpp
 * @brief HTMLStyleElement 类实现
 */

#include "html_style_element.h"
#include "../document.h"
#include "core/lexbor/style_manager.h"

namespace mbink {

HTMLStyleElement::HTMLStyleElement()
    : Element("style") {
}

std::string HTMLStyleElement::GetType() const {
    std::string type = GetAttribute("type");
    return type.empty() ? "text/css" : type;
}

void HTMLStyleElement::SetType(const std::string& type) {
    SetAttribute("type", type);
}

std::string HTMLStyleElement::GetMedia() const {
    return GetAttribute("media");
}

void HTMLStyleElement::SetMedia(const std::string& media) {
    SetAttribute("media", media);
}

void HTMLStyleElement::SetDisabled(bool disabled) {
    if (disabled_ != disabled) {
        disabled_ = disabled;
        NotifyStyleUpdate();
    }
}

std::string HTMLStyleElement::GetCSSText() const {
    return GetTextContent();
}

void HTMLStyleElement::SetCSSText(const std::string& css) {
    // 清除现有子节点
    auto children = GetChildNodes();
    for (auto& child : children) {
        RemoveChild(child);
    }
    
    // 创建新的文本节点
    auto doc = GetOwnerDocument();
    if (doc) {
        auto text_node = doc->CreateTextNode(css);
        AppendChild(text_node);
    }
    
    // 通知样式更新
    NotifyStyleUpdate();
}

void HTMLStyleElement::SetTextContent(const std::string& content) {
    // 调用基类实现
    Element::SetTextContent(content);
    
    // 通知样式更新
    NotifyStyleUpdate();
}

void HTMLStyleElement::NotifyStyleUpdate() {
    if (disabled_) {
        return;
    }
    
    auto doc = GetOwnerDocument();
    if (doc) {
        StyleManager* style_manager = doc->GetStyleManager();
        if (style_manager) {
            // 重新解析此 style 元素
            style_manager->ParseStyleElement(this);
            if (auto document_element = doc->GetDocumentElement()) {
                document_element->SetNeedsStyleRecalc(StyleChangeType::kSubtreeStyleChange);
                document_element->SetNeedsLayout();
            } else if (auto body = doc->GetBody()) {
                body->SetNeedsStyleRecalc(StyleChangeType::kSubtreeStyleChange);
                body->SetNeedsLayout();
            }
        }
    }
}

} // namespace mbink
