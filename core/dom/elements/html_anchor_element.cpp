/**
 * @file html_anchor_element.cpp
 * @brief HTML Anchor元素实现
 */

#include "html_anchor_element.h"
#include "../document.h"
#include "../text.h"
#include "../event.h"
#include <algorithm>

namespace mblink {

HTMLAnchorElement::HTMLAnchorElement()
    : Element("a")
    , href_("")
    , target_("_self")
    , download_("")
    , rel_("")
    , visited_(false) {
}

void HTMLAnchorElement::SetAttribute(const std::string& name, const std::string& value) {
    // 调用基类方法设置属性
    Element::SetAttribute(name, value);
    
    // 同步到内部成员
    if (name == "href") {
        href_ = value;
        UpdatePseudoClasses();
    } else if (name == "target") {
        target_ = value;
    } else if (name == "download") {
        download_ = value;
    } else if (name == "rel") {
        rel_ = value;
    }
}

void HTMLAnchorElement::RemoveAttribute(const std::string& name) {
    // 调用基类方法移除属性
    Element::RemoveAttribute(name);
    
    // 同步到内部成员
    if (name == "href") {
        href_ = "";
        UpdatePseudoClasses();
    } else if (name == "target") {
        target_ = "_self";
    } else if (name == "download") {
        download_ = "";
    } else if (name == "rel") {
        rel_ = "";
    }
}

std::string HTMLAnchorElement::GetHref() const {
    return href_;
}

void HTMLAnchorElement::SetHref(const std::string& href) {
    href_ = href;
    SetAttribute("href", href);
    UpdatePseudoClasses();
}

std::string HTMLAnchorElement::GetTarget() const {
    return target_;
}

void HTMLAnchorElement::SetTarget(const std::string& target) {
    target_ = target;
    SetAttribute("target", target);
}

std::string HTMLAnchorElement::GetDownload() const {
    return download_;
}

void HTMLAnchorElement::SetDownload(const std::string& download) {
    download_ = download;
    SetAttribute("download", download);
}

std::string HTMLAnchorElement::GetRel() const {
    return rel_;
}

void HTMLAnchorElement::SetRel(const std::string& rel) {
    rel_ = rel;
    SetAttribute("rel", rel);
}

std::string HTMLAnchorElement::GetText() const {
    return GetTextContent();
}

void HTMLAnchorElement::SetText(const std::string& text) {
    // 清除所有子节点
    while (GetFirstChild()) {
        RemoveChild(GetFirstChild());
    }

    // 创建文本节点
    auto doc = GetOwnerDocument();
    if (!doc) {
        // 如果没有owner document，无法创建文本节点
        return;
    }

    auto text_node = doc->CreateTextNode(text);
    if (text_node) {
        AppendChild(text_node);
    }
}

void HTMLAnchorElement::HandleClick() {
    // 触发click事件
    auto event = std::make_shared<Event>("click");
    event->SetTarget(shared_from_this());
    event->SetCurrentTarget(shared_from_this());
    
    DispatchEvent(event);
    
    // 如果有download属性，执行下载
    if (!download_.empty()) {
        Download();
        return;
    }
    
    // 否则执行导航
    if (!href_.empty()) {
        Navigate();
        
        // 标记为已访问
        MarkAsVisited();
    }
}

void HTMLAnchorElement::MarkAsVisited() {
    if (!visited_) {
        visited_ = true;
        UpdatePseudoClasses();
    }
}

bool HTMLAnchorElement::IsVisited() const {
    return visited_;
}

void HTMLAnchorElement::UpdatePseudoClasses() {
    // 如果没有href，不设置任何伪类
    if (href_.empty()) {
        SetPseudoClass("link", false);
        SetPseudoClass("visited", false);
        return;
    }

    // 设置:link或:visited伪类
    if (visited_) {
        SetPseudoClass("link", false);
        SetPseudoClass("visited", true);
    } else {
        SetPseudoClass("link", true);
        SetPseudoClass("visited", false);
    }
}

void HTMLAnchorElement::Navigate() {
    // 在MBlink中，导航由应用层处理
    // 这里触发一个自定义的navigate事件
    
    auto event = std::make_shared<Event>("navigate");
    event->SetTarget(shared_from_this());
    event->SetCurrentTarget(shared_from_this());
    
    DispatchEvent(event);
    
    // TODO: 在实际实现中，这里可能需要：
    // 1. 解析href（相对路径、绝对路径、锚点等）
    // 2. 根据target决定在哪里打开
    //    - _self: 当前窗口
    //    - _blank: 新窗口
    //    - _parent: 父窗口
    //    - _top: 顶层窗口
    // 3. 处理rel属性（noopener, noreferrer等）
    // 4. 更新浏览历史
}

void HTMLAnchorElement::Download() {
    // 在MBlink中，下载由应用层处理
    // 这里触发一个自定义的download事件
    
    auto event = std::make_shared<Event>("download");
    event->SetTarget(shared_from_this());
    event->SetCurrentTarget(shared_from_this());
    
    DispatchEvent(event);
    
    // TODO: 在实际实现中，这里可能需要：
    // 1. 获取href指向的资源
    // 2. 使用download属性作为文件名
    // 3. 保存到本地文件系统
}

} // namespace mblink

