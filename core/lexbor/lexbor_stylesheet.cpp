/**
 * @file lexbor_stylesheet.cpp
 * @brief Lexbor CSS StyleSheet 包装类实现
 */

#include "lexbor_stylesheet.h"
#include <fstream>
#include <sstream>
#include <lexbor/css/stylesheet.h>
#include <lexbor/css/parser.h>
#include <lexbor/css/rule.h>
#include <lexbor/css/selectors/selectors.h>

namespace lightui {

// ========== 静态成员 ==========

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
    , errors_(std::move(other.errors_)) {
    
    other.parser_ = nullptr;
    other.stylesheet_ = nullptr;
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
        
        other.parser_ = nullptr;
        other.stylesheet_ = nullptr;
    }
    return *this;
}

// ========== CSS 解析 ==========

bool LexborStyleSheet::ParseCSS(const std::string& css) {
    errors_.clear();
    rules_.clear();
    
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
    stylesheet_ = lxb_css_stylesheet_parse(parser_,
                                           reinterpret_cast<const lxb_char_t*>(css.c_str()),
                                           css.length());
    
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
    auto rule = std::make_unique<CSSRule>();
    rule->selector = selector;
    rule->declarations = declarations;
    rule->specificity = CalculateSpecificity(selector);
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
    
    rules_.push_back(std::move(rule));
}

int LexborStyleSheet::CalculateSpecificity(const std::string& selector) {
    // 简化的优先级计算
    // ID选择器: 100, 类选择器: 10, 标签选择器: 1
    int specificity = 0;
    
    for (size_t i = 0; i < selector.length(); i++) {
        if (selector[i] == '#') {
            specificity += 100;
        } else if (selector[i] == '.' || selector[i] == '[' || selector[i] == ':') {
            specificity += 10;
        }
    }
    
    // 如果没有特殊选择器，可能是标签选择器
    if (specificity == 0 && !selector.empty()) {
        specificity = 1;
    }
    
    return specificity;
}

} // namespace lightui

