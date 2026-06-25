/**
 * @file lexbor_stylesheet.cpp
 * @brief Lexbor CSS StyleSheet 包装类实现
 */
#include "lexbor_stylesheet.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include <lexbor/css/parser.h>
#include <lexbor/css/rule.h>
#include <lexbor/css/selectors/selectors.h>
#include <lexbor/css/stylesheet.h>

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

bool IsNameStart(char ch) {
    unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalpha(uch) || ch == '_' || ch == '-';
}

bool IsNameChar(char ch) {
    unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalnum(uch) || ch == '_' || ch == '-';
}

bool StartsWithAtRule(const std::string& value, const std::string& name) {
    std::string trimmed = TrimASCIIWhitespace(value);
    if (trimmed.size() < name.size()) {
        return false;
    }

    std::string prefix = trimmed.substr(0, name.size());
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return prefix == name;
}

bool ContainsTopLevelPseudoElement(const std::string& selector) {
    int paren_depth = 0;
    int bracket_depth = 0;
    char quote = '\0';
    bool escape_next = false;

    for (size_t i = 0; i + 1 < selector.size(); ++i) {
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
            ++paren_depth;
            continue;
        }
        if (ch == ')' && paren_depth > 0) {
            --paren_depth;
            continue;
        }
        if (ch == '[') {
            ++bracket_depth;
            continue;
        }
        if (ch == ']' && bracket_depth > 0) {
            --bracket_depth;
            continue;
        }
        if (paren_depth == 0 && bracket_depth == 0 && ch == ':' && selector[i + 1] == ':') {
            return true;
        }
    }

    return false;
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

std::string JoinSelectorList(const std::vector<std::string>& selectors) {
    std::string result;
    for (const auto& selector : selectors) {
        if (!result.empty()) {
            result += ", ";
        }
        result += selector;
    }
    return result;
}

std::string SanitizeSelectorListForSupportedElements(const std::string& selector_list) {
    std::vector<std::string> supported_selectors;
    for (const auto& selector : SplitSelectorList(selector_list)) {
        if (!ContainsTopLevelPseudoElement(selector)) {
            supported_selectors.push_back(selector);
        }
    }
    return JoinSelectorList(supported_selectors);
}

size_t FindNextOpenBrace(const std::string& css, size_t start) {
    char quote = '\0';
    bool escape_next = false;
    bool in_comment = false;

    for (size_t i = start; i < css.size(); ++i) {
        char ch = css[i];
        if (in_comment) {
            if (ch == '*' && i + 1 < css.size() && css[i + 1] == '/') {
                in_comment = false;
                ++i;
            }
            continue;
        }
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
        if (ch == '/' && i + 1 < css.size() && css[i + 1] == '*') {
            in_comment = true;
            ++i;
            continue;
        }
        if (ch == '"' || ch == '\'') {
            quote = ch;
            continue;
        }
        if (ch == '{') {
            return i;
        }
    }

    return std::string::npos;
}

