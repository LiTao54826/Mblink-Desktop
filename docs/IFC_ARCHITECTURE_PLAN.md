# MBink Inline Formatting Context (IFC) 架构开发计划

## 概述

实现一个完整的 Inline Formatting Context (IFC) 布局引擎，与 Taffy (Flexbox/Grid) 配合工作，使 MBink 的布局行为更接近真实浏览器。

## 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                    LayoutEngine                          │
├─────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │
│  │   Taffy     │  │    IFC      │  │      BFC        │  │
│  │  (Flex/Grid)│  │  (Inline)   │  │    (Block)      │  │
│  └─────────────┘  └─────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### 核心组件

```
core/layout/
├── layout_engine.h/cpp          # 主布局引擎（已存在）
├── inline_formatting_context.h/cpp  # IFC 实现
├── line_box.h/cpp               # 行盒
├── inline_box.h/cpp             # 内联盒
├── text_run.h/cpp               # 文本片段
└── line_breaker.h/cpp           # 断行器
```

---

## Phase 1: 核心数据结构 (预计 3-5 天)

### 1.1 TextRun - 文本片段

```cpp
// core/layout/text_run.h
struct TextRun {
    std::string text;           // 文本内容
    size_t start_offset;        // 在原始文本中的起始位置
    size_t end_offset;          // 在原始文本中的结束位置
    
    float width;                // 渲染宽度
    float height;               // 高度（line-height）
    float baseline;             // 基线位置
    
    // 来源信息
    RenderText* render_text;    // 关联的 RenderText
    const ComputedStyle* style; // 样式引用
    
    // 换行信息
    bool can_break_before;      // 此片段前可以换行
    bool can_break_after;       // 此片段后可以换行
    bool is_whitespace;         // 是否是空白
};
```

### 1.2 InlineBox - 内联盒

```cpp
// core/layout/inline_box.h
struct InlineBox {
    enum class Type {
        TEXT,           // 文本内容
        ATOMIC,         // 原子内联元素 (img, inline-block, svg)
        INLINE_START,   // <span> 开始
        INLINE_END      // </span> 结束
    };
    
    Type type;
    RenderObject* render_object;
    
    float width;
    float height;
    float baseline;             // 距离顶部的基线位置
    
    // margin/padding/border（仅用于 INLINE_START/END）
    float margin_left, margin_right;
    float padding_left, padding_right;
    float border_left, border_right;
    
    // 子内容（对于 TEXT 类型）
    std::vector<TextRun> text_runs;
};
```

### 1.3 LineBox - 行盒

```cpp
// core/layout/line_box.h
struct LineBox {
    float x, y;                 // 行盒位置
    float width;                // 行盒宽度
    float height;               // 行盒高度
    float baseline;             // 行盒基线（距离顶部）
    
    std::vector<InlineBox*> boxes;  // 此行包含的内联盒
    
    // 对齐信息
    float available_width;      // 可用宽度
    float content_width;        // 实际内容宽度
    
    // 计算方法
    void CalculateHeight();     // 根据内容计算高度
    void AlignBoxes();          // 对齐内联盒
    void ApplyTextAlign(const std::string& align); // 应用 text-align
};
```

### 1.4 InlineFormattingContext

```cpp
// core/layout/inline_formatting_context.h
class InlineFormattingContext {
public:
    InlineFormattingContext(RenderObject* container);
    
    // 主布局方法
    void Layout(float available_width);
    
    // 获取布局结果
    float GetContentHeight() const;
    const std::vector<LineBox>& GetLineBoxes() const;
    
private:
    RenderObject* container_;
    std::vector<LineBox> line_boxes_;
    std::vector<InlineBox> inline_boxes_;
    
    // 布局步骤
    void CollectInlineBoxes();          // 1. 收集所有内联盒
    void BreakIntoLines(float width);   // 2. 断行
    void LayoutLines();                 // 3. 布局每行
    void PositionBoxes();               // 4. 定位盒子
};
```

---

## Phase 2: 断行算法 (预计 5-7 天)

### 2.1 LineBreaker - 断行器

```cpp
// core/layout/line_breaker.h
class LineBreaker {
public:
    struct BreakOpportunity {
        size_t position;        // 断行位置
        float width_before;     // 断行前的宽度
        BreakType type;         // 断行类型
    };
    
    enum class BreakType {
        NORMAL,         // 正常断行点（空格、标点后）
        WORD_BREAK,     // word-break: break-all
        OVERFLOW_WRAP,  // overflow-wrap: break-word
        FORCED          // <br> 或 \n
    };
    
    // 查找断行机会
    std::vector<BreakOpportunity> FindBreakOpportunities(
        const std::vector<InlineBox>& boxes,
        float available_width
    );
    
    // 执行断行
    std::vector<LineBox> BreakIntoLines(
        std::vector<InlineBox>& boxes,
        float available_width,
        const ComputedStyle& style
    );
    
private:
    // Unicode 断行算法 (UAX #14 简化版)
    bool CanBreakBetween(uint32_t prev_char, uint32_t next_char);
    bool IsBreakableWhitespace(uint32_t ch);
};
```

### 2.2 支持的 CSS 属性

| 属性 | 值 | 说明 |
|------|-----|------|
| `white-space` | normal, nowrap, pre, pre-wrap, pre-line | 空白处理 |
| `word-break` | normal, break-all, keep-all | 断词规则 |
| `overflow-wrap` | normal, break-word, anywhere | 溢出换行 |
| `hyphens` | none, manual, auto (可选) | 连字符 |

---

## Phase 3: 行盒布局 (预计 4-5 天)

### 3.1 行高计算

