/**
 * @file element.cpp
 * @brief DOM元素类实现
 */

#include "element.h"
#include "text.h"
#include "event.h"
#include "document.h"
#include "dom_observer.h"
#include "selector_engine.h"
#include "dom_token_list.h"
#include "css_style_declaration.h"
#include "dom_string_map.h"
#include "html_input_element.h"
#include "html_textarea_element.h"
#include "html_button_element.h"
#include "html_form_element.h"
#include "html_select_element.h"
#include "html_option_element.h"
#include <iostream>
#include "html_anchor_element.h"
#include "html_label_element.h"
#include "html_image_element.h"
#include "html_div_element.h"
#include "html_span_element.h"
#include "html_paragraph_element.h"
#include "html_heading_element.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <lexbor/html/interfaces/document.h>
#include <lexbor/html/serialize.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/text.h>
#include "core/lexbor/lexbor_document.h"

namespace lightui {

// 初始化静态成员
uint64_t Element::next_listener_id_ = 1;

// ========== 构造函数 ==========

Element::Element(const std::string& tag_name)
    : Node(NodeType::ELEMENT_NODE)
    , tag_name_(tag_name)
    , attributes_()
    , styles_()
    , event_listeners_() {
}

// ========== 属性操作 ==========

void Element::SetAttribute(const std::string& name, const std::string& value) {
    std::string old_value = GetAttribute(name);

    // 如果值没有改变，直接返回
    if (old_value == value) {
        return;
    }

    attributes_[name] = value;

    // 智能脏标记：根据属性类型精确标记
    if (IsLayoutAttribute(name)) {
        // 布局属性改变：需要重新布局和绘制
        MarkDirty(DirtyType::LAYOUT | DirtyType::PAINT);
    } else if (IsStyleAttribute(name)) {
        // 样式属性改变：只需要重新绘制
        MarkDirty(DirtyType::PAINT);
    } else {
        // 其他属性：保守标记为全部脏
        MarkDirty(DirtyType::ALL);
    }

    // 特殊处理 id 属性
    if (name == "id") {
        auto doc = GetOwnerDocument();
        if (doc) {
            // 取消注册旧 ID
            if (!old_value.empty()) {
                doc->UnregisterElementId(old_value);
            }
            // 注册新 ID
            if (!value.empty()) {
                doc->RegisterElementId(value, std::static_pointer_cast<Element>(shared_from_this()));
            }
        }
    }

    // 通知观察者
    auto doc = GetOwnerDocument();
    if (doc) {
        doc->GetObserverManager().NotifyAttributeChanged(this, name, old_value, value);
    }

    // 标记 Lexbor 需要同步
    MarkLexborDirty();
}

std::string Element::GetAttribute(const std::string& name) const {
    auto it = attributes_.find(name);
    if (it != attributes_.end()) {
        return it->second;
    }
    return "";
}

bool Element::HasAttribute(const std::string& name) const {
    return attributes_.find(name) != attributes_.end();
}

const std::unordered_map<std::string, std::string>& Element::GetAllAttributes() const {
    return attributes_;
}

void Element::RemoveAttribute(const std::string& name) {
    // 获取旧值
    std::string old_value = GetAttribute(name);

    // 移除属性
    attributes_.erase(name);
    MarkDirty();
    MarkLexborDirty();

    // 通知观察者（只有当属性存在时才通知）
    if (!old_value.empty()) {
        auto doc = GetOwnerDocument();
        if (doc) {
            doc->GetObserverManager().NotifyAttributeChanged(this, name, old_value, "");
        }
    }
}

// ========== 样式操作 ==========

std::string Element::GetClassName() const {
    return GetAttribute("class");
}

void Element::SetClassName(const std::string& class_name) {
    SetAttribute("class", class_name);
}

void Element::AddClass(const std::string& class_name) {
    std::string current = GetClassName();
    if (current.empty()) {
        SetClassName(class_name);
        return;
    }

    // 检查是否已存在
    if (HasClass(class_name)) {
        return;
    }

    SetClassName(current + " " + class_name);
}

void Element::RemoveClass(const std::string& class_name) {
    std::string current = GetClassName();
    if (current.empty()) {
        return;
    }

    // 分割class列表
    std::istringstream iss(current);
    std::string token;
    std::string result;

    while (iss >> token) {
        if (token != class_name) {
            if (!result.empty()) {
                result += " ";
            }
            result += token;
        }
    }

    SetClassName(result);
}

bool Element::ToggleClass(const std::string& class_name) {
    if (HasClass(class_name)) {
        RemoveClass(class_name);
        return false;
    } else {
        AddClass(class_name);
        return true;
    }
}

bool Element::HasClass(const std::string& class_name) const {
    std::string current = GetClassName();
    if (current.empty()) {
        return false;
    }

    // 分割class列表
    std::istringstream iss(current);
    std::string token;

    while (iss >> token) {
        if (token == class_name) {
            return true;
        }
    }

    return false;
}

std::shared_ptr<DOMTokenList> Element::GetClassList() {
    // 懒加载：第一次调用时创建
    if (!class_list_) {
        class_list_ = std::make_shared<DOMTokenList>(
            std::static_pointer_cast<Element>(shared_from_this()),
            "class"
        );
    }
    return class_list_;
}

void Element::SetStyle(const std::string& property, const std::string& value) {
    std::string old_value = GetStyle(property);
    styles_[property] = value;
    MarkDirty();

    // 通知观察者
    auto doc = GetOwnerDocument();
    if (doc) {
        doc->GetObserverManager().NotifyStyleChanged(this, property, old_value, value);
    }
}

std::string Element::GetStyle(const std::string& property) const {
    auto it = styles_.find(property);
    if (it != styles_.end()) {
        return it->second;
    }
    return "";
}

std::shared_ptr<CSSStyleDeclaration> Element::GetStyleDeclaration() {
    // 懒加载：第一次调用时创建
    if (!style_declaration_) {
        style_declaration_ = std::make_shared<CSSStyleDeclaration>(
            std::static_pointer_cast<Element>(shared_from_this())
        );

        // 从style属性初始化
        std::string style_attr = GetAttribute("style");
        if (!style_attr.empty()) {
            style_declaration_->SetCssText(style_attr);
        }
    }
    return style_declaration_;
}

std::shared_ptr<DOMStringMap> Element::GetDataset() {
    // 懒加载：第一次调用时创建
    if (!dataset_) {
        dataset_ = std::make_shared<DOMStringMap>(
            std::static_pointer_cast<Element>(shared_from_this())
        );
    }
    return dataset_;
}

// ========== 克隆和文本内容 ==========

std::shared_ptr<Node> Element::CloneNode(bool deep) {
    // 参考：RmlUi/Source/Core/Element.cpp - Clone
    // 参考：Lexbor lxb_dom_element_interface_copy

    auto cloned = std::make_shared<Element>(tag_name_);

    // 复制属性
    cloned->attributes_ = attributes_;

    // 复制样式
    cloned->styles_ = styles_;

    // 复制CSS伪类状态（注意：某些伪类如:hover不应该被复制）
    // 只复制持久性伪类，不复制交互性伪类
    for (const auto& [pseudo_class, active] : pseudo_classes_) {
        // 跳过交互性伪类（这些应该由用户交互触发）
        if (pseudo_class == "hover" ||
            pseudo_class == "active" ||
            pseudo_class == "focus" ||
            pseudo_class == "focus-visible" ||
            pseudo_class == "drag") {
            continue;
        }

        // 复制其他伪类
        cloned->pseudo_classes_[pseudo_class] = active;
    }

    // 深度克隆子节点
    if (deep) {
        for (const auto& child : child_nodes_) {
            auto cloned_child = child->CloneNode(true);
            cloned->AppendChild(cloned_child);
        }
    }

    return cloned;
}

std::string Element::GetTextContent() const {
    return Node::GetTextContent();
}

void Element::SetTextContent(const std::string& content) {
    Node::SetTextContent(content);
}

// ========== 事件监听 ==========

uint64_t Element::AddEventListener(const std::string& type, EventListener listener, bool use_capture, bool once) {
    // 分配唯一ID
    uint64_t listener_id = next_listener_id_++;

    // 创建EventListenerEntry并添加到列表（包含once选项）
    event_listeners_[type].emplace_back(listener_id, std::move(listener), use_capture, once);

    return listener_id;
}

bool Element::RemoveEventListener(const std::string& type, uint64_t listener_id) {
    auto it = event_listeners_.find(type);
    if (it == event_listeners_.end()) {
        return false;
    }

    // 查找并移除指定ID的监听器
    auto& listeners = it->second;
    for (auto listener_it = listeners.begin(); listener_it != listeners.end(); ++listener_it) {
        if (listener_it->id == listener_id) {
            listeners.erase(listener_it);

            // 如果该事件类型没有监听器了，移除整个条目
            if (listeners.empty()) {
                event_listeners_.erase(it);
            }

            return true;
        }
    }

    return false;
}

bool Element::DispatchEvent(std::shared_ptr<Event> event) {
    if (!event) {
        return false;
    }

    // 设置事件目标
    event->SetTarget(shared_from_this());

    // 1. 构建事件传播路径（从目标到根）
    std::vector<std::shared_ptr<Node>> path;
    for (auto node = std::static_pointer_cast<Node>(shared_from_this());
         node;
         node = node->GetParentNode()) {
        path.push_back(node);
    }

    // 2. 捕获阶段（从根到目标，不包括目标）
    // 参考：RmlUi/Source/Core/Element.cpp - AddEventListener
    event->SetEventPhase(EventPhase::CAPTURING_PHASE);
    for (int i = static_cast<int>(path.size()) - 1; i > 0; --i) {
        if (event->IsPropagationStopped()) {
            break;
        }

        auto element = std::dynamic_pointer_cast<Element>(path[i]);
        if (element) {
            event->SetCurrentTarget(element);
            element->HandleEvent(event, true);  // use_capture = true
        }
    }

    // 3. 目标阶段
    if (!event->IsPropagationStopped()) {
        event->SetEventPhase(EventPhase::AT_TARGET);
        event->SetCurrentTarget(shared_from_this());
        // 在目标阶段，同时触发捕获和冒泡监听器
        HandleEvent(event, true);   // 先触发捕获监听器
        HandleEvent(event, false);  // 再触发冒泡监听器
    }

    // 4. 冒泡阶段（从目标的父节点到根）
    if (event->GetBubbles() && !event->IsPropagationStopped()) {
        event->SetEventPhase(EventPhase::BUBBLING_PHASE);
        for (size_t i = 1; i < path.size(); ++i) {
            if (event->IsPropagationStopped()) {
                break;
            }

            auto element = std::dynamic_pointer_cast<Element>(path[i]);
            if (element) {
                event->SetCurrentTarget(element);
                element->HandleEvent(event, false);  // use_capture = false
            }
        }
    }

    return !event->IsDefaultPrevented();
}

void Element::HandleEvent(std::shared_ptr<Event> event, bool use_capture) {
    auto it = event_listeners_.find(event->GetType());
    if (it == event_listeners_.end()) {
        return;
    }

    // 收集需要移除的once监听器ID
    std::vector<uint64_t> once_listeners_to_remove;

    // 调用匹配捕获阶段的监听器
    // 参考：RmlUi的事件分发机制
    for (const auto& entry : it->second) {
        // 只调用匹配当前阶段的监听器
        if (entry.use_capture != use_capture) {
            continue;
        }

        if (event->IsImmediatePropagationStopped()) {
            break;
        }

        // 调用监听器
        entry.listener(event);

        // 如果是once监听器，标记为待移除
        if (entry.once) {
            once_listeners_to_remove.push_back(entry.id);
        }
    }

    // 移除once监听器
    for (uint64_t listener_id : once_listeners_to_remove) {
        RemoveEventListener(event->GetType(), listener_id);
    }
}

// ========== 查询选择器 ==========

std::shared_ptr<Element> Element::QuerySelector(const std::string& selector) {
    auto self = std::static_pointer_cast<Element>(shared_from_this());
    return SelectorEngine::QuerySelector(self, selector);
}

std::vector<std::shared_ptr<Element>> Element::QuerySelectorAll(const std::string& selector) {
    auto self = std::static_pointer_cast<Element>(shared_from_this());
    return SelectorEngine::QuerySelectorAll(self, selector);
}

bool Element::Matches(const std::string& selector) const {
    // 需要 const_cast 因为 SelectorEngine 需要 shared_ptr
    auto self = std::static_pointer_cast<Element>(
        const_cast<Element*>(this)->shared_from_this());
    return SelectorEngine::Matches(self, selector);
}

std::shared_ptr<Element> Element::Closest(const std::string& selector) {
    auto self = std::static_pointer_cast<Element>(shared_from_this());
    return SelectorEngine::Closest(self, selector);
}

// ========== CSS伪类支持（参考RmlUi） ==========

void Element::SetPseudoClass(const std::string& pseudo_class, bool activate) {
    // 参考：RmlUi/Source/Core/Element.cpp - SetPseudoClass
    // 参考：Chrome/Blink - 伪类是浏览器内部状态，不应触发框架重新渲染

    bool current_state = HasPseudoClass(pseudo_class);

    // 状态没有变化，直接返回
    if (current_state == activate) {
        return;
    }

    // 更新伪类状态
    if (activate) {
        pseudo_classes_[pseudo_class] = true;
    } else {
        pseudo_classes_.erase(pseudo_class);
    }

    // 标记需要重新计算样式和重绘
    // 伪类变化需要样式重计算（如:focus, :hover改变边框颜色）
    MarkDirty(DirtyType::STYLE | DirtyType::PAINT);

    // 注意：不通知 DOM 观察者，因为伪类是浏览器内部状态
    // 这样可以避免 React/Preact 等框架因伪类变化而重新渲染整个组件树
    // 参考 Chrome 行为：:focus, :hover, :active 等伪类变化不会触发 MutationObserver
}

bool Element::HasPseudoClass(const std::string& pseudo_class) const {
    auto it = pseudo_classes_.find(pseudo_class);
    return it != pseudo_classes_.end() && it->second;
}

std::vector<std::string> Element::GetActivePseudoClasses() const {
    std::vector<std::string> result;
    for (const auto& pair : pseudo_classes_) {
        if (pair.second) {
            result.push_back(pair.first);
        }
    }
    return result;
}

// ========== HTML内容操作 ==========

std::string Element::GetInnerHTML() const {
    // 序列化所有子节点为HTML字符串
    std::ostringstream html;

    for (const auto& child : GetChildNodes()) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            // 元素节点：递归获取outerHTML
            auto element = std::static_pointer_cast<Element>(child);
            html << element->GetOuterHTML();
        } else if (child->GetNodeType() == NodeType::TEXT_NODE) {
            // 文本节点：直接输出文本内容（需要HTML转义）
            auto text = std::static_pointer_cast<Text>(child);
            std::string content = text->GetData();

            // HTML转义
            std::string escaped;
            for (char c : content) {
                switch (c) {
                    case '<': escaped += "&lt;"; break;
                    case '>': escaped += "&gt;"; break;
                    case '&': escaped += "&amp;"; break;
                    case '"': escaped += "&quot;"; break;
                    case '\'': escaped += "&#39;"; break;
                    default: escaped += c; break;
                }
            }
            html << escaped;
        }
    }

    return html.str();
}