size_t FindMatchingCloseBrace(const std::string& css, size_t open_brace) {
    int depth = 1;
    char quote = '\0';
    bool escape_next = false;
    bool in_comment = false;

    for (size_t i = open_brace + 1; i < css.size(); ++i) {
        char ch = css[i];
        if (in_comment) {
            if (ch == '*' && i + 1 < css.size() && css[i + 1] == '/') {
                in_comment = false;
                ++i;
            }
            continue;
        }
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
        if (ch == '/' && i + 1 < css.size() && css[i + 1] == '*') {
            in_comment = true;
            ++i;
            continue;
        }
        if (ch == '"' || ch == '\'') {
            quote = ch;
            continue;
        }
        if (ch == '{') {
            ++depth;
            continue;
        }
        if (ch == '}') {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }

    return std::string::npos;
}

bool ShouldSanitizeNestedAtRule(const std::string& prelude) {
    return StartsWithAtRule(prelude, "@media") ||
           StartsWithAtRule(prelude, "@supports") ||
           StartsWithAtRule(prelude, "@container") ||
           StartsWithAtRule(prelude, "@layer") ||
           StartsWithAtRule(prelude, "@scope");
}

std::string SanitizeUnsupportedPseudoElementRules(const std::string& css) {
    std::string output;
    size_t cursor = 0;

    while (cursor < css.size()) {
        size_t open = FindNextOpenBrace(css, cursor);
        if (open == std::string::npos) {
            output += css.substr(cursor);
            break;
        }

        size_t close = FindMatchingCloseBrace(css, open);
        if (close == std::string::npos) {
            output += css.substr(cursor);
            break;
        }

        std::string prelude = css.substr(cursor, open - cursor);
        std::string block = css.substr(open + 1, close - open - 1);
        std::string trimmed_prelude = TrimASCIIWhitespace(prelude);

        if (trimmed_prelude.empty()) {
            output += css.substr(cursor, close - cursor + 1);
        } else if (!trimmed_prelude.empty() && trimmed_prelude[0] == '@') {
            output += prelude;
            output += "{";
            output += ShouldSanitizeNestedAtRule(trimmed_prelude)
                ? SanitizeUnsupportedPseudoElementRules(block)
                : block;
            output += "}";
        } else if (ContainsTopLevelPseudoElement(trimmed_prelude)) {
            std::string sanitized_selector = SanitizeSelectorListForSupportedElements(prelude);
            if (!TrimASCIIWhitespace(sanitized_selector).empty()) {
                output += sanitized_selector;
                output += "{";
                output += block;
                output += "}";
            }
        } else {
            output += css.substr(cursor, close - cursor + 1);
        }

        cursor = close + 1;
    }

    return output;
}

int CalculateSingleSelectorSpecificity(const std::string& selector) {
    int specificity = 0;
    bool expecting_type_selector = true;

    for (size_t i = 0; i < selector.size(); ++i) {
        char ch = selector[i];

        if (std::isspace(static_cast<unsigned char>(ch)) || ch == '>' || ch == '+' || ch == '~') {
            expecting_type_selector = true;
            continue;
        }

        if (ch == '#') {
            specificity += 100;
            expecting_type_selector = false;
            while (i + 1 < selector.size() && IsNameChar(selector[i + 1])) {
                ++i;
            }
            continue;
        }

        if (ch == '.' || ch == '[' || ch == ':') {
            specificity += 10;
            expecting_type_selector = false;

            if (ch == '[') {
                size_t end = selector.find(']', i + 1);
                i = (end == std::string::npos) ? selector.size() - 1 : end;
            } else if (ch == ':') {
                if (i + 1 < selector.size() && selector[i + 1] == ':') {
                    ++i;
                }
                while (i + 1 < selector.size() && IsNameChar(selector[i + 1])) {
                    ++i;
                }
                if (i + 1 < selector.size() && selector[i + 1] == '(') {
                    size_t end = selector.find(')', i + 2);
                    i = (end == std::string::npos) ? selector.size() - 1 : end;
                }
            } else {
                while (i + 1 < selector.size() && IsNameChar(selector[i + 1])) {
                    ++i;
                }
            }
            continue;
        }

        if (ch == '*') {
            expecting_type_selector = false;
            continue;
        }

        if (expecting_type_selector && IsNameStart(ch)) {
            specificity += 1;
            expecting_type_selector = false;
            while (i + 1 < selector.size() && IsNameChar(selector[i + 1])) {
                ++i;
            }
            continue;
        }

        expecting_type_selector = false;
    }

    return specificity;
}

bool AppendSelectorListRules(
    std::vector<std::unique_ptr<CSSRule>>& rules,
    size_t& next_source_order,
    const std::string& selector_list,
    const std::map<std::string, std::string>& declarations,
    bool important) {
    bool added_rule = false;

    for (const auto& single_selector : SplitSelectorList(selector_list)) {
        auto rule = std::make_unique<CSSRule>();
        rule->selector = single_selector;
        rule->declarations = declarations;
        rule->specificity = CalculateSingleSelectorSpecificity(single_selector);
        rule->source_order = next_source_order++;
        rule->important = important;
        rules.push_back(std::move(rule));
        added_rule = true;
    }

    return added_rule;
}

} // namespace

CSSAssetProvider LexborStyleSheet::asset_provider_ = nullptr;

void LexborStyleSheet::SetAssetProvider(CSSAssetProvider provider) {
    asset_provider_ = provider;
}

CSSAssetProvider LexborStyleSheet::GetAssetProvider() {
    return asset_provider_;
}

// ========== 构造函数和析构函数 ==========

LexborStyleSheet::LexborStyleSheet()
    : parser_(nullptr)
    , stylesheet_(nullptr)
    , rules_()
    , errors_() {
    
    // 创建 CSS 解析器
    parser_ = lxb_css_parser_create();
    if (parser_) {
        lxb_status_t status = lxb_css_parser_init(parser_, nullptr);
        if (status != LXB_STATUS_OK) {
            lxb_css_parser_destroy(parser_, true);
            parser_ = nullptr;
            errors_.push_back("Failed to initialize CSS parser");
        }
    } else {
        errors_.push_back("Failed to create CSS parser");
    }
}

