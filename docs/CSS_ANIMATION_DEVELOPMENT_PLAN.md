# CSS 动画支持开发计划

> **创建日期**: 2025-12-22
> **项目**: MBink / LightUI
> **当前状态**: 基础框架已完成，需要完善集成和高级特性

---

## 📊 当前实现状态分析

### ✅ 已完成的组件

| 组件 | 文件 | 状态 | 说明 |
|------|------|------|------|
| **CSSAnimation** | `animation.h/cpp` | ✅ 完成 | 动画属性解析 |
| **KeyframesRule** | `keyframes.h/cpp` | ✅ 完成 | @keyframes 规则解析 |
| **AnimationController** | `animation_controller.h/cpp` | ✅ 完成 | 动画生命周期管理 |
| **PropertyInterpolation** | `property_interpolation.h/cpp` | ✅ 完成 | 属性值插值 |
| **EasingFunctions** | `easing_functions.h/cpp` | ✅ 完成 | 缓动函数 |
| **AnimationOptimizer** | `animation_optimizer.h/cpp` | ✅ 完成 | 性能优化 |
| **AnimationTimeline** | `animation_timeline.h/cpp` | ✅ 完成 | Transition 时间线 |
| **CSSTransform** | `transform.h/cpp` | ✅ 完成 | Transform 解析 |

### 🔄 需要完善的部分

| 功能 | 当前状态 | 问题 |
|------|---------|------|
| **CSS 解析集成** | ⚠️ 部分 | @keyframes 未从 StyleManager 自动注册 |
| **样式应用集成** | ⚠️ 部分 | animation 属性未在 StyleResolver 中处理 |
| **Transform 插值** | ⚠️ 简单实现 | 仅使用阶跃函数，未实现真正插值 |
| **动画事件** | ⚠️ 框架存在 | FireAnimationEvent 未完整实现 |
| **渲染循环集成** | ⚠️ 部分 | 需要在渲染循环中调用 AnimationController::Update |

---

## 🎯 开发计划

### Phase 1: CSS 解析集成 (预计 2-3 天)

#### 1.1 @keyframes 规则自动注册

**目标**: 在 StyleManager 解析 CSS 时自动识别和注册 @keyframes 规则

**修改文件**:
- `core/lexbor/style_manager.h/cpp`
- `core/lexbor/lexbor_stylesheet.h/cpp`

**实现步骤**:
```cpp
// style_manager.h 添加
class StyleManager {
public:
    // 获取 AnimationController 引用
    AnimationController& GetAnimationController();
    
private:
    // 解析 @keyframes 规则
    void ParseKeyframesRules(const std::string& css_text);
    
    AnimationController animation_controller_;
};
```

**任务清单**:
- [ ] 在 CSS 解析时检测 @keyframes 规则
- [ ] 调用 KeyframesRule::Parse() 解析规则
- [ ] 注册到 AnimationController
- [ ] 支持多个 @keyframes 规则
- [ ] 处理规则覆盖（同名规则）

#### 1.2 animation 属性解析

**目标**: 在 StyleResolver 中解析 animation 相关属性

**修改文件**:
- `core/render/style_resolver.h/cpp`
- `core/render/render_object.h/cpp`

**实现步骤**:
```cpp
// style_resolver.cpp 添加
bool StyleResolver::ParseAnimationProperty(ComputedStyle& style,
                                          const std::string& property,
                                          const std::string& value) {
    if (property == "animation") {
        style.animations = CSSAnimation::Parse(value);
        return true;
    }
    if (property == "animation-name") {
        // 解析 animation-name
    }
    // ... 其他 animation-* 属性
}
```

**任务清单**:
- [ ] 在 ComputedStyle 中添加 animations 字段
- [ ] 解析 animation 简写属性
- [ ] 解析 animation-name, animation-duration 等分解属性
- [ ] 处理多个动画（逗号分隔）

---

### Phase 2: 渲染循环集成 (预计 2-3 天)

#### 2.1 动画更新循环

**目标**: 在渲染循环中更新动画状态

**修改文件**:
- `core/window/window.cpp` 或主渲染循环
- `core/render/renderer.cpp`

**实现步骤**:
```cpp
// 在渲染循环中
void RenderLoop() {
    double current_time = GetCurrentTime();
    
    // 更新动画
    animation_controller_.Update(current_time);
    
    // 应用动画属性到渲染对象
    ApplyAnimationProperties();
    
    // 渲染
    Render();
}
```

**任务清单**:
- [ ] 获取高精度时间戳
- [ ] 在每帧调用 AnimationController::Update()
- [ ] 将动画属性应用到 RenderObject
- [ ] 触发重绘

#### 2.2 动画属性应用