void Element::SetInnerHTML(const std::string& html) {
    // 清空所有子节点
    auto children = GetChildNodes();  // 复制一份，避免迭代时修改
    for (const auto& child : children) {
        RemoveChild(child);
    }

    if (html.empty()) {
        MarkDirty();
        return;
    }

    // 使用Lexbor解析HTML片段
    // 获取Document以访问Lexbor文档
    auto doc = std::dynamic_pointer_cast<Document>(GetOwnerDocument());
    if (!doc) {
        // 如果没有Document，回退到简单的文本节点
        auto text_node = std::make_shared<Text>(html);
        AppendChild(text_node);
        MarkDirty();
        return;
    }

    // 创建临时Lexbor文档用于解析
    lxb_html_document_t* temp_doc = lxb_html_document_create();
    if (!temp_doc) {
        return;
    }

    // 创建一个临时元素作为解析上下文
    lxb_dom_element_t* temp_elem = lxb_dom_document_create_element(
        &temp_doc->dom_document,
        reinterpret_cast<const lxb_char_t*>(tag_name_.c_str()),
        tag_name_.length(),
        nullptr
    );

    if (!temp_elem) {
        lxb_html_document_destroy(temp_doc);
        return;
    }

    // 解析HTML片段
    lxb_dom_node_t* fragment = lxb_html_document_parse_fragment(
        temp_doc,
        temp_elem,
        reinterpret_cast<const lxb_char_t*>(html.c_str()),
        html.length()
    );

    if (fragment) {
        // 遍历解析结果的子节点，转换为我们的Node对象
        lxb_dom_node_t* child = fragment->first_child;
        while (child) {
            std::shared_ptr<Node> new_node = ConvertLexborNodeToNode(child, doc);
            if (new_node) {
                AppendChild(new_node);
            }
            child = child->next;
        }
    }

    // 清理临时文档
    lxb_html_document_destroy(temp_doc);

    MarkDirty();
}