LexborStyleSheet::~LexborStyleSheet() {
    if (stylesheet_) {
        lxb_css_stylesheet_destroy(stylesheet_, true);
        stylesheet_ = nullptr;
    }
    
    if (parser_) {
        lxb_css_parser_destroy(parser_, true);
        parser_ = nullptr;
    }
}

LexborStyleSheet::LexborStyleSheet(LexborStyleSheet&& other) noexcept
    : parser_(other.parser_)
    , stylesheet_(other.stylesheet_)
    , rules_(std::move(other.rules_))
    , errors_(std::move(other.errors_))
    , next_source_order_(other.next_source_order_) {
    
    other.parser_ = nullptr;
    other.stylesheet_ = nullptr;
    other.next_source_order_ = 0;
}

LexborStyleSheet& LexborStyleSheet::operator=(LexborStyleSheet&& other) noexcept {
    if (this != &other) {
        // 清理当前资源
        if (stylesheet_) {
            lxb_css_stylesheet_destroy(stylesheet_, true);
        }
        if (parser_) {
            lxb_css_parser_destroy(parser_, true);
        }
        
        // 移动资源
        parser_ = other.parser_;
        stylesheet_ = other.stylesheet_;
        rules_ = std::move(other.rules_);
        errors_ = std::move(other.errors_);
        next_source_order_ = other.next_source_order_;
        
        other.parser_ = nullptr;
        other.stylesheet_ = nullptr;
        other.next_source_order_ = 0;
    }
    return *this;
}

// ========== CSS 解析 ==========

bool LexborStyleSheet::ParseCSS(const std::string& css) {
    errors_.clear();
    rules_.clear();
    next_source_order_ = 0;
    
    if (!parser_) {
        errors_.push_back("Parser not initialized");
        return false;
    }
    
    // 清理旧的样式表
    if (stylesheet_) {
        lxb_css_stylesheet_destroy(stylesheet_, true);
        stylesheet_ = nullptr;
    }
    
    // 解析 CSS
    std::string parse_css = SanitizeUnsupportedPseudoElementRules(css);
    stylesheet_ = lxb_css_stylesheet_parse(parser_,
                                           reinterpret_cast<const lxb_char_t*>(parse_css.c_str()),
                                           parse_css.length());
    
    if (!stylesheet_) {
        errors_.push_back("Failed to parse CSS");
        return false;
    }
    
    // 提取规则
    ExtractRules();
    
    return true;
}

