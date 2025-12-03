# HTML 标签完整支持开发计划

> **版本**: 1.1
> **创建日期**: 2025-12-01
> **最后更新**: 2025-12-01
> **目标**: 兼容 React 生态和主流 UI 组件库（Ant Design、Material UI 等）

---

## 📊 当前状态

### 总体进度：**4/5 阶段已完成** ✅

| 阶段 | 内容 | 复杂度 | 状态 |
|------|------|--------|------|
| **Phase 1** | 语义化标签（30+个） | ⭐ 低 | ✅ 已完成 |
| **Phase 2** | 列表标签（li, ol） | ⭐ 低 | ✅ 已完成 |
| **Phase 3** | 表格标签（10个） | ⭐⭐⭐ 高 | ✅ 已完成 |
| **Phase 4** | SVG 支持 | ⭐⭐⭐⭐ 极高 | ✅ 已完成 |
| **Phase 5** | 表单增强标签（8个） | ⭐⭐ 中 | ✅ 已完成 |

### 已实现的专用元素类（32种）

#### 原有元素（14种）
| 标签 | 类名 | 状态 |
|------|------|------|
| `<input>` | HTMLInputElement | ✅ |
| `<textarea>` | HTMLTextAreaElement | ✅ |
| `<button>` | HTMLButtonElement | ✅ |
| `<form>` | HTMLFormElement | ✅ |
| `<select>` | HTMLSelectElement | ✅ |
| `<option>` | HTMLOptionElement | ✅ |
| `<a>` | HTMLAnchorElement | ✅ |
| `<label>` | HTMLLabelElement | ✅ |
| `<img>` | HTMLImageElement | ✅ |
| `<div>` | HTMLDivElement | ✅ |
| `<span>` | HTMLSpanElement | ✅ |
| `<p>` | HTMLParagraphElement | ✅ |
| `<h1>`~`<h6>` | HTMLHeadingElement | ✅ |
| `<ul>` | HTMLUListElement | ✅ |

#### 新增列表元素（2种）
| 标签 | 类名 | 状态 |
|------|------|------|
| `<ol>` | HTMLOListElement | ✅ start, type, reversed |
| `<li>` | HTMLLIElement | ✅ value |

#### 新增表格元素（10种）
| 标签 | 类名 | 状态 |
|------|------|------|
| `<table>` | HTMLTableElement | ✅ |
| `<thead>` | HTMLTableSectionElement | ✅ |
| `<tbody>` | HTMLTableSectionElement | ✅ |
| `<tfoot>` | HTMLTableSectionElement | ✅ |
| `<tr>` | HTMLTableRowElement | ✅ rowIndex, sectionRowIndex |
| `<th>` | HTMLTableCellElement | ✅ colspan, rowspan, cellIndex |
| `<td>` | HTMLTableCellElement | ✅ colspan, rowspan, cellIndex |
| `<caption>` | HTMLTableCaptionElement | ✅ |
| `<col>` | HTMLTableColElement | ✅ span |
| `<colgroup>` | HTMLTableColElement | ✅ span |

#### 新增表单增强元素（8种）
| 标签 | 类名 | 状态 |
|------|------|------|
| `<fieldset>` | HTMLFieldSetElement | ✅ disabled, name, form |
| `<legend>` | HTMLLegendElement | ✅ form |
| `<optgroup>` | HTMLOptGroupElement | ✅ disabled, label |
| `<datalist>` | HTMLDataListElement | ✅ options |
| `<output>` | HTMLOutputElement | ✅ value, name, for |
| `<progress>` | HTMLProgressElement | ✅ value, max, position |
| `<meter>` | HTMLMeterElement | ✅ value, min, max, low, high, optimum |
| `<dialog>` | HTMLDialogElement | ✅ open, show(), showModal(), close() |

### 通用元素支持（40+种语义化标签）

所有以下标签使用通用 `Element` 类，但已在 `StyleResolver` 中注册正确的默认样式：

