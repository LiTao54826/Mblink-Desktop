/**
 * @file ansi_parser.cpp
 * @brief ANSI 转义序列解析器实现
 */

#include "ansi_parser.h"

#include <algorithm>
#include <cctype>

namespace mbink {

AnsiParser::AnsiParser() {
    current_style_.Reset();
}

void AnsiParser::Parse(const char* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        ProcessByte(static_cast<uint8_t>(data[i]));
    }
}

void AnsiParser::Reset() {
    state_ = ParserState::Ground;
    current_style_.Reset();
    csi_params_.clear();
    osc_string_.clear();
    utf8_buffer_.clear();
}

void AnsiParser::ProcessByte(uint8_t byte) {
    // 处理 UTF-8 多字节序列
    if (!utf8_buffer_.empty()) {
        if (IsUtf8Continuation(byte)) {
            utf8_buffer_.push_back(static_cast<char>(byte));
            // 检查是否完成
            size_t expected_len = 0;
            uint8_t first = static_cast<uint8_t>(utf8_buffer_[0]);
            if ((first & 0xE0) == 0xC0) expected_len = 2;
            else if ((first & 0xF0) == 0xE0) expected_len = 3;
            else if ((first & 0xF8) == 0xF0) expected_len = 4;
            
            if (utf8_buffer_.size() >= expected_len) {
                char32_t cp = DecodeUtf8();
                utf8_buffer_.clear();
                if (state_ == ParserState::Ground) {
                    OutputChar(cp);
                }
            }
            return;
        } else {
            // 无效的 UTF-8 序列
            utf8_buffer_.clear();
            OutputChar(0xFFFD);  // 替换字符
        }
    }

    switch (state_) {
        case ParserState::Ground:
            HandleGround(byte);
            break;
        case ParserState::Escape:
            HandleEscape(byte);
            break;
        case ParserState::CsiEntry:
        case ParserState::CsiParam:
        case ParserState::CsiIntermediate:
            HandleCsi(byte);
            break;
        case ParserState::OscString:
            HandleOsc(byte);
            break;
        default:
            state_ = ParserState::Ground;
            break;
    }
}

void AnsiParser::HandleGround(uint8_t byte) {
    // ESC
    if (byte == 0x1B) {
        state_ = ParserState::Escape;
        return;
    }

    // 控制字符
    if (byte < 0x20) {
        switch (byte) {
            case 0x07:  // BEL
                // 忽略响铃
                break;
            case 0x08:  // BS (Backspace)
                if (cursor_cb_) cursor_cb_(0, -1);  // 向左移动一列
                break;
            case 0x09:  // HT (Tab)
                // 输出空格到下一个制表位
                OutputChar(' ');
                break;
            case 0x0A:  // LF (Line Feed)
                if (newline_cb_) newline_cb_();
                break;
            case 0x0D:  // CR (Carriage Return)
                if (cr_cb_) cr_cb_();
                break;
            default:
                // 其他控制字符忽略
                break;
        }
        return;
    }

    // UTF-8 多字节起始
    if (IsUtf8Start(byte)) {
        utf8_buffer_.push_back(static_cast<char>(byte));
        return;
    }

    // 普通 ASCII 字符
    if (byte < 0x80) {
        OutputChar(static_cast<char32_t>(byte));
        return;
    }

    // 无效字节
    OutputChar(0xFFFD);
}

void AnsiParser::HandleEscape(uint8_t byte) {
    switch (byte) {
        case '[':  // CSI
            state_ = ParserState::CsiEntry;
            csi_params_.clear();
            break;
        case ']':  // OSC
            state_ = ParserState::OscString;
            osc_string_.clear();
            break;
        case 'P':  // DCS
            state_ = ParserState::DcsEntry;
            break;
        case '\\':  // ST (String Terminator)
            state_ = ParserState::Ground;
            break;
        case 'c':  // RIS (Reset to Initial State)
            Reset();
            break;
        case '7':  // DECSC (Save Cursor)
        case '8':  // DECRC (Restore Cursor)
            // 暂不支持
            state_ = ParserState::Ground;
            break;
        default:
            // 未知序列，回到 Ground
            state_ = ParserState::Ground;
            break;
    }
}

