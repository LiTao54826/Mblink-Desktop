/**
 * @file html_script_element.cpp
 * @brief HTMLScriptElement 类实现
 */

#include "html_script_element.h"
#include "../document.h"

namespace mblink {

HTMLScriptElement::HTMLScriptElement()
    : Element("script") {
}

std::string HTMLScriptElement::GetType() const {
    std::string type = GetAttribute("type");
    // 默认类型为 text/javascript
    return type.empty() ? "text/javascript" : type;
}

void HTMLScriptElement::SetType(const std::string& type) {
    SetAttribute("type", type);
}

std::string HTMLScriptElement::GetSrc() const {
    return GetAttribute("src");
}

void HTMLScriptElement::SetSrc(const std::string& src) {
    SetAttribute("src", src);
}

bool HTMLScriptElement::IsAsync() const {
    return HasAttribute("async");
}

void HTMLScriptElement::SetAsync(bool async) {
    if (async) {
        SetAttribute("async", "");
    } else {
        RemoveAttribute("async");
    }
}

bool HTMLScriptElement::IsDefer() const {
    return HasAttribute("defer");
}

void HTMLScriptElement::SetDefer(bool defer) {
    if (defer) {
        SetAttribute("defer", "");
    } else {
        RemoveAttribute("defer");
    }
}

std::string HTMLScriptElement::GetScriptText() const {
    return GetTextContent();
}

void HTMLScriptElement::SetScriptText(const std::string& text) {
    // 清除现有子节点
    auto children = GetChildNodes();
    for (auto& child : children) {
        RemoveChild(child);
    }
    
    // 创建新的文本节点
    auto doc = GetOwnerDocument();
    if (doc) {
        auto text_node = doc->CreateTextNode(text);
        AppendChild(text_node);
    }
    
    // 重置执行状态，允许重新执行
    ResetExecuted();
}

bool HTMLScriptElement::IsModule() const {
    std::string type = GetAttribute("type");
    return type == "module";
}

} // namespace mblink
