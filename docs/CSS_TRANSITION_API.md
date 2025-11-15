# CSS Transition API Documentation

## Overview

MBink 实现了完整的 CSS Transitions 规范，允许元素的 CSS 属性在一段时间内平滑过渡。

## CSS Syntax

### transition 简写属性

```css
transition: <property> <duration> <timing-function> <delay>;
```

**示例:**
```css
/* 单个属性 */
transition: opacity 0.3s ease-in-out;

/* 多个属性 */
transition: width 0.3s ease, height 0.3s ease 0.1s;

/* 所有属性 */
transition: all 0.5s ease-in-out;
```

### 分解属性

#### transition-property
指定要过渡的 CSS 属性名称。

```css
transition-property: opacity;
transition-property: width, height;
transition-property: all;
```

**支持的属性:**
- `opacity` - 透明度
- `width`, `height` - 尺寸
- `background-color` - 背景颜色
- `color` - 文本颜色
- `transform` - 变换 (translate, rotate, scale, skew)
- `all` - 所有可动画属性

#### transition-duration
指定过渡的持续时间。

```css
transition-duration: 0.3s;    /* 秒 */
transition-duration: 300ms;   /* 毫秒 */
```

#### transition-timing-function
指定过渡的缓动函数。

```css
transition-timing-function: linear;
transition-timing-function: ease;
transition-timing-function: ease-in;
transition-timing-function: ease-out;
transition-timing-function: ease-in-out;
transition-timing-function: cubic-bezier(0.42, 0, 0.58, 1);
```

**预定义缓动函数:**
- `linear` - 线性过渡，匀速
- `ease` - 默认值，慢-快-慢
- `ease-in` - 慢速开始
- `ease-out` - 慢速结束
- `ease-in-out` - 慢速开始和结束
- `cubic-bezier(x1, y1, x2, y2)` - 自定义贝塞尔曲线

#### transition-delay
指定过渡开始前的延迟时间。

```css
transition-delay: 0s;      /* 无延迟 */
transition-delay: 0.5s;    /* 延迟 0.5 秒 */
transition-delay: 500ms;   /* 延迟 500 毫秒 */
```

## C++ API

### CSSTransition 结构

```cpp
namespace lightui {

struct CSSTransition {
    std::string property;           // 属性名称
    float duration;                 // 持续时间（秒）
    TimingFunction timing_function; // 缓动函数
    CubicBezier bezier;            // 自定义贝塞尔曲线
    float delay;                    // 延迟时间（秒）

    // 解析 CSS transition 字符串
    static std::vector<CSSTransition> Parse(const std::string& str);
};

} // namespace lightui
```

### AnimationTimeline 类

```cpp
namespace lightui {

class AnimationTimeline {
public:
    AnimationTimeline();
    ~AnimationTimeline();

    // 启动过渡动画
    void StartTransition(
        RenderObject* object,
        const std::string& property,
        const CSSTransition& transition,
        const TransitionValue& start_value,
        const TransitionValue& end_value
    );

    // 停止指定对象的指定属性的过渡
    void StopTransition(RenderObject* object, const std::string& property);

    // 停止指定对象的所有过渡
    void StopAllTransitions(RenderObject* object);

    // 更新所有动画（每帧调用）
    void Update(double current_time);

    // 获取当前动画值
    std::optional<TransitionValue> GetCurrentValue(
        RenderObject* object,
        const std::string& property
    ) const;

    // 静态插值方法
    static TransitionValue Interpolate(
        const TransitionValue& start,
        const TransitionValue& end,
        float progress
    );
};

} // namespace lightui
```

### TransitionValue 类型

```cpp
namespace lightui {

// 支持的过渡值类型
using TransitionValue = std::variant<
    float,          // 数值（opacity, width, height等）
    SkColor,        // 颜色（background-color, color等）
    CSSTransform    // 变换（transform）
>;

} // namespace lightui
```

