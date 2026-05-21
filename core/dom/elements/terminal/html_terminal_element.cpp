/**
 * @file html_terminal_element.cpp
 * @brief HTML 终端元素实现
 */

#include "html_terminal_element.h"
#include "command_executor.h"
#include "pty_backend.h"
#include "core/dom/document.h"
#include "core/dom/elements/native_text_repaint_coalescer.h"
#include "core/window/repaint_reason.h"

#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mbink {

namespace {
constexpr int64_t kStreamingRepaintIntervalMs = 120;
constexpr float kScrollbarThickness = 8.0f;
constexpr float kScrollbarGap = 2.0f;
constexpr float kScrollbarPadding = 4.0f;
constexpr float kScrollbarMinThumbSize = 20.0f;
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
        RequestRepaint(RepaintReason::Terminal);
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
        RequestRepaint(RepaintReason::Terminal);
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
        RequestRepaint(RepaintReason::Terminal);
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

        RequestCoalescedRepaint(RepaintReason::Terminal);
    }
}

void HTMLTerminalElement::Clear() {
    if (buffer_) {
        buffer_->Clear(ClearMode::All);
    }
    if (parser_) {
        parser_->Reset();
    }
    RequestRepaint(RepaintReason::Terminal);
}

void HTMLTerminalElement::ScrollTo(int line) {
    if (renderer_) {
        renderer_->ScrollTo(line);
        RequestRepaint(RepaintReason::Terminal);
    }
}

void HTMLTerminalElement::ScrollToBottom() {
    if (!renderer_ || !buffer_) {
        return;
    }

    if (last_bounds_.height() > 0) {
        const float scrollbar_thickness = 8.0f;
        const float scrollbar_gap = 2.0f;
        float content_height = last_bounds_.height();
        if (renderer_->max_horizontal_scroll_offset() > 0) {
            content_height -= scrollbar_thickness + scrollbar_gap;
        }
        renderer_->UpdateMetrics(content_height);
    }

    renderer_->SetTotalLines(buffer_->display_line_count());
    renderer_->ScrollTo(renderer_->max_scroll_offset());
}

bool HTMLTerminalElement::IsAtBottom() {
    if (!renderer_ || !buffer_) {
        return true;
    }

    if (last_bounds_.height() > 0) {
        const float scrollbar_thickness = 8.0f;
        const float scrollbar_gap = 2.0f;
        float content_height = last_bounds_.height();
        if (renderer_->max_horizontal_scroll_offset() > 0) {
            content_height -= scrollbar_thickness + scrollbar_gap;
        }
        renderer_->UpdateMetrics(content_height);
    }

    renderer_->SetTotalLines(buffer_->display_line_count());
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

        auto post_output = [weak = weak_from_this()](std::string data) {
            auto node = weak.lock();
            auto terminal = std::dynamic_pointer_cast<HTMLTerminalElement>(node);
            if (!terminal) {
                return;
            }

            auto doc = terminal->GetOwnerDocument();
            if (!doc) {
                return;
            }

            doc->PostUiTask([weak, data = std::move(data)]() {
                auto node = weak.lock();
                auto terminal = std::dynamic_pointer_cast<HTMLTerminalElement>(node);
                if (terminal) {
                    terminal->Write(data);
                }
            });
        };

        executor_->SetOutputCallback([post_output](const std::string& data, bool is_stderr) {
            // stderr 可以用不同颜色显示
            if (is_stderr) {
                post_output(std::string("\x1b[31m") + data + "\x1b[0m");
            } else {
                post_output(data);
            }
        });

        executor_->SetExitCallback([](int exit_code) {
            // 可以触发事件
            (void)exit_code;
        });
    }

    executor_->Execute(command);
}

