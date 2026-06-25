/**
 * @file style_manager.cpp
 * @brief 样式管理器实现
 */

#include "style_manager.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include "core/render/animation/keyframes.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include <regex>
#include <fstream>
#include <iostream>

namespace mblink {

namespace {

std::string TrimASCIIWhitespace(const std::string& value) {
    size_t start = value.find_first_not_of(" \t\n\r\f");
    if (start == std::string::npos) {
        return "";
    }

    size_t end = value.find_last_not_of(" \t\n\r\f");
    return value.substr(start, end - start + 1);
}

std::string ToLowerASCII(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string RemoveASCIIWhitespace(std::string value) {
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }), value.end());
    return value;
}

bool ParseInteger(const std::string& value, int& result) {
    if (value.empty()) {
        return false;
    }

    size_t index = 0;
    if (value[index] == '+' || value[index] == '-') {
        ++index;
    }
    if (index >= value.size()) {
        return false;
    }

    for (; index < value.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(value[index]))) {
            return false;
        }
    }

    try {
        result = std::stoi(value);
    } catch (...) {
        return false;
    }
    return true;
}

int ElementChildIndex(const Element* element, int* element_child_count = nullptr) {
    if (element_child_count) {
        *element_child_count = 0;
    }
    if (!element) {
        return 0;
    }

    auto parent = element->GetParentNode();
    if (!parent) {
        return 0;
    }

    int index = 0;
    int count = 0;
    for (const auto& child : parent->GetChildNodes()) {
        auto child_element = std::dynamic_pointer_cast<Element>(child);
        if (!child_element) {
            continue;
        }

        ++count;
        if (child_element.get() == element) {
            index = count;
        }
    }

    if (element_child_count) {
        *element_child_count = count;
    }
    return index;
}

bool MatchesAnPlusB(int index, const std::string& expression) {
    if (index <= 0) {
        return false;
    }

    std::string normalized = ToLowerASCII(RemoveASCIIWhitespace(expression));
    if (normalized == "odd") {
        return index % 2 == 1;
    }
    if (normalized == "even") {
        return index % 2 == 0;
    }

    size_t n_pos = normalized.find('n');
    if (n_pos == std::string::npos) {
        int child_index = 0;
        return ParseInteger(normalized, child_index) && index == child_index;
    }

    std::string a_part = normalized.substr(0, n_pos);
    std::string b_part = normalized.substr(n_pos + 1);

    int a = 0;
    if (a_part.empty() || a_part == "+") {
        a = 1;
    } else if (a_part == "-") {
        a = -1;
    } else if (!ParseInteger(a_part, a)) {
        return false;
    }

    int b = 0;
    if (!b_part.empty() && !ParseInteger(b_part, b)) {
        return false;
    }

    if (a == 0) {
        return index == b;
    }

    int delta = index - b;
    if (a > 0) {
        return delta >= 0 && delta % a == 0;
    }

    return delta <= 0 && delta % a == 0;
}

bool MatchesStructuralPseudoClass(const std::string& pseudo_class, const Element* element) {
    std::string pseudo = ToLowerASCII(TrimASCIIWhitespace(pseudo_class));

    if (pseudo == "first-child") {
        return ElementChildIndex(element) == 1;
    }

    if (pseudo == "last-child") {
        int child_count = 0;
        int index = ElementChildIndex(element, &child_count);
        return index > 0 && index == child_count;
    }

    constexpr const char* nth_child_prefix = "nth-child(";
    const size_t prefix_length = std::char_traits<char>::length(nth_child_prefix);
    if (pseudo.rfind(nth_child_prefix, 0) == 0 && pseudo.size() > prefix_length && pseudo.back() == ')') {
        std::string expression = pseudo.substr(prefix_length, pseudo.size() - prefix_length - 1);
        return MatchesAnPlusB(ElementChildIndex(element), expression);
    }

    return false;
}

bool IsStructuralPseudoClass(const std::string& pseudo_class) {
    std::string pseudo = ToLowerASCII(TrimASCIIWhitespace(pseudo_class));
    return pseudo == "first-child" ||
           pseudo == "last-child" ||
           pseudo.rfind("nth-child(", 0) == 0;
}

