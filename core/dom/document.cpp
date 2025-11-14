/**
 * @file document.cpp
 * @brief Document 类实现
 */

// 性能优化：默认关闭调试日志
// #define LIGHTUI_DEBUG_BATCH

#ifdef LIGHTUI_DEBUG_BATCH
    #include <iostream>
    #define DEBUG_BATCH_LOG(msg) std::cout << msg << std::endl
#else
    #define DEBUG_BATCH_LOG(msg) ((void)0)
#endif

#include "document.h"
#include "html_input_element.h"
#include "html_textarea_element.h"
#include "html_button_element.h"
#include "html_form_element.h"
#include "html_select_element.h"
#include "html_option_element.h"
#include "html_anchor_element.h"
#include "html_label_element.h"
#include "html_image_element.h"
#include "html_div_element.h"
#include "html_span_element.h"
#include "html_paragraph_element.h"
#include "html_heading_element.h"
#include "core/lexbor/lexbor_document.h"
#include <algorithm>
#include <iostream>
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

Document::~Document() = default;

void Document::Initialize() {
    // 创建基本的 HTML 结构
    document_element_ = CreateElement("html");
    body_ = CreateElement("body");
    document_element_->AppendChild(body_);
    AppendChild(document_element_);
}

// ========== 工厂方法 ==========

std::shared_ptr<Element> Document::CreateElement(const std::string& tag_name) {
    // 根据标签名创建特定类型的元素
    std::shared_ptr<Element> element;

    if (tag_name == "input") {
        element = std::make_shared<HTMLInputElement>();
    } else if (tag_name == "textarea") {
        element = std::make_shared<HTMLTextAreaElement>();
    } else if (tag_name == "button") {
        auto button = std::make_shared<HTMLButtonElement>();
        // 初始化伪类状态（必须在shared_ptr创建后）
        button->SetPseudoClass(":enabled", true);
        // 设置默认type属性
        button->Element::SetAttribute("type", "submit");
        element = button;
    } else if (tag_name == "form") {
        element = std::make_shared<HTMLFormElement>();
    } else if (tag_name == "select") {
        auto select = std::make_shared<HTMLSelectElement>();
        // 初始化伪类状态
        select->SetPseudoClass(":enabled", true);
        element = select;
    } else if (tag_name == "option") {
        auto option = std::make_shared<HTMLOptionElement>();
        // 初始化伪类状态
        option->SetPseudoClass(":enabled", true);
        element = option;
    } else if (tag_name == "a") {
        auto anchor = std::make_shared<HTMLAnchorElement>();
        // 不设置默认伪类，只有在有href时才设置:link
        element = anchor;
    } else if (tag_name == "label") {
        auto label = std::make_shared<HTMLLabelElement>();
        element = label;
    } else if (tag_name == "img") {
        auto img = std::make_shared<HTMLImageElement>();
        element = img;
    } else if (tag_name == "div") {
        auto div = std::make_shared<HTMLDivElement>();
        element = div;
    } else if (tag_name == "span") {
        auto span = std::make_shared<HTMLSpanElement>();
        element = span;
    } else if (tag_name == "p") {
        auto p = std::make_shared<HTMLParagraphElement>();
        element = p;
    } else if (tag_name == "h1") {
        auto h1 = std::make_shared<HTMLHeadingElement>(1);
        element = h1;
    } else if (tag_name == "h2") {
        auto h2 = std::make_shared<HTMLHeadingElement>(2);
        element = h2;
    } else if (tag_name == "h3") {
        auto h3 = std::make_shared<HTMLHeadingElement>(3);
        element = h3;
    } else if (tag_name == "h4") {
        auto h4 = std::make_shared<HTMLHeadingElement>(4);
        element = h4;
    } else if (tag_name == "h5") {
        auto h5 = std::make_shared<HTMLHeadingElement>(5);
        element = h5;
    } else if (tag_name == "h6") {
        auto h6 = std::make_shared<HTMLHeadingElement>(6);
        element = h6;
    } else {
        element = std::make_shared<Element>(tag_name);
    }

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

// ========== 批量更新API (Week 2 - Task 2.3) ==========

void Document::BeginBatch() {
    batch_depth_++;
    DEBUG_BATCH_LOG("[Document::BeginBatch] Batch depth: " << batch_depth_);
}

void Document::EndBatch() {
    if (batch_depth_ <= 0) {
        DEBUG_BATCH_LOG("[Document::EndBatch] Warning: EndBatch() called without matching BeginBatch()");
        return;
    }

    batch_depth_--;
    DEBUG_BATCH_LOG("[Document::EndBatch] Batch depth: " << batch_depth_);

    // 只在最外层批量结束时触发重绘
    if (batch_depth_ == 0) {
        DEBUG_BATCH_LOG("[Document::EndBatch] Batch complete, notifying observers...");

        // 通知观察者整个文档子树已修改
        // 这会触发Window的SetNeedsRepaint()
        observer_manager_.NotifySubtreeModified(this);
    }
}

} // namespace lightui
