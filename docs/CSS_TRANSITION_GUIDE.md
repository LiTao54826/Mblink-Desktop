# CSS Transition 使用指南

## 简介

CSS Transitions 允许你在 CSS 属性值变化时创建平滑的动画效果。这个指南将帮助你理解如何在 MBink 中使用 CSS Transitions。

## 基础概念

### 什么是 Transition？

Transition（过渡）是一种在两个状态之间创建平滑动画的方式。当 CSS 属性值发生变化时，transition 会在指定的时间内逐渐改变属性值，而不是立即跳变。

### 基本语法

```css
transition: property duration timing-function delay;
```

- **property**: 要过渡的属性名称（如 `opacity`, `width`, `all`）
- **duration**: 过渡持续时间（如 `0.3s`, `300ms`）
- **timing-function**: 缓动函数（如 `ease`, `linear`, `ease-in-out`）
- **delay**: 延迟时间（可选，如 `0.1s`）

## 快速开始

### 示例 1: 简单的透明度过渡

```html
<style>
    .fade-box {
        opacity: 1;
        transition: opacity 0.3s ease;
    }
    
    .fade-box:hover {
        opacity: 0.5;
    }
</style>

<div class="fade-box">Hover me!</div>
```

**效果**: 当鼠标悬停时，元素会在 0.3 秒内平滑地从完全不透明变为半透明。

### 示例 2: 尺寸和颜色过渡

```html
<style>
    .box {
        width: 100px;
        height: 100px;
        background-color: #3498db;
        transition: all 0.5s ease-in-out;
    }
    
    .box:hover {
        width: 150px;
        height: 150px;
        background-color: #e74c3c;
    }
</style>

<div class="box"></div>
```

**效果**: 悬停时，盒子会同时改变大小和颜色，所有变化都在 0.5 秒内完成。

### 示例 3: 按钮交互

```html
<style>
    .button {
        padding: 10px 20px;
        background-color: #2ecc71;
        color: white;
        border: none;
        border-radius: 5px;
        transition: background-color 0.2s ease, transform 0.2s ease;
    }
    
    .button:hover {
        background-color: #27ae60;
        transform: scale(1.1);
    }
    
    .button:active {
        transform: scale(0.95);
    }
</style>

<button class="button">Click Me!</button>
```

**效果**: 按钮在悬停时会变色并放大，点击时会缩小，所有动画都很平滑。

## 缓动函数详解

缓动函数（Timing Function）控制过渡的速度曲线。

### 预定义缓动函数

#### linear
匀速运动，没有加速或减速。

```css
transition: opacity 0.3s linear;
```

**适用场景**: 简单的淡入淡出、旋转动画

#### ease (默认)
慢速开始，然后加速，最后减速。

```css
transition: opacity 0.3s ease;
```

**适用场景**: 大多数通用动画

#### ease-in
慢速开始，然后加速。

```css
transition: opacity 0.3s ease-in;
```

**适用场景**: 元素离开屏幕、淡出效果

#### ease-out
快速开始，然后减速。

```css
transition: opacity 0.3s ease-out;
```

**适用场景**: 元素进入屏幕、淡入效果

#### ease-in-out
慢速开始和结束，中间加速。

```css
transition: opacity 0.3s ease-in-out;
```

**适用场景**: 平滑的往返动画

### 自定义贝塞尔曲线

使用 `cubic-bezier()` 函数可以创建自定义的缓动效果。

```css
/* 弹跳效果 */
transition: transform 0.5s cubic-bezier(0.68, -0.55, 0.265, 1.55);

/* 快速进入 */
transition: opacity 0.3s cubic-bezier(0.4, 0, 1, 1);

/* 快速离开 */
transition: opacity 0.3s cubic-bezier(0, 0, 0.2, 1);
```

