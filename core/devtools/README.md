# DevTools Subsystem

开发者工具子系统，提供元素检查、样式编辑等调试功能。

## 模块列表

| 文件 | 描述 |
|------|------|
| `devtools_manager.h/cpp` | DevTools 管理器，协调各组件 |
| `devtools_panel.h/cpp` | DevTools 面板渲染 |
| `devtools_state.h/cpp` | DevTools 状态管理 |

### 子目录

| 目录 | 描述 |
|------|------|
| `editor/` | 样式编辑器 |
| `inspector/` | 元素检查器 |
| `search/` | 搜索功能 |
| `serializer/` | DOM 序列化 |
| `styles/` | 样式面板 |

## 依赖关系

### 依赖的模块
- `core/dom` - DOM 树访问
- `core/render` - 渲染对象检查
- `core/window` - 窗口集成
- `Skia` - UI 渲染

### 被依赖的模块
- `core/event` - 事件处理集成

## 功能说明

DevTools 提供类似浏览器开发者工具的功能：
- 元素检查和选择
- 样式查看和编辑
- Box Model 可视化
- DOM 树导航
