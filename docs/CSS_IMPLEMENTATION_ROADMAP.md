# CSS 高级特性实施路线图

> **版本**: 1.0  
> **日期**: 2025-11-14  
> **目标**: 将 CSS 高级特性集成到现有 MBink 架构

---

## 🏗️ 现有架构分析

### 当前渲染系统结构

```
core/render/
├── css_value.h/cpp          # CSS 值解析（已有）
├── style_resolver.h/cpp     # 样式解析器（已有）
├── render_object.h/cpp      # 渲染对象（已有）
├── unified_renderer.h/cpp   # 统一渲染器（已有）
├── box_renderer.h/cpp       # 盒子渲染器（已有）
├── text_renderer.h/cpp      # 文本渲染器（已有）
├── layer.h/cpp              # 图层系统（已有）
├── dirty_region.h/cpp       # 脏区域管理（已有）
└── performance_monitor.h/cpp # 性能监控（已有）
```

### 需要新增的模块

```
core/render/
├── shadow_renderer.h/cpp        # 阴影渲染器（新增）
├── gradient_renderer.h/cpp      # 渐变渲染器（新增）
├── transform_renderer.h/cpp     # Transform 渲染器（新增）
├── transition_manager.h/cpp     # 过渡管理器（新增）
├── animation_manager.h/cpp      # 动画管理器（新增）
├── filter_renderer.h/cpp        # 滤镜渲染器（新增）
└── easing.h/cpp                 # 缓动函数（新增）

core/lexbor/
└── keyframes_parser.h/cpp       # @keyframes 解析器（新增）
```

---

## 📋 实施步骤

### Step 1: 扩展 CSS 数据结构 (1天)

#### 修改文件: `core/render/css_value.h`

**新增结构体**:

```cpp
/**
 * @brief CSS 盒子阴影
 */
struct CSSBoxShadow {
    float offset_x;
    float offset_y;
    float blur_radius;
    float spread_radius;
    SkColor color;
    bool inset;
    
    CSSBoxShadow() 
        : offset_x(0), offset_y(0), blur_radius(0), 
          spread_radius(0), color(SK_ColorBLACK), inset(false) {}
};

/**
 * @brief CSS 文本阴影
 */
struct CSSTextShadow {
    float offset_x;
    float offset_y;
    float blur_radius;
    SkColor color;
    
    CSSTextShadow() 
        : offset_x(0), offset_y(0), blur_radius(0), 
          color(SK_ColorBLACK) {}
};

/**
 * @brief CSS 渐变色标
 */
struct CSSGradientStop {
    SkColor color;
    float position;  // 0.0 到 1.0
    
    CSSGradientStop(SkColor c, float p) : color(c), position(p) {}
};

/**
 * @brief CSS 线性渐变
 */
struct CSSLinearGradient {
    float angle;  // 角度（度）
    std::vector<CSSGradientStop> stops;
    
    CSSLinearGradient() : angle(180.0f) {}  // 默认向下
};

/**
 * @brief CSS 径向渐变
 */
struct CSSRadialGradient {
    float center_x;  // 0.0 到 1.0
    float center_y;  // 0.0 到 1.0
    float radius;
    bool is_circle;
    std::vector<CSSGradientStop> stops;
    
    CSSRadialGradient() 
        : center_x(0.5f), center_y(0.5f), radius(0.5f), 
          is_circle(true) {}
};

/**
 * @brief CSS Transform 类型
 */
enum class TransformType {
    TRANSLATE,
    ROTATE,
    SCALE,
    SKEW,
    MATRIX
};

/**
 * @brief CSS Transform
 */
struct Transform {
    TransformType type;
    std::vector<float> values;
    
    Transform(TransformType t) : type(t) {}
};

/**
 * @brief CSS Transform 集合
 */
struct CSSTransform {
    std::vector<Transform> transforms;
    
    // 转换为 Skia Matrix
    SkMatrix ToSkMatrix(const SkRect& rect, const SkPoint& origin) const;
};

/**
 * @brief CSS Transform Origin
 */
struct TransformOrigin {
    CSSLength x;
    CSSLength y;
    
    TransformOrigin() 
        : x(50.0f, CSSUnit::PERCENT), 
          y(50.0f, CSSUnit::PERCENT) {}
    
    SkPoint ToPoint(const SkRect& rect, float font_size = 16.0f) const;
};
```

#### 修改文件: `core/render/css_value.cpp`

**新增解析函数**:

