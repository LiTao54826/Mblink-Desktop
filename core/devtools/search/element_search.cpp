/**
 * @file element_search.cpp
 * @brief 元素搜索组件实现
 */

#include "element_search.h"
#include <algorithm>

namespace mblink {

ElementSearch::ElementSearch(Document* document)
    : document_(document) {
}

ElementSearch::~ElementSearch() = default;

SearchResult ElementSearch::Search(const std::string& query) {
    current_result_.elements.clear();
    current_result_.current_index = -1;

    if (query.empty() || !document_) {
        return current_result_;
    }

    if (search_type_ == SearchType::CSSSelector) {
        current_result_.elements = SearchBySelector(query);
    } else {
        current_result_.elements = SearchByText(query);
    }

    if (!current_result_.elements.empty()) {
        current_result_.current_index = 0;
    }

    return current_result_;
}

void ElementSearch::NextResult() {
    if (current_result_.elements.empty()) return;

    current_result_.current_index++;
    if (current_result_.current_index >= static_cast<int>(current_result_.elements.size())) {
        current_result_.current_index = 0;  // 循环
    }
}

void ElementSearch::PreviousResult() {
    if (current_result_.elements.empty()) return;

    current_result_.current_index--;
    if (current_result_.current_index < 0) {
        current_result_.current_index = static_cast<int>(current_result_.elements.size()) - 1;  // 循环
    }
}

std::shared_ptr<Element> ElementSearch::GetCurrentResult() const {
    if (current_result_.current_index >= 0 &&
        current_result_.current_index < static_cast<int>(current_result_.elements.size())) {
        return current_result_.elements[current_result_.current_index];
    }
    return nullptr;
}

void ElementSearch::Clear() {
    current_result_.elements.clear();
    current_result_.current_index = -1;
}

std::vector<std::shared_ptr<Element>> ElementSearch::SearchBySelector(const std::string& selector) {
    std::vector<std::shared_ptr<Element>> results;

    if (!document_) return results;

    try {
        // 使用 body 元素的 QuerySelectorAll 进行搜索
        auto body = document_->GetBody();
        if (body) {
            auto elements = body->QuerySelectorAll(selector);
            for (const auto& elem : elements) {
                results.push_back(elem);
            }
        }
    } catch (...) {
        // 无效的选择器，返回空结果
    }

    return results;
}

std::vector<std::shared_ptr<Element>> ElementSearch::SearchByText(const std::string& text) {
    std::vector<std::shared_ptr<Element>> results;

    if (!document_ || text.empty()) return results;

    // 转换为小写进行不区分大小写的搜索
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);

    // 递归搜索所有元素
    std::function<void(std::shared_ptr<Node>)> search_recursive;
    search_recursive = [&](std::shared_ptr<Node> node) {
        if (!node) return;

        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(node);
            if (element) {
                bool match = false;

                // 检查标签名
                std::string tag = element->GetTagName();
                std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
                if (tag.find(lower_text) != std::string::npos) {
                    match = true;
                }

                // 检查 id
                if (!match) {
                    std::string id = element->GetAttribute("id");
                    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
                    if (id.find(lower_text) != std::string::npos) {
                        match = true;
                    }
                }

                // 检查 class
                if (!match) {
                    std::string cls = element->GetAttribute("class");
                    std::transform(cls.begin(), cls.end(), cls.begin(), ::tolower);
                    if (cls.find(lower_text) != std::string::npos) {
                        match = true;
                    }
                }

                if (match) {
                    results.push_back(element);
                }
            }
        }

        // 递归搜索子节点
        for (const auto& child : node->GetChildNodes()) {
            search_recursive(child);
        }
    };

    search_recursive(document_->GetDocumentElement());

    return results;
}

} // namespace mblink
