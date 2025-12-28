/**
 * @file ifc_layout.cpp
 * @brief IFC 布局入口实现
 */

#include "ifc_layout.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <iostream>

// Skia 字体测量
#include "../../render/text/font_manager.h"
#include "../../render/text/text_renderer.h"
#include "../../render/render_inline_block.h"
#include "../../render/render_svg.h"

// DOM 类型（用于检测 BR 元素）
#include "../../dom/element.h"
#include "../../dom/text.h"

// 调试开关
#define IFC_DEBUG 0

namespace lightui {

// ========== 静态辅助方法 ==========

bool IFCLayout::HasInlineContent(RenderObject* container) {
    if (!container) return false;

    const auto& children = container->GetChildren();
    for (const auto& child : children) {
        if (IsInlineLevel(child.get())) {
            return true;
        }
    }
    return false;
}

bool IFCLayout::IsInlineLevel(RenderObject* render_obj) {
    if (!render_obj) return false;

    RenderObjectType type = render_obj->GetType();

    switch (type) {
        case RenderObjectType::TEXT:
        case RenderObjectType::INLINE:
        case RenderObjectType::INLINE_BLOCK:
            return true;
        default:
            return false;
    }
}

// ========== 静态文本测量 ==========

// 内部使用的别名，兼容旧代码
using TextMeasurement = TextMeasureResult;

TextMeasureResult IFCLayout::MeasureTextStatic(
    const std::string& text,
    float font_size,
    const std::string& font_family,
    float letter_spacing,
    float word_spacing,
    float line_height_multiplier
) {
    TextMeasureResult result;

    if (text.empty()) {
        result.height = font_size * line_height_multiplier;
        result.skia_ascent = font_size * 0.8f;
        result.skia_descent = font_size * 0.2f;
        return result;
    }

    // 使用 FontManager 获取字体
    auto& font_manager = FontManager::GetInstance();

    // 创建字体描述符
    FontDescriptor font_desc;
    font_desc.family = font_family.empty() ? "Arial" : font_family;
    font_desc.size = font_size;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;

    // 加载字体
    SkFont font = font_manager.LoadFont(font_desc);

    // 使用 Skia 测量文本宽度（支持混合字符：ASCII、CJK、emoji）
    result.width = TextRenderer::MeasureMixedTextWidth(text, font);

    // 获取字体度量计算高度
    SkFontMetrics metrics;
    font.getMetrics(&metrics);

    // 存储 Skia 测量的精确 ascent 和 descent（转换为正值）
    result.skia_ascent = -metrics.fAscent;  // fAscent 是负值
    result.skia_descent = metrics.fDescent;  // fDescent 是正值

    // 计算 Skia 测量的精确高度（内容高度）
    float skia_content_height = result.skia_ascent + result.skia_descent;

    // 计算 line-height: normal 的值
    // 浏览器的 line-height: normal 基于字体的实际 metrics
    //
    // 根据浏览器测试（Chrome/Edge on Windows with sans-serif font），
    // line-height: normal 的值因字体大小而异。
    // 使用查找表来匹配常见字体大小的浏览器行为：
    float browser_normal_line_height;

    // 检查是否是等宽字体（monospace）
    // 等宽字体的 line-height: normal 等于 font-size（根据浏览器测试）
    bool is_monospace = (font_family == "Courier New" || font_family == "Consolas" ||
                         font_family == "monospace" || font_family == "Courier" ||
                         font_family == "Monaco" || font_family == "Menlo");

    if (is_monospace) {
        // Courier New 等宽字体的 line-height: normal 查找表
        // 基于 Chrome 浏览器使用 Courier New 字体测量的实际值
        // 比率约为 1.156（与 Arial 相同，因为浏览器使用相同的 line-height 计算）
        int font_size_int = static_cast<int>(font_size + 0.5f);
        switch (font_size_int) {
            case 13: browser_normal_line_height = 15.0f; break;  // 13px 字体
            case 16: browser_normal_line_height = 18.5f; break;  // 16px 字体 (从浏览器测量)
            default:
                browser_normal_line_height = font_size * 1.156f;
                browser_normal_line_height = std::round(browser_normal_line_height * 2.0f) / 2.0f;
                break;
        }
    } else {
        // Arial 字体的 line-height: normal 查找表
        // 基于 Chrome 浏览器使用 Arial 字体测量的实际值
        // 比率约为 1.15 (比默认 sans-serif 字体更小)
        int font_size_int = static_cast<int>(font_size + 0.5f);  // 四舍五入到整数
        switch (font_size_int) {
            case 10: browser_normal_line_height = 11.5f; break;   // ~1.15
            case 11: browser_normal_line_height = 13.0f; break;   // ~1.18
            case 12: browser_normal_line_height = 14.0f; break;   // ~1.17
            case 13: browser_normal_line_height = 15.0f; break;   // ~1.15
            case 14: browser_normal_line_height = 16.0f; break;   // ~1.14
            case 15: browser_normal_line_height = 17.5f; break;   // ~1.17
            case 16: browser_normal_line_height = 18.5f; break;   // ~1.156 (从浏览器测量)
            case 17: browser_normal_line_height = 19.5f; break;   // ~1.15
            case 18: browser_normal_line_height = 21.0f; break;   // ~1.17
            case 19: browser_normal_line_height = 22.0f; break;   // ~1.16
            case 20: browser_normal_line_height = 23.0f; break;   // ~1.15
            case 22: browser_normal_line_height = 25.5f; break;   // ~1.16
            case 24: browser_normal_line_height = 28.0f; break;   // ~1.17
            case 32: browser_normal_line_height = 37.0f; break;   // h1 (32px -> 37px, 从浏览器测量)
            default:
                // 对于其他字体大小，使用 1.156 倍数并四舍五入到 0.5px
                browser_normal_line_height = font_size * 1.156f;
                browser_normal_line_height = std::round(browser_normal_line_height * 2.0f) / 2.0f;
                break;
        }
    }

    // 如果指定了 line-height 倍数（非默认的 1.2），使用 CSS 指定的值
    // 否则使用浏览器风格的 line-height: normal
    float final_line_height;
    if (std::abs(line_height_multiplier - 1.2f) < 0.001f) {
        // 使用 line-height: normal（基于文本类型）
        final_line_height = browser_normal_line_height;
    } else {
        // 用户指定了具体的 line-height
        final_line_height = font_size * line_height_multiplier;
    }

    // 直接使用计算的 line-height，与浏览器行为一致
    result.height = final_line_height;

    // 如果 Skia 测量的高度大于 final_line_height，需要调整 ascent 和 descent
    // 以确保 VerticalAligner 中的行高计算正确
    if (skia_content_height > final_line_height) {
        // 按比例缩放 ascent 和 descent
        float scale = final_line_height / skia_content_height;
        result.skia_ascent = result.skia_ascent * scale;
        result.skia_descent = result.skia_descent * scale;
    }

    // 计算 UTF-8 字符数（用于 letter-spacing）
    int char_count = 0;
    size_t pos = 0;
    while (pos < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[pos]);
        if ((c & 0x80) == 0) {
            pos += 1;
        } else if ((c & 0xE0) == 0xC0) {
            pos += 2;
        } else if ((c & 0xF0) == 0xE0) {
            pos += 3;
        } else if ((c & 0xF8) == 0xF0) {
            pos += 4;
        } else {
            pos += 1;
        }
        char_count++;
    }

