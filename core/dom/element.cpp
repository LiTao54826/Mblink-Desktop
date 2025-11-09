/**
 * @file element.cpp
 * @brief DOM元素类实现
 */

#include "element.h"
#include "text.h"
#include "event.h"
#include <algorithm>
#include <sstream>

namespace lightui {

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
    attributes_[name] = value;
    MarkDirty();
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

void Element::RemoveAttribute(const std::string& name) {
    attributes_.erase(name);
    MarkDirty();
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

void Element::SetStyle(const std::string& property, const std::string& value) {
    styles_[property] = value;
    MarkDirty();
}

std::string Element::GetStyle(const std::string& property) const {
    auto it = styles_.find(property);
    if (it != styles_.end()) {
        return it->second;
    }
    return "";
}

// ========== 克隆和文本内容 ==========

std::shared_ptr<Node> Element::CloneNode(bool deep) {
    auto cloned = std::make_shared<Element>(tag_name_);

    // 复制属性
    cloned->attributes_ = attributes_;
    cloned->styles_ = styles_;

    // 深度克隆子节点
    if (deep) {
        for (const auto& child : child_nodes_) {
            cloned->AppendChild(child->CloneNode(true));
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

// ========== 事件监听（暂时为空实现） ==========

void Element::AddEventListener(const std::string& type, EventListener listener) {
    event_listeners_[type].push_back(listener);
}

void Element::RemoveEventListener(const std::string& type, EventListener listener) {
    auto it = event_listeners_.find(type);
    if (it == event_listeners_.end()) {
        return;
    }

    // 注意：由于 std::function 没有 operator==，这里无法直接比较
    // 实际应用中可能需要使用 ID 或其他方式来标识监听器
    // 这里暂时不实现移除功能
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
    // 注意：简化实现，暂时跳过捕获阶段
    // 完整实现需要支持 addEventListener 的 useCapture 参数

    // 3. 目标阶段
    if (!event->IsPropagationStopped()) {
        event->SetEventPhase(EventPhase::AT_TARGET);
        event->SetCurrentTarget(shared_from_this());
        HandleEvent(event, false);
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
                element->HandleEvent(event, false);
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

    // 调用所有监听器
    for (const auto& listener : it->second) {
        if (event->IsImmediatePropagationStopped()) {
            break;
        }

        listener(event);
    }
}

// ========== 查询选择器 ==========

namespace {
    /**
     * @brief 简单的 CSS 选择器匹配
     * 支持：标签选择器（div）、ID选择器（#id）、类选择器（.class）、
     *       属性选择器（[name="value"]）、通配符（*）
     */
    bool MatchesSimpleSelector(const Element* element, const std::string& selector) {
        if (selector.empty()) {
            return false;
        }

        // 通配符
        if (selector == "*") {
            return true;
        }

        // ID 选择器 (#id)
        if (selector[0] == '#') {
            std::string id = selector.substr(1);
            return element->GetAttribute("id") == id;
        }

        // 类选择器 (.class)
        if (selector[0] == '.') {
            std::string class_name = selector.substr(1);
            return element->HasClass(class_name);
        }

        // 属性选择器 ([name="value"])
        if (selector[0] == '[') {
            size_t end = selector.find(']');
            if (end == std::string::npos) {
                return false;
            }

            std::string attr_expr = selector.substr(1, end - 1);
            size_t eq_pos = attr_expr.find('=');

            if (eq_pos == std::string::npos) {
                // [name] - 只检查属性存在
                return element->HasAttribute(attr_expr);
            } else {
                // [name="value"] - 检查属性值
                std::string attr_name = attr_expr.substr(0, eq_pos);
                std::string attr_value = attr_expr.substr(eq_pos + 1);

                // 移除引号
                if (!attr_value.empty() &&
                    (attr_value.front() == '"' || attr_value.front() == '\'')) {
                    attr_value = attr_value.substr(1, attr_value.length() - 2);
                }

                return element->GetAttribute(attr_name) == attr_value;
            }
        }

        // 标签选择器 (div, span, etc.)
        return element->GetTagName() == selector;
    }

    /**
     * @brief 递归查询第一个匹配的元素
     */
    std::shared_ptr<Element> QuerySelectorRecursive(
        std::shared_ptr<Node> node,
        const std::string& selector) {

        auto element = std::dynamic_pointer_cast<Element>(node);
        if (element && MatchesSimpleSelector(element.get(), selector)) {
            return element;
        }

        for (const auto& child : node->GetChildNodes()) {
            auto result = QuerySelectorRecursive(child, selector);
            if (result) {
                return result;
            }
        }

        return nullptr;
    }

    /**
     * @brief 递归查询所有匹配的元素
     */
    void QuerySelectorAllRecursive(
        std::shared_ptr<Node> node,
        const std::string& selector,
        std::vector<std::shared_ptr<Element>>& results) {

        auto element = std::dynamic_pointer_cast<Element>(node);
        if (element && MatchesSimpleSelector(element.get(), selector)) {
            results.push_back(element);
        }

        for (const auto& child : node->GetChildNodes()) {
            QuerySelectorAllRecursive(child, selector, results);
        }
    }
}

std::shared_ptr<Element> Element::QuerySelector(const std::string& selector) {
    // 从当前元素的子节点开始查询
    for (const auto& child : GetChildNodes()) {
        auto result = QuerySelectorRecursive(child, selector);
        if (result) {
            return result;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Element>> Element::QuerySelectorAll(const std::string& selector) {
    std::vector<std::shared_ptr<Element>> results;

    // 从当前元素的子节点开始查询
    for (const auto& child : GetChildNodes()) {
        QuerySelectorAllRecursive(child, selector, results);
    }

    return results;
}

bool Element::Matches(const std::string& selector) const {
    return MatchesSimpleSelector(this, selector);
}

std::shared_ptr<Element> Element::Closest(const std::string& selector) {
    // 从当前元素开始向上查找
    auto current = std::dynamic_pointer_cast<Element>(shared_from_this());

    while (current) {
        if (current->Matches(selector)) {
            return current;
        }

        auto parent = current->GetParentNode();
        current = std::dynamic_pointer_cast<Element>(parent);
    }

    return nullptr;
}

// ========== innerHTML ==========

std::string Element::GetInnerHTML() const {
    std::string result;

    for (const auto& child : GetChildNodes()) {
        auto element = std::dynamic_pointer_cast<Element>(child);
        if (element) {
            // 元素节点：<tagName>innerHTML</tagName>
            result += "<" + element->GetTagName();

            // 添加属性
            std::string id = element->GetAttribute("id");
            if (!id.empty()) {
                result += " id=\"" + id + "\"";
            }
            if (!element->GetClassName().empty()) {
                result += " class=\"" + element->GetClassName() + "\"";
            }

            result += ">";
            result += element->GetInnerHTML();  // 递归
            result += "</" + element->GetTagName() + ">";
        } else {
            // 文本节点
            result += child->GetTextContent();
        }
    }

    return result;
}

void Element::SetInnerHTML(const std::string& html) {
    // 简单实现：清空所有子节点，然后添加一个文本节点
    // 完整的 HTML 解析器实现会更复杂，这里只做简单处理

    // 清空所有子节点
    while (!GetChildNodes().empty()) {
        RemoveChild(GetChildNodes()[0]);
    }

    // 如果 HTML 为空，直接返回
    if (html.empty()) {
        return;
    }

    // 简单处理：如果不包含 <，当作纯文本
    if (html.find('<') == std::string::npos) {
        auto text = std::make_shared<Text>(html);
        AppendChild(text);
        return;
    }

    // TODO: 完整的 HTML 解析器实现
    // 目前只支持纯文本，复杂的 HTML 解析留待后续实现
    auto text = std::make_shared<Text>(html);
    AppendChild(text);
}

} // namespace lightui