void HTMLTerminalElement::StartShell(const std::string& shell) {
    if (!pty_) {
        pty_ = PtyBackend::Create();

        auto post_output = [weak = weak_from_this()](std::string data) {
            auto node = weak.lock();
            auto terminal = std::dynamic_pointer_cast<HTMLTerminalElement>(node);
            if (!terminal) {
                return;
            }

            auto doc = terminal->GetOwnerDocument();
            if (!doc) {
                return;
            }

            doc->PostUiTask([weak, data = std::move(data)]() {
                auto node = weak.lock();
                auto terminal = std::dynamic_pointer_cast<HTMLTerminalElement>(node);
                if (terminal) {
                    terminal->Write(data);
                }
            });
        };

        pty_->SetDataCallback([post_output](const char* data, size_t len) {
            post_output(std::string(data, len));
        });

        pty_->SetExitCallback([](int exit_code) {
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

    if (renderer_ && renderer_->has_horizontal_scrollbar()) {
        const float content_padding = 4.0f;
        bool has_vertical_scrollbar = renderer_->has_vertical_scrollbar();
        float content_height =
            last_bounds_.height() - (kScrollbarThickness + kScrollbarGap);
        float track_x = content_padding;
        float track_y = content_height + kScrollbarGap;
        float track_width = last_bounds_.width() -
                            (has_vertical_scrollbar
                                 ? kScrollbarThickness + kScrollbarGap
                                 : 0.0f) -
                            2 * content_padding;
        if (track_width > 0 && y >= track_y &&
            y <= track_y + kScrollbarThickness) {
            float cell_width = renderer_->cell_width() > 1.0f ? renderer_->cell_width() : 1.0f;
            float visible_columns = track_width / cell_width;
            if (visible_columns < 1.0f) visible_columns = 1.0f;
            float total_columns =
                static_cast<float>(renderer_->max_horizontal_scroll_offset()) +
                visible_columns;
            if (total_columns < visible_columns) total_columns = visible_columns;
            float thumb_width = track_width * (visible_columns / total_columns);
            if (thumb_width < 20.0f) thumb_width = 20.0f;
            if (thumb_width > track_width) thumb_width = track_width;
            float available_track = track_width - thumb_width;
            if (available_track < 0.0f) available_track = 0.0f;
            float scroll_ratio = renderer_->max_horizontal_scroll_offset() > 0
                                     ? static_cast<float>(renderer_->horizontal_scroll_offset()) /
                                           renderer_->max_horizontal_scroll_offset()
                                     : 0.0f;
            float thumb_x = track_x + scroll_ratio * available_track;
            if (x >= thumb_x && x <= thumb_x + thumb_width) {
                is_dragging_horizontal_scrollbar_ = true;
                drag_start_x_ = x;
                drag_start_offset_ = renderer_->horizontal_scroll_offset();
                last_drag_horizontal_offset_ = drag_start_offset_;
                return;
            }
        }
    }

    if (renderer_ && renderer_->has_vertical_scrollbar()) {
        bool has_horizontal_scrollbar = renderer_->has_horizontal_scrollbar();
        float scrollbar_x = last_bounds_.width() - kScrollbarThickness;
        float track_height = last_bounds_.height() -
                             (has_horizontal_scrollbar
                                  ? kScrollbarThickness + kScrollbarGap
                                  : 0.0f) -
                             2 * kScrollbarPadding;
        if (x >= scrollbar_x && x <= scrollbar_x + kScrollbarThickness &&
            y >= kScrollbarPadding &&
            y <= kScrollbarPadding + track_height) {
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
        RequestRepaint(RepaintReason::Terminal);
    } else if (clicks == 2) {
        // 双击：选择单词
        if (buffer_) {
            std::string line_text = buffer_->GetLineText(row);
            selection_.SelectWord(row, col, line_text);
            RequestRepaint(RepaintReason::Terminal);
        }
    } else if (clicks == 3) {
        // 三击：选择整行
        if (buffer_) {
            std::string line_text = buffer_->GetLineText(row);
            selection_.SelectLine(row, static_cast<int>(line_text.size()));
            RequestRepaint(RepaintReason::Terminal);
        }
    }
}

void HTMLTerminalElement::HandleMouseMove(float x, float y) {
    if (is_dragging_horizontal_scrollbar_ && renderer_) {
        const float content_padding = 4.0f;
        bool has_vertical_scrollbar = renderer_->has_vertical_scrollbar();
        float track_width = last_bounds_.width() -
                            (has_vertical_scrollbar
                                 ? kScrollbarThickness + kScrollbarGap
                                 : 0.0f) -
                            2 * content_padding;
        float cell_width = renderer_->cell_width() > 1.0f ? renderer_->cell_width() : 1.0f;
        float visible_columns = track_width / cell_width;
        if (visible_columns < 1.0f) visible_columns = 1.0f;
        float total_columns =
            static_cast<float>(renderer_->max_horizontal_scroll_offset()) +
            visible_columns;
        if (total_columns < visible_columns) total_columns = visible_columns;
        float thumb_width = track_width * (visible_columns / total_columns);
        if (thumb_width < 20.0f) thumb_width = 20.0f;
        if (thumb_width > track_width) thumb_width = track_width;
        float available_track = track_width - thumb_width;
        if (available_track > 0) {
            float delta_x = x - drag_start_x_;
            int max_scroll = renderer_->max_horizontal_scroll_offset();
            int delta_offset = static_cast<int>((delta_x / available_track) * max_scroll);
            int new_offset = drag_start_offset_ + delta_offset;
            if (new_offset != last_drag_horizontal_offset_) {
                renderer_->SetHorizontalScrollOffset(new_offset);
                last_drag_horizontal_offset_ = renderer_->horizontal_scroll_offset();
                RequestRepaint(RepaintReason::Terminal);
            }
        }
        return;
    }

    if (is_dragging_scrollbar_ && renderer_) {
        bool has_horizontal_scrollbar = renderer_->has_horizontal_scrollbar();
        float track_height = last_bounds_.height() -
                             (has_horizontal_scrollbar
                                  ? kScrollbarThickness + kScrollbarGap
                                  : 0.0f) -
                             2 * kScrollbarPadding;

        // 计算滑块高度
        float content_ratio =
            static_cast<float>(renderer_->visible_lines()) /
            (std::max)(1, renderer_->total_lines());
        float thumb_height = track_height * content_ratio;
        thumb_height =
            std::clamp(thumb_height, kScrollbarMinThumbSize, track_height);

        // 计算可用轨道高度
        float available_track = track_height - thumb_height;
        if (available_track > 0) {
            // 计算拖动距离对应的滚动偏移变化
            float delta_y = y - drag_start_y_;
            int max_scroll = renderer_->max_scroll_offset();
            int delta_offset = static_cast<int>((delta_y / available_track) * max_scroll);

            renderer_->ScrollTo(drag_start_offset_ + delta_offset);
            RequestRepaint(RepaintReason::Terminal);
        }
        return;
    }

    if (!selection_.IsSelecting()) {
        return;
    }

    int row, col;
    ScreenToCell(x, y, row, col);
    selection_.UpdateSelection(row, col);
    RequestRepaint(RepaintReason::Terminal);
}

void HTMLTerminalElement::HandleMouseUp(float x, float y, int button) {
    if (button == 0) {
        if (is_dragging_horizontal_scrollbar_) {
            is_dragging_horizontal_scrollbar_ = false;
            last_drag_horizontal_offset_ = -1;
        } else if (is_dragging_scrollbar_) {
            is_dragging_scrollbar_ = false;
        } else {
            selection_.EndSelection();
            RequestRepaint(RepaintReason::Terminal);
        }
    }
}

void HTMLTerminalElement::HandleWheel(float delta, bool horizontal) {
    if (renderer_) {
        int lines = static_cast<int>(delta / 40);
        if (horizontal) {
            renderer_->ScrollHorizontallyBy(lines);
        } else {
            // delta > 0 表示向下滚动（查看最新内容，scroll_offset 增加）
            // delta < 0 表示向上滚动（查看历史内容，scroll_offset 减少）
            renderer_->ScrollBy(lines);
        }
        RequestRepaint(RepaintReason::Terminal);
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

void HTMLTerminalElement::RequestCoalescedRepaint(RepaintReason reason) {
    NativeTextRepaintCoalescer::Instance().Request(
        weak_from_this(), reason, kStreamingRepaintIntervalMs);
}

}  // namespace mbink
