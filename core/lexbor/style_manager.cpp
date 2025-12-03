/**
 * @file style_manager.cpp
 * @brief 样式管理器实现
 */

#include "style_manager.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace lightui {

// ========== 构造函数和析构函数 ==========

StyleManager::StyleManager(Document* doc)
    : document_(doc)
    , stylesheets_()
    , inline_styles_() {
}

StyleManager::~StyleManager() {
    ClearStyleSheets();
}

// ========== 样式表管理 ==========

void StyleManager::AddStyleSheet(std::shared_ptr<LexborStyleSheet> sheet,
                                  int priority,
                                  const std::string& source) {
    if (!sheet) {
        return;
    }
    
    stylesheets_.emplace_back(sheet, priority, source);
    
    // 按优先级排序（优先级高的在后面，这样后面的会覆盖前面的）
    std::sort(stylesheets_.begin(), stylesheets_.end(),
              [](const StyleSheetEntry& a, const StyleSheetEntry& b) {
                  return a.priority < b.priority;
              });
}

bool StyleManager::RemoveStyleSheet(std::shared_ptr<LexborStyleSheet> sheet) {
    auto it = std::find_if(stylesheets_.begin(), stylesheets_.end(),
                           [&sheet](const StyleSheetEntry& entry) {
                               return entry.sheet == sheet;
                           });
    
    if (it != stylesheets_.end()) {
        stylesheets_.erase(it);
        return true;
    }
    
    return false;
}

void StyleManager::ClearStyleSheets() {
    stylesheets_.clear();
    inline_styles_.clear();
}

// ========== 样式解析 ==========

bool StyleManager::ParseStyleElement(Element* style_element) {
    if (!style_element || style_element->GetTagName() != "style") {
        return false;
    }
    
    // 获取<style>标签的文本内容
    std::string css_text = style_element->GetTextContent();
    if (css_text.empty()) {
        return false;
    }
    
    // 创建新的样式表并解析
    auto sheet = std::make_shared<LexborStyleSheet>();
    if (!sheet->ParseCSS(css_text)) {
        return false;
    }
    
    // 添加到样式表列表（<style>标签优先级为100）
    AddStyleSheet(sheet, 100, "style-element");
    
    return true;
}

std::map<std::string, std::string> StyleManager::ParseInlineStyle(const std::string& style) const {
    return ParseDeclarations(style);
}

bool StyleManager::LoadCSSFile(const std::string& file_path, int priority) {
    auto sheet = std::make_shared<LexborStyleSheet>();
    if (!sheet->ParseCSSFile(file_path)) {
        return false;
    }
    
    AddStyleSheet(sheet, priority, "external-file");
    return true;
}

// ========== 规则匹配 ==========

std::vector<const CSSRule*> StyleManager::GetMatchingRules(Element* element) const {
    if (!element) {
        return {};
    }
    
    std::vector<const CSSRule*> matching_rules;
    
    // 遍历所有样式表
    for (const auto& entry : stylesheets_) {
        const auto& rules = entry.sheet->GetRules();
        
        // 遍历样式表中的所有规则
        for (const auto& rule : rules) {
            if (MatchesSelector(rule->selector, element)) {
                matching_rules.push_back(rule.get());
            }
        }
    }
    
    // 按优先级排序
    std::sort(matching_rules.begin(), matching_rules.end(),
              [](const CSSRule* a, const CSSRule* b) {
                  return a->specificity < b->specificity;
              });
    
    return matching_rules;
}

// ========== 样式计算 ==========

std::map<std::string, std::string> StyleManager::ComputeStyle(Element* element) const {
    if (!element) {
        return {};
    }
    
    std::map<std::string, std::string> computed_style;
    
    // 1. 获取匹配的规则
    auto matching_rules = GetMatchingRules(element);
    
    // 2. 按优先级应用规则
    for (const auto* rule : matching_rules) {
        computed_style = MergeStyles(computed_style, rule->declarations);
    }
    
    // 3. 应用内联样式（优先级最高）
    std::string inline_style = element->GetAttribute("style");
    if (!inline_style.empty()) {
        auto inline_declarations = ParseInlineStyle(inline_style);
        computed_style = MergeStyles(computed_style, inline_declarations);
    }
    
    return computed_style;
}

// ========== 私有辅助方法 ==========

