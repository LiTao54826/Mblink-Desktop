/**
 * @file html_logview_element.cpp
 * @brief 日志视图 DOM 元素实现
 */

#include "html_logview_element.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkTypeface.h"

#include <cstdio>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#undef GetMessage  // Windows 宏冲突
#endif

namespace mbink {

HTMLLogViewElement::HTMLLogViewElement()
    : Element("logview"),
      buffer_(std::make_unique<LogBuffer>()),
      renderer_(std::make_unique<LogRenderer>()),
      filter_(std::make_unique<LogFilter>()),
      search_(std::make_unique<LogSearch>()) {
    renderer_->SetBuffer(buffer_.get());
}

HTMLLogViewElement::~HTMLLogViewElement() = default;

// === DOM 属性 ===

int HTMLLogViewElement::max_entries() const {
    return static_cast<int>(buffer_->max_entries());
}

void HTMLLogViewElement::set_max_entries(int value) {
    // 需要重建 buffer
    auto new_buffer = std::make_unique<LogBuffer>(value);
    // 复制源名
    for (const auto& name : buffer_->GetSourceNames()) {
        new_buffer->RegisterSource(name);
    }
    buffer_ = std::move(new_buffer);
    renderer_->SetBuffer(buffer_.get());
    filter_dirty_ = true;
}

bool HTMLLogViewElement::show_timestamp() const {
    return renderer_->config().show_timestamp;
}

void HTMLLogViewElement::set_show_timestamp(bool value) {
    auto config = renderer_->config();
    config.show_timestamp = value;
    renderer_->SetConfig(config);
}

bool HTMLLogViewElement::show_level() const {
    return renderer_->config().show_level;
}

void HTMLLogViewElement::set_show_level(bool value) {
    auto config = renderer_->config();
    config.show_level = value;
    renderer_->SetConfig(config);
}

bool HTMLLogViewElement::show_source() const {
    return renderer_->config().show_source;
}

void HTMLLogViewElement::set_show_source(bool value) {
    auto config = renderer_->config();
    config.show_source = value;
    renderer_->SetConfig(config);
}

// === 日志操作 ===

void HTMLLogViewElement::Append(const std::string& level,
                                const std::string& source,
                                const std::string& message) {
    Append(StringToLogLevel(level), source, message);
}

void HTMLLogViewElement::Append(LogLevel level, const std::string& source,
                                const std::string& message) {
    bool was_at_bottom = IsAtBottom();

    buffer_->Append(level, source, message);
    filter_dirty_ = true;

    // 自动滚动
    if (auto_scroll_ && was_at_bottom) {
        ScrollToBottom();
    }
}

void HTMLLogViewElement::Clear() {
    buffer_->Clear();
    search_->Clear();
    selection_.ClearSelection();
    filtered_indices_.clear();
    filter_dirty_ = false;
    renderer_->SetScrollOffset(0);
}

size_t HTMLLogViewElement::GetLogCount() const {
    return buffer_->size();
}

// === 滚动控制 ===

void HTMLLogViewElement::ScrollTo(int line) {
    renderer_->ScrollTo(line);
}

void HTMLLogViewElement::ScrollToBottom() {
    int total = renderer_->GetDisplayLineCount();
    int visible = renderer_->visible_lines();
    if (total > visible) {
        renderer_->ScrollTo(total - visible);
    } else {
        renderer_->ScrollTo(0);
    }
}

void HTMLLogViewElement::ScrollToTop() {
    renderer_->ScrollTo(0);
}

// === 过滤 ===

void HTMLLogViewElement::SetLevelFilter(const std::vector<std::string>& levels) {
    uint8_t mask = 0;
    for (const auto& level : levels) {
        LogLevel l = StringToLogLevel(level);
        mask |= (1 << static_cast<int>(l));
    }
    filter_->SetLevelMask(mask);
    filter_dirty_ = true;
}

void HTMLLogViewElement::SetSourceFilter(const std::vector<std::string>& sources) {
    filter_->SetSourceFilter(sources);
    filter_dirty_ = true;
}

void HTMLLogViewElement::ClearFilter() {
    filter_->SetLevelMask(0xFF);
    filter_->ClearSourceFilter();
    filter_dirty_ = true;
}

// === 搜索 ===

int HTMLLogViewElement::Search(const std::string& query, bool use_regex) {
    UpdateFilteredIndices();

    const std::vector<size_t>* indices = nullptr;
    if (filter_->HasFilter()) {
        indices = &filtered_indices_;
    }

    int count = search_->Search(*buffer_, query, use_regex, indices);
    renderer_->SetSearch(search_.get());

    // 跳转到第一个匹配
    if (count > 0) {
        int log_idx = search_->GoToMatch(0);
        if (log_idx >= 0) {
            // 找到该日志在显示列表中的位置
            if (indices) {
                for (size_t i = 0; i < indices->size(); ++i) {
                    if ((*indices)[i] == static_cast<size_t>(log_idx)) {
                        ScrollTo(static_cast<int>(i));
                        break;
                    }
                }
            } else {
                ScrollTo(log_idx);
            }
        }
    }

    return count;
}

void HTMLLogViewElement::NextMatch() {
    int log_idx = search_->NextMatch();
    if (log_idx >= 0) {
        // 滚动到匹配位置
        if (filter_->HasFilter()) {
            for (size_t i = 0; i < filtered_indices_.size(); ++i) {
                if (filtered_indices_[i] == static_cast<size_t>(log_idx)) {
                    ScrollTo(static_cast<int>(i));
                    break;
                }
            }
        } else {
            ScrollTo(log_idx);
        }
    }
}

void HTMLLogViewElement::PrevMatch() {
    int log_idx = search_->PrevMatch();
    if (log_idx >= 0) {
        if (filter_->HasFilter()) {
            for (size_t i = 0; i < filtered_indices_.size(); ++i) {
                if (filtered_indices_[i] == static_cast<size_t>(log_idx)) {
                    ScrollTo(static_cast<int>(i));
                    break;
                }
            }
        } else {
            ScrollTo(log_idx);
        }
    }
}

void HTMLLogViewElement::ClearSearch() {
    search_->Clear();
    renderer_->SetSearch(nullptr);
}

int HTMLLogViewElement::GetMatchCount() const {
    return search_->match_count();
}

int HTMLLogViewElement::GetCurrentMatch() const {
    return search_->current_match();
}

// === 选择 ===

std::string HTMLLogViewElement::GetSelectedText() const {
    if (!selection_.HasSelection()) {
        return "";
    }

    auto range = selection_.GetSelection();
    std::string result;

    // 按行选择，直接返回整行内容
    for (int line = range.start_line; line <= range.end_line; ++line) {
        size_t log_idx = renderer_->DisplayIndexToLogIndex(line);
        if (log_idx >= buffer_->size()) continue;

        // 构建完整的渲染行文本（包括时间戳、级别、源名）
        auto entry = buffer_->GetEntry(log_idx);
        
        // 时间戳
        if (renderer_->config().show_timestamp) {
            uint32_t ts = entry.timestamp();
            uint32_t ms = ts % 1000;
            uint32_t total_sec = ts / 1000;
            uint32_t sec = total_sec % 60;
            uint32_t min = (total_sec / 60) % 60;
            uint32_t hour = total_sec / 3600;
            char buf[16];
            snprintf(buf, sizeof(buf), "%02u:%02u:%02u.%03u ", hour, min, sec, ms);
            result += buf;
        }
        
        // 级别
        if (renderer_->config().show_level) {
            result += "[";
            result += LogLevelToString(entry.level());
            result += "] ";
        }
        
        // 源名
        if (renderer_->config().show_source) {
            const std::string& source = buffer_->GetSourceName(entry.source_id());
            if (!source.empty()) {
                result += "[";
                result += source;
                result += "] ";
            }
        }
        
        // 消息
        result += entry.message();

        if (line < range.end_line) {
            result += '\n';
        }
    }

    return result;
}

void HTMLLogViewElement::CopySelection() {
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
#endif

    if (selection_callback_) {
        selection_callback_(text);
    }
}

void HTMLLogViewElement::SelectAll() {
    int total_lines = renderer_->GetDisplayLineCount();
    if (total_lines == 0) {
        return;
    }

    // 按行选择，列位置不重要
    selection_.StartSelection(0, 0);
    selection_.UpdateSelection(total_lines - 1, 9999);  // 用一个大数表示行尾
    selection_.EndSelection();
}

// === 导出 ===

std::string HTMLLogViewElement::Export(const std::string& format) const {
    if (format == "json") {
        return buffer_->ExportAsJson();
    }
    return buffer_->ExportAsText();
}

// === 渲染 ===

void HTMLLogViewElement::Render(SkCanvas* canvas, float x, float y,
                                float width, float height) {
    view_x_ = x;
    view_y_ = y;
    view_width_ = width;
    view_height_ = height;

    // 更新过滤索引
    UpdateFilteredIndices();

    // 设置过滤索引
    if (filter_->HasFilter()) {
        renderer_->SetFilteredIndices(&filtered_indices_);
    } else {
        renderer_->SetFilteredIndices(nullptr);
    }

    // 设置选择状态
    renderer_->SetSelection(&selection_);

    // 渲染
    SkRect bounds = SkRect::MakeXYWH(x, y, width, height);
    renderer_->Render(canvas, bounds);
}

void HTMLLogViewElement::SetFont(const std::string& family, float size) {
    auto typeface = SkTypeface::MakeEmpty();
    if (!family.empty()) {
        // 尝试加载指定字体，如果失败则使用空字体
        // 实际应用中应使用 FontManager 来加载字体
    }
    renderer_->SetFont(typeface, size);
}

// === 事件处理 ===

void HTMLLogViewElement::OnMouseDown(float x, float y, int button,
                                     int click_count) {
    if (button != 0) return;  // 只处理左键

    auto [line, col] = ScreenToLineCol(x, y);

    if (click_count == 1) {
        selection_.StartSelection(line, col);
    } else if (click_count == 2) {
        // 双击选词
        size_t log_idx = renderer_->DisplayIndexToLogIndex(line);
        if (log_idx < buffer_->size()) {
            std::string msg(buffer_->GetMessage(log_idx));
            selection_.SelectWord(line, col, msg);
        }
    } else if (click_count == 3) {
        // 三击选行
        size_t log_idx = renderer_->DisplayIndexToLogIndex(line);
        if (log_idx < buffer_->size()) {
            int len = static_cast<int>(buffer_->GetMessage(log_idx).length());
            selection_.SelectLine(line, len);
        }
    }
}

void HTMLLogViewElement::OnMouseMove(float x, float y) {
    if (selection_.IsSelecting()) {
        auto [line, col] = ScreenToLineCol(x, y);
        selection_.UpdateSelection(line, col);
    }
}

void HTMLLogViewElement::OnMouseUp(float x, float y, int button) {
    (void)x;
    (void)y;
    if (button != 0) return;
    selection_.EndSelection();
}

void HTMLLogViewElement::OnWheel(float delta_y) {
    int delta = static_cast<int>(delta_y / 40.0f);  // 约 3 行
    renderer_->ScrollBy(delta);
}

void HTMLLogViewElement::OnKeyDown(const std::string& key, bool ctrl, bool shift) {
    // Ctrl+C: 复制
    if (ctrl && (key == "c" || key == "C")) {
        CopySelection();
        return;
    }

    // Ctrl+A: 全选
    if (ctrl && (key == "a" || key == "A")) {
        SelectAll();
        return;
    }

    // F3 / Shift+F3: 下一个/上一个匹配
    if (key == "F3") {
        if (shift) {
            PrevMatch();
        } else {
            NextMatch();
        }
        return;
    }

    // Ctrl+Home / Ctrl+End
    if (ctrl) {
        if (key == "Home") {
            ScrollToTop();
        } else if (key == "End") {
            ScrollToBottom();
        }
    }

    // Page Up / Page Down
    if (key == "PageUp") {
        renderer_->ScrollBy(-renderer_->visible_lines());
    } else if (key == "PageDown") {
        renderer_->ScrollBy(renderer_->visible_lines());
    }
}

void HTMLLogViewElement::SetSelectionCallback(SelectionCallback callback) {
    selection_callback_ = std::move(callback);
}

// === 私有方法 ===

void HTMLLogViewElement::UpdateFilteredIndices() {
    if (!filter_dirty_) return;

    filter_->UpdateFilteredIndices(*buffer_, filtered_indices_);
    filter_dirty_ = false;
}

bool HTMLLogViewElement::IsAtBottom() const {
    int total = renderer_->GetDisplayLineCount();
    int visible = renderer_->visible_lines();
    int offset = renderer_->scroll_offset();

    return (total <= visible) || (offset >= total - visible - 1);
}

std::pair<int, int> HTMLLogViewElement::ScreenToLineCol(float x, float y) const {
    // x, y 已经是相对于元素的本地坐标
    // HitTestLine 已经包含了 scroll_offset
    int line = renderer_->HitTestLine(y);
    int col = renderer_->HitTestColumn(x);

    return {line, col};
}

}  // namespace mbink