```cpp
void LineBox::CalculateHeight() {
    float max_ascent = 0;
    float max_descent = 0;
    
    for (auto* box : boxes) {
        float ascent = box->baseline;
        float descent = box->height - box->baseline;
        
        // 应用 vertical-align
        ApplyVerticalAlign(box, ascent, descent);
        
        max_ascent = std::max(max_ascent, ascent);
        max_descent = std::max(max_descent, descent);
    }
    
    height = max_ascent + max_descent;
    baseline = max_ascent;
}
```

### 3.2 vertical-align 支持

| 值 | 实现 |
|----|------|
| baseline | 基线对齐（默认） |
| top | 顶部对齐 |
| middle | 中线对齐 |
| bottom | 底部对齐 |
| text-top | 文本顶部对齐 |
| text-bottom | 文本底部对齐 |
| super | 上标 |
| sub | 下标 |
| `<length>` | 相对基线偏移 |
| `<percentage>` | 相对 line-height 偏移 |

---

## Phase 4: 与 Taffy 集成 (预计 3-4 天)

### 4.1 作为 Measure 函数

```cpp
// 在 layout_engine.cpp 中
TaffySize IFCMeasureFunction(
    float known_width, float known_height,
    float available_width, float available_height,
    void* context
) {
    auto* render_obj = static_cast<RenderObject*>(context);
    
    // 创建 IFC 并布局
    InlineFormattingContext ifc(render_obj);
    ifc.Layout(available_width > 0 ? available_width : INFINITY);
    
    return {
        .width = ifc.GetContentWidth(),
        .height = ifc.GetContentHeight()
    };
}
```

### 4.2 集成策略

```cpp
void LayoutEngine::CreateNode(RenderObject* render_obj) {
    // ...
    
    // 判断是否需要使用 IFC
    if (NeedsInlineFormattingContext(render_obj)) {
        // 设置 IFC measure 函数
        TaffyTree_SetNodeContext(taffy_tree_, node, IFCMeasureFunction, render_obj);
        
        // 不添加子节点到 Taffy（IFC 自己处理）
        return;
    }
    
    // 正常添加子节点...
}

bool LayoutEngine::NeedsInlineFormattingContext(RenderObject* obj) {
    // Block 容器且只包含内联内容
    if (obj->GetType() != RenderObjectType::BLOCK) return false;
    
    for (auto& child : obj->GetChildren()) {
        auto type = child->GetType();
        if (type == RenderObjectType::BLOCK ||
            type == RenderObjectType::FLEX ||
            type == RenderObjectType::GRID) {
            return false;  // 有块级子元素，不使用纯 IFC
        }
    }
    return true;
}
```

---

## Phase 5: 高级特性 (预计 5-7 天)

### 5.1 text-align

```cpp
void LineBox::ApplyTextAlign(const std::string& align) {
    float extra_space = available_width - content_width;
    if (extra_space <= 0) return;
    
    if (align == "left" || align == "start") {
        // 默认，无需调整
    } else if (align == "right" || align == "end") {
        // 所有盒子右移
        for (auto* box : boxes) box->x += extra_space;
    } else if (align == "center") {
        float offset = extra_space / 2;
        for (auto* box : boxes) box->x += offset;
    } else if (align == "justify") {
        DistributeSpace(extra_space);
    }
}
```

### 5.2 其他属性

| 属性 | 说明 |
|------|------|
| `text-indent` | 首行缩进 |
| `letter-spacing` | 字符间距 |
| `word-spacing` | 单词间距 |
| `direction` | 文本方向 (ltr/rtl) |
| `unicode-bidi` | BiDi 控制 |

---

## Phase 6: 优化与测试 (预计 3-5 天)

### 6.1 性能优化

1. **缓存机制**
   - 缓存已测量的 TextRun
   - 缓存断行结果

2. **增量布局**
   - 只重新布局改变的行
   - 文本编辑时局部更新

3. **并行处理**
   - 多行可并行计算高度

### 6.2 测试用例

```javascript
// 测试用例示例
const ifcTests = [
    "基本文本换行",
    "混合内联元素 (span, strong, em)",
    "inline-block 混排",
    "不同 vertical-align 值",
    "text-align: justify",
    "中英文混排",
    "Emoji 支持",
    "RTL 文本",
    "嵌套内联元素",
    "white-space 属性"
];
```

---

## 时间表

| 阶段 | 预计时间 | 依赖 |
|------|---------|------|
| Phase 1: 核心数据结构 | 3-5 天 | 无 |
| Phase 2: 断行算法 | 5-7 天 | Phase 1 |
| Phase 3: 行盒布局 | 4-5 天 | Phase 1, 2 |
| Phase 4: Taffy 集成 | 3-4 天 | Phase 1, 2, 3 |
| Phase 5: 高级特性 | 5-7 天 | Phase 4 |
| Phase 6: 优化测试 | 3-5 天 | Phase 5 |

**总计: 23-33 天 (约 4-6 周)**

---

## 风险与挑战

1. **Unicode 复杂性** - 断行算法需要处理各种语言
2. **性能** - 大量文本时的布局性能
3. **边缘情况** - 浏览器有很多历史遗留行为
4. **BiDi 支持** - 双向文本处理复杂

## 参考资料

- [CSS Inline Layout Module Level 3](https://www.w3.org/TR/css-inline-3/)
- [CSS Text Module Level 3](https://www.w3.org/TR/css-text-3/)
- [Unicode Line Breaking Algorithm (UAX #14)](https://www.unicode.org/reports/tr14/)
- [Chromium Layout NG](https://chromium.googlesource.com/chromium/src/+/master/third_party/blink/renderer/core/layout/ng/)

