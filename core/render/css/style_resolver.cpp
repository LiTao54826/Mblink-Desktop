/**
 * @file style_resolver.cpp
 * @brief 样式解析器实现
 * 
 * @note 大文件说明 (3054 行)
 * 本文件包含 CSS 样式解析和计算的完整实现。
 * 文件较大的原因：
 * 1. 解析所有 CSS 属性（100+ 属性）
 * 2. 处理 CSS 值的各种格式（长度、颜色、关键字等）
 * 3. 实现样式继承和级联
 * 4. 处理简写属性展开
 * 5. 支持 CSS 变量和 calc()
 * 6. 包含默认样式处理
 *
 * 计划重构：
 * - 按属性类别拆分解析函数
 * - 提取值解析到 css/ 子目录
 */

#include "style_resolver.h"
#include "core/render/objects/render_inline_block.h"
#include "core/render/objects/render_inline_flex.h"
#include "core/render/objects/render_flex.h"
#include "core/render/objects/render_svg.h"
#include "css_clip_path.h"
#include "core/render/animation/animation.h"
#include "core/dom/text.h"
#include "core/dom/document.h"
#include "core/dom/elements/svg_element.h"
#include "core/dom/elements/html_image_element.h"
#include "core/lexbor/style_manager.h"
#include "core/render/utils/color.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <vector>

