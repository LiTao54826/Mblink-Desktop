# 虚拟文本组件设计文档

## 概述

本文档定义 MBink 框架的虚拟文本组件系列设计，包含共享核心层和两个专用元素：

1. **共享核心层** - VirtualBuffer、VirtualScrollRenderer、SelectionManager
2. **终端元素** - ANSI 解析、PTY 支持、命令执行
3. **日志视图元素** - 日志存储、级别过滤、搜索高亮

### 设计目标

1. **高性能** - 支持 100 万条数据，60fps 滚动
2. **内存高效** - 紧凑数据结构，固定内存上限
3. **代码复用** - 共享核心基础设施
4. **跨平台** - Windows/macOS/Linux 一致体验

## 架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      JavaScript API 层                           │
│  <terminal> / <logview> 的 DOM 接口和事件                        │
└───────────────────────────┬─────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────┐
│                      DOM 元素层                                  │
├────────────────────────┬────────────────────────────────────────┤
│  HTMLTerminalElement   │  HTMLLogViewElement                    │
│  - ANSI 渲染           │  - 日志级别过滤                        │
│  - PTY 交互            │  - 搜索高亮                            │
│  - 命令执行            │  - 时间戳格式化                        │
└────────────────────────┴────────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────┐
│                      共享核心层                                  │
├─────────────────┬─────────────────┬─────────────────────────────┤
│ VirtualBuffer<T>│ VirtualScroll   │ SelectionManager            │
│ 泛型环形缓冲区  │ Renderer        │ 文本选择                    │
│                 │ 虚拟滚动渲染    │                             │
└─────────────────┴─────────────────┴─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────┐
│                      平台后端层                                  │
├─────────────────┬─────────────────┬─────────────────────────────┤
│ ConPTY (Win)    │ forkpty (Unix)  │ CommandExecutor             │
└─────────────────┴─────────────────┴─────────────────────────────┘
```

## 目录结构

```
core/dom/elements/
├── virtual_text/                    # 共享核心层
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── virtual_buffer.h             # 泛型环形缓冲区
│   ├── virtual_scroll_renderer.h    # 虚拟滚动渲染基类
│   ├── virtual_scroll_renderer.cpp
│   ├── selection_manager.h          # 文本选择管理
│   ├── selection_manager.cpp
│   └── text_style.h                 # 共享样式定义
│
├── terminal/                        # 终端元素
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── html_terminal_element.h
│   ├── html_terminal_element.cpp
│   ├── ansi_parser.h
│   ├── ansi_parser.cpp
│   ├── terminal_buffer.h
│   ├── terminal_buffer.cpp
│   ├── terminal_renderer.h
│   ├── terminal_renderer.cpp
│   ├── pty_backend.h
│   ├── conpty_backend.cpp           # Windows
│   ├── unix_pty_backend.cpp         # Unix
│   └── command_executor.cpp
│
└── logview/                         # 日志视图元素
    ├── CMakeLists.txt
    ├── README.md
    ├── html_logview_element.h
    ├── html_logview_element.cpp
    ├── log_entry.h
    ├── log_buffer.h
    ├── log_buffer.cpp
    ├── log_renderer.h
    ├── log_renderer.cpp
    ├── log_filter.h
    └── log_search.cpp
```

## 组件和接口

### 第一部分：共享核心层

#### 1. VirtualBuffer<T>

泛型环形缓冲区模板，支持固定容量和自动淘汰。

```cpp
// core/dom/elements/virtual_text/virtual_buffer.h

template<typename T>
class VirtualBuffer {
public:
    explicit VirtualBuffer(size_t max_capacity);

    // === 写入操作 ===
    void Append(T&& item);
    void Append(const T& item);
    void Clear();

    // === 读取操作 ===
    const T& operator[](size_t index) const;  // O(1) 随机访问
    const T& Front() const;
    const T& Back() const;

    // === 容量信息 ===
    size_t size() const;
    size_t capacity() const;
    bool empty() const;
    bool full() const;

    // === 迭代器 ===
    class Iterator;
    Iterator begin() const;
    Iterator end() const;

private:
    std::deque<T> data_;
    size_t max_capacity_;

    void TrimToCapacity();
};
```

#### 2. VirtualScrollRenderer

虚拟滚动渲染基类，只渲染可见区域。

```cpp
// core/dom/elements/virtual_text/virtual_scroll_renderer.h

class VirtualScrollRenderer {
public:
    VirtualScrollRenderer();
    virtual ~VirtualScrollRenderer() = default;

    // === 配置 ===
    void SetFont(sk_sp<SkTypeface> typeface, float size);
    void SetLineHeight(float height);
    void SetPadding(float padding);

