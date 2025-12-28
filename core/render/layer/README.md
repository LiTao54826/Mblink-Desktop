# Layer 子模块

图层系统，管理渲染层级和合成。

## 文件列表

| 文件 | 描述 |
|------|------|
| `layer.h/cpp` | 图层类，管理 z-index 范围内的渲染对象 |
| `layer_manager.h/cpp` | 图层管理器，负责图层创建和排序 |
| `fbo_manager.h/cpp` | FBO（帧缓冲对象）管理器 |

## 依赖关系

### 依赖的模块
- `core/dom` - DOM 元素
- `core/event/input` - Hit Testing

### 被依赖的模块
- `../` - 渲染对象
- `core/window` - 窗口渲染
- `core/compositor` - 合成器

## 使用示例

```cpp
#include "core/render/layer/layer_manager.h"

// 获取图层管理器
auto& layer_manager = LayerManager::Instance();

// 创建图层
auto layer = layer_manager.CreateLayer(z_min, z_max);
```