- **布局标签**: header, footer, main, nav, section, article, aside, figure, figcaption, address, details, summary
- **文本标签**: strong, b, em, i, u, s, mark, small, sub, sup, code, kbd, samp, var, cite, q, abbr, time, data, ins, del, dfn
- **其他标签**: pre, blockquote, hr, br, wbr, dl, dt, dd, noscript, template, ruby, rt, rp, bdo, bdi

---

## Phase 1: 语义化标签（通用注册） ✅ 已完成

> **复杂度**: ⭐ 低
> **预计时间**: 1天
> **状态**: ✅ 已完成 (2025-12-01)
> **实现方式**: 在 `Document::CreateElement` 中注册，使用通用 `Element` 类 + 默认样式

### 实现内容

已在 `core/render/style_resolver.cpp` 中完成以下修改：

1. **`ApplyDefaultStyle()`** - 添加了所有语义化标签的 display 类型
   - 块级元素：header, footer, main, nav, section, article, aside, figure, figcaption, address, details, summary, dialog, menu, pre, blockquote, hr, ul, ol, li, dl, dt, dd, fieldset, legend, video, audio, noscript, template
   - 内联元素：strong, b, em, i, u, ins, s, strike, del, mark, small, big, sub, sup, code, kbd, samp, var, abbr, cite, dfn, q, time, data, br, wbr, ruby, rt, rp, bdo, bdi, output

2. **`ApplyElementSpecificStyle()`** - 添加了以下标签的默认样式：
   - `ins` - text-decoration: underline
   - `kbd` - monospace 字体 + 边框 + 圆角
   - `samp` - monospace 字体
   - `var`, `cite`, `dfn` - font-style: italic
   - `address` - font-style: italic + margin
   - `figure` - margin
   - `figcaption` - text-align: center
   - `details` - margin
   - `summary` - font-weight: bold
   - `dialog` - 白色背景 + padding + 边框
   - `progress`, `meter` - inline-block + 固定尺寸

### 1.1 布局语义标签

这些标签行为与 `<div>` 相同，仅语义不同：

| 标签 | 默认 display | 说明 |
|------|-------------|------|
| `<header>` | block | 页头 |
| `<footer>` | block | 页脚 |
| `<main>` | block | 主内容 |
| `<nav>` | block | 导航 |
| `<section>` | block | 区块 |
| `<article>` | block | 文章 |
| `<aside>` | block | 侧边栏 |
| `<figure>` | block | 图文容器 |
| `<figcaption>` | block | 图文标题 |
| `<address>` | block | 地址 |
| `<details>` | block | 折叠面板 |
| `<summary>` | block | 折叠标题 |

### 1.2 文本格式标签

这些标签行为与 `<span>` 相同，仅样式不同：

| 标签 | 默认样式 | 说明 |
|------|----------|------|
| `<strong>` | font-weight: bold | 重要文本 |
| `<b>` | font-weight: bold | 粗体 |
| `<em>` | font-style: italic | 强调 |
| `<i>` | font-style: italic | 斜体 |
| `<u>` | text-decoration: underline | 下划线 |
| `<s>` | text-decoration: line-through | 删除线 |
| `<mark>` | background: yellow | 高亮 |
| `<small>` | font-size: smaller | 小字 |
| `<sub>` | vertical-align: sub | 下标 |
| `<sup>` | vertical-align: super | 上标 |
| `<code>` | font-family: monospace | 代码 |
| `<kbd>` | font-family: monospace | 键盘输入 |
| `<samp>` | font-family: monospace | 示例输出 |
| `<var>` | font-style: italic | 变量 |
| `<cite>` | font-style: italic | 引用来源 |
| `<q>` | 添加引号 | 短引用 |
| `<abbr>` | (无特殊样式) | 缩写 |
| `<time>` | (无特殊样式) | 时间 |
| `<data>` | (无特殊样式) | 数据 |

### 1.3 块级格式标签

| 标签 | 默认样式 | 说明 |
|------|----------|------|
| `<pre>` | white-space: pre; font-family: monospace | 预格式化 |
| `<blockquote>` | margin-left: 40px | 块引用 |
| `<hr>` | border-top: 1px solid | 水平线（自闭合） |
| `<br>` | (换行) | 换行（自闭合） |

