/**
 * @file line_breaker.cpp
 * @brief 断行器实现
 */

#include "line_breaker.h"
#include "ifc_layout.h"
#include <algorithm>
#include <cctype>

namespace lightui {

// ========== UTF-8 解码 ==========

uint32_t LineBreaker::DecodeUTF8(const std::string& str, size_t& pos) {
    if (pos >= str.size()) return 0;

    unsigned char c = static_cast<unsigned char>(str[pos]);
    uint32_t cp = 0;

    if ((c & 0x80) == 0) {
        // ASCII (0xxxxxxx)
        cp = c;
        pos += 1;
    } else if ((c & 0xE0) == 0xC0) {
        // 2字节 (110xxxxx)
        if (pos + 1 < str.size()) {
            cp = (c & 0x1F) << 6;
            cp |= (static_cast<unsigned char>(str[pos + 1]) & 0x3F);
            pos += 2;
        } else {
            pos += 1;
        }
    } else if ((c & 0xF0) == 0xE0) {
        // 3字节 (1110xxxx)
        if (pos + 2 < str.size()) {
            cp = (c & 0x0F) << 12;
            cp |= (static_cast<unsigned char>(str[pos + 1]) & 0x3F) << 6;
            cp |= (static_cast<unsigned char>(str[pos + 2]) & 0x3F);
            pos += 3;
        } else {
            pos += 1;
        }
    } else if ((c & 0xF8) == 0xF0) {
        // 4字节 (11110xxx)
        if (pos + 3 < str.size()) {
            cp = (c & 0x07) << 18;
            cp |= (static_cast<unsigned char>(str[pos + 1]) & 0x3F) << 12;
            cp |= (static_cast<unsigned char>(str[pos + 2]) & 0x3F) << 6;
            cp |= (static_cast<unsigned char>(str[pos + 3]) & 0x3F);
            pos += 4;
        } else {
            pos += 1;
        }
    } else {
        // 无效 UTF-8
        pos += 1;
    }

    return cp;
}

// ========== Unicode 字符分类 ==========

bool LineBreaker::IsBreakableWhitespace(uint32_t ch) {
    // 空格、制表符等
    return ch == ' ' || ch == '\t' || ch == 0x00A0;  // 0x00A0 = NBSP
}

bool LineBreaker::IsCJK(uint32_t ch) {
    // CJK 统一表意文字
    if (ch >= 0x4E00 && ch <= 0x9FFF) return true;   // 基本
    if (ch >= 0x3400 && ch <= 0x4DBF) return true;   // 扩展A
    if (ch >= 0x20000 && ch <= 0x2A6DF) return true; // 扩展B
    if (ch >= 0x2A700 && ch <= 0x2B73F) return true; // 扩展C
    if (ch >= 0x2B740 && ch <= 0x2B81F) return true; // 扩展D

    // CJK 符号和标点
    if (ch >= 0x3000 && ch <= 0x303F) return true;

    // 全角 ASCII
    if (ch >= 0xFF00 && ch <= 0xFFEF) return true;

    // 日文假名
    if (ch >= 0x3040 && ch <= 0x309F) return true;  // 平假名
    if (ch >= 0x30A0 && ch <= 0x30FF) return true;  // 片假名

    // 韩文
    if (ch >= 0xAC00 && ch <= 0xD7AF) return true;

    return false;
}

bool LineBreaker::IsLineStartProhibited(uint32_t ch) {
    // 行首禁止字符（不能出现在行首）
    // 中文标点
    if (ch == 0x3002 || ch == 0xFF0C || ch == 0x3001) return true; // 。，、
    if (ch == 0xFF1A || ch == 0xFF1B) return true; // ：；
    if (ch == 0xFF01 || ch == 0xFF1F) return true; // ！？
    if (ch == 0x3009 || ch == 0x300B || ch == 0x300D || ch == 0x300F) return true; // 〉》】」
    if (ch == 0xFF09 || ch == 0xFF3D) return true; // ）］

    // ASCII 标点
    if (ch == ')' || ch == ']' || ch == '}') return true;
    if (ch == '.' || ch == ',' || ch == '!' || ch == '?' || ch == ':' || ch == ';') return true;

    return false;
}

bool LineBreaker::IsLineEndProhibited(uint32_t ch) {
    // 行尾禁止字符（不能出现在行尾）
    // 左括号类
    if (ch == 0x3008 || ch == 0x300A || ch == 0x300C || ch == 0x300E) return true; // 〈《「『
    if (ch == 0xFF08 || ch == 0xFF3B) return true; // （［

    // ASCII
    if (ch == '(' || ch == '[' || ch == '{') return true;

    return false;
}

bool LineBreaker::CanBreakBetween(uint32_t prev_char, uint32_t next_char) {
    // word-break: break-all 允许在任意字符间断行
    if (word_break_ == WordBreakMode::BREAK_ALL) {
        // 仍然尊重行首/行尾禁止字符
        if (IsLineStartProhibited(next_char)) return false;
        if (IsLineEndProhibited(prev_char)) return false;
        // 允许在任意字符间断行
        return true;
    }

    // 行首禁止字符不能出现在断行后
    if (IsLineStartProhibited(next_char)) return false;

    // 行尾禁止字符不能出现在断行前
    if (IsLineEndProhibited(prev_char)) return false;

    // 空白后可以断行
    if (IsBreakableWhitespace(prev_char)) return true;

    // 连字符后可以断行（CSS 默认行为）
    // U+002D: HYPHEN-MINUS (-)
    // U+2010: HYPHEN (‐)
    // U+2011: NON-BREAKING HYPHEN (不断行)
    // U+2012: FIGURE DASH (‒)
    // U+2013: EN DASH (–)
    if (prev_char == '-' || prev_char == 0x2010 || prev_char == 0x2012 || prev_char == 0x2013) {
        return true;
    }

    // word-break: keep-all 阻止 CJK 文本内部断行
    if (word_break_ == WordBreakMode::KEEP_ALL) {
        // CJK 字符间不允许断行
        if (IsCJK(prev_char) && IsCJK(next_char)) return false;
        // CJK 和其他字符间也不允许断行
        if (IsCJK(prev_char) || IsCJK(next_char)) return false;
    } else {
        // word-break: normal - CJK 字符间可以断行
        if (IsCJK(prev_char) && IsCJK(next_char)) return true;
        // CJK 和其他字符间可以断行
        if (IsCJK(prev_char) || IsCJK(next_char)) return true;
    }

    // 默认不断行（英文单词内部）
    return false;
}

// ========== 空白处理 ==========

std::string LineBreaker::ProcessWhitespace(const std::string& text) {
    if (text.empty()) return text;

    switch (white_space_) {
        case WhiteSpaceMode::PRE:
        case WhiteSpaceMode::PRE_WRAP:
            // 保留所有空白
            return text;

        case WhiteSpaceMode::PRE_LINE: {
            // 合并空格和制表符，保留换行符
            std::string result;
            result.reserve(text.size());
            bool in_space = false;

            for (char c : text) {
                if (c == '\n') {
                    result += c;
                    in_space = false;
                } else if (c == ' ' || c == '\t') {
                    if (!in_space) {
                        result += ' ';
                        in_space = true;
                    }
                } else {
                    result += c;
                    in_space = false;
                }
            }
            return result;
        }

        case WhiteSpaceMode::NORMAL:
        case WhiteSpaceMode::NOWRAP:
        default: {
            // 合并所有空白为单个空格
            std::string result;
            result.reserve(text.size());
            bool in_space = false;

            for (char c : text) {
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    if (!in_space) {
                        result += ' ';
                        in_space = true;
                    }
                } else {
                    result += c;
                    in_space = false;
                }
            }
            return result;
        }
    }
}