```cpp
// 解析 box-shadow
std::vector<CSSBoxShadow> CSSValue::ParseBoxShadow(const std::string& value);

// 解析 text-shadow
std::vector<CSSTextShadow> CSSValue::ParseTextShadow(const std::string& value);

// 解析 linear-gradient
CSSLinearGradient CSSValue::ParseLinearGradient(const std::string& value);

// 解析 radial-gradient
CSSRadialGradient CSSValue::ParseRadialGradient(const std::string& value);

// 解析 transform
CSSTransform CSSValue::ParseTransform(const std::string& value);

// 解析 transform-origin
TransformOrigin CSSValue::ParseTransformOrigin(const std::string& value);
```

---

### Step 2: 扩展 ComputedStyle (0.5天)

#### 修改文件: `core/render/render_object.h`

**在 ComputedStyle 结构体中新增字段**:

```cpp
struct ComputedStyle {
    // ... 现有字段 ...
    
    // === 阴影 ===
    std::vector<CSSBoxShadow> box_shadows;
    std::vector<CSSTextShadow> text_shadows;
    
    // === 渐变 ===
    std::optional<CSSLinearGradient> background_linear_gradient;
    std::optional<CSSRadialGradient> background_radial_gradient;
    
    // === Transform ===
    CSSTransform transform;
    TransformOrigin transform_origin;
    
    // === Transition ===
    struct TransitionConfig {
        std::string property;
        float duration;
        std::string timing_function;
        float delay;
    };
    std::vector<TransitionConfig> transitions;
    
    // === Animation ===
    struct AnimationConfig {
        std::string name;
        float duration;
        std::string timing_function;
        float delay;
        int iteration_count;
        std::string direction;
        std::string fill_mode;
    };
    std::vector<AnimationConfig> animations;
    
    // === Filter ===
    struct FilterConfig {
        std::string type;
        float value;
    };
    std::vector<FilterConfig> filters;
    
    // === CSS 变量 ===
    std::map<std::string, std::string> css_variables;
};
```

---

### Step 3: 扩展 StyleResolver (1天)

#### 修改文件: `core/render/style_resolver.cpp`

**在 `ParseStyleProperty()` 中新增处理**:

```cpp
void StyleResolver::ParseStyleProperty(ComputedStyle& style,
                                      const std::string& property,
                                      const std::string& value) {
    // ... 现有代码 ...
    
    // === 阴影 ===
    if (property == "box-shadow") {
        style.box_shadows = CSSValue::ParseBoxShadow(value);
    }
    else if (property == "text-shadow") {
        style.text_shadows = CSSValue::ParseTextShadow(value);
    }
    
    // === 渐变 ===
    else if (property == "background-image") {
        if (value.find("linear-gradient") != std::string::npos) {
            style.background_linear_gradient = CSSValue::ParseLinearGradient(value);
        }
        else if (value.find("radial-gradient") != std::string::npos) {
            style.background_radial_gradient = CSSValue::ParseRadialGradient(value);
        }
    }
    
    // === Transform ===
    else if (property == "transform") {
        style.transform = CSSValue::ParseTransform(value);
    }
    else if (property == "transform-origin") {
        style.transform_origin = CSSValue::ParseTransformOrigin(value);
    }
    
    // === Transition ===
    else if (property == "transition") {
        // 解析 transition 属性
        // 格式: property duration timing-function delay
    }
    
    // === Animation ===
    else if (property == "animation") {
        // 解析 animation 属性
    }
    
    // === Filter ===
    else if (property == "filter") {
        // 解析 filter 属性
    }
}
```

---

### Step 4: 创建渲染器模块 (6天)

#### 4.1 创建 ShadowRenderer (2天)

**文件**: `core/render/shadow_renderer.h`

```cpp
#pragma once

#include "include/core/SkCanvas.h"
#include "css_value.h"

namespace lightui {

class ShadowRenderer {
public:
    ShadowRenderer() = default;
    ~ShadowRenderer() = default;
    
    // 渲染盒子阴影
    void RenderBoxShadow(SkCanvas* canvas,
                        const SkRect& rect,
                        const std::vector<CSSBoxShadow>& shadows,
                        float border_radius = 0.0f);
    
    // 渲染文本阴影
    void RenderTextWithShadow(SkCanvas* canvas,
                             const std::string& text,
                             const SkFont& font,
                             float x, float y,
                             SkColor text_color,
                             const std::vector<CSSTextShadow>& shadows);
    
private:
    void RenderOutsetShadow(SkCanvas* canvas,
                           const SkRect& rect,
                           const CSSBoxShadow& shadow,
                           float border_radius);
    
    void RenderInsetShadow(SkCanvas* canvas,
                          const SkRect& rect,
                          const CSSBoxShadow& shadow,
                          float border_radius);
};

} // namespace lightui
```

