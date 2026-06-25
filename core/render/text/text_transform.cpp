/**
 * @file text_transform.cpp
 * @brief CSS text-transform property implementation
 */

#include "text_transform.h"
#include <cctype>
#include <algorithm>

namespace mblink {

// UTF-8 decoding helper function
// Returns codepoint and number of bytes consumed
static std::pair<uint32_t, int> DecodeUTF8Char(const char* str, size_t len) {
    if (len == 0 || !str) {
        return {0, 0};
    }

    uint8_t first = static_cast<uint8_t>(str[0]);

    // ASCII (0xxxxxxx)
    if ((first & 0x80) == 0) {
        return {first, 1};
    }

    // 2-byte sequence (110xxxxx 10xxxxxx)
    if ((first & 0xE0) == 0xC0 && len >= 2) {
        uint32_t cp = (first & 0x1F) << 6;
        cp |= (static_cast<uint8_t>(str[1]) & 0x3F);
        return {cp, 2};
    }

    // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
    if ((first & 0xF0) == 0xE0 && len >= 3) {
        uint32_t cp = (first & 0x0F) << 12;
        cp |= (static_cast<uint8_t>(str[1]) & 0x3F) << 6;
        cp |= (static_cast<uint8_t>(str[2]) & 0x3F);
        return {cp, 3};
    }

    // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
    if ((first & 0xF8) == 0xF0 && len >= 4) {
        uint32_t cp = (first & 0x07) << 18;
        cp |= (static_cast<uint8_t>(str[1]) & 0x3F) << 12;
        cp |= (static_cast<uint8_t>(str[2]) & 0x3F) << 6;
        cp |= (static_cast<uint8_t>(str[3]) & 0x3F);
        return {cp, 4};
    }

    // Invalid UTF-8 sequence, skip one byte
    return {0xFFFD, 1};  // Replacement character
}

// UTF-8 encoding helper function
// Encodes a codepoint to UTF-8 and appends to result
static void EncodeUTF8Char(std::string& result, uint32_t codepoint) {
    if (codepoint < 0x80) {
        // ASCII
        result += static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
        // 2-byte sequence
        result += static_cast<char>(0xC0 | (codepoint >> 6));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        // 3-byte sequence
        result += static_cast<char>(0xE0 | (codepoint >> 12));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x110000) {
        // 4-byte sequence
        result += static_cast<char>(0xF0 | (codepoint >> 18));
        result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        // Invalid codepoint, use replacement character
        result += "\xEF\xBF\xBD";  // U+FFFD
    }
}

// Check if a codepoint is a whitespace character
static bool IsWhitespace(uint32_t codepoint) {
    return codepoint == ' ' || codepoint == '\t' || codepoint == '\n' || 
           codepoint == '\r' || codepoint == '\f' || codepoint == '\v';
}

// Convert ASCII character to uppercase
static uint32_t ToUpperCodepoint(uint32_t codepoint) {
    // ASCII lowercase letters
    if (codepoint >= 'a' && codepoint <= 'z') {
        return codepoint - 32;
    }
    // For non-ASCII, return as-is (full Unicode case mapping is complex)
    return codepoint;
}

// Convert ASCII character to lowercase
static uint32_t ToLowerCodepoint(uint32_t codepoint) {
    // ASCII uppercase letters
    if (codepoint >= 'A' && codepoint <= 'Z') {
        return codepoint + 32;
    }
    // For non-ASCII, return as-is (full Unicode case mapping is complex)
    return codepoint;
}

std::string ToUpperCase(const std::string& text) {
    if (text.empty()) {
        return text;
    }

    std::string result;
    result.reserve(text.size());

    const char* str = text.c_str();
    size_t len = text.size();
    size_t pos = 0;

    while (pos < len) {
        auto [codepoint, bytes] = DecodeUTF8Char(str + pos, len - pos);
        if (bytes == 0) break;

        uint32_t upper = ToUpperCodepoint(codepoint);
        EncodeUTF8Char(result, upper);
        pos += bytes;
    }

    return result;
}

std::string ToLowerCase(const std::string& text) {
    if (text.empty()) {
        return text;
    }

    std::string result;
    result.reserve(text.size());

    const char* str = text.c_str();
    size_t len = text.size();
    size_t pos = 0;

    while (pos < len) {
        auto [codepoint, bytes] = DecodeUTF8Char(str + pos, len - pos);
        if (bytes == 0) break;

        uint32_t lower = ToLowerCodepoint(codepoint);
        EncodeUTF8Char(result, lower);
        pos += bytes;
    }

    return result;
}

std::string Capitalize(const std::string& text) {
    if (text.empty()) {
        return text;
    }

    std::string result;
    result.reserve(text.size());

    const char* str = text.c_str();
    size_t len = text.size();
    size_t pos = 0;
    bool capitalize_next = true;  // Start of string is like after whitespace

    while (pos < len) {
        auto [codepoint, bytes] = DecodeUTF8Char(str + pos, len - pos);
        if (bytes == 0) break;

        if (IsWhitespace(codepoint)) {
            // Whitespace: output as-is and mark next char for capitalization
            EncodeUTF8Char(result, codepoint);
            capitalize_next = true;
        } else if (capitalize_next) {
            // First character after whitespace: capitalize
            uint32_t upper = ToUpperCodepoint(codepoint);
            EncodeUTF8Char(result, upper);
            capitalize_next = false;
        } else {
            // Not first character: output as-is
            EncodeUTF8Char(result, codepoint);
        }

        pos += bytes;
    }

    return result;
}

std::string TransformText(const std::string& text, const std::string& transform) {
    if (text.empty() || transform.empty() || transform == "none") {
        return text;
    }

    if (transform == "uppercase") {
        return ToUpperCase(text);
    } else if (transform == "lowercase") {
        return ToLowerCase(text);
    } else if (transform == "capitalize") {
        return Capitalize(text);
    }

    // Unknown transform value, return original text
    return text;
}

} // namespace mblink
