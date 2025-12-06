/**
 * @file css_style_declaration.cpp
 * @brief CSSStyleDeclaration类实现
 * 
 * 参考：
 * - W3C CSSOM - CSSStyleDeclaration
 * - MDN Web Docs - HTMLElement.style
 * - RmlUi/Source/Core/ElementStyle.cpp
 */

#include "dom/css_style_declaration.h"
#include "dom/element.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace lightui {

CSSStyleDeclaration::CSSStyleDeclaration(std::weak_ptr<Element> element)
    : element_(element)
    , properties_()
    , priorities_()
    , property_order_() {
}

void CSSStyleDeclaration::SetProperty(const std::string& property, const std::string& value, const std::string& priority) {
    std::string normalized_property = NormalizePropertyName(property);
    
    if (normalized_property.empty()) {
        return;
    }
    
    // 如果是新属性，添加到顺序列表
    if (properties_.find(normalized_property) == properties_.end()) {
        property_order_.push_back(normalized_property);
    }
    
    // 设置属性值
    properties_[normalized_property] = value;
    
    // 设置优先级
    if (priority == "important") {
        priorities_[normalized_property] = "important";
    } else {
        priorities_.erase(normalized_property);
    }
    
    // 更新元素的style属性
    UpdateStyleAttribute();
}

std::string CSSStyleDeclaration::GetPropertyValue(const std::string& property) const {
    std::string normalized_property = NormalizePropertyName(property);
    
    auto it = properties_.find(normalized_property);
    if (it != properties_.end()) {
        return it->second;
    }
    
    return "";
}

std::string CSSStyleDeclaration::RemoveProperty(const std::string& property) {
    std::string normalized_property = NormalizePropertyName(property);
    
    auto it = properties_.find(normalized_property);
    if (it == properties_.end()) {
        return "";
    }
    
    std::string old_value = it->second;
    
    // 移除属性
    properties_.erase(it);
    priorities_.erase(normalized_property);
    
    // 从顺序列表中移除
    property_order_.erase(
        std::remove(property_order_.begin(), property_order_.end(), normalized_property),
        property_order_.end()
    );
    
    // 更新元素的style属性
    UpdateStyleAttribute();
    
    return old_value;
}

std::string CSSStyleDeclaration::GetPropertyPriority(const std::string& property) const {
    std::string normalized_property = NormalizePropertyName(property);
    
    auto it = priorities_.find(normalized_property);
    if (it != priorities_.end()) {
        return it->second;
    }
    
    return "";
}

size_t CSSStyleDeclaration::Length() const {
    return properties_.size();
}

std::string CSSStyleDeclaration::Item(size_t index) const {
    if (index < property_order_.size()) {
        return property_order_[index];
    }
    return "";
}

std::string CSSStyleDeclaration::GetCssText() const {
    return SerializeCssText();
}