bool LexborStyleSheet::ParseCSSFile(const std::string& file_path) {
    errors_.clear();
    
    // 1. 优先从嵌入资源加载
    if (asset_provider_) {
        std::vector<uint8_t> asset_data;
        if (asset_provider_(file_path, asset_data)) {
            std::string css(asset_data.begin(), asset_data.end());
            return ParseCSS(css);
        }
    }
    
    // 2. 回退到文件系统
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        errors_.push_back("Failed to open file: " + file_path);
        return false;
    }
    
    std::string css((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
    file.close();
    
    return ParseCSS(css);
}

// ========== 规则访问 ==========

const CSSRule* LexborStyleSheet::GetRule(size_t index) const {
    if (index >= rules_.size()) {
        return nullptr;
    }
    return rules_[index].get();
}

// ========== 规则修改 ==========

bool LexborStyleSheet::AddRule(const std::string& selector,
                                const std::map<std::string, std::string>& declarations) {
    if (AppendSelectorListRules(rules_, next_source_order_, selector, declarations, false)) {
        return true;
    }

    auto rule = std::make_unique<CSSRule>();
    rule->selector = selector;
    rule->declarations = declarations;
    rule->specificity = CalculateSpecificity(selector);
    rule->source_order = next_source_order_++;
    rule->important = false;
    
    rules_.push_back(std::move(rule));
    return true;
}

bool LexborStyleSheet::RemoveRule(size_t index) {
    if (index >= rules_.size()) {
        return false;
    }
    
    rules_.erase(rules_.begin() + index);
    return true;
}

void LexborStyleSheet::ClearRules() {
    rules_.clear();
    next_source_order_ = 0;
}

// ========== 序列化 ==========

std::string LexborStyleSheet::SerializeToCSS() {
    if (!stylesheet_ || !stylesheet_->root) {
        return "";
    }
    
    std::string result;
    
    auto callback = [](const lxb_char_t* data, size_t len, void* ctx) -> lxb_status_t {
        auto* str = static_cast<std::string*>(ctx);
        str->append(reinterpret_cast<const char*>(data), len);
        return LXB_STATUS_OK;
    };
    
    lxb_css_rule_serialize(stylesheet_->root, callback, &result);
    
    return result;
}

// ========== 私有辅助方法 ==========

void LexborStyleSheet::ExtractRules() {
    if (!stylesheet_ || !stylesheet_->root) {
        return;
    }
    
    // 遍历规则树
    lxb_css_rule_t* rule = stylesheet_->root;
    
    // 如果根规则是列表，遍历其子规则
    if (rule->type == LXB_CSS_RULE_LIST) {
        lxb_css_rule_list_t* list = lxb_css_rule_list(rule);
        rule = list->first;
        
        while (rule) {
            if (rule->type == LXB_CSS_RULE_STYLE) {
                ProcessStyleRule(lxb_css_rule_style(rule));
            }
            rule = rule->next;
        }
    } else if (rule->type == LXB_CSS_RULE_STYLE) {
        ProcessStyleRule(lxb_css_rule_style(rule));
    }
}

void LexborStyleSheet::ProcessStyleRule(lxb_css_rule_style_t* style_rule) {
    if (!style_rule || !style_rule->selector) {
        return;
    }
    
    auto rule = std::make_unique<CSSRule>();
    
    // 序列化选择器 - 使用 list_chain 版本来正确处理逗号分隔的选择器列表
    std::string selector_str;
    auto callback = [](const lxb_char_t* data, size_t len, void* ctx) -> lxb_status_t {
        auto* str = static_cast<std::string*>(ctx);
        str->append(reinterpret_cast<const char*>(data), len);
        return LXB_STATUS_OK;
    };
    
    lxb_css_selector_serialize_list_chain(style_rule->selector, callback, &selector_str);
    rule->selector = selector_str;
    rule->specificity = CalculateSpecificity(selector_str);
    
    // 提取声明
    if (style_rule->declarations) {
        lxb_css_rule_declaration_list_t* decl_list = style_rule->declarations;
        lxb_css_rule_t* decl = decl_list->first;
        
        while (decl) {
            if (decl->type == LXB_CSS_RULE_DECLARATION) {
                lxb_css_rule_declaration_t* declaration = lxb_css_rule_declaration(decl);

                // 序列化整个声明（包括属性名和值）
                std::string full_decl;
                lxb_css_rule_serialize(decl, callback, &full_decl);

                // 解析 "property: value" 格式
                size_t colon_pos = full_decl.find(':');
                if (colon_pos != std::string::npos) {
                    // 提取属性名
                    std::string prop_name = full_decl.substr(0, colon_pos);
                    // 去除前后空格
                    size_t start = prop_name.find_first_not_of(" \t\n\r");
                    size_t end = prop_name.find_last_not_of(" \t\n\r");
                    if (start != std::string::npos && end != std::string::npos) {
                        prop_name = prop_name.substr(start, end - start + 1);
                    }

                    // 提取属性值
                    std::string prop_value = full_decl.substr(colon_pos + 1);
                    // 去除前后空格和分号
                    start = prop_value.find_first_not_of(" \t\n\r");
                    end = prop_value.find_last_not_of(" \t\n\r;");
                    if (start != std::string::npos && end != std::string::npos) {
                        prop_value = prop_value.substr(start, end - start + 1);
                    }

                    // 处理 !important：从值中移除并标记
                    bool is_important = declaration->important;
                    size_t important_pos = prop_value.find("!important");
                    if (important_pos != std::string::npos) {
                        is_important = true;
                        prop_value = prop_value.substr(0, important_pos);
                        // 去除尾部空格
                        end = prop_value.find_last_not_of(" \t\n\r");
                        if (end != std::string::npos) {
                            prop_value = prop_value.substr(0, end + 1);
                        }
                    }

                    rule->declarations[prop_name] = prop_value;

                    if (is_important) {
                        rule->important = true;
                    }
                }
            }
            decl = decl->next;
        }
    }
    
    if (!AppendSelectorListRules(rules_, next_source_order_, rule->selector, rule->declarations, rule->important)) {
        rule->source_order = next_source_order_++;
        rules_.push_back(std::move(rule));
    }
}

int LexborStyleSheet::CalculateSpecificity(const std::string& selector) {
    int max_specificity = 0;

    for (const auto& single_selector : SplitSelectorList(selector)) {
        max_specificity = std::max(max_specificity, CalculateSingleSelectorSpecificity(single_selector));
    }

    return max_specificity;
}

} // namespace mblink