void AnsiParser::HandleCsi(uint8_t byte) {
    // 参数字节 (0x30-0x3F: 0-9, ;, <, =, >, ?)
    if (byte >= 0x30 && byte <= 0x3F) {
        if (byte >= '0' && byte <= '9') {
            // 数字参数
            if (csi_params_.empty()) {
                csi_params_.push_back(0);
            }
            csi_params_.back() = csi_params_.back() * 10 + (byte - '0');
        } else if (byte == ';') {
            // 参数分隔符
            csi_params_.push_back(0);
        }
        state_ = ParserState::CsiParam;
        return;
    }

    // 中间字节 (0x20-0x2F)
    if (byte >= 0x20 && byte <= 0x2F) {
        state_ = ParserState::CsiIntermediate;
        return;
    }

    // 终止字节 (0x40-0x7E)
    if (byte >= 0x40 && byte <= 0x7E) {
        ExecuteCsi(static_cast<char>(byte));
        state_ = ParserState::Ground;
        return;
    }

    // 无效字节
    HandleInvalidSequence();
}

void AnsiParser::HandleOsc(uint8_t byte) {
    // ST (String Terminator): ESC \ 或 BEL
    if (byte == 0x07) {
        ExecuteOsc();
        state_ = ParserState::Ground;
        return;
    }

    if (byte == 0x1B) {
        // 可能是 ESC \
        // 简化处理：下一个字节如果是 \，则结束
        // 这里先保存状态
        return;
    }

    if (byte == '\\' && !osc_string_.empty() && osc_string_.back() == '\x1B') {
        osc_string_.pop_back();
        ExecuteOsc();
        state_ = ParserState::Ground;
        return;
    }

    // 收集 OSC 字符串
    osc_string_.push_back(static_cast<char>(byte));

    // 防止过长
    if (osc_string_.size() > 4096) {
        HandleInvalidSequence();
    }
}

void AnsiParser::ExecuteCsi(char final_byte) {
    switch (final_byte) {
        case 'm':  // SGR (Select Graphic Rendition)
            ExecuteSgr();
            break;

        case 'A':  // CUU (Cursor Up)
            if (cursor_cb_) {
                int n = GetCsiParam(0, 1);
                cursor_cb_(-n, 0);
            }
            break;

        case 'B':  // CUD (Cursor Down)
            if (cursor_cb_) {
                int n = GetCsiParam(0, 1);
                cursor_cb_(n, 0);
            }
            break;

        case 'C':  // CUF (Cursor Forward)
            if (cursor_cb_) {
                int n = GetCsiParam(0, 1);
                cursor_cb_(0, n);
            }
            break;

        case 'D':  // CUB (Cursor Back)
            if (cursor_cb_) {
                int n = GetCsiParam(0, 1);
                cursor_cb_(0, -n);
            }
            break;

        case 'H':  // CUP (Cursor Position)
        case 'f':  // HVP (Horizontal Vertical Position)
            if (cursor_cb_) {
                int row = GetCsiParam(0, 1) - 1;  // 1-based to 0-based
                int col = GetCsiParam(1, 1) - 1;
                cursor_cb_(row, col);
            }
            break;

        case 'J':  // ED (Erase in Display)
            if (clear_cb_) {
                int mode = GetCsiParam(0, 0);
                clear_cb_(static_cast<ClearMode>(mode));
            }
            break;

        case 'K':  // EL (Erase in Line)
            // 暂不支持行内清除
            break;

        case 'S':  // SU (Scroll Up)
        case 'T':  // SD (Scroll Down)
            // 暂不支持滚动
            break;

        default:
            // 未知 CSI 命令，忽略
            break;
    }
}

