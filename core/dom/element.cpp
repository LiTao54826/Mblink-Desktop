/**
 * @file element.cpp
 * @brief DOM元素类实现
 */

#include "element.h"
#include "text.h"
#include "event.h"
#include "document.h"
#include "observers/dom_observer.h"
#include "selection/selector_engine.h"
#include "utils/dom_token_list.h"
#include "style/css_style_declaration.h"
#include "utils/dom_string_map.h"
#include "elements/html_input_element.h"
#include "elements/html_textarea_element.h"
#include "elements/html_button_element.h"
#include "elements/html_form_element.h"
#include "elements/html_select_element.h"
#include "elements/html_option_element.h"
#include <iostream>
#include "elements/html_anchor_element.h"
#include "elements/html_label_element.h"
#include "elements/html_image_element.h"
#include "elements/html_div_element.h"
#include "elements/html_span_element.h"
#include "elements/html_paragraph_element.h"
#include "elements/html_heading_element.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <lexbor/html/interfaces/document.h>
#include <lexbor/html/serialize.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/text.h>
#include "core/lexbor/lexbor_document.h"
#include "core/render/objects/render_object.h"
#include "core/window/window.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/event/input/focus_manager.h"
#include "../quickjs/quickjs.h"
#include "../quickjs/quickjs-libc.h"
#include "../quickjs/quickjs_runtime.h"
#include <SDL3/SDL.h>

