# Event Loop 子模块

事件循环核心组件。

## 计划包含的文件

当重构完成后，此目录将包含：
- `event_loop.h/cpp` - 主事件循环
- `frame_controller.h/cpp` - 帧控制器
- `task_scheduler.h/cpp` - 任务调度器

## 当前状态

这些文件目前位于 `core/event/` 根目录。
由于涉及大量 include 路径更新，移动操作暂缓执行。

## 依赖关系

- 依赖 `input/` 子模块的输入处理
- 依赖 `dispatch/` 子模块的事件分发
- 依赖 `types/` 子模块的事件类型定义