**工具推荐**: 使用 [cubic-bezier.com](https://cubic-bezier.com/) 可视化调整贝塞尔曲线。

## 高级技巧

### 多属性过渡

可以为不同的属性设置不同的过渡参数。

```css
.box {
    transition: 
        width 0.3s ease,
        height 0.3s ease 0.1s,
        background-color 0.5s ease-in-out 0.2s;
}
```

**效果**: 
1. width 立即开始过渡（0.3秒）
2. height 延迟 0.1 秒后开始（0.3秒）
3. background-color 延迟 0.2 秒后开始（0.5秒）

### 交错动画

使用延迟创建交错效果。

```css
.item:nth-child(1) { transition-delay: 0s; }
.item:nth-child(2) { transition-delay: 0.1s; }
.item:nth-child(3) { transition-delay: 0.2s; }
.item:nth-child(4) { transition-delay: 0.3s; }
```

### Transform 过渡

Transform 属性特别适合做动画，因为它可以利用 GPU 加速。

```css
.box {
    transform: translateX(0) rotate(0deg) scale(1);
    transition: transform 0.3s ease-in-out;
}

.box:hover {
    transform: translateX(100px) rotate(45deg) scale(1.2);
}
```

## 最佳实践

### 1. 选择合适的持续时间

- **快速交互** (0.1s - 0.2s): 按钮点击、小元素悬停
- **中等交互** (0.2s - 0.5s): 卡片翻转、菜单展开
- **慢速交互** (0.5s - 1s): 页面过渡、大型元素动画

### 2. 使用 `all` 要谨慎

```css
/* 不推荐 - 可能导致意外的动画 */
transition: all 0.3s ease;

/* 推荐 - 明确指定属性 */
transition: opacity 0.3s ease, transform 0.3s ease;
```

### 3. 优先使用 Transform 和 Opacity

这两个属性可以利用 GPU 加速，性能最好。

```css
/* 好 - GPU 加速 */
.box {
    transform: translateX(100px);
    opacity: 0.5;
}

/* 避免 - 可能触发重排 */
.box {
    left: 100px;
    width: 200px;
}
```

### 4. 避免过度使用

不是所有变化都需要过渡。过多的动画会让用户感到疲劳。

```css
/* 好 - 只在重要交互上使用 */
.button:hover {
    transition: background-color 0.2s ease;
}

/* 避免 - 不必要的动画 */
.text {
    transition: color 0.5s ease;
}
```

## 常见问题

### Q: 为什么我的过渡不工作？

**A**: 检查以下几点：
1. 确保起始和结束状态都有明确的值
2. 确保属性是可动画的（如 `display` 不能过渡）
3. 检查 transition 属性是否正确设置

### Q: 如何让过渡只在一个方向生效？

**A**: 将 transition 属性放在不同的选择器中：

```css
.box {
    /* 只在悬停时过渡 */
}

.box:hover {
    transition: opacity 0.3s ease;
    opacity: 0.5;
}
```

### Q: 如何停止正在进行的过渡？

**A**: 在 C++ 代码中使用 AnimationTimeline：

```cpp
timeline.StopTransition(render_object, "opacity");
// 或停止所有过渡
timeline.StopAllTransitions(render_object);
```

## 性能优化

### 1. 使用 will-change (未来支持)

```css
.box {
    will-change: transform, opacity;
}
```

### 2. 避免同时动画大量元素

```css
/* 避免 */
.item {
    transition: all 0.3s ease;
}

/* 使用交错延迟 */
.item:nth-child(n) {
    transition-delay: calc(n * 0.05s);
}
```

### 3. 使用 requestAnimationFrame

在 JavaScript 中触发动画时，使用 requestAnimationFrame 确保动画与浏览器刷新率同步。

## 示例集合

查看 `examples/css_transition.html` 获取更多实际示例。

## 下一步

- 学习 [CSS Transform](./CSS_TRANSFORM_API.md)
- 了解 [CSS Animation](./CSS_ANIMATION_API.md)（即将推出）
- 查看 [性能优化指南](./PERFORMANCE_GUIDE.md)

## 参考资料

- [W3C CSS Transitions](https://www.w3.org/TR/css-transitions-1/)
- [MDN CSS Transitions](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Transitions)
- [Google Web Fundamentals - Animations](https://developers.google.com/web/fundamentals/design-and-ux/animations)