### 1.4 实现步骤

1. **修改 `Document::CreateElement`**：添加标签注册
2. **修改 `StyleResolver`**：添加默认样式（UA 样式表）
3. **修改 `Element::GetOuterHTML`**：更新自闭合标签列表

---

## Phase 2: 列表标签 ✅ 已完成

> **复杂度**: ⭐ 低
> **预计时间**: 0.5天
> **状态**: ✅ 已完成 (2025-12-01)

### 2.1 已实现的标签

| 标签 | 类名 | 状态 |
|------|------|------|
| `<li>` | HTMLLIElement | ✅ 支持 value 属性 |
| `<ol>` | HTMLOListElement | ✅ 支持 start、type、reversed 属性 |
| `<ul>` | HTMLUListElement | ✅ 已存在 |
| `<dl>` | Element (通用) | ✅ 使用通用元素 |
| `<dt>` | Element (通用) | ✅ 使用通用元素 + font-weight: bold |
| `<dd>` | Element (通用) | ✅ 使用通用元素 + margin-left: 40px |

### 2.2 实现内容

1. ✅ 创建 `HTMLLIElement` 类（`core/dom/html_li_element.cpp/h`）
   - 支持 `value` 属性（GetValue/SetValue）
2. ✅ 创建 `HTMLOListElement` 类（`core/dom/html_olist_element.cpp/h`）
   - 支持 `start` 属性（GetStart/SetStart）
   - 支持 `type` 属性（GetType/SetType）
   - 支持 `reversed` 属性（GetReversed/SetReversed）
3. ✅ 在 `Document::CreateElement` 中注册 ul、ol、li
4. ✅ dl、dt、dd 使用通用 Element 类，样式已在 Phase 1 中添加

---

## Phase 3: 表格标签 ✅ 已完成

> **复杂度**: ⭐⭐⭐ 高
> **预计时间**: 3-5天
> **状态**: ✅ 已完成 (2025-12-01)

### 3.1 已实现的标签

| 标签 | 类名 | 状态 |
|------|------|------|
| `<table>` | HTMLTableElement | ✅ |
| `<thead>` | HTMLTableSectionElement | ✅ |
| `<tbody>` | HTMLTableSectionElement | ✅ |
| `<tfoot>` | HTMLTableSectionElement | ✅ |
| `<tr>` | HTMLTableRowElement | ✅ 支持 rowIndex、sectionRowIndex |
| `<th>` | HTMLTableCellElement | ✅ 支持 colspan、rowspan、cellIndex |
| `<td>` | HTMLTableCellElement | ✅ 支持 colspan、rowspan、cellIndex |
| `<caption>` | HTMLTableCaptionElement | ✅ |
| `<colgroup>` | HTMLTableColElement | ✅ 支持 span |
| `<col>` | HTMLTableColElement | ✅ 支持 span |

### 3.2 已实现功能

- ✅ **DOM 元素类**：`html_table_element.cpp/h`（包含所有表格相关元素）
- ✅ **渲染对象**：`RenderTable`, `RenderTableRowGroup`, `RenderTableRow`, `RenderTableCell`, `RenderTableCaption`
- ✅ **表格布局算法**：自动列宽计算（min/preferred widths）
- ✅ **单元格合并**：colspan、rowspan 支持
- ✅ **边框模式**：border-collapse / border-separate
- ✅ **边框间距**：border-spacing 支持
- ✅ **colspan 传递**：DOM 属性自动传递到渲染对象

---

## Phase 4: SVG 支持 ✅ 已完成

> **复杂度**: ⭐⭐⭐⭐ 极高
> **预计时间**: 5-7天
> **状态**: ✅ 已完成 (2025-12-01)

### 4.1 已实现的元素

