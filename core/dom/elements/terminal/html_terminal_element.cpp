/**
 * @file html_terminal_element.cpp
 * @brief HTML 终端元素实现
 */

#include "html_terminal_element.h"
#include "command_executor.h"
#include "pty_backend.h"

#include <algorithm>
#include <atomic>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mbink {

// 全局原子标志，用于通知主线程终端需要重绘
// 这是线程安全的方式，因为 PTY 回调在后台线程中执行
static std::atomic<bool> g_terminal_needs_repaint{false};

bool TerminalNeedsRepaint() {
    return g_terminal_needs_repaint.exchange(false);
}

HTMLTerminalElement::HTMLTerminalElement()
    : Element("terminal") {
    Initialize();
}

HTMLTerminalElement::~HTMLTerminalElement() = default;

void HTMLTerminalElement::Initialize() {
    parser_ = std::make_unique<AnsiParser>();
    buffer_ = std::make_unique<TerminalBuffer>(cols_, scrollback_);
    renderer_ = std::make_unique<TerminalRenderer>();

    buffer_->set_visible_rows(rows_);
    renderer_->SetBuffer(buffer_.get());

    SetupParserCallbacks();
}

void HTMLTerminalElement::SetupParserCallbacks() {
    parser_->SetOutputCallback([this](const Cell& cell) {
        buffer_->PutCell(cell);
    });

    parser_->SetNewLineCallback([this]() {
        buffer_->NewLine();
    });

    parser_->SetCarriageReturnCallback([this]() {
        buffer_->CarriageReturn();
    });

    parser_->SetCursorCallback([this](int row, int col) {
        // 如果是相对移动（负值表示相对）
        if (row < 0 || col < 0) {
            buffer_->MoveCursor(row, col);
        } else {
            buffer_->SetCursor(row, col);
        }
    });

    parser_->SetClearCallback([this](ClearMode mode) {
        buffer_->Clear(mode);
    });
}

void HTMLTerminalElement::set_rows(int value) {
    if (value > 0 && value != rows_) {
        rows_ = value;
        if (buffer_) {
            buffer_->set_visible_rows(rows_);
        }
        // 通知 PTY 大小变化
        if (pty_ && pty_->IsRunning()) {
            pty_->Resize(rows_, cols_);
        }
    }
}

void HTMLTerminalElement::set_cols(int value) {
    if (value > 0 && value != cols_) {
        cols_ = value;
        // 需要重建缓冲区
        buffer_ = std::make_unique<TerminalBuffer>(cols_, scrollback_);
        buffer_->set_visible_rows(rows_);
        renderer_->SetBuffer(buffer_.get());
        SetupParserCallbacks();
        // 通知 PTY 大小变化
        if (pty_ && pty_->IsRunning()) {
            pty_->Resize(rows_, cols_);
        }
    }
}

void HTMLTerminalElement::set_scrollback(int value) {
    if (value > 0) {
        scrollback_ = value;
        // 需要重建缓冲区
        buffer_ = std::make_unique<TerminalBuffer>(cols_, scrollback_);
        buffer_->set_visible_rows(rows_);
        renderer_->SetBuffer(buffer_.get());
        SetupParserCallbacks();
    }
}

void HTMLTerminalElement::Write(const std::string& data) {
    if (parser_) {
        bool was_at_bottom = IsAtBottom();
        parser_->Parse(data);

        // 自动滚动到底部显示最新内容
        if (renderer_ && buffer_ && was_at_bottom) {
            ScrollToBottom();
        }

        // 设置全局标志，通知主线程需要重绘
        g_terminal_needs_repaint.store(true);
    }
}

void HTMLTerminalElement::Clear() {
    if (buffer_) {
        buffer_->Clear(ClearMode::All);
    }
    if (parser_) {
        parser_->Reset();
    }
}

void HTMLTerminalElement::ScrollTo(int line) {
    if (renderer_) {
        renderer_->ScrollTo(line);
    }
}

