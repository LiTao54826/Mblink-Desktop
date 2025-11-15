# MBink CSS 高级特性完善计划

> **版本**: 1.0  
> **日期**: 2025-11-14  
> **目标**: 完善所有桌面 UI 库必需的 CSS 高级特性  
> **预计时间**: 8-10周

---

## 📊 当前 CSS 支持状态

### ✅ 已支持的基础特性

| 特性类别 | 支持情况 | 说明 |
|---------|---------|------|
| **基础布局** | ✅ 完成 | Flexbox (Yoga), width/height, padding/margin |
| **颜色和背景** | ✅ 完成 | background-color, color, 颜色解析 |
| **边框** | ✅ 完成 | border, border-radius, border-style |
| **文本样式** | ✅ 完成 | font-family, font-size, font-weight, text-align |
| **CSS 选择器** | ✅ 完成 | 类、ID、属性、伪类选择器 (Lexbor) |
| **样式级联** | ✅ 完成 | 继承、优先级、!important |
| **基础渲染** | ✅ 完成 | Skia 渲染、文本渲染、图像渲染 |

### ⚠️ 部分支持的特性

| 特性类别 | 当前状态 | 缺失内容 |
|---------|---------|---------|
| **阴影** | ⚠️ 定义存在 | 未实现渲染逻辑 |
| **渐变** | ⚠️ 定义存在 | 未实现渲染逻辑 |
| **伪类** | ⚠️ 部分支持 | :hover, :active, :focus 已支持，其他未实现 |

### ❌ 未支持的高级特性

| 特性类别 | 优先级 | 对 UI 库的重要性 |
|---------|--------|----------------|
| **CSS 动画** | P0 | ⭐⭐⭐⭐⭐ 必需 |
| **CSS 过渡** | P0 | ⭐⭐⭐⭐⭐ 必需 |
| **CSS Transform** | P0 | ⭐⭐⭐⭐⭐ 必需 |
| **阴影渲染** | P0 | ⭐⭐⭐⭐⭐ 必需 |
| **渐变渲染** | P1 | ⭐⭐⭐⭐ 重要 |
| **CSS Filter** | P1 | ⭐⭐⭐ 有用 |
| **CSS Grid** | P2 | ⭐⭐ 可选 |
| **CSS 变量** | P1 | ⭐⭐⭐⭐ 重要 |

---

## 🎯 开发计划总览

### Phase 1: 阴影和渐变渲染 (2周)
**目标**: 实现 box-shadow, text-shadow, linear-gradient, radial-gradient

### Phase 2: CSS Transform (2周)
**目标**: 实现 translate, rotate, scale, skew, matrix

### Phase 3: CSS 过渡 (1.5周)
**目标**: 实现 transition 属性和过渡动画

### Phase 4: CSS 动画 (2周)
**目标**: 实现 @keyframes 和 animation 属性

### Phase 5: CSS 变量和 Filter (1.5周)
**目标**: 实现 CSS 自定义属性和滤镜效果

### Phase 6: 高级布局和优化 (1周)
**目标**: 完善 Grid 布局、性能优化

---

## 📅 详细实施计划

## Phase 1: 阴影和渐变渲染 (2周)

### Week 1: 阴影渲染

#### 任务 1.1: Box Shadow 渲染 (3天)

**文件**: `core/render/shadow_renderer.h/cpp`

**功能需求**:
```cpp
class ShadowRenderer {
public:
    // 渲染盒子阴影
    void RenderBoxShadow(SkCanvas* canvas,
                        const SkRect& rect,
                        const std::vector<CSSBoxShadow>& shadows,
                        float border_radius = 0.0f);
    
    // 渲染内阴影
    void RenderInsetShadow(SkCanvas* canvas,
                          const SkRect& rect,
                          const CSSBoxShadow& shadow,
                          float border_radius = 0.0f);
};
```

**实现要点**:
- ✅ 支持多重阴影
- ✅ 支持 inset 阴影
- ✅ 支持模糊半径 (blur-radius)
- ✅ 支持扩展半径 (spread-radius)
- ✅ 支持圆角边框的阴影
- ✅ 使用 Skia `SkMaskFilter::MakeBlur()` 实现模糊

**测试用例**:
```css
/* 单个阴影 */
box-shadow: 2px 2px 4px rgba(0,0,0,0.3);

/* 多重阴影 */
box-shadow: 0 2px 4px rgba(0,0,0,0.1),
            0 4px 8px rgba(0,0,0,0.2);

/* 内阴影 */
box-shadow: inset 0 2px 4px rgba(0,0,0,0.2);

/* 带扩展的阴影 */
box-shadow: 0 0 0 4px rgba(66,153,225,0.5);
```

