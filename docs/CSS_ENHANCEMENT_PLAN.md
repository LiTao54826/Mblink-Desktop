# CSS 属性支持补齐计划

## 概述

本计划旨在逐步补齐 MBink 渲染引擎中缺失的常用 CSS 属性支持，提升与现代 Web 标准的兼容性。

## 优先级划分

### P0 - 紧急（阻塞性问题）

| 任务 | 说明 | 预计工作量 | 状态 |
|------|------|-----------|------|
| **DOM 绑定系统统一** | 合并两套重复的 DOM 绑定系统 | 3天 | ✅ 已完成 |
| ~~CSSStyleDeclaration exotic 支持~~ | ~~支持 `element.style.propertyName` 动态属性访问语法~~ | ~~2天~~ | ✅ 已有实现 |

### P1 - 高优先级（影响基本布局和交互）

| 属性 | 说明 | 预计工作量 | 状态 |
|------|------|-----------|------|
| `outline` | 轮廓线，常用于焦点状态 | 2天 | ✅ 已完成 |
| `text-transform` | 文本大小写转换 | 1天 | ✅ 已完成 |
| `pointer-events` | 控制元素是否响应鼠标事件 | 1天 | ✅ 已完成 |
| `user-select` | 控制文本是否可选中 | 1天 | ✅ 已完成 |
| `word-break` | 单词换行规则 | 1天 | ✅ 已完成 |

### P1 - 中优先级（增强视觉效果）

| 属性 | 说明 | 预计工作量 | 状态 |
|------|------|-----------|------|
| `object-fit` | 图片/视频适应方式 | 2天 | ✅ 已完成 |
| `object-position` | 图片/视频位置 | 1天 | ✅ 已完成 |
| `aspect-ratio` | 宽高比 | 1天 | ✅ 已完成 |
| `list-style-*` | 列表样式 | 2天 | ✅ 已完成 |
| `clip-path` | 裁剪路径 | 3天 | ✅ 已完成 |

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

### Phase 0: DOM 绑定系统统一 - ✅ 已完成

**完成的工作：**
- 移除了 `DOMBindings::Init()` 调用（旧系统的重复初始化）
- 保留新系统的模块化初始化（有 exotic 支持）
- Canvas 绑定改为独立初始化

---

### Phase 1: 基础交互属性 - ✅ 已完成

**完成日期：** 2024年12月

**实现的属性：**

#### 1.1 outline 属性族 ✅
- `outline` - 简写属性，支持 `[width] [style] [color]` 任意顺序
- `outline-width` - 轮廓宽度
- `outline-style` - 轮廓样式 (none, solid, dashed, dotted, double)
- `outline-color` - 轮廓颜色
- `outline-offset` - 轮廓偏移

**实现位置：**
- 解析：`core/render/style_resolver.cpp`
- 渲染：`core/render/render_object.cpp` (PaintOutline 方法)
- 测试：`tests/property/render/test_outline_properties.cpp`

#### 1.2 text-transform 属性 ✅
- 支持值：`none`, `uppercase`, `lowercase`, `capitalize`
- 支持 UTF-8 文本处理

**实现位置：**
- 工具函数：`core/render/text_transform.cpp`
- 应用：`core/render/render_object.cpp` (RenderText::Paint)
- 测试：`tests/property/render/test_text_transform_properties.cpp`

#### 1.3 pointer-events 属性 ✅
- 支持值：`auto`, `none`
- 支持继承
- 子元素可覆盖父元素的 `pointer-events: none`

**实现位置：**
- 解析：`core/render/style_resolver.cpp`
- 命中测试：`core/event/hit_testing.cpp`
- 测试：`tests/property/render/test_pointer_events_properties.cpp`

#### 1.4 user-select 属性 ✅
- 支持值：`auto`, `none`, `text`, `all`
- 支持继承

**实现位置：**
- 解析：`core/render/style_resolver.cpp`
- 测试：`tests/property/render/test_user_select_properties.cpp`

#### 1.5 word-break 属性 ✅
- 支持值：`normal`, `break-all`, `keep-all`, `break-word`
- 集成到 LineBreaker

**实现位置：**
- 解析：`core/render/style_resolver.cpp`
- 换行逻辑：`core/layout/ifc/line_breaker.cpp`
- 测试：`tests/property/render/test_word_break_properties.cpp`

**测试结果：** 29/29 属性测试通过 ✅

---

### Phase 2: 媒体和布局属性 - ✅ 已完成

**完成日期：** 2024年12月

**实现的属性：**

#### 2.1 object-fit / object-position ✅

用于控制 `<img>` 和 `<video>` 元素的内容适应方式。

**涉及属性：**
- `object-fit`: fill, contain, cover, none, scale-down
- `object-position`: 位置值 (如 `center`, `top left`, `50% 50%`)