    // 应用 letter-spacing：在每个字符之间添加间距（最后一个字符后面不加）
    if (char_count > 1 && letter_spacing != 0.0f) {
        result.width += letter_spacing * (char_count - 1);
    }

    // 应用 word-spacing：在每个空格处添加额外间距
    int space_count = 0;
    for (char ch : text) {
        if (ch == ' ') {
            space_count++;
        }
    }
    if (space_count > 0 && word_spacing != 0.0f) {
        result.width += word_spacing * space_count;
    }

    return result;
}

// 内部辅助函数：兼容旧代码调用
static inline TextMeasurement MeasureTextForIFC(
    const std::string& text,
    float font_size,
    const std::string& font_family,
    float letter_spacing,
    float word_spacing,
    float line_height_multiplier = 1.2f
) {
    return IFCLayout::MeasureTextStatic(text, font_size, font_family, letter_spacing, word_spacing, line_height_multiplier);
}

// ========== 缓存方法 ==========

size_t IFCLayout::GetContentVersion(RenderObject* container) const {
    if (!container) return 0;

    // 简单的版本计算：基于子节点数量和容器地址
    // 实际项目中应该使用更精确的脏标记系统
    size_t version = reinterpret_cast<size_t>(container);
    version ^= container->GetChildren().size();

    // 遍历子节点，累加版本信息
    for (const auto& child : container->GetChildren()) {
        version ^= reinterpret_cast<size_t>(child.get());
        if (child->GetType() == RenderObjectType::TEXT) {
            // 文本节点：使用文本内容的哈希
            auto text_node = std::dynamic_pointer_cast<RenderText>(child);
            if (text_node) {
                std::hash<std::string> hasher;
                version ^= hasher(text_node->GetText());
            }
        }
    }

    return version;
}

bool IFCLayout::IsCacheValid(RenderObject* container, float available_width, uint64_t content_version) const {
    auto it = cache_.find(container);
    if (it == cache_.end()) return false;

    const auto& cache = it->second;
    if (!cache.valid) return false;

    // 检查可用宽度是否相同
    if (std::abs(cache.available_width - available_width) > 0.01f) return false;

    // 检查内容版本是否相同
    // 如果传入的 content_version 为 0，使用旧的哈希计算方式（向后兼容）
    // 否则直接比较版本号
    if (content_version != 0) {
        // 使用外部传入的版本号直接比较
        if (cache.content_version != content_version) return false;
    } else {
        // 向后兼容：使用旧的哈希计算方式
        if (cache.content_version != GetContentVersion(container)) return false;
    }

    return true;
}

void IFCLayout::InvalidateCache(RenderObject* container) {
    auto it = cache_.find(container);
    if (it != cache_.end()) {
        it->second.valid = false;
    }
}

void IFCLayout::ClearCache() {
    cache_.clear();
}