#### 任务 1.2: Text Shadow 渲染 (2天)

**文件**: `core/render/text/text_shadow_renderer.h/cpp`

**功能需求**:
```cpp
class TextShadowRenderer {
public:
    // 渲染文本阴影
    void RenderTextWithShadow(SkCanvas* canvas,
                             const std::string& text,
                             const SkFont& font,
                             float x, float y,
                             SkColor text_color,
                             const std::vector<CSSTextShadow>& shadows);
};
```

**实现要点**:
- ✅ 支持多重文本阴影
- ✅ 支持模糊效果
- ✅ 正确的渲染顺序（先渲染阴影，再渲染文本）

**测试用例**:
```css
/* 简单文本阴影 */
text-shadow: 1px 1px 2px rgba(0,0,0,0.5);

/* 发光效果 */
text-shadow: 0 0 10px #fff, 0 0 20px #fff;

/* 3D 效果 */
text-shadow: 1px 1px 0 #ccc,
             2px 2px 0 #bbb,
             3px 3px 0 #aaa;
```

### Week 2: 渐变渲染

#### 任务 1.3: Linear Gradient 渲染 (3天)

**文件**: `core/render/gradient_renderer.h/cpp`

**功能需求**:
```cpp
class GradientRenderer {
public:
    // 创建线性渐变 Shader
    sk_sp<SkShader> CreateLinearGradient(
        const SkRect& rect,
        const CSSLinearGradient& gradient);
    
    // 渲染线性渐变背景
    void RenderLinearGradient(SkCanvas* canvas,
                             const SkRect& rect,
                             const CSSLinearGradient& gradient);
};
```

**实现要点**:
- ✅ 支持角度 (0deg, 90deg, 180deg, 270deg)
- ✅ 支持方向关键字 (to top, to right, to bottom, to left)
- ✅ 支持多个色标 (color stops)
- ✅ 支持色标位置 (0%, 50%, 100%)
- ✅ 使用 Skia `SkGradientShader::MakeLinear()`

**测试用例**:
```css
/* 简单渐变 */
background: linear-gradient(to right, #ff0000, #0000ff);

/* 多色渐变 */
background: linear-gradient(90deg, 
    #ff0000 0%, 
    #00ff00 50%, 
    #0000ff 100%);

/* 对角渐变 */
background: linear-gradient(45deg, #ff0000, #0000ff);
```

#### 任务 1.4: Radial Gradient 渲染 (2天)

**功能需求**:
```cpp
// 创建径向渐变 Shader
sk_sp<SkShader> CreateRadialGradient(
    const SkRect& rect,
    const CSSRadialGradient& gradient);
```

**实现要点**:
- ✅ 支持圆形和椭圆形
- ✅ 支持中心位置 (center, top left, 50% 50%)
- ✅ 支持多个色标
- ✅ 使用 Skia `SkGradientShader::MakeRadial()`

**测试用例**:
```css
/* 圆形渐变 */
background: radial-gradient(circle, #ff0000, #0000ff);

/* 椭圆渐变 */
background: radial-gradient(ellipse at center, #ff0000, #0000ff);

/* 偏移中心 */
background: radial-gradient(circle at 30% 30%, #ff0000, #0000ff);
```

---

## Phase 2: CSS Transform (2周)

### Week 3-4: Transform 实现

#### 任务 2.1: Transform 解析 (2天)

**文件**: `core/render/transform.h/cpp`

**数据结构**:
```cpp
enum class TransformType {
    TRANSLATE,
    ROTATE,
    SCALE,
    SKEW,
    MATRIX
};

struct Transform {
    TransformType type;
    std::vector<float> values;
};

class CSSTransform {
public:
    std::vector<Transform> transforms;
    
    // 解析 transform 字符串
    static CSSTransform Parse(const std::string& str);
    
    // 转换为 Skia Matrix
    SkMatrix ToSkMatrix() const;
};
```

**支持的函数**:
```css
transform: translate(10px, 20px);
transform: translateX(10px);
transform: translateY(20px);
transform: rotate(45deg);
transform: scale(1.5);
transform: scaleX(1.5);
transform: scaleY(1.5);
transform: skew(10deg, 20deg);
transform: skewX(10deg);
transform: skewY(20deg);
transform: matrix(a, b, c, d, e, f);
```

