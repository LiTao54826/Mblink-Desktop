# CSS 属性支持补齐计划

## 概述

本计划旨在逐步补齐 MBink 渲染引擎中缺失的常用 CSS 属性支持，提升与现代 Web 标准的兼容性。

## 优先级划分

### P0 - 高优先级（影响基本布局和交互）

| 属性 | 说明 | 预计工作量 | 状态 |
|------|------|-----------|------|
| `outline` | 轮廓线，常用于焦点状态 | 2天 | 待开发 |
| `text-transform` | 文本大小写转换 | 1天 | 待开发 |
| `pointer-events` | 控制元素是否响应鼠标事件 | 1天 | 待开发 |
| `user-select` | 控制文本是否可选中 | 1天 | 待开发 |
| `word-break` | 单词换行规则 | 1天 | 待开发 |

### P1 - 中优先级（增强视觉效果）

| 属性 | 说明 | 预计工作量 | 状态 |
|------|------|-----------|------|
| `object-fit` | 图片/视频适应方式 | 2天 | 待开发 |
| `object-position` | 图片/视频位置 | 1天 | 待开发 |
| `aspect-ratio` | 宽高比 | 1天 | 待开发 |
| `list-style-*` | 列表样式 | 2天 | 待开发 |
| `clip-path` | 裁剪路径 | 3天 | 待开发 |

### P2 - 低优先级（高级功能）

| 属性 | 说明 | 预计工作量 | 状态 |
|------|------|-----------|------|
| `animation-*` | CSS 动画 | 5天 | 待开发 |
| `@keyframes` | 关键帧定义 | 3天 | 待开发 |
| `columns-*` | 多列布局 | 3天 | 待开发 |
| `scroll-snap-*` | 滚动吸附 | 2天 | 待开发 |
| `mask-*` | 遮罩效果 | 3天 | 待开发 |

---

## 详细实现计划

### Phase 1: 基础交互属性 (1周)

#### 1.1 outline 属性族

**涉及属性：**
- `outline`
- `outline-width`
- `outline-style`
- `outline-color`
- `outline-offset`

**实现步骤：**

1. 在 `ComputedStyle` 中添加字段：
```cpp
// core/render/computed_style.h
float outline_width = 0.0f;
std::string outline_style = "none";  // none, solid, dashed, dotted, double
std::string outline_color = "currentColor";
float outline_offset = 0.0f;
```

2. 在 `StyleResolver::ParseStyleProperty` 中添加解析：
```cpp
else if (property == "outline") {
    // 解析简写属性: outline: [width] [style] [color]
}
else if (property == "outline-width") {
    style.outline_width = CSSValue::ParseLength(resolved_value).ToPx(...);
}
// ... 其他属性
```

3. 在 `RenderObject::Paint` 中添加绘制逻辑：
```cpp
void RenderObject::PaintOutline(SkCanvas* canvas) {
    if (style.outline_style == "none" || style.outline_width <= 0) return;
    // 在边框外绘制轮廓
}
```

#### 1.2 text-transform 属性

**实现步骤：**

1. 在 `ComputedStyle` 中添加：
```cpp
std::string text_transform = "none";  // none, uppercase, lowercase, capitalize
```

2. 在 `RenderText::Paint` 中应用转换：
```cpp
std::string TransformText(const std::string& text, const std::string& transform) {
    if (transform == "uppercase") return ToUpperCase(text);
    if (transform == "lowercase") return ToLowerCase(text);
    if (transform == "capitalize") return Capitalize(text);
    return text;
}
```

#### 1.3 pointer-events 属性

**实现步骤：**

1. 在 `ComputedStyle` 中添加：
```cpp
std::string pointer_events = "auto";  // auto, none, visiblePainted, etc.
```

2. 在 `InputHandler::HitTest` 中检查：
```cpp
if (element->GetComputedStyle().pointer_events == "none") {
    // 跳过此元素，继续检查下层元素
}
```

#### 1.4 user-select 属性

**实现步骤：**

1. 在 `ComputedStyle` 中添加：
```cpp
std::string user_select = "auto";  // auto, none, text, all
```

2. 在文本选择逻辑中检查此属性

#### 1.5 word-break 属性

**实现步骤：**

1. 在 `ComputedStyle` 中添加：
```cpp
std::string word_break = "normal";  // normal, break-all, keep-all, break-word
```

2. 在 `RenderText::Layout` 中应用换行规则

---

### Phase 2: 媒体和布局属性 (1周)

#### 2.1 object-fit / object-position

用于控制 `<img>` 和 `<video>` 元素的内容适应方式。

**涉及属性：**
- `object-fit`: fill, contain, cover, none, scale-down
- `object-position`: 位置值

**实现位置：** `RenderImage::Paint`

#### 2.2 aspect-ratio

保持元素宽高比。

**实现步骤：**

1. 在 `ComputedStyle` 中添加：
```cpp
std::optional<float> aspect_ratio;  // width / height
```

2. 在布局计算中应用：
```cpp
if (style.aspect_ratio.has_value() && style.height.IsAuto()) {
    computed_height = computed_width / style.aspect_ratio.value();
}
```

#### 2.3 list-style 属性族

**涉及属性：**
- `list-style`
- `list-style-type`: disc, circle, square, decimal, none, etc.
- `list-style-position`: inside, outside
- `list-style-image`: url(...)

**实现位置：** 新建 `RenderListItem` 类

---

### Phase 3: 高级视觉效果 (2周)

#### 3.1 clip-path

支持基本形状裁剪。

**支持的值：**
- `inset(top right bottom left)`
- `circle(radius at x y)`
- `ellipse(rx ry at x y)`
- `polygon(x1 y1, x2 y2, ...)`

**实现步骤：**

1. 解析 clip-path 值
2. 在 Paint 前设置 SkCanvas 裁剪区域

#### 3.2 CSS 动画

**涉及属性：**
- `animation-name`
- `animation-duration`
- `animation-timing-function`
- `animation-delay`
- `animation-iteration-count`
- `animation-direction`
- `animation-fill-mode`
- `animation-play-state`

**实现步骤：**

1. 解析 `@keyframes` 规则
2. 创建 `CSSAnimation` 类管理动画状态
3. 在渲染循环中更新动画
4. 应用插值后的样式值

---

## 测试计划

每个属性实现后需要：

1. **单元测试**：测试属性解析
2. **渲染测试**：测试视觉效果
3. **集成测试**：测试与其他属性的交互

### 测试文件结构

```
tests/
├── unit/
│   └── render/
│       ├── test_outline.cpp
│       ├── test_text_transform.cpp
│       └── test_clip_path.cpp
└── render/
    ├── test_outline_rendering.cpp
    └── test_animation.cpp
```

---

## 时间线

| 阶段 | 内容 | 预计时间 |
|------|------|---------|
| Phase 1 | 基础交互属性 | 1周 |
| Phase 2 | 媒体和布局属性 | 1周 |
| Phase 3 | 高级视觉效果 | 2周 |
| 测试和修复 | 全面测试 | 1周 |

**总计：约 5 周**

---

## 参考资源

- [MDN CSS Reference](https://developer.mozilla.org/en-US/docs/Web/CSS/Reference)
- [CSS Specification](https://www.w3.org/Style/CSS/)
- [Skia Documentation](https://skia.org/docs/)
