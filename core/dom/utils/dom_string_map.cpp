/**
 * @file dom_string_map.cpp
 * @brief DOMStringMap类实现
 * 
 * 参考：
 * - W3C HTML5 - DOMStringMap
 * - MDN Web Docs - HTMLElement.dataset
 */

#include "dom_string_map.h"
#include "core/dom/element.h"
#include <cctype>
#include <algorithm>

namespace lightui {

DOMStringMap::DOMStringMap(std::weak_ptr<Element> element)
    : element_(element) {
}

std::string DOMStringMap::Get(const std::string& name) const {
    auto elem = element_.lock();
    if (!elem) {
        return "";
    }
    
    std::string html_attr = JsNameToHtmlAttr(name);
    return elem->GetAttribute(html_attr);
}

void DOMStringMap::Set(const std::string& name, const std::string& value) {
    auto elem = element_.lock();
    if (!elem) {
        return;
    }
    
    std::string html_attr = JsNameToHtmlAttr(name);
    elem->SetAttribute(html_attr, value);
}

void DOMStringMap::Remove(const std::string& name) {
    auto elem = element_.lock();
    if (!elem) {
        return;
    }
    
    std::string html_attr = JsNameToHtmlAttr(name);
    elem->RemoveAttribute(html_attr);
}

bool DOMStringMap::Has(const std::string& name) const {
    auto elem = element_.lock();
    if (!elem) {
        return false;
    }
    
    std::string html_attr = JsNameToHtmlAttr(name);
    return elem->HasAttribute(html_attr);
}

std::unordered_map<std::string, std::string> DOMStringMap::GetAll() const {
    std::unordered_map<std::string, std::string> result;
    
    auto elem = element_.lock();
    if (!elem) {
        return result;
    }
    
    // 获取所有属性
    auto all_attrs = elem->GetAllAttributes();
    
    // 过滤data-*属性并转换名称
    for (const auto& [attr_name, attr_value] : all_attrs) {
        if (IsDataAttribute(attr_name)) {
            std::string js_name = HtmlAttrToJsName(attr_name);
            result[js_name] = attr_value;
        }
    }
    
    return result;
}

std::string DOMStringMap::JsNameToHtmlAttr(const std::string& js_name) const {
    // 转换规则：
    // "userId" -> "data-user-id"
    // "fooBarBaz" -> "data-foo-bar-baz"
    
    std::string result = "data-";
    
    for (char c : js_name) {
        if (std::isupper(c)) {
            // 大写字母转换为"-小写字母"
            result += '-';
            result += std::tolower(c);
        } else {
            result += c;
        }
    }
    
    return result;
}

std::string DOMStringMap::HtmlAttrToJsName(const std::string& html_attr) const {
    // 转换规则：
    // "data-user-id" -> "userId"
    // "data-foo-bar-baz" -> "fooBarBaz"
    
    if (!IsDataAttribute(html_attr)) {
        return "";
    }
    
    // 移除"data-"前缀
    std::string result;
    bool next_upper = false;
    
    for (size_t i = 5; i < html_attr.length(); ++i) {  // 从"data-"后开始
        char c = html_attr[i];
        
        if (c == '-') {
            // 下一个字母应该大写
            next_upper = true;
        } else {
            if (next_upper) {
                result += std::toupper(c);
                next_upper = false;
            } else {
                result += c;
            }
        }
    }
    
    return result;
}

bool DOMStringMap::IsDataAttribute(const std::string& attr_name) const {
    // 检查是否以"data-"开头
    if (attr_name.length() < 5) {
        return false;
    }
    
    return attr_name.substr(0, 5) == "data-";
}

} // namespace lightui