// Windows API 宏冲突修复：取消 GetClassName 宏定义
#ifdef GetClassName
#undef GetClassName
#endif

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
    // 检查属性是否已存在
    bool has_attr = HasAttribute(name);
    std::string old_value = has_attr ? GetAttribute(name) : "";

    // 如果属性已存在且值没有改变，直接返回
    // 注意：如果属性不存在，即使值是空字符串也需要设置（用于 boolean 属性如 open, disabled 等）
    if (has_attr && old_value == value) {
        return;
    }

    attributes_[name] = value;

    // TODO: 特殊处理 style 属性：解析并应用内联样式
    // if (name == "style") {
    //     ParseStyleAttribute(value);
    // }

    // 特殊处理内联事件处理器（onclick, onload, onmouseover等）
    if (name.length() > 2 && name[0] == 'o' && name[1] == 'n') {
        std::string event_type = name.substr(2);  // 去掉 "on" 前缀
        SetupInlineEventHandler(event_type, value);
    }

    // 智能脏标记：根据属性类型精确标记
    if (name == "style") {
        // style 属性特殊处理：只标记 STYLE | PAINT
        // 布局相关的变化会在 Element::SetStyle 中单独处理
        // 这避免了 hover 等伪类变化时不必要的布局重算
        MarkDirty(DirtyType::STYLE | DirtyType::PAINT);
    } else if (IsLayoutAttribute(name)) {
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
    
    // 移动元素双区域标记优化：位置属性变化时记录旧位置
    // 这些属性会导致元素位置变化，需要同时重绘新旧两个位置
    static const std::unordered_set<std::string> position_properties = {
        "left", "top", "right", "bottom", "transform"
    };
    
    SkRect old_bounds = SkRect::MakeEmpty();
    bool is_position_change = position_properties.count(property) > 0;
    
    if (is_position_change) {
        // 记录变化前的边界框
        if (auto render_obj = GetRenderObject()) {
            old_bounds = render_obj->GetBoundingRect();
        }
    }
    
    styles_[property] = value;
    
    // 关键修复：同步更新 style attribute
    // StyleResolver 从 style attribute 读取内联样式，而不是从 styles_ map
    UpdateStyleAttribute();
    
    // 性能优化：根据CSS属性类型决定脏标记类型
    // 只有影响布局的属性才需要重新布局，其他属性只需要重绘
    
    // 关键优化：定位元素的 left/top/right/bottom 通常不触发布局！
    // - absolute/fixed: 脱离文档流，不影响其他元素
    // - relative: 只是视觉偏移，不改变占据的空间，不影响其他元素
    // - static: left/top 无效
    static const std::unordered_set<std::string> position_offset_properties = {
        "left", "top", "right", "bottom"
    };
    
    bool is_position_offset = position_offset_properties.count(property) > 0;
    bool skip_layout = false;
    
    if (is_position_offset) {
        // 检查元素的 position 属性
        std::string position = GetStyle("position");
        // 关键发现：absolute/fixed/relative 的位置偏移都不影响布局！
        // - absolute/fixed: 脱离文档流
        // - relative: 只是视觉偏移，仍占据原始空间
        // 只有 static（默认）时 left/top 无效，但也不需要布局
        skip_layout = (position == "absolute" || position == "fixed" || 
                      position == "relative" || position == "static" || position.empty());
    }
    
    // 总是触发布局的属性（无论定位方式）
    static const std::unordered_set<std::string> always_layout_properties = {
        // 尺寸属性
        "width", "height", "min-width", "min-height", "max-width", "max-height",
        // 内边距
        "padding", "padding-left", "padding-right", "padding-top", "padding-bottom",
        // 外边距
        "margin", "margin-left", "margin-right", "margin-top", "margin-bottom",
        // 边框
        "border", "border-width", "border-left-width", "border-right-width", 
        "border-top-width", "border-bottom-width",
        // 定位方式改变
        "position",
        // 显示
        "display", "visibility", "overflow", "overflow-x", "overflow-y",
        // Flexbox
        "flex", "flex-direction", "flex-wrap", "justify-content", "align-items",
        "flex-grow", "flex-shrink", "flex-basis",
        // 字体（影响文本尺寸）
        "font-size", "font-family", "font-weight", "line-height", "letter-spacing",
        // 其他
        "float", "clear", "vertical-align"
    };
    
    if (is_position_offset && skip_layout) {
        // 位置偏移属性（left/top/right/bottom）：只需要重绘！
        // 适用于 absolute/fixed/relative，都不影响其他元素的布局
        MarkDirty(DirtyType::PAINT);
    } else if (always_layout_properties.count(property) > 0) {
        // 总是触发布局的属性
        MarkDirty(DirtyType::LAYOUT | DirtyType::PAINT);
    } else {
        // 只影响外观的属性（如颜色、背景等）：只需要重绘
        MarkDirty(DirtyType::PAINT);
    }

    // 移动元素双区域标记：添加旧位置到脏区域列表
    if (is_position_change && !old_bounds.isEmpty()) {
        auto doc = GetOwnerDocument();
        if (doc) {
            // 将旧位置添加到文档的脏区域列表
            // 这样渲染时会同时重绘旧位置（擦除残影）和新位置
            doc->AddDirtyRect(old_bounds);
        }
    }

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

void Element::UpdateStyleAttribute() {
    // 将 styles_ map 转换为 CSS 字符串并更新 style attribute
    // 格式: "property1: value1; property2: value2;"
    if (styles_.empty()) {
        // 如果没有样式，移除 style attribute
        if (HasAttribute("style")) {
            RemoveAttribute("style");
        }
        return;
    }
    
    std::string css_text;
    for (const auto& [property, value] : styles_) {
        if (!css_text.empty()) {
            css_text += " ";
        }
        css_text += property + ": " + value + ";";
    }
    
    // 直接设置 attribute，避免递归调用 SetStyle
    attributes_["style"] = css_text;
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
    // 只对 input 事件打印详细日志
    bool verbose = (event->GetType() == "input");
    if (verbose) {
        std::cout << "[Element::HandleEvent] Event: " << event->GetType()
                  << " on <" << tag_name_ << ">, capture=" << use_capture << std::endl;
    }

    auto it = event_listeners_.find(event->GetType());
    if (it == event_listeners_.end()) {
        return;
    }

    if (verbose) {
        std::cout << "[Element::HandleEvent] Found " << it->second.size() << " listeners" << std::endl;
    }

    // 收集需要移除的once监听器ID
    std::vector<uint64_t> once_listeners_to_remove;

    // 调用匹配捕获阶段的监听器
    // 参考：RmlUi的事件分发机制
    int listener_index = 0;
    for (const auto& entry : it->second) {
        // 只调用匹配当前阶段的监听器
        if (entry.use_capture != use_capture) {
            continue;
        }

        if (event->IsImmediatePropagationStopped()) {
            break;
        }

        if (verbose) {
            std::cout << "[Element::HandleEvent] Calling listener " << listener_index
                      << " (id=" << entry.id << ")" << std::endl;
        }

        // 调用监听器
        entry.listener(event);

        if (verbose) {
            std::cout << "[Element::HandleEvent] Listener " << listener_index << " returned" << std::endl;
        }

        // 如果是once监听器，标记为待移除
        if (entry.once) {
            once_listeners_to_remove.push_back(entry.id);
        }
        listener_index++;
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

    // 特殊处理：hover 伪类需要通知观察者以触发样式重新解析
    // 这是因为 :hover 选择器可能定义了动画，需要重新解析样式来获取
    // 其他伪类（如 :focus, :active）不需要，因为它们通常只改变颜色等简单属性
    if (pseudo_class == "hover") {
        auto doc = GetOwnerDocument();
        if (doc) {
            doc->GetObserverManager().NotifyPseudoClassChanged(
                std::static_pointer_cast<Element>(shared_from_this()),
                pseudo_class,
                activate
            );
        }
    }
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

void Element::SetupInlineEventHandler(const std::string& event_type, const std::string& handler_code) {
    std::cout << "[Element::SetupInlineEventHandler] Setting up handler for event '" << event_type 
              << "' with code: " << handler_code << std::endl;

    // 移除旧的内联事件处理器（如果存在）
    auto it = inline_event_handlers_.find(event_type);
    if (it != inline_event_handlers_.end()) {
        RemoveEventListener(event_type, it->second);
        inline_event_handlers_.erase(it);
    }

    // 如果handler_code为空，只移除不添加
    if (handler_code.empty()) {
        return;
    }

    // 创建事件监听器，在事件触发时动态获取 JS 上下文并执行代码
    // 这样可以确保在脚本执行后，全局函数已经定义
    std::string code = handler_code;  // 复制一份，避免引用悬空
    
    uint64_t listener_id = AddEventListener(event_type, [this, code, event_type](std::shared_ptr<Event> event) {
        std::cout << "[InlineEventHandler] Executing inline handler for '" << event_type << "': " << code << std::endl;
        
        // 动态获取 Document 和 JavaScript 上下文
        auto doc = std::dynamic_pointer_cast<Document>(GetOwnerDocument());
        if (!doc) {
            std::cerr << "[InlineEventHandler] No document, cannot execute inline handler" << std::endl;
            return;
        }

        auto js_runtime = doc->GetJSRuntime();
        if (!js_runtime) {
            std::cerr << "[InlineEventHandler] No JS runtime, cannot execute inline handler" << std::endl;
            return;
        }

        auto js_ctx = js_runtime->GetContext();
        if (!js_ctx) {
            std::cerr << "[InlineEventHandler] No JS context, cannot execute inline handler" << std::endl;
            return;
        }
        
        // 在全局作用域执行代码
        JSValue result = JS_Eval(js_ctx, code.c_str(), code.length(), "<inline>", JS_EVAL_TYPE_GLOBAL);
        
        if (JS_IsException(result)) {
            std::cerr << "[InlineEventHandler] Exception in inline handler:" << std::endl;
            js_std_dump_error(js_ctx);
        }
        
        JS_FreeValue(js_ctx, result);
        std::cout << "[InlineEventHandler] Inline handler completed" << std::endl;
    }, false, false);

    // 记录listener ID，以便后续移除
    inline_event_handlers_[event_type] = listener_id;
    
    std::cout << "[Element::SetupInlineEventHandler] Handler setup complete, listener_id=" << listener_id << std::endl;
}

// ========== ContentEditable 支持 ==========

bool Element::IsContentEditable() const {
    // 获取 contenteditable 属性
    std::string value = GetContentEditable();

    if (value == "true") {
        return true;
    }

    if (value == "false") {
        return false;
    }

    // 继承父元素的可编辑状态
    auto parent = GetParentNode();
    if (parent && parent->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto parent_element = std::dynamic_pointer_cast<Element>(parent);
        if (parent_element) {
            return parent_element->IsContentEditable();
        }
    }

    // 默认不可编辑
    return false;
}

std::string Element::GetContentEditable() const {
    auto it = attributes_.find("contenteditable");
    if (it != attributes_.end()) {
        const std::string& value = it->second;
        // 标准化值
        if (value == "true" || value == "") {
            return "true";
        }
        if (value == "false") {
            return "false";
        }
        if (value == "inherit") {
            return "inherit";
        }
        // 其他值视为 "inherit"
        return "inherit";
    }
    return "inherit";
}

void Element::SetContentEditable(const std::string& value) {
    if (value == "true" || value == "false" || value == "inherit") {
        SetAttribute("contenteditable", value);
    } else if (value.empty()) {
        RemoveAttribute("contenteditable");
    } else {
        // 无效值，设置为 "inherit"
        SetAttribute("contenteditable", "inherit");
    }
}

// ========== 几何信息 ==========

Element::DOMRect Element::GetBoundingClientRect() const {
    DOMRect rect;

    // 强制同步布局（模拟浏览器的 forced reflow）
    auto doc = GetOwnerDocument();
    if (doc) {
        doc->ForceLayout();
    }

    // 尝试从关联的 RenderObject 获取布局信息
    auto render_object = GetRenderObject();
    if (render_object) {
        const auto& layout = render_object->GetLayoutInfo();
        
        // 获取绝对位置（相对于视口）
        float abs_x = layout.x;
        float abs_y = layout.y;
        
        // 累加所有祖先的位置，同时考虑滚动偏移
        auto parent = render_object->GetParent();
        while (parent) {
            const auto& parent_layout = parent->GetLayoutInfo();
            abs_x += parent_layout.x;
            abs_y += parent_layout.y;
            
            // 减去父元素的滚动偏移（视口坐标需要考虑滚动）
            abs_x -= parent->GetScrollX();
            abs_y -= parent->GetScrollY();
            
            parent = parent->GetParent();
        }

        rect.x = abs_x;
        rect.y = abs_y;
        rect.width = layout.width;
        rect.height = layout.height;
        rect.top = abs_y;
        rect.left = abs_x;
        rect.right = abs_x + layout.width;
        rect.bottom = abs_y + layout.height;
    }

    return rect;
}

void Element::ScrollIntoView(bool align_to_top) {
    // 获取关联的 RenderObject
    auto render_object = GetRenderObject();
    if (!render_object) {
        return;
    }

    // 查找最近的可滚动祖先
    std::shared_ptr<RenderObject> scrollable_ancestor = render_object->GetParent();
    while (scrollable_ancestor) {
        if (scrollable_ancestor->IsScrollable()) {
            break;
        }
        scrollable_ancestor = scrollable_ancestor->GetParent();
    }

    if (!scrollable_ancestor) {
        return;
    }

    // 计算元素相对于可滚动祖先的位置
    const auto& layout = render_object->GetLayoutInfo();
    float element_top = 0;
    float element_bottom = 0;
    auto current = render_object->GetParent();

    while (current && current.get() != scrollable_ancestor.get()) {
        const auto& current_layout = current->GetLayoutInfo();
        element_top += current_layout.y;
        current = current->GetParent();
    }
    element_top += layout.y;
    element_bottom = element_top + layout.height;

    // 获取当前滚动位置和可视区域
    float scroll_top = scrollable_ancestor->GetScrollY();
    const auto& ancestor_layout = scrollable_ancestor->GetLayoutInfo();
    float viewport_height = ancestor_layout.height;

    // 计算新的滚动位置
    float new_scroll_top = scroll_top;

    if (align_to_top) {
        // 顶部对齐
        new_scroll_top = element_top;
    } else {
        // 底部对齐
        new_scroll_top = element_bottom - viewport_height;
    }

    // 设置滚动位置
    scrollable_ancestor->SetScrollY(new_scroll_top);
}

void Element::Focus() {
    std::cout << "[Element::Focus] Called on <" << tag_name_ << ">" << std::endl;
    
    // 获取所属文档
    auto doc = GetOwnerDocument();
    if (!doc) {
        std::cout << "[Element::Focus] ERROR: No owner document!" << std::endl;
        return;
    }
    
    // 获取 Window 和 FocusManager
    Window* window = doc->GetWindow();
    if (!window) {
        std::cout << "[Element::Focus] WARNING: No window!" << std::endl;
        // 回退到简单的焦点处理
        SetPseudoClass("focus", true);
        auto self = std::static_pointer_cast<Element>(shared_from_this());
        doc->SetActiveElement(self);
        auto event = std::make_shared<Event>("focus", false, false);
        DispatchEvent(event);
        return;
    }
    
    // 关键修复：使用 FocusManager 来设置焦点
    // 这样光标闪烁和键盘输入才能正常工作
    FocusManager* focus_manager = window->GetFocusManager();
    if (focus_manager) {
        std::cout << "[Element::Focus] Using FocusManager::SetFocus()" << std::endl;
        focus_manager->SetWindow(window);
        auto self = std::static_pointer_cast<Element>(shared_from_this());
        focus_manager->SetFocus(self, false);
    } else {
        std::cout << "[Element::Focus] WARNING: No FocusManager, using fallback" << std::endl;
        // 回退到原来的实现
        auto old_active = doc->GetActiveElement();
        if (old_active && old_active.get() != this) {
            old_active->SetPseudoClass("focus", false);
            old_active->SetPseudoClass("focus-visible", false);
            if (auto render_obj = old_active->GetRenderObject()) {
                render_obj->MarkNeedsPaint();
                render_obj->InvalidatePaintCache();
            }
            auto blur_event = std::make_shared<Event>("blur", false, false);
            old_active->DispatchEvent(blur_event);
        }
        
        SetPseudoClass("focus", true);
        auto self = std::static_pointer_cast<Element>(shared_from_this());
        doc->SetActiveElement(self);
        
        if (auto render_obj = GetRenderObject()) {
            render_obj->MarkNeedsPaint();
            render_obj->InvalidatePaintCache();
        }
        
        if (tag_name_ == "input" || tag_name_ == "textarea" || tag_name_ == "terminal" || IsContentEditable()) {
            if (window->GetSDLWindow()) {
                SDL_StartTextInput(window->GetSDLWindow());
            }
        }
        
        window->SetNeedsRepaint();
        if (auto pipeline = window->GetRenderPipeline()) {
            pipeline->MarkNeedsPaint();
        }
        
        auto event = std::make_shared<Event>("focus", false, false);
        DispatchEvent(event);
    }
}

void Element::Blur() {
    // 获取所属文档
    auto doc = GetOwnerDocument();
    
    // 获取 Window 和 FocusManager
    Window* window = doc ? doc->GetWindow() : nullptr;
    FocusManager* focus_manager = window ? window->GetFocusManager() : nullptr;
    
    if (focus_manager) {
        // 使用 FocusManager 来处理 blur
        auto self = std::static_pointer_cast<Element>(shared_from_this());
        focus_manager->Blur(self);
    } else {
        // 回退到原来的实现
        SetPseudoClass("focus", false);
        SetPseudoClass("focus-visible", false);
        
        if (doc) {
            auto active = doc->GetActiveElement();
            if (active.get() == this) {
                doc->SetActiveElement(nullptr);
            }
        }
        
        if (auto render_obj = GetRenderObject()) {
            render_obj->MarkNeedsPaint();
            render_obj->InvalidatePaintCache();
        }
        
        if (tag_name_ == "input" || tag_name_ == "textarea" || tag_name_ == "terminal" || IsContentEditable()) {
            if (window && window->GetSDLWindow()) {
                SDL_StopTextInput(window->GetSDLWindow());
            }
        }
        
        if (window) {
            window->SetNeedsRepaint();
            if (auto pipeline = window->GetRenderPipeline()) {
                pipeline->MarkNeedsPaint();
            }
        }
        
        auto event = std::make_shared<Event>("blur", false, false);
        DispatchEvent(event);
    }
}

} // namespace lightui