std::string Element::GetOuterHTML() const {
    // 序列化元素自身及其所有子节点为HTML字符串
    std::ostringstream html;

    // 开始标签
    html << "<" << tag_name_;

    // 属性
    for (const auto& attr : attributes_) {
        html << " " << attr.first << "=\"";

        // HTML转义属性值
        std::string escaped;
        for (char c : attr.second) {
            switch (c) {
                case '"': escaped += "&quot;"; break;
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                default: escaped += c; break;
            }
        }
        html << escaped << "\"";
    }

    // 自闭合标签检查
    bool is_void_element = (
        tag_name_ == "area" || tag_name_ == "base" || tag_name_ == "br" ||
        tag_name_ == "col" || tag_name_ == "embed" || tag_name_ == "hr" ||
        tag_name_ == "img" || tag_name_ == "input" || tag_name_ == "link" ||
        tag_name_ == "meta" || tag_name_ == "param" || tag_name_ == "source" ||
        tag_name_ == "track" || tag_name_ == "wbr"
    );

    if (is_void_element && GetChildNodes().empty()) {
        // 自闭合标签
        html << " />";
    } else {
        // 结束开始标签
        html << ">";

        // 子节点内容
        html << GetInnerHTML();

        // 结束标签
        html << "</" << tag_name_ << ">";
    }

    return html.str();
}