// ========== 断行查找 ==========

std::vector<BreakOpportunity> LineBreaker::FindBreakOpportunities(
    const std::vector<InlineBox>& boxes,
    float available_width
) {
    std::vector<BreakOpportunity> opportunities;
    float current_width = 0.0f;

    for (size_t i = 0; i < boxes.size(); ++i) {
        const InlineBox& box = boxes[i];

        // 盒子边界是断行机会
        if (i > 0 && box.IsText()) {
            BreakOpportunity op;
            op.box_index = i;
            op.text_offset = 0;
            op.width_before = current_width;
            op.type = BreakType::NORMAL;
            opportunities.push_back(op);
        }

        // 文本内部的断行机会
        if (box.IsText()) {
            for (const auto& run : box.text_runs) {
                // 强制换行
                if (run.is_forced_break) {
                    BreakOpportunity op;
                    op.box_index = i;
                    op.text_offset = run.end_offset;
                    op.width_before = current_width + run.width;
                    op.type = BreakType::FORCED;
                    opportunities.push_back(op);
                }

                // 查找文本内的断行点
                if (!run.text.empty() && white_space_ != WhiteSpaceMode::NOWRAP
                    && white_space_ != WhiteSpaceMode::PRE) {
                    size_t pos = 0;
                    uint32_t prev_char = 0;
                    float char_width = run.width / std::max(1.0f, static_cast<float>(run.CharacterCount()));
                    float text_width = 0;

                    while (pos < run.text.size()) {
                        size_t start_pos = pos;
                        uint32_t ch = DecodeUTF8(run.text, pos);

                        if (prev_char != 0 && CanBreakBetween(prev_char, ch)) {
                            BreakOpportunity op;
                            op.box_index = i;
                            op.text_offset = run.start_offset + start_pos;
                            op.width_before = current_width + text_width;
                            op.type = BreakType::NORMAL;
                            opportunities.push_back(op);
                        }

                        text_width += char_width;
                        prev_char = ch;
                    }
                }
            }
        }

        current_width += box.GetTotalWidth();
    }

    return opportunities;
}

