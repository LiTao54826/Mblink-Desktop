/**
 * @file utf8_utils.h
 * @brief UTF-8 字符串处理工具函数
 * 
 * 提供对UTF-8编码字符串的正确处理，包括：
 * - 字符计数（而不是字节计数）
 * - 删除单个字符（支持多字节字符如中文）
 * - 光标移动
 */

#pragma once

#include <string>
#include <cstdint>

namespace mbink {
namespace utf8 {

/**
 * @brief 获取UTF-8字符的字节长度
 * @param lead_byte 首字节
 * @return 字符占用的字节数（1-4），如果无效返回1
 */
inline int GetCharByteLength(unsigned char lead_byte) {
    if ((lead_byte & 0x80) == 0) {
        // ASCII: 0xxxxxxx
        return 1;
    } else if ((lead_byte & 0xE0) == 0xC0) {
        // 2字节: 110xxxxx
        return 2;
    } else if ((lead_byte & 0xF0) == 0xE0) {
        // 3字节: 1110xxxx (中文字符)
        return 3;
    } else if ((lead_byte & 0xF8) == 0xF0) {
        // 4字节: 11110xxx (emoji等)
        return 4;
    }
    // 无效或续接字节
    return 1;
}

/**
 * @brief 检查是否是UTF-8续接字节（10xxxxxx）
 * @param byte 字节
 * @return true如果是续接字节
 */
inline bool IsContinuationByte(unsigned char byte) {
    return (byte & 0xC0) == 0x80;
}

/**
 * @brief 计算UTF-8字符串中的字符数（不是字节数）
 * @param str UTF-8编码的字符串
 * @return 字符数
 */
inline size_t CharCount(const std::string& str) {
    size_t count = 0;
    for (size_t i = 0; i < str.size(); ) {
        int len = GetCharByteLength(static_cast<unsigned char>(str[i]));
        i += len;
        count++;
    }
    return count;
}

/**
 * @brief 将字符位置转换为字节位置
 * @param str UTF-8编码的字符串
 * @param char_pos 字符位置
 * @return 对应的字节位置
 */
inline size_t CharPosToBytePos(const std::string& str, size_t char_pos) {
    size_t byte_pos = 0;
    size_t current_char = 0;
    
    while (byte_pos < str.size() && current_char < char_pos) {
        int len = GetCharByteLength(static_cast<unsigned char>(str[byte_pos]));
        byte_pos += len;
        current_char++;
    }
    
    return byte_pos;
}

/**
 * @brief 将字节位置转换为字符位置
 * @param str UTF-8编码的字符串
 * @param byte_pos 字节位置
 * @return 对应的字符位置
 */
inline size_t BytePosToCharPos(const std::string& str, size_t byte_pos) {
    size_t current_byte = 0;
    size_t char_count = 0;
    
    while (current_byte < byte_pos && current_byte < str.size()) {
        int len = GetCharByteLength(static_cast<unsigned char>(str[current_byte]));
        current_byte += len;
        char_count++;
    }
    
    return char_count;
}

/**
 * @brief 获取前一个字符的起始字节位置
 * @param str UTF-8编码的字符串
 * @param byte_pos 当前字节位置
 * @return 前一个字符的起始字节位置，如果已经在开头则返回0
 */
inline size_t PrevCharBytePos(const std::string& str, size_t byte_pos) {
    if (byte_pos == 0 || str.empty()) {
        return 0;
    }
    
    // 向前移动，跳过续接字节
    size_t pos = byte_pos - 1;
    while (pos > 0 && IsContinuationByte(static_cast<unsigned char>(str[pos]))) {
        pos--;
    }
    
    return pos;
}

/**
 * @brief 获取下一个字符的起始字节位置
 * @param str UTF-8编码的字符串
 * @param byte_pos 当前字节位置
 * @return 下一个字符的起始字节位置，如果已经在末尾则返回str.size()
 */
inline size_t NextCharBytePos(const std::string& str, size_t byte_pos) {
    if (byte_pos >= str.size()) {
        return str.size();
    }
    
    int len = GetCharByteLength(static_cast<unsigned char>(str[byte_pos]));
    return std::min(byte_pos + len, str.size());
}

/**
 * @brief 在指定字符位置删除一个字符（向前删除，如Backspace）
 * @param str UTF-8编码的字符串（会被修改）
 * @param char_pos 字符位置（不是字节位置）
 * @return 新的字符位置
 */
inline size_t DeleteCharBefore(std::string& str, size_t char_pos) {
    if (char_pos == 0 || str.empty()) {
        return char_pos;
    }
    
    // 转换为字节位置
    size_t byte_pos = CharPosToBytePos(str, char_pos);
    size_t prev_byte_pos = CharPosToBytePos(str, char_pos - 1);
    
    // 删除字符
    str.erase(prev_byte_pos, byte_pos - prev_byte_pos);
    
    return char_pos - 1;
}

/**
 * @brief 在指定字符位置删除一个字符（向后删除，如Delete键）
 * @param str UTF-8编码的字符串（会被修改）
 * @param char_pos 字符位置（不是字节位置）
 * @return 字符位置（不变）
 */
inline size_t DeleteCharAfter(std::string& str, size_t char_pos) {
    size_t char_count = CharCount(str);
    if (char_pos >= char_count || str.empty()) {
        return char_pos;
    }
    
    // 转换为字节位置
    size_t byte_pos = CharPosToBytePos(str, char_pos);
    size_t next_byte_pos = CharPosToBytePos(str, char_pos + 1);
    
    // 删除字符
    str.erase(byte_pos, next_byte_pos - byte_pos);
    
    return char_pos;
}

/**
 * @brief 在指定字符位置插入文本
 * @param str UTF-8编码的字符串（会被修改）
 * @param char_pos 字符位置（不是字节位置）
 * @param text 要插入的文本
 * @return 新的字符位置（插入文本之后）
 */
inline size_t InsertText(std::string& str, size_t char_pos, const std::string& text) {
    size_t byte_pos = CharPosToBytePos(str, char_pos);
    str.insert(byte_pos, text);
    return char_pos + CharCount(text);
}

/**
 * @brief 获取子字符串（按字符位置，不是字节位置）
 * @param str UTF-8编码的字符串
 * @param start_char 起始字符位置
 * @param end_char 结束字符位置（不包含）
 * @return 子字符串
 */
inline std::string SubstrByChar(const std::string& str, size_t start_char, size_t end_char) {
    size_t start_byte = CharPosToBytePos(str, start_char);
    size_t end_byte = CharPosToBytePos(str, end_char);
    return str.substr(start_byte, end_byte - start_byte);
}

} // namespace utf8
} // namespace mbink