**目标**: 将动画计算的属性值应用到渲染对象

**修改文件**:
- `core/render/render_object.cpp`
- `core/render/animation_controller.cpp`

**实现步骤**:
```cpp
void ApplyAnimationProperties(RenderObject* obj) {
    for (const auto& anim : obj->GetAnimations()) {
        auto props = animation_controller_.GetCurrentProperties(obj, anim.name);
        if (props) {
            for (const auto& [prop, value] : *props) {
                ApplyStyleProperty(obj, prop, value);
            }
        }
    }
}
```

**任务清单**:
- [ ] 遍历元素的所有动画
- [ ] 获取当前帧的属性值
- [ ] 应用属性到 ComputedStyle
- [ ] 标记需要重新布局/绘制

---

### Phase 3: Transform 插值完善 (预计 3-4 天)

#### 3.1 Transform 分解和插值

**目标**: 实现 Transform 属性的真正插值

**修改文件**:
- `core/render/property_interpolation.cpp`
- `core/render/transform.h/cpp`

**实现步骤**:
```cpp
// 分解 Transform 为可插值的组件
struct DecomposedTransform {
    float translate_x, translate_y;
    float rotate;  // 角度
    float scale_x, scale_y;
    float skew_x, skew_y;
};

// 插值分解后的 Transform
DecomposedTransform InterpolateDecomposed(
    const DecomposedTransform& from,
    const DecomposedTransform& to,
    float factor);

// 重新组合为 Transform 字符串
std::string ComposeTransform(const DecomposedTransform& decomposed);
```

**任务清单**:
- [ ] 实现 Transform 分解 (Decompose)
- [ ] 实现各组件的线性插值
- [ ] 实现 Transform 重组 (Compose)
- [ ] 处理不同 Transform 函数的插值
- [ ] 处理 transform-origin

#### 3.2 矩阵插值

**目标**: 支持 matrix() 函数的插值

**实现步骤**:
```cpp
// 矩阵分解为 translate, rotate, scale, skew
bool DecomposeMatrix(const SkMatrix& matrix, DecomposedTransform& out);

// 从分解值重建矩阵
SkMatrix ComposeMatrix(const DecomposedTransform& decomposed);
```

**任务清单**:
- [ ] 实现 2D 矩阵分解算法
- [ ] 处理奇异矩阵
- [ ] 实现矩阵重组

---

### Phase 4: 动画事件系统 (预计 2 天)

#### 4.1 完善动画事件触发

**目标**: 正确触发 animationstart, animationend, animationiteration 事件

**修改文件**:
- `core/render/animation_controller.cpp`
- `core/dom/event.h/cpp`

**实现步骤**:
```cpp
void AnimationController::FireAnimationEvent(
    const RunningAnimation& anim,
    const std::string& event_type,
    float elapsed_time) {
    
    // 获取关联的 DOM 元素
    auto element = anim.object->GetElement();
    if (!element) return;
    
    // 创建 AnimationEvent
    auto event = std::make_shared<AnimationEvent>(
        event_type,
        anim.config.name,
        elapsed_time
    );
    
    // 分发事件
    element->DispatchEvent(event);
}
```

**任务清单**:
- [ ] 在 RenderObject 中添加获取 Element 的方法
- [ ] 实现 AnimationEvent 类
- [ ] 在正确的时机触发事件
- [ ] 支持事件冒泡

---

### Phase 5: JavaScript API 集成 (预计 2-3 天)

#### 5.1 Animation API 绑定

**目标**: 暴露动画控制 API 到 JavaScript

**修改文件**:
- `core/quickjs/dom_bindings.cpp`
- `core/dom/element.h/cpp`

**实现步骤**:
```javascript
// JavaScript API
element.animate(keyframes, options);
element.getAnimations();
animation.play();
animation.pause();
animation.cancel();
animation.finish();
```

**任务清单**:
- [ ] 实现 Element.animate() 方法
- [ ] 实现 Element.getAnimations() 方法
- [ ] 实现 Animation 对象的 JS 绑定
- [ ] 支持 Web Animations API 子集

---

### Phase 6: 高级特性 (预计 3-4 天)

#### 6.1 animation-play-state 支持

**任务清单**:
- [ ] 解析 animation-play-state 属性
- [ ] 支持 running/paused 状态切换
- [ ] 支持通过 CSS 控制暂停

#### 6.2 animation-fill-mode 完善

**任务清单**:
- [ ] 实现 backwards 填充模式
- [ ] 实现 both 填充模式
- [ ] 处理延迟期间的样式

#### 6.3 多动画支持

**任务清单**:
- [ ] 支持同一元素多个动画
- [ ] 处理动画属性冲突
- [ ] 实现动画优先级