// ========== 执行断行 ==========

static bool IsCollapsibleWhitespaceMode(WhiteSpaceMode mode) {
    return mode == WhiteSpaceMode::NORMAL ||
           mode == WhiteSpaceMode::NOWRAP ||
           mode == WhiteSpaceMode::PRE_LINE;
}

static bool IsWhitespaceOnlyTextBox(const InlineBox& box, WhiteSpaceMode mode) {
    if (!box.IsText() || !IsCollapsibleWhitespaceMode(mode) || box.text_runs.empty()) {
        return false;
    }

    for (const auto& run : box.text_runs) {
        if (run.is_forced_break || !run.IsOnlyWhitespace()) {
            return false;
        }
    }
    return true;
}

static bool HasForcedBreakTextRun(const InlineBox& box) {
    if (!box.IsText()) {
        return false;
    }

    for (const auto& run : box.text_runs) {
        if (run.is_forced_break) {
            return true;
        }
    }
    return false;
}

static bool LineHasVisibleContent(const LineBox& line, WhiteSpaceMode mode) {
    for (auto* box : line.boxes) {
        if (!box) continue;
        if (box->IsInlineStart() || box->IsInlineEnd()) continue;
        if (IsWhitespaceOnlyTextBox(*box, mode)) continue;
        return true;
    }
    return false;
}

static bool TrimLeadingCollapsibleWhitespaceFromTextBox(InlineBox& box, WhiteSpaceMode mode) {
    if (!box.IsText() || !IsCollapsibleWhitespaceMode(mode) || box.text_runs.empty() || !box.style) {
        return false;
    }

    TextRun& run = box.text_runs[0];
    if (run.is_forced_break || run.text.empty()) {
        return false;
    }

    size_t trim_len = 0;
    while (trim_len < run.text.size()) {
        char c = run.text[trim_len];
        if (c == ' ' || c == '\t') {
            ++trim_len;
            continue;
        }
        break;
    }

    if (trim_len == 0) {
        return false;
    }

    const ComputedStyle& style = *box.style;
    std::string trimmed_text = run.text.substr(trim_len);
    if (trimmed_text.empty()) {
        run.text.clear();
        run.start_offset += trim_len;
        run.end_offset = run.start_offset;
        run.width = 0.0f;
        run.height = style.font_size * style.line_height;
        run.baseline = style.font_size * 0.8f;
        run.is_whitespace = true;
        box.width = 0.0f;
        return true;
    }

    auto measurement = IFCLayout::MeasureTextStatic(
        trimmed_text,
        style.font_size,
        style.font_family,
        style.letter_spacing.ToPx(0, style.font_size),
        style.word_spacing.ToPx(0, style.font_size),
        style.line_height,
        style.font_weight,
        style.font_style);

    run.text = trimmed_text;
    run.start_offset += trim_len;
    run.end_offset = run.start_offset + trimmed_text.size();
    run.width = measurement.width;
    run.height = measurement.height;
    run.baseline = measurement.skia_ascent;
    run.is_whitespace = run.IsOnlyWhitespace();

    box.width = measurement.width;
    box.height = measurement.height;
    box.baseline = measurement.skia_ascent;
    box.skia_ascent = measurement.skia_ascent;
    box.skia_descent = measurement.skia_descent;
    return true;
}

static void TrimTrailingCollapsibleWhitespace(LineBox* line, float& current_width, WhiteSpaceMode mode) {
    if (!line || !IsCollapsibleWhitespaceMode(mode)) return;

    while (!line->boxes.empty()) {
        InlineBox* last = line->boxes.back();
        if (!last || !IsWhitespaceOnlyTextBox(*last, mode)) {
            break;
        }

        float removed_width = last->GetTotalWidth();
        line->boxes.pop_back();
        line->content_width = std::max(0.0f, line->content_width - removed_width);
        current_width = std::max(0.0f, current_width - removed_width);
    }
}

