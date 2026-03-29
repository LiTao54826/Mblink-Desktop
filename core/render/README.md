# MBink 渲染引擎

基于 Skia 的高性能 2D 图形渲染系统。

## 目录结构

```
core/render/
├── CMakeLists.txt
├── README.md
├── animation/      # 动画系统 (18 文件)
├── canvas/         # Canvas 2D API (10 文件)
├── css/            # CSS 相关 (8 文件)
├── image/          # 图片处理 (8 文件)
├── layer/          # 图层系统 (6 文件)
├── objects/        # 渲染对象 (18 文件)
├── painters/       # 绘制器 (8 文件)
├── pipeline/       # 渲染管线 (12 文件)
├── text/           # 文本渲染 (6 文件)
└── utils/          # 工具类 (24 文件)
```

## 子目录说明

| 目录 | 描述 | 主要文件 |
|------|------|----------|
| animation/ | CSS 动画和过渡 | animation.h, keyframes.h, transition.h |
| canvas/ | HTML Canvas 2D API | canvas_rendering_context_2d.h |
| css/ | CSS 值解析和样式 | css_value.h, style_resolver.h |
| image/ | 图片加载和渲染 | image_loader.h, image_renderer.h |
| layer/ | 图层和合成 | layer.h, layer_manager.h |
| objects/ | 渲染对象类型 | render_object.h, render_block.cpp |
| painters/ | 绘制器组件 | box_renderer.h, background_painter.h |
| pipeline/ | 渲染管线控制 | render_pipeline.h, renderer.h |
| text/ | 文本和字体 | text_renderer.h, font_manager.h |
| utils/ | 工具和辅助类 | color.h, paint.h, shapes.h |

## 核心类

### 渲染对象 (objects/)
- `RenderObject` - 渲染对象基类
- `RenderBlock` - 块级元素
- `RenderInline` - 行内元素
- `RenderText` - 文本渲染

### 渲染管线 (pipeline/)
- `RenderPipeline` - 渲染管线协调
- `Renderer` - 主渲染器
- `RenderTreeSynchronizer` - DOM/渲染树同步

### 工具类 (utils/)
- `Color` - 颜色解析和转换
- `Paint` - 画笔封装
- `Shapes` - 基础图形绘制
- `DirtyRegion` - 脏区域管理

## 使用示例

```cpp
#include "core/render/utils/color.h"
#include "core/render/utils/paint.h"
#include "core/render/objects/render_object.h"

// 颜色
SkColor red = Color::FromHex("#FF0000");
SkColor blue = Color::FromName("blue");

// 画笔
Paint paint;
paint.SetColor(red);
paint.SetStyle(PaintStyle::FILL);
```

## 编译

```bash
cmake -B build
cmake --build build --target mbink_render
```
