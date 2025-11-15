# CSS 高级特性快速参考

> **版本**: 1.0  
> **日期**: 2025-11-14  
> **用途**: 开发时快速查阅 CSS 特性实现要点

---

## 📋 特性支持清单

### ✅ 已支持 (Phase 1-3 完成)

- [x] 基础布局 (Flexbox)
- [x] 颜色和背景
- [x] 边框和圆角
- [x] 文本样式
- [x] CSS 选择器
- [x] 样式级联和继承
- [x] 基础伪类 (:hover, :active, :focus)

### 🚧 开发中 (Phase 4)

- [ ] box-shadow
- [ ] text-shadow
- [ ] linear-gradient
- [ ] radial-gradient
- [ ] transform (2D)
- [ ] transition
- [ ] animation
- [ ] CSS 变量
- [ ] filter

### 📅 计划中 (Phase 5+)

- [ ] transform (3D)
- [ ] CSS Grid
- [ ] backdrop-filter
- [ ] clip-path
- [ ] mask

---

## 🎨 阴影 (Box Shadow)

### 语法

```css
box-shadow: [inset] offset-x offset-y blur-radius spread-radius color;
```

### 实现要点

```cpp
// 核心 API
void RenderBoxShadow(SkCanvas* canvas,
                    const SkRect& rect,
                    const std::vector<CSSBoxShadow>& shadows,
                    float border_radius = 0.0f);

// 关键步骤
1. 从后往前渲染（最后的阴影在最底层）
2. 计算阴影矩形：rect + offset + spread
3. 创建模糊滤镜：SkMaskFilter::MakeBlur()
4. 绘制阴影：drawRect() 或 drawRoundRect()
```

### 示例

```css
/* 基础阴影 */
box-shadow: 2px 2px 4px rgba(0,0,0,0.3);

/* 多重阴影 */
box-shadow: 0 2px 4px rgba(0,0,0,0.1),
            0 4px 8px rgba(0,0,0,0.2);

/* 内阴影 */
box-shadow: inset 0 2px 4px rgba(0,0,0,0.2);

/* 发光效果 */
box-shadow: 0 0 0 4px rgba(66,153,225,0.5);
```

### Skia 代码

```cpp
// 创建模糊滤镜
if (shadow.blur_radius > 0) {
    paint.setMaskFilter(
        SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 
                              shadow.blur_radius / 2.0f)
    );
}

// 绘制阴影
if (border_radius > 0) {
    canvas->drawRoundRect(shadow_rect, border_radius, border_radius, paint);
} else {
    canvas->drawRect(shadow_rect, paint);
}
```

---

## 🌈 渐变 (Gradients)

### Linear Gradient

```css
background: linear-gradient(angle, color-stop1, color-stop2, ...);
```

#### 实现要点

```cpp
// 核心 API
sk_sp<SkShader> CreateLinearGradient(const SkRect& rect,
                                     const CSSLinearGradient& gradient);

// 关键步骤
1. 解析角度或方向关键字
2. 计算起点和终点坐标
3. 准备颜色和位置数组
4. 创建 SkGradientShader::MakeLinear()
```

#### 示例

```css
/* 水平渐变 */
background: linear-gradient(to right, #ff0000, #0000ff);

/* 垂直渐变 */
background: linear-gradient(to bottom, #ff0000, #0000ff);

/* 角度渐变 */
background: linear-gradient(45deg, #ff0000, #0000ff);

/* 多色渐变 */
background: linear-gradient(90deg, 
    #ff0000 0%, 
    #00ff00 50%, 
    #0000ff 100%);
```

#### Skia 代码

```cpp
// 计算起点和终点
SkPoint pts[2];
float angle_rad = gradient.angle * M_PI / 180.0f;
pts[0].set(rect.left(), rect.top());
pts[1].set(rect.right(), rect.bottom());

// 创建渐变 Shader
return SkGradientShader::MakeLinear(
    pts,
    colors.data(),
    positions.data(),
    colors.size(),
    SkTileMode::kClamp
);
```

### Radial Gradient

```css
background: radial-gradient(shape at position, color-stop1, color-stop2, ...);
```

#### Skia 代码

```cpp
// 创建径向渐变
return SkGradientShader::MakeRadial(
    center,
    radius,
    colors.data(),
    positions.data(),
    colors.size(),
    SkTileMode::kClamp
);
```

