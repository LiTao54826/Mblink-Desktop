# DevTools Subsystem | 开发者工具子系统

## Overview | 概览

Developer tooling for inspection and debugging in the current source tree.
当前源码树中的检查与调试工具模块。

## Main Files | 主要文件

- `devtools_manager.h/cpp` — manager / 管理器
- `devtools_panel.h/cpp` — panel rendering / 面板渲染
- `devtools_state.h/cpp` — shared state / 状态管理

## Subdirectories | 子目录

- `editor/` — style editing / 样式编辑
- `inspector/` — element inspection / 元素检查
- `search/` — search / 搜索
- `serializer/` — DOM serialization / DOM 序列化
- `styles/` — style panels / 样式面板

## Dependencies | 依赖关系

Depends on | 依赖：

- `core/dom`
- `core/render`
- `core/window`
- `Skia`

## Features | 功能

- element inspection / 元素检查
- style viewing and editing / 样式查看与编辑
- box model visualization / Box Model 可视化
- DOM tree navigation / DOM 树导航