---

## 📁 文件结构

```
core/render/
├── animation.h/cpp              # ✅ CSSAnimation 属性
├── animation_controller.h/cpp   # ✅ 动画控制器
├── animation_optimizer.h/cpp    # ✅ 性能优化
├── animation_timeline.h/cpp     # ✅ Transition 时间线
├── keyframes.h/cpp              # ✅ @keyframes 规则
├── property_interpolation.h/cpp # ✅ 属性插值
├── easing_functions.h/cpp       # ✅ 缓动函数
├── transition.h/cpp             # ✅ CSS Transition
└── transform.h/cpp              # ✅ CSS Transform

core/lexbor/
├── style_manager.h/cpp          # 🔄 需要添加 @keyframes 解析
└── lexbor_stylesheet.h/cpp      # 🔄 需要添加 @keyframes 检测

core/dom/
├── element.h/cpp                # 🔄 需要添加动画相关方法
└── event.h/cpp                  # ✅ AnimationEvent 已定义
```

---

## 🧪 测试计划

### 单元测试

```cpp
// test_css_animation_integration.cpp

// 1. @keyframes 解析测试
TEST(CSSAnimationIntegration, ParseKeyframesFromCSS) {
    StyleManager manager;
    manager.ParseCSSString(R"(
        @keyframes slide-in {
            from { transform: translateX(-100%); }
            to { transform: translateX(0); }
        }
    )");
    
    auto* rule = manager.GetAnimationController()
                        .GetKeyframes("slide-in");
    ASSERT_NE(rule, nullptr);
    EXPECT_EQ(rule->keyframes.size(), 2);
}

// 2. 动画属性应用测试
TEST(CSSAnimationIntegration, ApplyAnimationProperty) {
    // 测试 animation 属性被正确解析和应用
}

// 3. Transform 插值测试
TEST(CSSAnimationIntegration, InterpolateTransform) {
    auto result = PropertyInterpolation::Interpolate(
        "transform",
        "translateX(0px)",
        "translateX(100px)",
        0.5f
    );
    EXPECT_EQ(*result, "translateX(50px)");
}

// 4. 动画事件测试
TEST(CSSAnimationIntegration, AnimationEvents) {
    // 测试 animationstart, animationend, animationiteration 事件
}
```

### 集成测试

```javascript
// test_animation.js

// 1. CSS 动画测试
const div = document.createElement('div');
div.style.animation = 'slide-in 1s ease-in-out';
document.body.appendChild(div);

// 2. 动画事件测试
div.addEventListener('animationstart', (e) => {
    console.log('Animation started:', e.animationName);
});

div.addEventListener('animationend', (e) => {
    console.log('Animation ended:', e.animationName);
});

// 3. Web Animations API 测试
const animation = div.animate([
    { transform: 'translateX(0)' },
    { transform: 'translateX(100px)' }
], {
    duration: 1000,
    easing: 'ease-in-out'
});
```

---

## 📊 性能目标

| 指标 | 目标 | 说明 |
|------|------|------|
| 动画帧率 | 60 FPS | 稳定 60 FPS |
| 单帧更新时间 | < 2ms | 100 个动画 |
| 内存占用 | < 1MB | 100 个动画 |
| 缓存命中率 | > 80% | 插值缓存 |

---

## 📅 时间线

| 阶段 | 预计时间 | 优先级 |
|------|---------|--------|
| Phase 1: CSS 解析集成 | 2-3 天 | 🔴 高 |
| Phase 2: 渲染循环集成 | 2-3 天 | 🔴 高 |
| Phase 3: Transform 插值 | 3-4 天 | 🟡 中 |
| Phase 4: 动画事件系统 | 2 天 | 🟡 中 |
| Phase 5: JavaScript API | 2-3 天 | 🟢 低 |
| Phase 6: 高级特性 | 3-4 天 | 🟢 低 |

**总计**: 约 14-19 天

---

## 🔗 参考资源

- [CSS Animations Level 1](https://www.w3.org/TR/css-animations-1/)
- [CSS Transforms Level 1](https://www.w3.org/TR/css-transforms-1/)
- [Web Animations API](https://www.w3.org/TR/web-animations-1/)
- [CSS Easing Functions](https://www.w3.org/TR/css-easing-1/)

---

## 📝 备注

1. **优先级建议**: 先完成 Phase 1 和 Phase 2，这是让动画系统工作的最小可行方案
2. **Transform 插值**: 这是最复杂的部分，可以先用简单实现，后续优化
3. **Web Animations API**: 这是可选的高级特性，可以根据需求决定是否实现
4. **性能优化**: 现有的 AnimationOptimizer 已经提供了良好的基础，主要关注集成