void CSSStyleDeclaration::SetCssText(const std::string& css_text) {
    // 清空现有属性
    properties_.clear();
    priorities_.clear();
    property_order_.clear();

    if (css_text.empty()) {
        UpdateStyleAttribute();
        return;
    }

    // 直接按顺序解析CSS文本，保持属性顺序
    // 这对于简写属性和长写属性的覆盖顺序很重要
    // 例如：padding: 10px; padding-top: 15px; 应该先设置 padding，再覆盖 padding-top
    std::istringstream iss(css_text);
    std::string declaration;

    while (std::getline(iss, declaration, ';')) {
        // 去除前后空格
        size_t start = declaration.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            continue;
        }

        size_t end = declaration.find_last_not_of(" \t\n\r");
        declaration = declaration.substr(start, end - start + 1);

        // 查找冒号
        size_t colon_pos = declaration.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }

        // 提取属性名和值
        std::string property = declaration.substr(0, colon_pos);
        std::string value = declaration.substr(colon_pos + 1);

        // 去除属性名和值的空格
        start = property.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            end = property.find_last_not_of(" \t\n\r");
            property = property.substr(start, end - start + 1);
        }

        start = value.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            end = value.find_last_not_of(" \t\n\r");
            value = value.substr(start, end - start + 1);
        }

        if (property.empty() || value.empty()) {
            continue;
        }

        // 规范化属性名
        std::string normalized_property = NormalizePropertyName(property);

        // 检查是否有!important
        std::string actual_value = value;
        std::string priority = "";

        size_t important_pos = value.find("!important");
        if (important_pos != std::string::npos) {
            actual_value = value.substr(0, important_pos);
            priority = "important";

            // 去除尾部空格
            while (!actual_value.empty() && std::isspace(actual_value.back())) {
                actual_value.pop_back();
            }
        }

        // 直接设置属性（不调用 SetProperty 以避免多次 UpdateStyleAttribute）
        // 如果是新属性，添加到顺序列表
        if (properties_.find(normalized_property) == properties_.end()) {
            property_order_.push_back(normalized_property);
        }

        // 设置属性值
        properties_[normalized_property] = actual_value;

        // 设置优先级
        if (priority == "important") {
            priorities_[normalized_property] = "important";
        } else {
            priorities_.erase(normalized_property);
        }
    }

    // 最后更新元素的style属性
    UpdateStyleAttribute();
}

const std::unordered_map<std::string, std::string>& CSSStyleDeclaration::GetAllProperties() const {
    return properties_;
}

std::unordered_map<std::string, std::string> CSSStyleDeclaration::ParseCssText(const std::string& css_text) const {
    std::unordered_map<std::string, std::string> result;
    
    if (css_text.empty()) {
        return result;
    }
    
    // 分割声明（以分号分隔）
    std::istringstream iss(css_text);
    std::string declaration;
    
    while (std::getline(iss, declaration, ';')) {
        // 去除前后空格
        size_t start = declaration.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            continue;
        }
        
        size_t end = declaration.find_last_not_of(" \t\n\r");
        declaration = declaration.substr(start, end - start + 1);
        
        // 查找冒号
        size_t colon_pos = declaration.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }
        
        // 提取属性名和值
        std::string property = declaration.substr(0, colon_pos);
        std::string value = declaration.substr(colon_pos + 1);
        
        // 去除属性名和值的空格
        start = property.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            end = property.find_last_not_of(" \t\n\r");
            property = property.substr(start, end - start + 1);
        }
        
        start = value.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            end = value.find_last_not_of(" \t\n\r");
            value = value.substr(start, end - start + 1);
        }
        
        if (!property.empty() && !value.empty()) {
            result[NormalizePropertyName(property)] = value;
        }
    }
    
    return result;
}

std::string CSSStyleDeclaration::SerializeCssText() const {
    if (properties_.empty()) {
        return "";
    }
    
    std::string result;
    
    // 按照插入顺序序列化
    for (const auto& property : property_order_) {
        auto it = properties_.find(property);
        if (it == properties_.end()) {
            continue;
        }
        
        if (!result.empty()) {
            result += " ";
        }
        
        result += property + ": " + it->second;
        
        // 添加优先级
        auto priority_it = priorities_.find(property);
        if (priority_it != priorities_.end() && priority_it->second == "important") {
            result += " !important";
        }
        
        result += ";";
    }
    
    return result;
}

std::string CSSStyleDeclaration::NormalizePropertyName(const std::string& property) const {
    std::string result;
    
    // 去除前后空格
    size_t start = property.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = property.find_last_not_of(" \t\n\r");
    std::string trimmed = property.substr(start, end - start + 1);
    
    // 转换为小写
    result.reserve(trimmed.size());
    for (char c : trimmed) {
        result += std::tolower(c);
    }
    
    return result;
}

void CSSStyleDeclaration::UpdateStyleAttribute() {
    auto elem = element_.lock();
    if (!elem) {
        return;
    }

    // 将样式序列化为CSS文本并设置到style属性
    std::string css_text = SerializeCssText();
    elem->SetAttribute("style", css_text);
}

} // namespace lightui