std::vector<std::string> SplitSelectorList(const std::string& selector_list) {
    std::vector<std::string> selectors;
    std::string current;
    int paren_depth = 0;
    int bracket_depth = 0;
    char quote = '\0';
    bool escape_next = false;

    for (char ch : selector_list) {
        if (escape_next) {
            current += ch;
            escape_next = false;
            continue;
        }

        if (ch == '\\') {
            current += ch;
            escape_next = true;
            continue;
        }

        if (quote != '\0') {
            current += ch;
            if (ch == quote) {
                quote = '\0';
            }
            continue;
        }

        if (ch == '"' || ch == '\'') {
            current += ch;
            quote = ch;
            continue;
        }

        if (ch == '(') {
            ++paren_depth;
            current += ch;
            continue;
        }

        if (ch == ')' && paren_depth > 0) {
            --paren_depth;
            current += ch;
            continue;
        }

        if (ch == '[') {
            ++bracket_depth;
            current += ch;
            continue;
        }

        if (ch == ']' && bracket_depth > 0) {
            --bracket_depth;
            current += ch;
            continue;
        }

        if (ch == ',' && paren_depth == 0 && bracket_depth == 0) {
            std::string trimmed = TrimASCIIWhitespace(current);
            if (!trimmed.empty()) {
                selectors.push_back(trimmed);
            }
            current.clear();
            continue;
        }

        current += ch;
    }

    std::string trimmed = TrimASCIIWhitespace(current);
    if (!trimmed.empty()) {
        selectors.push_back(trimmed);
    }
    return selectors;
}

void PushSelectorPart(std::vector<std::string>& parts, std::string& current_part) {
    std::string trimmed = TrimASCIIWhitespace(current_part);
    if (!trimmed.empty()) {
        parts.push_back(trimmed);
    }
    current_part.clear();
}

bool SplitComplexSelector(
    const std::string& selector,
    std::vector<std::string>& parts,
    std::vector<char>& combinators) {
    std::string current_part;
    bool pending_descendant = false;
    int paren_depth = 0;
    int bracket_depth = 0;
    char quote = '\0';
    bool escape_next = false;

    for (char ch : selector) {
        if (escape_next) {
            current_part += ch;
            escape_next = false;
            continue;
        }

        if (ch == '\\') {
            current_part += ch;
            escape_next = true;
            continue;
        }

        if (quote != '\0') {
            current_part += ch;
            if (ch == quote) {
                quote = '\0';
            }
            continue;
        }

        if (ch == '"' || ch == '\'') {
            current_part += ch;
            quote = ch;
            continue;
        }

        if (ch == '(') {
            ++paren_depth;
            current_part += ch;
            continue;
        }

        if (ch == ')' && paren_depth > 0) {
            --paren_depth;
            current_part += ch;
            continue;
        }

        if (ch == '[') {
            ++bracket_depth;
            current_part += ch;
            continue;
        }

        if (ch == ']' && bracket_depth > 0) {
            --bracket_depth;
            current_part += ch;
            continue;
        }

        if (paren_depth == 0 && bracket_depth == 0 && ch == '>') {
            PushSelectorPart(parts, current_part);
            if (parts.empty()) {
                return false;
            }
            combinators.push_back('>');
            pending_descendant = false;
            continue;
        }

        if (paren_depth == 0 && bracket_depth == 0 && std::isspace(static_cast<unsigned char>(ch))) {
            if (!current_part.empty()) {
                pending_descendant = true;
            }
            continue;
        }

        if (pending_descendant) {
            PushSelectorPart(parts, current_part);
            if (parts.empty()) {
                return false;
            }
            combinators.push_back(' ');
            pending_descendant = false;
        }

        current_part += ch;
    }

    PushSelectorPart(parts, current_part);
    return !parts.empty() && combinators.size() + 1 == parts.size();
}

std::string ExtractPseudoArgument(const std::string& selector, size_t open_paren_pos) {
    int depth = 1;
    char quote = '\0';
    bool escape_next = false;

    for (size_t i = open_paren_pos + 1; i < selector.size(); ++i) {
        char ch = selector[i];

        if (escape_next) {
            escape_next = false;
            continue;
        }
        if (ch == '\\') {
            escape_next = true;
            continue;
        }
        if (quote != '\0') {
            if (ch == quote) {
                quote = '\0';
            }
            continue;
        }
        if (ch == '"' || ch == '\'') {
            quote = ch;
            continue;
        }
        if (ch == '(') {
            ++depth;
            continue;
        }
        if (ch == ')' && --depth == 0) {
            return selector.substr(open_paren_pos + 1, i - open_paren_pos - 1);
        }
    }

    return "";
}

bool IsSelectorNameChar(char ch) {
    unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalnum(uch) || ch == '_' || ch == '-';
}

bool IsSelectorNameStart(char ch) {
    unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalpha(uch) || ch == '_' || ch == '-';
}

bool IsSimpleSelectorBoundary(char ch) {
    return ch == '#' || ch == '.' || ch == '[' || ch == ':';
}