void HTMLTerminalElement::ScrollToBottom() {
    if (!renderer_ || !buffer_) {
        return;
    }

    if (last_bounds_.height() > 0) {
        renderer_->UpdateMetrics(last_bounds_.height());
    }

    renderer_->SetTotalLines(buffer_->total_lines());
    renderer_->ScrollTo(renderer_->max_scroll_offset());
}

bool HTMLTerminalElement::IsAtBottom() {
    if (!renderer_ || !buffer_) {
        return true;
    }

    if (last_bounds_.height() > 0) {
        renderer_->UpdateMetrics(last_bounds_.height());
    }

    renderer_->SetTotalLines(buffer_->total_lines());
    return renderer_->scroll_offset() >= renderer_->max_scroll_offset();
}


std::string HTMLTerminalElement::Serialize() const {
    if (buffer_) {
        return buffer_->Serialize();
    }
    return "";
}

void HTMLTerminalElement::Focus() {
    is_focused_ = true;
    // TODO: 通知焦点管理器
}

void HTMLTerminalElement::Execute(const std::string& command) {
    if (!executor_) {
        executor_ = std::make_unique<CommandExecutor>();

        executor_->SetOutputCallback([this](const std::string& data, bool is_stderr) {
            // stderr 可以用不同颜色显示
            if (is_stderr) {
                Write("\x1b[31m");  // 红色
            }
            Write(data);
            if (is_stderr) {
                Write("\x1b[0m");   // 重置
            }
        });

        executor_->SetExitCallback([this](int exit_code) {
            // 可以触发事件
            (void)exit_code;
        });
    }

    executor_->Execute(command);
}

void HTMLTerminalElement::StartShell(const std::string& shell) {
    if (!pty_) {
        pty_ = PtyBackend::Create();

        pty_->SetDataCallback([this](const char* data, size_t len) {
            Write(std::string(data, len));
        });

        pty_->SetExitCallback([this](int exit_code) {
            (void)exit_code;
        });
    }

    pty_->Start(shell, rows_, cols_);
}

void HTMLTerminalElement::SendInput(const std::string& input) {
    if (pty_ && pty_->IsRunning()) {
        pty_->Write(input.data(), input.size());
    }
}

void HTMLTerminalElement::Resize(int rows, int cols) {
    set_rows(rows);
    set_cols(cols);
}

std::string HTMLTerminalElement::GetSelectedText() const {
    if (!selection_.HasSelection() || !buffer_) {
        return "";
    }

    return selection_.GetSelectedText([this](int line) {
        return buffer_->GetLineText(line);
    });
}

void HTMLTerminalElement::CopySelection() {
    std::string text = GetSelectedText();
    if (text.empty()) {
        return;
    }

#ifdef _WIN32
    // Windows 剪贴板操作
    if (!OpenClipboard(nullptr)) {
        return;
    }

    EmptyClipboard();

    // 转换为宽字符（UTF-16）
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
                                        static_cast<int>(text.size()), nullptr, 0);
    if (wide_len > 0) {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wide_len + 1) * sizeof(wchar_t));
        if (hMem) {
            wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
            if (pMem) {
                MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
                                   static_cast<int>(text.size()), pMem, wide_len);
                pMem[wide_len] = 0;
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
        }
    }

    CloseClipboard();
#else
    // TODO: 其他平台的剪贴板实现
#endif
}

