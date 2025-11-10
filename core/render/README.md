# LightUI 渲染引擎

LightUI 的渲染引擎是一个基于 Skia 的高性能 2D 图形渲染系统，提供了完整的图形绘制、文本渲染和图片处理功能。

## 目录结构

```
core/render/
├── renderer.h/cpp           # 渲染器基类
├── render_context.h/cpp     # 渲染上下文
├── color.h/cpp              # 颜色管理
├── paint.h/cpp              # 画笔管理
├── shapes.h/cpp             # 图形绘制
├── text_renderer.h/cpp      # 文本渲染器
├── text/
│   └── font_manager.h/cpp   # 字体管理器
└── image/
    ├── image_loader.h/cpp   # 图片加载器
    ├── image_cache.h/cpp    # 图片缓存
    └── image_renderer.h/cpp # 图片渲染器
```

## 核心模块

### 1. Renderer - 渲染器基类

渲染器是所有渲染操作的基础，提供画布管理和基本变换操作。

```cpp
#include "core/render/renderer.h"

// 创建渲染表面
sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(800, 600);

// 创建渲染器
Renderer renderer(surface);

// 清空画布
renderer.Clear(SK_ColorWHITE);

// 状态管理
renderer.Save();
renderer.Translate(100, 100);
renderer.Rotate(45.0f);
// ... 绘制操作 ...
renderer.Restore();
```

### 2. Color - 颜色管理

提供多种颜色创建和转换方式。

```cpp
#include "core/render/color.h"

// RGB 颜色
SkColor red = Color::FromRGB(255, 0, 0);

// RGBA 颜色（带透明度）
SkColor semi_transparent = Color::FromRGBA(100, 150, 200, 128);

// HEX 颜色
SkColor orange = Color::FromHex("#FFA500");

// 命名颜色
SkColor blue = Color::FromName("blue");

// CSS 颜色字符串
SkColor parsed = Color::Parse("rgb(255, 128, 64)");

// 颜色转换
std::string hex = Color::ToHex(SK_ColorRED);  // "#FF0000"
```

### 3. Paint - 画笔管理

画笔控制绘制的样式和外观。

```cpp
#include "core/render/paint.h"

Paint paint;

// 设置颜色
paint.SetColor(SK_ColorRED);

// 设置透明度
paint.SetAlpha(128);

// 设置样式
paint.SetStyle(PaintStyle::STROKE);

// 描边设置
paint.SetStrokeWidth(5.0f);
paint.SetStrokeCap(StrokeCap::ROUND);
paint.SetStrokeJoin(StrokeJoin::ROUND);

// 抗锯齿
paint.SetAntiAlias(true);
```

### 4. Shapes - 图形绘制

提供各种基本图形的绘制功能。

```cpp
#include "core/render/shapes.h"

Shapes shapes(canvas);
Paint paint;

// 矩形
shapes.FillRect(50, 50, 100, 80, paint);
shapes.StrokeRect(200, 50, 100, 80, paint);

// 圆形
shapes.FillCircle(100, 200, 40, paint);
shapes.StrokeCircle(250, 200, 40, paint);

// 圆角矩形
shapes.FillRoundRect(50, 300, 100, 80, 10, paint);

// 线条
shapes.DrawLine(50, 400, 300, 400, paint);

// 自定义路径
PathBuilder path;
path.MoveTo(100, 100)
    .LineTo(150, 150)
    .QuadTo(200, 100, 250, 150)
    .Close();
shapes.FillPath(path.Build(), paint);
```

### 5. TextRenderer - 文本渲染

提供文本绘制、测量和排版功能。

```cpp
#include "core/render/text_renderer.h"
#include "core/render/text/font_manager.h"

// 初始化字体管理器
FontManager::GetInstance().Initialize();

TextRenderer text_renderer(canvas);

// 创建字体
FontDescriptor font_desc;
font_desc.family = "Arial";
font_desc.size = 24.0f;
font_desc.weight = FontWeight::BOLD;
SkFont font = FontManager::GetInstance().LoadFont(font_desc);

// 绘制文本
Paint paint;
paint.SetColor(SK_ColorBLACK);
text_renderer.DrawText("Hello, World!", 50, 100, font, paint);

// 测量文本
TextMetrics metrics = text_renderer.MeasureText("Test", font);
float width = metrics.width;
float height = metrics.height;

// 多行文本
text_renderer.DrawMultilineText(
    "This is a long text that will wrap...",
    50, 200, 400, 30, font, paint
);

// 文本装饰
float text_width = text_renderer.MeasureTextWidth("Underlined", font);
text_renderer.DrawText("Underlined", 50, 300, font, paint);
text_renderer.DrawUnderline(50, 300, text_width, paint);
```

