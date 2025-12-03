/**
 * @file ifc_layout.cpp
 * @brief IFC 布局入口实现
 */

#include "ifc_layout.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

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

// ========== 文本测量 ==========

std::pair<float, float> MeasureTextForIFC(
    const std::string& text,
    float font_size,
    const std::string& font_family,
    float letter_spacing = 0.0f,
    float word_spacing = 0.0f
) {
    (void)font_family; // TODO: 使用字体信息

    if (text.empty()) {
        return {0.0f, font_size * 1.2f};
    }

    // 简化的文本测量
    // TODO: 集成 TextRenderer 或 Skia 进行精确测量
    float char_width = font_size * 0.5f;  // 平均字符宽度估计
    float width = 0.0f;
    int char_count = 0;  // 字符计数（用于 letter-spacing）
    int word_count = 0;  // 单词计数（用于 word-spacing）
    bool in_word = false;

    // 计算 UTF-8 字符数
    size_t pos = 0;
    while (pos < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[pos]);

        if ((c & 0x80) == 0) {
            // ASCII
            if (c == ' ' || c == '\t') {
                width += font_size * 0.25f;  // 空格较窄
                // 单词结束，计数
                if (in_word) {
                    word_count++;
                    in_word = false;
                }
            } else {
                width += char_width;
                in_word = true;
            }
            char_count++;
            pos += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2字节
            width += font_size * 1.0f;  // CJK 等宽字符
            char_count++;
            in_word = true;
            pos += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3字节（大部分中文）
            width += font_size * 1.0f;
            char_count++;
            in_word = true;
            pos += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4字节
            width += font_size * 1.0f;
            char_count++;
            in_word = true;
            pos += 4;
        } else {
            pos += 1;
        }
    }

    // 应用 letter-spacing：在每个字符之间添加间距（最后一个字符后面不加）
    if (char_count > 1 && letter_spacing != 0.0f) {
        width += letter_spacing * (char_count - 1);
    }

    // 应用 word-spacing：在每个空格处添加额外间距
    // 统计空格数量
    int space_count = 0;
    for (char ch : text) {
        if (ch == ' ') {
            space_count++;
        }
    }
    if (space_count > 0 && word_spacing != 0.0f) {
        width += word_spacing * space_count;
    }

    float height = font_size * 1.2f;  // 行高通常是字体大小的 1.2 倍
    return {width, height};
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

    // 检查缓存
    if (IsCacheValid(container, available_width)) {
        const auto& cache = cache_[container];
        line_boxes_ = cache.line_boxes;
        content_height_ = cache.content_height;
        content_width_ = cache.content_width;

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
    const auto& style = container->GetComputedStyle();

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

        // 计算行度量
        auto line_metrics = vertical_aligner_.CalculateLineMetrics(box_ptrs, aligns);
        line.height = line_metrics.line_height;
        line.baseline = line_metrics.baseline;

        // 设置行位置
        line.y = current_y;

        // 首先设置每个盒子的水平位置
        float current_x = line.x;  // 行的起始 x 位置（可能有 text-indent）
        for (auto* box : line.boxes) {
            if (!box) continue;
            box->x = current_x + box->margin_left;
            current_x += box->GetTotalWidth();
        }

        // 应用垂直对齐
        vertical_aligner_.AlignBoxes(box_ptrs, aligns, current_y);

        // 应用水平对齐（调整 x 坐标）
        line.ApplyTextAlign(style.text_align);

        // 更新统计
        current_y += line_metrics.line_height;
        content_width_ = std::max(content_width_, line.content_width);
    }

    content_height_ = current_y;

    // 5. 应用布局结果到渲染对象
    ApplyLayoutResults(container);

    // 6. 更新缓存
    LayoutCache& cache = cache_[container];
    cache.available_width = available_width;
    cache.content_height = content_height_;
    cache.content_width = content_width_;
    cache.line_boxes = line_boxes_;
    cache.content_version = GetContentVersion(container);
    cache.valid = true;

    // 填充结果
    result.total_height = content_height_;
    result.max_width = content_width_;
    result.line_count = line_boxes_.size();
    result.success = true;

    return result;
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

            auto [width, height] = MeasureTextForIFC(
                text, style.font_size, style.font_family, letter_spacing, word_spacing);

            InlineBox box = InlineBox::CreateTextBox(render_obj);
            box.width = width;
            box.height = height;
            box.baseline = style.font_size * 0.8f;
            box.line_height_multiplier = style.line_height;  // 设置行高倍数

            // 添加 TextRun
            TextRun run;
            run.text = text;
            run.start_offset = 0;
            run.end_offset = text.size();
            run.width = width;
            run.height = height;
            run.baseline = style.font_size * 0.8f;
            box.text_runs.push_back(run);

            inline_boxes_.push_back(std::move(box));
            break;
        }

        case RenderObjectType::INLINE_BLOCK: {
            // 原子内联元素
            // 优先使用 CSS 样式中的尺寸，其次使用已计算的布局尺寸
            float w = 100.0f;  // 默认宽度
            float h = 20.0f;   // 默认高度

            // 从 CSS 样式获取尺寸
            if (style.width.unit != CSSUnit::AUTO) {
                w = style.width.ToPx(current_available_width_, style.font_size);
            } else {
                const auto& layout = render_obj->GetLayoutInfo();
                if (layout.width > 0) {
                    w = layout.width;
                }
            }

            if (style.height.unit != CSSUnit::AUTO) {
                h = style.height.ToPx(0, style.font_size);
            } else {
                const auto& layout = render_obj->GetLayoutInfo();
                if (layout.height > 0) {
                    h = layout.height;
                }
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
            // 内联元素 - 递归处理子元素
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
    return MeasureTextForIFC(text, style.font_size, style.font_family, letter_spacing, word_spacing);
}

// ========== 应用布局结果 ==========

void IFCLayout::ApplyLayoutResults(RenderObject* container) {
    if (!container) return;

    // 用于跟踪内联元素的边界
    struct InlineElementBounds {
        float min_x = std::numeric_limits<float>::max();
        float min_y = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float max_y = std::numeric_limits<float>::lowest();
        bool has_content = false;
    };
    std::unordered_map<RenderObject*, InlineElementBounds> inline_bounds;

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
            float box_left = box.x + box.margin_left;
            float box_right = box_left + box.width;
            float box_top = box.y;
            float box_bottom = box.y + box.height;

            // 更新自身的布局信息
            LayoutInfo& layout = render_obj->GetLayoutInfo();
            layout.x = box_left;
            layout.y = box_top;
            layout.width = box.width;
            layout.height = box.height;

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

