/**
 * @file html_link_element.cpp
 * @brief HTMLLinkElement 类实现
 */

#include "html_link_element.h"
#include "document.h"

namespace lightui {

HTMLLinkElement::HTMLLinkElement()
    : Element("link") {
}

std::string HTMLLinkElement::GetRel() const {
    return GetAttribute("rel");
}

void HTMLLinkElement::SetRel(const std::string& rel) {
    SetAttribute("rel", rel);
}

std::string HTMLLinkElement::GetHref() const {
    return GetAttribute("href");
}

void HTMLLinkElement::SetHref(const std::string& href) {
    SetAttribute("href", href);
    // 当 href 改变时，重置加载状态
    ResetLoaded();
}

std::string HTMLLinkElement::GetType() const {
    std::string type = GetAttribute("type");
    // 对于样式表，默认类型为 text/css
    if (type.empty() && IsStylesheet()) {
        return "text/css";
    }
    return type;
}

void HTMLLinkElement::SetType(const std::string& type) {
    SetAttribute("type", type);
}

std::string HTMLLinkElement::GetMedia() const {
    return GetAttribute("media");
}

void HTMLLinkElement::SetMedia(const std::string& media) {
    SetAttribute("media", media);
}

void HTMLLinkElement::SetDisabled(bool disabled) {
    if (disabled_ != disabled) {
        disabled_ = disabled;
        // TODO: 通知样式更新
    }
}

bool HTMLLinkElement::IsStylesheet() const {
    std::string rel = GetRel();
    return rel == "stylesheet";
}

} // namespace lightui