std::vector<LineBox> LineBreaker::BreakIntoLines(
    std::vector<InlineBox>& boxes,
    float available_width
) {
    std::vector<LineBox> lines;
    if (boxes.empty()) return lines;

    // 可能在断行时对文本盒做细粒度切分，预留容量避免插入后指针失效
    boxes.reserve(boxes.size() + 8192);

    bool is_first_line = true;
    float first_line_indent = text_indent_;

    float effective_width = available_width - (is_first_line ? first_line_indent : 0.0f);
    lines.emplace_back(effective_width);
    LineBox* current_line = &lines.back();

    if (is_first_line && first_line_indent > 0) {
        current_line->x = first_line_indent;
    }

    float current_width = 0.0f;

    for (size_t i = 0; i < boxes.size(); ++i) {
        InlineBox& box = boxes[i];

        if (box.IsInlineStart() || box.IsInlineEnd()) {
            current_line->AddBox(&box);
            current_width += box.GetTotalWidth();
            continue;
        }

        bool is_leading_collapsible_ws =
            IsWhitespaceOnlyTextBox(box, white_space_) &&
            !LineHasVisibleContent(*current_line, white_space_);
        if (is_leading_collapsible_ws) {
            continue;
        }

        if (!LineHasVisibleContent(*current_line, white_space_)) {
            TrimLeadingCollapsibleWhitespaceFromTextBox(box, white_space_);
            if (box.IsText() && box.text_runs.empty()) {
                continue;
            }
            // 修复原因：CodeMirror 等编辑器会用 <br>/forced break 表示空行。
            // 这里不能把 text.empty() 的 forced break 盒子当成可折叠空白直接丢掉，
            // 否则正文行盒数量会少于 gutter 行号数量，表现为空行不显示、
            // 两位数行号阶段的错位闪烁/重影更明显。
            if (box.IsText() && !box.text_runs.empty() && box.text_runs[0].text.empty() && !HasForcedBreakTextRun(box)) {
                continue;
            }
        }


        bool box_was_split = false;

        // 字符级切分：当文本盒在当前行放不下时，尝试在合法断点切分为前后两段
        if (box.IsText() &&
            white_space_ != WhiteSpaceMode::NOWRAP && white_space_ != WhiteSpaceMode::PRE &&
            current_width + box.GetTotalWidth() > effective_width && !box.text_runs.empty()) {

            TextRun& run = box.text_runs[0];
            const std::string& text = run.text;
            if (!text.empty() && box.style) {
                float content_limit = effective_width - current_width - box.GetLeftSpace() - box.GetRightSpace();
                if (content_limit > 0.0f && run.width > content_limit) {
                    const ComputedStyle& style = *box.style;
                    size_t pos = 0;
                    size_t prev_pos = 0;
                    uint32_t prev_char = 0;
                    size_t best_break_byte = 0;
                    TextMeasureResult best_head_measure;
                    bool found_break = false;

                    while (pos < text.size()) {
                        prev_pos = pos;
                        uint32_t ch = DecodeUTF8(text, pos);

                        if (prev_char != 0 && CanBreakBetween(prev_char, ch)) {
                            std::string candidate = text.substr(0, prev_pos);
                            auto candidate_measure = IFCLayout::MeasureTextStatic(
                                candidate,
                                style.font_size,
                                style.font_family,
                                letter_spacing_,
                                word_spacing_,
                                style.line_height,
                                style.font_weight,
                                style.font_style);
                            if (candidate_measure.width <= content_limit) {
                                best_break_byte = prev_pos;
                                best_head_measure = candidate_measure;
                                found_break = true;
                            } else {
                                break;
                            }
                        }

                        prev_char = ch;
                    }

                    bool allow_anywhere = (overflow_wrap_ == OverflowWrapMode::ANYWHERE ||
                                           overflow_wrap_ == OverflowWrapMode::BREAK_WORD ||
                                           word_break_ == WordBreakMode::BREAK_ALL);

                    if (!found_break && allow_anywhere) {
                        pos = 0;
                        size_t fallback_prev_pos = 0;
                        while (pos < text.size()) {
                            fallback_prev_pos = pos;
                            DecodeUTF8(text, pos);
                            if (fallback_prev_pos == 0) {
                                continue;
                            }

                            std::string candidate = text.substr(0, fallback_prev_pos);
                            auto candidate_measure = IFCLayout::MeasureTextStatic(
                                candidate,
                                style.font_size,
                                style.font_family,
                                letter_spacing_,
                                word_spacing_,
                                style.line_height,
                                style.font_weight,
                                style.font_style);
                            if (candidate_measure.width <= content_limit) {
                                best_break_byte = fallback_prev_pos;
                                best_head_measure = candidate_measure;
                                found_break = true;
                            } else {
                                break;
                            }
                        }
                    }

                    if (found_break && best_break_byte > 0 && best_break_byte < text.size()) {
                        InlineBox tail = box;
                        std::string head_text = text.substr(0, best_break_byte);
                        std::string tail_text = text.substr(best_break_byte);

                        auto tail_measure = IFCLayout::MeasureTextStatic(
                            tail_text,
                            style.font_size,
                            style.font_family,
                            letter_spacing_,
                            word_spacing_,
                            style.line_height,
                            style.font_weight,
                            style.font_style);

                        run.text = head_text;
                        run.end_offset = run.start_offset + best_break_byte;
                        run.width = best_head_measure.width;
                        run.height = best_head_measure.height;
                        run.baseline = best_head_measure.skia_ascent;
                        run.is_whitespace = run.IsOnlyWhitespace();
                        box.width = best_head_measure.width;
                        box.height = best_head_measure.height;
                        box.baseline = best_head_measure.skia_ascent;
                        box.skia_ascent = best_head_measure.skia_ascent;
                        box.skia_descent = best_head_measure.skia_descent;

                        tail.text_runs.clear();
                        TextRun tail_run = run;
                        tail_run.text = tail_text;
                        tail_run.start_offset = run.end_offset;
                        tail_run.end_offset = tail_run.start_offset + tail_text.size();
                        tail_run.width = tail_measure.width;
                        tail_run.height = tail_measure.height;
                        tail_run.baseline = tail_measure.skia_ascent;
                        tail_run.is_forced_break = false;
                        tail_run.is_whitespace = tail_run.IsOnlyWhitespace();
                        tail.text_runs.push_back(tail_run);
                        tail.width = tail_measure.width;
                        tail.height = tail_measure.height;
                        tail.baseline = tail_measure.skia_ascent;
                        tail.skia_ascent = tail_measure.skia_ascent;
                        tail.skia_descent = tail_measure.skia_descent;

                        boxes.insert(boxes.begin() + static_cast<long long>(i + 1), std::move(tail));
                        box_was_split = true;
                    }
                }
            }
        }

        float box_width = box.GetTotalWidth();
        bool need_break = false;

        if (white_space_ != WhiteSpaceMode::NOWRAP && white_space_ != WhiteSpaceMode::PRE) {
            if (!current_line->IsEmpty() && current_width + box_width > effective_width) {
                need_break = true;
            }
        }

        bool has_forced_break = false;
        if (box.IsText()) {
            for (const auto& run : box.text_runs) {
                if (run.is_forced_break) {
                    has_forced_break = true;
                    break;
                }
            }
        }

        if (need_break) {
            TrimTrailingCollapsibleWhitespace(current_line, current_width, white_space_);
            is_first_line = false;
            effective_width = available_width;
            lines.emplace_back(effective_width);
            current_line = &lines.back();
            current_width = 0.0f;

            if (IsWhitespaceOnlyTextBox(box, white_space_) &&
                !LineHasVisibleContent(*current_line, white_space_)) {
                continue;
            }
        }

        current_line->AddBox(&box);
        current_width += box_width;

        if (box_was_split) {
            TrimTrailingCollapsibleWhitespace(current_line, current_width, white_space_);
            is_first_line = false;
            effective_width = available_width;
            lines.emplace_back(effective_width);
            current_line = &lines.back();
            current_width = 0.0f;
            continue;
        }

        if (has_forced_break) {
            TrimTrailingCollapsibleWhitespace(current_line, current_width, white_space_);
            is_first_line = false;
            effective_width = available_width;
            lines.emplace_back(effective_width);
            current_line = &lines.back();
            current_width = 0.0f;
        }
    }

    if (!lines.empty()) {
        TrimTrailingCollapsibleWhitespace(&lines.back(), current_width, white_space_);
    }

    // 修复原因：显式换行（如 CodeMirror 空行对应的 <br>）会在 forced break 后
    // 追加一个新的空 LineBox，用于承接后续内容。这里只裁掉“最后追加出来、
    // 且没有任何盒子”的尾随空行，保留前面真正承载 forced break 语义的空行盒。
    while (lines.size() >= 2 && lines.back().IsEmpty()) {
        lines.pop_back();
    }


    return lines;
}

} // namespace lightui

