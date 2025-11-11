/**
 * @file lexbor_document.cpp
 * @brief Lexbor Document C++ 包装类实现
 */

#include "lexbor_document.h"
#include <cstring>
#include <sstream>
#include <vector>

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
        return false;
    }
    
    lxb_status_t status = lxb_html_document_parse(
        document_,
        reinterpret_cast<const lxb_char_t*>(html.c_str()),
        html.length()
    );
    
    return status == LXB_STATUS_OK;
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
    
    auto callback = [](lxb_dom_node_t* node, lxb_css_selector_specificity_t spec, void* ctx) -> lxb_status_t {
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
    auto callback = [](lxb_dom_node_t* node, lxb_css_selector_specificity_t spec, void* ctx) -> lxb_status_t {
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
    return classes.find(class_name) != std::string::npos;
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

} // namespace lightui