void HTMLTerminalElement::Paste() {
#ifdef _WIN32
    if (!OpenClipboard(nullptr)) {
        return;
    }

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        wchar_t* pData = static_cast<wchar_t*>(GlobalLock(hData));
        if (pData) {
            // 转换为 UTF-8
            int utf8_len = WideCharToMultiByte(CP_UTF8, 0, pData, -1,
                                               nullptr, 0, nullptr, nullptr);
            if (utf8_len > 0) {
                std::string utf8_text(utf8_len - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, pData, -1,
                                   &utf8_text[0], utf8_len, nullptr, nullptr);

                // 发送到 PTY
                if (pty_ && pty_->IsRunning()) {
                    SendInput(utf8_text);
                }
            }
            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
#else
    // TODO: 其他平台的剪贴板实现
#endif
}

void HTMLTerminalElement::Render(SkCanvas* canvas, float x, float y, float width, float height) {
    if (!renderer_) {
        return;
    }

    SkRect bounds = SkRect::MakeXYWH(x, y, width, height);
    last_bounds_ = bounds;  // 缓存用于滚动条命中测试

    // 更新焦点状态到渲染器
    renderer_->SetFocused(HasPseudoClass("focus"));

    // 更新选择高亮
    UpdateSelectionHighlight();

    renderer_->Render(canvas, bounds);
}

void HTMLTerminalElement::HandleKeyInput(const std::string& key, int modifiers) {
    // 注意：焦点状态由 FocusManager 管理，这里不再检查 is_focused_
    // KeyboardEventDispatcher 只会在元素有焦点时调用此方法

    bool ctrl_key = (modifiers & 1) != 0;
    bool shift_key = (modifiers & 2) != 0;

    // 处理复制粘贴快捷键
    if (ctrl_key && !shift_key) {
        if (key == "c" || key == "C") {
            // Ctrl+C: 如果有选择则复制，否则发送中断信号
            if (selection_.HasSelection()) {
                CopySelection();
                return;
            }
            // 没有选择时，发送 Ctrl+C 中断信号
        } else if (key == "v" || key == "V") {
            // Ctrl+V: 粘贴
            Paste();
            return;
        }
    }

    // Ctrl+Shift+C: 强制复制（即使没有选择也不发送中断）
    if (ctrl_key && shift_key && (key == "c" || key == "C")) {
        CopySelection();
        return;
    }

    // Ctrl+Shift+V: 强制粘贴
    if (ctrl_key && shift_key && (key == "v" || key == "V")) {
        Paste();
        return;
    }

    // 如果没有 PTY 或 PTY 未运行，直接返回
    if (!pty_ || !pty_->IsRunning()) {
        return;
    }

    // 将特殊按键转换为终端序列
    std::string sequence;

    if (key == "Enter") {
        sequence = "\r";
    } else if (key == "Backspace") {
        sequence = "\x7f";  // DEL 字符 (ASCII 127) - Windows ConPTY 需要 DEL 而非 BS
    } else if (key == "Tab") {
        sequence = "\t";
    } else if (key == "Escape") {
        sequence = "\x1b";
    } else if (key == "ArrowUp") {
        sequence = "\x1b[A";
    } else if (key == "ArrowDown") {
        sequence = "\x1b[B";
    } else if (key == "ArrowRight") {
        sequence = "\x1b[C";
    } else if (key == "ArrowLeft") {
        sequence = "\x1b[D";
    } else if (key == "Home") {
        sequence = "\x1bOH";  // xterm 应用模式
    } else if (key == "End") {
        sequence = "\x1bOF";  // xterm 应用模式
    } else if (key == "PageUp") {
        sequence = "\x1b[5~";
    } else if (key == "PageDown") {
        sequence = "\x1b[6~";
    } else if (key == "Insert") {
        sequence = "\x1b[2~";
    } else if (key == "Delete") {
        sequence = "\x1b[3~";
    } else if (key == "F1") {
        sequence = "\x1bOP";
    } else if (key == "F2") {
        sequence = "\x1bOQ";
    } else if (key == "F3") {
        sequence = "\x1bOR";
    } else if (key == "F4") {
        sequence = "\x1bOS";
    } else if (key == "F5") {
        sequence = "\x1b[15~";
    } else if (key == "F6") {
        sequence = "\x1b[17~";
    } else if (key == "F7") {
        sequence = "\x1b[18~";
    } else if (key == "F8") {
        sequence = "\x1b[19~";
    } else if (key == "F9") {
        sequence = "\x1b[20~";
    } else if (key == "F10") {
        sequence = "\x1b[21~";
    } else if (key == "F11") {
        sequence = "\x1b[23~";
    } else if (key == "F12") {
        sequence = "\x1b[24~";
    } else if (ctrl_key && key.length() == 1) {
        // Ctrl+字母 -> 控制字符
        char c = key[0];
        if (c >= 'a' && c <= 'z') {
            sequence = std::string(1, c - 'a' + 1);  // Ctrl+A = 0x01, Ctrl+B = 0x02, etc.
        } else if (c >= 'A' && c <= 'Z') {
            sequence = std::string(1, c - 'A' + 1);
        }
    }
    // 对于普通字符，不在这里处理，由 TEXT_INPUT 事件处理
    // 这样可以正确处理 IME 输入和组合键

    if (!sequence.empty()) {
        SendInput(sequence);
    }
}

void HTMLTerminalElement::HandleMouseDown(float x, float y, int button, int clicks) {
    if (button != 0) {  // 只处理左键
        return;
    }

    // 检查是否点击了滚动条区域
    if (renderer_ && renderer_->total_lines() > renderer_->visible_lines()) {
        const float scrollbar_width = 8.0f;
        float scrollbar_x = last_bounds_.right() - scrollbar_width - 2.0f;

        if (x >= scrollbar_x && x <= last_bounds_.right()) {
            // 点击在滚动条区域，开始拖动
            is_dragging_scrollbar_ = true;
            drag_start_y_ = y;
            drag_start_offset_ = renderer_->scroll_offset();
            return;
        }
    }

    int row, col;
    ScreenToCell(x, y, row, col);

    if (clicks == 1) {
        // 单击：开始选择
        selection_.StartSelection(row, col);
    } else if (clicks == 2) {
        // 双击：选择单词
        if (buffer_) {
            std::string line_text = buffer_->GetLineText(row);
            selection_.SelectWord(row, col, line_text);
        }
    } else if (clicks == 3) {
        // 三击：选择整行
        if (buffer_) {
            std::string line_text = buffer_->GetLineText(row);
            selection_.SelectLine(row, static_cast<int>(line_text.size()));
        }
    }
}

void HTMLTerminalElement::HandleMouseMove(float x, float y) {
    // 处理滚动条拖动
    if (is_dragging_scrollbar_ && renderer_) {
        float padding = 4.0f;
        float track_height = last_bounds_.height() - 2 * padding;

        // 计算滑块高度
        float content_ratio = static_cast<float>(renderer_->visible_lines()) / renderer_->total_lines();
        float thumb_height = track_height * content_ratio;
        const float min_thumb_height = 20.0f;
        if (thumb_height < min_thumb_height) thumb_height = min_thumb_height;

        // 计算可用轨道高度
        float available_track = track_height - thumb_height;
        if (available_track > 0) {
            // 计算拖动距离对应的滚动偏移变化
            float delta_y = y - drag_start_y_;
            int max_scroll = renderer_->max_scroll_offset();
            int delta_offset = static_cast<int>((delta_y / available_track) * max_scroll);

            renderer_->ScrollTo(drag_start_offset_ + delta_offset);
        }
        return;
    }

    if (!selection_.IsSelecting()) {
        return;
    }

    int row, col;
    ScreenToCell(x, y, row, col);
    selection_.UpdateSelection(row, col);
}

void HTMLTerminalElement::HandleMouseUp(float x, float y, int button) {
    if (button == 0) {
        if (is_dragging_scrollbar_) {
            is_dragging_scrollbar_ = false;
        } else {
            selection_.EndSelection();
        }
    }
}

void HTMLTerminalElement::HandleWheel(float delta) {
    if (renderer_) {
        // delta > 0 表示向下滚动（查看最新内容，scroll_offset 增加）
        // delta < 0 表示向上滚动（查看历史内容，scroll_offset 减少）
        int lines = static_cast<int>(delta / 40);  // 约 40 像素一行
        renderer_->ScrollBy(lines);
    }
}

void HTMLTerminalElement::UpdateSelectionHighlight() {
    if (!renderer_) {
        return;
    }

    if (selection_.HasSelection()) {
        virtual_text::SelectionRange sel = selection_.GetSelection();
        renderer_->SetSelection(sel.start_line, sel.start_col,
                               sel.end_line, sel.end_col);
    } else {
        renderer_->ClearSelection();
    }
}

void HTMLTerminalElement::ScreenToCell(float x, float y, int& row, int& col) const {
    if (!renderer_) {
        row = 0;
        col = 0;
        return;
    }

    row = renderer_->HitTestLine(y);
    col = renderer_->HitTestColumn(x);
}

}  // namespace mbink