    // === 滚动控制 ===
    void SetScrollOffset(int line_offset);
    int scroll_offset() const;
    void ScrollTo(int line);
    void ScrollBy(int delta);

    // === 尺寸计算 ===
    int visible_lines() const;
    float line_height() const;
    float cell_width() const;
    void UpdateMetrics(float view_height);

    // === 命中测试 ===
    int HitTestLine(float y) const;
    int HitTestColumn(float x) const;

    // === 渲染（子类实现）===
    virtual void Render(SkCanvas* canvas, const SkRect& bounds) = 0;

protected:
    sk_sp<SkTypeface> typeface_;
    float font_size_ = 14.0f;
    float line_height_ = 0;
    float cell_width_ = 0;
    float padding_ = 4.0f;
    int scroll_offset_ = 0;
    int visible_lines_ = 0;
    int total_lines_ = 0;

    void UpdateCellMetrics();
    SkRect GetLineRect(int line_index, const SkRect& bounds) const;
};
```

#### 3. SelectionManager

文本选择管理，支持单词和行选择。

```cpp
// core/dom/elements/virtual_text/selection_manager.h

struct SelectionRange {
    int start_line;
    int start_col;
    int end_line;
    int end_col;

    bool IsEmpty() const;
    bool Contains(int line, int col) const;
    void Normalize();  // 确保 start <= end
};

class SelectionManager {
public:
    // === 选择操作 ===
    void StartSelection(int line, int col);
    void UpdateSelection(int line, int col);
    void EndSelection();
    void ClearSelection();

    // === 快捷选择 ===
    void SelectWord(int line, int col, const std::string& text);
    void SelectLine(int line, int line_length);
    void SelectAll(int total_lines, int last_line_length);

    // === 查询 ===
    bool HasSelection() const;
    SelectionRange GetSelection() const;
    bool IsSelected(int line, int col) const;

    // === 文本提取 ===
    template<typename GetLineFunc>
    std::string GetSelectedText(GetLineFunc get_line) const;

private:
    SelectionRange selection_;
    bool is_selecting_ = false;

    static bool IsWordChar(char32_t c);
};
```

#### 4. TextStyle

共享样式定义。

```cpp
// core/dom/elements/virtual_text/text_style.h

struct TextStyle {
    uint8_t fg_color = 7;       // 前景色索引 (0-255)
    uint8_t bg_color = 0;       // 背景色索引 (0-255)
    uint8_t flags = 0;          // 样式标志

    // 标志位
    static constexpr uint8_t BOLD        = 0x01;
    static constexpr uint8_t ITALIC      = 0x02;
    static constexpr uint8_t UNDERLINE   = 0x04;
    static constexpr uint8_t INVERSE     = 0x08;
    static constexpr uint8_t STRIKETHROUGH = 0x10;

    bool IsDefault() const;
    void Reset();
};

struct ColorPalette {
    SkColor colors[256];

    static ColorPalette Default();
    static ColorPalette Solarized();
    static ColorPalette Monokai();

    SkColor Resolve(uint8_t index) const;
};
```

---

### 第二部分：终端元素

#### 5. HTMLTerminalElement

```cpp
// core/dom/elements/terminal/html_terminal_element.h

class HTMLTerminalElement : public Element {
public:
    HTMLTerminalElement();
    ~HTMLTerminalElement() override;

    // === 属性 ===
    int rows() const;
    void set_rows(int value);
    int cols() const;
    void set_cols(int value);
    int scrollback() const;
    void set_scrollback(int value);

    // === 方法 ===
    void Write(const std::string& data);
    void Clear();
    void ScrollTo(int line);
    std::string Serialize() const;
    void Focus();

    // === 命令执行 ===
    void Execute(const std::string& command);
    void StartShell(const std::string& shell = "");
    void SendInput(const std::string& input);
    void Resize(int rows, int cols);

    // === 渲染 ===
    void Render(SkCanvas* canvas, const LayoutBox& box) override;

private:
    std::unique_ptr<AnsiParser> parser_;
    std::unique_ptr<TerminalBuffer> buffer_;
    std::unique_ptr<TerminalRenderer> renderer_;
    std::unique_ptr<PtyBackend> pty_;
    SelectionManager selection_;
};
```

#### 6. AnsiParser

```cpp
// core/dom/elements/terminal/ansi_parser.h

enum class ParserState {
    Ground, Escape, CsiEntry, CsiParam, CsiIntermediate, OscString, DcsEntry
};

struct Cell {
    char32_t codepoint = ' ';
    TextStyle style;
};