void Element::SetOuterHTML(const std::string& html) {
    // 替换元素自身
    // 需要在父节点中替换
    auto parent = GetParentNode();
    if (!parent) {
        throw std::runtime_error("Cannot set outerHTML on element without parent");
    }

    if (html.empty()) {
        parent->RemoveChild(shared_from_this());
        return;
    }

    // 使用Lexbor解析HTML片段
    auto doc = std::dynamic_pointer_cast<Document>(GetOwnerDocument());
    if (!doc) {
        // 尝试从父节点获取document
        if (parent) {
            doc = std::dynamic_pointer_cast<Document>(parent->GetOwnerDocument());
        }
    }

    if (!doc) {
        // 改为返回而不是抛出异常，避免崩溃
        std::cerr << "Warning: Cannot set outerHTML without document" << std::endl;
        return;
    }

    // 创建临时Lexbor文档用于解析
    lxb_html_document_t* temp_doc = lxb_html_document_create();
    if (!temp_doc) {
        throw std::runtime_error("Failed to create temporary document");
    }

    // 创建body元素作为解析上下文
    lxb_dom_element_t* temp_elem = lxb_dom_document_create_element(
        &temp_doc->dom_document,
        reinterpret_cast<const lxb_char_t*>("body"),
        4,
        nullptr
    );

    if (!temp_elem) {
        lxb_html_document_destroy(temp_doc);
        throw std::runtime_error("Failed to create temporary element");
    }

    // 解析HTML片段
    lxb_dom_node_t* fragment = lxb_html_document_parse_fragment(
        temp_doc,
        temp_elem,
        reinterpret_cast<const lxb_char_t*>(html.c_str()),
        html.length()
    );

    if (fragment) {
        // 收集所有新节点
        std::vector<std::shared_ptr<Node>> new_nodes;
        lxb_dom_node_t* child = fragment->first_child;
        while (child) {
            std::shared_ptr<Node> new_node = ConvertLexborNodeToNode(child, doc);
            if (new_node) {
                new_nodes.push_back(new_node);
            }
            child = child->next;
        }

        // 替换当前元素为所有新节点
        if (!new_nodes.empty()) {
            // 先用第一个节点替换当前元素
            parent->ReplaceChild(new_nodes[0], shared_from_this());

            // 然后在第一个节点后面插入其余节点
            for (size_t i = 1; i < new_nodes.size(); ++i) {
                parent->InsertBefore(new_nodes[i], new_nodes[i-1]->GetNextSibling());
            }
        }
    }

    // 清理临时文档
    lxb_html_document_destroy(temp_doc);
}

