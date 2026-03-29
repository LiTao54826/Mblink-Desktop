# Render Module | 渲染模块

## Overview | 概览

Skia-based 2D rendering infrastructure for the current MBink source tree.
当前 MBink 源码树中的基于 Skia 的 2D 渲染基础设施。

## Directory Layout | 目录结构

- `animation/` — animation system / 动画系统
- `canvas/` — Canvas 2D API / Canvas 2D API
- `css/` — CSS-related rendering data / CSS 相关渲染数据
- `image/` — image loading and rendering / 图像加载与渲染
- `layer/` — layers and composition / 图层与合成
- `objects/` — render object types / 渲染对象类型
- `painters/` — paint helpers / 绘制组件
- `pipeline/` — render pipeline / 渲染管线
- `text/` — text rendering / 文本渲染
- `utils/` — helper types / 工具类型

## Core Areas | 核心区域

- `RenderObject` hierarchy / `RenderObject` 层级
- render pipeline coordination / 渲染管线协调
- color / paint / geometry utilities / 颜色、画笔与几何工具

## Example | 示例

```cpp
#include "core/render/utils/color.h"
#include "core/render/utils/paint.h"

SkColor red = Color::FromHex("#FF0000");
Paint paint;
paint.SetColor(red);
paint.SetStyle(PaintStyle::FILL);
```

## Build | 构建

```bash
cmake -B build
cmake --build build --target mbink_render
```

## Notes | 说明

- exact module coverage should be verified against source files
  具体模块覆盖范围应以源码为准
- this page should not be used to imply all render paths are production-complete
  本页不应被用来暗示所有渲染路径都已达到生产级
