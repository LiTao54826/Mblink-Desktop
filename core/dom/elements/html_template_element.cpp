#include "html_template_element.h"

#include "../comment.h"
#include "../document.h"
#include "../text.h"

namespace mblink {

HTMLTemplateElement::HTMLTemplateElement()
    : Element("template")
    , content_(std::make_shared<DocumentFragment>()) {
}

std::shared_ptr<Node> HTMLTemplateElement::CloneNode(bool deep) {
    auto cloned = std::make_shared<HTMLTemplateElement>();
    for (const auto& attr : GetAllAttributes()) {
        cloned->SetAttribute(attr.first, attr.second);
    }
    if (deep && content_) {
        for (const auto& child : content_->GetChildNodes()) {
            auto child_clone = child->CloneNode(true);
            if (child_clone) {
                cloned->content_->AppendChild(child_clone);
            }
        }
    }
    return cloned;
}

std::string HTMLTemplateElement::GetTextContent() const {
    return content_ ? content_->GetTextContent() : std::string();
}

void HTMLTemplateElement::SetTextContent(const std::string& content) {
    if (!content_) return;
    content_->SetTextContent(content);
    MarkDirty();
}

std::string HTMLTemplateElement::GetInnerHTML() const {
    if (!content_) return "";
    std::string html;
    for (const auto& child : content_->GetChildNodes()) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            html += std::static_pointer_cast<Element>(child)->GetOuterHTML();
        } else if (child->GetNodeType() == NodeType::TEXT_NODE) {
            html += std::static_pointer_cast<Text>(child)->GetData();
        } else if (child->GetNodeType() == NodeType::COMMENT_NODE) {
            html += "<!--" + std::static_pointer_cast<Comment>(child)->GetData() + "-->";
        }
    }
    return html;
}

void HTMLTemplateElement::SetInnerHTML(const std::string& html) {
    if (!content_) return;
    content_->SetTextContent("");
    auto children = content_->GetChildNodes();
    for (const auto& child : children) {
        content_->RemoveChild(child);
    }
    auto doc = std::dynamic_pointer_cast<Document>(GetOwnerDocument());
    if (!doc) {
        if (!html.empty()) {
            content_->AppendChild(std::make_shared<Text>(html));
        }
        MarkDirty();
        return;
    }

    auto host = doc->CreateElement("div");
    host->SetInnerHTML(html);
    auto parsed = host->GetChildNodes();
    for (const auto& child : parsed) {
        host->RemoveChild(child);
        content_->AppendChild(child);
    }
    MarkDirty();
}

} // namespace mblink
