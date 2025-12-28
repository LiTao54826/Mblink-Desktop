# Event Types 子模块

事件类型定义。

## 文件列表

| 文件 | 描述 |
|------|------|
| `event.h/cpp` | 基础事件类 |
| `event_types.h/cpp` | 事件类型枚举和工具函数 |
| `mouse_event.h/cpp` | 鼠标事件类 |
| `keyboard_event.h/cpp` | 键盘事件类 |
| `data_transfer.h/cpp` | 拖拽数据传输（DataTransfer API） |

## 依赖关系

### 依赖的模块
- `core/dom` - DOM 元素（事件目标）
- `SDL3` - 底层事件类型

### 被依赖的模块
- `../loop/` - 事件循环
- `../dispatch/` - 事件分发器
- `core/dom` - DOM 事件处理
- `core/editing` - 拖拽管理

## 使用示例

```cpp
#include "core/event/types/mouse_event.h"
#include "core/event/types/keyboard_event.h"

// 创建鼠标事件
auto mouse_event = std::make_shared<MouseEvent>("click", 100.0f, 200.0f);

// 创建键盘事件
auto key_event = std::make_shared<KeyboardEvent>("keydown", SDLK_RETURN);
```