bool StyleManager::MatchesSelector(const std::string& selector, Element* element) const {
    if (!element || selector.empty()) {
        return false;
    }

    // 简化的选择器匹配实现
    // 支持：标签选择器、类选择器、ID选择器、后代选择器

    std::string trimmed_selector = selector;
    // 去除前后空格
    size_t start = trimmed_selector.find_first_not_of(" \t\n\r");
    size_t end = trimmed_selector.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        trimmed_selector = trimmed_selector.substr(start, end - start + 1);
    }

    // 检查是否是后代选择器（包含空格）
    size_t space_pos = trimmed_selector.find(' ');
    if (space_pos != std::string::npos) {
        // 后代选择器：从右向左匹配
        // 例如 "#test4 .inline-block-item" 分解为 ["#test4", ".inline-block-item"]
        std::vector<std::string> parts;
        std::istringstream iss(trimmed_selector);
        std::string part;
        while (iss >> part) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }

        if (parts.empty()) {
            return false;
        }

        // 最后一个选择器必须匹配当前元素
        if (!MatchesSimpleSelector(parts.back(), element)) {
            return false;
        }

        // 从右向左检查祖先元素
        if (parts.size() > 1) {
            auto parent_node = element->GetParentNode();
            Element* ancestor = dynamic_cast<Element*>(parent_node.get());
            int part_index = static_cast<int>(parts.size()) - 2;

            while (ancestor && part_index >= 0) {
                if (MatchesSimpleSelector(parts[part_index], ancestor)) {
                    part_index--;
                }
                parent_node = ancestor->GetParentNode();
                ancestor = dynamic_cast<Element*>(parent_node.get());
            }

            // 所有部分都必须匹配
            return part_index < 0;
        }

        return true;
    }

    // 简单选择器匹配
    return MatchesSimpleSelector(trimmed_selector, element);
}

bool StyleManager::MatchesSimpleSelector(const std::string& selector, Element* element) const {
    if (!element || selector.empty()) {
        return false;
    }

    // ID选择器 (#id)
    if (selector[0] == '#') {
        std::string id = selector.substr(1);
        return element->GetAttribute("id") == id;
    }

    // 类选择器 (.class)
    if (selector[0] == '.') {
        std::string class_name = selector.substr(1);
        return element->HasClass(class_name);
    }

    // 标签选择器 (tag)
    // 处理复合选择器（如 div.container）
    size_t dot_pos = selector.find('.');
    size_t hash_pos = selector.find('#');

    if (dot_pos != std::string::npos || hash_pos != std::string::npos) {
        // 复合选择器：先匹配标签
        size_t sep_pos = (dot_pos != std::string::npos) ? dot_pos : hash_pos;
        std::string tag = selector.substr(0, sep_pos);

        if (!tag.empty() && element->GetTagName() != tag) {
            return false;
        }

        // 再匹配类或ID
        if (dot_pos != std::string::npos) {
            std::string class_name = selector.substr(dot_pos + 1);
            // 移除可能的ID部分
            size_t hash_in_class = class_name.find('#');
            if (hash_in_class != std::string::npos) {
                class_name = class_name.substr(0, hash_in_class);
            }
            if (!element->HasClass(class_name)) {
                return false;
            }
        }

        if (hash_pos != std::string::npos) {
            std::string id = selector.substr(hash_pos + 1);
            if (element->GetAttribute("id") != id) {
                return false;
            }
        }

        return true;
    }

    // 简单标签选择器
    return element->GetTagName() == selector;
}

std::map<std::string, std::string> StyleManager::ParseDeclarations(const std::string& declarations) const {
    std::map<std::string, std::string> result;
    
    if (declarations.empty()) {
        return result;
    }
    
    // 分割声明（按分号分割）
    std::istringstream stream(declarations);
    std::string declaration;
    
    while (std::getline(stream, declaration, ';')) {
        // 去除前后空格
        size_t start = declaration.find_first_not_of(" \t\n\r");
        size_t end = declaration.find_last_not_of(" \t\n\r");
        
        if (start == std::string::npos || end == std::string::npos) {
            continue;
        }
        
        declaration = declaration.substr(start, end - start + 1);
        
        // 分割属性名和值（按冒号分割）
        size_t colon_pos = declaration.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }
        
        std::string property = declaration.substr(0, colon_pos);
        std::string value = declaration.substr(colon_pos + 1);
        
        // 去除属性名和值的空格
        start = property.find_first_not_of(" \t\n\r");
        end = property.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos) {
            property = property.substr(start, end - start + 1);
        }
        
        start = value.find_first_not_of(" \t\n\r");
        end = value.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos) {
            value = value.substr(start, end - start + 1);
        }
        
        if (!property.empty() && !value.empty()) {
            result[property] = value;
        }
    }
    
    return result;
}

std::map<std::string, std::string> StyleManager::MergeStyles(
    const std::map<std::string, std::string>& base,
    const std::map<std::string, std::string>& override) const {
    
    std::map<std::string, std::string> result = base;
    
    // 覆盖样式中的属性会替换基础样式中的同名属性
    for (const auto& [property, value] : override) {
        result[property] = value;
    }
    
    return result;
}

} // namespace lightui

