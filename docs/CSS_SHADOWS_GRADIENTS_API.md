# CSS 阴影和渐变 API 文档

> **版本**: 1.0  
> **日期**: 2025-11-14  
> **状态**: ✅ 已实现

---

## 📋 目录

1. [概述](#概述)
2. [Box Shadow (盒阴影)](#box-shadow-盒阴影)
3. [Text Shadow (文本阴影)](#text-shadow-文本阴影)
4. [Linear Gradient (线性渐变)](#linear-gradient-线性渐变)
5. [Radial Gradient (径向渐变)](#radial-gradient-径向渐变)
6. [C++ API](#c-api)
7. [性能指标](#性能指标)
8. [示例代码](#示例代码)

---

## 概述

MBink 现已完整支持 CSS 阴影和渐变特性，包括：

- ✅ **Box Shadow**: 盒阴影（外阴影和内阴影）
- ✅ **Text Shadow**: 文本阴影
- ✅ **Linear Gradient**: 线性渐变
- ✅ **Radial Gradient**: 径向渐变

所有特性均使用 Skia 图形库实现，提供高性能的硬件加速渲染。

---

## Box Shadow (盒阴影)

### CSS 语法

```css
box-shadow: [inset] offset-x offset-y blur-radius spread-radius color;
```

### 参数说明

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `inset` | 关键字 | 否 | 内阴影（默认为外阴影） |
| `offset-x` | 长度 | 是 | 水平偏移量（正值向右，负值向左） |
| `offset-y` | 长度 | 是 | 垂直偏移量（正值向下，负值向上） |
| `blur-radius` | 长度 | 否 | 模糊半径（默认 0，值越大越模糊） |
| `spread-radius` | 长度 | 否 | 扩展半径（默认 0，正值扩大，负值缩小） |
| `color` | 颜色 | 是 | 阴影颜色 |

### 示例

```css
/* 基础外阴影 */
box-shadow: 2px 2px 8px rgba(0, 0, 0, 0.2);

/* 内阴影 */
box-shadow: inset 0 2px 8px rgba(0, 0, 0, 0.2);

/* 多重阴影 */
box-shadow: 
    0 2px 4px rgba(0, 0, 0, 0.1),
    0 4px 8px rgba(0, 0, 0, 0.1),
    0 8px 16px rgba(0, 0, 0, 0.1);

/* 带扩展半径的阴影 */
box-shadow: 0 0 0 4px rgba(0, 123, 255, 0.3);
```

### 特性支持

- ✅ 单个和多个阴影
- ✅ 外阴影 (outset)
- ✅ 内阴影 (inset)
- ✅ 模糊效果
- ✅ 扩展半径
- ✅ 圆角边框的阴影
- ✅ 任意颜色（包括透明度）

---

## Text Shadow (文本阴影)

### CSS 语法

```css
text-shadow: offset-x offset-y blur-radius color;
```

### 参数说明

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `offset-x` | 长度 | 是 | 水平偏移量 |
| `offset-y` | 长度 | 是 | 垂直偏移量 |
| `blur-radius` | 长度 | 否 | 模糊半径（默认 0） |
| `color` | 颜色 | 是 | 阴影颜色 |

### 示例

```css
/* 基础文本阴影 */
text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);

/* 发光效果 */
text-shadow: 0 0 10px rgba(0, 123, 255, 0.8);

/* 多重文本阴影 */
text-shadow: 
    2px 2px 0 #ff0000,
    4px 4px 0 #00ff00,
    6px 6px 0 #0000ff;

/* 3D 效果 */
text-shadow: 
    1px 1px 0 #ccc,
    2px 2px 0 #bbb,
    3px 3px 0 #aaa,
    4px 4px 0 #999,
    5px 5px 10px rgba(0, 0, 0, 0.5);
```

### 特性支持

- ✅ 单个和多个文本阴影
- ✅ 模糊效果
- ✅ 任意颜色
- ✅ 正确的渲染顺序（从后往前）

---

## Linear Gradient (线性渐变)

### CSS 语法

```css
background-image: linear-gradient(angle|direction, color-stop1, color-stop2, ...);
```

### 参数说明

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `angle` | 角度 | 否 | 渐变角度（如 `45deg`，默认 180deg） |
| `direction` | 关键字 | 否 | 渐变方向（`to top`, `to right`, `to bottom`, `to left`） |
| `color-stop` | 颜色 [位置] | 是 | 颜色停止点（位置可选，如 `red 0%`, `blue 100%`） |

### 示例

```css
/* 基础线性渐变 */
background-image: linear-gradient(45deg, #ff6b6b, #4ecdc4);

/* 垂直渐变 */
background-image: linear-gradient(to bottom, #667eea, #764ba2);

/* 水平渐变 */
background-image: linear-gradient(to right, #f093fb, #f5576c);

/* 多色渐变 */
background-image: linear-gradient(45deg, 
    #ff0000, 
    #ff7f00, 
    #ffff00, 
    #00ff00, 
    #0000ff, 
    #4b0082, 
    #9400d3);

/* 带位置的色标 */
background-image: linear-gradient(90deg, 
    #ff6b6b 0%, 
    #4ecdc4 50%, 
    #45b7d1 100%);
```

### 特性支持

- ✅ 角度（如 `45deg`, `90deg`）
- ✅ 方向关键字（`to top`, `to right`, `to bottom`, `to left`）
- ✅ 多个色标
- ✅ 色标位置（百分比）
- ✅ 任意颜色

---

## Radial Gradient (径向渐变)

### CSS 语法

```css
background-image: radial-gradient(shape, color-stop1, color-stop2, ...);
```

### 参数说明

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `shape` | 关键字 | 否 | 形状（`circle` 或 `ellipse`，默认 `ellipse`） |
| `color-stop` | 颜色 [位置] | 是 | 颜色停止点 |

### 示例

```css
/* 基础径向渐变 */
background-image: radial-gradient(circle, #ff6b6b, #4ecdc4);

/* 椭圆渐变 */
background-image: radial-gradient(ellipse, #667eea, #764ba2);

/* 多色径向渐变 */
background-image: radial-gradient(circle, 
    #ff0000, 
    #ffff00, 
    #00ff00, 
    #0000ff);

/* 带位置的色标 */
background-image: radial-gradient(circle, 
    #ff6b6b 0%, 
    #4ecdc4 50%, 
    #45b7d1 100%);
```

### 特性支持

- ✅ 圆形 (circle)
- ✅ 椭圆形 (ellipse)
- ✅ 多个色标
- ✅ 色标位置
- ✅ 任意颜色

---

## C++ API

### 数据结构

#### CSSBoxShadow

```cpp
struct CSSBoxShadow {
    float offset_x;      // 水平偏移
    float offset_y;      // 垂直偏移
    float blur_radius;   // 模糊半径
    float spread_radius; // 扩展半径
    SkColor color;       // 阴影颜色
    bool inset;          // 是否为内阴影
};
```

#### CSSTextShadow

```cpp
struct CSSTextShadow {
    float offset_x;    // 水平偏移
    float offset_y;    // 垂直偏移
    float blur_radius; // 模糊半径
    SkColor color;     // 阴影颜色
};
```

#### CSSLinearGradient

```cpp
struct CSSLinearGradient {
    float angle;                          // 渐变角度（度）
    std::vector<CSSGradientStop> stops;   // 色标列表
};
```

#### CSSRadialGradient

```cpp
struct CSSRadialGradient {
    float center_x;                       // 中心 X 坐标（0.0-1.0）
    float center_y;                       // 中心 Y 坐标（0.0-1.0）
    bool is_circle;                       // 是否为圆形
    std::vector<CSSGradientStop> stops;   // 色标列表
};
```

#### CSSGradientStop

```cpp
struct CSSGradientStop {
    SkColor color;     // 颜色
    float position;    // 位置（0.0-1.0）
};
```

### 渲染器 API

#### ShadowRenderer

```cpp
class ShadowRenderer {
public:
    // 渲染盒阴影
    static void RenderBoxShadow(SkCanvas* canvas,
                               const SkRect& rect,
                               const std::vector<CSSBoxShadow>& shadows,
                               float border_radius = 0.0f);

    // 渲染带阴影的文本
    static void RenderTextWithShadow(SkCanvas* canvas,
                                    const std::string& text,
                                    const SkFont& font,
                                    float x, float y,
                                    SkColor text_color,
                                    const std::vector<CSSTextShadow>& shadows);
};
```

#### GradientRenderer

```cpp
class GradientRenderer {
public:
    // 渲染线性渐变
    static void RenderLinearGradient(SkCanvas* canvas,
                                    const SkRect& rect,
                                    const CSSLinearGradient& gradient);

    // 渲染径向渐变
    static void RenderRadialGradient(SkCanvas* canvas,
                                    const SkRect& rect,
                                    const CSSRadialGradient& gradient);
};
```

---

## 性能指标

所有性能测试在 Windows 11, Intel Core i7 上进行。

| 特性 | 性能指标 | 实际测试结果 |
|------|----------|--------------|
| Box Shadow | >100 阴影/秒 | ~100 阴影/991ms ✅ |
| Text Shadow | >100 渲染/秒 | 100次渲染(5阴影)/49ms ✅ |
| Linear Gradient | >200 渐变/秒 | 263 渐变/秒 ✅ |
| Radial Gradient | >200 渐变/秒 | 203 渐变/秒 ✅ |
| Style Resolution | <200ms/1000次 | 170ms/1000次 ✅ |

---

## 示例代码

### HTML/CSS 示例

参见 `examples/css_shadows_gradients.html`

### C++ 示例

```cpp
#include "render/shadow_renderer.h"
#include "render/gradient_renderer.h"

// 渲染盒阴影
std::vector<CSSBoxShadow> shadows = {
    {2.0f, 2.0f, 8.0f, 0.0f, SkColorSetARGB(51, 0, 0, 0), false}
};
ShadowRenderer::RenderBoxShadow(canvas, rect, shadows, 8.0f);

// 渲染线性渐变
CSSLinearGradient gradient;
gradient.angle = 45.0f;
gradient.stops = {
    {SK_ColorRED, 0.0f},
    {SK_ColorBLUE, 1.0f}
};
GradientRenderer::RenderLinearGradient(canvas, rect, gradient);
```

---

## 相关文档

- [CSS 高级特性开发计划](CSS_ADVANCED_FEATURES_PLAN.md)
- [CSS 特性任务追踪](CSS_FEATURES_TASK_TRACKER.md)
- [CSS 特性快速参考](CSS_FEATURES_QUICK_REFERENCE.md)

---

**最后更新**: 2025-11-14  
**维护者**: MBink Team

