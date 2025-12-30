/**
 * @file style_manager.cpp
 * @brief 样式管理器实现
 */

#include "style_manager.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include "core/render/animation/keyframes.h"
#include <algorithm>
#include <sstream>
#include <vector>
#include <regex>
#include <fstream>
#include <iostream>

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
    
    // 首先提取并注册 @keyframes 规则
    ExtractAndRegisterKeyframes(css_text);
    
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
    // 读取文件内容
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::string css_text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    
    if (css_text.empty()) {
        return false;
    }
    
    // 使用 ParseCSSString 来处理（包含 @keyframes 提取）
    return ParseCSSString(css_text, priority, "external-file");
}

bool StyleManager::ParseCSSString(const std::string& css_text, int priority, const std::string& source) {
    if (css_text.empty()) {
        return false;
    }

    // 首先提取并注册 @keyframes 规则
    ExtractAndRegisterKeyframes(css_text);

    auto sheet = std::make_shared<LexborStyleSheet>();
    if (!sheet->ParseCSS(css_text)) {
        return false;
    }

    AddStyleSheet(sheet, priority, source);
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
    // 支持：标签选择器、类选择器、ID选择器、后代选择器、逗号分隔选择器

    std::string trimmed_selector = selector;
    // 去除前后空格
    size_t start = trimmed_selector.find_first_not_of(" \t\n\r");
    size_t end = trimmed_selector.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        trimmed_selector = trimmed_selector.substr(start, end - start + 1);
    }

    // 检查是否是逗号分隔的选择器组（如 "html, body"）
    size_t comma_pos = trimmed_selector.find(',');
    if (comma_pos != std::string::npos) {
        // 分割逗号分隔的选择器，任一匹配即返回 true
        std::istringstream iss(trimmed_selector);
        std::string single_selector;
        while (std::getline(iss, single_selector, ',')) {
            // 去除前后空格
            size_t s_start = single_selector.find_first_not_of(" \t\n\r");
            size_t s_end = single_selector.find_last_not_of(" \t\n\r");
            if (s_start != std::string::npos && s_end != std::string::npos) {
                single_selector = single_selector.substr(s_start, s_end - s_start + 1);
                // 递归调用匹配单个选择器
                if (MatchesSelector(single_selector, element)) {
                    return true;
                }
            }
        }
        return false;
    }

    // 解析复合选择器（支持后代选择器和子选择器）
    // 将选择器分解为 parts 和 combinators
    // 例如 ".parent > .child .grandchild" 分解为:
    //   parts: [".parent", ".child", ".grandchild"]
    //   combinators: ['>', ' ']  (> 表示子选择器，空格表示后代选择器)
    
    std::vector<std::string> parts;
    std::vector<char> combinators;  // '>' 或 ' '
    
    std::string current_part;
    bool last_was_space = false;
    bool last_was_combinator = false;
    
    for (size_t i = 0; i < trimmed_selector.size(); ++i) {
        char c = trimmed_selector[i];
        
        if (c == '>') {
            // 子选择器
            if (!current_part.empty()) {
                // 去除 current_part 的前后空格
                size_t ps = current_part.find_first_not_of(" \t");
                size_t pe = current_part.find_last_not_of(" \t");
                if (ps != std::string::npos && pe != std::string::npos) {
                    parts.push_back(current_part.substr(ps, pe - ps + 1));
                }
                current_part.clear();
            }
            combinators.push_back('>');
            last_was_combinator = true;
            last_was_space = false;
        } else if (c == ' ' || c == '\t') {
            if (!current_part.empty() && !last_was_combinator) {
                // 可能是后代选择器，但需要等待看下一个非空字符
                last_was_space = true;
            }
        } else {
            // 普通字符
            if (last_was_space && !current_part.empty()) {
                // 这是一个后代选择器（空格分隔）
                size_t ps = current_part.find_first_not_of(" \t");
                size_t pe = current_part.find_last_not_of(" \t");
                if (ps != std::string::npos && pe != std::string::npos) {
                    parts.push_back(current_part.substr(ps, pe - ps + 1));
                }
                current_part.clear();
                combinators.push_back(' ');
            }
            current_part += c;
            last_was_space = false;
            last_was_combinator = false;
        }
    }
    
    // 添加最后一个部分
    if (!current_part.empty()) {
        size_t ps = current_part.find_first_not_of(" \t");
        size_t pe = current_part.find_last_not_of(" \t");
        if (ps != std::string::npos && pe != std::string::npos) {
            parts.push_back(current_part.substr(ps, pe - ps + 1));
        }
    }
    
    if (parts.empty()) {
        return false;
    }
    
    // 如果只有一个部分，直接匹配
    if (parts.size() == 1) {
        return MatchesSimpleSelector(parts[0], element);
    }
    
    // 最后一个选择器必须匹配当前元素
    if (!MatchesSimpleSelector(parts.back(), element)) {
        return false;
    }
    
    // 从右向左检查祖先元素
    auto parent_node = element->GetParentNode();
    Element* ancestor = dynamic_cast<Element*>(parent_node.get());
    int part_index = static_cast<int>(parts.size()) - 2;
    int comb_index = static_cast<int>(combinators.size()) - 1;
    
    while (ancestor && part_index >= 0 && comb_index >= 0) {
        char combinator = combinators[comb_index];
        
        if (combinator == '>') {
            // 子选择器：必须是直接父元素
            if (MatchesSimpleSelector(parts[part_index], ancestor)) {
                part_index--;
                comb_index--;
                parent_node = ancestor->GetParentNode();
                ancestor = dynamic_cast<Element*>(parent_node.get());
            } else {
                // 直接父元素不匹配，整个选择器不匹配
                return false;
            }
        } else {
            // 后代选择器：可以是任意祖先
            if (MatchesSimpleSelector(parts[part_index], ancestor)) {
                part_index--;
                comb_index--;
            }
            parent_node = ancestor->GetParentNode();
            ancestor = dynamic_cast<Element*>(parent_node.get());
        }
    }
    
    // 所有部分都必须匹配
    return part_index < 0;

    // 简单选择器匹配
    return MatchesSimpleSelector(trimmed_selector, element);
}