size_t FindMatchingParen(const std::string& selector, size_t open_paren_pos) {
    if (open_paren_pos >= selector.size() || selector[open_paren_pos] != '(') {
        return std::string::npos;
    }

    int depth = 1;
    char quote = '\0';
    bool escape_next = false;

    for (size_t i = open_paren_pos + 1; i < selector.size(); ++i) {
        char ch = selector[i];

        if (escape_next) {
            escape_next = false;
            continue;
        }
        if (ch == '\\') {
            escape_next = true;
            continue;
        }
        if (quote != '\0') {
            if (ch == quote) {
                quote = '\0';
            }
            continue;
        }
        if (ch == '"' || ch == '\'') {
            quote = ch;
            continue;
        }
        if (ch == '(') {
            ++depth;
            continue;
        }
        if (ch == ')' && --depth == 0) {
            return i;
        }
    }

    return std::string::npos;
}

size_t FindMatchingBracket(const std::string& selector, size_t open_bracket_pos) {
    if (open_bracket_pos >= selector.size() || selector[open_bracket_pos] != '[') {
        return std::string::npos;
    }

    char quote = '\0';
    bool escape_next = false;

    for (size_t i = open_bracket_pos + 1; i < selector.size(); ++i) {
        char ch = selector[i];

        if (escape_next) {
            escape_next = false;
            continue;
        }
        if (ch == '\\') {
            escape_next = true;
            continue;
        }
        if (quote != '\0') {
            if (ch == quote) {
                quote = '\0';
            }
            continue;
        }
        if (ch == '"' || ch == '\'') {
            quote = ch;
            continue;
        }
        if (ch == ']') {
            return i;
        }
    }

    return std::string::npos;
}

void SkipASCIIWhitespace(const std::string& value, size_t& index) {
    while (index < value.size() && std::isspace(static_cast<unsigned char>(value[index]))) {
        ++index;
    }
}

std::string ParseAttributeValue(const std::string& value, size_t& index) {
    if (index >= value.size()) {
        return "";
    }

    std::string result;
    if (value[index] == '"' || value[index] == '\'') {
        char quote = value[index++];
        bool escape_next = false;
        while (index < value.size()) {
            char ch = value[index++];
            if (escape_next) {
                result += ch;
                escape_next = false;
                continue;
            }
            if (ch == '\\') {
                escape_next = true;
                continue;
            }
            if (ch == quote) {
                break;
            }
            result += ch;
        }
        return result;
    }

    while (index < value.size() &&
           !std::isspace(static_cast<unsigned char>(value[index]))) {
        result += value[index++];
    }
    return result;
}

bool MatchesAttributeSelector(const std::string& attribute_selector, Element* element) {
    if (!element) {
        return false;
    }

    std::string selector = TrimASCIIWhitespace(attribute_selector);
    if (selector.empty()) {
        return false;
    }

    size_t index = 0;
    SkipASCIIWhitespace(selector, index);

    size_t name_start = index;
    while (index < selector.size() &&
           (IsSelectorNameChar(selector[index]) || selector[index] == ':')) {
        ++index;
    }

    std::string attr_name = selector.substr(name_start, index - name_start);
    if (attr_name.empty()) {
        return false;
    }

    SkipASCIIWhitespace(selector, index);
    if (index >= selector.size()) {
        return element->HasAttribute(attr_name);
    }

    std::string op;
    if ((selector[index] == '~' || selector[index] == '|' ||
         selector[index] == '^' || selector[index] == '$' ||
         selector[index] == '*') &&
        index + 1 < selector.size() && selector[index + 1] == '=') {
        op = selector.substr(index, 2);
        index += 2;
    } else if (selector[index] == '=') {
        op = "=";
        ++index;
    } else {
        return false;
    }

    SkipASCIIWhitespace(selector, index);
    std::string expected_value = ParseAttributeValue(selector, index);
    SkipASCIIWhitespace(selector, index);
    if (index != selector.size() || !element->HasAttribute(attr_name)) {
        return false;
    }

    std::string actual_value = element->GetAttribute(attr_name);
    if (op == "=") {
        return actual_value == expected_value;
    }
    if (op == "~=") {
        std::istringstream stream(actual_value);
        std::string token;
        while (stream >> token) {
            if (token == expected_value) {
                return true;
            }
        }
        return false;
    }
    if (op == "|=") {
        return actual_value == expected_value ||
               (actual_value.size() > expected_value.size() &&
                actual_value.rfind(expected_value + "-", 0) == 0);
    }
    if (op == "^=") {
        return !expected_value.empty() && actual_value.rfind(expected_value, 0) == 0;
    }
    if (op == "$=") {
        return !expected_value.empty() &&
               actual_value.size() >= expected_value.size() &&
               actual_value.compare(actual_value.size() - expected_value.size(),
                                    expected_value.size(),
                                    expected_value) == 0;
    }
    if (op == "*=") {
        return !expected_value.empty() &&
               actual_value.find(expected_value) != std::string::npos;
    }

    return false;
}

} // namespace