class AnsiParser {
public:
    using OutputCallback = std::function<void(const Cell& cell)>;
    using CursorCallback = std::function<void(int row, int col)>;
    using ClearCallback = std::function<void(int mode)>;

    void SetOutputCallback(OutputCallback cb);
    void SetCursorCallback(CursorCallback cb);
    void SetClearCallback(ClearCallback cb);

    void Parse(const char* data, size_t length);
    void Reset();

private:
    ParserState state_ = ParserState::Ground;
    TextStyle current_style_;
    std::vector<int> csi_params_;
    std::string utf8_buffer_;  // UTF-8 多字节缓冲

    void ProcessByte(uint8_t byte);
    void ExecuteCSI(char final_byte);
    void ExecuteSGR();
    void HandleInvalidSequence();
};
```

#### 7. TerminalBuffer

```cpp
// core/dom/elements/terminal/terminal_buffer.h

class TerminalBuffer {
public:
    TerminalBuffer(int cols, int scrollback_lines = 10000);

    // === 写入 ===
    void PutCell(const Cell& cell);
    void NewLine();
    void CarriageReturn();
    void Clear(int mode);

    // === 光标 ===
    void SetCursor(int row, int col);
    void MoveCursor(int dr, int dc);
    int cursor_row() const;
    int cursor_col() const;

    // === 读取 ===
    const Cell& GetCell(int row, int col) const;
    int total_lines() const;
    int cols() const;

    // === 序列化 ===
    std::string Serialize() const;
    std::string GetText(int start_row, int start_col, int end_row, int end_col) const;

private:
    VirtualBuffer<std::vector<Cell>> lines_;
    int cols_;
    int cursor_row_ = 0;
    int cursor_col_ = 0;
};
```

---

### 第三部分：日志视图元素

#### 8. LogEntry

紧凑的日志条目结构。

```cpp
// core/dom/elements/logview/log_entry.h

enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

struct LogEntry {
    uint32_t timestamp;      // 4 bytes - 相对时间戳（毫秒）
    LogLevel level;          // 1 byte
    uint8_t source_id;       // 1 byte - 预注册源名索引
    uint16_t message_length; // 2 bytes
    // message 数据紧随其后

    static constexpr size_t HEADER_SIZE = 8;
};

// 内存布局: [LogEntry header][message bytes...]
// 每条日志: 8 + message_length bytes
```

#### 9. LogBuffer

日志专用缓冲区，比终端更紧凑。

```cpp
// core/dom/elements/logview/log_buffer.h

class LogBuffer {
public:
    explicit LogBuffer(size_t max_entries = 100000);

    // === 写入 ===
    void Append(LogLevel level, uint8_t source_id, 
                std::string_view message, uint32_t timestamp = 0);
    void Clear();

    // === 读取 ===
    size_t size() const;
    LogLevel GetLevel(size_t index) const;
    uint8_t GetSourceId(size_t index) const;
    uint32_t GetTimestamp(size_t index) const;
    std::string_view GetMessage(size_t index) const;

    // === 源名管理 ===
    uint8_t RegisterSource(const std::string& name);
    const std::string& GetSourceName(uint8_t id) const;

    // === 导出 ===
    std::string ExportAsText() const;
    std::string ExportAsJson() const;

private:
    // 紧凑存储：所有日志数据连续存放
    std::vector<uint8_t> data_;
    std::vector<size_t> entry_offsets_;  // 每条日志的起始偏移
    std::vector<std::string> source_names_;
    size_t max_entries_;

    void TrimToCapacity();
};
```

#### 10. HTMLLogViewElement

```cpp
// core/dom/elements/logview/html_logview_element.h

class HTMLLogViewElement : public Element {
public:
    HTMLLogViewElement();
    ~HTMLLogViewElement() override;

    // === 属性 ===
    int max_entries() const;
    void set_max_entries(int value);
    bool auto_scroll() const;
    void set_auto_scroll(bool value);

    // === 方法 ===
    void Append(LogLevel level, const std::string& source,
                const std::string& message);
    void Clear();
    void ScrollTo(int line);

    // === 过滤 ===
    void SetLevelFilter(uint8_t level_mask);  // 位掩码
    void SetSourceFilter(const std::vector<std::string>& sources);

    // === 搜索 ===
    int Search(const std::string& query, bool regex = false);
    void NextMatch();
    void PrevMatch();
    void ClearSearch();

    // === 导出 ===
    std::string Export(const std::string& format = "text") const;

    // === 渲染 ===
    void Render(SkCanvas* canvas, const LayoutBox& box) override;

private:
    std::unique_ptr<LogBuffer> buffer_;
    std::unique_ptr<LogRenderer> renderer_;
    SelectionManager selection_;

