/**
 * @file style_resolver.cpp
 * @brief 样式解析器实现
 */

#include "style_resolver.h"
#include "render_inline_block.h"
#include "core/dom/text.h"
#include "core/dom/document.h"
#include "core/lexbor/style_manager.h"
#include "color.h"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace lightui {

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
        "opacity"
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

    // 4. CSS 规则（<style> 标签和外部样式表）
    ApplyCSSRules(style, element);

    // 5. 伪类样式（如 :hover, :active, :focus）
    ApplyPseudoClassStyles(style, element);

    // 6. 内联样式（最高优先级）- 覆盖所有
    ApplyInlineStyle(style, element);

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
    // 只在根元素时设置基础默认值
    if (is_root) {
        style.color = "#000000";
        style.font_family = "Arial";
        style.font_size = 16.0f;
        style.font_weight = "normal";
        style.font_style = "normal";
        style.text_align = "left";
        style.text_decoration = "none";
        style.line_height = 1.2f;
        style.opacity = 1.0f;
    }

    // 设置 display 属性（不可继承）
    style.display = RenderObjectType::BLOCK;

    if (tag_name == "div" || tag_name == "p" || tag_name == "section" ||
        tag_name == "article" || tag_name == "header" || tag_name == "footer" ||
        tag_name == "h1" || tag_name == "h2" || tag_name == "h3" ||
        tag_name == "h4" || tag_name == "h5" || tag_name == "h6") {
        style.display = RenderObjectType::BLOCK;
    }
    else if (tag_name == "span" || tag_name == "a" || tag_name == "strong" ||
             tag_name == "em" || tag_name == "b" || tag_name == "i" ||
             tag_name == "u" || tag_name == "s" || tag_name == "strike" || tag_name == "del" ||
             tag_name == "mark" || tag_name == "small" || tag_name == "big" ||
             tag_name == "sub" || tag_name == "sup" || tag_name == "code" ||
             tag_name == "kbd" || tag_name == "samp" || tag_name == "var" ||
             tag_name == "abbr" || tag_name == "cite" || tag_name == "dfn" ||
             tag_name == "q" || tag_name == "time") {
        style.display = RenderObjectType::INLINE;
    }
    else if (tag_name == "img") {
        style.display = RenderObjectType::INLINE_BLOCK;
    }
    // 按钮使用 INLINE（RenderInline 已经有正确的文本渲染）
    // 输入框使用 INLINE_BLOCK（需要特殊的表单控件渲染）
    else if (tag_name == "button") {
        style.display = RenderObjectType::INLINE;
    }
    else if (tag_name == "input") {
        style.display = RenderObjectType::INLINE_BLOCK;
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
}

