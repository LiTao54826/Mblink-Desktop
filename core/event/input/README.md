# Event Input 子模块

输入处理组件。

## 计划包含的文件

当重构完成后，此目录将包含：
- `input_handler.h/cpp` - 输入处理器
- `hit_testing.h/cpp` - 命中测试
- `focus_manager.h/cpp` - 焦点管理器
- `scrollbar_controller.h/cpp` - 滚动条控制器（待提取）

## 当前状态

这些文件目前位于 `core/event/` 根目录。
由于涉及大量 include 路径更新，移动操作暂缓执行。

## 功能

- 输入事件预处理
- 元素命中测试（Hit Testing）
- 焦点管理和 Tab 导航
- 滚动条拖动控制
