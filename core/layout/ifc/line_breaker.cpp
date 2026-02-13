/**
 * @file line_breaker.cpp
 * @brief 断行器实现
 */

#include "line_breaker.h"
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

        // 字符级切分：当文本盒在当前行放不下时，尝试在合法断点切分为前后两段
        if (box.IsText() && !current_line->IsEmpty() &&
            white_space_ != WhiteSpaceMode::NOWRAP && white_space_ != WhiteSpaceMode::PRE &&
            current_width + box.GetTotalWidth() > effective_width && !box.text_runs.empty()) {

            TextRun& run = box.text_runs[0];
            const std::string& text = run.text;
            if (!text.empty()) {
                float content_limit = effective_width - current_width - box.GetLeftSpace() - box.GetRightSpace();
                if (content_limit > 0.0f && run.width > content_limit) {
                    size_t total_chars = std::max<size_t>(1, run.CharacterCount());
                    float avg_char_width = run.width / static_cast<float>(total_chars);

                    size_t pos = 0;
                    size_t prev_pos = 0;
                    uint32_t prev_char = 0;
                    float used_width = 0.0f;
                    size_t used_chars = 0;
                    size_t best_break_byte = 0;
                    size_t best_break_chars = 0;

                    while (pos < text.size()) {
                        prev_pos = pos;
                        uint32_t ch = DecodeUTF8(text, pos);

                        float next_w = used_width + avg_char_width;
                        if (next_w > content_limit && used_chars > 0) {
                            break;
                        }

                        used_width = next_w;
                        used_chars++;

                        if (prev_char != 0 && CanBreakBetween(prev_char, ch)) {
                            best_break_byte = prev_pos;
                            best_break_chars = used_chars - 1;
                        }

                        prev_char = ch;
                    }

                    bool allow_anywhere = (overflow_wrap_ == OverflowWrapMode::ANYWHERE ||
                                           overflow_wrap_ == OverflowWrapMode::BREAK_WORD ||
                                           word_break_ == WordBreakMode::BREAK_ALL);

                    size_t split_byte = best_break_byte;
                    size_t split_chars = best_break_chars;

                    if (split_byte == 0 && allow_anywhere && used_chars > 0) {
                        split_byte = prev_pos;
                        split_chars = used_chars;
                    }

                    if (split_byte > 0 && split_byte < text.size()) {
                        InlineBox tail = box;

                        std::string head_text = text.substr(0, split_byte);
                        std::string tail_text = text.substr(split_byte);

                        float head_w = avg_char_width * static_cast<float>(split_chars);
                        if (head_w <= 0.0f) head_w = std::max(0.0f, used_width);
                        head_w = std::min(head_w, run.width);
                        float tail_w = std::max(0.0f, run.width - head_w);

                        run.text = head_text;
                        run.end_offset = run.start_offset + split_byte;
                        run.width = head_w;
                        box.width = head_w;

                        tail.text_runs.clear();
                        TextRun tail_run = run;
                        tail_run.text = tail_text;
                        tail_run.start_offset = run.end_offset;
                        tail_run.end_offset = tail_run.start_offset + tail_text.size();
                        tail_run.width = tail_w;
                        tail_run.is_forced_break = false;
                        tail.text_runs.push_back(tail_run);
                        tail.width = tail_w;

                        boxes.insert(boxes.begin() + static_cast<long long>(i + 1), std::move(tail));
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
            is_first_line = false;
            effective_width = available_width;
            lines.emplace_back(effective_width);
            current_line = &lines.back();
            current_width = 0.0f;
        }

        current_line->AddBox(&box);
        current_width += box_width;

        if (has_forced_break) {
            is_first_line = false;
            effective_width = available_width;
            lines.emplace_back(effective_width);
            current_line = &lines.back();
            current_width = 0.0f;
        }
    }

    while (!lines.empty() && lines.back().IsEmpty()) {
        lines.pop_back();
    }

    return lines;
}

} // namespace lightui

