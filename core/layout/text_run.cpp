/**
 * @file text_run.cpp
 * @brief TextRun 结构的实现
 */

#include "text_run.h"
#include <cctype>

namespace lightui {

size_t TextRun::CharacterCount() const {
    size_t count = 0;
    size_t i = 0;
    
    while (i < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        
        if ((c & 0x80) == 0) {
            // ASCII 字符 (0xxxxxxx)
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2字节 UTF-8 (110xxxxx)
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3字节 UTF-8 (1110xxxx)
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4字节 UTF-8 (11110xxx)
            i += 4;
        } else {
            // 无效的 UTF-8，跳过一个字节
            i += 1;
        }
        
        count++;
    }
    
    return count;
}

bool TextRun::IsOnlyWhitespace() const {
    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        
        // 检查 ASCII 空白字符
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f') {
            continue;
        }
        
        // 检查多字节 UTF-8 字符（不是空白）
        if (c & 0x80) {
            return false;
        }
        
        // 其他 ASCII 字符不是空白
        return false;
    }
    
    return true;
}

bool TextRun::CanBreakAt(size_t byte_offset) const {
    if (byte_offset == 0 || byte_offset >= text.size()) {
        return false;
    }
    
    // 不能在 UTF-8 多字节序列中间断行
    unsigned char c = static_cast<unsigned char>(text[byte_offset]);
    if ((c & 0xC0) == 0x80) {
        // 这是一个 UTF-8 continuation byte (10xxxxxx)
        return false;
    }
    
    // 检查前一个字符是否是空白
    unsigned char prev = static_cast<unsigned char>(text[byte_offset - 1]);
    if (prev == ' ' || prev == '\t') {
        return true;
    }
    
    // 检查当前字符
    if (c == ' ' || c == '\t') {
        return true;
    }
    
    // 检查是否是 CJK 字符（可以在任意位置断行）
    // CJK 字符范围：U+4E00-U+9FFF, U+3400-U+4DBF, U+20000-U+2A6DF
    // 在 UTF-8 中，这些字符是 3-4 字节
    if ((c & 0xF0) == 0xE0 || (c & 0xF8) == 0xF0) {
        // 可能是 CJK 字符，允许断行
        // TODO: 更精确的 CJK 范围检查
        return true;
    }
    
    return false;
}

} // namespace lightui

