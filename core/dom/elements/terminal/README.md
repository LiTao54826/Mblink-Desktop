# core/dom/elements/terminal/

## 概述

终端元素实现，提供 ANSI 终端模拟、命令执行和交互式 Shell 能力。

## 模块列表

- `html_terminal_element.h/cpp` - DOM 元素类，终端功能入口
- `ansi_parser.h/cpp` - ANSI 转义序列状态机解析器
- `terminal_buffer.h/cpp` - 终端专用缓冲区，存储 Cell 数据
- `terminal_renderer.h/cpp` - 终端渲染器，继承自 VirtualScrollRenderer
- `pty_backend.h` - PTY 抽象接口
- `conpty_backend.cpp` - Windows ConPTY 实现
- `unix_pty_backend.cpp` - Unix forkpty 实现
- `command_executor.h/cpp` - 单次命令执行器

## 设计目标

1. **ANSI 兼容** - 支持常用 ANSI 转义序列（颜色、样式、光标）
2. **高性能** - 10 万行数据流畅滚动
3. **跨平台** - Windows ConPTY / Unix forkpty

## 依赖关系

- 依赖: virtual_text (共享核心层), Skia
- 被依赖: DOM 绑定层

## 使用示例

```html
<terminal rows="24" cols="80" scrollback="10000"></terminal>
```

```javascript
const term = document.querySelector('terminal');
term.write('Hello, World!\n');
term.write('\x1b[31mRed text\x1b[0m\n');

// 执行命令
term.execute('ls -la');

// 启动交互式 shell
term.startShell('/bin/bash');
term.sendInput('echo hello\n');
```