**实现位置：**
- 解析：`core/render/style_resolver.cpp`
- 计算：`core/render/image/image_fit.cpp`
- 渲染：`core/render/render_inline_block.cpp`
- 测试：`tests/property/render/test_object_fit_properties.cpp`

#### 2.2 aspect-ratio ✅

保持元素宽高比。

**支持的值：**
- `auto` - 使用元素固有宽高比
- `<ratio>` - 如 `16 / 9`, `4/3`, `1`
- `auto <ratio>` - 优先使用固有宽高比，否则使用指定比例

**实现位置：**
- 解析：`core/render/style_resolver.cpp`
- 布局转换：`core/render/render_object.cpp` (ConvertComputedStyleToLayoutStyle)
- 测试：`tests/property/render/test_aspect_ratio_properties.cpp`

#### 2.3 list-style 属性族 ✅

**涉及属性：**
- `list-style` - 简写属性
- `list-style-type`: disc, circle, square, decimal, decimal-leading-zero, lower-roman, upper-roman, lower-alpha, upper-alpha, none
- `list-style-position`: inside, outside
- `list-style-image`: url(...), none

**实现位置：**
- 解析：`core/render/style_resolver.cpp`
- 标记工具：`core/render/list_marker.cpp`
- 渲染：`core/render/render_object.cpp` (RenderBlock::Paint)
- 测试：`tests/property/render/test_list_style_properties.cpp`

**测试结果：** 29个属性测试通过 ✅
- 11 个 object-fit/position 测试
- 7 个 aspect-ratio 测试
- 11 个 list-style 测试

---

### Phase 3: 高级视觉效果 (2周)

#### 3.1 clip-path ✅

支持基本形状裁剪。

**支持的值：**
- `inset(top right bottom left [round radius])`
- `circle(radius at x y)`
- `ellipse(rx ry at x y)`
- `polygon(x1 y1, x2 y2, ...)`
- `none`

**实现位置：**
- 数据结构：`core/render/css_clip_path.h`
- 解析和转换：`core/render/css_clip_path.cpp`
- 样式解析：`core/render/style_resolver.cpp`
- 渲染应用：`core/render/render_object.cpp`, `core/render/render_inline_block.cpp`
- 测试：`tests/property/render/test_clip_path_properties.cpp`

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
2. **属性测试**：使用随机输入验证属性不变量
3. **渲染测试**：测试视觉效果
4. **集成测试**：测试与其他属性的交互

### 测试文件结构

```
tests/
├── unit/
│   └── render/
│       ├── test_outline.cpp
│       ├── test_text_transform.cpp
│       └── test_clip_path.cpp
├── property/
│   └── render/
│       ├── test_outline_properties.cpp        ✅
│       ├── test_text_transform_properties.cpp ✅
│       ├── test_pointer_events_properties.cpp ✅
│       ├── test_user_select_properties.cpp    ✅
│       ├── test_word_break_properties.cpp     ✅
│       ├── test_object_fit_properties.cpp     ✅
│       ├── test_aspect_ratio_properties.cpp   ✅
│       ├── test_list_style_properties.cpp     ✅
│       └── test_clip_path_properties.cpp      ✅
└── render/
    ├── test_outline_rendering.cpp
    └── test_animation.cpp
```

---

## 时间线

| 阶段 | 内容 | 预计时间 | 状态 |
|------|------|---------|------|
| **Phase 0** | DOM 绑定系统统一 | 3天 | ✅ 已完成 |
| **Phase 1** | 基础交互属性 | 1周 | ✅ 已完成 |
| **Phase 2** | 媒体和布局属性 | 1周 | ✅ 已完成 |
| Phase 3 | 高级视觉效果 | 2周 | 待开发 |
| 测试和修复 | 全面测试 | 1周 | 待开发 |

**总计：约 5.5 周**

---

## 进度摘要

### 已完成 ✅
- Phase 0: DOM 绑定系统统一
- Phase 1: 基础交互属性
  - outline 属性族 (5个属性)
  - text-transform
  - pointer-events
  - user-select
  - word-break
  - 29个属性测试全部通过
- Phase 2: 媒体和布局属性
  - object-fit / object-position
  - aspect-ratio
  - list-style 属性族 (4个属性)
  - 29个属性测试全部通过
- Phase 3: 高级视觉效果（部分完成）
  - clip-path ✅
  - 8个属性测试全部通过
  - **总计：66个属性测试通过**

### 待开发
- Phase 3: 高级视觉效果（剩余）
  - CSS 动画

---

## 参考资源

- [MDN CSS Reference](https://developer.mozilla.org/en-US/docs/Web/CSS/Reference)
- [CSS Specification](https://www.w3.org/Style/CSS/)
- [Skia Documentation](https://skia.org/docs/)