// ========== 辅助函数 ==========

std::shared_ptr<Node> Element::ConvertLexborNodeToNode(lxb_dom_node_t* lexbor_node,
                                                        std::shared_ptr<Document> doc) {
    if (!lexbor_node) {
        return nullptr;
    }

    if (lexbor_node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        // 元素节点
        lxb_dom_element_t* lexbor_elem = lxb_dom_interface_element(lexbor_node);

        // 获取标签名
        size_t tag_name_len;
        const lxb_char_t* tag_name_data = lxb_dom_element_qualified_name(lexbor_elem, &tag_name_len);
        std::string tag_name(reinterpret_cast<const char*>(tag_name_data), tag_name_len);

        // 使用 Document::CreateElement 创建元素，这样会自动设置正确的类型和 owner_document
        std::shared_ptr<Element> new_elem;
        if (doc) {
            new_elem = doc->CreateElement(tag_name);
        } else {
            // 如果没有 document，回退到直接创建（但这种情况下 owner_document 会是 null）
            new_elem = std::make_shared<Element>(tag_name);
        }

        // 复制属性
        lxb_dom_attr_t* attr = lexbor_elem->first_attr;
        while (attr) {
            size_t attr_name_len;
            const lxb_char_t* attr_name_data = lxb_dom_attr_qualified_name(attr, &attr_name_len);
            std::string attr_name(reinterpret_cast<const char*>(attr_name_data), attr_name_len);

            size_t attr_value_len;
            const lxb_char_t* attr_value_data = lxb_dom_attr_value(attr, &attr_value_len);
            std::string attr_value;
            if (attr_value_data) {
                attr_value = std::string(reinterpret_cast<const char*>(attr_value_data), attr_value_len);
            }

            new_elem->SetAttribute(attr_name, attr_value);

            attr = attr->next;
        }

        // 递归处理子节点
        lxb_dom_node_t* child = lexbor_node->first_child;
        while (child) {
            std::shared_ptr<Node> child_node = ConvertLexborNodeToNode(child, doc);
            if (child_node) {
                new_elem->AppendChild(child_node);
            }
            child = child->next;
        }

        return new_elem;

    } else if (lexbor_node->type == LXB_DOM_NODE_TYPE_TEXT) {
        // 文本节点
        lxb_dom_text_t* lexbor_text = lxb_dom_interface_text(lexbor_node);

        size_t text_len;
        const lxb_char_t* text_data = lxb_dom_node_text_content(lexbor_node, &text_len);
        std::string text;
        if (text_data) {
            text = std::string(reinterpret_cast<const char*>(text_data), text_len);
        }

        auto new_text = std::make_shared<Text>(text);
        // 注意：owner_document会在AppendChild时自动设置

        return new_text;
    }

    // 其他类型节点暂不支持
    return nullptr;
}

