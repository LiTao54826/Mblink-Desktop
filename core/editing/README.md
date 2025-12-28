# Editing Subsystem

文本编辑子系统，负责处理所有与文本编辑相关的功能。

## 模块列表

| 文件 | 描述 |
|------|------|
| `selection_manager.h/cpp` | 文本选择管理，处理选区的创建、修改和查询 |
| `clipboard_manager.h/cpp` | 剪贴板管理，处理复制、剪切、粘贴操作 |
| `contenteditable_handler.h/cpp` | ContentEditable 事件处理，响应用户输入 |
| `contenteditable_controller.h/cpp` | ContentEditable 控制器，管理编辑状态和行为 |
| `drag_manager.h/cpp` | 拖拽管理，处理文本和元素的拖放操作 |

## 依赖关系

### 依赖的模块
- `core/dom` - DOM 节点和元素操作
- `core/event` - 事件系统
- `SDL3` - 平台剪贴板 