#### 任务 2.2: Transform Origin (1天)

**功能需求**:
```cpp
struct TransformOrigin {
    CSSLength x;  // left, center, right, 或长度值
    CSSLength y;  // top, center, bottom, 或长度值
    
    // 转换为像素坐标
    SkPoint ToPoint(const SkRect& rect) const;
};
```

**测试用例**:
```css
transform-origin: center center;  /* 默认 */
transform-origin: top left;
transform-origin: 50% 50%;
transform-origin: 10px 20px;
```

#### 任务 2.3: Transform 渲染 (3天)

**文件**: `core/render/transform_renderer.h/cpp`

**功能需求**:
```cpp
class TransformRenderer {
public:
    // 应用 transform 到 canvas
    void ApplyTransform(SkCanvas* canvas,
                       const SkRect& rect,
                       const CSSTransform& transform,
                       const TransformOrigin& origin);
    
    // 计算变换后的边界框
    SkRect ComputeTransformedBounds(const SkRect& rect,
                                   const CSSTransform& transform,
                                   const TransformOrigin& origin);
};
```

**实现要点**:
- ✅ 正确处理 transform-origin
- ✅ 支持多个 transform 函数组合
- ✅ 使用 Skia `SkCanvas::save()` 和 `restore()`
- ✅ 更新脏区域计算（考虑变换后的边界）

#### 任务 2.4: 3D Transform (可选，P2) (2天)

**支持的函数**:
```css
transform: perspective(500px);
transform: rotateX(45deg);
transform: rotateY(45deg);
transform: rotateZ(45deg);
transform: translate3d(10px, 20px, 30px);
transform: scale3d(1, 1.5, 1);
```

**注意**: 3D transform 需要 Skia 的 3D 支持，可能需要使用 `SkM44` (4x4 矩阵)

---

## Phase 3: CSS 过渡 (1.5周)

### Week 5: Transition 实现

#### 任务 3.1: Transition 解析 (2天)

**文件**: `core/render/transition.h/cpp`

**数据结构**:
```cpp
enum class TimingFunction {
    LINEAR,
    EASE,
    EASE_IN,
    EASE_OUT,
    EASE_IN_OUT,
    CUBIC_BEZIER
};

struct CSSTransition {
    std::string property;        // 属性名
    float duration;              // 持续时间 (秒)
    TimingFunction timing;       // 缓动函数
    float delay;                 // 延迟 (秒)
    std::vector<float> bezier;   // cubic-bezier 参数
    
    static std::vector<CSSTransition> Parse(const std::string& str);
};
```

**支持的语法**:
```css
transition: all 0.3s ease;
transition: opacity 0.5s ease-in-out;
transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
transition: width 0.3s, height 0.3s;
```

#### 任务 3.2: Transition 动画引擎 (3天)

**文件**: `core/render/transition_manager.h/cpp`

**功能需求**:
```cpp
class TransitionManager {
public:
    // 开始过渡
    void StartTransition(RenderObject* obj,
                        const std::string& property,
                        const std::string& from_value,
                        const std::string& to_value,
                        const CSSTransition& transition);
    
    // 更新所有过渡 (每帧调用)
    void Update(float delta_time);
    
    // 取消过渡
    void CancelTransition(RenderObject* obj, const std::string& property);
    
private:
    struct ActiveTransition {
        RenderObject* object;
        std::string property;
        std::string from_value;
        std::string to_value;
        CSSTransition config;
        float elapsed_time;
        float start_delay;
    };
    
    std::vector<ActiveTransition> active_transitions_;
    
    // 插值函数
    float Interpolate(float from, float to, float progress, TimingFunction timing);
    std::string InterpolateValue(const std::string& from, 
                                 const std::string& to, 
                                 float progress);
};
```

**支持的可过渡属性**:
- ✅ opacity
- ✅ width, height
- ✅ margin, padding
- ✅ background-color, color
- ✅ transform
- ✅ border-width, border-color
- ✅ font-size

#### 任务 3.3: 缓动函数实现 (1天)

**文件**: `core/render/easing.h/cpp`

**功能需求**:
```cpp
class Easing {
public:
    static float Linear(float t);
    static float Ease(float t);
    static float EaseIn(float t);
    static float EaseOut(float t);
    static float EaseInOut(float t);
    static float CubicBezier(float t, float p1, float p2, float p3, float p4);
};
```

---

## Phase 4: CSS 动画 (2周)

### Week 6-7: Animation 实现

#### 任务 4.1: @keyframes 解析 (2天)

