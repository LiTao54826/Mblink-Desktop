/**
 * @file comment.cpp
 * @brief 注释节点类实现
 */

#include "comment.h"

namespace mbink {

Comment::Comment(const std::string& data)
    : Text(data) {
    node_type_ = NodeType::COMMENT_NODE;
}

std::shared_ptr<Node> Comment::CloneNode(bool deep) {
    return std::make_shared<Comment>(GetData());
}

} // namespace mbink
