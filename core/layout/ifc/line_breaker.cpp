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

    // 第一行应用首行缩进
    bool is_first_line = true;
    float first_line_indent = text_indent_;

    // 创建第一行（考虑首行缩进）
    float effective_width = available_width - (is_first_line ? first_line_indent : 0.0f);
    lines.emplace_back(effective_width);
    LineBox* current_line = &lines.back();

    // 设置首行的起始 x 偏移
    if (is_first_line && first_line_indent > 0) {
        current_line->x = first_line_indent;
    }

    float current_width = 0.0f;

    for (size_t i = 0; i < boxes.size(); ++i) {
        InlineBox& box = boxes[i];
        float box_width = box.GetTotalWidth();

        // 检查是否需要换行
        bool need_break = false;

        if (white_space_ != WhiteSpaceMode::NOWRAP && white_space_ != WhiteSpaceMode::PRE) {
            // 允许换行
            if (!current_line->IsEmpty() && current_width + box_width > effective_width) {
                need_break = true;
            }
            
            // overflow-wrap: break-word - 如果单词溢出容器，允许在单词内部断行
            // 这也处理 word-break: break-word 的情况（已映射到 overflow-wrap: break-word）
            if (overflow_wrap_ == OverflowWrapMode::BREAK_WORD || overflow_wrap_ == OverflowWrapMode::ANYWHERE) {
                // 如果当前行为空但盒子仍然溢出，需要在盒子内部断行
                // 这种情况在文本渲染时处理，这里只标记需要换行
                if (current_line->IsEmpty() && box_width > effective_width) {
                    // 盒子太宽，需要在内部断行（由文本渲染处理）
                    // 这里仍然添加盒子，让渲染器处理溢出
                }
            }
        }

        // 检查强制换行
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
            // 创建新行（不再是第一行）
            is_first_line = false;
            effective_width = available_width;
            lines.emplace_back(effective_width);
            current_line = &lines.back();
            current_width = 0.0f;
        }

        // 添加盒子到当前行
        current_line->AddBox(&box);
        current_width += box_width;

        // 强制换行后创建新行
        if (has_forced_break) {
            // 强制换行后也不是第一行了
            is_first_line = false;
            effective_width = available_width;
            lines.emplace_back(effective_width);
            current_line = &lines.back();
            current_width = 0.0f;
        }
    }

    // 移除末尾空行
    while (!lines.empty() && lines.back().IsEmpty()) {
        lines.pop_back();
    }

    return lines;
}

} // namespace lightui

