# Event Input 子模块

输入处理组件。

## 文件列表

| 文件 | 描述 |
|------|------|
| `input_handler.h/cpp` | 输入事件处理器 |
| `hit_testing.h/cpp` | 命中测试（Hit Testing）引擎 |
| `focus_manager.h/cpp` | 焦点管理器，处理 Tab 导航 |
| `keyboard_utils.h/cpp` | 键盘工具函数（按键映射等） |

## 依赖关系

### 依赖的模块
- `core/dom` - DOM 元素
- `core/render` - 渲染对象（用于 Hit Testing）
- `SDL3` - 底层输入事件

### 被依赖的模块
- `../loop/` - 事件循环
- `../dispatch/` - 事件分发器
- `core/editing` - 编辑子系统

## 使用示例

```cpp
#include "core/event/input/hit_testing.h"
#include "core/event/input/focus_manager.h"

// 执行命中测试
HitTesting hit_testing;
auto result = hit_testing.HitTestRenderObject(root_render, x, y, 0, 0);

// 焦点管理
FocusManager focus_manager;
focus_manager.SetFocusElement(element);
auto focused = focus_manager.GetFocusElement();
```
