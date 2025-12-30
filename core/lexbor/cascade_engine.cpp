/**
 * @file cascade_engine.cpp
 * @brief CSS级联和继承引擎实现
 */

#include "cascade_engine.h"
#include "core/dom/element.h"
#include "core/lexbor/lexbor_stylesheet.h"
#include <algorithm>
#include <sstream>
#include <set>

namespace lightui {

// 可继承属性列表
static const std::set<std::string> INHERITABLE_PROPERTIES = {
    "color",
    "font-family",
    "font-size",
    "font-style",
    "font-weight",
    "line-height",
    "text-align",
    "text-indent",
    "text-transform",
    "visibility",
    "white-space",
    "word-spacing",
    "letter-spacing",
    "direction",
    "quotes",
    "cursor"
};

// 属性初始值
static const std::map<std::string, std::string> INITIAL_VALUES = {
    {"color", "black"},
    {"background-color", "transparent"},
    {"font-size", "16px"},
    {"font-weight", "normal"},
    {"font-style", "normal"},
    {"text-align", "left"},
    {"display", "inline"},
    {"position", "static"},
    {"visibility", "visible"},
    {"width", "auto"},
    {"height", "auto"},
    {"margin", "0"},
    {"padding", "0"},
    {"border-width", "0"},
    {"opacity", "1"}
};

// ========== 构造函数和析构函数 ==========

CascadeEngine::CascadeEngine() {
}

CascadeEngine::~CascadeEngine() {
}

// ========== 优先级计算 ==========

Specificity CascadeEngine::CalculateSpecificity(const std::string& selector) const {
    return ParseSelector(selector);
}

Specificity CascadeEngine::ParseSelector(const std::string& selector) const {
    Specificity spec;
    
    // 简化的选择器解析
    // 统计ID选择器 (#)
    size_t pos = 0;
    while ((pos = selector.find('#', pos)) != std::string::npos) {
        spec.id_count++;
        pos++;
    }
    
    // 统计类选择器 (.)
    pos = 0;
    while ((pos = selector.find('.', pos)) != std::string::npos) {
        spec.class_count++;
        pos++;
    }
    
    // 统计属性选择器 ([])
    pos = 0;
    while ((pos = selector.find('[', pos)) != std::string::npos) {
        spec.class_count++;
        pos++;
    }
    
    // 统计伪类选择器 (:)
    pos = 0;
    while ((pos = selector.find(':', pos)) != std::string::npos) {
        // 排除伪元素 (::)
        if (pos + 1 < selector.length() && selector[pos + 1] != ':') {
            spec.class_count++;
        }
        pos++;
    }
    
    // 统计元素选择器
    // 简化：如果选择器中有字母且不是全部大写（排除通配符*），则至少有一个元素选择器
    bool has_element = false;
    for (char c : selector) {
        if (std::isalpha(c) && std::islower(c)) {
            has_element = true;
            break;
        }
    }
    
    if (has_element) {
        // 简化：假设每个空格分隔的部分都是一个元素选择器
        std::istringstream iss(selector);
        std::string token;
        while (iss >> token) {
            // 检查token是否以字母开头（元素选择器）
            if (!token.empty() && std::isalpha(token[0]) && std::islower(token[0])) {
                spec.element_count++;
            }
        }
    }
    
    return spec;
}

// ========== 级联规则 ==========

std::map<std::string, std::string> CascadeEngine::ApplyCascade(
    Element* element,
    const std::vector<const CSSRule*>& matching_rules
) const {
    if (!element) {
        return {};
    }
    
    // 收集所有属性值及其优先级
    std::map<std::string, std::vector<PropertyValue>> property_values;
    
    int order = 0;
    for (const auto* rule : matching_rules) {
        Specificity spec = CalculateSpecificity(rule->selector);
        
        for (const auto& [property, value] : rule->declarations) {
            property_values[property].emplace_back(value, spec, order++);
        }
    }
    
    // 处理内联样式（优先级最高）
    std::string inline_style = element->GetAttribute("style");
    if (!inline_style.empty()) {
        // 解析内联样式
        std::istringstream stream(inline_style);
        std::string declaration;
        
        while (std::getline(stream, declaration, ';')) {
            size_t colon_pos = declaration.find(':');
            if (colon_pos == std::string::npos) {
                continue;
            }
            
            std::string property = declaration.substr(0, colon_pos);
            std::string value = declaration.substr(colon_pos + 1);
            
            // 去除空格
            property.erase(0, property.find_first_not_of(" \t\n\r"));
            property.erase(property.find_last_not_of(" \t\n\r") + 1);
            value.erase(0, value.find_first_not_of(" \t\n\r"));
            value.erase(value.find_last_not_of(" \t\n\r") + 1);
            
            if (!property.empty() && !value.empty()) {
                Specificity inline_spec(1, 0, 0, 0); // 内联样式优先级最高
                property_values[property].emplace_back(value, inline_spec, order++);
            }
        }
    }
    
    // 合并每个属性的值
    std::map<std::string, std::string> result;
    for (const auto& [property, values] : property_values) {
        result[property] = MergePropertyValues(property, values);
    }
    
    return result;
}

std::string CascadeEngine::MergePropertyValues(
    const std::string& /*property*/,
    const std::vector<PropertyValue>& values
) const {
    if (values.empty()) {
        return "";
    }
    
    // 找到优先级最高的值
    const PropertyValue* best = &values[0];
    
    for (size_t i = 1; i < values.size(); ++i) {
        const PropertyValue& current = values[i];
        
        // 比较优先级
        int cmp = current.specificity.Compare(best->specificity);
        if (cmp > 0) {
            // 当前值优先级更高
            best = &current;
        } else if (cmp == 0 && current.order > best->order) {
            // 优先级相同，使用后声明的值
            best = &current;
        }
    }
    
    return best->value;
}

// ========== 继承规则 ==========

std::map<std::string, std::string> CascadeEngine::ApplyInheritance(
    Element* element,
    const std::map<std::string, std::string>& base_style
) const {
    if (!element) {
        return base_style;
    }
    
    std::map<std::string, std::string> result = base_style;
    
    // 对于可继承属性，如果当前元素没有设置，则从父元素继承
    for (const auto& property : INHERITABLE_PROPERTIES) {
        if (result.find(property) == result.end()) {
            std::string inherited_value = InheritFromParent(element, property);
            if (!inherited_value.empty()) {
                result[property] = inherited_value;
            }
        }
    }
    
    // 对于未设置的属性，使用初始值
    for (const auto& [property, initial_value] : INITIAL_VALUES) {
        if (result.find(property) == result.end()) {
            result[property] = initial_value;
        }
    }
    
    return result;
}

std::string CascadeEngine::InheritFromParent(Element* element, const std::string& property) const {
    if (!element) {
        return "";
    }

    auto parent_node = element->GetParentNode();
    if (!parent_node || parent_node->GetNodeType() != NodeType::ELEMENT_NODE) {
        return "";
    }

    Element* parent = static_cast<Element*>(parent_node.get());
    
    // 从父元素的计算样式中获取属性值
    // 注意：这里简化处理，实际应该从父元素的计算样式中获取
    // 为了避免循环依赖，这里只检查父元素的内联样式
    std::string parent_inline = parent->GetAttribute("style");
    if (parent_inline.empty()) {
        return "";
    }
    
    // 解析父元素的内联样式
    std::istringstream stream(parent_inline);
    std::string declaration;
    
    while (std::getline(stream, declaration, ';')) {
        size_t colon_pos = declaration.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }
        
        std::string prop = declaration.substr(0, colon_pos);
        std::string value = declaration.substr(colon_pos + 1);
        
        // 去除空格
        prop.erase(0, prop.find_first_not_of(" \t\n\r"));
        prop.erase(prop.find_last_not_of(" \t\n\r") + 1);
        value.erase(0, value.find_first_not_of(" \t\n\r"));
        value.erase(value.find_last_not_of(" \t\n\r") + 1);
        
        if (prop == property) {
            return value;
        }
    }
    
    return "";
}

// ========== 样式计算 ==========

std::map<std::string, std::string> CascadeEngine::ComputeStyle(
    Element* element,
    const std::vector<const CSSRule*>& matching_rules
) const {
    // 1. 应用级联规则
    auto cascaded_style = ApplyCascade(element, matching_rules);
    
    // 2. 应用继承规则
    auto final_style = ApplyInheritance(element, cascaded_style);
    
    return final_style;
}

// ========== 辅助方法 ==========

bool CascadeEngine::IsInheritableProperty(const std::string& property) const {
    return INHERITABLE_PROPERTIES.find(property) != INHERITABLE_PROPERTIES.end();
}

std::string CascadeEngine::GetInitialValue(const std::string& property) const {
    auto it = INITIAL_VALUES.find(property);
    if (it != INITIAL_VALUES.end()) {
        return it->second;
    }
    return "";
}

} // namespace lightui