void StyleResolver::ApplyElementSpecificStyle(ComputedStyle& style, const std::string& tag_name, std::shared_ptr<Element> element) {
    // ========== 块级元素 ==========

    // HTML, BODY
    if (tag_name == "html" || tag_name == "body") {
        style.margin.top = CSSLength(0, CSSUnit::PX);
        style.margin.bottom = CSSLength(0, CSSUnit::PX);
        style.margin.left = CSSLength(0, CSSUnit::PX);
        style.margin.right = CSSLength(0, CSSUnit::PX);
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
    if (tag_name == "blockquote") {
        style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        style.margin.left = CSSLength(40, CSSUnit::PX);
        style.margin.right = CSSLength(40, CSSUnit::PX);

        // TODO: 当前 CSSBorder 不支持单独设置左边框，需要扩展为四个方向的边框
        // 临时方案：增加左内边距来模拟左边框效果
        style.padding.left = CSSLength(20, CSSUnit::PX);
        style.background_color = "#F5F5F5";  // 浅灰色背景以区分引用块
    }

    // 预格式化文本 (Preformatted)
    if (tag_name == "pre") {
        style.font_family = "Consolas, Monaco, Courier New, monospace";
        style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        // TODO: 添加 white-space: pre 支持
    }

    // 代码 (Code)
    if (tag_name == "code") {
        style.font_family = "Consolas, Monaco, Courier New, monospace";
    }

    // 水平线 (Horizontal Rule)
    if (tag_name == "hr") {
        style.margin.top = CSSLength(16, CSSUnit::PX);
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        style.height = CSSLength(1, CSSUnit::PX);
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = SkColorSetRGB(200, 200, 200);
    }

    // ========== 列表 (Lists) ==========

    if (tag_name == "ul" || tag_name == "ol") {
        style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        style.padding.left = CSSLength(40, CSSUnit::PX);  // 左侧缩进
    }

    if (tag_name == "li") {
        style.display = RenderObjectType::BLOCK;  // 列表项是块级
    }

    if (tag_name == "dl") {  // Definition List
        style.margin.top = CSSLength(16, CSSUnit::PX);
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
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
        style.margin.left = CSSLength(2, CSSUnit::PX);
        style.margin.right = CSSLength(2, CSSUnit::PX);
        style.padding.top = CSSLength(10, CSSUnit::PX);
        style.padding.bottom = CSSLength(10, CSSUnit::PX);
        style.padding.left = CSSLength(10, CSSUnit::PX);
        style.padding.right = CSSLength(10, CSSUnit::PX);
        style.border.width = CSSLength(2, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = Color::FromRGB(192, 192, 192);
    }

    if (tag_name == "legend") {
        style.padding.left = CSSLength(2, CSSUnit::PX);
        style.padding.right = CSSLength(2, CSSUnit::PX);
    }

    if (tag_name == "button" || tag_name == "input" || tag_name == "select" || tag_name == "textarea") {
        style.padding.top = CSSLength(2, CSSUnit::PX);
        style.padding.bottom = CSSLength(2, CSSUnit::PX);
        style.padding.left = CSSLength(6, CSSUnit::PX);
        style.padding.right = CSSLength(6, CSSUnit::PX);
        style.border.width = CSSLength(2, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = Color::FromRGB(169, 169, 169);
    }

    if (tag_name == "button") {
        // 背景色
        style.background_color = "#F0F0F0";

        // 圆角（增强到 4px）
        style.border_radius.top_left = CSSLength(4, CSSUnit::PX);
        style.border_radius.top_right = CSSLength(4, CSSUnit::PX);
        style.border_radius.bottom_left = CSSLength(4, CSSUnit::PX);
        style.border_radius.bottom_right = CSSLength(4, CSSUnit::PX);

        // 边框
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = SkColorSetRGB(200, 200, 200);

        // 内边距 - 使用浏览器默认值
        style.padding.left = CSSLength(6, CSSUnit::PX);
        style.padding.right = CSSLength(6, CSSUnit::PX);
        style.padding.top = CSSLength(1, CSSUnit::PX);
        style.padding.bottom = CSSLength(1, CSSUnit::PX);

        // 不设置固定宽高，让按钮根据内容自动计算尺寸
        // width 和 height 保持默认的 AUTO

        // 文本居中
        style.text_align = "center";
    }

    // Input 元素
    if (tag_name == "input" && element) {
        std::string type = element->GetAttribute("type");

        if (type == "text" || type == "password" || type.empty()) {
            // 文本输入框
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

            // 边框
            style.border.width = CSSLength(1, CSSUnit::PX);
            style.border.style = CSSBorderStyle::SOLID;
            style.border.color = SkColorSetRGB(200, 200, 200);

            // 内边距（浏览器默认较小，会被 inline style 覆盖）
            style.padding.left = CSSLength(2, CSSUnit::PX);
            style.padding.right = CSSLength(2, CSSUnit::PX);
            style.padding.top = CSSLength(1, CSSUnit::PX);
            style.padding.bottom = CSSLength(1, CSSUnit::PX);
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
            // 复选框：小方块
            style.width = CSSLength(16, CSSUnit::PX);
            style.height = CSSLength(16, CSSUnit::PX);

            style.border.width = CSSLength(1, CSSUnit::PX);
            style.border.style = CSSBorderStyle::SOLID;
            style.border.color = SkColorSetRGB(150, 150, 150);

            style.border_radius.top_left = CSSLength(2, CSSUnit::PX);
            style.border_radius.top_right = CSSLength(2, CSSUnit::PX);
            style.border_radius.bottom_left = CSSLength(2, CSSUnit::PX);
            style.border_radius.bottom_right = CSSLength(2, CSSUnit::PX);

            style.background_color = "#FFFFFF";
        }
        else if (type == "radio") {
            // 单选按钮：小圆圈
            style.width = CSSLength(16, CSSUnit::PX);
            style.height = CSSLength(16, CSSUnit::PX);

            style.border.width = CSSLength(1, CSSUnit::PX);
            style.border.style = CSSBorderStyle::SOLID;
            style.border.color = SkColorSetRGB(150, 150, 150);

            // 圆形
            style.border_radius.top_left = CSSLength(8, CSSUnit::PX);
            style.border_radius.top_right = CSSLength(8, CSSUnit::PX);
            style.border_radius.bottom_left = CSSLength(8, CSSUnit::PX);
            style.border_radius.bottom_right = CSSLength(8, CSSUnit::PX);

            style.background_color = "#FFFFFF";
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

        // 边框
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = SkColorSetRGB(200, 200, 200);

        // 内边距
        style.padding.left = CSSLength(8, CSSUnit::PX);
        style.padding.right = CSSLength(8, CSSUnit::PX);
        style.padding.top = CSSLength(6, CSSUnit::PX);
        style.padding.bottom = CSSLength(6, CSSUnit::PX);
    }

    // Select 元素
    if (tag_name == "select") {
        // Display类型: inline-block (符合CSS标准)
        style.display = RenderObjectType::INLINE_BLOCK;

        style.background_color = "#FFFFFF";

        // 宽度: 根据内容自动计算 (shrink-to-fit)
        // 这里不设置固定宽度，让布局引擎根据内容计算
        // 如果用户在HTML中设置了style="width: xxx"，会被覆盖

        // 高度: 单行选择框的标准高度
        style.height = CSSLength(32, CSSUnit::PX);

        // 圆角
        style.border_radius.top_left = CSSLength(3, CSSUnit::PX);
        style.border_radius.top_right = CSSLength(3, CSSUnit::PX);
        style.border_radius.bottom_left = CSSLength(3, CSSUnit::PX);
        style.border_radius.bottom_right = CSSLength(3, CSSUnit::PX);

        // 边框
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = SkColorSetRGB(200, 200, 200);

        // 内边距 (为下拉箭头留出空间)
        style.padding.left = CSSLength(8, CSSUnit::PX);
        style.padding.right = CSSLength(24, CSSUnit::PX);  // 右侧留空间给箭头
        style.padding.top = CSSLength(6, CSSUnit::PX);
        style.padding.bottom = CSSLength(6, CSSUnit::PX);
    }

    // Option 元素（隐藏，只在select内部显示选中的）
    if (tag_name == "option") {
        // Option元素默认隐藏，由Select元素负责渲染选中的option
        style.display = RenderObjectType::NONE;
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

    // 上标和下标 (Superscript & Subscript)
    if (tag_name == "sup" || tag_name == "sub") {
        style.font_size = 12.0f;  // 0.75em
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
}

void StyleResolver::ApplyInlineStyle(ComputedStyle& style, std::shared_ptr<Element> element) {
    // 获取style属性
    std::string style_attr = element->GetAttribute("style");

    if (style_attr.empty()) {
        return;
    }

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

        // 应用样式属性
        if (!property.empty() && !value.empty()) {
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
}

void StyleResolver::ParseStyleProperty(ComputedStyle& style,
                                       const std::string& property,
                                       const std::string& value) {
    // 1. 检查是否为 CSS 自定义属性（--custom-property）
    if (IsCustomProperty(property)) {
        style.css_variables.SetVariable(property, value);
        return;
    }

    // 2. 解析 var() 函数（如果值包含 var()）
    std::string resolved_value = value;
    if (CSSVarResolver::ContainsVar(value)) {
        resolved_value = CSSVarResolver::ResolveVar(value, style.css_variables);
    }

    // 3. 解析标准 CSS 属性
    if (property == "display") {
        style.display = ParseDisplay(resolved_value);
    }
    else if (property == "width") {
        style.width = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "height") {
        style.height = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "min-width") {
        style.min_width = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "max-width") {
        style.max_width = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "min-height") {
        style.min_height = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "max-height") {
        style.max_height = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "margin") {
        style.margin = CSSValue::ParseEdges(resolved_value);
        // 同步到单独的字段
        style.margin_top = style.margin.top;
        style.margin_right = style.margin.right;
        style.margin_bottom = style.margin.bottom;
        style.margin_left = style.margin.left;
    }
    else if (property == "margin-top") {
        style.margin.top = CSSValue::ParseLength(resolved_value);
        style.margin_top = style.margin.top;
    }
    else if (property == "margin-right") {
        style.margin.right = CSSValue::ParseLength(resolved_value);
        style.margin_right = style.margin.right;
    }
    else if (property == "margin-bottom") {
        style.margin.bottom = CSSValue::ParseLength(resolved_value);
        style.margin_bottom = style.margin.bottom;
    }
    else if (property == "margin-left") {
        style.margin.left = CSSValue::ParseLength(resolved_value);
        style.margin_left = style.margin.left;
    }
    else if (property == "padding") {
        style.padding = CSSValue::ParseEdges(resolved_value);
        // 同步到单独的字段
        style.padding_top = style.padding.top;
        style.padding_right = style.padding.right;
        style.padding_bottom = style.padding.bottom;
        style.padding_left = style.padding.left;
    }
    else if (property == "padding-top") {
        style.padding.top = CSSValue::ParseLength(resolved_value);
        style.padding_top = style.padding.top;
    }
    else if (property == "padding-right") {
        style.padding.right = CSSValue::ParseLength(resolved_value);
        style.padding_right = style.padding.right;
    }
    else if (property == "padding-bottom") {
        style.padding.bottom = CSSValue::ParseLength(resolved_value);
        style.padding_bottom = style.padding.bottom;
    }
    else if (property == "padding-left") {
        style.padding.left = CSSValue::ParseLength(resolved_value);
        style.padding_left = style.padding.left;
    }
    // border 简写属性：border: [width] [style] [color]
    else if (property == "border") {
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        // 设置所有边
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
    }
    else if (property == "border-width") {
        style.border.width = CSSValue::ParseLength(resolved_value);
        // 同步到单独的字段
        float width = style.border.width.ToPx(0, style.font_size);
        style.border_top_width = width;
        style.border_right_width = width;
        style.border_bottom_width = width;
        style.border_left_width = width;
    }
    else if (property == "border-top-width") {
        auto length = CSSValue::ParseLength(resolved_value);
        style.border_top_width = length.ToPx(0, style.font_size);
    }
    else if (property == "border-right-width") {
        auto length = CSSValue::ParseLength(resolved_value);
        style.border_right_width = length.ToPx(0, style.font_size);
    }
    else if (property == "border-bottom-width") {
        auto length = CSSValue::ParseLength(resolved_value);
        style.border_bottom_width = length.ToPx(0, style.font_size);
    }
    else if (property == "border-left-width") {
        auto length = CSSValue::ParseLength(resolved_value);
        style.border_left_width = length.ToPx(0, style.font_size);
    }
    else if (property == "border-style") {
        style.border.style = CSSValue::ParseBorderStyle(resolved_value);
        // 同步到所有边
        style.border_top_style = style.border.style;
        style.border_right_style = style.border.style;
        style.border_bottom_style = style.border.style;
        style.border_left_style = style.border.style;
    }
    else if (property == "border-color") {
        style.border.color = CSSValue::ParseColor(resolved_value);
        // 同步到所有边
        style.border_top_color = style.border.color;
        style.border_right_color = style.border.color;
        style.border_bottom_color = style.border.color;
        style.border_left_color = style.border.color;
    }
    // 单边边框简写属性: border-left, border-right, border-top, border-bottom
    else if (property == "border-left") {
        // 解析 "width style color" 格式，如 "4px solid #4CAF50"
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        style.border_left_width = border.width.ToPx(0, style.font_size);
        style.border_left_style = border.style;
        style.border_left_color = border.color;
    }
    else if (property == "border-right") {
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        style.border_right_width = border.width.ToPx(0, style.font_size);
        style.border_right_style = border.style;
        style.border_right_color = border.color;
    }
    else if (property == "border-top") {
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        style.border_top_width = border.width.ToPx(0, style.font_size);
        style.border_top_style = border.style;
        style.border_top_color = border.color;
    }
    else if (property == "border-bottom") {
        auto border = ParseBorderShorthand(resolved_value, style.font_size);
        style.border_bottom_width = border.width.ToPx(0, style.font_size);
        style.border_bottom_style = border.style;
        style.border_bottom_color = border.color;
    }
    // 单边边框样式
    else if (property == "border-left-style") {
        style.border_left_style = CSSValue::ParseBorderStyle(resolved_value);
    }
    else if (property == "border-right-style") {
        style.border_right_style = CSSValue::ParseBorderStyle(resolved_value);
    }
    else if (property == "border-top-style") {
        style.border_top_style = CSSValue::ParseBorderStyle(resolved_value);
    }
    else if (property == "border-bottom-style") {
        style.border_bottom_style = CSSValue::ParseBorderStyle(resolved_value);
    }
    // 单边边框颜色
    else if (property == "border-left-color") {
        style.border_left_color = CSSValue::ParseColor(resolved_value);
    }
    else if (property == "border-right-color") {
        style.border_right_color = CSSValue::ParseColor(resolved_value);
    }
    else if (property == "border-top-color") {
        style.border_top_color = CSSValue::ParseColor(resolved_value);
    }
    else if (property == "border-bottom-color") {
        style.border_bottom_color = CSSValue::ParseColor(resolved_value);
    }
    else if (property == "border-radius") {
        style.border_radius = CSSValue::ParseBorderRadius(resolved_value);
    }
    // 表格边框属性
    else if (property == "border-collapse") {
        // CSS 标准值: collapse, separate
        if (resolved_value == "collapse" || resolved_value == "separate") {
            style.border_collapse = resolved_value;
        }
    }
    else if (property == "border-spacing") {
        style.border_spacing = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "background") {
        // 简化处理：如果是颜色值，设置 background-color
        // 完整的 background 解析应该支持 image, position, size, repeat 等
        style.background_color = resolved_value;
    }
    else if (property == "background-color") {
        style.background_color = resolved_value;
    }
    else if (property == "background-repeat") {
        style.background_repeat = CSSValue::ParseBackgroundRepeat(resolved_value);
    }
    else if (property == "background-size") {
        style.background_size = CSSValue::ParseBackgroundSize(resolved_value);
    }
    else if (property == "color") {
        style.color = resolved_value;
    }
    else if (property == "font-family") {
        style.font_family = resolved_value;
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
    else if (property == "line-height") {
        // line-height 可以是：
        // 1. 无单位数字（如 "1.6"）- 表示 font-size 的倍数
        // 2. 带单位的长度（如 "24px", "1.5em"）- 转换为 font-size 的倍数
        // 3. 百分比（如 "150%"）- 表示 font-size 的百分比
        std::string trimmed = resolved_value;
        // 去除首尾空格
        size_t start = trimmed.find_first_not_of(" \t");
        size_t end = trimmed.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            trimmed = trimmed.substr(start, end - start + 1);
        }

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
            } catch (...) {
                style.line_height = 1.2f; // 默认值
            }
        } else {
            // 带单位的值，解析并转换为倍数
            auto length = CSSValue::ParseLength(resolved_value);
            if (length.unit == CSSUnit::PERCENT) {
                style.line_height = length.value / 100.0f;
            } else {
                style.line_height = length.ToPx(style.font_size, style.font_size) / style.font_size;
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
    else if (property == "transition") {
        style.transitions = CSSTransition::Parse(resolved_value);
    }
    else if (property == "filter") {
        style.filter = CSSFilterParser::Parse(resolved_value);
    }
    else if (property == "backdrop-filter") {
        style.backdrop_filter = CSSFilterParser::Parse(resolved_value);
    }
    // Flexbox 属性
    else if (property == "flex-direction") {
        style.flex_direction = resolved_value;
    }
    else if (property == "flex-wrap") {
        style.flex_wrap = resolved_value;
    }
    else if (property == "justify-content") {
        style.justify_content = resolved_value;
    }
    else if (property == "align-items") {
        style.align_items = resolved_value;
    }
    else if (property == "align-content") {
        style.align_content = resolved_value;
    }
    else if (property == "align-self") {
        style.align_self = resolved_value;
    }
    else if (property == "flex") {
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
    }
    else if (property == "flex-grow") {
        try {
            style.flex_grow = std::stof(resolved_value);
        } catch (...) {
            style.flex_grow = 0.0f;
        }
    }
    else if (property == "flex-shrink") {
        try {
            style.flex_shrink = std::stof(resolved_value);
        } catch (...) {
            style.flex_shrink = 1.0f;
        }
    }
    else if (property == "flex-basis") {
        style.flex_basis = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "order") {
        try {
            style.order = std::stoi(resolved_value);
        } catch (...) {
            style.order = 0;
        }
    }
    else if (property == "gap") {
        style.gap = CSSValue::ParseLength(resolved_value);
        style.row_gap = style.gap;
        style.column_gap = style.gap;
    }
    else if (property == "row-gap") {
        style.row_gap = CSSValue::ParseLength(resolved_value);
    }
    else if (property == "column-gap") {
        style.column_gap = CSSValue::ParseLength(resolved_value);
    }
    // Grid 属性
    else if (property == "grid-template-columns") {
        style.grid_template_columns = resolved_value;
    }
    else if (property == "grid-template-rows") {
        style.grid_template_rows = resolved_value;
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
}

void StyleResolver::ApplyCSSRules(ComputedStyle& style, std::shared_ptr<Element> element) {
    if (!style_manager_ || !element) {
        return;
    }

    // 从 StyleManager 获取匹配的 CSS 规则
    auto css_properties = style_manager_->ComputeStyle(element.get());

    // 应用每个 CSS 属性
    for (const auto& [property, value] : css_properties) {
        ParseStyleProperty(style, property, value);
    }
}

void StyleResolver::ApplyPseudoClassStyles(ComputedStyle& style, std::shared_ptr<Element> element) {
    if (!element) {
        return;
    }

    std::string tag_name = element->GetTagName();

    // ========== :hover 伪类样式 ==========
    if (element->HasPseudoClass("hover")) {
        if (tag_name == "button") {
            // 按钮悬停：背景色变深
            style.background_color = "#E0E0E0";

            // 边框颜色稍微变深
            style.border.color = SkColorSetRGB(180, 180, 180);

            // 增加阴影效果
            if (!style.box_shadow.empty()) {
                style.box_shadow[0].offset_y = 6;
                style.box_shadow[0].blur_radius = 12;
                style.box_shadow[0].color = SkColorSetARGB(100, 0, 0, 0);
            }
        }
        else if (tag_name == "a") {
            // 链接悬停：下划线
            style.text_decoration = "underline";
        }
    }

    // ========== :active 伪类样式 ==========
    if (element->HasPseudoClass("active")) {
        if (tag_name == "button") {
            // 按钮按下：背景色更深，阴影减小（按下效果）
            style.background_color = "#D0D0D0";

            // 边框颜色更深
            style.border.color = SkColorSetRGB(160, 160, 160);

            // 减小阴影（按下效果）
            if (!style.box_shadow.empty()) {
                style.box_shadow[0].offset_y = 2;
                style.box_shadow[0].blur_radius = 4;
                style.box_shadow[0].color = SkColorSetARGB(60, 0, 0, 0);
            }
        }
    }

    // ========== :focus 伪类样式 ==========
    // 使用 outline 显示焦点指示器（符合浏览器行为）
    // outline 不占用布局空间，不受内联样式中的 border 影响
    if (element->HasPseudoClass("focus")) {
        if (tag_name == "input" || tag_name == "textarea" || tag_name == "button" || tag_name == "select") {
            // 使用 outline 显示焦点，类似浏览器默认行为
            style.outline_width = CSSLength(2, CSSUnit::PX);
            style.outline_style = "solid";
            style.outline_color = SkColorSetRGB(0, 0, 0);  // 黑色轮廓
            style.outline_offset = CSSLength(1, CSSUnit::PX);  // 轮廓距离边框1px
        }
    }

    // ========== :focus-visible 伪类样式 ==========
    // 键盘导航时的焦点指示器（更明显的蓝色边框）
    // 这符合现代浏览器的行为：https://developer.mozilla.org/en-US/docs/Web/CSS/:focus-visible
    if (element->HasPseudoClass("focus-visible")) {
        if (tag_name == "button" || tag_name == "input" || tag_name == "textarea" || tag_name == "select") {
            // 焦点样式：蓝色边框和外发光（覆盖:focus的样式）
            style.border.width = CSSLength(2, CSSUnit::PX);
            style.border.color = SkColorSetRGB(66, 153, 225);  // 蓝色

            // 清除之前的阴影，添加蓝色外发光效果
            style.box_shadow.clear();
            CSSBoxShadow focus_shadow;
            focus_shadow.offset_x = 0;
            focus_shadow.offset_y = 0;
            focus_shadow.blur_radius = 4;
            focus_shadow.spread_radius = 0;
            focus_shadow.color = SkColorSetARGB(128, 66, 153, 225);  // 半透明蓝色
            focus_shadow.inset = false;

            style.box_shadow.push_back(focus_shadow);
        }
    }

    // ========== :disabled 伪类样式 ==========
    if (element->HasPseudoClass("disabled")) {
        // 禁用状态：灰色，半透明
        style.opacity = 0.6f;
        style.background_color = "#F5F5F5";
        style.color = "#999999";
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

bool StyleResolver::IsInheritableProperty(const std::string& property) {
    return inheritable_properties_.find(property) != inheritable_properties_.end();
}

RenderObjectType StyleResolver::ParseDisplay(const std::string& value) {
    if (value == "block") return RenderObjectType::BLOCK;
    if (value == "inline") return RenderObjectType::INLINE;
    if (value == "inline-block") return RenderObjectType::INLINE_BLOCK;
    if (value == "flex") return RenderObjectType::FLEX;
    if (value == "inline-flex") return RenderObjectType::FLEX;  // inline-flex 也使用 FLEX 类型
    if (value == "grid") return RenderObjectType::GRID;
    if (value == "inline-grid") return RenderObjectType::GRID;  // inline-grid 也使用 GRID 类型
    if (value == "none") return RenderObjectType::NONE;
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

    // 分割值，格式如 "4px solid #4CAF50"
    std::istringstream iss(value);
    std::vector<std::string> parts;
    std::string part;
    while (iss >> part) {
        parts.push_back(part);
    }

    for (const auto& p : parts) {
        // 尝试解析为长度值（宽度）
        if (p.find("px") != std::string::npos ||
            p.find("em") != std::string::npos ||
            p.find("rem") != std::string::npos ||
            (std::isdigit(p[0]) && p.find("px") == std::string::npos && p.find("em") == std::string::npos)) {
            border.width = CSSValue::ParseLength(p);
        }
        // 尝试解析为样式
        else if (p == "solid" || p == "dashed" || p == "dotted" || p == "double" || p == "none") {
            border.style = CSSValue::ParseBorderStyle(p);
        }
        // 尝试解析为颜色
        else if (p[0] == '#' || p.find("rgb") == 0 || p.find("hsl") == 0 ||
                 p == "black" || p == "white" || p == "red" || p == "green" || p == "blue" ||
                 p == "transparent" || p == "currentColor") {
            border.color = CSSValue::ParseColor(p);
        }
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
        render_obj = CreateRenderObjectForElement(element, parent_style);
        // DEBUG: Log element with children count
        std::cout << "[DEBUG BuildRenderTree] Element <" << element->GetTagName()
                  << "> has " << node->GetChildNodes().size() << " children" << std::endl;
    }
    else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::static_pointer_cast<Text>(node);
        render_obj = CreateRenderObjectForText(text, parent_style);
        if (render_obj) {
            std::cout << "[DEBUG BuildRenderTree] TEXT node created, len="
                      << text->GetData().length() << std::endl;
        }
    }

    if (!render_obj) {
        return nullptr;
    }

    // 如果是 display: none，不创建渲染对象
    if (render_obj->GetComputedStyle().display == RenderObjectType::NONE) {
        return nullptr;
    }

    // 递归构建子树
    for (const auto& child : node->GetChildNodes()) {
        auto child_render_obj = BuildRenderTree(child, &render_obj->GetComputedStyle());
        if (child_render_obj) {
            render_obj->AppendChild(child_render_obj);
        }
    }

    return render_obj;
}

std::shared_ptr<RenderObject> RenderTreeBuilder::CreateRenderObjectForElement(
    std::shared_ptr<Element> element,
    const ComputedStyle* parent_style) {
    
    // 计算样式
    auto style = style_resolver_.ResolveStyle(element, parent_style);
    
    // 创建渲染对象
    auto render_obj = CreateRenderObjectByType(style.display);
    if (render_obj) {
        render_obj->SetNode(element);
        render_obj->SetComputedStyle(style);
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

    // 跳过纯空白文本节点
    if (text_data.find_first_not_of(" \t\n\r") == std::string::npos) {
        return nullptr;
    }

    // 规范化空白字符：将连续的空白字符（包括换行）替换为单个空格
    std::string normalized_text;
    bool in_whitespace = false;
    for (char c : text_data) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!in_whitespace) {
                normalized_text += ' ';
                in_whitespace = true;
            }
        } else {
            normalized_text += c;
            in_whitespace = false;
        }
    }

    auto render_obj = std::make_shared<RenderText>();
    render_obj->SetNode(text);
    render_obj->SetText(normalized_text);

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
            return std::make_shared<RenderBlock>(); // 简化：暂时用 Block
        case RenderObjectType::GRID:
            return std::make_shared<RenderBlock>(); // 简化：暂时用 Block
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
        case RenderObjectType::NONE:
            return nullptr;
        default:
            return std::make_shared<RenderBlock>();
    }
}

} // namespace lightui

