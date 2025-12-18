# Requirements Document

## Introduction

本功能实现 CSS `clip-path` 属性，用于裁剪元素的可见区域。`clip-path` 是现代 Web 设计中常用的属性，可以创建圆形、椭圆、多边形等各种形状的裁剪效果。

## Glossary

- **MBink**: 正在开发的轻量级浏览器渲染引擎
- **clip-path**: CSS 属性，定义元素的裁剪区域，区域外的内容不可见
- **inset()**: 矩形裁剪函数，支持圆角
- **circle()**: 圆形裁剪函数
- **ellipse()**: 椭圆裁剪函数
- **polygon()**: 多边形裁剪函数
- **SkPath**: Skia 图形库中的路径对象，用于定义裁剪区域

## Requirements

### Requirement 1: clip-path 基本解析

**User Story:** 作为 Web 开发者，我希望能够使用 `clip-path` 属性裁剪元素，以创建各种视觉效果。

#### Acceptance Criteria

1. WHEN CSS `clip-path` 属性值为 `none` THEN 元素不应用任何裁剪
2. WHEN CSS `clip-path` 属性值为 `inset(...)` THEN StyleResolver 应正确解析 inset 函数参数
3. WHEN CSS `clip-path` 属性值为 `circle(...)` THEN StyleResolver 应正确解析 circle 函数参数
4. WHEN CSS `clip-path` 属性值为 `ellipse(...)` THEN StyleResolver 应正确解析 ellipse 函数参数
5. WHEN CSS `clip-path` 属性值为 `polygon(...)` THEN StyleResolver 应正确解析 polygon 函数参数

### Requirement 2: inset() 函数支持

**User Story:** 作为 Web 开发者，我希望使用 `inset()` 函数创建矩形裁剪区域，可选支持圆角。

#### Acceptance Criteria

1. WHEN `inset(10px)` THEN 应从四边各向内裁剪 10px
2. WHEN `inset(10px 20px)` THEN 应从上下各裁剪 10px，左右各裁剪 20px
3. WHEN `inset(10px 20px 30px)` THEN 应从上裁剪 10px，左右各裁剪 20px，下裁剪 30px
4. WHEN `inset(10px 20px 30px 40px)` THEN 应从上、右、下、左分别裁剪对应值
5. WHEN `inset(10px round 5px)` THEN 应创建带 5px 圆角的矩形裁剪区域
6. WHEN 使用百分比值如 `inset(10%)` THEN 应相对于元素尺寸计算裁剪距离

### Requirement 3: circle() 函数支持

**User Story:** 作为 Web 开发者，我希望使用 `circle()` 函数创建圆形裁剪区域。

#### Acceptance Criteria

1. WHEN `circle()` 无参数 THEN 应使用元素中心和最近边距离作为半径
2. WHEN `circle(50px)` THEN 应创建半径 50px 的圆形，圆心在元素中心
3. WHEN `circle(50%)` THEN 应创建半径为元素较小边 50% 的圆形
4. WHEN `circle(50px at 100px 100px)` THEN 应创建圆心在 (100px, 100px) 的圆形
5. WHEN `circle(50% at center)` THEN 应创建圆心在元素中心的圆形
6. WHEN `circle(closest-side)` THEN 半径应为圆心到最近边的距离
7. WHEN `circle(farthest-side)` THEN 半径应为圆心到最远边的距离

### Requirement 4: ellipse() 函数支持

**User Story:** 作为 Web 开发者，我希望使用 `ellipse()` 函数创建椭圆形裁剪区域。

#### Acceptance Criteria

1. WHEN `ellipse()` 无参数 THEN 应使用元素中心，半径为到最近边的距离
2. WHEN `ellipse(50px 30px)` THEN 应创建水平半径 50px、垂直半径 30px 的椭圆
3. WHEN `ellipse(50% 30%)` THEN 应创建相对于元素尺寸的椭圆
4. WHEN `ellipse(50px 30px at 100px 100px)` THEN 应创建指定圆心位置的椭圆
5. WHEN `ellipse(closest-side closest-side)` THEN 半径应为到最近边的距离

### Requirement 5: polygon() 函数支持

**User Story:** 作为 Web 开发者，我希望使用 `polygon()` 函数创建任意多边形裁剪区域。

#### Acceptance Criteria

1. WHEN `polygon(0 0, 100% 0, 100% 100%, 0 100%)` THEN 应创建矩形多边形
2. WHEN `polygon(50% 0, 100% 100%, 0 100%)` THEN 应创建三角形
3. WHEN 使用像素值如 `polygon(0 0, 100px 0, 50px 100px)` THEN 应正确解析像素坐标
4. WHEN 混合使用百分比和像素值 THEN 应正确处理混合单位
5. WHEN `polygon(evenodd, ...)` THEN 应使用 evenodd 填充规则
6. WHEN `polygon(nonzero, ...)` THEN 应使用 nonzero 填充规则（默认）

### Requirement 6: clip-path 渲染应用

**User Story:** 作为 Web 开发者，我希望 clip-path 能正确应用到元素渲染中。

#### Acceptance Criteria

1. WHEN 元素设置了 clip-path THEN Paint 方法应在绘制前设置 SkCanvas 裁剪区域
2. WHEN clip-path 为 none THEN 不应设置任何裁剪
3. WHEN 元素有子元素 THEN 子元素也应被裁剪
4. WHEN 元素有 transform THEN clip-path 应在 transform 之后应用
5. WHEN clip-path 区域完全在元素外部 THEN 元素应完全不可见