| 标签 | 类名 | 状态 |
|------|------|------|
| `<svg>` | SVGSVGElement | ✅ 支持 viewBox, width, height |
| `<path>` | SVGPathElement | ✅ 支持 d 属性完整解析 |
| `<g>` | SVGGElement | ✅ 支持分组和 transform |
| `<circle>` | SVGCircleElement | ✅ 支持 cx, cy, r |
| `<rect>` | SVGRectElement | ✅ 支持 x, y, width, height, rx, ry |
| `<line>` | SVGLineElement | ✅ 支持 x1, y1, x2, y2 |
| `<polyline>` | SVGPolylineElement | ✅ 支持 points |
| `<polygon>` | SVGPolygonElement | ✅ 支持 points |
| `<ellipse>` | SVGEllipseElement | ✅ 支持 cx, cy, rx, ry |
| `<text>` | SVGTextElement | ✅ 支持 x, y, font-size |

### 4.2 已实现的功能

- **SVG Path 解析器**：完整支持 M, L, H, V, C, S, Q, T, A, Z 命令（大小写）
- **Transform 解析器**：支持 translate, scale, rotate, skewX, skewY, matrix
- **Points 解析器**：支持 polyline/polygon 的 points 属性
- **Skia 集成**：使用 SkPath, SkPaint, SkCanvas 渲染
- **通用 SVG 属性**：fill, stroke, stroke-width, opacity, transform

### 4.3 新增文件

| 文件 | 说明 |
|------|------|
| `core/dom/svg_element.h` | SVG 元素类声明 |
| `core/dom/svg_element.cpp` | SVG 元素类实现 |
| `core/render/svg_path_parser.h` | SVG Path/Transform/Points 解析器声明 |
| `core/render/svg_path_parser.cpp` | SVG Path/Transform/Points 解析器实现 |
| `core/render/render_svg.h` | SVG 渲染器声明 |
| `core/render/render_svg.cpp` | SVG 渲染器实现 |

---

## Phase 5: 表单增强标签 ✅ 已完成

> **复杂度**: ⭐⭐ 中
> **预计时间**: 2天
> **状态**: ✅ 已完成 (2025-12-01)

### 5.1 已实现的标签

| 标签 | 类名 | 状态 |
|------|------|------|
| `<fieldset>` | HTMLFieldSetElement | ✅ 支持 disabled, name, form |
| `<legend>` | HTMLLegendElement | ✅ 支持 form 关联 |
| `<optgroup>` | HTMLOptGroupElement | ✅ 支持 disabled, label |
| `<datalist>` | HTMLDataListElement | ✅ 支持 options 获取 |
| `<output>` | HTMLOutputElement | ✅ 支持 value, name, for |
| `<progress>` | HTMLProgressElement | ✅ 支持 value, max, position |
| `<meter>` | HTMLMeterElement | ✅ 支持 value, min, max, low, high, optimum |
| `<dialog>` | HTMLDialogElement | ✅ 支持 open, show(), showModal(), close() |

### 5.2 实现文件

- `core/dom/html_form_controls.h` - 所有表单增强元素的头文件
- `core/dom/html_form_controls.cpp` - 所有表单增强元素的实现

---

## ✅ 验收标准

- [x] 所有语义化标签可正常解析和渲染 ✅ (Phase 1 完成)
- [x] 列表嵌套正确渲染 ✅ (Phase 2 完成)
- [x] 表格 DOM 元素支持 ✅ (Phase 3 完成)
- [x] 表单增强元素支持 ✅ (Phase 5 完成)
- [ ] 表格组件可正常展示数据（需要实际测试）
- [ ] Ant Design Table 组件可运行
- [ ] Ant Design Menu 组件可运行
- [x] 基础图标（SVG）可显示（Phase 4）

---

##  备注

1. ✅ **Phase 1-5 全部完成**：语义化标签、列表、表格、SVG 支持、表单增强
2. 📁 **新增文件汇总**：
   - `core/dom/html_li_element.cpp/h`
   - `core/dom/html_olist_element.cpp/h`
   - `core/dom/html_table_element.cpp/h`
   - `core/dom/html_form_controls.cpp/h`
   - `core/dom/svg_element.cpp/h`
   - `core/render/svg_path_parser.cpp/h`
   - `core/render/render_svg.cpp/h`