### 6. ImageRenderer - 图片渲染

提供图片加载、缓存和渲染功能。

```cpp
#include "core/render/image/image_renderer.h"
#include "core/render/image/image_loader.h"

ImageRenderer image_renderer(canvas);

// 从文件绘制图片
image_renderer.DrawImageFromFile("image.png", 100, 100);

// 绘制缩放的图片
image_renderer.DrawImageFromFile("image.png", 100, 100, 200, 150);

// 绘制旋转的图片
sk_sp<SkImage> image = ImageLoader::LoadFromFile("image.png");
image_renderer.DrawRotatedImage(image, 100, 100, 45.0f);

// 绘制带透明度的图片
image_renderer.DrawImageWithAlpha(image, 100, 100, 0.5f);

// 缓存管理
image_renderer.SetCacheEnabled(true);
image_renderer.ClearCache();
```

## 特性

### 🎨 丰富的绘图功能
- 基本图形：矩形、圆形、椭圆、线条
- 高级图形：圆角矩形、自定义路径
- 文本渲染：单行、多行、装饰
- 图片渲染：缩放、旋转、透明度

### ⚡ 高性能
- 基于 Skia 图形库
- 字体缓存系统
- LRU 图片缓存
- 延迟图片解码

### 🔧 易用的 API
- 一致的命名规范
- Fill/Stroke/Draw 变体
- 流式 API（PathBuilder）
- RAII 资源管理

### 🌍 跨平台
- Windows (DirectWrite)
- macOS (CoreText)
- Linux (FontConfig)

## 编译

```bash
# 配置
cmake -B build

# 编译渲染模块
cmake --build build --target lightui_render

# 运行测试
cmake --build build --target test_render_engine
./build/tests/Debug/test_render_engine
```

## 示例

完整示例请参考：
- `examples/render_example.cpp` - 基本使用示例
- `tests/test_render_engine.cpp` - 功能测试示例

## API 文档

### Renderer

| 方法 | 说明 |
|------|------|
| `Clear(color)` | 清空画布 |
| `Save()` | 保存状态 |
| `Restore()` | 恢复状态 |
| `Translate(dx, dy)` | 平移 |
| `Scale(sx, sy)` | 缩放 |
| `Rotate(degrees)` | 旋转 |
| `ClipRect(rect)` | 矩形裁剪 |

### Color

| 方法 | 说明 |
|------|------|
| `FromRGB(r, g, b)` | 从 RGB 创建 |
| `FromRGBA(r, g, b, a)` | 从 RGBA 创建 |
| `FromHex(hex)` | 从 HEX 创建 |
| `FromName(name)` | 从命名颜色创建 |
| `Parse(str)` | 解析 CSS 颜色 |
| `ToHex(color)` | 转换为 HEX |

### Paint

| 方法 | 说明 |
|------|------|
| `SetColor(color)` | 设置颜色 |
| `SetAlpha(alpha)` | 设置透明度 |
| `SetStyle(style)` | 设置样式 |
| `SetStrokeWidth(width)` | 设置描边宽度 |
| `SetAntiAlias(aa)` | 设置抗锯齿 |

### Shapes

| 方法 | 说明 |
|------|------|
| `FillRect(x, y, w, h, paint)` | 填充矩形 |
| `StrokeRect(x, y, w, h, paint)` | 描边矩形 |
| `FillCircle(cx, cy, r, paint)` | 填充圆形 |
| `StrokeCircle(cx, cy, r, paint)` | 描边圆形 |
| `FillRoundRect(...)` | 填充圆角矩形 |
| `DrawLine(x1, y1, x2, y2, paint)` | 绘制线条 |
| `FillPath(path, paint)` | 填充路径 |

### TextRenderer

| 方法 | 说明 |
|------|------|
| `DrawText(text, x, y, font, paint)` | 绘制文本 |
| `DrawMultilineText(...)` | 绘制多行文本 |
| `MeasureText(text, font)` | 测量文本 |
| `MeasureTextWidth(text, font)` | 测量宽度 |
| `DrawUnderline(...)` | 绘制下划线 |
| `DrawLineThrough(...)` | 绘制删除线 |

### ImageRenderer

| 方法 | 说明 |
|------|------|
| `DrawImage(image, x, y)` | 绘制图片 |
| `DrawImage(image, x, y, w, h)` | 绘制缩放图片 |
| `DrawImageFromFile(path, x, y)` | 从文件绘制 |
| `DrawRotatedImage(...)` | 绘制旋转图片 |
| `DrawImageWithAlpha(...)` | 绘制透明图片 |

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！