// ========== 智能脏标记辅助方法 ==========

bool Element::IsLayoutAttribute(const std::string& name) {
    // 影响布局的属性（会改变元素的位置、大小、排列）
    static const std::unordered_set<std::string> layout_attrs = {
        // 尺寸相关
        "width", "height", "min-width", "max-width", "min-height", "max-height",

        // 间距相关
        "padding", "padding-top", "padding-right", "padding-bottom", "padding-left",
        "margin", "margin-top", "margin-right", "margin-bottom", "margin-left",

        // 边框相关（影响盒模型尺寸）
        "border", "border-width", "border-top-width", "border-right-width",
        "border-bottom-width", "border-left-width",

        // 定位相关
        "position", "top", "right", "bottom", "left",
        "display", "visibility", "float", "clear",

        // Flexbox相关
        "flex", "flex-direction", "flex-wrap", "flex-flow",
        "justify-content", "align-items", "align-content", "align-self",
        "flex-grow", "flex-shrink", "flex-basis", "order",

        // Grid相关
        "grid", "grid-template-columns", "grid-template-rows",
        "grid-column", "grid-row", "grid-area",
        "gap", "row-gap", "column-gap",

        // 文本布局相关
        "font-size", "line-height", "letter-spacing", "word-spacing",
        "white-space", "text-align", "vertical-align",

        // 其他布局属性
        "overflow", "overflow-x", "overflow-y",
        "box-sizing", "z-index"
    };

    return layout_attrs.count(name) > 0;
}

