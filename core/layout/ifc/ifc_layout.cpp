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
#include "../../render/text_renderer.h"
#include "../../render/render_inline_block.h"

// DOM 类型（用于检测 BR 元素）
#include "../../dom/element.h"

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
    // 浏览器的 line-height: normal 基于字体的 metrics，通常约为 1.15-1.35 倍字体大小
    // 根据浏览器测试：16px -> 21px (1.3125), 24px -> 32px (1.333), 32px -> 43px (1.34375)
    // 使用公式：normal_line_height = ceil(font_size * 1.3)，然后取整到最接近的像素
    float browser_normal_line_height = std::ceil(font_size * 1.3f);

    // 如果指定了 line-height 倍数（非默认的 1.2），使用 CSS 指定的值
    // 否则使用浏览器风格的 line-height: normal
    float final_line_height;
    if (line_height_multiplier != 1.2f) {
        // 用户指定了具体的 line-height
        final_line_height = font_size * line_height_multiplier;
    } else {
        // 使用 line-height: normal（浏览器风格）
        final_line_height = browser_normal_line_height;
    }

    // 使用较大的高度，确保行间距足够
    result.height = std::max(skia_content_height, final_line_height);



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

bool IFCLayout::IsCacheValid(RenderObject* container, float available_width) const {
    auto it = cache_.find(container);
    if (it == cache_.end()) return false;

    const auto& cache = it->second;
    if (!cache.valid) return false;

    // 检查可用宽度是否相同
    if (std::abs(cache.available_width - available_width) > 0.01f) return false;

    // 检查内容版本是否相同
    if (cache.content_version != GetContentVersion(container)) return false;

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

// ========== 主布局方法 ==========

IFCLayoutResult IFCLayout::Layout(RenderObject* container, float available_width) {
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

    // 检查缓存
    if (IsCacheValid(container, available_width)) {
        const auto& cache = cache_[container];
        line_boxes_ = cache.line_boxes;
        inline_boxes_ = cache.inline_boxes;  // Also restore inline_boxes for ApplyLayoutResults
        content_height_ = cache.content_height;
        content_width_ = cache.content_width;

        // Re-apply layout results to update render object positions
        ApplyLayoutResults(container, container_width);

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

    // 默认断词模式
    line_breaker_.SetWordBreak(WordBreakMode::NORMAL);

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
        float container_line_height = style.line_height * style.font_size;

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
            box->x = current_x + box->margin_left;
            current_x += box->GetTotalWidth();
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

    // 5. 应用布局结果到渲染对象
    ApplyLayoutResults(container, container_width);

    // 6. 更新缓存
    LayoutCache& cache = cache_[container];
    cache.available_width = available_width;
    cache.content_height = content_height_;
    cache.content_width = content_width_;
    cache.line_boxes = line_boxes_;
    cache.inline_boxes = inline_boxes_;  // Cache inline boxes for ApplyLayoutResults
    cache.content_version = GetContentVersion(container);
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
            // 使用 MeasureIntrinsicSize 计算尺寸
            auto* inline_block = static_cast<RenderInlineBlock*>(render_obj);
            auto [w, h] = inline_block->MeasureIntrinsicSize(current_available_width_);

            // 如果 CSS 样式中有显式尺寸，使用 CSS 尺寸
            if (style.width.unit != CSSUnit::AUTO && style.width.unit != CSSUnit::NONE) {
                w = style.width.ToPx(current_available_width_, style.font_size);
            }
            if (style.height.unit != CSSUnit::AUTO && style.height.unit != CSSUnit::NONE) {
                h = style.height.ToPx(0, style.font_size);
            }

            InlineBox box = InlineBox::CreateAtomicBox(render_obj, w, h, h);

            // 应用 margin
            box.margin_left = style.margin.left.ToPx(w, style.font_size);
            box.margin_right = style.margin.right.ToPx(w, style.font_size);

            // 设置行高倍数（用于计算行高）
            box.line_height_multiplier = style.line_height;

            inline_boxes_.push_back(std::move(box));
            break;
        }

        case RenderObjectType::INLINE: {
            // 检查是否是 BR 元素
            auto node = render_obj->GetNode();
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
            float box_left = box.x + box.margin_left + offset_x;
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

    // 应用文本节点的合并边界
    for (auto& [render_obj, bounds] : text_bounds) {
        if (!bounds.has_content) continue;

        LayoutInfo& layout = render_obj->GetLayoutInfo();
        layout.x = bounds.min_x;
        layout.y = bounds.min_y;
        layout.width = bounds.max_x - bounds.min_x;
        layout.height = bounds.max_y - bounds.min_y;
    }

    // 第二遍：应用内联元素的边界
    for (auto& [render_obj, bounds] : inline_bounds) {
        if (!bounds.has_content) continue;

        LayoutInfo& layout = render_obj->GetLayoutInfo();
        layout.x = bounds.min_x;
        layout.y = bounds.min_y;
        layout.width = bounds.max_x - bounds.min_x;
        layout.height = bounds.max_y - bounds.min_y;
    }
}

} // namespace lightui

