/**
 * @file document.cpp
 * @brief Document 类实现
 */

#include "document.h"
#include "core/lexbor/lexbor_document.h"
#include <algorithm>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/text.h>

namespace lightui {

// ========== 构造函数 ==========

Document::Document()
    : Node(NodeType::DOCUMENT_NODE)
    , document_element_(nullptr)
    , body_(nullptr)
    , id_map_()
    , lexbor_doc_(std::make_unique<LexborDocument>())
    , lexbor_dirty_(false) {
}

void Document::Initialize() {
    // 创建基本的 HTML 结构
    document_element_ = CreateElement("html");
    body_ = CreateElement("body");
    document_element_->AppendChild(body_);
    AppendChild(document_element_);
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

// ========== Lexbor 集成 ==========

bool Document::LoadHTML(const std::string& html) {
    if (!lexbor_doc_) {
        return false;
    }

    // 使用 Lexbor 解析 HTML
    if (!lexbor_doc_->ParseHTML(html)) {
        return false;
    }

    // 从 Lexbor DOM 同步到 MBink DOM
    SyncFromLexbor();

    lexbor_dirty_ = false;
    return true;
}

bool Document::LoadHTMLFile(const std::string& file_path) {
    if (!lexbor_doc_) {
        return false;
    }

    // 使用 Lexbor 从文件解析 HTML
    if (!lexbor_doc_->ParseHTMLFile(file_path)) {
        return false;
    }

    // 从 Lexbor DOM 同步到 MBink DOM
    SyncFromLexbor();

    lexbor_dirty_ = false;
    return true;
}

std::string Document::SaveHTML() {
    if (!lexbor_doc_) {
        return "";
    }

    // 如果 MBink DOM 已修改，先同步到 Lexbor
    if (lexbor_dirty_) {
        SyncToLexbor();
        lexbor_dirty_ = false;
    }

    // 使用 Lexbor 序列化 HTML
    return lexbor_doc_->SerializeToHTML();
}

void Document::SyncFromLexbor() {
    if (!lexbor_doc_) {
        return;
    }

    // 清空当前 DOM 树
    child_nodes_.clear();
    document_element_ = nullptr;
    body_ = nullptr;
    id_map_.clear();

    // 获取 Lexbor 文档元素
    LexborElement* lexbor_html = lexbor_doc_->GetDocumentElement();
    if (!lexbor_html) {
        return;
    }

    // 转换 Lexbor DOM 到 MBink DOM
    lxb_dom_node_t* lexbor_node = lxb_dom_interface_node(lexbor_html->GetNativeElement());
    auto doc_ptr = std::dynamic_pointer_cast<Document>(shared_from_this());
    std::shared_ptr<Node> html_node = Element::ConvertLexborNodeToNode(lexbor_node, doc_ptr);

    if (html_node) {
        auto html_elem = std::dynamic_pointer_cast<Element>(html_node);
        if (html_elem) {
            document_element_ = html_elem;
            AppendChild(html_elem);

            // 查找 body 元素
            for (const auto& child : html_elem->GetChildNodes()) {
                auto child_elem = std::dynamic_pointer_cast<Element>(child);
                if (child_elem && child_elem->GetTagName() == "body") {
                    body_ = child_elem;
                    break;
                }
            }

            // 重建 ID 映射
            RebuildIdMap(html_elem);
        }
    }
}

void Document::SyncToLexbor() {
    if (!lexbor_doc_ || !document_element_) {
        return;
    }

    // 序列化 MBink DOM 为 HTML
    std::string html = document_element_->GetOuterHTML();

    // 重新解析到 Lexbor
    lexbor_doc_->ParseHTML(html);
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

void Document::RebuildIdMap(std::shared_ptr<Element> root) {
    if (!root) {
        return;
    }

    // 如果元素有 ID，注册到映射表
    std::string id = root->GetAttribute("id");
    if (!id.empty()) {
        RegisterElementId(id, root);
    }

    // 递归处理子节点
    for (const auto& child : root->GetChildNodes()) {
        auto child_elem = std::dynamic_pointer_cast<Element>(child);
        if (child_elem) {
            RebuildIdMap(child_elem);
        }
    }
}

} // namespace lightui
