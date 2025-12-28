# Implementation Plan

## Phase 0: 创建开发规范文档 ✅

- [x] 1. 创建强制开发规范文档
  - [x] 1.1 创建 docs/CODE_STRUCTURE_STANDARDS.md 规范文档
  - [x] 1.2 创建 .kiro/steering/code-structure.md 引导文件

## Phase 1: 创建编辑子系统目录 ✅

- [x] 2. 创建 core/editing/ 目录结构
  - [x] 2.1 创建目录和 CMakeLists.txt
  - [x] 2.2 移动 selection_manager.h/cpp
  - [x] 2.3 移动 clipboard_manager.h/cpp
  - [x] 2.4 移动 contenteditable_handler.h/cpp
  - [x] 2.5 移动 contenteditable_controller.h/cpp
  - [x] 2.6 移动 drag_manager.h/cpp
  - [x] 2.7 创建 README.md

- [x] 3. Checkpoint - 编译通过

## Phase 2: 验证脚本和文档 ✅

- [x] 4. 创建代码结构验证脚本
  - [x] 4.1 创建 scripts/check_code_structure.py
  - [x] 4.2 创建 scripts/check_circular_deps.py
  - [x] 4.3 集成到 CI/CD 流程

- [x] 5. 创建缺失的 README.md 文档（18 个）

- [x] 6. 为大文件添加文档说明（7 个）

- [x] 7. Checkpoint - 编译通过，代码结构检查通过

---

## Phase 3: 准备 event 目录重组（当前阶段）

- [x] 8. 创建 event 子目录结构（仅目录和 README）
  - [x] 8.1 创建 core/event/loop/ 目录和 README.md
  - [x] 8.2 创建 core/event/dispatch/ 目录和 README.md
  - [x] 8.3 创建 core/event/input/ 目录和 README.md
  - [x] 8.4 创建 core/event/types/ 目录和 README.md
  - [x] 8.5 更新 core/event/README.md 说明目录结构

- [x] 9. Checkpoint - 编译通过

---

## Phase 4: 提取 event_loop.cpp 事件分发器 ✅

> **注意**: 以下任务涉及从 3920 行的 event_loop.cpp 提取代码

- [x] 10. 提取鼠标事件分发器
  - [x] 10.1 创建 core/event/dispatch/mouse_event_dispatcher.h
  - [x] 10.2 创建 core/event/dispatch/mouse_event_dispatcher.cpp
  - [x] 10.3 更新 CMakeLists.txt
  - [x] 10.4 实现 UpdateHoverChain, SendEvents, UpdateMouseCursor 方法

- [x] 11. Checkpoint - 编译通过

- [x] 12. 提取键盘事件分发器
  - [x] 12.1 创建 core/event/dispatch/keyboard_event_dispatcher.h
  - [x] 12.2 创建 core/event/dispatch/keyboard_event_dispatcher.cpp
  - [x] 12.3 更新 CMakeLists.txt
  - [x] 12.4 实现 HandleShortcut, SDLKeycodeToDOMKey, SDLScancodeToDOMCode 方法

- [x] 13. Checkpoint - 编译通过

---

## Phase 5: DOM 目录重组 ✅

- [x] 14. DOM 目录重组
  - [x] 14.1 创建 core/dom/elements/ 目录和 README.md
  - [x] 14.2 移动 24 个 HTML 元素文件到 elements/ 目录
  - [x] 14.3 更新所有 include 路径
  - [x] 14.4 更新 CMakeLists.txt

- [x] 15. Checkpoint - 编译通过

- [ ] 16. 重组 render 目录（已有子目录结构，暂不需要）

- [ ] 17. 重组 window 目录（文件数量符合规范，暂不需要）

---

## 完成状态

### 已完成 ✅
- Phase 0: 开发规范文档
- Phase 1: 编辑子系统（core/editing/）
- Phase 2: 验证脚本和文档
- Phase 3: event 子目录结构准备
- Phase 4: 事件分发器（mouse_event_dispatcher, keyboard_event_dispatcher）
- Phase 5: DOM elements 子目录（移动 24 个文件）

### 验证结果
- 编译: ✅ 通过（全部模块）
- 代码结构检查: ✅ 通过（55 警告，0 错误）

### 目录文件数量变化
- core/dom: 42 → 18 个文件
- core/dom/elements: 新建，24 个文件

### 项目统计
- 创建的新目录: 7 个（editing, event/loop, event/dispatch, event/input, event/types, dom/elements）
- 创建的新文件: 约 35 个（包括 README.md、验证脚本、分发器等）
- 移动的文件: 36 个（editing 12 个 + dom/elements 24 个）