bool StyleManager::MatchesSimpleSelector(const std::string& selector, Element* element) const {
    if (!element || selector.empty()) {
        return false;
    }

    // 通配符选择器 (*)
    if (selector == "*") {
        return true;
    }

    // 检查是否包含伪类选择器（如 .class:hover, div:active）
    size_t pseudo_pos = selector.find(':');
    std::string base_selector = selector;
    std::string pseudo_class;
    
    if (pseudo_pos != std::string::npos) {
        base_selector = selector.substr(0, pseudo_pos);
        pseudo_class = selector.substr(pseudo_pos + 1);
        
        // 移除伪类中可能的额外部分（如 :hover::after）
        size_t double_colon = pseudo_class.find(':');
        if (double_colon != std::string::npos) {
            pseudo_class = pseudo_class.substr(0, double_colon);
        }
        
        // 检查元素是否有该伪类状态
        if (!pseudo_class.empty() && !element->HasPseudoClass(pseudo_class)) {
            return false;
        }
    }
    
    // 如果只有伪类（如 :hover），匹配所有有该伪类的元素
    if (base_selector.empty()) {
        return true;  // 伪类已经在上面检查过了
    }

    // ID选择器 (#id)
    if (base_selector[0] == '#') {
        std::string id = base_selector.substr(1);
        return element->GetAttribute("id") == id;
    }

    // 类选择器 (.class)
    if (base_selector[0] == '.') {
        std::string class_name = base_selector.substr(1);
        // 处理多个类选择器（如 .class1.class2）
        size_t next_dot = class_name.find('.');
        if (next_dot != std::string::npos) {
            // 多个类选择器
            std::string first_class = class_name.substr(0, next_dot);
            std::string rest = class_name.substr(next_dot);
            if (!element->HasClass(first_class)) {
                return false;
            }
            // 递归检查剩余的类
            return MatchesSimpleSelector(rest, element);
        }
        return element->HasClass(class_name);
    }

    // 标签选择器 (tag)
    // 处理复合选择器（如 div.container）
    size_t dot_pos = base_selector.find('.');
    size_t hash_pos = base_selector.find('#');

    if (dot_pos != std::string::npos || hash_pos != std::string::npos) {
        // 复合选择器：先匹配标签
        size_t sep_pos = (dot_pos != std::string::npos) ? dot_pos : hash_pos;
        std::string tag = base_selector.substr(0, sep_pos);

        if (!tag.empty() && element->GetTagName() != tag) {
            return false;
        }

        // 再匹配类或ID
        if (dot_pos != std::string::npos) {
            std::string class_name = base_selector.substr(dot_pos + 1);
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
            std::string id = base_selector.substr(hash_pos + 1);
            if (element->GetAttribute("id") != id) {
                return false;
            }
        }

        return true;
    }

    // 简单标签选择器
    return element->GetTagName() == base_selector;
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

// ========== @keyframes 解析 ==========

std::vector<std::string> StyleManager::FindKeyframesBlocks(const std::string& css_text) const {
    std::vector<std::string> blocks;
    
    if (css_text.empty()) {
        return blocks;
    }
    
    // 查找所有 @keyframes 块
    // 使用手动解析来处理嵌套大括号
    size_t pos = 0;
    while (pos < css_text.length()) {
        // 查找 @keyframes 关键字
        size_t keyframes_pos = css_text.find("@keyframes", pos);
        if (keyframes_pos == std::string::npos) {
            break;
        }
        
        // 查找开始大括号
        size_t brace_start = css_text.find('{', keyframes_pos);
        if (brace_start == std::string::npos) {
            break;
        }
        
        // 计算嵌套大括号，找到匹配的结束大括号
        int brace_count = 1;
        size_t brace_end = brace_start + 1;
        
        while (brace_end < css_text.length() && brace_count > 0) {
            if (css_text[brace_end] == '{') {
                brace_count++;
            } else if (css_text[brace_end] == '}') {
                brace_count--;
            }
            brace_end++;
        }
        
        if (brace_count == 0) {
            // 提取完整的 @keyframes 块
            std::string block = css_text.substr(keyframes_pos, brace_end - keyframes_pos);
            blocks.push_back(block);
        }
        
        pos = brace_end;
    }
    
    return blocks;
}

void StyleManager::ExtractAndRegisterKeyframes(const std::string& css_text) {
    // 查找所有 @keyframes 块
    auto blocks = FindKeyframesBlocks(css_text);
    
    // 解析并注册每个 @keyframes 规则
    for (const auto& block : blocks) {
        KeyframesRule rule = KeyframesRule::Parse(block);
        if (rule.IsValid()) {
            // 注册到动画控制器（如果同名规则已存在，会被覆盖 - CSS 级联行为）
            animation_controller_.RegisterKeyframes(rule);
            
            // 同时注册到全局 KeyframesManager，供 AnimationBoundsCalculator 使用
            // 这是为了解决动画边界计算时需要访问 keyframes 的问题
            KeyframesManager::Instance().RegisterKeyframes(rule);
        }
    }
}

bool StyleManager::HasHoverRules(Element* element) const {
    if (!element) {
        return false;
    }
    
    // 遍历所有样式表，检查是否有匹配该元素的 :hover 规则
    for (const auto& entry : stylesheets_) {
        const auto& rules = entry.sheet->GetRules();
        
        for (const auto& rule : rules) {
            // 检查选择器是否包含 :hover
            if (rule->selector.find(":hover") == std::string::npos) {
                continue;
            }
            
            // 临时设置 hover 状态来检查选择器是否匹配
            // 注意：这里需要检查选择器的基础部分是否匹配元素
            std::string selector = rule->selector;
            
            // 提取 :hover 之前的基础选择器
            size_t hover_pos = selector.find(":hover");
            std::string base_selector = selector.substr(0, hover_pos);
            
            // 如果基础选择器为空（如 ":hover"），则匹配所有元素
            if (base_selector.empty()) {
                return true;
            }
            
            // 检查基础选择器是否匹配元素
            if (MatchesSelector(base_selector, element)) {
                return true;
            }
        }
    }
    
    return false;
}

} // namespace lightui

