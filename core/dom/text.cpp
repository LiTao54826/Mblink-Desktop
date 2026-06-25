/**
 * @file text.cpp
 * @brief 文本节点类实现
 */

#include "text.h"
#include "document.h"
#include "elements/html_style_element.h"
#include <iostream>

namespace mblink {

Text::Text(const std::string& data)
    : Node(NodeType::TEXT_NODE)
    , data_(data) {
}

void Text::SetData(const std::string& data) {
    std::string old_data = data_;
    data_ = data;
    MarkDirty();

    // 通知观察者和记录变化
    auto doc = GetOwnerDocument();
    if (doc) {
        // 记录到 DirtyNodeTracker（延迟处理）
        doc->GetDirtyTracker().RecordTextChanged(shared_from_this(), old_data, data);
        
        // 通知观察者（立即处理，用于兼容旧代码）
        doc->GetObserverManager().NotifyTextChanged(this, old_data, data);
    }

    auto parent = GetParentNode();
    auto style = std::dynamic_pointer_cast<HTMLStyleElement>(parent);
    if (style) {
        style->NotifyStyleUpdate();
    }
}

std::shared_ptr<Node> Text::CloneNode(bool deep) {
    // Text节点没有子节点，deep参数无效
    return std::make_shared<Text>(data_);
}

std::string Text::GetTextContent() const {
    return data_;
}

void Text::SetTextContent(const std::string& content) {
    SetData(content);
}

} // namespace mblink
