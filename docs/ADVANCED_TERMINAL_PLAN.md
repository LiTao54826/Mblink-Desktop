# 高级终端功能规划

## 概述

基于现有 `<terminal>` 元素，打造现代化、高效的终端体验，参考 Warp、iTerm2 等先进终端的设计理念。

## 功能规划

### Phase 1: 命令块化 (基础)
- [ ] Prompt 检测与命令分割
- [ ] 每个命令作为独立卡片显示
- [ ] 退出码标记（成功/失败颜色）
- [ ] 执行时间戳和耗时显示
- [ ] 块级折叠/展开

### Phase 2: 交互增强
- [ ] 右键上下文菜单
  - 复制命令/输出
  - 重新执行命令
  - 复制为 Markdown
- [ ] 点击块展开详情视图
- [ ] 快捷键支持 (Ctrl+L, Ctrl+R 等)
- [ ] URL/文件路径可点击

### Phase 3: 智能优化
- [ ] 重复行智能合并 (`[repeated 100x]`)
- [ ] 大输出虚拟滚动优化
- [ ] 实时输出过滤 (grep 式)
- [ ] 搜索高亮
- [ ] 语法高亮 (JSON, 日志等)

### Phase 4: 高级功能
- [ ] 分屏/标签页支持
- [ ] 命令历史侧边栏
- [ ] 命令模板/片段保存
- [ ] 输出导出 (txt/html/markdown)
- [ ] 书签标记

### Phase 5: AI 集成
- [ ] 命令智能补全
- [ ] 错误解释与修复建议
- [ ] 自然语言转命令
- [ ] 输出内容摘要

## 技术要点

### Prompt 检测
```cpp
// 常见 prompt 模式
// bash: user@host:path$
// zsh: user@host path %
// cmd: C:\path>
// powershell: PS C:\path>
```

### 数据结构
```cpp
struct CommandBlock {
    std::string command;           // 输入的命令
    std::string output;            // 命令输出
    int exit_code;                 // 退出码
    std::chrono::time_point start; // 开始时间
    std::chrono::time_point end;   // 结束时间
    bool collapsed;                // 是否折叠
};
```

## 参考产品
- [Warp](https://www.warp.dev/) - 命令块化先驱
- [Fig](https://fig.io/) - 命令补全
- [iTerm2](https://iterm2.com/) - macOS 高级终端
- [Windows Terminal](https://github.com/microsoft/terminal) - 现代化设计

## 优先级
1. 命令块化 - 核心体验改进
2. 右键菜单 - 基础交互
3. 重复合并 - 日志场景优化
4. 搜索功能 - 效率提升
5. AI 集成 - 差异化功能
