# Event Types 子模块

事件类型定义。

## 计划包含的文件

当重构完成后，此目录将包含：
- `event.h/cpp` - 基础事件类
- `mouse_event.h/cpp` - 鼠标事件
- `keyboard_event.h/cpp` - 键盘事件
- `event_types.h/cpp` - 事件类型枚举
- `data_transfer.h/cpp` - 拖拽数据传输

## 当前状态

这些文件目前位于 `core/event/` 根目录。
由于涉及大量 include 路径更新，移动操作暂缓执行。

## 功能

- 事件基类定义
- 鼠标事件（MouseEvent）
- 键盘事件（KeyboardEvent）
- 事件类型枚举
- 拖拽数据传输（DataTransfer）