// ========== 构造函数和析构函数 ==========

StyleManager::StyleManager(Document* doc)
    : document_(doc)
    , stylesheets_()
    , inline_styles_()
    , hover_rule_cache_()
    , stylesheet_version_(0)
    , next_stylesheet_order_(0) {
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
    
    stylesheets_.emplace_back(sheet, priority, source, next_stylesheet_order_++);
    
    // 按优先级排序（优先级高的在后面，这样后面的会覆盖前面的）
    std::stable_sort(stylesheets_.begin(), stylesheets_.end(),
              [](const StyleSheetEntry& a, const StyleSheetEntry& b) {
                  return a.priority < b.priority;
              });
    InvalidateHoverRuleCache();
}

bool StyleManager::RemoveStyleSheet(std::shared_ptr<LexborStyleSheet> sheet) {
    auto it = std::find_if(stylesheets_.begin(), stylesheets_.end(),
                           [&sheet](const StyleSheetEntry& entry) {
                               return entry.sheet == sheet;
                           });
    
    if (it != stylesheets_.end()) {
        stylesheets_.erase(it);
        InvalidateHoverRuleCache();
        return true;
    }
    
    return false;
}

void StyleManager::ClearStyleSheets() {
    stylesheets_.clear();
    inline_styles_.clear();
    next_stylesheet_order_ = 0;
    InvalidateHoverRuleCache();
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
    
    struct MatchingRule {
        const CSSRule* rule = nullptr;
        int stylesheet_priority = 0;
        size_t stylesheet_order = 0;
    };

    std::vector<MatchingRule> matching_rule_entries;
    
    // 遍历所有样式表
    for (const auto& entry : stylesheets_) {
        const auto& rules = entry.sheet->GetRules();
        
        // 遍历样式表中的所有规则
        for (const auto& rule : rules) {
            if (MatchesSelector(rule->selector, element)) {
                matching_rule_entries.push_back({
                    rule.get(),
                    entry.priority,
                    entry.insertion_order
                });
            }
        }
    }
    
    // 按优先级排序
    std::sort(matching_rule_entries.begin(), matching_rule_entries.end(),
              [](const MatchingRule& a, const MatchingRule& b) {
                  if (a.stylesheet_priority != b.stylesheet_priority) {
                      return a.stylesheet_priority < b.stylesheet_priority;
                  }
                  if (a.rule->specificity != b.rule->specificity) {
                      return a.rule->specificity < b.rule->specificity;
                  }
                  if (a.stylesheet_order != b.stylesheet_order) {
                      return a.stylesheet_order < b.stylesheet_order;
                  }
                  return a.rule->source_order < b.rule->source_order;
              });

    std::vector<const CSSRule*> matching_rules;
    matching_rules.reserve(matching_rule_entries.size());
    for (const auto& entry : matching_rule_entries) {
        matching_rules.push_back(entry.rule);
    }

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

    std::string normalized_selector = TrimASCIIWhitespace(selector);
    if (normalized_selector.empty()) {
        return false;
    }

    auto selector_list = SplitSelectorList(normalized_selector);
    if (selector_list.size() > 1) {
        for (const auto& single_selector : selector_list) {
            if (MatchesSelector(single_selector, element)) {
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> parts;
    std::vector<char> combinators;
    if (!SplitComplexSelector(normalized_selector, parts, combinators)) {
        return false;
    }

    if (parts.size() == 1) {
        return MatchesSimpleSelector(parts[0], element);
    }

    if (!MatchesSimpleSelector(parts.back(), element)) {
        return false;
    }

    auto parent_node = element->GetParentNode();
    Element* ancestor = dynamic_cast<Element*>(parent_node.get());
    int part_index = static_cast<int>(parts.size()) - 2;
    int comb_index = static_cast<int>(combinators.size()) - 1;

    while (ancestor && part_index >= 0 && comb_index >= 0) {
        char combinator = combinators[comb_index];
        if (combinator == '>') {
            if (!MatchesSimpleSelector(parts[part_index], ancestor)) {
                return false;
            }
            --part_index;
            --comb_index;
            parent_node = ancestor->GetParentNode();
            ancestor = dynamic_cast<Element*>(parent_node.get());
            continue;
        }

        if (MatchesSimpleSelector(parts[part_index], ancestor)) {
            --part_index;
            --comb_index;
        }
        parent_node = ancestor->GetParentNode();
        ancestor = dynamic_cast<Element*>(parent_node.get());
    }

    return part_index < 0;
}
bool StyleManager::MatchesSimpleSelector(const std::string& selector, Element* element) const {
    if (!element || selector.empty()) {
        return false;
    }

    std::string simple = TrimASCIIWhitespace(selector);
    if (simple.empty()) {
        return false;
    }

    size_t index = 0;
    if (simple[index] == '*') {
        ++index;
    } else if (!IsSimpleSelectorBoundary(simple[index])) {
        if (!IsSelectorNameStart(simple[index])) {
            return false;
        }
        size_t tag_start = index;
        while (index < simple.size() && IsSelectorNameChar(simple[index])) {
            ++index;
        }
        std::string expected_tag = simple.substr(tag_start, index - tag_start);
        if (ToLowerASCII(element->GetTagName()) != ToLowerASCII(expected_tag)) {
            return false;
        }
    }

    while (index < simple.size()) {
        char ch = simple[index];

        if (ch == '#') {
            ++index;
            size_t id_start = index;
            while (index < simple.size() && IsSelectorNameChar(simple[index])) {
                ++index;
            }
            if (id_start == index ||
                element->GetAttribute("id") != simple.substr(id_start, index - id_start)) {
                return false;
            }
            continue;
        }

        if (ch == '.') {
            ++index;
            size_t class_start = index;
            while (index < simple.size() && IsSelectorNameChar(simple[index])) {
                ++index;
            }
            if (class_start == index ||
                !element->HasClass(simple.substr(class_start, index - class_start))) {
                return false;
            }
            continue;
        }

        if (ch == '[') {
            size_t end = FindMatchingBracket(simple, index);
            if (end == std::string::npos ||
                !MatchesAttributeSelector(simple.substr(index + 1, end - index - 1), element)) {
                return false;
            }
            index = end + 1;
            continue;
        }

        if (ch == ':') {
            if (index + 1 < simple.size() && simple[index + 1] == ':') {
                return false;
            }

            ++index;
            size_t name_start = index;
            while (index < simple.size() && IsSelectorNameChar(simple[index])) {
                ++index;
            }
            if (name_start == index) {
                return false;
            }

            std::string pseudo_class = ToLowerASCII(simple.substr(name_start, index - name_start));
            if (index < simple.size() && simple[index] == '(') {
                size_t end = FindMatchingParen(simple, index);
                if (end == std::string::npos) {
                    return false;
                }
                pseudo_class += "(" + simple.substr(index + 1, end - index - 1) + ")";
                index = end + 1;
            }

            if (pseudo_class == "root") {
                auto parent = element->GetParentNode();
                if (!parent || parent->GetNodeType() != NodeType::DOCUMENT_NODE) {
                    return false;
                }
            } else if (IsStructuralPseudoClass(pseudo_class)) {
                if (!MatchesStructuralPseudoClass(pseudo_class, element)) {
                    return false;
                }
            } else if (!element->HasPseudoClass(pseudo_class)) {
                return false;
            }
            continue;
        }

        return false;
    }

    return true;
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

    std::string selector_signature;
    for (Element* current = element; current;) {
        selector_signature += current->GetTagName();
        selector_signature += '#';
        selector_signature += current->GetAttribute("id");
        selector_signature += '.';
        selector_signature += current->GetClassName();
        selector_signature += '|';

        auto parent_node = current->GetParentNode();
        if (parent_node && parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
            current = static_cast<Element*>(parent_node.get());
        } else {
            current = nullptr;
        }
    }

    auto cached = hover_rule_cache_.find(element);
    if (cached != hover_rule_cache_.end()) {
        const auto& entry = cached->second;
        if (entry.stylesheet_version == stylesheet_version_ &&
            entry.selector_signature == selector_signature) {
            return entry.has_hover_rule;
        }
    }

    auto remember = [&](bool has_hover_rule) {
        hover_rule_cache_[element] = HoverRuleCacheEntry{
            selector_signature,
            stylesheet_version_,
            has_hover_rule
        };
        return has_hover_rule;
    };
    
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
                return remember(true);
            }
            
            // 检查基础选择器是否匹配元素
            if (MatchesSelector(base_selector, element)) {
                return remember(true);
            }
        }
    }
    
    return remember(false);
}

void StyleManager::InvalidateHoverRuleCache() {
    hover_rule_cache_.clear();
    ++stylesheet_version_;
}

} // namespace mblink
