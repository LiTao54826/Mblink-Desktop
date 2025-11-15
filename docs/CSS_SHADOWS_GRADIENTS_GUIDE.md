# CSS 阴影和渐变使用指南

> **版本**: 1.0  
> **日期**: 2025-11-14  
> **难度**: 初级

---

## 📋 目录

1. [快速开始](#快速开始)
2. [Box Shadow 使用指南](#box-shadow-使用指南)
3. [Text Shadow 使用指南](#text-shadow-使用指南)
4. [Linear Gradient 使用指南](#linear-gradient-使用指南)
5. [Radial Gradient 使用指南](#radial-gradient-使用指南)
6. [最佳实践](#最佳实践)
7. [常见问题](#常见问题)
8. [性能优化建议](#性能优化建议)

---

## 快速开始

MBink 完整支持 CSS 阴影和渐变特性。你可以像在浏览器中一样使用这些特性。

### 最简单的例子

```html
<!DOCTYPE html>
<html>
<head>
    <style>
        .card {
            width: 200px;
            height: 100px;
            background-image: linear-gradient(45deg, #667eea, #764ba2);
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
            border-radius: 8px;
        }
    </style>
</head>
<body>
    <div class="card">Hello MBink!</div>
</body>
</html>
```

---

## Box Shadow 使用指南

### 基础用法

```css
/* 语法 */
box-shadow: offset-x offset-y blur-radius spread-radius color;

/* 示例 */
.box {
    box-shadow: 2px 2px 8px rgba(0, 0, 0, 0.2);
}
```

### 参数详解

#### 1. offset-x 和 offset-y（偏移量）

```css
/* 向右下偏移 */
box-shadow: 5px 5px 0 black;

/* 向左上偏移 */
box-shadow: -5px -5px 0 black;

/* 只向右偏移 */
box-shadow: 5px 0 0 black;

/* 只向下偏移 */
box-shadow: 0 5px 0 black;
```

#### 2. blur-radius（模糊半径）

```css
/* 无模糊 */
box-shadow: 0 0 0 black;

/* 轻微模糊 */
box-shadow: 0 0 5px black;

/* 强烈模糊 */
box-shadow: 0 0 20px black;
```

#### 3. spread-radius（扩展半径）

```css
/* 扩大阴影 */
box-shadow: 0 0 10px 5px black;

/* 缩小阴影 */
box-shadow: 0 0 10px -5px black;

/* 创建边框效果 */
box-shadow: 0 0 0 4px rgba(0, 123, 255, 0.3);
```

#### 4. color（颜色）

```css
/* 黑色半透明 */
box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);

/* 彩色阴影 */
box-shadow: 0 4px 20px rgba(255, 0, 0, 0.3);

/* 完全不透明 */
box-shadow: 0 2px 8px rgb(0, 0, 0);
```

### 内阴影 (inset)

```css
/* 外阴影（默认） */
box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);

/* 内阴影 */
box-shadow: inset 0 2px 8px rgba(0, 0, 0, 0.2);
```

### 多重阴影

```css
/* 多层阴影效果 */
.card {
    box-shadow: 
        0 2px 4px rgba(0, 0, 0, 0.1),
        0 4px 8px rgba(0, 0, 0, 0.1),
        0 8px 16px rgba(0, 0, 0, 0.1);
}

/* 发光效果 */
.glow {
    box-shadow: 
        0 0 5px rgba(0, 123, 255, 0.5),
        0 0 10px rgba(0, 123, 255, 0.4),
        0 0 20px rgba(0, 123, 255, 0.3);
}
```

### 常见效果

#### 卡片阴影

```css
.card {
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.card:hover {
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.2);
}
```

#### 浮起效果

```css
.elevated {
    box-shadow: 0 10px 40px rgba(0, 0, 0, 0.3);
}
```

#### 按钮按下效果

```css
.button {
    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
}

.button:active {
    box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2);
}
```

---

## Text Shadow 使用指南

### 基础用法

```css
/* 语法 */
text-shadow: offset-x offset-y blur-radius color;

/* 示例 */
h1 {
    text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
}
```

### 常见效果

#### 基础阴影

```css
.text {
    color: #333;
    text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
}
```

#### 发光效果

```css
.glow-text {
    color: white;
    text-shadow: 0 0 10px rgba(0, 123, 255, 0.8);
}
```

#### 3D 效果

```css
.text-3d {
    color: white;
    text-shadow: 
        1px 1px 0 #ccc,
        2px 2px 0 #bbb,
        3px 3px 0 #aaa,
        4px 4px 0 #999,
        5px 5px 10px rgba(0, 0, 0, 0.5);
}
```

#### 描边效果

```css
.outlined-text {
    color: white;
    text-shadow: 
        -1px -1px 0 black,
        1px -1px 0 black,
        -1px 1px 0 black,
        1px 1px 0 black;
}
```

#### 彩虹效果

```css
.rainbow-text {
    color: white;
    text-shadow: 
        2px 2px 0 #ff0000,
        4px 4px 0 #00ff00,
        6px 6px 0 #0000ff;
}
```

---

## Linear Gradient 使用指南

### 基础用法

```css
/* 语法 */
background-image: linear-gradient(angle, color1, color2, ...);

/* 示例 */
.box {
    background-image: linear-gradient(45deg, #ff6b6b, #4ecdc4);
}
```

### 方向控制

#### 使用角度

```css
/* 0deg = 向上 */
background-image: linear-gradient(0deg, red, blue);

/* 45deg = 右上 */
background-image: linear-gradient(45deg, red, blue);

/* 90deg = 向右 */
background-image: linear-gradient(90deg, red, blue);

/* 180deg = 向下（默认） */
background-image: linear-gradient(180deg, red, blue);
```

#### 使用方向关键字

```css
/* 向上 */
background-image: linear-gradient(to top, red, blue);

/* 向右 */
background-image: linear-gradient(to right, red, blue);

/* 向下 */
background-image: linear-gradient(to bottom, red, blue);

/* 向左 */
background-image: linear-gradient(to left, red, blue);
```

### 多色渐变

```css
/* 三色渐变 */
background-image: linear-gradient(45deg, red, yellow, blue);

/* 彩虹渐变 */
background-image: linear-gradient(90deg, 
    #ff0000, 
    #ff7f00, 
    #ffff00, 
    #00ff00, 
    #0000ff, 
    #4b0082, 
    #9400d3);
```

### 色标位置

```css
/* 精确控制色标位置 */
background-image: linear-gradient(90deg, 
    red 0%, 
    yellow 50%, 
    blue 100%);

/* 创建硬边界 */
background-image: linear-gradient(90deg, 
    red 0%, 
    red 50%, 
    blue 50%, 
    blue 100%);
```

### 常见效果

#### 渐变背景

```css
.hero {
    background-image: linear-gradient(135deg, #667eea, #764ba2);
}
```

#### 渐变叠加

```css
.overlay {
    background-image: linear-gradient(
        to bottom, 
        rgba(0, 0, 0, 0), 
        rgba(0, 0, 0, 0.7)
    );
}
```

#### 条纹效果

```css
.stripes {
    background-image: linear-gradient(45deg, 
        #f0f0f0 25%, 
        transparent 25%, 
        transparent 75%, 
        #f0f0f0 75%);
}
```

---

## Radial Gradient 使用指南

### 基础用法

```css
/* 语法 */
background-image: radial-gradient(shape, color1, color2, ...);

/* 示例 */
.box {
    background-image: radial-gradient(circle, #ff6b6b, #4ecdc4);
}
```

### 形状控制

```css
/* 圆形 */
background-image: radial-gradient(circle, red, blue);

/* 椭圆形（默认） */
background-image: radial-gradient(ellipse, red, blue);
```

### 多色径向渐变

```css
background-image: radial-gradient(circle, 
    red, 
    yellow, 
    green, 
    blue);
```

### 色标位置

```css
background-image: radial-gradient(circle, 
    red 0%, 
    yellow 50%, 
    blue 100%);
```

### 常见效果

#### 聚光灯效果

```css
.spotlight {
    background-image: radial-gradient(circle, 
        rgba(255, 255, 255, 0.8), 
        rgba(0, 0, 0, 0.8));
}
```

#### 按钮渐变

```css
.button {
    background-image: radial-gradient(ellipse, #667eea, #764ba2);
}
```

---

## 最佳实践

### 1. 性能优化

```css
/* ✅ 好：使用简单的阴影 */
box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);

/* ❌ 避免：过多的多重阴影 */
box-shadow: 
    0 1px 2px rgba(0,0,0,0.1),
    0 2px 4px rgba(0,0,0,0.1),
    0 4px 8px rgba(0,0,0,0.1),
    0 8px 16px rgba(0,0,0,0.1),
    0 16px 32px rgba(0,0,0,0.1);
```

### 2. 可访问性

```css
/* ✅ 好：确保文本可读 */
.text {
    color: #333;
    text-shadow: 1px 1px 2px rgba(255, 255, 255, 0.5);
}

/* ❌ 避免：阴影遮挡文本 */
.text {
    color: #333;
    text-shadow: 0 0 20px black;
}
```

### 3. 渐变使用

```css
/* ✅ 好：柔和的渐变 */
background-image: linear-gradient(135deg, #667eea, #764ba2);

/* ❌ 避免：过于刺眼的颜色组合 */
background-image: linear-gradient(0deg, #ff0000, #00ff00);
```

---

## 常见问题

### Q1: 为什么我的阴影没有显示？

**A**: 检查以下几点：
1. 确保元素有足够的空间显示阴影
2. 检查阴影颜色是否与背景色相同
3. 确保 `blur-radius` 不为 0（如果需要模糊效果）

### Q2: 如何创建柔和的阴影？

**A**: 使用较大的模糊半径和较低的透明度：

```css
box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
```

### Q3: 渐变如何与阴影结合使用？

**A**: 直接组合即可：

```css
.card {
    background-image: linear-gradient(135deg, #667eea, #764ba2);
    box-shadow: 0 10px 30px rgba(102, 126, 234, 0.4);
}
```

---

## 性能优化建议

1. **限制多重阴影数量**: 尽量不超过 3 个
2. **避免过大的模糊半径**: 建议不超过 20px
3. **使用硬件加速**: MBink 自动使用 Skia 的硬件加速
4. **缓存渐变**: 相同的渐变会被自动缓存

---

## 相关资源

- [API 文档](CSS_SHADOWS_GRADIENTS_API.md)
- [示例代码](../examples/css_shadows_gradients.html)
- [MDN CSS Box Shadow](https://developer.mozilla.org/en-US/docs/Web/CSS/box-shadow)
- [MDN CSS Gradients](https://developer.mozilla.org/en-US/docs/Web/CSS/gradient)

---

**最后更新**: 2025-11-14  
**维护者**: MBink Team