### EasingFunctions 类

```cpp
namespace lightui {

class EasingFunctions {
public:
    // 应用缓动函数
    static float Apply(TimingFunction func, const CubicBezier& bezier, float t);

    // 预定义缓动函数
    static float Linear(float t);
    static float Ease(float t);
    static float EaseIn(float t);
    static float EaseOut(float t);
    static float EaseInOut(float t);
};

} // namespace lightui
```

## 使用示例

### 基本用法

```cpp
#include "core/render/animation_timeline.h"
#include "core/render/transition.h"

using namespace lightui;

// 创建动画时间轴
AnimationTimeline timeline;

// 解析 CSS transition
auto transitions = CSSTransition::Parse("opacity 0.3s ease-in-out");

// 启动过渡动画
timeline.StartTransition(
    render_object,
    "opacity",
    transitions[0],
    1.0f,  // 起始值
    0.5f   // 结束值
);

// 在渲染循环中更新
double current_time = GetCurrentTime();
timeline.Update(current_time);

// 获取当前值
auto value = timeline.GetCurrentValue(render_object, "opacity");
if (value.has_value()) {
    float opacity = std::get<float>(*value);
    // 应用到渲染对象
}
```

### 多属性过渡

```cpp
// 解析多个过渡
auto transitions = CSSTransition::Parse(
    "width 0.3s ease, height 0.3s ease 0.1s, opacity 0.5s ease-in-out"
);

// 启动多个过渡
for (const auto& transition : transitions) {
    if (transition.property == "width") {
        timeline.StartTransition(render_object, "width", transition, 100.0f, 200.0f);
    } else if (transition.property == "height") {
        timeline.StartTransition(render_object, "height", transition, 100.0f, 150.0f);
    } else if (transition.property == "opacity") {
        timeline.StartTransition(render_object, "opacity", transition, 1.0f, 0.5f);
    }
}
```

### 自定义贝塞尔曲线

```cpp
// 解析自定义缓动
auto transitions = CSSTransition::Parse(
    "transform 0.5s cubic-bezier(0.68, -0.55, 0.265, 1.55)"
);

// 创建变换
CSSTransform start_transform;
start_transform.type = TransformType::TRANSLATE;
start_transform.translate_x = 0;
start_transform.translate_y = 0;

CSSTransform end_transform;
end_transform.type = TransformType::TRANSLATE;
end_transform.translate_x = 100;
end_transform.translate_y = 0;

// 启动过渡
timeline.StartTransition(
    render_object,
    "transform",
    transitions[0],
    start_transform,
    end_transform
);
```

### 颜色过渡

```cpp
// 颜色过渡
auto transitions = CSSTransition::Parse("background-color 0.3s ease");

SkColor start_color = SkColorSetRGB(52, 152, 219);  // #3498db
SkColor end_color = SkColorSetRGB(231, 76, 60);     // #e74c3c

timeline.StartTransition(
    render_object,
    "background-color",
    transitions[0],
    start_color,
    end_color
);
```

## 性能考虑

1. **批量更新**: AnimationTimeline 在单次 Update() 调用中处理所有动画
2. **延迟启动**: 使用 transition-delay 可以创建交错动画效果
3. **硬件加速**: Transform 动画可以利用 GPU 加速（如果可用）
4. **内存管理**: 完成的动画会自动从时间轴中移除

## 限制和注意事项

1. **属性支持**: 目前支持数值、颜色和变换属性
2. **插值**: 某些属性（如字符串）不支持插值
3. **性能**: 大量同时运行的动画可能影响性能
4. **精度**: 使用浮点数进行插值，可能存在精度误差

## 参考资料

- [W3C CSS Transitions Specification](https://www.w3.org/TR/css-transitions-1/)
- [MDN CSS Transitions](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Transitions)
- [Cubic Bezier Easing Functions](https://cubic-bezier.com/)