**文件**: `core/lexbor/keyframes_parser.h/cpp`

**数据结构**:
```cpp
struct Keyframe {
    float offset;  // 0.0 到 1.0
    std::map<std::string, std::string> properties;
};

struct KeyframesRule {
    std::string name;
    std::vector<Keyframe> keyframes;
};

class KeyframesParser {
public:
    // 解析 @keyframes 规则
    static KeyframesRule Parse(const std::string& css);
};
```

**支持的语法**:
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

#### 任务 4.2: Animation 属性解析 (2天)

**数据结构**:
```cpp
enum class AnimationDirection {
    NORMAL,
    REVERSE,
    ALTERNATE,
    ALTERNATE_REVERSE
};

enum class AnimationFillMode {
    NONE,
    FORWARDS,
    BACKWARDS,
    BOTH
};

struct CSSAnimation {
    std::string name;
    float duration;
    TimingFunction timing;
    float delay;
    int iteration_count;  // -1 表示 infinite
    AnimationDirection direction;
    AnimationFillMode fill_mode;
    bool paused;
    
    static std::vector<CSSAnimation> Parse(const std::string& str);
};
```

**支持的语法**:
```css
animation: slide-in 0.5s ease-in-out;
animation: fade-in 1s ease 0.5s infinite alternate;
animation: bounce 0.3s ease-in-out 3;
animation-play-state: paused;
```

#### 任务 4.3: Animation 引擎 (4天)

**文件**: `core/render/animation_manager.h/cpp`

**功能需求**:
```cpp
class AnimationManager {
public:
    // 注册 @keyframes 规则
    void RegisterKeyframes(const KeyframesRule& rule);
    
    // 开始动画
    void StartAnimation(RenderObject* obj, const CSSAnimation& animation);
    
    // 更新所有动画 (每帧调用)
    void Update(float delta_time);
    
    // 暂停/恢复动画
    void PauseAnimation(RenderObject* obj, const std::string& name);
    void ResumeAnimation(RenderObject* obj, const std::string& name);
    
    // 停止动画
    void StopAnimation(RenderObject* obj, const std::string& name);
    
private:
    struct ActiveAnimation {
        RenderObject* object;
        CSSAnimation config;
        const KeyframesRule* keyframes;
        float elapsed_time;
        int current_iteration;
        bool is_paused;
    };
    
    std::map<std::string, KeyframesRule> keyframes_rules_;
    std::vector<ActiveAnimation> active_animations_;
    
    // 计算当前关键帧
    std::map<std::string, std::string> ComputeCurrentFrame(
        const ActiveAnimation& anim, float progress);
};
```

#### 任务 4.4: 动画事件 (1天)

**功能需求**:
```cpp
// 动画事件
class AnimationEvent : public Event {
public:
    std::string animation_name;
    float elapsed_time;
};

// 支持的事件
// - animationstart
// - animationend
// - animationiteration
```

**JavaScript 使用**:
```javascript
element.addEventListener('animationend', (e) => {
    console.log('Animation finished:', e.animationName);
});
```

---

## Phase 5: CSS 变量和 Filter (1.5周)

### Week 8: CSS 变量和滤镜

#### 任务 5.1: CSS 自定义属性 (3天)

**文件**: `core/lexbor/css_variables.h/cpp`

**功能需求**:
```cpp
class CSSVariables {
public:
    // 设置变量
    void SetVariable(const std::string& name, const std::string& value);
    
    // 获取变量值
    std::string GetVariable(const std::string& name, 
                           const std::string& fallback = "") const;
    
    // 解析 var() 函数
    std::string ResolveVar(const std::string& value) const;
    
private:
    std::map<std::string, std::string> variables_;
};
```

**支持的语法**:
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

#### 任务 5.2: CSS Filter (2天)

**文件**: `core/render/filter_renderer.h/cpp`

**支持的滤镜**:
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

**实现要点**:
- ✅ 使用 Skia `SkImageFilter`
- ✅ 支持多个滤镜组合
- ✅ 性能优化（缓存滤镜结果）

---

## Phase 6: 高级布局和优化 (1周)

### Week 9: Grid 布局和性能优化

#### 任务 6.1: CSS Grid 基础 (可选，P2) (3天)

**注意**: Grid 布局较复杂，可以考虑延后或使用第三方库

**基础支持**:
```css
display: grid;
grid-template-columns: 1fr 2fr 1fr;
grid-template-rows: auto 1fr auto;
grid-gap: 10px;
```