    // 过滤状态
    uint8_t level_mask_ = 0xFF;  // 默认显示所有级别
    std::vector<size_t> filtered_indices_;

    // 搜索状态
    std::string search_query_;
    std::vector<size_t> search_matches_;
    int current_match_ = -1;

    void UpdateFilteredIndices();
};
```

#### 11. LogRenderer

```cpp
// core/dom/elements/logview/log_renderer.h

struct LogViewConfig {
    bool show_timestamp = true;
    bool show_level = true;
    bool show_source = true;
    std::string timestamp_format = "%H:%M:%S.%f";

    SkColor level_colors[5] = {
        0xFF888888,  // DEBUG - 灰色
        0xFFFFFFFF,  // INFO - 白色
        0xFFFFFF00,  // WARN - 黄色
        0xFFFF4444,  // ERROR - 红色
        0xFFFF0000,  // FATAL - 亮红色
    };
};

class LogRenderer : public VirtualScrollRenderer {
public:
    LogRenderer();

    void SetConfig(const LogViewConfig& config);
    void SetBuffer(LogBuffer* buffer);
    void SetFilteredIndices(const std::vector<size_t>* indices);
    void SetSearchMatches(const std::vector<size_t>* matches, int current);

    void Render(SkCanvas* canvas, const SkRect& bounds) override;

private:
    LogBuffer* buffer_ = nullptr;
    const std::vector<size_t>* filtered_indices_ = nullptr;
    const std::vector<size_t>* search_matches_ = nullptr;
    int current_match_ = -1;
    LogViewConfig config_;

    void RenderLine(SkCanvas* canvas, size_t log_index, 
                    const SkRect& line_rect, bool is_match);
};
```

---

## 数据模型

### 内存对比

```
终端 Cell: 8 bytes/字符
  - char32_t codepoint: 4 bytes
  - TextStyle style: 4 bytes
  
终端 100K 行 × 80 列 = 64 MB

日志 Entry: 8 bytes + 消息长度
  - header: 8 bytes
  - message: 平均 50 bytes
  
