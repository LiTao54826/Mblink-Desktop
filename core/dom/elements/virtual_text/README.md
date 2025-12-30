# core/dom/elements/virtual_text/

## 概述

虚拟文本组件共享核心层，提供高性能文本显示的基础设施。

## 模块列表

- `virtual_buffer.h` - 泛型环形缓冲区模板，支持固定容量和自动淘汰
- `virtual_scroll_renderer.h/cpp` - 虚拟滚动渲染基类，只渲染可见区域
- `selection_manager.h/cpp` - 文本选择管理，支持单词和行选择
- `text_style.h` - 共享样式定义和颜色调色板

## 设计目标

1. **高性能** - 支持 100 万条数据，60fps 滚动
2. **内存高效** - 固定内存上限，自动淘汰旧数据
3. **代码复用** - 被 terminal/ 和 logview/ 共享使用

## 依赖关系

- 依赖: Skia (渲染)
- 被依赖: core/dom/elements/terminal, core/dom/elements/logview

## 使用示例

```cpp
// 环形缓冲区
VirtualBuffer<std::string> buffer(10000);
buffer.Append("line 1");
buffer.Append("line 2");

// 虚拟滚动渲染
class MyRenderer : public VirtualScrollRenderer {
    void Render(SkCanvas* canvas, const SkRect& bounds) override {
        // 只渲染可见行
        for (int i = scroll_offset_; i < scroll_offset_ + visible_lines_; i++) {
            RenderLine(canvas, i);
        }
    }
};

// 文本选择
SelectionManager selection;
selection.StartSelection(0, 5);
selection.UpdateSelection(2, 10);
std::string text = selection.GetSelectedText(get_line_func);
```
