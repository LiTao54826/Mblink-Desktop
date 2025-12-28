# Render Objects 模块

渲染对象类集合，表示 DOM 元素的可视化表示。

## 文件列表

| 文件 | 描述 |
|------|------|
| render_object.h/cpp | 渲染对象基类 |
| render_block.cpp | 块级元素渲染 |
| render_inline.cpp | 行内元素渲染 |
| render_inline_block.h/cpp | 行内块元素渲染 |
| render_text.cpp | 文本渲染 |
| render_table.cpp | 表格渲染 |
| render_svg.h/cpp | SVG 渲染 |
| svg_path_parser.h/cpp | SVG 路径解析 |
| scrollbar_controller.h/cpp | 滚动条控制 |
| list_marker.h/cpp | 列表标记渲染 |
| select_dropdown.h/cpp | 下拉菜单渲染 |

## 继承关系

```
RenderObject (基类)
├── RenderBlock
├── RenderInline
├── RenderInlineBlock
├── RenderText
├── RenderTable
└── RenderSVG
```