日志 100 万条 × 58 bytes = 58 MB
```

### 过滤实现

```cpp
// 过滤不复制数据，只维护索引
void HTMLLogViewElement::UpdateFilteredIndices() {
    filtered_indices_.clear();
    for (size_t i = 0; i < buffer_->size(); i++) {
        LogLevel level = buffer_->GetLevel(i);
        if (level_mask_ & (1 << static_cast<int>(level))) {
            filtered_indices_.push_back(i);
        }
    }
    renderer_->SetFilteredIndices(&filtered_indices_);
}
```

---

## 正确性属性

*属性是指在系统所有有效执行中都应保持为真的特征或行为。*

### 共享核心层属性

#### 属性 1: 缓冲区溢出处理

*对于任何*写入序列使缓冲区超过最大容量，缓冲区 SHALL 恰好包含 max_capacity 条目，且这些条目 SHALL 是最近写入的。

**验证: 需求 1.1, 3.3, 9.3**

#### 属性 2: 清除重置状态

*对于任何*缓冲区状态，调用 Clear() SHALL 导致空缓冲区，size() 返回 0。

**验证: 需求 1.4, 6.2**

#### 属性 3: 虚拟滚动正确性

*对于任何*总共 N 行且可见窗口为 M 行的视图，渲染 SHALL 只访问 [scroll_offset, scroll_offset + M) 范围内的行。

**验证: 需求 2.1, 3.4**

#### 属性 4: 命中测试准确性

*对于任何*点击位置 (x, y)，HitTestLine() 和 HitTestColumn() SHALL 返回正确的行列索引，误差不超过 1 像素。

**验证: 需求 2.4**

#### 属性 5: 单词选择

*对于任何*包含单词边界的文本，在位置 (row, col) 调用 SelectWord() SHALL 选中围绕该位置的连续单词字符。

**验证: 需求 14.3**

#### 属性 6: 行选择

*对于任何*多行缓冲区，在行 R 调用 SelectLine() SHALL 选中行 R 的所有字符。

**验证: 需求 14.4**

### 终端元素属性

#### 属性 7: ANSI 解析正确性

*对于任何*有效 ANSI 转义序列，解析后的 Cell SHALL 具有正确的颜色索引和样式标志。

**验证: 需求 3.1, 3.2**

#### 属性 8: Write/Serialize 往返

*对于任何*纯文本字符串，Write() 后调用 Serialize() SHALL 返回包含原始文本的字符串。

**验证: 需求 6.1, 6.4**

#### 属性 9: ScrollTo 定位

*对于任何*有效行号 L，调用 ScrollTo(L) SHALL 设置视口使行 L 为第一个可见行。

**验证: 需求 6.3**

#### 属性 10: 解析器错误恢复

*对于任何*包含格式错误 ANSI 序列的输入，解析器 SHALL 不崩溃且正确处理有效部分。

**验证: 需求 7.1, 7.2**

#### 属性 11: UTF-8 边界处理

*对于任何*在任意字节边界分割的 UTF-8 字符串，终端 SHALL 正确重组并显示所有字符。

**验证: 需求 7.3**

#### 属性 12: 行换行

*对于任何*长度 L > 终端宽度 W 的行，SHALL 显示在 ceil(L/W) 个可视行上。

**验证: 需求 7.4**

### 日志视图属性

#### 属性 13: 日志追加正确性

*对于任何*日志条目，Append() 后 buffer 的最后一条 SHALL 包含相同的 level、source 和 message。

**验证: 需求 9.1**

#### 属性 14: 自动滚动行为

*对于任何*启用 auto_scroll 且视口在底部的状态，新日志到达后视口 SHALL 仍在底部。

**验证: 需求 9.4**

#### 属性 15: 日志级别过滤

*对于任何*级别掩码设置，过滤后显示的所有日志 SHALL 满足 (1 << level) & mask != 0。

**验证: 需求 10.1, 10.3**

#### 属性 16: 搜索匹配正确性

*对于任何*搜索关键词，返回的匹配数量 SHALL 等于缓冲区中包含该关键词的日志条目数。

**验证: 需求 11.1, 11.2**

#### 属性 17: 正则搜索

*对于任何*有效正则表达式，搜索结果 SHALL 包含所有匹配该正则的日志条目。

**验证: 需求 11.4**

---

## 错误处理

### 解析错误

```cpp
void AnsiParser::HandleInvalidSequence() {
    state_ = ParserState::Ground;
    csi_params_.clear();
    // 继续处理后续输入
}
```

### 缓冲区保护

```cpp
template<typename T>
void VirtualBuffer<T>::TrimToCapacity() {
    while (data_.size() > max_capacity_) {
        data_.pop_front();
    }
}
```

### 搜索错误

```cpp
int HTMLLogViewElement::Search(const std::string& query, bool regex) {
    if (regex) {
        try {
            std::regex re(query);
            // 执行正则搜索
        } catch (const std::regex_error&) {
            // 无效正则，回退到普通搜索
            return SearchPlainText(query);
        }
    }
    return SearchPlainText(query);
}
```

---

## 测试策略

### 测试框架

- **单元测试**: Google Test
- **属性测试**: RapidCheck

### 属性测试示例

```cpp
// 属性 1: 缓冲区溢出处理
RC_GTEST_PROP(VirtualBuffer, OverflowKeepsNewest, ()) {
    auto capacity = *rc::gen::inRange(10, 100);
    auto items = *rc::gen::container<std::vector<int>>(
        rc::gen::inRange(0, 1000));

    VirtualBuffer<int> buffer(capacity);
    for (int item : items) {
        buffer.Append(item);
    }

    RC_ASSERT(buffer.size() <= capacity);
    if (items.size() > capacity) {
        // 验证保留的是最新的
        for (size_t i = 0; i < buffer.size(); i++) {
            RC_ASSERT(buffer[i] == items[items.size() - buffer.size() + i]);
        }
    }
}

// 属性 15: 日志级别过滤
RC_GTEST_PROP(LogBuffer, FilterByLevel, ()) {
    auto entries = *rc::gen::container<std::vector<std::pair<int, std::string>>>(
        rc::gen::pair(rc::gen::inRange(0, 5), rc::gen::string<std::string>()));
    auto mask = *rc::gen::inRange(0, 32);

    LogBuffer buffer(10000);
    for (const auto& [level, msg] : entries) {
        buffer.Append(static_cast<LogLevel>(level), 0, msg);
    }

    std::vector<size_t> filtered;
    for (size_t i = 0; i < buffer.size(); i++) {
        if ((1 << static_cast<int>(buffer.GetLevel(i))) & mask) {
            filtered.push_back(i);
        }
    }

    // 验证过滤结果
    for (size_t idx : filtered) {
        RC_ASSERT((1 << static_cast<int>(buffer.GetLevel(idx))) & mask);
    }
}
```

### 性能测试

```cpp
TEST(Performance, LogBuffer100万条) {
    LogBuffer buffer(1000000);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; i++) {
        buffer.Append(LogLevel::INFO, 0, "Test log message " + std::to_string(i));
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 2000);  // < 2 秒
}
```