float IFCLayout::MeasureMinContentWidth(RenderObject* container) {
    if (!container) return 0.0f;

    float max_word_width = 0.0f;
    auto& font_manager = FontManager::GetInstance();

    // Helper function to measure a single word
    auto measureWord = [](const std::string& word, const SkFont& font) -> float {
        if (word.empty()) return 0.0f;
        float width = TextRenderer::MeasureMixedTextWidth(word, font);
        return width;
    };

    // Helper function to check if a character is CJK
    auto isCJK = [](uint32_t codepoint) -> bool {
        return (codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||   // CJK Unified Ideographs
               (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||   // CJK Unified Ideographs Extension A
               (codepoint >= 0x20000 && codepoint <= 0x2A6DF) || // CJK Unified Ideographs Extension B
               (codepoint >= 0x2A700 && codepoint <= 0x2B73F) || // CJK Unified Ideographs Extension C
               (codepoint >= 0x2B740 && codepoint <= 0x2B81F) || // CJK Unified Ideographs Extension D
               (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||   // CJK Compatibility Ideographs
               (codepoint >= 0x3000 && codepoint <= 0x303F) ||   // CJK Symbols and Punctuation
               (codepoint >= 0xFF00 && codepoint <= 0xFFEF);     // Halfwidth and Fullwidth Forms
    };

    // Recursively measure all text nodes
    std::function<void(RenderObject*)> measureChildren = [&](RenderObject* obj) {
        if (!obj) return;

        if (obj->GetType() == RenderObjectType::TEXT) {
            RenderText* text_obj = static_cast<RenderText*>(obj);
            const std::string& text = text_obj->GetText();
            const auto& style = obj->GetComputedStyle();

            // Create font for measurement using FontManager
            FontDescriptor font_desc;
            font_desc.family = style.font_family.empty() ? "Arial" : style.font_family;
            font_desc.size = style.font_size;
            font_desc.weight = FontWeight::NORMAL;
            font_desc.style = FontStyle::NORMAL;
            SkFont font = font_manager.LoadFont(font_desc);

            // Measure the minimum content width (longest word)
            // For CJK text, each character is a potential break point
            // For Latin text, words are separated by spaces
            std::string current_word;
            size_t i = 0;
            while (i < text.size()) {
                unsigned char c = text[i];
                uint32_t codepoint = 0;
                size_t char_len = 1;

                // Decode UTF-8
                if ((c & 0x80) == 0) {
                    codepoint = c;
                    char_len = 1;
                } else if ((c & 0xE0) == 0xC0) {
                    codepoint = c & 0x1F;
                    char_len = 2;
                } else if ((c & 0xF0) == 0xE0) {
                    codepoint = c & 0x0F;
                    char_len = 3;
                } else if ((c & 0xF8) == 0xF0) {
                    codepoint = c & 0x07;
                    char_len = 4;
                }

                for (size_t j = 1; j < char_len && i + j < text.size(); ++j) {
                    codepoint = (codepoint << 6) | (text[i + j] & 0x3F);
                }

                std::string char_str = text.substr(i, char_len);

                if (codepoint == ' ' || codepoint == '\t' || codepoint == '\n' || codepoint == '\r') {
                    // Whitespace - end of word
                    if (!current_word.empty()) {
                        float word_width = measureWord(current_word, font);
                        max_word_width = std::max(max_word_width, word_width);
                        current_word.clear();
                    }
                } else if (isCJK(codepoint)) {
                    // CJK character - each is a potential break point
                    if (!current_word.empty()) {
                        float word_width = measureWord(current_word, font);
                        max_word_width = std::max(max_word_width, word_width);
                        current_word.clear();
                    }
                    // Measure the CJK character itself
                    float char_width = measureWord(char_str, font);
                    max_word_width = std::max(max_word_width, char_width);
                } else {
                    // Non-CJK, non-whitespace - add to current word
                    current_word += char_str;
                }

                i += char_len;
            }

            // Don't forget the last word
            if (!current_word.empty()) {
                float word_width = measureWord(current_word, font);
                max_word_width = std::max(max_word_width, word_width);
            }
        }

        // Recurse into children
        for (const auto& child : obj->GetChildren()) {
            measureChildren(child.get());
        }
    };

    measureChildren(container);

    return max_word_width;
}

// ========== 主布局方法 ==========

IFCLayoutResult IFCLayout::Layout(RenderObject* container, float available_width, bool apply_results, uint64_t content_version) {
    IFCLayoutResult result;

    if (!container) {
        return result;
    }

    // Calculate container's border-box width for ApplyLayoutResults
    // We need this because ApplyLayoutResults needs to calculate padding offset
    const auto& style = container->GetComputedStyle();

    // For percentage padding, we need the container's border-box width
    // But we only have content width (available_width). We need to estimate.
    // For fixed pixel padding, this doesn't matter.
    // For percentage padding, we use available_width as an approximation.
    float padding_left = style.padding.left.ToPx(available_width, style.font_size);
    float padding_right = style.padding.right.ToPx(available_width, style.font_size);
    float border_left = style.border_left_width;
    float border_right = style.border_right_width;
    if (border_left == 0 && border_right == 0) {
        float border_width = style.border.width.ToPx(available_width, style.font_size);
        border_left = border_right = border_width;
    }
    float container_width = available_width + padding_left + padding_right + border_left + border_right;

    // 检查缓存（传入外部版本号）
    if (IsCacheValid(container, available_width, content_version)) {
        const auto& cache = cache_[container];
        line_boxes_ = cache.line_boxes;
        inline_boxes_ = cache.inline_boxes;  // Also restore inline_boxes for ApplyLayoutResults
        content_height_ = cache.content_height;
        content_width_ = cache.content_width;

        // Re-apply layout results to update render object positions (only if requested)
        if (apply_results) {
            ApplyLayoutResults(container, container_width);
        }

        result.total_height = content_height_;
        result.max_width = content_width_;
        result.line_count = line_boxes_.size();
        result.success = true;
        return result;
    }

    // 清除之前的结果
    line_boxes_.clear();
    inline_boxes_.clear();
    content_height_ = 0.0f;
    content_width_ = 0.0f;

    // 保存当前可用宽度（供 CreateInlineBox 使用）
    current_available_width_ = available_width;

    // 1. 收集内联内容
    CollectInlineContent(container);

    if (inline_boxes_.empty()) {
        result.success = true;
        return result;
    }

    // 2. 配置断行器
    // Note: 'style' is already defined at the beginning of this function

    // 解析 white-space
    if (style.white_space == "pre" || style.white_space == "pre-wrap") {
        line_breaker_.SetWhiteSpace(WhiteSpaceMode::PRE_WRAP);
    } else if (style.white_space == "nowrap") {
        line_breaker_.SetWhiteSpace(WhiteSpaceMode::NOWRAP);
    } else {
        line_breaker_.SetWhiteSpace(WhiteSpaceMode::NORMAL);
    }

    // word-wrap: break-word 映射到 overflow-wrap
    if (style.word_wrap == "break-word") {
        line_breaker_.SetOverflowWrap(OverflowWrapMode::BREAK_WORD);
    } else {
        line_breaker_.SetOverflowWrap(OverflowWrapMode::NORMAL);
    }

    // 解析 word-break 属性
    if (style.word_break == "break-all") {
        line_breaker_.SetWordBreak(WordBreakMode::BREAK_ALL);
    } else if (style.word_break == "keep-all") {
        line_breaker_.SetWordBreak(WordBreakMode::KEEP_ALL);
    } else if (style.word_break == "break-word") {
        // word-break: break-word 等同于 overflow-wrap: break-word
        line_breaker_.SetWordBreak(WordBreakMode::NORMAL);
        line_breaker_.SetOverflowWrap(OverflowWrapMode::BREAK_WORD);
    } else {
        // normal 或其他值
        line_breaker_.SetWordBreak(WordBreakMode::NORMAL);
    }

    // 设置文本间距和缩进
    float text_indent = style.text_indent.ToPx(available_width, style.font_size);
    float letter_spacing = style.letter_spacing.ToPx(available_width, style.font_size);
    float word_spacing = style.word_spacing.ToPx(available_width, style.font_size);

    line_breaker_.SetTextIndent(text_indent);
    line_breaker_.SetLetterSpacing(letter_spacing);
    line_breaker_.SetWordSpacing(word_spacing);

    // 3. 断行
    line_boxes_ = line_breaker_.BreakIntoLines(inline_boxes_, available_width);

    // 4. 计算每行高度和对齐
    float current_y = 0.0f;

    for (auto& line : line_boxes_) {
        // 获取行内盒子的对齐信息
        std::vector<InlineBox*> box_ptrs;
        std::vector<VerticalAlignInfo> aligns;

        for (auto* box : line.boxes) {
            box_ptrs.push_back(box);

            // 从渲染对象读取 vertical-align
            VerticalAlignInfo align_info{VerticalAlignType::BASELINE, 0.0f};
            if (box->render_object) {
                const auto& box_style = box->render_object->GetComputedStyle();
                align_info = vertical_aligner_.ParseVerticalAlign(
                    box_style.vertical_align, box_style.font_size);
            }
            aligns.push_back(align_info);
        }

        // 计算容器的 line-height（像素值）
        // 如果 style.line_height 是默认值 1.2，使用浏览器风格的 line-height: normal
        float container_line_height;
        if (std::abs(style.line_height - 1.2f) < 0.001f) {
            // 使用浏览器风格的 line-height: normal（与 MeasureTextStatic 中的查找表一致）
            // Arial 字体的 line-height: normal 值（比率约 1.156）
            int font_size_int = static_cast<int>(style.font_size + 0.5f);
            switch (font_size_int) {
                case 10: container_line_height = 11.5f; break;   // ~1.15
                case 11: container_line_height = 13.0f; break;   // ~1.18
                case 12: container_line_height = 14.0f; break;   // ~1.17
                case 13: container_line_height = 15.0f; break;   // ~1.15
                case 14: container_line_height = 16.0f; break;   // ~1.14
                case 15: container_line_height = 17.5f; break;   // ~1.17
                case 16: container_line_height = 18.5f; break;   // ~1.156 (从浏览器测量)
                case 17: container_line_height = 19.5f; break;   // ~1.15
                case 18: container_line_height = 21.0f; break;   // ~1.17
                case 19: container_line_height = 22.0f; break;   // ~1.16
                case 20: container_line_height = 23.0f; break;   // ~1.15
                case 22: container_line_height = 25.5f; break;   // ~1.16
                case 24: container_line_height = 28.0f; break;   // ~1.17
                case 32: container_line_height = 37.0f; break;   // h1 (32px -> 37px)
                default:
                    container_line_height = style.font_size * 1.156f;
                    container_line_height = std::round(container_line_height * 2.0f) / 2.0f;
                    break;
            }
        } else {
            // 用户指定了具体的 line-height
            container_line_height = style.line_height * style.font_size;
        }

#if IFC_DEBUG
        std::cout << "[IFC] Container line-height: " << container_line_height
                  << " (multiplier=" << style.line_height << ", font_size=" << style.font_size << ")" << std::endl;
#endif

        // 计算行度量，传入容器的 line-height
        auto line_metrics = vertical_aligner_.CalculateLineMetrics(box_ptrs, aligns, container_line_height);
        line.height = line_metrics.line_height;
        line.baseline = line_metrics.baseline;

#if IFC_DEBUG
        std::cout << "[IFC] Line metrics: line_height=" << line_metrics.line_height
                  << ", baseline=" << line_metrics.baseline
                  << ", boxes=" << box_ptrs.size() << std::endl;
#endif

        // 设置行位置
        line.y = current_y;

        // 首先设置每个盒子的水平位置
        float current_x = line.x;  // 行的起始 x 位置（可能有 text-indent）
        for (auto* box : line.boxes) {
            if (!box) continue;
            // 盒子的 x 位置是内容区域的起始位置（在 margin_left 之后）
            // current_x 指向当前可用空间的起始位置
            current_x += box->margin_left;  // 先跳过左边距
            box->x = current_x;             // 内容区域从这里开始
            current_x += box->width + box->margin_right;  // 移动到下一个盒子的起始位置
        }

        // 应用垂直对齐，传入容器的 line-height
        vertical_aligner_.AlignBoxes(box_ptrs, aligns, current_y, container_line_height);

        // 应用水平对齐（调整 x 坐标）
        line.ApplyTextAlign(style.text_align);

        // 更新统计
#if IFC_DEBUG
        std::cout << "[IFC] current_y: " << current_y << " -> " << (current_y + line_metrics.line_height) << std::endl;
#endif
        current_y += line_metrics.line_height;
        content_width_ = std::max(content_width_, line.content_width);
    }

    content_height_ = current_y;
#if IFC_DEBUG
    std::cout << "[IFC] Total content_height: " << content_height_ << std::endl;
#endif

    // 5. 应用布局结果到渲染对象 (only if requested)
    if (apply_results) {
        ApplyLayoutResults(container, container_width);
    }

    // 6. 更新缓存
    LayoutCache& cache = cache_[container];
    cache.available_width = available_width;
    cache.content_height = content_height_;
    cache.content_width = content_width_;
    cache.line_boxes = line_boxes_;
    cache.inline_boxes = inline_boxes_;  // Cache inline boxes for ApplyLayoutResults
    // 使用外部传入的版本号，如果为0则使用旧的哈希计算方式（向后兼容）
    cache.content_version = (content_version != 0) ? content_version : GetContentVersion(container);
    cache.valid = true;

    // 填充结果
    result.total_height = content_height_;
    result.max_width = content_width_;
    result.line_count = line_boxes_.size();
    result.success = true;

    return result;
}

IFCMeasureResult IFCLayout::LayoutWithResult(RenderObject* container, float available_width) {
    IFCMeasureResult measure_result;

    // 先调用 Layout 进行实际布局
    IFCLayoutResult layout_result = Layout(container, available_width);

    if (!layout_result.success) {
        return measure_result;
    }

    // 填充 IFCMeasureResult
    measure_result.content_width = layout_result.max_width;
    measure_result.content_height = layout_result.total_height;
    measure_result.success = layout_result.success;

    // 将 LineBox 转换为 LineBoxInfo
    measure_result.line_boxes.reserve(line_boxes_.size());
    for (const auto& line : line_boxes_) {
        LineBoxInfo info;
        info.y = line.y;
        info.height = line.height;
        info.baseline = line.baseline;
        info.content_width = line.content_width;
        info.box_count = line.boxes.size();
        measure_result.line_boxes.push_back(info);
    }

    return measure_result;
}

// ========== 内联内容收集 ==========

void IFCLayout::CollectInlineContent(RenderObject* container) {
    // 收集内容（已禁用调试输出）
    /*
    std::string container_tag = "?";
    if (container) {
        auto node = container->GetNode();
        (void)node; // unused
    }
    */
    
    const auto& children = container->GetChildren();
    
    for (const auto& child : children) {
        CreateInlineBox(child.get());
    }
}

void IFCLayout::CreateInlineBox(RenderObject* render_obj) {
    if (!render_obj) return;

    RenderObjectType type = render_obj->GetType();
    const auto& style = render_obj->GetComputedStyle();

    switch (type) {
        case RenderObjectType::TEXT: {
            // 文本节点
            RenderText* text_obj = dynamic_cast<RenderText*>(render_obj);
            if (!text_obj) break;

            std::string text = text_obj->GetText();
            if (text.empty()) break;

            // 获取 letter-spacing 和 word-spacing
            float letter_spacing = style.letter_spacing.ToPx(0, style.font_size);
            float word_spacing = style.word_spacing.ToPx(0, style.font_size);

            // 检查 white-space 属性
            bool wrap_allowed = (style.white_space != "nowrap" && style.white_space != "pre");
            bool preserve_newlines = (style.white_space == "pre" || style.white_space == "pre-wrap" || style.white_space == "pre-line");

            // 如果 white-space: pre/pre-wrap/pre-line，需要按 \n 分割文本
            if (preserve_newlines && text.find('\n') != std::string::npos) {
                // 按换行符分割文本
                std::vector<std::string> lines;
                size_t start = 0;
                size_t pos = 0;
                while ((pos = text.find('\n', start)) != std::string::npos) {
                    lines.push_back(text.substr(start, pos - start));
                    start = pos + 1;
                }
                // 添加最后一行（\n 后面的内容）
                if (start < text.size()) {
                    lines.push_back(text.substr(start));
                } else if (start == text.size()) {
                    // 文本以 \n 结尾，添加空行
                    lines.push_back("");
                }

#if IFC_DEBUG
                std::cout << "[IFC CreateInlineBox] PRE mode: split into " << lines.size() << " lines by \\n" << std::endl;
#endif

                // 保存换行后的文本到 RenderText 对象
                text_obj->SetWrappedLines(lines);

                // 为每一行创建一个 InlineBox
                for (size_t i = 0; i < lines.size(); ++i) {
                    const std::string& line_text = lines[i];

                    // 对于空行，使用空格来获取正确的行高
                    std::string measure_text = line_text.empty() ? " " : line_text;
                    TextMeasurement line_measurement = MeasureTextForIFC(
                        measure_text, style.font_size, style.font_family, letter_spacing, word_spacing, style.line_height);

                    // 空行宽度为0
                    if (line_text.empty()) {
                        line_measurement.width = 0;
                    }

                    InlineBox box = InlineBox::CreateTextBox(render_obj);
                    box.width = line_measurement.width;
                    box.height = line_measurement.height;
                    box.baseline = line_measurement.skia_ascent;
                    box.skia_ascent = line_measurement.skia_ascent;
                    box.skia_descent = line_measurement.skia_descent;
                    box.line_height_multiplier = style.line_height;

                    // 添加 TextRun
                    TextRun run;
                    run.text = line_text;
                    run.start_offset = 0;
                    run.end_offset = line_text.size();
                    run.width = line_measurement.width;
                    run.height = line_measurement.height;
                    run.baseline = line_measurement.skia_ascent;
                    // 标记除最后一行外的所有行为强制换行
                    if (i < lines.size() - 1) {
                        run.is_forced_break = true;
                    }
                    box.text_runs.push_back(run);

                    inline_boxes_.push_back(std::move(box));
                }
            } else {
                // 测量整个文本
                TextMeasurement measurement = MeasureTextForIFC(
                    text, style.font_size, style.font_family, letter_spacing, word_spacing, style.line_height);

                // 如果文本宽度超过可用宽度且允许换行，则分割文本
#if IFC_DEBUG
                std::cout << "[IFC CreateInlineBox] TEXT: wrap_allowed=" << wrap_allowed
                          << ", available_width=" << current_available_width_
                          << ", text_width=" << measurement.width
                          << ", text=" << text.substr(0, 50) << "..." << std::endl;
#endif
                if (wrap_allowed && current_available_width_ > 0 && measurement.width > current_available_width_) {
                    // 使用 TextRenderer::WrapText 进行文本换行
                    auto& font_manager = FontManager::GetInstance();
                    FontDescriptor font_desc;
                    font_desc.family = style.font_family.empty() ? "Arial" : style.font_family;
                    font_desc.size = style.font_size;
                    font_desc.weight = FontWeight::NORMAL;
                    font_desc.style = FontStyle::NORMAL;
                    SkFont font = font_manager.LoadFont(font_desc);

                    TextRenderer text_renderer(nullptr);  // 创建 TextRenderer 实例
                    std::vector<std::string> wrapped_lines = text_renderer.WrapText(text, current_available_width_, font);

#if IFC_DEBUG
                    std::cout << "[IFC CreateInlineBox] Wrapped into " << wrapped_lines.size() << " lines:" << std::endl;
                    for (size_t i = 0; i < wrapped_lines.size(); ++i) {
                        std::cout << "  Line " << i << ": \"" << wrapped_lines[i] << "\"" << std::endl;
                    }
#endif

                    // 保存换行后的文本到 RenderText 对象
                    text_obj->SetWrappedLines(wrapped_lines);

                    // 为每一行创建一个 InlineBox
                    for (size_t i = 0; i < wrapped_lines.size(); ++i) {
                        const std::string& line_text = wrapped_lines[i];
                        if (line_text.empty()) continue;

                        TextMeasurement line_measurement = MeasureTextForIFC(
                            line_text, style.font_size, style.font_family, letter_spacing, word_spacing, style.line_height);

                        InlineBox box = InlineBox::CreateTextBox(render_obj);
                        box.width = line_measurement.width;
                        box.height = line_measurement.height;
                        box.baseline = line_measurement.skia_ascent;
                        box.skia_ascent = line_measurement.skia_ascent;
                        box.skia_descent = line_measurement.skia_descent;
                        box.line_height_multiplier = style.line_height;

                        // 添加 TextRun
                        TextRun run;
                        run.text = line_text;
                        run.start_offset = 0;
                        run.end_offset = line_text.size();
                        run.width = line_measurement.width;
                        run.height = line_measurement.height;
                        run.baseline = line_measurement.skia_ascent;
                        // 标记除最后一行外的所有行为强制换行
                        if (i < wrapped_lines.size() - 1) {
                            run.is_forced_break = true;
                        }
                        box.text_runs.push_back(run);

                        inline_boxes_.push_back(std::move(box));
                    }
                } else {
                    // 不需要换行，创建单个 InlineBox
                    text_obj->SetWrappedLines({});  // 清除之前的换行信息

                    InlineBox box = InlineBox::CreateTextBox(render_obj);
                    box.width = measurement.width;
                    box.height = measurement.height;
                    box.baseline = measurement.skia_ascent;
                    box.skia_ascent = measurement.skia_ascent;
                    box.skia_descent = measurement.skia_descent;
                    box.line_height_multiplier = style.line_height;

                    // 添加 TextRun
                    TextRun run;
                    run.text = text;
                    run.start_offset = 0;
                    run.end_offset = text.size();
                    run.width = measurement.width;
                    run.height = measurement.height;
                    run.baseline = measurement.skia_ascent;
                    box.text_runs.push_back(run);

                    inline_boxes_.push_back(std::move(box));
                }
            }
            break;
        }

        case RenderObjectType::INLINE_BLOCK: {
            // 原子内联元素
            // 先检查是否是 SVG 元素（RenderSVGRoot 类型也是 INLINE_BLOCK，但不是 RenderInlineBlock）
            float w = 0, h = 0;
            auto* svg_root = dynamic_cast<RenderSVGRoot*>(render_obj);
            if (svg_root) {
                // SVG 元素使用 RenderSVGRoot::MeasureIntrinsicSize
                std::tie(w, h) = svg_root->MeasureIntrinsicSize(current_available_width_);
            } else {
                // 普通 inline-block 元素使用 RenderInlineBlock::MeasureIntrinsicSize
                auto* inline_block = static_cast<RenderInlineBlock*>(render_obj);
                std::tie(w, h) = inline_block->MeasureIntrinsicSize(current_available_width_);
            }

            // 注意：不再覆盖 MeasureIntrinsicSize 返回的尺寸
            // MeasureIntrinsicSize 已经正确处理了 box-sizing 和显式尺寸

            InlineBox box = InlineBox::CreateAtomicBox(render_obj, w, h, h);

            // 应用 margin（水平和垂直）
            box.margin_left = style.margin.left.ToPx(w, style.font_size);
            box.margin_right = style.margin.right.ToPx(w, style.font_size);
            box.margin_top = style.margin.top.ToPx(w, style.font_size);
            box.margin_bottom = style.margin.bottom.ToPx(w, style.font_size);

            // 设置行高倍数（用于计算行高）
            box.line_height_multiplier = style.line_height;

            inline_boxes_.push_back(std::move(box));
            break;
        }

        case RenderObjectType::INLINE: {
            // 检查是否是 BR 元素
            auto node = render_obj->GetNode();
            
            // 🔍 调试：输出INLINE元素信息（已禁用）
            /*
            std::string tag_name = "?";
            std::string text_content = "";
            if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto element = std::static_pointer_cast<Element>(node);
                tag_name = element->GetTagName();
                text_content = element->GetTextContent();
            }
            size_t children_count = render_obj->GetChildren().size();
            printf("[IFC CreateInlineBox] INLINE: tag=%s, textContent='%s', children=%zu\n",
                   tag_name.c_str(), text_content.c_str(), children_count);
            */
            
            if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto element = std::static_pointer_cast<Element>(node);
                std::string tag_name = element->GetTagName();

                if (tag_name == "br" || tag_name == "BR") {
                    // BR 元素 - 创建一个带有强制换行标记的空文本盒子
                    InlineBox box = InlineBox::CreateTextBox(render_obj);
                    box.width = 0.0f;
                    // 使用父元素的 line-height 来确定 BR 的高度
                    box.height = style.font_size * style.line_height;
                    box.baseline = style.font_size * 0.8f;
                    box.skia_ascent = style.font_size * 0.8f;
                    box.skia_descent = style.font_size * 0.2f;
                    box.line_height_multiplier = style.line_height;

                    // 添加一个空的 TextRun，标记为强制换行
                    TextRun run;
                    run.text = "";
                    run.start_offset = 0;
                    run.end_offset = 0;
                    run.width = 0.0f;
                    run.height = box.height;
                    run.baseline = box.baseline;
                    run.is_forced_break = true;  // 关键：标记为强制换行
                    box.text_runs.push_back(run);

                    inline_boxes_.push_back(std::move(box));
                    break;
                }
            }

            // 普通内联元素 - 递归处理子元素
            // 添加 INLINE_START 标记
            InlineBox start = InlineBox::CreateInlineStart(render_obj);
            inline_boxes_.push_back(std::move(start));

            // 处理子元素
            for (const auto& child : render_obj->GetChildren()) {
                CreateInlineBox(child.get());
            }

            // 添加 INLINE_END 标记
            InlineBox end = InlineBox::CreateInlineEnd(render_obj);
            inline_boxes_.push_back(std::move(end));
            break;
        }

        default:
            // 非内联元素，不处理
            break;
    }
}

// ========== 测量文本 ==========

std::pair<float, float> IFCLayout::MeasureText(
    const std::string& text,
    const ComputedStyle& style
) {
    float letter_spacing = style.letter_spacing.ToPx(0, style.font_size);
    float word_spacing = style.word_spacing.ToPx(0, style.font_size);
    TextMeasurement measurement = MeasureTextForIFC(text, style.font_size, style.font_family, letter_spacing, word_spacing, style.line_height);
    return {measurement.width, measurement.height};
}

// ========== 应用布局结果 ==========

void IFCLayout::ApplyLayoutResults(RenderObject* container, float container_width) {
    if (!container) return;

    // Get container's padding and border for offset calculation
    const auto& container_style = container->GetComputedStyle();

    // Use the passed container_width instead of container_layout.width
    // because container_layout may not be set yet during layout computation
    float padding_left = container_style.padding.left.ToPx(container_width, container_style.font_size);
    float padding_top = container_style.padding.top.ToPx(container_width, container_style.font_size);
    float border_left = container_style.border_left_width;
    float border_top = container_style.border_top_width;
    if (border_left == 0 && border_top == 0) {
        float border_width = container_style.border.width.ToPx(container_width, container_style.font_size);
        border_left = border_top = border_width;
    }

    // Offset to add to convert from content-area coordinates to border-box coordinates
    float offset_x = padding_left + border_left;
    float offset_y = padding_top + border_top;

    // 用于跟踪内联元素的边界
    struct InlineElementBounds {
        float min_x = std::numeric_limits<float>::max();
        float min_y = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float max_y = std::numeric_limits<float>::lowest();
        bool has_content = false;
    };
    std::unordered_map<RenderObject*, InlineElementBounds> inline_bounds;

    // 用于跟踪文本节点的边界（多行文本需要合并边界）
    std::unordered_map<RenderObject*, InlineElementBounds> text_bounds;

    // 第一遍：收集所有内联盒的位置信息
    std::vector<RenderObject*> inline_stack;  // 当前活跃的内联元素栈

    for (const auto& box : inline_boxes_) {
        RenderObject* render_obj = box.render_object;
        if (!render_obj) continue;

        if (box.type == InlineBoxType::INLINE_START) {
            // 开始一个新的内联元素
            inline_stack.push_back(render_obj);
            inline_bounds[render_obj] = InlineElementBounds{};
        } else if (box.type == InlineBoxType::INLINE_END) {
            // 结束当前内联元素
            if (!inline_stack.empty() && inline_stack.back() == render_obj) {
                inline_stack.pop_back();
            }
        } else {
            // TEXT 或 ATOMIC 盒子
            // Add offset to convert to border-box coordinates
            // box.x 已经是内容区域的起始位置（margin_left 已经在布局时处理过了）
            float box_left = box.x + offset_x;
            float box_right = box_left + box.width;
            float box_top = box.y + offset_y;
            float box_bottom = box_top + box.height;

#if IFC_DEBUG
            if (box.type == InlineBoxType::TEXT) {
                std::cout << "[IFC Apply] TEXT box: x=" << box.x << ", y=" << box.y
                          << ", width=" << box.width << ", height=" << box.height
                          << ", offset_x=" << offset_x << ", box_left=" << box_left
                          << std::endl;
            }
#endif

            // 对于文本节点，合并所有行的边界
            if (box.type == InlineBoxType::TEXT) {
                auto& bounds = text_bounds[render_obj];
                if (!bounds.has_content) {
                    bounds.min_x = box_left;
                    bounds.min_y = box_top;
                    bounds.max_x = box_right;
                    bounds.max_y = box_bottom;
                    bounds.has_content = true;
                } else {
                    bounds.min_x = std::min(bounds.min_x, box_left);
                    bounds.min_y = std::min(bounds.min_y, box_top);
                    bounds.max_x = std::max(bounds.max_x, box_right);
                    bounds.max_y = std::max(bounds.max_y, box_bottom);
                }
            } else {
                // ATOMIC 盒子直接更新布局信息
                LayoutInfo& layout = render_obj->GetLayoutInfo();
                layout.x = box_left;
                layout.y = box_top;
                layout.width = box.width;
                layout.height = box.height;

                // 对于 inline-block 元素，需要调用 Layout 来设置其内部子元素的位置
                // 这样才能正确应用 text-align 等属性
                if (render_obj->GetType() == RenderObjectType::INLINE_BLOCK) {
                    auto* inline_block = static_cast<RenderInlineBlock*>(render_obj);
                    // 调用 Layout 来设置子元素位置（尺寸已经在 MeasureIntrinsicSize 中计算过了）
                    inline_block->Layout(box.width, box.height);
                }
            }

            // 更新所有父级内联元素的边界
            for (RenderObject* inline_elem : inline_stack) {
                auto& bounds = inline_bounds[inline_elem];
                bounds.min_x = std::min(bounds.min_x, box_left);
                bounds.min_y = std::min(bounds.min_y, box_top);
                bounds.max_x = std::max(bounds.max_x, box_right);
                bounds.max_y = std::max(bounds.max_y, box_bottom);
                bounds.has_content = true;
            }
        }
    }

    // 第二遍：先应用内联元素的边界（因为子元素需要相对于父元素的位置）
    for (auto& [render_obj, bounds] : inline_bounds) {
        if (!bounds.has_content) continue;

        LayoutInfo& layout = render_obj->GetLayoutInfo();
        layout.x = bounds.min_x;
        layout.y = bounds.min_y;
        layout.width = bounds.max_x - bounds.min_x;
        layout.height = bounds.max_y - bounds.min_y;
    }

    // 第三遍：应用文本节点的合并边界
    // 文本节点的位置需要相对于其父 INLINE 元素（如果有的话）
    // 因为 RenderInline::Paint 会先 translate 到自己的位置
    for (auto& [render_obj, bounds] : text_bounds) {
        if (!bounds.has_content) continue;

        LayoutInfo& layout = render_obj->GetLayoutInfo();

        // 获取父元素
        auto parent = render_obj->GetParent();

        // 检查父元素是否是 INLINE 类型（且不是 IFC 容器）
        // 如果是，则文本位置需要相对于父 INLINE 元素
        if (parent && parent->GetType() == RenderObjectType::INLINE) {
            const LayoutInfo& parent_layout = parent->GetLayoutInfo();
            // 文本位置 = 绝对位置 - 父元素绝对位置
            layout.x = bounds.min_x - parent_layout.x;
            layout.y = bounds.min_y - parent_layout.y;
        } else {
            // 没有 INLINE 父元素，使用绝对位置
            layout.x = bounds.min_x;
            layout.y = bounds.min_y;
        }

        layout.width = bounds.max_x - bounds.min_x;
        layout.height = bounds.max_y - bounds.min_y;
    }
}

} // namespace lightui

