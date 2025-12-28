# Inline Formatting Context (IFC)

行内格式化上下文实现。

## 概述

IFC 负责处理行内元素的布局，包括：
- 文本换行
- 行高计算
- 垂直对齐
- 行内块元素

## 模块列表

| 文件 | 描述 |
|------|------|
| `inline_box.h/cpp` | 行内盒模型 |
| `line_box.h/cpp` | 行盒 |
| `line_breaker.h/cpp` | 换行算法 |
| `text_shaper.h/cpp` | 文本整形 |

## 参考

- CSS Inline Layout Module Level 3
- CSS Text Module Level 3
