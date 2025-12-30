/**
 * @file lexbor_document.cpp
 * @brief Lexbor Document C++ 包装类实现
 */

#include "lexbor_document.h"
#include <cstring>
#include <sstream>
#include <vector>
#include <fstream>
#include <functional>

namespace lightui {

// ========== LexborDocument 实现 ==========

LexborDocument::LexborDocument()
    : document_(nullptr)
    , css_parser_(nullptr)
    , selectors_(nullptr)
    , css_initialized_(false)
    , selectors_initialized_(false) {
    
    // 创建 Lexbor HTML 文档
    document_ = lxb_html_document_create();
    if (!document_) {
        throw std::runtime_error("Failed to create Lexbor document");
    }
}

LexborDocument::~LexborDocument() {
    Cleanup();
}

LexborDocument::LexborDocument(LexborDocument&& other) noexcept
    : document_(other.document_)
    , css_parser_(other.css_parser_)
    , selectors_(other.selectors_)
    , css_initialized_(other.css_initialized_)
    , selectors_initialized_(other.selectors_initialized_) {
    
    other.document_ = nullptr;
    other.css_parser_ = nullptr;
    other.selectors_ = nullptr;
    other.css_initialized_ = false;
    other.selectors_initialized_ = false;
}

LexborDocument& LexborDocument::operator=(LexborDocument&& other) noexcept {
    if (this != &other) {
        Cleanup();
        
        document_ = other.document_;
        css_parser_ = other.css_parser_;
        selectors_ = other.selectors_;
        css_initialized_ = other.css_initialized_;
        selectors_initialized_ = other.selectors_initialized_;
        
        other.document_ = nullptr;
        other.css_parser_ = nullptr;
        other.selectors_ = nullptr;
        other.css_initialized_ = false;
        other.selectors_initialized_ = false;
    }
    return *this;
}

void LexborDocument::Cleanup() {
    if (selectors_) {
        lxb_selectors_destroy(selectors_, true);
        selectors_ = nullptr;
    }
    
    if (css_parser_) {
        lxb_css_parser_destroy(css_parser_, true);
        css_parser_ = nullptr;
    }
    
    if (document_) {
        lxb_html_document_destroy(document_);
        document_ = nullptr;
    }
}

bool LexborDocument::ParseHTML(const std::string& html) {
    if (!document_) {
        errors_.push_back("Document is null");
        return false;
    }

    errors_.clear();  // 清空之前的错误

    lxb_status_t status = lxb_html_document_parse(
        document_,
        reinterpret_cast<const lxb_char_t*>(html.c_str()),
        html.length()
    );

    if (status != LXB_STATUS_OK) {
        errors_.push_back("Failed to parse HTML: status code " + std::to_string(status));
        return false;
    }

    return true;
}

bool LexborDocument::ParseHTMLFile(const std::string& file_path) {
    errors_.clear();

    // 读取文件内容
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        errors_.push_back("Failed to open file: " + file_path);
        return false;
    }