bool Element::IsStyleAttribute(const std::string& name) {
    // 只影响样式的属性（不改变布局，只改变外观）
    static const std::unordered_set<std::string> style_attrs = {
        // 颜色相关
        "color", "background-color", "background", "background-image",
        "background-position", "background-size", "background-repeat",

        // 边框样式（不影响尺寸）
        "border-color", "border-style", "border-radius",
        "border-top-color", "border-right-color", "border-bottom-color", "border-left-color",
        "border-top-style", "border-right-style", "border-bottom-style", "border-left-style",

        // 阴影和效果
        "box-shadow", "text-shadow", "opacity", "filter",

        // 文本样式
        "font-family", "font-weight", "font-style", "text-decoration",
        "text-transform", "text-overflow",

        // 光标和用户交互
        "cursor", "pointer-events", "user-select",

        // 动画和过渡
        "transition", "animation", "transform"
    };

    return style_attrs.count(name) > 0;
}

// ========== DOM 同步机制 ==========

void Element::SyncToLexbor() {
    // 如果不需要同步，直接返回
    if (!lexbor_dirty_) {
        return;
    }

    // 获取 owner document
    auto doc = std::dynamic_pointer_cast<Document>(GetOwnerDocument());
    if (!doc || !doc->GetLexborDocument()) {
        return;
    }

    auto lexbor_doc = doc->GetLexborDocument();
    auto native_doc = lexbor_doc->GetNativeDocument();
    if (!native_doc) {
        return;
    }

    // 如果还没有 Lexbor 元素，创建一个
    if (!lexbor_element_) {
        lexbor_element_ = reinterpret_cast<lxb_dom_node_t*>(
            lxb_dom_document_create_element(
                lxb_dom_interface_document(native_doc),
                reinterpret_cast<const lxb_char_t*>(tag_name_.c_str()),
                tag_name_.length(),
                nullptr
            )
        );

        if (!lexbor_element_) {
            return;
        }
    }

    // 同步属性
    lxb_dom_element_t* lexbor_elem = reinterpret_cast<lxb_dom_element_t*>(lexbor_element_);
    for (const auto& [name, value] : attributes_) {
        lxb_dom_element_set_attribute(
            lexbor_elem,
            reinterpret_cast<const lxb_char_t*>(name.c_str()),
            name.length(),
            reinterpret_cast<const lxb_char_t*>(value.c_str()),
            value.length()
        );
    }

    // 同步子节点
    // 先清空 Lexbor 元素的子节点
    lxb_dom_node_t* child = lexbor_element_->first_child;
    while (child) {
        lxb_dom_node_t* next = child->next;
        lxb_dom_node_remove(child);
        lxb_dom_node_destroy(child);
        child = next;
    }

    // 添加新的子节点
    for (const auto& mbink_child : child_nodes_) {
        if (auto elem_child = std::dynamic_pointer_cast<Element>(mbink_child)) {
            // 递归同步子元素
            elem_child->SyncToLexbor();
            if (elem_child->GetLexborElement()) {
                lxb_dom_node_insert_child(lexbor_element_, elem_child->GetLexborElement());
            }
        } else if (auto text_child = std::dynamic_pointer_cast<Text>(mbink_child)) {
            // 创建文本节点
            auto text_content = text_child->GetTextContent();
            lxb_dom_text_t* lexbor_text = lxb_dom_document_create_text_node(
                lxb_dom_interface_document(native_doc),
                reinterpret_cast<const lxb_char_t*>(text_content.c_str()),
                text_content.length()
            );
            if (lexbor_text) {
                lxb_dom_node_insert_child(lexbor_element_, lxb_dom_interface_node(lexbor_text));
            }
        }
    }

    lexbor_dirty_ = false;
}

} // namespace lightui
