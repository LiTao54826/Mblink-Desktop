#pragma once

#include "../document_fragment.h"
#include "../element.h"

namespace mbink {

class HTMLTemplateElement : public Element {
public:
    HTMLTemplateElement();
    ~HTMLTemplateElement() override = default;

    std::shared_ptr<DocumentFragment> GetContent() const { return content_; }

    std::shared_ptr<Node> CloneNode(bool deep) override;
    std::string GetTextContent() const override;
    void SetTextContent(const std::string& content) override;
    std::string GetInnerHTML() const override;
    void SetInnerHTML(const std::string& html) override;

private:
    std::shared_ptr<DocumentFragment> content_;
};

} // namespace mbink
