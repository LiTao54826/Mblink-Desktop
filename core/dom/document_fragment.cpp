/**
 * @file document_fragment.cpp
 * @brief DocumentFragment 类实现
 */

#include "document_fragment.h"

namespace lightui {

DocumentFragment::DocumentFragment() 
    : Node(NodeType::DOCUMENT_FRAGMENT_NODE) {
}

std::shared_ptr<Node> DocumentFragment::CloneNode(bool deep) {
    auto clone = std::make_shared<DocumentFragment>();
    
    if (deep) {
        // 深度克隆：复制所有子节点
        for (const auto& child : GetChildNodes()) {
            auto child_clone = child->CloneNode(true);
            if (child_clone) {
                clone->AppendChild(child_clone);
            }
        }
    }
    
    return clone;
}

} // namespace lightui