    // 读取整个文件到字符串
    std::string html((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    file.close();

    if (html.empty()) {
        errors_.push_back("File is empty: " + file_path);
        return false;
    }

    // 使用 ParseHTML 解析
    return ParseHTML(html);
}

LexborElement* LexborDocument::GetBody() {
    if (!document_) {
        return nullptr;
    }
    
    lxb_html_body_element_t* body = lxb_html_document_body_element(document_);
    if (!body) {
        return nullptr;
    }
    
    return new LexborElement(lxb_dom_interface_element(body), this);
}

LexborElement* LexborDocument::GetHead() {
    if (!document_) {
        return nullptr;
    }
    
    lxb_html_head_element_t* head = lxb_html_document_head_element(document_);
    if (!head) {
        return nullptr;
    }
    
    return new LexborElement(lxb_dom_interface_element(head), this);
}

LexborElement* LexborDocument::GetDocumentElement() {
    if (!document_) {
        return nullptr;
    }
    
    lxb_dom_element_t* elem = lxb_dom_document_element(lxb_dom_interface_document(document_));
    if (!elem) {
        return nullptr;
    }
    
    return new LexborElement(elem, this);
}

LexborElement* LexborDocument::GetElementById(const std::string& id) {
    // Lexbor 没有直接的 getElementById API，使用选择器代替
    return QuerySelector("#" + id);
}

bool LexborDocument::InitializeCSS() {
    if (css_initialized_) {
        return true;
    }

    // 创建 CSS 解析器
    css_parser_ = lxb_css_parser_create();
    if (!css_parser_) {
        return false;
    }

    lxb_status_t status = lxb_css_parser_init(css_parser_, nullptr);
    if (status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(css_parser_, true);
        css_parser_ = nullptr;
        return false;
    }

    css_initialized_ = true;
    return true;
}

bool LexborDocument::InitializeSelectors() {
    if (selectors_initialized_) {
        return true;
    }
    
    if (!InitializeCSS()) {
        return false;
    }
    
    // 创建选择器引擎
    selectors_ = lxb_selectors_create();
    if (!selectors_) {
        return false;
    }
    
    lxb_status_t status = lxb_selectors_init(selectors_);
    if (status != LXB_STATUS_OK) {
        lxb_selectors_destroy(selectors_, true);
        selectors_ = nullptr;
        return false;
    }
    
    selectors_initialized_ = true;
    return true;
}

LexborElement* LexborDocument::QuerySelector(const std::string& selector) {
    if (!InitializeSelectors()) {
        return nullptr;
    }
    
    // 解析选择器
    lxb_css_selector_list_t* list = lxb_css_selectors_parse(
        css_parser_,
        reinterpret_cast<const lxb_char_t*>(selector.c_str()),
        selector.length()
    );
    
    if (!list || css_parser_->status != LXB_STATUS_OK) {
        if (list) {
            lxb_css_selector_list_destroy_memory(list);
        }
        return nullptr;
    }
    
    // 查找第一个匹配的元素
    lxb_dom_element_t* result = nullptr;
    
    auto callback = [](lxb_dom_node_t* node, lxb_css_selector_specificity_t /*spec*/, void* ctx) -> lxb_status_t {
        lxb_dom_element_t** result_ptr = static_cast<lxb_dom_element_t**>(ctx);
        *result_ptr = lxb_dom_interface_element(node);
        return LXB_STATUS_STOP;  // 只需要第一个
    };
    
    lxb_dom_node_t* root = lxb_dom_interface_node(document_);
    lxb_selectors_find(selectors_, root, list, callback, &result);
    
    lxb_css_selector_list_destroy_memory(list);
    
    if (!result) {
        return nullptr;
    }
    
    return new LexborElement(result, this);
}

std::vector<LexborElement*> LexborDocument::QuerySelectorAll(const std::string& selector) {
    std::vector<LexborElement*> results;
    
    if (!InitializeSelectors()) {
        return results;
    }
    
    // 解析选择器
    lxb_css_selector_list_t* list = lxb_css_selectors_parse(
        css_parser_,
        reinterpret_cast<const lxb_char_t*>(selector.c_str()),
        selector.length()
    );
    
    if (!list || css_parser_->status != LXB_STATUS_OK) {
        if (list) {
            lxb_css_selector_list_destroy_memory(list);
        }
        return results;
    }
    
    // 查找所有匹配的元素
    auto callback = [](lxb_dom_node_t* node, lxb_css_selector_specificity_t /*spec*/, void* ctx) -> lxb_status_t {
        auto* results_ptr = static_cast<std::vector<lxb_dom_element_t*>*>(ctx);
        results_ptr->push_back(lxb_dom_interface_element(node));
        return LXB_STATUS_OK;
    };
    
    std::vector<lxb_dom_element_t*> native_results;
    lxb_dom_node_t* root = lxb_dom_interface_node(document_);
    lxb_selectors_find(selectors_, root, list, callback, &native_results);
    
    lxb_css_selector_list_destroy_memory(list);
    
    // 转换为 LexborElement
    for (auto* elem : native_results) {
        results.push_back(new LexborElement(elem, this));
    }
    
    return results;
}

bool LexborDocument::AttachStyleSheet(const std::string& css) {
    if (!InitializeCSS()) {
        return false;
    }

    // 解析 CSS 样式表
    lxb_css_stylesheet_t* stylesheet = lxb_css_stylesheet_parse(
        css_parser_,
        reinterpret_cast<const lxb_char_t*>(css.c_str()),
        css.length()
    );

    if (!stylesheet || css_parser_->status != LXB_STATUS_OK) {
        return false;
    }

    // TODO: Lexbor 2.6.0 可能没有 stylesheet_attach API
    // 暂时只解析，不附加
    // 未来版本可能会添加这个功能

    return true;
}

LexborElement* LexborDocument::CreateElement(const std::string& tag_name) {
    if (!document_) {
        return nullptr;
    }
    
    lxb_dom_element_t* elem = lxb_dom_document_create_element(
        lxb_dom_interface_document(document_),
        reinterpret_cast<const lxb_char_t*>(tag_name.c_str()),
        tag_name.length(),
        nullptr
    );
    
    if (!elem) {
        return nullptr;
    }
    
    return new LexborElement(elem, this);
}

LexborText* LexborDocument::CreateTextNode(const std::string& text) {
    if (!document_) {
        return nullptr;
    }
    
    lxb_dom_text_t* text_node = lxb_dom_document_create_text_node(
        lxb_dom_interface_document(document_),
        reinterpret_cast<const lxb_char_t*>(text.c_str()),
        text.length()
    );
    
    if (!text_node) {
        return nullptr;
    }
    
    return new LexborText(text_node, this);
}

std::string LexborDocument::SerializeToHTML() {
    if (!document_) {
        return "";
    }

    std::string result;

    auto callback = [](const lxb_char_t* data, size_t len, void* ctx) -> lxb_status_t {
        auto* str = static_cast<std::string*>(ctx);
        str->append(reinterpret_cast<const char*>(data), len);
        return LXB_STATUS_OK;
    };

    lxb_dom_node_t* root = lxb_dom_interface_node(document_);
    lxb_html_serialize_tree_cb(root, callback, &result);

    return result;
}

std::string LexborDocument::SerializeNode(lxb_dom_node_t* node) {
    if (!node) {
        return "";
    }

    std::string result;

    auto callback = [](const lxb_char_t* data, size_t len, void* ctx) -> lxb_status_t {
        auto* str = static_cast<std::string*>(ctx);
        str->append(reinterpret_cast<const char*>(data), len);
        return LXB_STATUS_OK;
    };

    // 使用 lxb_html_serialize_tree_cb 序列化节点及其子树
    lxb_html_serialize_tree_cb(node, callback, &result);

    return result;
}

void LexborDocument::Walk(std::function<void(LexborElement*)> callback) {
    if (!document_ || !callback) {
        return;
    }
    
    struct Context {
        std::function<void(LexborElement*)> callback;
        LexborDocument* document;
    };
    
    Context ctx{callback, this};
    
    auto walker = [](lxb_dom_node_t* node, void* ctx_ptr) -> lexbor_action_t {
        auto* context = static_cast<Context*>(ctx_ptr);
        
        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            LexborElement elem(lxb_dom_interface_element(node), context->document);
            context->callback(&elem);
        }
        
        return LEXBOR_ACTION_OK;
    };
    
    lxb_dom_node_t* root = lxb_dom_interface_node(document_);
    lxb_dom_node_simple_walk(root, walker, &ctx);
}

// ========== LexborElement 实现 ==========

LexborElement::LexborElement(lxb_dom_element_t* element, LexborDocument* document)
    : element_(element)
    , document_(document) {
}

std::string LexborElement::GetTagName() const {
    if (!element_) {
        return "";
    }
    
    size_t len;
    const lxb_char_t* name = lxb_dom_element_qualified_name(element_, &len);
    
    return std::string(reinterpret_cast<const char*>(name), len);
}

std::string LexborElement::GetAttribute(const std::string& name) const {
    if (!element_) {
        return "";
    }
    
    size_t value_len;
    const lxb_char_t* value = lxb_dom_element_get_attribute(
        element_,
        reinterpret_cast<const lxb_char_t*>(name.c_str()),
        name.length(),
        &value_len
    );
    
    if (!value) {
        return "";
    }
    
    return std::string(reinterpret_cast<const char*>(value), value_len);
}

void LexborElement::SetAttribute(const std::string& name, const std::string& value) {
    if (!element_) {
        return;
    }
    
    lxb_dom_element_set_attribute(
        element_,
        reinterpret_cast<const lxb_char_t*>(name.c_str()),
        name.length(),
        reinterpret_cast<const lxb_char_t*>(value.c_str()),
        value.length()
    );
}

bool LexborElement::HasAttribute(const std::string& name) const {
    if (!element_) {
        return false;
    }
    
    return lxb_dom_element_has_attribute(
        element_,
        reinterpret_cast<const lxb_char_t*>(name.c_str()),
        name.length()
    );
}

void LexborElement::RemoveAttribute(const std::string& name) {
    if (!element_) {
        return;
    }
    
    lxb_dom_element_remove_attribute(
        element_,
        reinterpret_cast<const lxb_char_t*>(name.c_str()),
        name.length()
    );
}

bool LexborElement::HasClass(const std::string& class_name) const {
    std::string classes = GetClassName();

    // 分割 class 列表并精确匹配
    std::string current;
    for (char c : classes) {
        if (c == ' ') {
            if (current == class_name) {
                return true;
            }
            current.clear();
        } else {
            current += c;
        }
    }

    // 检查最后一个 class
    return current == class_name;
}

void LexborElement::AddClass(const std::string& class_name) {
    std::string classes = GetClassName();
    if (classes.empty()) {
        SetClassName(class_name);
    } else if (!HasClass(class_name)) {
        SetClassName(classes + " " + class_name);
    }
}

void LexborElement::RemoveClass(const std::string& class_name) {
    std::string classes = GetClassName();

    // 分割 class 列表
    std::vector<std::string> class_list;
    std::string current;
    for (char c : classes) {
        if (c == ' ') {
            if (!current.empty()) {
                class_list.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        class_list.push_back(current);
    }

    // 移除指定的 class
    std::string result;
    for (const auto& cls : class_list) {
        if (cls != class_name) {
            if (!result.empty()) {
                result += " ";
            }
            result += cls;
        }
    }

    SetClassName(result);
}

std::string LexborElement::GetInnerHTML() const {
    if (!element_ || !document_) {
        return "";
    }

    std::string result;

    auto callback = [](const lxb_char_t* data, size_t len, void* ctx) -> lxb_status_t {
        auto* str = static_cast<std::string*>(ctx);
        str->append(reinterpret_cast<const char*>(data), len);
        return LXB_STATUS_OK;
    };

    // 序列化所有子节点
    lxb_dom_node_t* node = lxb_dom_interface_node(element_);
    lxb_dom_node_t* child = node->first_child;

    while (child) {
        lxb_html_serialize_tree_cb(child, callback, &result);
        child = child->next;
    }

    return result;
}

void LexborElement::SetInnerHTML(const std::string& html) {
    if (!element_ || !document_) {
        return;
    }

    // 清空当前所有子节点
    lxb_dom_node_t* node = lxb_dom_interface_node(element_);
    lxb_dom_node_t* child = node->first_child;

    while (child) {
        lxb_dom_node_t* next = child->next;
        lxb_dom_node_destroy_deep(child);
        child = next;
    }

    node->first_child = nullptr;
    node->last_child = nullptr;

    // 解析HTML片段
    lxb_html_document_t* doc = document_->GetNativeDocument();
    lxb_dom_node_t* fragment = lxb_html_document_parse_fragment(
        doc,
        element_,
        reinterpret_cast<const lxb_char_t*>(html.c_str()),
        html.length()
    );

    if (!fragment) {
        return;
    }

    // 将片段的子节点移动到当前元素
    child = fragment->first_child;
    while (child) {
        lxb_dom_node_t* next = child->next;
        lxb_dom_node_remove(child);
        lxb_dom_node_insert_child(node, child);
        child = next;
    }

    // 销毁片段节点
    lxb_dom_node_destroy(fragment);
}

std::string LexborElement::GetTextContent() const {
    if (!element_) {
        return "";
    }

    std::string result;

    // 递归收集所有文本节点的内容
    std::function<void(lxb_dom_node_t*)> collect_text = [&](lxb_dom_node_t* node) {
        if (!node) return;

        if (node->type == LXB_DOM_NODE_TYPE_TEXT) {
            lxb_dom_character_data_t* char_data = lxb_dom_interface_character_data(node);
            if (char_data && char_data->data.data) {
                result.append(
                    reinterpret_cast<const char*>(char_data->data.data),
                    char_data->data.length
                );
            }
        }

        // 递归处理子节点
        lxb_dom_node_t* child = node->first_child;
        while (child) {
            collect_text(child);
            child = child->next;
        }
    };

    lxb_dom_node_t* node = lxb_dom_interface_node(element_);
    collect_text(node);

    return result;
}

void LexborElement::SetTextContent(const std::string& text) {
    if (!element_ || !document_) {
        return;
    }

    // 清空当前所有子节点
    lxb_dom_node_t* node = lxb_dom_interface_node(element_);
    lxb_dom_node_t* child = node->first_child;

    while (child) {
        lxb_dom_node_t* next = child->next;
        lxb_dom_node_destroy_deep(child);
        child = next;
    }

    node->first_child = nullptr;
    node->last_child = nullptr;

    // 创建新的文本节点
    if (!text.empty()) {
        lxb_html_document_t* doc = document_->GetNativeDocument();
        lxb_dom_text_t* text_node = lxb_dom_document_create_text_node(
            lxb_dom_interface_document(doc),
            reinterpret_cast<const lxb_char_t*>(text.c_str()),
            text.length()
        );

        if (text_node) {
            lxb_dom_node_insert_child(node, lxb_dom_interface_node(text_node));
        }
    }
}

LexborElement* LexborElement::GetParentElement() {
    if (!element_) {
        return nullptr;
    }

    lxb_dom_node_t* node = lxb_dom_interface_node(element_);
    lxb_dom_node_t* parent = node->parent;

    if (!parent || parent->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return nullptr;
    }

    return new LexborElement(lxb_dom_interface_element(parent), document_);
}

std::vector<LexborElement*> LexborElement::GetChildren() {
    std::vector<LexborElement*> children;

    if (!element_) {
        return children;
    }

    lxb_dom_node_t* node = lxb_dom_interface_node(element_);
    lxb_dom_node_t* child = node->first_child;

    while (child) {
        if (child->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            children.push_back(new LexborElement(lxb_dom_interface_element(child), document_));
        }
        child = child->next;
    }

    return children;
}

LexborElement* LexborElement::GetFirstChild() {
    if (!element_) {
        return nullptr;
    }

    lxb_dom_node_t* node = lxb_dom_interface_node(element_);
    lxb_dom_node_t* child = node->first_child;

    while (child) {
        if (child->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            return new LexborElement(lxb_dom_interface_element(child), document_);
        }
        child = child->next;
    }

    return nullptr;
}

LexborElement* LexborElement::GetLastChild() {
    if (!element_) {
        return nullptr;
    }

    lxb_dom_node_t* node = lxb_dom_interface_node(element_);
    lxb_dom_node_t* child = node->last_child;

    while (child) {
        if (child->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            return new LexborElement(lxb_dom_interface_element(child), document_);
        }
        child = child->prev;
    }

    return nullptr;
}

void LexborElement::AppendChild(LexborElement* child) {
    if (!element_ || !child || !child->element_) {
        return;
    }

    lxb_dom_node_t* parent_node = lxb_dom_interface_node(element_);
    lxb_dom_node_t* child_node = lxb_dom_interface_node(child->element_);

    lxb_dom_node_insert_child(parent_node, child_node);
}

void LexborElement::RemoveChild(LexborElement* child) {
    if (!element_ || !child || !child->element_) {
        return;
    }

    lxb_dom_node_t* child_node = lxb_dom_interface_node(child->element_);
    lxb_dom_node_remove(child_node);
}

// ========== LexborText 实现 ==========

LexborText::LexborText(lxb_dom_text_t* text, LexborDocument* document)
    : text_(text)
    , document_(document) {
}

std::string LexborText::GetData() const {
    if (!text_) {
        return "";
    }
    
    lxb_dom_character_data_t* char_data = lxb_dom_interface_character_data(text_);
    if (!char_data || !char_data->data.data) {
        return "";
    }
    
    return std::string(
        reinterpret_cast<const char*>(char_data->data.data),
        char_data->data.length
    );
}

void LexborText::SetData(const std::string& data) {
    if (!text_) {
        return;
    }

    lxb_dom_character_data_t* char_data = lxb_dom_interface_character_data(text_);
    if (!char_data) {
        return;
    }

    lexbor_str_t* str = &char_data->data;
    lexbor_str_clean(str);
    lexbor_str_append(
        str,
        char_data->node.owner_document->text,
        reinterpret_cast<const lxb_char_t*>(data.c_str()),
        data.length()
    );
}

// ========== LexborDocument 增强功能 ==========

std::string LexborDocument::GetDocumentMode() const {
    if (!document_) {
        return "unknown";
    }

    lxb_dom_document_t* dom_doc = lxb_dom_interface_document(document_);
    if (!dom_doc) {
        return "unknown";
    }

    // 检查文档模式
    switch (dom_doc->compat_mode) {
        case LXB_DOM_DOCUMENT_CMODE_NO_QUIRKS:
            return "no-quirks";
        case LXB_DOM_DOCUMENT_CMODE_QUIRKS:
            return "quirks";
        case LXB_DOM_DOCUMENT_CMODE_LIMITED_QUIRKS:
            return "limited-quirks";
        default:
            return "unknown";
    }
}

bool LexborDocument::IsQuirksMode() const {
    if (!document_) {
        return false;
    }

    lxb_dom_document_t* dom_doc = lxb_dom_interface_document(document_);
    if (!dom_doc) {
        return false;
    }

    return dom_doc->compat_mode == LXB_DOM_DOCUMENT_CMODE_QUIRKS;
}

std::string LexborDocument::GetDoctype() const {
    if (!document_) {
        return "";
    }

    lxb_dom_document_t* dom_doc = lxb_dom_interface_document(document_);
    if (!dom_doc) {
        return "";
    }

    // 遍历文档的子节点查找 DOCTYPE
    lxb_dom_node_t* node = lxb_dom_interface_node(dom_doc);
    if (!node) {
        return "";
    }

    lxb_dom_node_t* child = node->first_child;
    while (child) {
        if (child->type == LXB_DOM_NODE_TYPE_DOCUMENT_TYPE) {
            size_t name_len;
            const lxb_char_t* name = lxb_dom_node_name(child, &name_len);
            if (name) {
                return std::string(reinterpret_cast<const char*>(name), name_len);
            }
        }
        child = child->next;
    }

    return "";
}

LexborElement* LexborElement::QuerySelector(const std::string& selector) {
    if (!element_ || !document_) {
        return nullptr;
    }

    // 使用文档的选择器引擎
    if (!document_->InitializeSelectors()) {
        return nullptr;
    }

    // 解析选择器
    lxb_css_selector_list_t* list = lxb_css_selectors_parse(
        document_->css_parser_,
        reinterpret_cast<const lxb_char_t*>(selector.c_str()),
        selector.length()
    );

    if (!list || document_->css_parser_->status != LXB_STATUS_OK) {
        if (list) {
            lxb_css_selector_list_destroy_memory(list);
        }
        return nullptr;
    }

    // 查找第一个匹配的元素（从当前元素开始）
    lxb_dom_element_t* result = nullptr;

    auto callback = [](lxb_dom_node_t* node, lxb_css_selector_specificity_t /*spec*/, void* ctx) -> lxb_status_t {
        lxb_dom_element_t** result_ptr = static_cast<lxb_dom_element_t**>(ctx);
        *result_ptr = lxb_dom_interface_element(node);
        return LXB_STATUS_STOP;  // 只需要第一个
    };

    lxb_dom_node_t* node = lxb_dom_interface_node(element_);
    lxb_selectors_find(document_->selectors_, node, list, callback, &result);

    lxb_css_selector_list_destroy_memory(list);

    if (result) {
        return new LexborElement(result, document_);
    }

    return nullptr;
}

std::vector<LexborElement*> LexborElement::QuerySelectorAll(const std::string& selector) {
    std::vector<LexborElement*> results;

    if (!element_ || !document_) {
        return results;
    }

    // 使用文档的选择器引擎
    if (!document_->InitializeSelectors()) {
        return results;
    }

    // 解析选择器
    lxb_css_selector_list_t* list = lxb_css_selectors_parse(
        document_->css_parser_,
        reinterpret_cast<const lxb_char_t*>(selector.c_str()),
        selector.length()
    );

    if (!list || document_->css_parser_->status != LXB_STATUS_OK) {
        if (list) {
            lxb_css_selector_list_destroy_memory(list);
        }
        return results;
    }

    // 查找所有匹配的元素（从当前元素开始）
    auto callback = [](lxb_dom_node_t* node, lxb_css_selector_specificity_t /*spec*/, void* ctx) -> lxb_status_t {
        auto* results_ptr = static_cast<std::vector<LexborElement*>*>(ctx);
        lxb_dom_element_t* elem = lxb_dom_interface_element(node);

        // 需要获取 document 指针，这里我们从第一个元素获取
        if (!results_ptr->empty()) {
            results_ptr->push_back(new LexborElement(elem, (*results_ptr)[0]->document_));
        }

        return LXB_STATUS_OK;  // 继续查找
    };

    lxb_dom_node_t* node = lxb_dom_interface_node(element_);

    // 先添加一个临时元素以便回调函数能获取 document 指针
    results.push_back(new LexborElement(element_, document_));

    lxb_selectors_find(document_->selectors_, node, list, callback, &results);

    // 移除临时元素
    delete results[0];
    results.erase(results.begin());

    lxb_css_selector_list_destroy_memory(list);

    return results;
}

} // namespace lightui