---

## 🔄 Transform

### 支持的函数

```css
transform: translate(x, y);
transform: translateX(x);
transform: translateY(y);
transform: rotate(angle);
transform: scale(x, y);
transform: scaleX(x);
transform: scaleY(y);
transform: skew(x-angle, y-angle);
transform: skewX(angle);
transform: skewY(angle);
transform: matrix(a, b, c, d, e, f);
```

### 实现要点

```cpp
// 核心 API
void ApplyTransform(SkCanvas* canvas,
                   const SkRect& rect,
                   const CSSTransform& transform,
                   const TransformOrigin& origin);

// 关键步骤
1. 计算变换原点
2. 移动到原点：canvas->translate(origin)
3. 应用变换：translate/rotate/scale/skew
4. 移回原位：canvas->translate(-origin)
```

### Transform Origin

```css
transform-origin: center center;  /* 默认 */
transform-origin: top left;
transform-origin: 50% 50%;
transform-origin: 10px 20px;
```

### Skia 代码

```cpp
// 移动到原点
canvas->translate(origin_point.x(), origin_point.y());

// 应用变换
switch (t.type) {
    case TransformType::TRANSLATE:
        canvas->translate(t.values[0], t.values[1]);
        break;
    case TransformType::ROTATE:
        canvas->rotate(t.values[0]);
        break;
    case TransformType::SCALE:
        canvas->scale(t.values[0], t.values[1]);
        break;
    case TransformType::SKEW:
        canvas->skew(tan(t.values[0] * M_PI / 180.0f),
                    tan(t.values[1] * M_PI / 180.0f));
        break;
}

// 移回原位
canvas->translate(-origin_point.x(), -origin_point.y());
```

---

## ⏱️ Transition

### 语法

```css
transition: property duration timing-function delay;
```

### 实现要点

```cpp
// 核心 API
void StartTransition(RenderObject* obj,
                    const std::string& property,
                    const std::string& from_value,
                    const std::string& to_value,
                    const CSSTransition& transition);

void Update(float delta_time);

// 关键步骤
1. 监听属性变化
2. 创建 ActiveTransition 对象
3. 每帧更新：计算进度、应用缓动、插值
4. 完成时触发 transitionend 事件
```

### 缓动函数

```cpp
enum class TimingFunction {
    LINEAR,
    EASE,
    EASE_IN,
    EASE_OUT,
    EASE_IN_OUT,
    CUBIC_BEZIER
};

// 实现
float Easing::EaseInOut(float t) {
    if (t < 0.5f) {
        return 2.0f * t * t;
    } else {
        return -1.0f + (4.0f - 2.0f * t) * t;
    }
}
```

### 示例

```css
/* 简单过渡 */
transition: all 0.3s ease;

/* 特定属性 */
transition: opacity 0.5s ease-in-out;

/* 自定义贝塞尔曲线 */
transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);

/* 多个属性 */
transition: width 0.3s, height 0.3s, opacity 0.5s;
```

---

## 🎬 Animation

### @keyframes

```css
@keyframes slide-in {
    from { transform: translateX(-100%); }
    to { transform: translateX(0); }
}

@keyframes fade-in {
    0% { opacity: 0; }
    50% { opacity: 0.5; }
    100% { opacity: 1; }
}
```

### Animation 属性

```css
animation: name duration timing-function delay iteration-count direction fill-mode;
```

### 实现要点

```cpp
// 核心 API
void RegisterKeyframes(const KeyframesRule& rule);
void StartAnimation(RenderObject* obj, const CSSAnimation& animation);
void Update(float delta_time);

// 关键步骤
1. 解析 @keyframes 规则
2. 存储关键帧数据
3. 每帧计算当前关键帧
4. 插值并应用属性
5. 处理 iteration 和 direction
```

### 示例

```css
/* 基础动画 */
animation: slide-in 0.5s ease-in-out;

/* 无限循环 */
animation: fade-in 1s ease infinite;

/* 交替方向 */
animation: bounce 0.3s ease-in-out 3 alternate;

/* 保持最终状态 */
animation: slide-in 0.5s ease forwards;
```

---

## 🎨 Filter

### 支持的滤镜