namespace mbink {

namespace {

std::vector<std::string> SplitWhitespaceTokens(const std::string& value) {
    std::vector<std::string> tokens;
    std::istringstream stream(value);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string TrimCSSValue(const std::string& value) {
    size_t start = value.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = value.find_last_not_of(" \t\n\r");
    return value.substr(start, end - start + 1);
}

bool ContainsNulByte(const std::string& value) {
    return value.find('\0') != std::string::npos;
}

std::string StripNulBytes(const std::string& value) {
    std::string cleaned;
    cleaned.reserve(value.size());
    for (char c : value) {
        if (c != '\0') {
            cleaned += c;
        }
    }
    return cleaned;
}

std::string ToLowerASCII(const std::string& value) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower;
}

bool IsSuffixOf(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string RecoverKnownCSSKeyword(const std::string& value,
                                   const std::vector<std::string>& candidates) {
    std::string cleaned = ToLowerASCII(TrimCSSValue(StripNulBytes(value)));
    if (cleaned.empty()) {
        return "";
    }

    for (const auto& candidate : candidates) {
        if (cleaned == candidate) {
            return candidate;
        }

        if (cleaned.size() >= 3 && IsSuffixOf(candidate, cleaned)) {
            return candidate;
        }
    }

    return "";
}

std::string NormalizeFontFamilyValue(const std::string& value) {
    std::vector<std::string> families;
    std::string current;
    bool in_quote = false;
    char quote_char = '\0';

    auto flush_family = [&]() {
        std::string family = TrimCSSValue(current);
        current.clear();

        if (family.size() >= 2 &&
            ((family.front() == '"' && family.back() == '"') ||
             (family.front() == '\'' && family.back() == '\''))) {
            family = family.substr(1, family.size() - 2);
            family = TrimCSSValue(family);
        }

        if (ContainsNulByte(family)) {
            static const std::vector<std::string> kGenericFamilies = {
                "serif",
                "sans-serif",
                "monospace",
                "cursive",
                "fantasy",
                "system-ui",
                "ui-serif",
                "ui-sans-serif",
                "ui-monospace",
                "ui-rounded"
            };
            family = RecoverKnownCSSKeyword(family, kGenericFamilies);
        }

        bool has_name_char = false;
        for (char c : family) {
            if (!std::isspace(static_cast<unsigned char>(c)) && c != '"' && c != '\'') {
                has_name_char = true;
                break;
            }
        }

        if (has_name_char) {
            families.push_back(family);
        }
    };

    for (char c : value) {
        if (in_quote) {
            current += c;
            if (c == quote_char) {
                in_quote = false;
            }
            continue;
        }

        if (c == '"' || c == '\'') {
            in_quote = true;
            quote_char = c;
            current += c;
        } else if (c == ',') {
            flush_family();
        } else {
            current += c;
        }
    }
    flush_family();

    std::string normalized;
    for (const auto& family : families) {
        if (!normalized.empty()) {
            normalized += ", ";
        }
        normalized += family;
    }
    return normalized;
}

}  // namespace

// ========== StyleResolver 实现 ==========

StyleResolver::StyleResolver() {
    // 初始化可继承属性集合
    inheritable_properties_ = {
        "color",
        "font-family",
        "font-size",
        "font-weight",
        "font-style",
        "line-height",
        "text-align",
        "text-decoration",
        "opacity",
        "letter-spacing",
        "word-spacing",
        "text-indent",
        "white-space",
        // CSS Basic Interaction Properties (Phase 1)
        "pointer-events",
        "user-select",
        // Note: text-transform is NOT inherited by default in CSS
        // Note: word-break is NOT inherited by default in CSS
        // CSS List Style Properties (Phase 2) - inherited
        "list-style-type",
        "list-style-position",
        "list-style-image"
    };
}

ComputedStyle StyleResolver::ResolveStyle(std::shared_ptr<Element> element,
                                         const ComputedStyle* parent_style) {
    if (!element) {
        return ComputedStyle();
    }

    ComputedStyle style;

    // CSS 层叠顺序（从低到高优先级）：
    // 1. 用户代理样式表（默认样式）- 最低优先级
    ApplyDefaultStyle(style, element->GetTagName(), parent_style == nullptr);

    // 2. 继承的值 - 从父元素继承可继承属性
    if (parent_style) {
        ApplyInheritance(style, parent_style);
    }

    // 3. 元素特定的默认样式 - 覆盖继承（如 h1 的 font-size, strong 的 bold）
    // 这一步必须在继承之后，以确保元素自身的样式优先级高于继承
    ApplyElementSpecificStyle(style, element->GetTagName(), element);

    if (element->HasPseudoClass("disabled")) {
        style.opacity = 0.6f;
        style.background_color = "#F5F5F5";
        style.color = "#999999";
    }

    // 4. CSS 规则（<style> 标签和外部样式表）
    ApplyCSSRules(style, element);

    // 5. 伪元素样式（::before, ::after）
    ApplyPseudoElementStyles(style, element);

    // 6. 内联样式
    ApplyInlineStyle(style, element);

    // 7. 伪类样式（如 :hover, :active, :focus）- 最高优先级
    // 放在内联样式之后，确保交互状态能够覆盖静态样式
    // 这符合用户对交互反馈的预期：hover 应该有视觉变化
    ApplyPseudoClassStyles(style, element);

    return style;
}

ComputedStyle StyleResolver::GetDefaultStyle(const std::string& tag_name) {
    // 检查缓存
    auto it = default_styles_.find(tag_name);
    if (it != default_styles_.end()) {
        return it->second;
    }

    ComputedStyle style;
    ApplyDefaultStyle(style, tag_name, true);
    ApplyElementSpecificStyle(style, tag_name);

    // 缓存
    default_styles_[tag_name] = style;

    return style;
}

void StyleResolver::ApplyDefaultStyle(ComputedStyle& style, const std::string& tag_name, bool is_root) {
    // Flexbox 默认值 - 所有元素都需要设置
    style.flex_basis = CSSLength{0.0f, CSSUnit::AUTO};  // flex-basis 的初始值是 auto

    // 只在根元素时设置基础默认值
    if (is_root) {
        style.color = "#000000";
        style.font_family = "Arial";
        style.font_size = 16.0f;
        style.font_weight = "normal";
        style.font_style = "normal";
        style.text_align = "left";
        style.text_decoration = "none";
        style.line_height = 1.2f;  // Chrome 默认 line-height: normal ≈ 1.2
        style.opacity = 1.0f;
    }

    // ========== Chrome 默认 display 属性 ==========
    // 默认为 inline（未知元素默认行为）
    style.display = RenderObjectType::INLINE;

    // ========== display: block 元素 ==========
    if (tag_name == "html" || tag_name == "body" ||
        tag_name == "div" || tag_name == "p" ||
        // 标题
        tag_name == "h1" || tag_name == "h2" || tag_name == "h3" ||
        tag_name == "h4" || tag_name == "h5" || tag_name == "h6" ||
        // 语义化布局标签
        tag_name == "header" || tag_name == "footer" || tag_name == "main" ||
        tag_name == "nav" || tag_name == "section" || tag_name == "article" ||
        tag_name == "aside" || tag_name == "hgroup" || tag_name == "search" ||
        // 图片/媒体容器
        tag_name == "figure" || tag_name == "figcaption" ||
        // 地址
        tag_name == "address" ||
        // 块级格式标签
        tag_name == "pre" || tag_name == "blockquote" || tag_name == "hr" ||
        tag_name == "center" ||
        // 列表
        tag_name == "ul" || tag_name == "ol" || tag_name == "li" ||
        tag_name == "dl" || tag_name == "dt" || tag_name == "dd" ||
        tag_name == "dir" || tag_name == "menu" ||
        // 表单
        tag_name == "form" || tag_name == "fieldset" || tag_name == "legend" ||
        // 多媒体
        tag_name == "video" || tag_name == "audio" ||
        // 其他块级
        tag_name == "layer" || tag_name == "marquee" ||
        tag_name == "noscript" || tag_name == "listing" || tag_name == "xmp" ||
        tag_name == "plaintext") {
        style.display = RenderObjectType::BLOCK;
    }
    // ========== display: none 元素 ==========
    else if (tag_name == "head" || tag_name == "meta" || tag_name == "title" ||
             tag_name == "link" || tag_name == "style" || tag_name == "script" ||
             tag_name == "noscript" || tag_name == "template" ||
             tag_name == "base" || tag_name == "basefont" ||
             tag_name == "datalist" || tag_name == "param" || tag_name == "source" ||
             tag_name == "track" || tag_name == "area" ||
             tag_name == "option" || tag_name == "optgroup") {
        style.display = RenderObjectType::NONE;
    }
    // ========== display: inline 元素 ==========
    else if (tag_name == "span" || tag_name == "a" ||
             // 文本格式标签
             tag_name == "strong" || tag_name == "b" ||
             tag_name == "em" || tag_name == "i" ||
             tag_name == "u" || tag_name == "ins" ||
             tag_name == "s" || tag_name == "strike" || tag_name == "del" ||
             tag_name == "mark" || tag_name == "small" || tag_name == "big" ||
             tag_name == "sub" || tag_name == "sup" ||
             tag_name == "nobr" || tag_name == "font" || tag_name == "tt" ||
             // 代码/键盘标签
             tag_name == "code" || tag_name == "kbd" || tag_name == "samp" || tag_name == "var" ||
             // 引用/定义标签
             tag_name == "abbr" || tag_name == "acronym" ||
             tag_name == "cite" || tag_name == "dfn" || tag_name == "q" ||
             // 时间/数据标签
             tag_name == "time" || tag_name == "data" ||
             // 换行
             tag_name == "br" || tag_name == "wbr" ||
             // Ruby 注音
             tag_name == "ruby" || tag_name == "rt" || tag_name == "rp" ||
             // 双向文本
             tag_name == "bdo" || tag_name == "bdi" ||
             // 表单内联元素
             tag_name == "label" || tag_name == "output" ||
             // map
             tag_name == "map" ||
             // 嵌入式 slot
             tag_name == "slot") {
        style.display = RenderObjectType::INLINE;
    }
    // ========== display: inline-block 元素 ==========
    else if (tag_name == "img" || tag_name == "input" ||
             tag_name == "button" || tag_name == "select" || tag_name == "textarea" ||
             tag_name == "meter" || tag_name == "progress" ||
             tag_name == "canvas" || tag_name == "embed" || tag_name == "object" ||
             tag_name == "iframe" || tag_name == "frame" || tag_name == "frameset") {
        style.display = RenderObjectType::INLINE_BLOCK;
    }
    // ========== display: list-item 元素 ==========
    else if (tag_name == "li" || tag_name == "dd" || tag_name == "dt") {
        // li 在 Chrome 中是 display: list-item，我们用 BLOCK 模拟
        style.display = RenderObjectType::BLOCK;
    }
    // ========== 表格元素 ==========
    else if (tag_name == "table") {
        style.display = RenderObjectType::TABLE;
    }
    else if (tag_name == "thead") {
        style.display = RenderObjectType::TABLE_HEADER_GROUP;
    }
    else if (tag_name == "tbody") {
        style.display = RenderObjectType::TABLE_ROW_GROUP;
    }
    else if (tag_name == "tfoot") {
        style.display = RenderObjectType::TABLE_FOOTER_GROUP;
    }
    else if (tag_name == "tr") {
        style.display = RenderObjectType::TABLE_ROW;
    }
    else if (tag_name == "td" || tag_name == "th") {
        style.display = RenderObjectType::TABLE_CELL;
    }
    else if (tag_name == "caption") {
        style.display = RenderObjectType::TABLE_CAPTION;
    }
    else if (tag_name == "colgroup" || tag_name == "col") {
        // colgroup/col 目前用 NONE 隐藏（暂未实现表格列样式）
        style.display = RenderObjectType::NONE;
    }
    // ========== display: block 但特殊 ==========
    else if (tag_name == "details") {
        style.display = RenderObjectType::BLOCK;
    }
    else if (tag_name == "summary") {
        style.display = RenderObjectType::BLOCK;  // Chrome: display: block (list-item in some contexts)
    }
    else if (tag_name == "dialog") {
        // dialog 的 display 由 ApplyElementSpecificStyle 根据 open 属性决定
        // 这里先设置为 block，后续会根据 open 属性调整
        style.display = RenderObjectType::BLOCK;
    }
    // ========== 虚拟文本组件 ==========
    else if (tag_name == "terminal" || tag_name == "logview") {
        style.display = RenderObjectType::BLOCK;
    }
    // ========== SVG 元素 ==========
    else if (tag_name == "svg") {
        style.display = RenderObjectType::INLINE_BLOCK;  // SVG 内联块
    }
    else if (tag_name == "path" || tag_name == "circle" || tag_name == "rect" ||
             tag_name == "ellipse" || tag_name == "line" || tag_name == "polyline" ||
             tag_name == "polygon" || tag_name == "text" || tag_name == "g" ||
             tag_name == "defs" || tag_name == "use" || tag_name == "symbol" ||
             tag_name == "clipPath" || tag_name == "mask" || tag_name == "pattern" ||
             tag_name == "linearGradient" || tag_name == "radialGradient" || tag_name == "stop") {
        style.display = RenderObjectType::BLOCK;  // SVG 子元素
    }
}

void StyleResolver::ApplyElementSpecificStyle(ComputedStyle& style, const std::string& tag_name, std::shared_ptr<Element> element) {
    // ========== 块级元素 ==========

    // HTML - 默认填满视口
    if (tag_name == "html") {
        style.margin.top = CSSLength(0, CSSUnit::PX);
        style.margin.bottom = CSSLength(0, CSSUnit::PX);
        style.margin.left = CSSLength(0, CSSUnit::PX);
        style.margin.right = CSSLength(0, CSSUnit::PX);
        // html 元素默认 100% 宽高，让子元素的百分比高度能生效
        style.width = CSSLength(100, CSSUnit::PERCENT);
        style.height = CSSLength(100, CSSUnit::PERCENT);
    }

    // BODY - 默认填满视口并启用滚动条
    if (tag_name == "body") {
        style.margin.top = CSSLength(0, CSSUnit::PX);
        style.margin.bottom = CSSLength(0, CSSUnit::PX);
        style.margin.left = CSSLength(0, CSSUnit::PX);
        style.margin.right = CSSLength(0, CSSUnit::PX);
        // body 元素默认 100% 宽高，让子元素的百分比高度能生效
        style.width = CSSLength(100, CSSUnit::PERCENT);
        style.height = CSSLength(100, CSSUnit::PERCENT);
        style.overflow = "auto";
        style.overflow_x = "auto";
        style.overflow_y = "auto";
    }

    // 标题 (Headings)
    if (tag_name == "h1") {
        style.font_size = 32.0f;  // 2em
        style.font_weight = "bold";
        style.margin.top = CSSLength(21, CSSUnit::PX);  // 0.67em
        style.margin.bottom = CSSLength(21, CSSUnit::PX);
    } else if (tag_name == "h2") {
        style.font_size = 24.0f;  // 1.5em
        style.font_weight = "bold";
        style.margin.top = CSSLength(19, CSSUnit::PX);  // 0.83em
        style.margin.bottom = CSSLength(19, CSSUnit::PX);
    } else if (tag_name == "h3") {
        style.font_size = 18.72f;  // 1.17em
        style.font_weight = "bold";
        style.margin.top = CSSLength(18, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(18, CSSUnit::PX);
    } else if (tag_name == "h4") {
        style.font_size = 16.0f;  // 1em
        style.font_weight = "bold";
        style.margin.top = CSSLength(21, CSSUnit::PX);  // 1.33em
        style.margin.bottom = CSSLength(21, CSSUnit::PX);
    } else if (tag_name == "h5") {
        style.font_size = 13.28f;  // 0.83em
        style.font_weight = "bold";
        style.margin.top = CSSLength(22, CSSUnit::PX);  // 1.67em
        style.margin.bottom = CSSLength(22, CSSUnit::PX);
    } else if (tag_name == "h6") {
        style.font_size = 10.72f;  // 0.67em
        style.font_weight = "bold";
        style.margin.top = CSSLength(24, CSSUnit::PX);  // 2.33em
        style.margin.bottom = CSSLength(24, CSSUnit::PX);
    }

    // 段落 (Paragraph)
    if (tag_name == "p") {
        style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        style.margin.left = CSSLength(0, CSSUnit::PX);
        style.margin.right = CSSLength(0, CSSUnit::PX);
    }

    // DIV - 无特殊样式，使用默认块级样式

    // 引用块 (Blockquote)
    // Chrome 浏览器默认样式：margin: 1em 40px
    if (tag_name == "blockquote") {
        style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        style.margin.left = CSSLength(40, CSSUnit::PX);
        style.margin.right = CSSLength(40, CSSUnit::PX);
        // 注意：浏览器默认没有 padding 和 background-color
    }

    // 预格式化文本 (Preformatted) - 等宽字体
    // 注意：浏览器默认使用较小字体（约 13px），但为了测试一致性，使用 16px
    if (tag_name == "pre") {
        style.font_family = "Courier New";
        // 使用 1em 单位而不是固定的16px，这样会根据实际font-size动态计算
        // Chrome浏览器默认：pre { margin: 1em 0; }
        style.margin.top = CSSLength(1, CSSUnit::EM);  // 1em = 1 * font-size
        style.margin.bottom = CSSLength(1, CSSUnit::EM);
        // 浏览器默认 white-space: pre，保留空白和换行，不自动换行
        style.white_space = "pre";
        // 浏览器默认 overflow-x: auto，内容超出时显示水平滚动条
        style.overflow_x = "auto";
    }

    // 代码 (Code) - 等宽字体
    // 注意：浏览器默认使用较小字体（约 13px），但为了测试一致性，使用 16px
    if (tag_name == "code") {
        style.font_family = "Courier New";
        // 不缩小字体，使用继承的 16px，与测试 HTML 中的设置一致
    }

    // 水平线 (Horizontal Rule)
    // Chrome 默认: margin: 8px 0; border: 1px inset (渲染为2px高)
    if (tag_name == "hr") {
        style.margin.top = CSSLength(8, CSSUnit::PX);
        style.margin.bottom = CSSLength(8, CSSUnit::PX);
        style.height = CSSLength(2, CSSUnit::PX);  // Chrome hr 实际高度为 2px (border-top + border-bottom)
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;  // 使用 SOLID 替代 INSET
        style.border.color = SkColorSetRGB(128, 128, 128);  // 灰色边框
    }

    // ========== 列表 (Lists) ==========

    if (tag_name == "ul" || tag_name == "ol") {
        style.padding.left = CSSLength(40, CSSUnit::PX);  // 左侧缩进

        // 浏览器默认行为：嵌套列表（父元素是 ul/ol/dir/menu）没有 margin
        // Chrome: ol ul, ul ol, ul ul, ol ol { margin-block-start: 0; margin-block-end: 0; }
        bool is_nested = false;
        if (element) {
            auto parent = element->GetParentNode();
            if (parent && parent->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto parent_elem = std::static_pointer_cast<Element>(parent);
                std::string parent_tag = parent_elem->GetTagName();
                // 检查是否嵌套在列表项中（li 内的 ul/ol 也视为嵌套）
                if (parent_tag == "ul" || parent_tag == "ol" || parent_tag == "li" ||
                    parent_tag == "dir" || parent_tag == "menu") {
                    is_nested = true;
                }
            }
        }

        if (is_nested) {
            style.margin.top = CSSLength(0, CSSUnit::PX);
            style.margin.bottom = CSSLength(0, CSSUnit::PX);
        } else {
            style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
            style.margin.bottom = CSSLength(16, CSSUnit::PX);
        }
    }

    if (tag_name == "li") {
        style.display = RenderObjectType::BLOCK;  // 列表项是块级
    }

    if (tag_name == "dl") {  // Definition List
        // 浏览器默认行为：嵌套 dl（在 ul/ol/dl 中）没有 margin
        bool is_nested = false;
        if (element) {
            auto parent = element->GetParentNode();
            if (parent && parent->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto parent_elem = std::static_pointer_cast<Element>(parent);
                std::string parent_tag = parent_elem->GetTagName();
                if (parent_tag == "ul" || parent_tag == "ol" || parent_tag == "dl" ||
                    parent_tag == "li" || parent_tag == "dd") {
                    is_nested = true;
                }
            }
        }

        if (is_nested) {
            style.margin.top = CSSLength(0, CSSUnit::PX);
            style.margin.bottom = CSSLength(0, CSSUnit::PX);
        } else {
            style.margin.top = CSSLength(16, CSSUnit::PX);
            style.margin.bottom = CSSLength(16, CSSUnit::PX);
        }
    }

    if (tag_name == "dt") {  // Definition Term
        style.font_weight = "bold";
    }

    if (tag_name == "dd") {  // Definition Description
        style.margin.left = CSSLength(40, CSSUnit::PX);
    }

    // ========== 表格 (Tables) ==========

    if (tag_name == "table") {
        style.border.style = CSSBorderStyle::SOLID;
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.color = SkColorSetRGB(200, 200, 200);
    }

    if (tag_name == "td" || tag_name == "th") {
        // 添加边框
        style.border.style = CSSBorderStyle::SOLID;
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.color = SkColorSetRGB(200, 200, 200);

        // 增加内边距
        style.padding.top = CSSLength(8, CSSUnit::PX);
        style.padding.bottom = CSSLength(8, CSSUnit::PX);
        style.padding.left = CSSLength(8, CSSUnit::PX);
        style.padding.right = CSSLength(8, CSSUnit::PX);

        // 表格单元格默认垂直居中（CSS 规范）
        style.vertical_align = "middle";
    }

    if (tag_name == "th") {
        style.font_weight = "bold";
        style.text_align = "center";
    }

    // ========== 表单元素 (Form Elements) ==========

    if (tag_name == "form") {
        style.margin.top = CSSLength(0, CSSUnit::PX);
    }

    if (tag_name == "fieldset") {
        // Chrome 默认: margin: 0px 2px; padding: 5.6px 12px 10px; border: 2px groove
        style.margin.left = CSSLength(2, CSSUnit::PX);
        style.margin.right = CSSLength(2, CSSUnit::PX);
        style.padding.top = CSSLength(5.6f, CSSUnit::PX);
        style.padding.bottom = CSSLength(10, CSSUnit::PX);
        style.padding.left = CSSLength(12, CSSUnit::PX);
        style.padding.right = CSSLength(12, CSSUnit::PX);
        style.border.width = CSSLength(2, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;  // 用 solid 模拟 groove（暂不支持 groove）
        style.border.color = Color::FromRGB(240, 240, 240);
    }

    if (tag_name == "legend") {
        // Chrome 默认: display: block; padding: 0px 2px
        style.display = RenderObjectType::BLOCK;
        style.padding.left = CSSLength(2, CSSUnit::PX);
        style.padding.right = CSSLength(2, CSSUnit::PX);
        // legend 的位置由 fieldset 的 Paint 方法特殊处理
    }

    // 表单元素基础样式 - 根据浏览器计算样式设置
    // 注意：不同类型的 input 有不同的默认值，在后面单独处理
    if (tag_name == "button" || tag_name == "input" || tag_name == "select" || tag_name == "textarea") {
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = Color::FromRGB(118, 118, 118);  // Chrome 默认边框色
        style.color = "#000000";
    }

    if (tag_name == "button") {
        // 背景色 - Chrome 默认 rgb(239, 239, 239)
        style.background_color = "#EFEFEF";

        // 字体 - Chrome 按钮使用系统 UI 字体
        // 参考: https://developer.mozilla.org/en-US/docs/Web/CSS/font-family
        style.font_family = "system-ui, -apple-system, Arial, sans-serif";

        // 字体大小 - Chrome 按钮默认 13.333px (约等于 13px)
        style.font_size = 13.333f;

        // 圆角 - Chrome 默认较小的圆角（约 2-3px）
        style.border_radius.top_left = CSSLength(2, CSSUnit::PX);
        style.border_radius.top_right = CSSLength(2, CSSUnit::PX);
        style.border_radius.bottom_left = CSSLength(2, CSSUnit::PX);
        style.border_radius.bottom_right = CSSLength(2, CSSUnit::PX);

        // 边框 - Chrome 默认使用浅灰色边框（rgb(118, 118, 118)）
        // 使用 2px solid 模拟 outset 效果（与浏览器一致）
        style.border.width = CSSLength(2, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = SkColorSetRGB(118, 118, 118);

        // 内边距 - Chrome 默认 1px 6px
        style.padding.left = CSSLength(6, CSSUnit::PX);
        style.padding.right = CSSLength(6, CSSUnit::PX);
        style.padding.top = CSSLength(1, CSSUnit::PX);
        style.padding.bottom = CSSLength(1, CSSUnit::PX);

        // 不设置固定宽高，让按钮根据内容自动计算尺寸
        // width 和 height 保持默认的 AUTO

        // 文本居中
        style.text_align = "center";

        // cursor: pointer
        style.cursor = "pointer";
    }

    // Input 元素
    if (tag_name == "input" && element) {
        std::string type = element->GetAttribute("type");

        if (type == "text" || type == "password" || type.empty()) {
            // 文本输入框 - Chrome 默认样式
            style.background_color = "#FFFFFF";

            // 宽度：默认 200px（会被 inline style 覆盖）
            // 高度：不设置固定高度，让 padding + line-height 决定（符合浏览器行为）
            style.width = CSSLength(200, CSSUnit::PX);
            // style.height 保持 AUTO，高度由内容决定

            // 圆角
            style.border_radius.top_left = CSSLength(3, CSSUnit::PX);
            style.border_radius.top_right = CSSLength(3, CSSUnit::PX);
            style.border_radius.bottom_left = CSSLength(3, CSSUnit::PX);
            style.border_radius.bottom_right = CSSLength(3, CSSUnit::PX);

            // 边框 - Chrome 默认 2px inset rgb(118, 118, 118)
            // 注意：CSSBorderStyle 没有 INSET，使用 SOLID 代替
            style.border.width = CSSLength(2, CSSUnit::PX);
            style.border.style = CSSBorderStyle::SOLID;
            style.border.color = SkColorSetRGB(118, 118, 118);

            // 内边距 - Chrome 默认 2px（不是 5px）
            // 注意：Chrome 的 input 元素 padding 比较小
            style.padding.left = CSSLength(2, CSSUnit::PX);
            style.padding.right = CSSLength(2, CSSUnit::PX);
            style.padding.top = CSSLength(2, CSSUnit::PX);
            style.padding.bottom = CSSLength(2, CSSUnit::PX);
        }
        else if (type == "number") {
            // 数字输入框 - Chrome 默认样式
            style.background_color = "#FFFFFF";

            // 宽度：默认 200px（会被 inline style 覆盖）
            // 高度：不设置固定高度，让 padding + line-height 决定（符合浏览器行为）
            style.width = CSSLength(200, CSSUnit::PX);
            // style.height 保持 AUTO，高度由内容决定

            // 圆角
            style.border_radius.top_left = CSSLength(3, CSSUnit::PX);
            style.border_radius.top_right = CSSLength(3, CSSUnit::PX);
            style.border_radius.bottom_left = CSSLength(3, CSSUnit::PX);
            style.border_radius.bottom_right = CSSLength(3, CSSUnit::PX);

            // 边框 - Chrome 默认 2px inset rgb(118, 118, 118)
            // 注意：CSSBorderStyle 没有 INSET，使用 SOLID 代替
            style.border.width = CSSLength(2, CSSUnit::PX);
            style.border.style = CSSBorderStyle::SOLID;
            style.border.color = SkColorSetRGB(118, 118, 118);

            // 内边距 - Chrome 默认 8px
            style.padding.left = CSSLength(8, CSSUnit::PX);
            style.padding.right = CSSLength(8, CSSUnit::PX);
            style.padding.top = CSSLength(8, CSSUnit::PX);
            style.padding.bottom = CSSLength(8, CSSUnit::PX);
        }
        else if (type == "button" || type == "submit") {
            // 按钮样式
            style.background_color = "#F0F0F0";

            // 宽度和高度
            style.width = CSSLength(80, CSSUnit::PX);
            style.height = CSSLength(32, CSSUnit::PX);

            style.border_radius.top_left = CSSLength(4, CSSUnit::PX);
            style.border_radius.top_right = CSSLength(4, CSSUnit::PX);
            style.border_radius.bottom_left = CSSLength(4, CSSUnit::PX);
            style.border_radius.bottom_right = CSSLength(4, CSSUnit::PX);

            style.border.width = CSSLength(1, CSSUnit::PX);
            style.border.style = CSSBorderStyle::SOLID;
            style.border.color = SkColorSetRGB(200, 200, 200);

            style.padding.left = CSSLength(6, CSSUnit::PX);
            style.padding.right = CSSLength(6, CSSUnit::PX);
            style.padding.top = CSSLength(4, CSSUnit::PX);
            style.padding.bottom = CSSLength(4, CSSUnit::PX);

            style.text_align = "center";
        }
        else if (type == "checkbox") {
            // 复选框 - Chrome 默认样式
            // 尺寸: 13x13px
            style.width = CSSLength(13, CSSUnit::PX);
            style.height = CSSLength(13, CSSUnit::PX);

            // margin: 3px 3px 3px 4px
            style.margin.top = CSSLength(3, CSSUnit::PX);
            style.margin.right = CSSLength(3, CSSUnit::PX);
            style.margin.bottom = CSSLength(3, CSSUnit::PX);
            style.margin.left = CSSLength(4, CSSUnit::PX);

            // 无边框（浏览器使用原生控件渲染）
            style.border.width = CSSLength(0, CSSUnit::PX);
            style.border.style = CSSBorderStyle::NONE;

            style.border_radius.top_left = CSSLength(2, CSSUnit::PX);
            style.border_radius.top_right = CSSLength(2, CSSUnit::PX);
            style.border_radius.bottom_left = CSSLength(2, CSSUnit::PX);
            style.border_radius.bottom_right = CSSLength(2, CSSUnit::PX);

            // 透明背景（浏览器使用原生控件渲染）
            style.background_color = "transparent";
        }
        else if (type == "radio") {
            // 单选按钮 - Chrome 默认样式
            // 尺寸: 13x13px
            style.width = CSSLength(13, CSSUnit::PX);
            style.height = CSSLength(13, CSSUnit::PX);

            // margin: 3px 3px 0px 5px
            style.margin.top = CSSLength(3, CSSUnit::PX);
            style.margin.right = CSSLength(3, CSSUnit::PX);
            style.margin.bottom = CSSLength(0, CSSUnit::PX);
            style.margin.left = CSSLength(5, CSSUnit::PX);

            // 无边框（浏览器使用原生控件渲染）
            style.border.width = CSSLength(0, CSSUnit::PX);
            style.border.style = CSSBorderStyle::NONE;

            // 圆形
            style.border_radius.top_left = CSSLength(7, CSSUnit::PX);
            style.border_radius.top_right = CSSLength(7, CSSUnit::PX);
            style.border_radius.bottom_left = CSSLength(7, CSSUnit::PX);
            style.border_radius.bottom_right = CSSLength(7, CSSUnit::PX);

            // 透明背景（浏览器使用原生控件渲染）
            style.background_color = "transparent";
        }
        else if (type == "range") {
            // 范围滑块 - Chrome 默认样式
            // 高度: 16px (Chrome 默认)
            style.height = CSSLength(16, CSSUnit::PX);

            // 默认宽度: 129px (Chrome 默认)
            style.width = CSSLength(129, CSSUnit::PX);

            // 无边框
            style.border.width = CSSLength(0, CSSUnit::PX);
            style.border.style = CSSBorderStyle::NONE;

            // 无内边距
            style.padding.left = CSSLength(0, CSSUnit::PX);
            style.padding.right = CSSLength(0, CSSUnit::PX);
            style.padding.top = CSSLength(0, CSSUnit::PX);
            style.padding.bottom = CSSLength(0, CSSUnit::PX);

            // margin: 2px
            style.margin.top = CSSLength(2, CSSUnit::PX);
            style.margin.right = CSSLength(2, CSSUnit::PX);
            style.margin.bottom = CSSLength(2, CSSUnit::PX);
            style.margin.left = CSSLength(2, CSSUnit::PX);

            // 透明背景
            style.background_color = "transparent";

            // 无 outline（range 滑块不需要焦点轮廓）
            style.outline_style = "none";
            style.outline_width = CSSLength(0, CSSUnit::PX);
        }
    }

    // Textarea 元素
    if (tag_name == "textarea") {
        // Display类型: inline-block (符合CSS标准)
        style.display = RenderObjectType::INLINE_BLOCK;

        style.background_color = "#FFFFFF";

        // 根据rows和cols属性计算宽度和高度 (符合CSS标准)
        // 默认值: cols=20, rows=2 (HTML标准)
        int cols = 20;
        int rows = 2;

        if (element) {
            std::string cols_attr = element->GetAttribute("cols");
            std::string rows_attr = element->GetAttribute("rows");

            if (!cols_attr.empty()) {
                try {
                    cols = std::stoi(cols_attr);
                    if (cols < 1) cols = 20;
                } catch (...) {
                    cols = 20;
                }
            }

            if (!rows_attr.empty()) {
                try {
                    rows = std::stoi(rows_attr);
                    if (rows < 1) rows = 2;
                } catch (...) {
                    rows = 2;
                }
            }
        }

        // 计算宽度: cols * 字符宽度 (约8px per char)
        // 计算高度: rows * 行高 (约20px per line)
        float char_width = 8.0f;
        float line_height = 20.0f;

        style.width = CSSLength(cols * char_width, CSSUnit::PX);
        style.height = CSSLength(rows * line_height, CSSUnit::PX);

        // 圆角
        style.border_radius.top_left = CSSLength(3, CSSUnit::PX);
        style.border_radius.top_right = CSSLength(3, CSSUnit::PX);
        style.border_radius.bottom_left = CSSLength(3, CSSUnit::PX);
        style.border_radius.bottom_right = CSSLength(3, CSSUnit::PX);

        // 边框 - Chrome 默认 2px solid rgb(118, 118, 118)
        // 注意：虽然 CSS 规范默认是 1px，但 Chrome 实际使用 2px
        style.border.width = CSSLength(2, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = SkColorSetRGB(118, 118, 118);
        style.border_top_width = 2.0f;
        style.border_right_width = 2.0f;
        style.border_bottom_width = 2.0f;
        style.border_left_width = 2.0f;

        // 内边距 - Chrome 默认 2px
        style.padding.left = CSSLength(2, CSSUnit::PX);
        style.padding.right = CSSLength(2, CSSUnit::PX);
        style.padding.top = CSSLength(2, CSSUnit::PX);
        style.padding.bottom = CSSLength(2, CSSUnit::PX);
    }

    // Select 元素 (Chrome 风格)
    if (tag_name == "select") {
        // Display类型: inline-block (符合CSS标准)
        style.display = RenderObjectType::INLINE_BLOCK;

        // Chrome 默认 font-size 约 13.3333px
        style.font_size = 13.3333f;

        style.background_color = "#FFFFFF";

        // Chrome 对 select 元素使用 border-box
        style.box_sizing = "border-box";

        // 边框（Chrome 标准灰色边框）
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = SkColorSetRGB(118, 118, 118);  // Chrome 默认边框颜色 #767676
        style.border_top_width = 1.0f;
        style.border_right_width = 1.0f;
        style.border_bottom_width = 1.0f;
        style.border_left_width = 1.0f;

        // 圆角（Chrome 默认轻微圆角）
        style.border_radius.top_left = CSSLength(2, CSSUnit::PX);
        style.border_radius.top_right = CSSLength(2, CSSUnit::PX);
        style.border_radius.bottom_left = CSSLength(2, CSSUnit::PX);
        style.border_radius.bottom_right = CSSLength(2, CSSUnit::PX);

        // Chrome 默认 padding (左侧文字需要间距，右侧留给箭头)
        style.padding.top = CSSLength(1, CSSUnit::PX);
        style.padding.bottom = CSSLength(1, CSSUnit::PX);
        style.padding.left = CSSLength(4, CSSUnit::PX);
        style.padding.right = CSSLength(16, CSSUnit::PX);  // 右侧需要更多空间给下拉箭头
    }

    // Option 元素（隐藏，只在select内部显示选中的）
    if (tag_name == "option") {
        // Option元素默认隐藏，由Select元素负责渲染选中的option
        style.display = RenderObjectType::NONE;
    }

    // OptGroup 元素（隐藏，只在select下拉菜单中显示）
    if (tag_name == "optgroup") {
        // OptGroup元素默认隐藏，由Select元素负责渲染
        style.display = RenderObjectType::NONE;
    }
    
    // Canvas 元素 - 读取width/height属性设置尺寸
    if (tag_name == "canvas" && element) {
        // Canvas默认尺寸: 300x150 (HTML5标准)
        unsigned long width = 300;
        unsigned long height = 150;
        
        // 读取width属性
        std::string width_attr = element->GetAttribute("width");
        if (!width_attr.empty()) {
            try {
                width = std::stoul(width_attr);
            } catch (...) {}
        }
        
        // 读取height属性
        std::string height_attr = element->GetAttribute("height");
        if (!height_attr.empty()) {
            try {
                height = std::stoul(height_attr);
            } catch (...) {}
        }
        
        style.width = CSSLength(static_cast<float>(width), CSSUnit::PX);
        style.height = CSSLength(static_cast<float>(height), CSSUnit::PX);
    }
    
    // Image 元素 - 读取width/height属性设置尺寸
    if (tag_name == "img" && element) {
        // 读取width属性
        std::string width_attr = element->GetAttribute("width");
        if (!width_attr.empty()) {
            try {
                unsigned long width = std::stoul(width_attr);
                style.width = CSSLength(static_cast<float>(width), CSSUnit::PX);
            } catch (...) {}
        }
        
        // 读取height属性
        std::string height_attr = element->GetAttribute("height");
        if (!height_attr.empty()) {
            try {
                unsigned long height = std::stoul(height_attr);
                style.height = CSSLength(static_cast<float>(height), CSSUnit::PX);
            } catch (...) {}
        }
        
        // 如果没有设置宽高，尝试从 HTMLImageElement 获取图片的自然尺寸
        auto img_element = std::dynamic_pointer_cast<HTMLImageElement>(element);
        if (img_element) {
            // 如果没有设置 width 属性，使用图片的自然宽度
            if (width_attr.empty() && img_element->GetNaturalWidth() > 0) {
                style.width = CSSLength(static_cast<float>(img_element->GetNaturalWidth()), CSSUnit::PX);
            }
            // 如果没有设置 height 属性，使用图片的自然高度
            if (height_attr.empty() && img_element->GetNaturalHeight() > 0) {
                style.height = CSSLength(static_cast<float>(img_element->GetNaturalHeight()), CSSUnit::PX);
            }
        }
    }

    // ========== 内联元素 (Inline Elements) ==========

    // 粗体 (Bold)
    if (tag_name == "strong" || tag_name == "b") {
        style.font_weight = "bold";
    }

    // 斜体 (Italic)
    if (tag_name == "em" || tag_name == "i") {
        style.font_style = "italic";
    }

    // 下划线 (Underline)
    if (tag_name == "u") {
        style.text_decoration = "underline";
    }

    // 删除线 (Strikethrough)
    if (tag_name == "s" || tag_name == "strike" || tag_name == "del") {
        style.text_decoration = "line-through";
    }

    // 上标 (Superscript)
    if (tag_name == "sup") {
        style.font_size = style.font_size * 0.83f;  // smaller
        style.vertical_align = "super";
    }

    // 下标 (Subscript)
    if (tag_name == "sub") {
        style.font_size = style.font_size * 0.83f;  // smaller
        style.vertical_align = "sub";
    }

    // 小字体 (Small)
    if (tag_name == "small") {
        style.font_size = 13.28f;  // 0.83em
    }

    // 大字体 (Big)
    if (tag_name == "big") {
        style.font_size = 18.72f;  // 1.17em
    }

    // 链接 (Anchor)
    if (tag_name == "a") {
        style.color = "#0000EE";  // 蓝色
        style.text_decoration = "underline";
    }

    // 标记/高亮 (Mark)
    if (tag_name == "mark") {
        style.background_color = "#FFFF00";  // 黄色背景
        style.color = "#000000";
    }

    // 引用 (Quote)
    if (tag_name == "q") {
        // 浏览器通常会自动添加引号，这里简化处理
    }

    // 缩写 (Abbreviation)
    if (tag_name == "abbr") {
        style.text_decoration = "underline dotted";
    }

    // 插入文本 (Inserted)
    if (tag_name == "ins") {
        style.text_decoration = "underline";
    }

    // 键盘输入 (Keyboard) - 等宽字体
    // 注意：浏览器默认使用较小字体（约 13px），但为了测试一致性，使用 16px
    if (tag_name == "kbd") {
        style.font_family = "Courier New";
        // 不缩小字体，使用继承的 16px，与测试 HTML 中的设置一致
    }

    // 示例输出 (Sample) - 等宽字体
    // 注意：浏览器默认使用较小字体（约 13px），但为了测试一致性，使用 16px
    if (tag_name == "samp") {
        style.font_family = "Courier New";
        // 不缩小字体，使用继承的 16px，与测试 HTML 中的设置一致
    }

    // 变量 (Variable)
    if (tag_name == "var") {
        style.font_style = "italic";
    }

    // 引用来源 (Citation)
    if (tag_name == "cite") {
        style.font_style = "italic";
    }

    // 定义 (Definition)
    if (tag_name == "dfn") {
        style.font_style = "italic";
    }

    // 地址 (Address)
    if (tag_name == "address") {
        style.font_style = "italic";
        style.margin.top = CSSLength(16, CSSUnit::PX);
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
    }

    // 时间 (Time) - 无特殊样式
    // 数据 (Data) - 无特殊样式

    // ========== 语义化布局标签 ==========
    // 这些标签默认无特殊样式，仅作为块级容器

    // 页眉 (Header)
    // 页脚 (Footer)
    // 主内容 (Main)
    // 导航 (Nav)
    // 区块 (Section)
    // 文章 (Article)
    // 侧边栏 (Aside)
    // 以上均使用默认块级样式，无需额外设置

    // 图文容器 (Figure)
    if (tag_name == "figure") {
        style.margin.top = CSSLength(16, CSSUnit::PX);
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        style.margin.left = CSSLength(40, CSSUnit::PX);
        style.margin.right = CSSLength(40, CSSUnit::PX);
    }

    // 图文标题 (Figcaption)
    // Chrome 默认: text-align: start (左对齐)
    if (tag_name == "figcaption") {
        style.text_align = "start";  // 浏览器默认左对齐
    }

    // 表格标题 (Caption)
    // Chrome 默认: text-align: center, 无边框
    if (tag_name == "caption") {
        style.text_align = "center";
        // caption 不应该有边框（与 td/th 不同）
        style.border.style = CSSBorderStyle::NONE;
        style.border.width = CSSLength(0, CSSUnit::PX);
    }

    // 折叠面板 (Details/Summary) - Chrome 默认
    if (tag_name == "details") {
        // Chrome: margin: 0px, padding: 0px
        // 不设置额外 margin
    }

    if (tag_name == "summary") {
        // Chrome: display: list-item, fontWeight: 400, listStyleType: disclosure-closed/open
        // 用 BLOCK 模拟 list-item（暂不支持 list-item）
        style.display = RenderObjectType::BLOCK;
        // 不设置 bold，使用默认 400
    }

    // 对话框 (Dialog) - Chrome 默认
    if (tag_name == "dialog") {
        // dialog 默认隐藏，只有设置 open 属性时才显示
        if (element && element->HasAttribute("open")) {
            style.display = RenderObjectType::BLOCK;
        } else {
            style.display = RenderObjectType::NONE;
        }
        style.background_color = "#FFFFFF";
        style.padding.top = CSSLength(16, CSSUnit::PX);
        style.padding.bottom = CSSLength(16, CSSUnit::PX);
        style.padding.left = CSSLength(16, CSSUnit::PX);
        style.padding.right = CSSLength(16, CSSUnit::PX);
        // Chrome: border: 1.5px solid black
        style.border.width = CSSLength(1.5f, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = SkColorSetRGB(0, 0, 0);
    }

    // Ruby 注音元素 - Chrome 默认
    // ruby: display: ruby（用 INLINE 模拟）
    // rt: display: ruby-text, font-size: 0.75em（用 INLINE 模拟）
    // rp: display: none（括号在支持 ruby 的浏览器中隐藏）
    if (tag_name == "ruby") {
        style.display = RenderObjectType::INLINE;
        // ruby 容器保持 inline
    }
    if (tag_name == "rt") {
        style.display = RenderObjectType::INLINE;
        // Chrome: font-size 约 0.75em (12px when parent is 16px)
        style.font_size = 12.0f;
    }
    if (tag_name == "rp") {
        // rp 在支持 ruby 的浏览器中隐藏
        style.display = RenderObjectType::NONE;
    }

    // 双向文本覆盖 (BDO) - Chrome 默认
    // unicode-bidi: isolate-override, direction 由 dir 属性决定
    if (tag_name == "bdo") {
        style.display = RenderObjectType::INLINE;
        style.unicode_bidi = "isolate-override";
        // direction 由 dir 属性决定，在 ApplyAttributeStyles 中处理
        if (element && element->HasAttribute("dir")) {
            std::string dir = element->GetAttribute("dir");
            if (dir == "rtl") {
                style.direction = "rtl";
            } else if (dir == "ltr") {
                style.direction = "ltr";
            }
        }
    }

    // 进度条 (Progress) - Chrome 默认: 160x16
    // 注意：Chrome 的 progress 元素有默认的 vertical-align: middle
    // 这会影响其在行内的垂直位置
    if (tag_name == "progress") {
        style.display = RenderObjectType::INLINE_BLOCK;
        style.width = CSSLength(160, CSSUnit::PX);
        style.height = CSSLength(16, CSSUnit::PX);
        style.vertical_align = "middle";
    }

    // 度量 (Meter) - Chrome 默认: 80x16
    // 注意：Chrome 的 meter 元素有默认的 vertical-align: middle
    if (tag_name == "meter") {
        style.display = RenderObjectType::INLINE_BLOCK;
        style.width = CSSLength(80, CSSUnit::PX);
        style.height = CSSLength(16, CSSUnit::PX);
        style.vertical_align = "middle";
    }

    // ========== SVG 元素 ==========
    // SVG 根元素
    if (tag_name == "svg") {
        style.display = RenderObjectType::INLINE_BLOCK;
        style.overflow = "hidden";  // SVG 默认裁剪溢出内容
    }

    // SVG 图形元素默认样式
    if (tag_name == "path" || tag_name == "circle" || tag_name == "rect" ||
        tag_name == "ellipse" || tag_name == "line" || tag_name == "polyline" ||
        tag_name == "polygon") {
        // SVG 图形元素默认填充黑色，无描边
        // 这些样式通过 SVGElement 的属性处理，这里只设置布局相关
    }

    // SVG 文本元素
    if (tag_name == "text") {
        style.font_size = 16.0f;  // SVG 默认字体大小
    }

    // SVG 分组元素
    if (tag_name == "g") {
        // g 元素只是容器，不需要特殊样式
    }
}

void StyleResolver::ApplyInlineStyle(ComputedStyle& style, std::shared_ptr<Element> element) {
    // 获取style属性
    std::string style_attr = element->GetAttribute("style");

    if (style_attr.empty()) {
        return;
    }

    std::vector<std::pair<std::string, std::string>> declarations;

    // 解析内联样式（格式：property: value; property: value;）
    std::istringstream iss(style_attr);
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
            declarations.emplace_back(property, value);
        }
    }

    // 先处理自定义属性，确保同一个 style 属性中后定义/先定义的变量都能被后续普通属性引用
    for (const auto& [property, value] : declarations) {
        if (IsCustomProperty(property)) {
            ParseStyleProperty(style, property, value);
        }
    }

    for (const auto& [property, value] : declarations) {
        if (!IsCustomProperty(property)) {
            ParseStyleProperty(style, property, value);
        }
    }
}

void StyleResolver::ApplyInheritance(ComputedStyle& style, const ComputedStyle* parent_style) {
    if (!parent_style) {
        return;
    }

    // 继承 CSS 变量（CSS 变量总是可继承的）
    style.css_variables.InheritFrom(&parent_style->css_variables);

    // 继承可继承属性
    // 注意：text-decoration 在 CSS 规范中不应该被继承
    // 它看起来像继承是因为装饰会绘制在整个元素上包括子元素
    // 但子元素不应该继承这个属性值
    style.color = parent_style->color;
    style.font_family = parent_style->font_family;
    style.font_size = parent_style->font_size;
    style.font_weight = parent_style->font_weight;
    style.font_style = parent_style->font_style;
    style.line_height = parent_style->line_height;
    style.text_align = parent_style->text_align;
    // text_decoration 不继承 - 保持默认值 "none"

    // 继承文本间距相关属性
    style.letter_spacing = parent_style->letter_spacing;
    style.word_spacing = parent_style->word_spacing;
    style.text_indent = parent_style->text_indent;
    style.white_space = parent_style->white_space;

    // CSS Basic Interaction Properties (Phase 1) - 继承
    style.pointer_events = parent_style->pointer_events;
    style.user_select = parent_style->user_select;

    // CSS List Style Properties (Phase 2) - 继承
    style.list_style_type = parent_style->list_style_type;
    style.list_style_position = parent_style->list_style_position;
    style.list_style_image = parent_style->list_style_image;
}

// Helper function to parse layout properties (display, width, height, margin, padding)
bool StyleResolver::ParseLayoutProperty(ComputedStyle& style,
                                        const std::string& property,
                                        const std::string& resolved_value) {
    if (property == "display") {
        style.display = ParseDisplay(resolved_value);
        return true;
    }
    if (property == "box-sizing") {
        if (resolved_value == "border-box" || resolved_value == "content-box") {
            style.box_sizing = resolved_value;
        }
        return true;
    }
    if (property == "width") {
        style.width = CSSValue::ParseLength(resolved_value);
        return true;
    }
    if (property == "height") {
        style.height = CSSValue::ParseLength(resolved_value);
        return true;
    }
    if (property == "min-width") {
        style.min_width = CSSValue::ParseLength(resolved_value);
        return true;
    }
    if (property == "max-width") {
        style.max_width = CSSValue::ParseLength(resolved_value);
        return true;
    }
    if (property == "min-height") {
        style.min_height = CSSValue::ParseLength(resolved_value);
        return true;
    }
    if (property == "max-height") {
        style.max_height = CSSValue::ParseLength(resolved_value);
        return true;
    }
    if (property == "margin") {
        style.margin = CSSValue::ParseEdges(resolved_value);
        style.margin_top = style.margin.top;
        style.margin_right = style.margin.right;
        style.margin_bottom = style.margin.bottom;
        style.margin_left = style.margin.left;
        return true;
    }
    if (property == "margin-top") {
        style.margin.top = CSSValue::ParseLength(resolved_value);
        style.margin_top = style.margin.top;
        return true;
    }
    if (property == "margin-right") {
        style.margin.right = CSSValue::ParseLength(resolved_value);
        style.margin_right = style.margin.right;
        return true;
    }
    if (property == "margin-bottom") {
        style.margin.bottom = CSSValue::ParseLength(resolved_value);
        style.margin_bottom = style.margin.bottom;
        return true;
    }
    if (property == "margin-left") {
        style.margin.left = CSSValue::ParseLength(resolved_value);
        style.margin_left = style.margin.left;
        return true;
    }
    if (property == "padding") {
        style.padding = CSSValue::ParseEdges(resolved_value);
        style.padding_top = style.padding.top;
        style.padding_right = style.padding.right;
        style.padding_bottom = style.padding.bottom;
        style.padding_left = style.padding.left;
        return true;
    }
    if (property == "padding-top") {
        style.padding.top = CSSValue::ParseLength(resolved_value);
        style.padding_top = style.padding.top;
        return true;
    }
    if (property == "padding-right") {
        style.padding.right = CSSValue::ParseLength(resolved_value);
        style.padding_right = style.padding.right;
        return true;
    }
    if (property == "padding-bottom") {
        style.padding.bottom = CSSValue::ParseLength(resolved_value);
        style.padding_bottom = style.padding.bottom;
        return true;
    }
    if (property == "padding-left") {
        style.padding.left = CSSValue::ParseLength(resolved_value);
        style.padding_left = style.padding.left;
        return true;
    }
    // Flexbox item properties
    if (property == "flex") {
        // 解析 flex 简写属性
        // flex: none => flex-grow: 0; flex-shrink: 0; flex-basis: auto
        // flex: auto => flex-grow: 1; flex-shrink: 1; flex-basis: auto
        // flex: <number> => flex-grow: <number>; flex-shrink: 1; flex-basis: 0%
        // flex: <number> <number> => flex-grow; flex-shrink; flex-basis: 0%
        // flex: <number> <number> <length> => flex-grow; flex-shrink; flex-basis
        if (resolved_value == "none") {
            style.flex_grow = 0.0f;
            style.flex_shrink = 0.0f;
            style.flex_basis = CSSLength{0.0f, CSSUnit::AUTO};
        } else if (resolved_value == "auto") {
            style.flex_grow = 1.0f;
            style.flex_shrink = 1.0f;
            style.flex_basis = CSSLength{0.0f, CSSUnit::AUTO};
        } else if (resolved_value == "initial") {
            style.flex_grow = 0.0f;
            style.flex_shrink = 1.0f;
            style.flex_basis = CSSLength{0.0f, CSSUnit::AUTO};
        } else {
            // 尝试解析数值
            std::istringstream iss(resolved_value);
            std::vector<std::string> parts;
            std::string part;
            while (iss >> part) {
                parts.push_back(part);
            }

            if (parts.size() == 1) {
                // flex: <number> => flex-grow: <number>; flex-shrink: 1; flex-basis: 0%
                try {
                    style.flex_grow = std::stof(parts[0]);
                    style.flex_shrink = 1.0f;
                    style.flex_basis = CSSLength{0.0f, CSSUnit::PERCENT};
                } catch (...) {
                    // 可能是 flex-basis 值如 "100px"
                    style.flex_grow = 1.0f;
                    style.flex_shrink = 1.0f;
                    style.flex_basis = CSSValue::ParseLength(parts[0]);
                }
            } else if (parts.size() == 2) {
                // flex: <number> <number> => flex-grow; flex-shrink; flex-basis: 0%
                try {
                    style.flex_grow = std::stof(parts[0]);
                    style.flex_shrink = std::stof(parts[1]);
                    style.flex_basis = CSSLength{0.0f, CSSUnit::PERCENT};
                } catch (...) {
                    // 第二个可能是 flex-basis
                    try {
                        style.flex_grow = std::stof(parts[0]);
                        style.flex_shrink = 1.0f;
                        style.flex_basis = CSSValue::ParseLength(parts[1]);
                    } catch (...) {}
                }
            } else if (parts.size() >= 3) {
                // flex: <number> <number> <length>
                try {
                    style.flex_grow = std::stof(parts[0]);
                    style.flex_shrink = std::stof(parts[1]);
                    style.flex_basis = CSSValue::ParseLength(parts[2]);
                } catch (...) {}
            }
        }
        return true;
    }
    if (property == "flex-grow") {
        try {
            style.flex_grow = std::stof(resolved_value);
        } catch (...) {
            style.flex_grow = 0.0f;
        }
        return true;
    }
    if (property == "flex-shrink") {
        try {
            style.flex_shrink = std::stof(resolved_value);
        } catch (...) {
            style.flex_shrink = 1.0f;
        }
        return true;
    }
    if (property == "flex-basis") {
        style.flex_basis = CSSValue::ParseLength(resolved_value);
        return true;
    }
    if (property == "order") {
        try {
            style.order = std::stoi(resolved_value);
        } catch (...) {
            style.order = 0;
        }
        return true;
    }
    // Flexbox container properties
    if (property == "flex-direction") {
        style.flex_direction = resolved_value;
        return true;
    }
    if (property == "flex-wrap") {
        style.flex_wrap = resolved_value;
        return true;
    }
    if (property == "justify-content") {
        style.justify_content = resolved_value;
        return true;
    }
    if (property == "align-items") {
        style.align_items = resolved_value;
        return true;
    }
    if (property == "justify-items") {
        style.justify_items = resolved_value;
        return true;
    }
    if (property == "align-content") {
        style.align_content = resolved_value;
        return true;
    }
    if (property == "align-self") {
        style.align_self = resolved_value;
        return true;
    }
    if (property == "justify-self") {
        style.justify_self = resolved_value;
        return true;
    }
    if (property == "gap") {
        style.gap = CSSValue::ParseLength(resolved_value);
        style.row_gap = style.gap;
        style.column_gap = style.gap;
        return true;
    }
    if (property == "row-gap") {
        style.row_gap = CSSValue::ParseLength(resolved_value);
        return true;
    }
    if (property == "column-gap") {
        style.column_gap = CSSValue::ParseLength(resolved_value);
        return true;
    }
    return false;
}

// Helper function to parse border properties
bool StyleResolver::ParseBorderProperty(ComputedStyle& style,
                                        const std::string& property,
                                        const std::string& resolved_value) {
    if (property == "border") {
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        style.border = border;
        float width = border.width.ToPx(0, style.font_size);
        style.border_top_width = width;
        style.border_right_width = width;
        style.border_bottom_width = width;
        style.border_left_width = width;
        style.border_top_style = border.style;
        style.border_right_style = border.style;
        style.border_bottom_style = border.style;
        style.border_left_style = border.style;
        style.border_top_color = border.color;
        style.border_right_color = border.color;
        style.border_bottom_color = border.color;
        style.border_left_color = border.color;
        return true;
    }
    if (property == "border-width") {
        style.border.width = CSSValue::ParseLength(resolved_value);
        float width = style.border.width.ToPx(0, style.font_size);
        style.border_top_width = width;
        style.border_right_width = width;
        style.border_bottom_width = width;
        style.border_left_width = width;
        return true;
    }
    if (property == "border-top-width") {
        auto length = CSSValue::ParseLength(resolved_value);
        style.border_top_width = length.ToPx(0, style.font_size);
        return true;
    }
    if (property == "border-right-width") {
        auto length = CSSValue::ParseLength(resolved_value);
        style.border_right_width = length.ToPx(0, style.font_size);
        return true;
    }
    if (property == "border-bottom-width") {
        auto length = CSSValue::ParseLength(resolved_value);
        style.border_bottom_width = length.ToPx(0, style.font_size);
        return true;
    }
    if (property == "border-left-width") {
        auto length = CSSValue::ParseLength(resolved_value);
        style.border_left_width = length.ToPx(0, style.font_size);
        return true;
    }
    if (property == "border-style") {
        style.border.style = CSSValue::ParseBorderStyle(resolved_value);
        style.border_top_style = style.border.style;
        style.border_right_style = style.border.style;
        style.border_bottom_style = style.border.style;
        style.border_left_style = style.border.style;
        return true;
    }
    if (property == "border-color") {
        style.border.color = CSSValue::ParseColor(resolved_value);
        style.border_top_color = style.border.color;
        style.border_right_color = style.border.color;
        style.border_bottom_color = style.border.color;
        style.border_left_color = style.border.color;
        return true;
    }
    if (property == "border-left") {
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        style.border_left_width = border.width.ToPx(0, style.font_size);
        style.border_left_style = border.style;
        style.border_left_color = border.color;
        return true;
    }
    if (property == "border-right") {
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        style.border_right_width = border.width.ToPx(0, style.font_size);
        style.border_right_style = border.style;
        style.border_right_color = border.color;
        return true;
    }
    if (property == "border-top") {
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        style.border_top_width = border.width.ToPx(0, style.font_size);
        style.border_top_style = border.style;
        style.border_top_color = border.color;
        return true;
    }
    if (property == "border-bottom") {
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        style.border_bottom_width = border.width.ToPx(0, style.font_size);
        style.border_bottom_style = border.style;
        style.border_bottom_color = border.color;
        return true;
    }
    if (property == "border-left-style") {
        style.border_left_style = CSSValue::ParseBorderStyle(resolved_value);
        return true;
    }
    if (property == "border-right-style") {
        style.border_right_style = CSSValue::ParseBorderStyle(resolved_value);
        return true;
    }
    if (property == "border-top-style") {
        style.border_top_style = CSSValue::ParseBorderStyle(resolved_value);
        return true;
    }
    if (property == "border-bottom-style") {
        style.border_bottom_style = CSSValue::ParseBorderStyle(resolved_value);
        return true;
    }
    if (property == "border-left-color") {
        style.border_left_color = CSSValue::ParseColor(resolved_value);
        return true;
    }
    if (property == "border-right-color") {
        style.border_right_color = CSSValue::ParseColor(resolved_value);
        return true;
    }
    if (property == "border-top-color") {
        style.border_top_color = CSSValue::ParseColor(resolved_value);
        return true;
    }
    if (property == "border-bottom-color") {
        style.border_bottom_color = CSSValue::ParseColor(resolved_value);
        return true;
    }
    if (property == "border-radius") {
        style.border_radius = CSSValue::ParseBorderRadius(resolved_value);
        return true;
    }
    if (property == "border-collapse") {
        if (resolved_value == "collapse" || resolved_value == "separate") {
            style.border_collapse = resolved_value;
        }
        return true;
    }
    if (property == "border-spacing") {
        style.border_spacing = CSSValue::ParseLength(resolved_value);
        return true;
    }
    return false;
}

// Helper function to parse background properties
bool StyleResolver::ParseBackgroundProperty(ComputedStyle& style,
                                            const std::string& property,
                                            const std::string& resolved_value) {
    if (property == "background") {
        // 处理 background 简写属性
        // 检查是否为渐变
        if (resolved_value.find("linear-gradient") != std::string::npos) {
            auto gradient = CSSValue::ParseLinearGradient(resolved_value);
            if (gradient.has_value()) {
                style.background_linear_gradient = gradient;
            } else {
                // 解析失败，存储原始值
                style.background_image = resolved_value;
            }
        }
        else if (resolved_value.find("radial-gradient") != std::string::npos) {
            auto gradient = CSSValue::ParseRadialGradient(resolved_value);
            if (gradient.has_value()) {
                style.background_radial_gradient = gradient;
            } else {
                // 解析失败，存储原始值
                style.background_image = resolved_value;
            }
        }
        else {
            // 简化处理：如果是颜色值，设置 background-color
            style.background_color = resolved_value;
        }
    }
    else if (property == "background-color") {
        style.background_color = resolved_value;
    }
    else if (property == "background-repeat") {
        style.background_repeat = CSSValue::ParseBackgroundRepeat(resolved_value);
    }
    else if (property == "background-size") {
        // 检测是否有多个值（逗号分隔）
        if (resolved_value.find(',') != std::string::npos) {
            // 多个背景尺寸
            style.background_sizes = CSSValue::ParseMultipleBackgroundSizes(resolved_value);
        } else {
            // 单个背景尺寸（向后兼容）
            style.background_size = CSSValue::ParseBackgroundSize(resolved_value);
        }
    }
    else if (property == "color") {
        style.color = resolved_value;
    }
    else if (property == "font-family") {
        auto normalized_font_family = NormalizeFontFamilyValue(resolved_value);
        if (!normalized_font_family.empty()) {
            style.font_family = normalized_font_family;
        }
    }
    else if (property == "font-size") {
        auto length = CSSValue::ParseLength(resolved_value);
        style.font_size = length.ToPx(16.0f, 16.0f); // 默认基准 16px
    }
    else if (property == "font-weight") {
        style.font_weight = resolved_value;
    }
    else if (property == "font-style") {
        style.font_style = resolved_value;
    }
    else if (property == "text-align") {
        style.text_align = resolved_value;
    }
    else if (property == "text-decoration") {
        style.text_decoration = resolved_value;
    }
    else if (property == "text-indent") {
        style.text_indent = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "letter-spacing") {
        if (resolved_value == "normal") {
            style.letter_spacing = CSSLength(0, CSSUnit::PX);
        } else {
            style.letter_spacing = CSSValue::ParseLength(resolved_value);
        }
    }
    else if (property == "word-spacing") {
        if (resolved_value == "normal") {
            style.word_spacing = CSSLength(0, CSSUnit::PX);
        } else {
            style.word_spacing = CSSValue::ParseLength(resolved_value);
        }
    }
    else if (property == "line-height") {
        // line-height 可以是：
        // 1. "normal" - 使用浏览器默认值（约 1.2 倍）
        // 2. 无单位数字（如 "1.6"）- 表示 font-size 的倍数
        // 3. 带单位的长度（如 "24px", "1.5em"）- 转换为 font-size 的倍数
        // 4. 百分比（如 "150%"）- 表示 font-size 的百分比
        std::string trimmed = resolved_value;
        // 去除首尾空格
        size_t start = trimmed.find_first_not_of(" \t");
        size_t end = trimmed.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            trimmed = trimmed.substr(start, end - start + 1);
        }

        // 调试日志
        static bool debug_line_height = std::getenv("DEBUG_LINE_HEIGHT") != nullptr;
        if (debug_line_height) {
        }

        // 处理 "normal" 关键字 - 使用默认值 1.2（会在渲染时使用 GetBrowserNormalLineHeight）
        if (trimmed == "normal") {
            style.line_height = 1.2f;
            if (debug_line_height) {
            }
        } else {
            // 检查是否为纯数字（无单位）
            bool is_pure_number = true;
            bool has_digit = false;
            for (char c : trimmed) {
                if (std::isdigit(c) || c == '.' || c == '-') {
                    if (std::isdigit(c)) has_digit = true;
                } else {
                    is_pure_number = false;
                    break;
                }
            }

            if (is_pure_number && has_digit) {
                // 无单位数字，直接作为倍数
                try {
                    style.line_height = std::stof(trimmed);
                    if (debug_line_height) {
                    }
                } catch (...) {
                    style.line_height = 1.2f; // 默认值
                }
            } else {
                // 带单位的值，解析并转换为倍数
                auto length = CSSValue::ParseLength(resolved_value);
                if (debug_line_height) {
                }
                if (length.unit == CSSUnit::PERCENT) {
                    style.line_height = length.value / 100.0f;
                } else {
                    float px_value = length.ToPx(style.font_size, style.font_size);
                    // 防止除以零或无效值
                    if (style.font_size > 0 && px_value > 0) {
                        style.line_height = px_value / style.font_size;
                    } else {
                        style.line_height = 1.2f; // 默认值
                    }
                }
                if (debug_line_height) {
                }
            }
        }
    }
    else if (property == "box-shadow") {
        style.box_shadow = CSSValue::ParseBoxShadow(resolved_value);
    }
    else if (property == "text-shadow") {
        style.text_shadow = CSSValue::ParseTextShadow(resolved_value);
    }
    else if (property == "background-image") {
        // 检查是否为渐变
        if (resolved_value.find("linear-gradient") != std::string::npos) {
            // 检测是否有多个渐变（顶层逗号分隔）
            // 简单检测：如果有多个 linear-gradient 关键字，则为多个渐变
            size_t first_pos = resolved_value.find("linear-gradient");
            size_t second_pos = resolved_value.find("linear-gradient", first_pos + 15);

            if (second_pos != std::string::npos) {
                // 多个线性渐变
                style.background_linear_gradients = CSSValue::ParseMultipleLinearGradients(resolved_value);
            } else {
                // 单个线性渐变（向后兼容）
                auto gradient = CSSValue::ParseLinearGradient(resolved_value);
                if (gradient.has_value()) {
                    style.background_linear_gradient = gradient;
                } else {
                    // 解析失败，存储原始值
                    style.background_image = resolved_value;
                }
            }
        }
        else if (resolved_value.find("radial-gradient") != std::string::npos) {
            auto gradient = CSSValue::ParseRadialGradient(resolved_value);
            if (gradient.has_value()) {
                style.background_radial_gradient = gradient;
            } else {
                // 解析失败，存储原始值
                style.background_image = resolved_value;
            }
        }
        else {
            // 普通图片URL
            style.background_image = resolved_value;
        }
    }
    else if (property == "opacity") {
        try {
            style.opacity = std::stof(resolved_value);
            style.opacity = std::max(0.0f, std::min(1.0f, style.opacity));
        } catch (...) {
            style.opacity = 1.0f;
        }
    }
    else if (property == "transform") {
        style.transform_str = resolved_value;
        style.transform = CSSTransform::Parse(resolved_value);
    }
    else if (property == "transform-origin") {
        auto origin = ParseTransformOrigin(resolved_value);
        if (origin.has_value()) {
            style.transform_origin = *origin;
        }
    }
    else if (property == "transition") {
        style.transitions = CSSTransition::Parse(resolved_value);
    }
    else if (property == "filter") {
        style.filter = CSSFilterParser::Parse(resolved_value);
    }
    else if (property == "backdrop-filter") {
        style.backdrop_filter = CSSFilterParser::Parse(resolved_value);
    }
    // Grid 属性
    else if (property == "grid-template-columns") {
        style.grid_template_columns = resolved_value;
    }
    else if (property == "grid-template-rows") {
        style.grid_template_rows = resolved_value;
    }
    else if (property == "grid-auto-columns") {
        style.grid_auto_columns = resolved_value;
    }
    else if (property == "grid-auto-rows") {
        style.grid_auto_rows = resolved_value;
    }
    else if (property == "grid-auto-flow") {
        style.grid_auto_flow = resolved_value;
    }
    else if (property == "grid-column-gap") {
        style.grid_column_gap = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "grid-row-gap") {
        style.grid_row_gap = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "grid-column") {
        style.grid_column = resolved_value;
    }
    else if (property == "grid-row") {
        style.grid_row = resolved_value;
    }
    // 定位属性
    else if (property == "position") {
        style.position = resolved_value;
    }
    else if (property == "top") {
        style.top = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "right") {
        style.right = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "bottom") {
        style.bottom = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "left") {
        style.left = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "z-index") {
        try {
            style.z_index = std::stoi(resolved_value);
        } catch (...) {
            style.z_index = 0;
        }
    }
    // 其他属性
    else if (property == "overflow") {
        style.overflow = resolved_value;
        // 简写属性同时设置 overflow-x 和 overflow-y
        style.overflow_x = resolved_value;
        style.overflow_y = resolved_value;
    }
    else if (property == "overflow-x") {
        style.overflow_x = resolved_value;
    }
    else if (property == "overflow-y") {
        style.overflow_y = resolved_value;
    }
    else if (property == "visibility") {
        style.visibility = resolved_value;
    }
    else if (property == "white-space") {
        style.white_space = resolved_value;
    }
    else if (property == "word-wrap") {
        style.word_wrap = resolved_value;
    }
    else if (property == "text-overflow") {
        style.text_overflow = resolved_value;
    }
    else if (property == "vertical-align") {
        style.vertical_align = resolved_value;
    }
    else if (property == "cursor") {
        style.cursor = resolved_value;
    }
    // Outline properties
    else if (property == "outline") {
        // Parse outline shorthand: [width] [style] [color] in any order
        // Examples: "2px solid red", "solid 2px #ff0000", "red solid 2px"
        std::istringstream iss(resolved_value);
        std::vector<std::string> parts;
        std::string part;
        while (iss >> part) {
            parts.push_back(part);
        }
        
        // Reset to defaults
        style.outline_width = CSSLength(0, CSSUnit::PX);
        style.outline_style = "none";
        style.outline_color = SK_ColorBLACK;
        
        for (const auto& p : parts) {
            // Check if it's a style keyword
            if (p == "none" || p == "solid" || p == "dashed" || p == "dotted" || p == "double") {
                style.outline_style = p;
            }
            // Check if it's a length (contains digits and unit)
            else if (p.find_first_of("0123456789") != std::string::npos) {
                style.outline_width = CSSValue::ParseLength(p);
            }
            // Otherwise assume it's a color
            else {
                style.outline_color = CSSValue::ParseColor(p);
            }
        }
    }
    else if (property == "outline-width") {
        style.outline_width = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "outline-style") {
        // Validate outline-style values: none, solid, dashed, dotted, double
        if (resolved_value == "none" || resolved_value == "solid" || 
            resolved_value == "dashed" || resolved_value == "dotted" || 
            resolved_value == "double") {
            style.outline_style = resolved_value;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    else if (property == "outline-color") {
        style.outline_color = CSSValue::ParseColor(resolved_value);
    }
    else if (property == "outline-offset") {
        style.outline_offset = CSSValue::ParseLength(resolved_value);
    }
    // Text transform property
    else if (property == "text-transform") {
        // Validate text-transform values: none, uppercase, lowercase, capitalize
        if (resolved_value == "none" || resolved_value == "uppercase" || 
            resolved_value == "lowercase" || resolved_value == "capitalize") {
            style.text_transform = resolved_value;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // Pointer events property
    else if (property == "pointer-events") {
        // Validate pointer-events values: auto, none
        if (resolved_value == "auto" || resolved_value == "none") {
            style.pointer_events = resolved_value;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // User select property
    else if (property == "user-select") {
        // Validate user-select values: auto, none, text, all
        if (resolved_value == "auto" || resolved_value == "none" || 
            resolved_value == "text" || resolved_value == "all") {
            style.user_select = resolved_value;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // Word break property
    else if (property == "word-break") {
        // Validate word-break values: normal, break-all, keep-all, break-word
        if (resolved_value == "normal" || resolved_value == "break-all" || 
            resolved_value == "keep-all" || resolved_value == "break-word") {
            style.word_break = resolved_value;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // CSS Media Properties (Phase 2) - object-fit
    else if (property == "object-fit") {
        // Validate object-fit values: fill, contain, cover, none, scale-down
        if (resolved_value == "fill" || resolved_value == "contain" || 
            resolved_value == "cover" || resolved_value == "none" || 
            resolved_value == "scale-down") {
            style.object_fit = resolved_value;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // CSS Media Properties (Phase 2) - object-position
    else if (property == "object-position") {
        // Store the object-position value directly
        // Valid values include:
        // - Keywords: center, top, bottom, left, right, and combinations
        // - Percentages: 50% 50%
        // - Lengths: 10px 20px
        // - Mixed: center 10px, left 50%
        // The value will be parsed during rendering
        if (!resolved_value.empty()) {
            style.object_position = resolved_value;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // CSS Layout Properties (Phase 2) - aspect-ratio
    else if (property == "aspect-ratio") {
        // Parse aspect-ratio values:
        // - auto: use intrinsic aspect ratio if available
        // - <ratio>: e.g., "16 / 9", "4/3", "1", "1.5"
        // - auto <ratio>: prefer intrinsic, fallback to specified ratio
        
        std::string trimmed = resolved_value;
        // Trim whitespace
        size_t start = trimmed.find_first_not_of(" \t");
        size_t end = trimmed.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            trimmed = trimmed.substr(start, end - start + 1);
        }
        
        if (trimmed.empty()) {
            // Invalid, keep default
            return true;  // Property was recognized, just invalid value
        }
        
        // Check for "auto" keyword
        bool has_auto = false;
        std::string ratio_part = trimmed;
        
        if (trimmed.find("auto") == 0) {
            has_auto = true;
            // Check if there's a ratio after "auto"
            size_t auto_end = 4; // length of "auto"
            if (trimmed.length() > auto_end) {
                ratio_part = trimmed.substr(auto_end);
                // Trim leading whitespace from ratio part
                size_t ratio_start = ratio_part.find_first_not_of(" \t");
                if (ratio_start != std::string::npos) {
                    ratio_part = ratio_part.substr(ratio_start);
                } else {
                    ratio_part = "";
                }
            } else {
                ratio_part = "";
            }
        }
        
        // Parse the ratio part
        float ratio = 0.0f;
        if (!ratio_part.empty()) {
            // Check for "/" separator (e.g., "16 / 9" or "16/9")
            size_t slash_pos = ratio_part.find('/');
            if (slash_pos != std::string::npos) {
                // Parse width / height format
                std::string width_str = ratio_part.substr(0, slash_pos);
                std::string height_str = ratio_part.substr(slash_pos + 1);
                
                // Trim whitespace
                size_t ws = width_str.find_first_not_of(" \t");
                size_t we = width_str.find_last_not_of(" \t");
                if (ws != std::string::npos && we != std::string::npos) {
                    width_str = width_str.substr(ws, we - ws + 1);
                }
                
                size_t hs = height_str.find_first_not_of(" \t");
                size_t he = height_str.find_last_not_of(" \t");
                if (hs != std::string::npos && he != std::string::npos) {
                    height_str = height_str.substr(hs, he - hs + 1);
                }
                
                try {
                    float width = std::stof(width_str);
                    float height = std::stof(height_str);
                    if (width > 0 && height > 0) {
                        ratio = width / height;
                    }
                } catch (...) {
                    // Invalid ratio, keep default
                }
            } else {
                // Single number (e.g., "1" or "1.5")
                try {
                    ratio = std::stof(ratio_part);
                    if (ratio <= 0) {
                        ratio = 0.0f; // Invalid
                    }
                } catch (...) {
                    // Invalid ratio, keep default
                }
            }
        }
        
        // Set the aspect-ratio
        if (has_auto && ratio == 0.0f) {
            // Just "auto" - use intrinsic ratio
            style.aspect_ratio.is_auto = true;
            style.aspect_ratio.ratio = 0.0f;
        } else if (has_auto && ratio > 0.0f) {
            // "auto <ratio>" - prefer intrinsic, fallback to specified
            style.aspect_ratio.is_auto = true;
            style.aspect_ratio.ratio = ratio;
        } else if (ratio > 0.0f) {
            // Just a ratio - use specified ratio
            style.aspect_ratio.is_auto = false;
            style.aspect_ratio.ratio = ratio;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // CSS List Style Properties (Phase 2) - list-style-type
    else if (property == "list-style-type") {
        // Validate list-style-type values
        if (resolved_value == "disc" || resolved_value == "circle" || 
            resolved_value == "square" || resolved_value == "decimal" || 
            resolved_value == "decimal-leading-zero" || resolved_value == "lower-roman" || 
            resolved_value == "upper-roman" || resolved_value == "lower-alpha" || 
            resolved_value == "upper-alpha" || resolved_value == "none") {
            style.list_style_type = resolved_value;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // CSS List Style Properties (Phase 2) - list-style-position
    else if (property == "list-style-position") {
        // Validate list-style-position values: inside, outside
        if (resolved_value == "inside" || resolved_value == "outside") {
            style.list_style_position = resolved_value;
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // CSS List Style Properties (Phase 2) - list-style-image
    else if (property == "list-style-image") {
        // Parse url(...) values or "none"
        if (resolved_value == "none") {
            style.list_style_image = "";
        } else if (resolved_value.find("url(") == 0) {
            // Extract URL from url(...) format
            size_t start = resolved_value.find('(') + 1;
            size_t end = resolved_value.rfind(')');
            if (start != std::string::npos && end != std::string::npos && end > start) {
                std::string url = resolved_value.substr(start, end - start);
                // Remove quotes if present
                if ((url.front() == '"' && url.back() == '"') ||
                    (url.front() == '\'' && url.back() == '\'')) {
                    url = url.substr(1, url.length() - 2);
                }
                style.list_style_image = url;
            }
        }
        // Invalid values are silently ignored (CSS behavior)
    }
    // CSS List Style Properties (Phase 2) - list-style shorthand
    else if (property == "list-style") {
        // Parse list-style shorthand: [type] [position] [image] in any order
        // Examples: "disc", "circle inside", "url(marker.png) outside", "square inside url(marker.png)"
        std::istringstream iss(resolved_value);
        std::vector<std::string> parts;
        std::string part;
        
        // Handle url() specially since it may contain spaces
        std::string remaining = resolved_value;
        while (!remaining.empty()) {
            // Trim leading whitespace
            size_t start = remaining.find_first_not_of(" \t");
            if (start == std::string::npos) break;
            remaining = remaining.substr(start);
            
            if (remaining.find("url(") == 0) {
                // Find matching closing parenthesis
                size_t paren_end = remaining.find(')');
                if (paren_end != std::string::npos) {
                    parts.push_back(remaining.substr(0, paren_end + 1));
                    remaining = remaining.substr(paren_end + 1);
                } else {
                    break; // Malformed url()
                }
            } else {
                // Regular token
                size_t space_pos = remaining.find_first_of(" \t");
                if (space_pos != std::string::npos) {
                    parts.push_back(remaining.substr(0, space_pos));
                    remaining = remaining.substr(space_pos);
                } else {
                    parts.push_back(remaining);
                    break;
                }
            }
        }
        
        for (const auto& p : parts) {
            // Check if it's a type keyword
            if (p == "disc" || p == "circle" || p == "square" || 
                p == "decimal" || p == "decimal-leading-zero" || 
                p == "lower-roman" || p == "upper-roman" || 
                p == "lower-alpha" || p == "upper-alpha" || p == "none") {
                style.list_style_type = p;
            }
            // Check if it's a position keyword
            else if (p == "inside" || p == "outside") {
                style.list_style_position = p;
            }
            // Check if it's a url()
            else if (p.find("url(") == 0) {
                size_t url_start = p.find('(') + 1;
                size_t url_end = p.rfind(')');
                if (url_start != std::string::npos && url_end != std::string::npos && url_end > url_start) {
                    std::string url = p.substr(url_start, url_end - url_start);
                    // Remove quotes if present
                    if ((url.front() == '"' && url.back() == '"') ||
                        (url.front() == '\'' && url.back() == '\'')) {
                        url = url.substr(1, url.length() - 2);
                    }
                    style.list_style_image = url;
                }
            }
        }
    }
    // CSS clip-path Property (Phase 3)
    else if (property == "clip-path") {
        // Parse clip-path values: none, inset(), circle(), ellipse(), polygon()
        if (resolved_value == "none" || resolved_value.empty()) {
            style.clip_path = std::nullopt;
        } else {
            style.clip_path = ParseClipPath(resolved_value);
        }
    }
    // CSS will-change Property (用于层提升优化)
    else if (property == "will-change") {
        // Parse will-change values: auto, transform, opacity, scroll-position, contents, etc.
        // 可以是逗号分隔的多个值，如 "transform, opacity"
        style.will_change = resolved_value;
    }
    // CSS Containment Property (用于布局边界优化)
    else if (property == "contain") {
        // Parse contain values: none, layout, paint, size, style, content, strict
        // content = layout + paint + style
        // strict = layout + paint + size + style
        // 也可以是空格分隔的多个值，如 "layout paint"
        if (resolved_value.empty()) {
            style.contain = "none";
        } else {
            style.contain = resolved_value;
        }
    }
    // CSS -webkit-app-region / app-region Property (用于无边框窗口拖拽区域)
    else if (property == "-webkit-app-region" || property == "app-region") {
        if (resolved_value == "drag" || resolved_value == "no-drag") {
            style.app_region = resolved_value;
        } else {
            style.app_region = "";
        }
        return true;
    }
    // CSS -webkit-window-control / window-control Property (用于无边框窗口控制按钮)
    else if (property == "-webkit-window-control" || property == "window-control") {
        if (resolved_value == "close" || resolved_value == "minimize" || resolved_value == "maximize" || resolved_value == "pin") {
            style.window_control = resolved_value;
        } else {
            style.window_control = "";
        }
        return true;
    }
    // CSS scrollbar-color Property (CSS Scrollbars Styling Module Level 1)
    // 格式: scrollbar-color: auto | <thumb-color> <track-color>
    else if (property == "scrollbar-color") {
        if (resolved_value == "auto" || resolved_value.empty()) {
            style.scrollbar_color_auto = true;
            return true;
        }
        // 解析两个颜色值: <thumb-color> <track-color>
        // 需要处理 rgb()/rgba() 等包含空格的颜色函数
        std::string val = resolved_value;
        std::string thumb_str, track_str;

        // 查找第二个颜色的起始位置（跳过第一个颜色值）
        size_t pos = 0;
        int paren_depth = 0;
        bool found_first = false;

        for (size_t i = 0; i < val.size(); i++) {
            if (val[i] == '(') paren_depth++;
            else if (val[i] == ')') paren_depth--;
            else if (val[i] == ' ' && paren_depth == 0 && !found_first) {
                // 跳过连续空格
                thumb_str = val.substr(0, i);
                // 跳过空格找到第二个值
                size_t j = i;
                while (j < val.size() && val[j] == ' ') j++;
                if (j < val.size()) {
                    track_str = val.substr(j);
                    found_first = true;
                }
                break;
            }
        }

        if (!thumb_str.empty() && !track_str.empty()) {
            style.scrollbar_color_auto = false;
            style.scrollbar_thumb_color = Color::Parse(thumb_str);
            style.scrollbar_track_color = Color::Parse(track_str);
        } else {
            // 只有一个值，当作 auto
            style.scrollbar_color_auto = true;
        }
        return true;
    }
    return false;
}

// Helper function to parse animation properties
bool StyleResolver::ParseAnimationProperty(ComputedStyle& style,
                                           const std::string& property,
                                           const std::string& resolved_value) {
    if (property == "animation") {
        // Parse animation shorthand property
        style.animations = CSSAnimation::Parse(resolved_value);
        return true;
    }
    else if (property == "animation-name") {
        // Parse animation-name (can be comma-separated for multiple animations)
        auto names = CSSAnimation::ParseName(resolved_value);
        // Ensure we have enough animation entries
        while (style.animations.size() < names.size()) {
            style.animations.push_back(CSSAnimation());
        }
        for (size_t i = 0; i < names.size(); ++i) {
            style.animations[i].name = names[i];
        }
        return true;
    }
    else if (property == "animation-duration") {
        auto durations = CSSAnimation::ParseDuration(resolved_value);
        while (style.animations.size() < durations.size()) {
            style.animations.push_back(CSSAnimation());
        }
        for (size_t i = 0; i < durations.size(); ++i) {
            style.animations[i].duration = durations[i];
        }
        return true;
    }
    else if (property == "animation-timing-function") {
        auto functions = CSSAnimation::ParseTimingFunction(resolved_value);
        while (style.animations.size() < functions.size()) {
            style.animations.push_back(CSSAnimation());
        }
        for (size_t i = 0; i < functions.size(); ++i) {
            style.animations[i].timing_function = functions[i].first;
            style.animations[i].bezier = functions[i].second;
        }
        return true;
    }
    else if (property == "animation-delay") {
        auto delays = CSSAnimation::ParseDelay(resolved_value);
        while (style.animations.size() < delays.size()) {
            style.animations.push_back(CSSAnimation());
        }
        for (size_t i = 0; i < delays.size(); ++i) {
            style.animations[i].delay = delays[i];
        }
        return true;
    }
    else if (property == "animation-iteration-count") {
        auto counts = CSSAnimation::ParseIterationCount(resolved_value);
        while (style.animations.size() < counts.size()) {
            style.animations.push_back(CSSAnimation());
        }
        for (size_t i = 0; i < counts.size(); ++i) {
            style.animations[i].iteration_count = counts[i];
        }
        return true;
    }
    else if (property == "animation-direction") {
        auto directions = CSSAnimation::ParseDirection(resolved_value);
        while (style.animations.size() < directions.size()) {
            style.animations.push_back(CSSAnimation());
        }
        for (size_t i = 0; i < directions.size(); ++i) {
            style.animations[i].direction = directions[i];
        }
        return true;
    }
    else if (property == "animation-fill-mode") {
        auto modes = CSSAnimation::ParseFillMode(resolved_value);
        while (style.animations.size() < modes.size()) {
            style.animations.push_back(CSSAnimation());
        }
        for (size_t i = 0; i < modes.size(); ++i) {
            style.animations[i].fill_mode = modes[i];
        }
        return true;
    }
    else if (property == "animation-play-state") {
        // Parse play-state and apply to all animations
        bool paused = CSSAnimation::ParsePlayState(resolved_value);
        style.animation_play_state = paused ? "paused" : "running";
        for (auto& anim : style.animations) {
            anim.paused = paused;
        }
        return true;
    }
    return false;
}

void StyleResolver::ParseStyleProperty(ComputedStyle& style,
                                       const std::string& property,
                                       const std::string& value) {
    // 1. Check for CSS custom properties (--custom-property)
    if (IsCustomProperty(property)) {
        style.css_variables.SetVariable(property, value);
        return;
    }

    // 2. Resolve var() functions if present
    std::string resolved_value = value;
    if (CSSVarResolver::ContainsVar(value)) {
        resolved_value = CSSVarResolver::ResolveVar(value, style.css_variables);
    }

    resolved_value = TrimCSSValue(resolved_value);

    if (ContainsNulByte(resolved_value)) {
        static const std::vector<std::string> kCSSWideKeywords = {
            "inherit",
            "initial",
            "unset",
            "revert"
        };
        std::string recovered_keyword = RecoverKnownCSSKeyword(resolved_value, kCSSWideKeywords);
        if (!recovered_keyword.empty()) {
            resolved_value = recovered_keyword;
        }
    }

    if ((resolved_value == "inherit" || resolved_value == "unset") && IsInheritableProperty(property)) {
        return;
    }

    // 3. Expand common alignment shorthands before longhand parsing
    if (property == "place-items" || property == "place-self" || property == "place-content") {
        auto tokens = SplitWhitespaceTokens(resolved_value);
        if (!tokens.empty() && tokens.size() <= 2) {
            const std::string& first_value = tokens[0];
            const std::string& second_value = tokens.size() >= 2 ? tokens[1] : tokens[0];

            if (property == "place-items") {
                ParseLayoutProperty(style, "align-items", first_value);
                ParseLayoutProperty(style, "justify-items", second_value);
                return;
            }
            if (property == "place-self") {
                ParseLayoutProperty(style, "align-self", first_value);
                ParseLayoutProperty(style, "justify-self", second_value);
                return;
            }
            ParseLayoutProperty(style, "align-content", first_value);
            ParseLayoutProperty(style, "justify-content", second_value);
            return;
        }
    }

    // 4. Delegate to category-specific parsers to reduce nesting depth
    if (ParseLayoutProperty(style, property, resolved_value)) return;
    if (ParseBorderProperty(style, property, resolved_value)) return;
    if (ParseAnimationProperty(style, property, resolved_value)) return;
    if (ParseBackgroundProperty(style, property, resolved_value)) return;

    // Unknown property - silently ignored (CSS behavior)
}

void StyleResolver::ApplyCSSRules(ComputedStyle& style, std::shared_ptr<Element> element) {
    if (!style_manager_ || !element) {
        return;
    }

    // 从 StyleManager 获取匹配的 CSS 规则
    auto css_properties = style_manager_->ComputeStyle(element.get());

    // bool debug_target = element->GetTagName() == "html" || element->GetTagName() == "body";
    // if (debug_target) {
    //     std::cerr << "[CSS DEBUG] ApplyCSSRules tag=" << element->GetTagName()
    //               << " properties=" << css_properties.size() << std::endl;
    //     for (const auto& [property, value] : css_properties) {
    //         if (property == "--bg" || property == "--text" || property == "--accent" ||
    //             property == "background" || property == "background-color" || property == "color") {
    //             std::cerr << "  [CSS DEBUG] property " << property << "=" << value << std::endl;
    //         }
    //     }
    // }

    // 先注入自定义属性，再解析普通属性，确保 var() 能读取到同一轮规则中声明的变量
    for (const auto& [property, value] : css_properties) {
        if (IsCustomProperty(property)) {
            ParseStyleProperty(style, property, value);
        }
    }

    // 应用普通 CSS 属性
    for (const auto& [property, value] : css_properties) {
        if (!IsCustomProperty(property)) {
            ParseStyleProperty(style, property, value);
        }
    }
}

void StyleResolver::ApplyPseudoClassStyles(ComputedStyle& style, std::shared_ptr<Element> element) {
    if (!element) {
        return;
    }

    std::string tag_name = element->GetTagName();

    // ========== :hover 伪类样式 ==========
    // 为有背景色的元素提供视觉反馈
    if (element->HasPseudoClass("hover")) {
        if (tag_name == "button" || tag_name == "a") {
            // 获取当前背景色，应用变暗效果
            if (!style.background_color.empty() && style.background_color != "transparent") {
                // 解析当前背景色
                SkColor current_color = Color::Parse(style.background_color);
                
                // 将颜色变暗 15%（hover 效果）
                int r = SkColorGetR(current_color);
                int g = SkColorGetG(current_color);
                int b = SkColorGetB(current_color);
                int a = SkColorGetA(current_color);
                
                // 变暗：乘以 0.85
                r = static_cast<int>(r * 0.85);
                g = static_cast<int>(g * 0.85);
                b = static_cast<int>(b * 0.85);
                
                // 设置新的背景色
                char hex[16];
                if (a == 255) {
                    snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);
                } else {
                    snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", r, g, b, a);
                }
                style.background_color = hex;
            } else {
                // 没有背景色时，使用默认的浅灰色
                style.background_color = "#E8E8E8";
            }
        }
        
        // 链接悬停：下划线
        if (tag_name == "a") {
            style.text_decoration = "underline";
        }
    }

    // ========== :active 伪类样式 ==========
    // 为有背景色的元素提供按下反馈
    if (element->HasPseudoClass("active")) {
        if (tag_name == "button" || tag_name == "a") {
            // 获取当前背景色，应用更深的变暗效果
            if (!style.background_color.empty() && style.background_color != "transparent") {
                // 解析当前背景色
                SkColor current_color = Color::Parse(style.background_color);
                
                // 变暗 25%（active 效果比 hover 更深）
                int r = SkColorGetR(current_color);
                int g = SkColorGetG(current_color);
                int b = SkColorGetB(current_color);
                int a = SkColorGetA(current_color);
                
                // 变暗：乘以 0.75
                r = static_cast<int>(r * 0.75);
                g = static_cast<int>(g * 0.75);
                b = static_cast<int>(b * 0.75);
                
                // 设置新的背景色
                char hex[16];
                if (a == 255) {
                    snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);
                } else {
                    snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", r, g, b, a);
                }
                style.background_color = hex;
            } else {
                // 没有背景色时，使用默认的深灰色
                style.background_color = "#E0E0E0";
            }
        }
    }

    // ========== :focus 伪类样式 ==========
    // 符合浏览器行为：
    // - 鼠标点击 input/textarea：显示 outline（需要显示光标位置）
    // - 鼠标点击 button/select/a：不显示 outline（Chrome 行为）
    // - input[type="range"]：不显示 outline（滑块不需要）
    // - 键盘导航：通过 :focus-visible 处理
    if (element->HasPseudoClass("focus")) {
        // 只有文本类 input 和 textarea 在鼠标点击时显示 outline
        // 因为它们需要显示光标/输入位置
        // range, checkbox, radio 等不需要 outline
        bool needs_outline = false;
        if (tag_name == "textarea") {
            needs_outline = true;
        } else if (tag_name == "input") {
            std::string input_type = element->GetAttribute("type");
            // 只有文本输入类型需要 outline
            if (input_type.empty() || input_type == "text" || input_type == "password" ||
                input_type == "email" || input_type == "tel" || input_type == "url" ||
                input_type == "search" || input_type == "number") {
                needs_outline = true;
            }
        }

        if (needs_outline) {
            // Chrome 的焦点 ring 不是一条固定死黑线，而更接近系统 accent color 的半透明 focus ring。
            // 这里用接近 Chromium 的蓝色半透明描边来模拟更自然的视觉效果。
            style.outline_width = CSSLength(2, CSSUnit::PX);
            style.outline_style = "solid";
            style.outline_color = SkColorSetARGB(168, 26, 115, 232);
            style.outline_offset = CSSLength(0, CSSUnit::PX);
        }
        // button, select, a, range, checkbox, radio 等元素鼠标点击时不显示 outline
    }

    // ========== :focus-visible 伪类样式 ==========
    // 键盘导航时的焦点指示器（所有可聚焦元素都显示，但 range 等除外）
    // 这符合现代浏览器的行为：https://developer.mozilla.org/en-US/docs/Web/CSS/:focus-visible
    if (element->HasPseudoClass("focus-visible")) {
        bool needs_outline = false;
        if (tag_name == "button" || tag_name == "textarea" || tag_name == "select" || tag_name == "a") {
            needs_outline = true;
        } else if (tag_name == "input") {
            std::string input_type = element->GetAttribute("type");
            // range 不需要键盘焦点 outline
            if (input_type != "range") {
                needs_outline = true;
            }
        }

        if (needs_outline) {
            // 键盘焦点同样使用接近 Chromium 的蓝色 focus ring，保证与鼠标 focus 视觉一致。
            style.outline_width = CSSLength(2, CSSUnit::PX);
            style.outline_style = "solid";
            style.outline_color = SkColorSetARGB(168, 26, 115, 232);
            style.outline_offset = CSSLength(0, CSSUnit::PX);
        }
    }

    // ========== :checked 伪类样式 ==========
    if (element->HasPseudoClass("checked")) {
        if (tag_name == "input") {
            std::string type = element->GetAttribute("type");
            if (type == "checkbox" || type == "radio") {
                // 选中状态：蓝色背景
                style.background_color = "#4299E1";
                style.border.color = SkColorSetRGB(66, 153, 225);
            }
        }
    }
}

void StyleResolver::ApplyPseudoElementStyles(ComputedStyle& style, std::shared_ptr<Element> element) {
    if (!element) {
        return;
    }

    std::string tag_name = element->GetTagName();

    // 转换为小写进行比较（HTML标签名不区分大小写）
    std::string tag_name_lower = tag_name;
    std::transform(tag_name_lower.begin(), tag_name_lower.end(), tag_name_lower.begin(), ::tolower);

    // ========== ::before 和 ::after 伪元素样式 ==========
    // 根据 HTML 标准，某些元素有默认的伪元素内容

    // <q> 引用元素 - 浏览器默认添加引号
    // CSS 规范: q::before { content: open-quote; } q::after { content: close-quote; }
    if (tag_name_lower == "q") {
        style.has_before = true;
        style.has_after = true;
        // 使用中文引号（也可以根据 lang 属性选择不同引号）
        // 英文使用 """ 和 """，中文使用 "「" 和 "」" 或 """ 和 """
        style.content_before = "\xe2\x80\x9c";  // UTF-8 编码的 "
        style.content_after = "\xe2\x80\x9d";   // UTF-8 编码的 "
    }

    // 未来可以在这里添加更多伪元素支持，例如：
    // - <li> 的列表标记（虽然这通常用 ::marker 而非 ::before）
    // - 自定义 CSS 规则中的 content 属性
}

bool StyleResolver::IsInheritableProperty(const std::string& property) {
    return inheritable_properties_.find(property) != inheritable_properties_.end();
}

RenderObjectType StyleResolver::ParseDisplay(const std::string& value) {
    if (value == "block") return RenderObjectType::BLOCK;
    if (value == "inline") return RenderObjectType::INLINE;
    if (value == "inline-block") return RenderObjectType::INLINE_BLOCK;
    if (value == "flex") return RenderObjectType::FLEX;
    if (value == "inline-flex") return RenderObjectType::INLINE_FLEX;  // inline-flex 使用独立的 INLINE_FLEX 类型
    if (value == "grid") return RenderObjectType::GRID;
    if (value == "inline-grid") return RenderObjectType::INLINE_GRID;  // inline-grid 使用独立的 INLINE_GRID 类型
    if (value == "none") return RenderObjectType::NONE;
    if (value == "contents") return RenderObjectType::CONTENTS;  // display: contents
    // 表格相关display类型
    if (value == "table") return RenderObjectType::TABLE;
    if (value == "table-row-group") return RenderObjectType::TABLE_ROW_GROUP;
    if (value == "table-header-group") return RenderObjectType::TABLE_HEADER_GROUP;
    if (value == "table-footer-group") return RenderObjectType::TABLE_FOOTER_GROUP;
    if (value == "table-row") return RenderObjectType::TABLE_ROW;
    if (value == "table-cell") return RenderObjectType::TABLE_CELL;
    if (value == "table-caption") return RenderObjectType::TABLE_CAPTION;
    return RenderObjectType::BLOCK;
}

CSSBorder StyleResolver::ParseBorderShorthand(const std::string& value, float font_size) {
    CSSBorder border;
    border.style = CSSBorderStyle::NONE;
    border.width = CSSLength(0, CSSUnit::PX);
    border.color = SK_ColorBLACK;

    if (value.empty() || value == "none") {
        return border;
    }

    // 处理 rgb()/rgba()/hsl()/hsla() 颜色函数，避免被空格分割
    std::string processed_value = value;
    std::string color_part;
    size_t color_start = std::string::npos;
    size_t color_end = std::string::npos;

    // 查找颜色函数
    size_t rgb_pos = processed_value.find("rgb(");
    size_t rgba_pos = processed_value.find("rgba(");
    size_t hsl_pos = processed_value.find("hsl(");
    size_t hsla_pos = processed_value.find("hsla(");

    if (rgb_pos != std::string::npos) {
        color_start = rgb_pos;
    } else if (rgba_pos != std::string::npos) {
        color_start = rgba_pos;
    } else if (hsl_pos != std::string::npos) {
        color_start = hsl_pos;
    } else if (hsla_pos != std::string::npos) {
        color_start = hsla_pos;
    }

    // 如果找到颜色函数，提取完整的颜色字符串
    if (color_start != std::string::npos) {
        color_end = processed_value.find(')', color_start);
        if (color_end != std::string::npos) {
            color_part = processed_value.substr(color_start, color_end - color_start + 1);
            // 从原字符串中移除颜色部分
            processed_value = processed_value.substr(0, color_start) +
                            processed_value.substr(color_end + 1);
        }
    }

    // 分割剩余的值
    std::istringstream iss(processed_value);
    std::vector<std::string> parts;
    std::string part;
    while (iss >> part) {
        parts.push_back(part);
    }

    // 解析各个部分
    for (const auto& p : parts) {
        // 尝试解析为长度值（宽度）
        if (p.find("px") != std::string::npos ||
            p.find("em") != std::string::npos ||
            p.find("rem") != std::string::npos ||
            (!p.empty() && std::isdigit(p[0]))) {
            border.width = CSSValue::ParseLength(p);
        }
        // 尝试解析为样式
        else if (p == "solid" || p == "dashed" || p == "dotted" || p == "double" || p == "none") {
            border.style = CSSValue::ParseBorderStyle(p);
        }
        // 尝试解析为颜色（非函数形式）
        else if (p[0] == '#' ||
                 p == "black" || p == "white" || p == "red" || p == "green" || p == "blue" ||
                 p == "transparent" || p == "currentColor") {
            border.color = CSSValue::ParseColor(p);
        }
    }

    // 解析提取的颜色函数
    if (!color_part.empty()) {
        border.color = CSSValue::ParseColor(color_part);
    }

    return border;
}

// ========== RenderTreeBuilder 实现 ==========

RenderTreeBuilder::RenderTreeBuilder()
    : style_resolver_() {
}

std::shared_ptr<RenderObject> RenderTreeBuilder::BuildRenderTree(
    std::shared_ptr<Node> node,
    const ComputedStyle* parent_style) {

    if (!node) {
        return nullptr;
    }

    // 设置 StyleManager（如果有 Document）
    if (document_ && document_->GetStyleManager()) {
        style_resolver_.SetStyleManager(document_->GetStyleManager());
    }

    std::shared_ptr<RenderObject> render_obj;

    // 根据节点类型创建渲染对象
    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        
        // 先计算样式，检查是否是 display: contents
        auto style = style_resolver_.ResolveStyle(element, parent_style);
        
        // display: contents 元素不创建渲染对象，但需要处理子元素
        // 子元素会在父级的 BuildRenderTree 中被处理
        if (style.display == RenderObjectType::CONTENTS) {
            return nullptr;  // 不创建渲染对象
        }
        
        render_obj = CreateRenderObjectForElement(element, parent_style);
    }
    else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::static_pointer_cast<Text>(node);
        render_obj = CreateRenderObjectForText(text, parent_style);
    }

    if (!render_obj) {
        return nullptr;
    }

    // 如果是 display: none，不创建渲染对象
    if (render_obj->GetComputedStyle().display == RenderObjectType::NONE) {
        return nullptr;
    }

    // 建立 DOM 节点与 RenderObject 的双向绑定
    // RenderObject 已经在 CreateRenderObjectForElement/CreateRenderObjectForText 中设置了 node_
    // 这里设置 Node 的 render_object_ 指向
    node->SetRenderObject(render_obj);

    // 获取当前元素的样式
    const auto& style = render_obj->GetComputedStyle();

    // 处理 ::before 伪元素
    if (style.has_before && !style.content_before.empty()) {
        auto before_text = std::make_shared<RenderText>();
        before_text->SetText(style.content_before);

        // 继承父元素的样式
        ComputedStyle before_style;
        before_style.color = style.color;
        before_style.font_family = style.font_family;
        before_style.font_size = style.font_size;
        before_style.font_weight = style.font_weight;
        before_style.font_style = style.font_style;
        before_style.line_height = style.line_height;
        before_style.text_align = style.text_align;
        before_style.text_decoration = style.text_decoration;
        before_style.vertical_align = style.vertical_align;
        before_text->SetComputedStyle(before_style);

        render_obj->AppendChild(before_text);
    }

    // 递归构建子树，处理 display: contents 元素
    BuildChildRenderObjects(node, render_obj, &render_obj->GetComputedStyle());

    // 处理 ::after 伪元素
    if (style.has_after && !style.content_after.empty()) {
        auto after_text = std::make_shared<RenderText>();
        after_text->SetText(style.content_after);

        // 继承父元素的样式
        ComputedStyle after_style;
        after_style.color = style.color;
        after_style.font_family = style.font_family;
        after_style.font_size = style.font_size;
        after_style.font_weight = style.font_weight;
        after_style.font_style = style.font_style;
        after_style.line_height = style.line_height;
        after_style.text_align = style.text_align;
        after_style.text_decoration = style.text_decoration;
        after_style.vertical_align = style.vertical_align;
        after_text->SetComputedStyle(after_style);

        render_obj->AppendChild(after_text);
    }

    return render_obj;
}

void RenderTreeBuilder::BuildChildRenderObjects(
    std::shared_ptr<Node> node,
    std::shared_ptr<RenderObject> parent_render_obj,
    const ComputedStyle* parent_style) {
    
    for (const auto& child : node->GetChildNodes()) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto child_element = std::static_pointer_cast<Element>(child);
            
            // 计算子元素样式
            auto child_style = style_resolver_.ResolveStyle(child_element, parent_style);
            
            // 如果子元素是 display: contents，递归处理其子元素
            if (child_style.display == RenderObjectType::CONTENTS) {
                // 递归处理 contents 元素的子元素，使用 contents 元素的样式作为继承基础
                BuildChildRenderObjects(child, parent_render_obj, &child_style);
                continue;
            }
        }

        // 正常构建子元素的渲染对象
        // 关键：对子树继续构建时，必须把“当前父元素”的计算样式传下去，
        // 而不是继续沿用更上一层的 parent_style，否则 html 上的继承值（如 :root 变量）
        // 无法传递给 body 及后代。
        auto child_render_obj = BuildRenderTree(child, parent_render_obj ? &parent_render_obj->GetComputedStyle() : parent_style);
        if (child_render_obj) {
            parent_render_obj->AppendChild(child_render_obj);
        }
    }
}

std::shared_ptr<RenderObject> RenderTreeBuilder::CreateRenderObjectForElement(
    std::shared_ptr<Element> element,
    const ComputedStyle* parent_style) {

    // 计算样式
    auto style = style_resolver_.ResolveStyle(element, parent_style);

    std::shared_ptr<RenderObject> render_obj;

    // ========== 处理 SVG 元素 ==========
    std::string tag_name = element->GetTagName();

    // Debug: 输出元素标签名
    if (tag_name == "svg" || tag_name == "circle" || tag_name == "rect" ||
        tag_name == "ellipse" || tag_name == "line" || tag_name == "path" ||
        tag_name == "polyline" || tag_name == "polygon" || tag_name == "g") {
    }

    if (tag_name == "svg") {
        auto svg_element = std::dynamic_pointer_cast<SVGSVGElement>(element);
        if (svg_element) {
            auto svg_root = std::make_shared<RenderSVGRoot>();
            svg_root->SetSVGSVGElement(svg_element);
            render_obj = svg_root;
        }
    } else if (tag_name == "circle") {
        auto circle_element = std::dynamic_pointer_cast<SVGCircleElement>(element);
        if (circle_element) {
            auto svg_circle = std::make_shared<RenderSVGCircle>();
            svg_circle->SetSVGCircleElement(circle_element);
            render_obj = svg_circle;
        }
    } else if (tag_name == "rect") {
        auto rect_element = std::dynamic_pointer_cast<SVGRectElement>(element);
        if (rect_element) {
            auto svg_rect = std::make_shared<RenderSVGRect>();
            svg_rect->SetSVGRectElement(rect_element);
            render_obj = svg_rect;
        }
    } else if (tag_name == "ellipse") {
        auto ellipse_element = std::dynamic_pointer_cast<SVGEllipseElement>(element);
        if (ellipse_element) {
            auto svg_ellipse = std::make_shared<RenderSVGEllipse>();
            svg_ellipse->SetSVGEllipseElement(ellipse_element);
            render_obj = svg_ellipse;
        }
    } else if (tag_name == "line") {
        auto line_element = std::dynamic_pointer_cast<SVGLineElement>(element);
        if (line_element) {
            auto svg_line = std::make_shared<RenderSVGLine>();
            svg_line->SetSVGLineElement(line_element);
            render_obj = svg_line;
        }
    } else if (tag_name == "polyline") {
        auto polyline_element = std::dynamic_pointer_cast<SVGPolylineElement>(element);
        if (polyline_element) {
            auto svg_polyline = std::make_shared<RenderSVGPolyline>();
            svg_polyline->SetSVGPolylineElement(polyline_element);
            render_obj = svg_polyline;
        }
    } else if (tag_name == "polygon") {
        auto polygon_element = std::dynamic_pointer_cast<SVGPolygonElement>(element);
        if (polygon_element) {
            auto svg_polygon = std::make_shared<RenderSVGPolygon>();
            svg_polygon->SetSVGPolygonElement(polygon_element);
            render_obj = svg_polygon;
        }
    } else if (tag_name == "path") {
        auto path_element = std::dynamic_pointer_cast<SVGPathElement>(element);
        if (path_element) {
            auto svg_path = std::make_shared<RenderSVGPath>();
            svg_path->SetSVGPathElement(path_element);
            render_obj = svg_path;
        }
    } else if (tag_name == "g") {
        auto g_element = std::dynamic_pointer_cast<SVGGElement>(element);
        if (g_element) {
            auto svg_group = std::make_shared<RenderSVGGroup>();
            svg_group->SetSVGGElement(g_element);
            render_obj = svg_group;
        }
    } else if (tag_name == "text") {
        // 检查是否是SVG text元素（父元素是svg或g）
        auto text_element = std::dynamic_pointer_cast<SVGTextElement>(element);
        if (text_element) {
            auto svg_text = std::make_shared<RenderSVGText>();
            svg_text->SetSVGTextElement(text_element);
            render_obj = svg_text;
        }
    }

    // 如果不是SVG元素，使用默认方式创建渲染对象
    if (!render_obj) {
        render_obj = CreateRenderObjectByType(style.display);
    }

    if (render_obj) {
        render_obj->SetNode(element);
        render_obj->SetComputedStyle(style);

        // 处理表格单元格的 colspan/rowspan
        if (style.display == RenderObjectType::TABLE_CELL) {
            auto table_cell = std::dynamic_pointer_cast<RenderTableCell>(render_obj);
            if (table_cell) {
                // 读取 colspan 属性
                std::string colspan_str = element->GetAttribute("colspan");
                if (!colspan_str.empty()) {
                    try {
                        int colspan = std::stoi(colspan_str);
                        if (colspan > 0) {
                            table_cell->SetColSpan(colspan);
                        }
                    } catch (...) {}
                }
                // 读取 rowspan 属性
                std::string rowspan_str = element->GetAttribute("rowspan");
                if (!rowspan_str.empty()) {
                    try {
                        int rowspan = std::stoi(rowspan_str);
                        if (rowspan >= 0) {
                            table_cell->SetRowSpan(rowspan);
                        }
                    } catch (...) {}
                }
            }
        }
    }

    return render_obj;
}

std::shared_ptr<RenderObject> RenderTreeBuilder::CreateRenderObjectForText(
    std::shared_ptr<Text> text,
    const ComputedStyle* parent_style) {

    // 如果父元素是 display: none，文本节点也不创建
    if (parent_style && parent_style->display == RenderObjectType::NONE) {
        return nullptr;
    }

    std::string text_data = text->GetData();

    // 检查父元素的 white-space 属性，决定是否保留换行符
    bool preserve_newlines = false;
    if (parent_style) {
        const std::string& ws = parent_style->white_space;
        preserve_newlines = (ws == "pre" || ws == "pre-wrap" || ws == "pre-line");
    }

    auto is_inline_level_display = [](RenderObjectType display) {
        return display == RenderObjectType::INLINE ||
               display == RenderObjectType::INLINE_BLOCK ||
               display == RenderObjectType::INLINE_FLEX ||
               display == RenderObjectType::INLINE_GRID;
    };

    auto sibling_preserves_inter_word_space = [&](const std::shared_ptr<Node>& sibling) {
        if (!sibling) {
            return false;
        }

        if (sibling->GetNodeType() == NodeType::TEXT_NODE) {
            auto sibling_text = std::dynamic_pointer_cast<Text>(sibling);
            if (!sibling_text) {
                return false;
            }
            const std::string& sibling_data = sibling_text->GetData();
            return sibling_data.find_first_not_of(" \t\n\r") != std::string::npos;
        }

        if (sibling->GetNodeType() != NodeType::ELEMENT_NODE) {
            return false;
        }

        auto sibling_element = std::dynamic_pointer_cast<Element>(sibling);
        if (!sibling_element) {
            return false;
        }

        auto sibling_style = style_resolver_.ResolveStyle(sibling_element, parent_style);
        return is_inline_level_display(sibling_style.display);
    };

    // 检查是否是纯空白文本节点
    bool is_whitespace_only = (text_data.find_first_not_of(" \t\n\r") == std::string::npos);

    // 对于纯空白文本节点：
    // - 如果 white-space: pre，保留
    // - 如果它只位于块级内容边界/块级兄弟之间，则不创建 RenderText
    // - 只有在确实承担 inline 间距时，才折叠为单个空格保留
    if (!preserve_newlines && is_whitespace_only) {
        // 空文本直接跳过
        if (text_data.empty()) {
            return nullptr;
        }

        // CSS Flex/Grid 规范：纯空白文本节点不渲染
        if (parent_style && (parent_style->display == RenderObjectType::FLEX ||
                             parent_style->display == RenderObjectType::INLINE_FLEX ||
                             parent_style->display == RenderObjectType::GRID ||
                             parent_style->display == RenderObjectType::INLINE_GRID)) {
            return nullptr;
        }

        // 对块级容器，如果空白并没有夹在 inline 内容之间，就不应生成可见文本行。
        if (parent_style && !is_inline_level_display(parent_style->display)) {
            bool prev_preserves = sibling_preserves_inter_word_space(text->GetPreviousSibling());
            bool next_preserves = sibling_preserves_inter_word_space(text->GetNextSibling());
            if (!(prev_preserves && next_preserves)) {
                return nullptr;
            }
        }

        // 纯空白文本节点折叠为单个空格，用于 inline 元素之间的间距
        // 不跳过，让 IFC 布局来处理
    }

    std::string final_text;
    if (preserve_newlines) {
        // 保留换行符，但根据 white-space 的不同处理空格/制表符
        // pre: 保留所有空白字符
        // pre-wrap: 保留换行，但合并空格
        // pre-line: 保留换行，合并空格
        const std::string& ws = parent_style ? parent_style->white_space : "normal";
        if (ws == "pre") {
            // 完全保留原始文本
            final_text = text_data;
        } else {
            // pre-wrap 或 pre-line: 保留换行，合并连续空格
            bool in_space = false;
            for (char c : text_data) {
                if (c == '\n') {
                    final_text += c;
                    in_space = false;
                } else if (c == ' ' || c == '\t' || c == '\r') {
                    if (!in_space) {
                        final_text += ' ';
                        in_space = true;
                    }
                } else {
                    final_text += c;
                    in_space = false;
                }
            }
        }
    } else {
        // 规范化空白字符：将连续的空白字符（包括换行）替换为单个空格
        bool in_whitespace = false;
        for (char c : text_data) {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                if (!in_whitespace) {
                    final_text += ' ';
                    in_whitespace = true;
                }
            } else {
                final_text += c;
                in_whitespace = false;
            }
        }
    }

    auto render_obj = std::make_shared<RenderText>();
    render_obj->SetNode(text);
    render_obj->SetText(final_text);

    // 文本节点只继承可继承的样式属性，不继承定位属性
    ComputedStyle text_style;
    if (parent_style) {
        // 只继承可继承属性
        text_style.color = parent_style->color;
        text_style.font_family = parent_style->font_family;
        text_style.font_size = parent_style->font_size;
        text_style.font_weight = parent_style->font_weight;
        text_style.font_style = parent_style->font_style;
        text_style.line_height = parent_style->line_height;
        text_style.text_align = parent_style->text_align;
        text_style.text_decoration = parent_style->text_decoration;
        text_style.vertical_align = parent_style->vertical_align;
        // 继承文本间距相关属性
        text_style.letter_spacing = parent_style->letter_spacing;
        text_style.word_spacing = parent_style->word_spacing;
        text_style.text_indent = parent_style->text_indent;
        text_style.white_space = parent_style->white_space;
        // 继承 CSS 变量
        text_style.css_variables.InheritFrom(&parent_style->css_variables);
    }
    render_obj->SetComputedStyle(text_style);

    return render_obj;
}

std::shared_ptr<RenderObject> RenderTreeBuilder::CreateRenderObjectByType(RenderObjectType type) {
    switch (type) {
        case RenderObjectType::BLOCK:
            return std::make_shared<RenderBlock>();
        case RenderObjectType::INLINE:
            return std::make_shared<RenderInline>();
        case RenderObjectType::TEXT:
            return std::make_shared<RenderText>();
        case RenderObjectType::INLINE_BLOCK:
            return std::make_shared<RenderInlineBlock>(); // 使用真正的InlineBlock
        case RenderObjectType::FLEX:
            return std::make_shared<RenderFlex>(); // 🎯 关键修复：使用专门的 RenderFlex 类
        case RenderObjectType::INLINE_FLEX:
            return std::make_shared<RenderInlineFlex>(); // inline-flex 使用独立的 RenderInlineFlex 类
        case RenderObjectType::GRID:
            return std::make_shared<RenderBlock>(); // 简化：暂时用 Block
        case RenderObjectType::INLINE_GRID:
            return std::make_shared<RenderBlock>(); // inline-grid 暂时用 Block，未来可创建 RenderInlineGrid
        // 表格相关类型
        case RenderObjectType::TABLE:
            return std::make_shared<RenderTable>();
        case RenderObjectType::TABLE_ROW_GROUP:
            return std::make_shared<RenderTableRowGroup>(RenderObjectType::TABLE_ROW_GROUP);
        case RenderObjectType::TABLE_HEADER_GROUP:
            return std::make_shared<RenderTableRowGroup>(RenderObjectType::TABLE_HEADER_GROUP);
        case RenderObjectType::TABLE_FOOTER_GROUP:
            return std::make_shared<RenderTableRowGroup>(RenderObjectType::TABLE_FOOTER_GROUP);
        case RenderObjectType::TABLE_ROW:
            return std::make_shared<RenderTableRow>();
        case RenderObjectType::TABLE_CELL:
            return std::make_shared<RenderTableCell>();
        case RenderObjectType::TABLE_CAPTION:
            return std::make_shared<RenderTableCaption>();
        case RenderObjectType::CONTENTS:
            return nullptr;  // display: contents 不创建渲染对象
        case RenderObjectType::NONE:
            return nullptr;
        default:
            return std::make_shared<RenderBlock>();
    }
}

// ========== Phase 3: 增量渲染树更新公有接口 ==========

std::shared_ptr<RenderObject> RenderTreeBuilder::CreateRenderObjectForElement(Element* element) {
    if (!element) {
        return nullptr;
    }

    // 确保 StyleManager 已设置（用于增量更新时的样式计算）
    if (document_ && document_->GetStyleManager()) {
        style_resolver_.SetStyleManager(document_->GetStyleManager());
    }

    // 获取父元素的样式作为继承基础
    // 注意：display: contents 元素虽然不生成渲染对象，但其样式仍应被子元素继承
    // 需要向上查找有渲染对象的祖先，或者使用 display: contents 元素的计算样式
    const ComputedStyle* parent_style = nullptr;
    if (auto parent_node = element->GetParentNode()) {
        auto parent_elem = std::dynamic_pointer_cast<Element>(parent_node);
        while (parent_elem) {
            if (auto parent_ro = parent_elem->GetRenderObject()) {
                parent_style = &parent_ro->GetComputedStyle();
                break;
            }
            // 继续向上查找
            auto grandparent = parent_elem->GetParentNode();
            parent_elem = std::dynamic_pointer_cast<Element>(grandparent);
        }
    }

    // 创建 shared_ptr 包装（临时，仅用于调用内部方法）
    // 注意：这里假设 element 已经被某个 shared_ptr 管理
    auto element_shared = std::dynamic_pointer_cast<Element>(element->shared_from_this());
    if (!element_shared) {
        return nullptr;
    }

    auto render_obj = CreateRenderObjectForElement(element_shared, parent_style);

    // 检查 display: none
    if (render_obj && render_obj->GetComputedStyle().display == RenderObjectType::NONE) {
        return nullptr;
    }

    return render_obj;
}

std::shared_ptr<RenderObject> RenderTreeBuilder::CreateRenderObjectForText(Text* text) {
    if (!text) {
        return nullptr;
    }

    // 确保 StyleManager 已设置（用于增量更新时的样式计算）
    if (document_ && document_->GetStyleManager()) {
        style_resolver_.SetStyleManager(document_->GetStyleManager());
    }

    // 获取父元素的样式作为继承基础
    const ComputedStyle* parent_style = nullptr;
    if (auto parent_node = text->GetParentNode()) {
        auto parent_elem = std::dynamic_pointer_cast<Element>(parent_node);
        if (parent_elem) {
            if (auto parent_ro = parent_elem->GetRenderObject()) {
                parent_style = &parent_ro->GetComputedStyle();
            }
        }
    }

    // 创建 shared_ptr 包装
    auto text_shared = std::dynamic_pointer_cast<Text>(text->shared_from_this());
    if (!text_shared) {
        return nullptr;
    }

    return CreateRenderObjectForText(text_shared, parent_style);
}

} // namespace mbink