void AnsiParser::ExecuteSgr() {
    if (csi_params_.empty()) {
        // ESC[m 等同于 ESC[0m
        current_style_.Reset();
        return;
    }

    for (size_t i = 0; i < csi_params_.size(); ++i) {
        int param = csi_params_[i];

        switch (param) {
            case 0:  // Reset
                current_style_.Reset();
                break;

            case 1:  // Bold
                current_style_.SetBold(true);
                break;

            case 2:  // Dim
                current_style_.SetDim(true);
                break;

            case 3:  // Italic
                current_style_.SetItalic(true);
                break;

            case 4:  // Underline
                current_style_.SetUnderline(true);
                break;

            case 7:  // Inverse
                current_style_.SetInverse(true);
                break;

            case 8:  // Hidden
                current_style_.SetHidden(true);
                break;

            case 9:  // Strikethrough
                current_style_.SetStrikethrough(true);
                break;

            case 22:  // Normal intensity (not bold, not dim)
                current_style_.SetBold(false);
                current_style_.SetDim(false);
                break;

            case 23:  // Not italic
                current_style_.SetItalic(false);
                break;

            case 24:  // Not underline
                current_style_.SetUnderline(false);
                break;

            case 27:  // Not inverse
                current_style_.SetInverse(false);
                break;

            case 28:  // Not hidden
                current_style_.SetHidden(false);
                break;

            case 29:  // Not strikethrough
                current_style_.SetStrikethrough(false);
                break;

            // 前景色 (30-37)
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                current_style_.fg_color = param - 30;
                break;

            case 38:  // 扩展前景色
                if (i + 1 < csi_params_.size()) {
                    if (csi_params_[i + 1] == 5 && i + 2 < csi_params_.size()) {
                        // 256 色: 38;5;n
                        current_style_.fg_color = static_cast<uint8_t>(csi_params_[i + 2]);
                        i += 2;
                    } else if (csi_params_[i + 1] == 2 && i + 4 < csi_params_.size()) {
                        // RGB: 38;2;r;g;b (暂不支持，使用最接近的 256 色)
                        i += 4;
                    }
                }
                break;

            case 39:  // 默认前景色
                current_style_.fg_color = 7;
                break;

            // 背景色 (40-47)
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                current_style_.bg_color = param - 40;
                break;

            case 48:  // 扩展背景色
                if (i + 1 < csi_params_.size()) {
                    if (csi_params_[i + 1] == 5 && i + 2 < csi_params_.size()) {
                        // 256 色: 48;5;n
                        current_style_.bg_color = static_cast<uint8_t>(csi_params_[i + 2]);
                        i += 2;
                    } else if (csi_params_[i + 1] == 2 && i + 4 < csi_params_.size()) {
                        // RGB: 48;2;r;g;b (暂不支持)
                        i += 4;
                    }
                }
                break;

            case 49:  // 默认背景色
                current_style_.bg_color = 0;
                break;

            // 高亮前景色 (90-97)
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                current_style_.fg_color = param - 90 + 8;
                break;

            // 高亮背景色 (100-107)
            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                current_style_.bg_color = param - 100 + 8;
                break;

            default:
                // 未知参数，忽略
                break;
        }
    }
}

void AnsiParser::ExecuteOsc() {
    // OSC 格式: Ps ; Pt ST
    // Ps = 0: 设置图标名和窗口标题
    // Ps = 1: 设置图标名
    // Ps = 2: 设置窗口标题

    size_t sep = osc_string_.find(';');
    if (sep == std::string::npos) {
        return;
    }

    int ps = 0;
    try {
        ps = std::stoi(osc_string_.substr(0, sep));
    } catch (...) {
        return;
    }

    std::string pt = osc_string_.substr(sep + 1);

    if ((ps == 0 || ps == 2) && title_cb_) {
        title_cb_(pt);
    }
}

void AnsiParser::OutputChar(char32_t codepoint) {
    if (output_cb_) {
        uint8_t width = Cell::IsWideChar(codepoint) ? 2 : 1;
        output_cb_(Cell(codepoint, current_style_, width));
    }
}

void AnsiParser::HandleInvalidSequence() {
    state_ = ParserState::Ground;
    csi_params_.clear();
    osc_string_.clear();
}

int AnsiParser::GetCsiParam(size_t index, int default_value) const {
    if (index < csi_params_.size()) {
        return csi_params_[index] > 0 ? csi_params_[index] : default_value;
    }
    return default_value;
}

bool AnsiParser::IsUtf8Start(uint8_t byte) {
    return (byte & 0xC0) == 0xC0 && byte < 0xFE;
}

bool AnsiParser::IsUtf8Continuation(uint8_t byte) {
    return (byte & 0xC0) == 0x80;
}

char32_t AnsiParser::DecodeUtf8() {
    if (utf8_buffer_.empty()) {
        return 0xFFFD;
    }

    uint8_t first = static_cast<uint8_t>(utf8_buffer_[0]);
    char32_t cp = 0;
    size_t len = 0;

    if ((first & 0x80) == 0) {
        return first;
    } else if ((first & 0xE0) == 0xC0) {
        cp = first & 0x1F;
        len = 2;
    } else if ((first & 0xF0) == 0xE0) {
        cp = first & 0x0F;
        len = 3;
    } else if ((first & 0xF8) == 0xF0) {
        cp = first & 0x07;
        len = 4;
    } else {
        return 0xFFFD;
    }

    if (utf8_buffer_.size() < len) {
        return 0xFFFD;
    }

    for (size_t i = 1; i < len; ++i) {
        uint8_t b = static_cast<uint8_t>(utf8_buffer_[i]);
        if (!IsUtf8Continuation(b)) {
            return 0xFFFD;
        }
        cp = (cp << 6) | (b & 0x3F);
    }

    return cp;
}

}  // namespace mbink