```css
filter: blur(5px);
filter: brightness(1.2);
filter: contrast(1.5);
filter: grayscale(100%);
filter: opacity(50%);
filter: saturate(2);
filter: sepia(100%);
filter: hue-rotate(90deg);
```

### 实现要点

```cpp
// 使用 Skia ImageFilter
sk_sp<SkImageFilter> CreateBlurFilter(float radius) {
    return SkImageFilters::Blur(radius, radius, nullptr);
}

sk_sp<SkImageFilter> CreateBrightnessFilter(float amount) {
    SkColorMatrix matrix;
    matrix.setScale(amount, amount, amount, 1.0f);
    return SkImageFilters::ColorFilter(
        SkColorFilters::Matrix(matrix), nullptr);
}
```

---

## 🔧 CSS 变量

### 语法

```css
:root {
    --primary-color: #007bff;
    --spacing: 16px;
}

.button {
    background-color: var(--primary-color);
    padding: var(--spacing);
}
```

### 实现要点

```cpp
// 核心 API
void SetVariable(const std::string& name, const std::string& value);
std::string GetVariable(const std::string& name, 
                       const std::string& fallback = "") const;
std::string ResolveVar(const std::string& value) const;

// 解析 var() 函数
std::string ResolveVar(const std::string& value) {
    // 查找 var(
    size_t start = value.find("var(");
    if (start == std::string::npos) {
        return value;
    }
    
    // 提取变量名
    size_t end = value.find(")", start);
    std::string var_name = value.substr(start + 4, end - start - 4);
    
    // 获取变量值
    return GetVariable(var_name);
}
```

---

## 📊 性能优化清单

### 图层提升

```cpp
bool ShouldPromoteToLayer(const ComputedStyle& style) {
    return !style.transform.transforms.empty() ||
           style.opacity < 1.0f ||
           !style.animations.empty() ||
           !style.transitions.empty();
}
```

### 缓存策略

```cpp
// 缓存阴影
std::map<std::string, sk_sp<SkImage>> shadow_cache_;

// 缓存渐变 Shader
std::map<std::string, sk_sp<SkShader>> gradient_cache_;

// 缓存滤镜
std::map<std::string, sk_sp<SkImageFilter>> filter_cache_;
```

### 脏区域优化

```cpp
// 只重绘变化的区域
void MarkDirtyRegion(const SkRect& rect) {
    dirty_regions_.push_back(rect);
}

void RenderDirtyRegions(SkCanvas* canvas) {
    for (const auto& region : dirty_regions_) {
        canvas->clipRect(region);
        RenderRegion(canvas, region);
    }
}
```

---

## 🐛 常见问题

### Q1: 阴影模糊效果不正确？

**A**: 检查模糊半径的计算，Skia 的 `MakeBlur()` 使用的是 sigma 值，通常是 CSS blur-radius 的一半。

```cpp
paint.setMaskFilter(
    SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 
                          shadow.blur_radius / 2.0f)  // 注意除以 2
);
```

### Q2: Transform 原点不正确？

**A**: 确保在应用变换前后正确移动坐标系。

```cpp
// 正确的顺序
canvas->translate(origin.x, origin.y);  // 移到原点
canvas->rotate(angle);                   // 应用变换
canvas->translate(-origin.x, -origin.y); // 移回
```

### Q3: 过渡动画不流畅？

**A**: 检查帧率和时间步长，确保每帧都调用 `Update()`。

```cpp
// 计算 delta_time
float current_time = GetCurrentTime();
float delta_time = current_time - last_time_;
last_time_ = current_time;

transition_manager_->Update(delta_time);
```

### Q4: 渐变方向不对？

**A**: 检查角度计算，CSS 的角度是顺时针，0deg 是向上。

```cpp
// CSS: 0deg = 向上, 90deg = 向右
// 需要转换为 Skia 坐标系
float angle_rad = (gradient.angle + 90.0f) * M_PI / 180.0f;
```

---

## 📚 快速链接

- [完整开发计划](CSS_ADVANCED_FEATURES_PLAN.md)
- [Skia API 文档](https://api.skia.org/)
- [CSS 规范](https://www.w3.org/Style/CSS/)
- [MDN CSS 参考](https://developer.mozilla.org/en-US/docs/Web/CSS)

---

**维护者**: MBink Team  
**最后更新**: 2025-11-14