#### 任务 6.2: 性能优化 (2天)

**优化项**:
- ✅ 动画和过渡的 GPU 加速
- ✅ Transform 的图层提升
- ✅ 阴影和滤镜的缓存
- ✅ 减少不必要的重绘

---

## 📊 测试计划

### 单元测试

每个 Phase 完成后编写对应的单元测试：

```cpp
// tests/unit/test_shadow_renderer.cpp
TEST(ShadowRenderer, BoxShadow) {
    // 测试基础阴影渲染
}

TEST(ShadowRenderer, MultipleBoxShadows) {
    // 测试多重阴影
}

TEST(ShadowRenderer, InsetShadow) {
    // 测试内阴影
}
```

### 集成测试

创建完整的示例应用测试所有特性：

```javascript
// examples/css_features_demo/app.js
// 展示所有 CSS 高级特性
```

### 性能测试

```cpp
// tests/benchmarks/css_performance.cpp
BENCHMARK(BoxShadowRendering);
BENCHMARK(GradientRendering);
BENCHMARK(TransformRendering);
BENCHMARK(AnimationPerformance);
```

---

## 🎯 里程碑和验收标准

### M1: 阴影和渐变 (Week 2)
- ✅ box-shadow 正确渲染
- ✅ text-shadow 正确渲染
- ✅ linear-gradient 正确渲染
- ✅ radial-gradient 正确渲染
- ✅ 通过 20+ 个测试用例

### M2: Transform (Week 4)
- ✅ 所有 2D transform 函数正确工作
- ✅ transform-origin 正确处理
- ✅ 多个 transform 组合正确
- ✅ 通过 30+ 个测试用例

### M3: 过渡 (Week 5)
- ✅ transition 属性正确解析
- ✅ 过渡动画流畅 (60 FPS)
- ✅ 所有缓动函数正确
- ✅ 通过 25+ 个测试用例

### M4: 动画 (Week 7)
- ✅ @keyframes 正确解析
- ✅ animation 属性正确工作
- ✅ 动画事件正确触发
- ✅ 通过 40+ 个测试用例

### M5: 变量和滤镜 (Week 8)
- ✅ CSS 变量正确工作
- ✅ 所有滤镜正确渲染
- ✅ 通过 20+ 个测试用例

---

## 📚 参考资料

