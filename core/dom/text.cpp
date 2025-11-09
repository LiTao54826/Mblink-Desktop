/**
 * @file text.cpp
 * @brief 文本节点类实现
 */

#include "text.h"

namespace lightui {

Text::Text(const std::string& data)
    : Node(NodeType::TEXT_NODE)
    , data_(data) {
}

void Text::SetData(const std::string& data) {
    data_ = data;
    MarkDirty();
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

} // namespace lightui