#### 4.2 创建 GradientRenderer (2天)

**文件**: `core/render/gradient_renderer.h`

```cpp
#pragma once

#include "include/core/SkCanvas.h"
#include "include/effects/SkGradientShader.h"
#include "css_value.h"

namespace lightui {

class GradientRenderer {
public:
    GradientRenderer() = default;
    ~GradientRenderer() = default;
    
    // 创建线性渐变 Shader
    sk_sp<SkShader> CreateLinearGradient(const SkRect& rect,
                                         const CSSLinearGradient& gradient);
    
    // 创建径向渐变 Shader
    sk_sp<SkShader> CreateRadialGradient(const SkRect& rect,
                                         const CSSRadialGradient& gradient);
    
    // 渲染线性渐变背景
    void RenderLinearGradient(SkCanvas* canvas,
                             const SkRect& rect,
                             const CSSLinearGradient& gradient);
    
    // 渲染径向渐变背景
    void RenderRadialGradient(SkCanvas* canvas,
                             const SkRect& rect,
                             const CSSRadialGradient& gradient);
};

} // namespace lightui
```

#### 4.3 创建 TransformRenderer (2天)

**文件**: `core/render/transform_renderer.h`

```cpp
#pragma once

#include "include/core/SkCanvas.h"
#include "include/core/SkMatrix.h"
#include "css_value.h"

namespace lightui {

class TransformRenderer {
public:
    TransformRenderer() = default;
    ~TransformRenderer() = default;
    
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

} // namespace lightui
```

---

### Step 5: 集成到 UnifiedRenderer (2天)

#### 修改文件: `core/render/unified_renderer.h`

**新增成员变量**:

```cpp
class UnifiedRenderer {
private:
    // ... 现有成员 ...
    
    std::unique_ptr<ShadowRenderer> shadow_renderer_;
    std::unique_ptr<GradientRenderer> gradient_renderer_;
    std::unique_ptr<TransformRenderer> transform_renderer_;
};
```

#### 修改文件: `core/render/unified_renderer.cpp`

**修改 `RenderElement()` 方法**:

```cpp
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
    
    // 2. 渲染阴影
    if (!style.box_shadows.empty()) {
        shadow_renderer_->RenderBoxShadow(canvas, rect, 
                                          style.box_shadows,
                                          GetBorderRadius(style));
    }
    
    // 3. 渲染背景（包括渐变）
    if (style.background_linear_gradient.has_value()) {
        gradient_renderer_->RenderLinearGradient(canvas, rect, 
                                                 *style.background_linear_gradient);
    } else if (style.background_radial_gradient.has_value()) {
        gradient_renderer_->RenderRadialGradient(canvas, rect,
                                                 *style.background_radial_gradient);
    } else {
        RenderBackground(canvas, obj);
    }
    
    // 4. 渲染边框
    RenderBorder(canvas, obj);
    
    // 5. 渲染内容
    RenderContent(canvas, obj);
    
    // 6. 渲染子元素
    for (auto& child : obj->GetChildren()) {
        RenderElement(canvas, child.get());
    }
    
    // 恢复 canvas 状态
    canvas->restore();
}
```

---

## 🧪 测试策略

### 单元测试

每个模块创建对应的测试文件：

```
tests/unit/
├── test_shadow_renderer.cpp
├── test_gradient_renderer.cpp
├── test_transform_renderer.cpp
├── test_transition_manager.cpp
└── test_animation_manager.cpp
```

### 集成测试

创建完整的示例应用：

```
examples/
├── shadow_demo/
├── gradient_demo/
├── transform_demo/
├── transition_demo/
└── animation_demo/
```

---

## 📊 进度追踪

| 步骤 | 预计时间 | 实际时间 | 状态 |
|------|---------|---------|------|
| Step 1: 扩展数据结构 | 1天 | - | ⚪ 未开始 |
| Step 2: 扩展 ComputedStyle | 0.5天 | - | ⚪ 未开始 |
| Step 3: 扩展 StyleResolver | 1天 | - | ⚪ 未开始 |
| Step 4.1: ShadowRenderer | 2天 | - | ⚪ 未开始 |
| Step 4.2: GradientRenderer | 2天 | - | ⚪ 未开始 |
| Step 4.3: TransformRenderer | 2天 | - | ⚪ 未开始 |
| Step 5: 集成到 UnifiedRenderer | 2天 | - | ⚪ 未开始 |

---

**维护者**: MBink Team  
**最后更新**: 2025-11-14