### CSS 规范
- [CSS Backgrounds and Borders Module Level 3](https://www.w3.org/TR/css-backgrounds-3/)
- [CSS Transforms Module Level 1](https://www.w3.org/TR/css-transforms-1/)
- [CSS Transitions](https://www.w3.org/TR/css-transitions-1/)
- [CSS Animations Level 1](https://www.w3.org/TR/css-animations-1/)
- [CSS Custom Properties](https://www.w3.org/TR/css-variables-1/)
- [Filter Effects Module Level 1](https://www.w3.org/TR/filter-effects-1/)

### Skia 文档
- [SkCanvas](https://api.skia.org/classSkCanvas.html)
- [SkShader](https://api.skia.org/classSkShader.html)
- [SkImageFilter](https://api.skia.org/classSkImageFilter.html)
- [SkMatrix](https://api.skia.org/classSkMatrix.html)

### 参考实现
- [Chromium Blink Renderer](https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/)
- [WebKit CSS](https://github.com/WebKit/WebKit/tree/main/Source/WebCore/css)
- [Servo Style](https://github.com/servo/servo/tree/main/components/style)

---

---

## 💻 实施指南

### 开发流程

每个特性的开发遵循以下流程：

1. **设计阶段** (1天)
   - 阅读 CSS 规范
   - 设计数据结构
   - 设计 API 接口
   - 编写设计文档

2. **实现阶段** (2-4天)
   - 实现解析器
   - 实现渲染逻辑
   - 集成到现有系统
   - 代码审查

3. **测试阶段** (1-2天)
   - 编写单元测试
   - 编写集成测试
   - 性能测试
   - 修复 Bug

4. **文档阶段** (半天)
   - 更新 API 文档
   - 编写使用示例
   - 更新 CHANGELOG

### 代码示例

#### 示例 1: 实现 Box Shadow 渲染

```cpp
// core/render/shadow_renderer.cpp

void ShadowRenderer::RenderBoxShadow(SkCanvas* canvas,
                                    const SkRect& rect,
                                    const std::vector<CSSBoxShadow>& shadows,
                                    float border_radius) {
    // 从后往前渲染（最后的阴影在最底层）
    for (auto it = shadows.rbegin(); it != shadows.rend(); ++it) {
        const auto& shadow = *it;

        if (shadow.inset) {
            RenderInsetShadow(canvas, rect, shadow, border_radius);
        } else {
            RenderOutsetShadow(canvas, rect, shadow, border_radius);
        }
    }
}

void ShadowRenderer::RenderOutsetShadow(SkCanvas* canvas,
                                       const SkRect& rect,
                                       const CSSBoxShadow& shadow,
                                       float border_radius) {
    // 1. 计算阴影矩形
    SkRect shadow_rect = rect;
    shadow_rect.offset(shadow.offset_x, shadow.offset_y);
    shadow_rect.outset(shadow.spread_radius, shadow.spread_radius);

    // 2. 创建模糊滤镜
    SkPaint paint;
    paint.setColor(shadow.color);
    paint.setAntiAlias(true);

    if (shadow.blur_radius > 0) {
        paint.setMaskFilter(
            SkMaskFilter::MakeBlur(kNormal_SkBlurStyle,
                                  shadow.blur_radius / 2.0f)
        );
    }

    // 3. 绘制阴影
    if (border_radius > 0) {
        canvas->drawRoundRect(shadow_rect, border_radius, border_radius, paint);
    } else {
        canvas->drawRect(shadow_rect, paint);
    }
}
```

#### 示例 2: 实现 Linear Gradient

```cpp
// core/render/gradient_renderer.cpp

sk_sp<SkShader> GradientRenderer::CreateLinearGradient(
    const SkRect& rect,
    const CSSLinearGradient& gradient) {

    // 1. 计算渐变方向
    SkPoint pts[2];
    float angle_rad = gradient.angle * M_PI / 180.0f;

    float center_x = rect.centerX();
    float center_y = rect.centerY();
    float length = std::max(rect.width(), rect.height());

    pts[0].set(center_x - cos(angle_rad) * length / 2,
               center_y - sin(angle_rad) * length / 2);
    pts[1].set(center_x + cos(angle_rad) * length / 2,
               center_y + sin(angle_rad) * length / 2);

    // 2. 准备颜色和位置数组
    std::vector<SkColor> colors;
    std::vector<SkScalar> positions;

    for (const auto& stop : gradient.stops) {
        colors.push_back(stop.color);
        positions.push_back(stop.position);
    }

    // 3. 创建 Shader
    return SkGradientShader::MakeLinear(
        pts,
        colors.data(),
        positions.data(),
        colors.size(),
        SkTileMode::kClamp
    );
}
```

#### 示例 3: 实现 Transform

```cpp
// core/render/transform_renderer.cpp

void TransformRenderer::ApplyTransform(SkCanvas* canvas,
                                      const SkRect& rect,
                                      const CSSTransform& transform,
                                      const TransformOrigin& origin) {
    // 1. 计算变换原点
    SkPoint origin_point = origin.ToPoint(rect);

    // 2. 移动到原点
    canvas->translate(origin_point.x(), origin_point.y());

    // 3. 应用所有变换
    for (const auto& t : transform.transforms) {
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

            case TransformType::MATRIX: {
                SkMatrix matrix;
                matrix.setAll(
                    t.values[0], t.values[2], t.values[4],
                    t.values[1], t.values[3], t.values[5],
                    0, 0, 1
                );
                canvas->concat(matrix);
                break;
            }
        }
    }

    // 4. 移回原位
    canvas->translate(-origin_point.x(), -origin_point.y());
}
```

#### 示例 4: 实现 Transition

```cpp
// core/render/transition_manager.cpp

void TransitionManager::StartTransition(RenderObject* obj,
                                       const std::string& property,
                                       const std::string& from_value,
                                       const std::string& to_value,
                                       const CSSTransition& transition) {
    // 取消已存在的过渡
    CancelTransition(obj, property);

    // 创建新的过渡
    ActiveTransition active;
    active.object = obj;
    active.property = property;
    active.from_value = from_value;
    active.to_value = to_value;
    active.config = transition;
    active.elapsed_time = 0.0f;
    active.start_delay = transition.delay;

    active_transitions_.push_back(active);
}

void TransitionManager::Update(float delta_time) {
    auto it = active_transitions_.begin();

    while (it != active_transitions_.end()) {
        auto& trans = *it;

        // 处理延迟
        if (trans.start_delay > 0) {
            trans.start_delay -= delta_time;
            if (trans.start_delay > 0) {
                ++it;
                continue;
            }
            delta_time = -trans.start_delay;  // 剩余时间
            trans.start_delay = 0;
        }

        // 更新时间
        trans.elapsed_time += delta_time;

        // 计算进度
        float progress = std::min(1.0f, trans.elapsed_time / trans.config.duration);

        // 应用缓动函数
        float eased_progress = Easing::Apply(progress, trans.config.timing);

        // 插值并应用值
        std::string current_value = InterpolateValue(
            trans.from_value,
            trans.to_value,
            eased_progress
        );

        // 更新对象属性
        trans.object->SetStyleProperty(trans.property, current_value);

        // 标记需要重绘
        trans.object->SetNeedsRepaint();

        // 检查是否完成
        if (progress >= 1.0f) {
            // 触发 transitionend 事件
            FireTransitionEndEvent(trans.object, trans.property);

            // 移除过渡
            it = active_transitions_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string TransitionManager::InterpolateValue(const std::string& from,
                                                const std::string& to,
                                                float progress) {
    // 尝试解析为数值
    float from_num = CSSValue::ParseFloat(from, 0.0f);
    float to_num = CSSValue::ParseFloat(to, 0.0f);

    // 线性插值
    float result = from_num + (to_num - from_num) * progress;

    // 提取单位
    std::string unit;
    for (size_t i = 0; i < from.length(); ++i) {
        if (!isdigit(from[i]) && from[i] != '.' && from[i] != '-') {
            unit = from.substr(i);
            break;
        }
    }

    return std::to_string(result) + unit;
}
```

---

## 🔧 集成到现有系统

### 1. 修改 StyleResolver

```cpp
// core/render/style_resolver.cpp

void StyleResolver::ParseStyleProperty(ComputedStyle& style,
                                      const std::string& property,
                                      const std::string& value) {
    // ... 现有代码 ...

    // 新增：阴影
    if (property == "box-shadow") {
        style.box_shadows = CSSValue::ParseBoxShadow(value);
    }
    else if (property == "text-shadow") {
        style.text_shadows = CSSValue::ParseTextShadow(value);
    }

    // 新增：渐变
    else if (property == "background-image") {
        if (value.find("linear-gradient") != std::string::npos) {
            style.background_gradient = CSSValue::ParseLinearGradient(value);
        }
        else if (value.find("radial-gradient") != std::string::npos) {
            style.background_radial_gradient = CSSValue::ParseRadialGradient(value);
        }
    }

    // 新增：Transform
    else if (property == "transform") {
        style.transform = CSSTransform::Parse(value);
    }
    else if (property == "transform-origin") {
        style.transform_origin = TransformOrigin::Parse(value);
    }

    // 新增：Transition
    else if (property == "transition") {
        style.transitions = CSSTransition::Parse(value);
    }

    // 新增：Animation
    else if (property == "animation") {
        style.animations = CSSAnimation::Parse(value);
    }

    // 新增：Filter
    else if (property == "filter") {
        style.filters = CSSFilter::Parse(value);
    }
}
```

### 2. 修改 ComputedStyle

```cpp
// core/render/render_object.h

struct ComputedStyle {
    // ... 现有字段 ...

    // 阴影
    std::vector<CSSBoxShadow> box_shadows;
    std::vector<CSSTextShadow> text_shadows;

    // 渐变
    std::optional<CSSLinearGradient> background_gradient;
    std::optional<CSSRadialGradient> background_radial_gradient;

    // Transform
    CSSTransform transform;
    TransformOrigin transform_origin;

    // Transition
    std::vector<CSSTransition> transitions;

    // Animation
    std::vector<CSSAnimation> animations;

    // Filter
    std::vector<CSSFilter> filters;

    // CSS 变量
    std::map<std::string, std::string> css_variables;
};
```

### 3. 修改 Renderer

```cpp
// core/render/unified_renderer.cpp

void UnifiedRenderer::RenderElement(SkCanvas* canvas, RenderObject* obj) {
    const auto& style = obj->GetComputedStyle();
    const auto& rect = obj->GetLayoutRect();

    // 保存 canvas 状态
    canvas->save();

    // 1. 应用 Transform
    if (!style.transform.transforms.empty()) {
        transform_renderer_->ApplyTransform(canvas, rect,
                                           style.transform,
                                           style.transform_origin);
    }

    // 2. 应用 Filter
    if (!style.filters.empty()) {
        filter_renderer_->ApplyFilters(canvas, style.filters);
    }

    // 3. 渲染阴影
    if (!style.box_shadows.empty()) {
        shadow_renderer_->RenderBoxShadow(canvas, rect,
                                          style.box_shadows,
                                          style.border_radius.top_left.value);
    }

    // 4. 渲染背景（包括渐变）
    if (style.background_gradient.has_value()) {
        gradient_renderer_->RenderLinearGradient(canvas, rect,
                                                 *style.background_gradient);
    } else if (style.background_radial_gradient.has_value()) {
        gradient_renderer_->RenderRadialGradient(canvas, rect,
                                                 *style.background_radial_gradient);
    } else {
        // 渲染纯色背景
        RenderBackground(canvas, obj);
    }

    // 5. 渲染边框
    RenderBorder(canvas, obj);

    // 6. 渲染内容
    RenderContent(canvas, obj);

    // 7. 渲染子元素
    for (auto& child : obj->GetChildren()) {
        RenderElement(canvas, child.get());
    }

    // 恢复 canvas 状态
    canvas->restore();
}
```

### 4. 集成到事件循环

```cpp
// core/window/window.cpp

void Window::RenderFrame() {
    // 1. 更新过渡
    transition_manager_->Update(delta_time_);

    // 2. 更新动画
    animation_manager_->Update(delta_time_);

    // 3. 检查是否需要重绘
    if (NeedsRepaint()) {
        // 4. 渲染
        RenderDocument();

        // 5. 交换缓冲区
        SwapBuffers();
    }

    // 6. 请求下一帧
    if (transition_manager_->HasActiveTransitions() ||
        animation_manager_->HasActiveAnimations()) {
        RequestAnimationFrame();
    }
}
```

---

## 📈 性能优化建议

### 1. GPU 加速

对于频繁变化的属性，使用图层提升：

```cpp
bool ShouldPromoteToLayer(const ComputedStyle& style) {
    // Transform 动画
    if (!style.transform.transforms.empty()) {
        return true;
    }

    // Opacity 动画
    if (style.opacity < 1.0f) {
        return true;
    }

    // 有活动的动画或过渡
    if (!style.animations.empty() || !style.transitions.empty()) {
        return true;
    }

    return false;
}
```

### 2. 缓存优化

缓存昂贵的计算结果：

```cpp
class ShadowCache {
public:
    sk_sp<SkImage> GetCachedShadow(const CSSBoxShadow& shadow,
                                   const SkRect& rect) {
        std::string key = GenerateKey(shadow, rect);

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }

        // 渲染并缓存
        auto image = RenderShadowToImage(shadow, rect);
        cache_[key] = image;
        return image;
    }

private:
    std::map<std::string, sk_sp<SkImage>> cache_;
};
```

### 3. 脏区域优化

只重绘变化的区域：

```cpp
void TransitionManager::Update(float delta_time) {
    for (auto& trans : active_transitions_) {
        // 计算旧的边界框
        SkRect old_bounds = trans.object->GetTransformedBounds();

        // 更新属性
        UpdateProperty(trans);

        // 计算新的边界框
        SkRect new_bounds = trans.object->GetTransformedBounds();

        // 标记脏区域
        MarkDirtyRegion(old_bounds);
        MarkDirtyRegion(new_bounds);
    }
}
```

---

## 🎓 学习资源

### 推荐阅读

1. **CSS 规范**
   - 从 W3C 规范开始，理解标准行为
   - 重点关注算法和计算公式

2. **Skia 示例**
   - 查看 Skia 官方示例代码
   - 学习如何使用各种 API

3. **Chromium 源码**
   - 参考 Blink 的实现
   - 学习最佳实践

### 调试技巧

1. **可视化调试**
   ```cpp
   // 绘制边界框
   void DebugDrawBounds(SkCanvas* canvas, const SkRect& rect) {
       SkPaint paint;
       paint.setStyle(SkPaint::kStroke_Style);
       paint.setColor(SK_ColorRED);
       paint.setStrokeWidth(1.0f);
       canvas->drawRect(rect, paint);
   }
   ```

2. **性能分析**
   ```cpp
   // 使用性能监控器
   PerformanceMonitor monitor;
   monitor.StartTimer("shadow_rendering");
   RenderBoxShadow(canvas, rect, shadows);
   monitor.EndTimer("shadow_rendering");
   ```

3. **日志输出**
   ```cpp
   #ifdef DEBUG_CSS
   std::cout << "Applying transform: " << transform.ToString() << std::endl;
   #endif
   ```

---

**维护者**: MBink Team
**最后更新**: 2025-11-14
**下次审查**: 每周五

