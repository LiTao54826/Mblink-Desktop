# Design Document

## Overview

本设计文档描述 CSS `clip-path` 属性的实现方案。实现将遵循现有代码架构，在 StyleResolver 中解析属性值，在 RenderObject 中应用裁剪。

## Architecture

### 数据结构

```cpp
// core/render/css_clip_path.h

// 裁剪形状类型
enum class ClipPathType {
    NONE,
    INSET,
    CIRCLE,
    ELLIPSE,
    POLYGON
};

// 填充规则
enum class FillRule {
    NONZERO,
    EVENODD
};

// 位置值（用于 circle/ellipse 的 at 参数）
struct ClipPosition {
    CSSLength x;
    CSSLength y;
    bool is_keyword_center = true;  // 默认居中
};

// Inset 裁剪
struct ClipInset {
    CSSLength top;
    CSSLength right;
    CSSLength bottom;
    CSSLength left;
    CSSLength border_radius;  // round 参数
    bool has_round = false;
};

// Circle 裁剪
struct ClipCircle {
    CSSLength radius;
    bool use_closest_side = false;
    bool use_farthest_side = false;
    ClipPosition position;
};

// Ellipse 裁剪
struct ClipEllipse {
    CSSLength radius_x;
    CSSLength radius_y;
    bool use_closest_side_x = false;
    bool use_closest_side_y = false;
    ClipPosition position;
};

// Polygon 裁剪
struct ClipPolygon {
    std::vector<std::pair<CSSLength, CSSLength>> points;
    FillRule fill_rule = FillRule::NONZERO;
};

// 统一的 ClipPath 结构
struct CSSClipPath {
    ClipPathType type = ClipPathType::NONE;
    
    // 根据 type 使用对应的数据
    ClipInset inset;
    ClipCircle circle;
    ClipEllipse ellipse;
    ClipPolygon polygon;
    
    // 转换为 SkPath
    SkPath ToSkPath(const SkRect& bounds) const;
    
    bool IsNone() const { return type == ClipPathType::NONE; }
};
```

### 文件结构

```
core/render/
├── css_clip_path.h      # ClipPath 数据结构定义
├── css_clip_path.cpp    # ClipPath 解析和转换实现
├── style_resolver.cpp   # 添加 clip-path 解析
├── render_object.h      # ComputedStyle 添加 clip_path 字段
└── render_object.cpp    # Paint 中应用裁剪
```

## Implementation Details

### 1. 解析流程

```
CSS 字符串 → StyleResolver::ResolveClipPath() → CSSClipPath 结构
```

解析器需要处理：
- 函数名识别：`inset`, `circle`, `ellipse`, `polygon`
- 参数解析：长度值、百分比、关键字
- 位置解析：`at` 关键字后的坐标

### 2. 渲染流程

```
RenderObject::Paint()
  ├── 检查 clip_path 是否有效
  ├── 计算 SkPath（基于元素 bounds）
  ├── canvas->save()
  ├── canvas->clipPath(path)
  ├── 绘制元素内容和子元素
  └── canvas->restore()
```

### 3. 坐标计算

- 百分比值相对于元素的 border-box 计算
- `closest-side` / `farthest-side` 需要根据圆心位置动态计算
- polygon 的坐标原点是元素的左上角

## API Design

### StyleResolver 扩展

```cpp
// style_resolver.cpp
void StyleResolver::ResolveClipPath(const std::string& value, ComputedStyle& style);

// 辅助函数
CSSClipPath ParseClipPath(const std::string& value);
ClipInset ParseInset(const std::string& params);
ClipCircle ParseCircle(const std::string& params);
ClipEllipse ParseEllipse(const std::string& params);
ClipPolygon ParsePolygon(const std::string& params);
```

### RenderObject 扩展

```cpp
// render_object.h - ComputedStyle
std::optional<CSSClipPath> clip_path;

// render_object.cpp
void RenderObject::ApplyClipPath(SkCanvas* canvas, const SkRect& bounds);
```

## Testing Strategy

### 属性测试

1. **解析测试**：验证各种 clip-path 值能正确解析
2. **边界测试**：验证裁剪区域计算正确
3. **渲染测试**：验证裁剪效果正确应用

### 测试用例

```cpp
// tests/property/render/test_clip_path_properties.cpp

// Property 1: clip-path none 不应用裁剪
// Property 2: inset 正确计算裁剪区域
// Property 3: circle 正确计算圆形区域
// Property 4: ellipse 正确计算椭圆区域
// Property 5: polygon 正确计算多边形区域
// Property 6: 百分比值正确相对于元素尺寸计算
```

## Dependencies

- Skia: `SkPath`, `SkCanvas::clipPath()`
- 现有模块: `CSSLength`, `StyleResolver`, `RenderObject`

## Risks and Mitigations

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| 复杂多边形性能 | 渲染变慢 | 缓存 SkPath，仅在样式变化时重新计算 |
| 解析边界情况 | 解析失败 | 完善的错误处理，回退到 none |
| 与 transform 交互 | 裁剪位置错误 | 确保在正确的坐标空间应用裁剪 |

