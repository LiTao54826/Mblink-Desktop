/**
 * @file document.cpp
 * @brief Document 类实现
 */

#include "document.h"
#include <algorithm>

namespace lightui {

// ========== 构造函数 ==========

Document::Document()
    : Node(NodeType::DOCUMENT_NODE)
    , document_element_(nullptr)
    , body_(nullptr)
    , id_map_() {
}

// ========== 工厂方法 ==========

std::shared_ptr<Element> Document::CreateElement(const std::string& tag_name) {
    auto element = std::make_shared<Element>(tag_name);

    // 如果是 html 元素，设置为 documentElement
    if (tag_name == "html" && !document_element_) {
        document_element_ = element;
        AppendChild(element);
    }

    return element;
}

std::shared_ptr<Text> Document::CreateTextNode(const std::string& data) {
    return std::make_shared<Text>(data);
}

// ========== 文档属性 ==========

void Document::SetBody(std::shared_ptr<Element> body) {
    body_ = body;
}

// ========== 查询方法 ==========

std::shared_ptr<Element> Document::GetElementById(const std::string& id) {
    auto it = id_map_.find(id);
    if (it != id_map_.end()) {
        return it->second.lock();
    }
    return nullptr;
}

std::vector<std::shared_ptr<Element>> Document::GetElementsByTagName(const std::string& tag_name) {
    std::vector<std::shared_ptr<Element>> result;

    if (document_element_) {
        CollectElementsByTagName(document_element_, tag_name, result);
    }

    return result;
}

std::vector<std::shared_ptr<Element>> Document::GetElementsByClassName(const std::string& class_name) {
    std::vector<std::shared_ptr<Element>> result;

    if (document_element_) {
        CollectElementsByClassName(document_element_, class_name, result);
    }

    return result;
}

// ========== ID 映射管理 ==========

void Document::RegisterElementId(const std::string& id, std::shared_ptr<Element> element) {
    id_map_[id] = element;
}

void Document::UnregisterElementId(const std::string& id) {
    id_map_.erase(id);
}

// ========== Node 接口实现 ==========

std::shared_ptr<Node> Document::CloneNode(bool deep) {
    auto cloned = std::make_shared<Document>();

    if (deep && document_element_) {
        auto cloned_element = std::dynamic_pointer_cast<Element>(document_element_->CloneNode(true));
        cloned->document_element_ = cloned_element;
        cloned->AppendChild(cloned_element);
    }

    return cloned;
}

// ========== 私有辅助方法 ==========

void Document::CollectElementsByTagName(std::shared_ptr<Node> node,
                                        const std::string& tag_name,
                                        std::vector<std::shared_ptr<Element>>& result) {
    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element) {
        if (element->GetTagName() == tag_name) {
            result.push_back(element);
        }
    }

    // 递归遍历子节点
    for (const auto& child : node->GetChildNodes()) {
        CollectElementsByTagName(child, tag_name, result);
    }
}

void Document::CollectElementsByClassName(std::shared_ptr<Node> node,
                                          const std::string& class_name,
                                          std::vector<std::shared_ptr<Element>>& result) {
    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element) {
        if (element->HasClass(class_name)) {
            result.push_back(element);
        }
    }

    // 递归遍历子节点
    for (const auto& child : node->GetChildNodes()) {
        CollectElementsByClassName(child, class_name, result);
    }
}

} // namespace lightui
