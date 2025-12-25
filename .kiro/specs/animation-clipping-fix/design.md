# Design Document: Animation Clipping Fix

## Overview

本设计文档描述了修复动画分层裁剪问题的技术方案。核心问题是动画元素在旋转或移动时被错误裁剪，因为层边界是基于元素的静态布局位置计算的，没有考虑动画过程中变换的动态变化范围。

解决方案的核心思路是：
1. **预计算动画边界** - 从 keyframes 提取所有变换值，计算能容纳整个动画过程的边界
2. **扩展层边界** - 使用预计算的动画边界来设置层的 bounds
3. **调整光栅化** - 确保光栅化时使用扩展后的边界，正确偏移绘制位置

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     Animation Bounds System                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────┐    ┌──────────────────┐                   │
│  │  KeyframesRule   │───▶│ AnimationBounds  │                   │
│  │  (keyframes.h)   │    │   Calculator     │                   │
│  └──────────────────┘    └────────┬─────────┘                   │
│                                   │                              │
│                                   ▼                              │
│  ┌──────────────────┐    ┌──────────────────┐                   │
│  │  RenderObject    │───▶│ LayerTreeBuilder │                   │
│  │  (layout info)   │    │ (bounds calc)    │                   │
│  └──────────────────┘    └────────┬─────────┘                   │
│                                   │                              │
│                                   ▼                              │
│  ┌──────────────────┐    ┌──────────────────┐                   │
│  │ CompositorLayer  │◀───│   Rasterizer     │                   │
│  │ (expanded bounds)│    │ (offset drawing) │                   │
│  └──────────────────┘    └──────────────────┘                   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 数据流

1. **Keyframes 解析** → 提取所有关键帧的 transform 值
2. **边界计算** → 计算所有变换的联合边界框
3. **层创建** → 使用扩展边界创建 CompositorLayer
4. **光栅化** → 使用扩展边界分配位图，偏移绘制位置
5. **合成** → 使用扩展边界的位置进行合成

## Components and Interfaces

### 1. AnimationBoundsCalculator (新增)

负责计算动画的完整边界范围。

```cpp
// core/compositor/animation_bounds_calculator.h

namespace lightui {

/**
 * @brief 动画边界信息
 */
struct AnimationBounds {
    SkRect bounds;           // 扩展后的边界（相对于元素原点）
    SkPoint offset;          // 相对于原始位置的偏移
    bool needs_expansion;    // 是否需要扩展
    
    AnimationBounds() 
        : bounds(SkRect::MakeEmpty())
        , offset({0, 0})
        , needs_expansion(false) {}
};

/**
 * @brief 动画边界计算器
 */
class AnimationBoundsCalculator {
public:
    /**
     * @brief 计算动画的完整边界
     * @param element_size 元素的原始尺寸
     * @param animation_name 动画名称
     * @param transform_origin 变换原点
     * @return 动画边界信息
     */
    static AnimationBounds Calculate(
        const SkSize& element_size,
        const std::string& animation_name,
        const TransformOrigin& transform_origin);
    
    /**
     * @brief 计算旋转动画的边界
     * @param element_size 元素尺寸
     * @param min_angle 最小旋转角度（度）
     * @param max_angle 最大旋转角度（度）
     * @param origin 变换原点
     * @return 动画边界
     */
    static AnimationBounds CalculateRotationBounds(
        const SkSize& element_size,
        float min_angle,
        float max_angle,
        const SkPoint& origin);
    
    /**
     * @brief 计算位移动画的边界
     * @param element_size 元素尺寸
     * @param translations 所有位移值列表
     * @return 动画边界
     */
    static AnimationBounds CalculateTranslationBounds(
        const SkSize& element_size,
        const std::vector<SkPoint>& translations);
    
    /**
     * @brief 计算缩放动画的边界
     * @param element_size 元素尺寸
     * @param max_scale_x 最大 X 缩放
     * @param max_scale_y 最大 Y 缩放
     * @param origin 变换原点
     * @return 动画边界
     */
    static AnimationBounds CalculateScaleBounds(
        const SkSize& element_size,
        float max_scale_x,
        float max_scale_y,
        const SkPoint& origin);
    
    /**
     * @brief 从 keyframes 提取所有变换值
     * @param keyframes 关键帧规则
     * @param element_size 元素尺寸（用于解析百分比）
     * @return 所有变换矩阵列表
     */
    static std::vector<SkMatrix> ExtractTransforms(
        const KeyframesRule& keyframes,
        const SkSize& element_size,
        const TransformOrigin& origin);
    
    /**
     * @brief 计算多个变换的联合边界
     * @param element_size 元素尺寸
     * @param transforms 变换矩阵列表
     * @return 联合边界
     */
    static AnimationBounds CalculateUnionBounds(
        const SkSize& element_size,
        const std::vector<SkMatrix>& transforms);
};

} // namespace lightui
```

### 2. LayerTreeBuilder 修改

修改 `UpdateLayerBounds()` 方法以使用动画边界计算器。

```cpp
// 修改 layer_tree_builder.cpp

void LayerTreeBuilder::UpdateLayerBounds(CompositorLayer* layer, RenderObject* obj) {
    // ... 现有代码 ...
    
    // 新增：计算动画边界
    AnimationBounds anim_bounds = CalculateAnimationBounds(obj);
    
    if (anim_bounds.needs_expansion) {
        // 使用动画边界扩展层边界
        width = anim_bounds.bounds.width();
        height = anim_bounds.bounds.height();
        offset_x = anim_bounds.offset.x();
        offset_y = anim_bounds.offset.y();
    }
    
    // ... 设置边界 ...
}

AnimationBounds LayerTreeBuilder::CalculateAnimationBounds(RenderObject* obj) {
    const auto& style = obj->GetComputedStyle();
    const auto& layout = obj->GetLayoutInfo();
    
    AnimationBounds result;
    
    // 检查是否有动画
    for (const auto& anim : style.animations) {
        if (anim.name.empty() || anim.name == "none") {
            continue;
        }
        
        // 获取 keyframes
        const KeyframesRule* keyframes = 
            KeyframesManager::Instance().GetKeyframes(anim.name);
        if (!keyframes) {
            continue;
        }
        
        // 计算动画边界
        SkSize element_size = SkSize::Make(layout.width, layout.height);
        AnimationBounds bounds = AnimationBoundsCalculator::Calculate(
            element_size, anim.name, style.transform_origin);
        
        // 合并边界
        if (bounds.needs_expansion) {
            result.bounds.join(bounds.bounds);
            result.offset.fX = std::min(result.offset.fX, bounds.offset.fX);
            result.offset.fY = std::min(result.offset.fY, bounds.offset.fY);
            result.needs_expansion = true;
        }
    }
    
    return result;
}
```

### 3. Rasterizer 修改

修改光栅化逻辑以正确处理扩展边界。

```cpp
// 修改 rasterizer.cpp

bool Rasterizer::RasterizeLayer(CompositorLayer* layer) {
    // ... 现有代码 ...
    
    // 关键修改：对于有动画边界扩展的层，需要额外偏移绘制位置
    if (layer->GetPromotionReason() != LayerPromotionReason::RootLayer) {
        const auto& layout = render_obj->GetLayoutInfo();
        canvas->translate(-layout.x, -layout.y);
        
        // 新增：应用动画边界偏移
        // 层边界已经扩展，需要将内容绘制在正确的位置
        const SkRect& bounds = layer->GetBounds();
        const AnimationBounds* anim_bounds = layer->GetAnimationBounds();
        if (anim_bounds && anim_bounds->needs_expansion) {
            // 偏移以补偿边界扩展
            canvas->translate(-anim_bounds->offset.fX, -anim_bounds->offset.fY);
        }
    }
    
    // ... 绘制 ...
}
```

### 4. CompositorLayer 修改

添加动画边界信息存储。

```cpp
// 修改 compositor_layer.h

class CompositorLayer {
public:
    // ... 现有代码 ...
    
    /**
     * @brief 获取动画边界信息
     */
    const AnimationBounds* GetAnimationBounds() const { 
        return animation_bounds_.has_value() ? &animation_bounds_.value() : nullptr; 
    }
    
    /**
     * @brief 设置动画边界信息
     */
    void SetAnimationBounds(const AnimationBounds& bounds) { 
        animation_bounds_ = bounds; 
    }
    
    /**
     * @brief 清除动画边界信息
     */
    void ClearAnimationBounds() { 
        animation_bounds_.reset(); 
    }

private:
    // ... 现有成员 ...
    std::optional<AnimationBounds> animation_bounds_;
};
```

## Data Models

### AnimationBounds

```cpp
struct AnimationBounds {
    SkRect bounds;           // 扩展后的边界（相对于元素原点）
    SkPoint offset;          // 相对于原始位置的偏移（通常为负值）
    bool needs_expansion;    // 是否需要扩展
};
```

**字段说明：**
- `bounds`: 能容纳整个动画过程的边界矩形
- `offset`: 边界相对于元素原始位置的偏移。例如，如果旋转导致左上角向左上移动 20px，则 offset = (-20, -20)
- `needs_expansion`: 标记是否需要扩展边界

### 边界计算示例

**旋转动画：**
```
原始矩形: 100x100
旋转 45°:
  - 对角线长度 = √(100² + 100²) ≈ 141.4
  - 扩展边界 = 141.4 x 141.4
  - 偏移 = (-(141.4-100)/2, -(141.4-100)/2) ≈ (-20.7, -20.7)
```

**位移动画：**
```
原始位置: (0, 0)
Keyframes: 
  0%: translateY(0)
  50%: translateY(-50px)
  100%: translateY(0)

扩展边界:
  - 最小 Y = -50
  - 最大 Y = 100 (原始高度)
  - 边界高度 = 100 + 50 = 150
  - 偏移 = (0, -50)
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Rotation bounds contain all rotated corners
*For any* rectangle with any rotation angle, the calculated animation bounds SHALL contain all four corners of the rotated rectangle.

**Validates: Requirements 1.2, 3.1, 3.2**

### Property 2: Translation bounds include all positions
*For any* set of translation keyframes, the calculated animation bounds SHALL include the element at every translated position.

**Validates: Requirements 1.3, 4.1, 4.2, 4.3**

### Property 3: Combined transform bounds are correct
*For any* combination of rotation, translation, and scale transforms, the calculated bounds SHALL contain the element at every keyframe.

**Validates: Requirements 1.1, 3.4, 4.4**

### Property 4: Rasterized content is not clipped
*For any* animated layer with expanded bounds, the rasterized bitmap SHALL contain the complete transformed content without clipping.

**Validates: Requirements 5.1, 5.2, 5.3, 5.4**

### Property 5: Dynamic bounds update correctly
*For any* element, when animation is added or removed, the layer bounds SHALL be recalculated to match the new animation requirements.

**Validates: Requirements 2.1, 2.2, 2.3**

### Property 6: Scale bounds accommodate maximum scale
*For any* scale animation, the bounds SHALL be large enough to contain the element at maximum scale.

**Validates: Requirements 1.4**

## Error Handling

1. **无效的 keyframes** - 如果动画名称对应的 keyframes 不存在，使用当前帧的变换值计算边界
2. **解析失败** - 如果 transform 字符串解析失败，回退到原始边界
3. **极端值** - 对于极大的缩放或位移值，设置合理的上限以避免内存问题

## Testing Strategy

### 单元测试

1. **AnimationBoundsCalculator 测试**
   - 测试旋转边界计算（各种角度）
   - 测试位移边界计算（正负值、百分比）
   - 测试缩放边界计算
   - 测试组合变换边界计算

2. **Keyframes 提取测试**
   - 测试从 keyframes 提取 transform 值
   - 测试百分比值解析

### 属性测试

使用 property-based testing 验证正确性属性：

1. **旋转边界属性测试** - 生成随机矩形和旋转角度，验证边界包含所有角点
2. **位移边界属性测试** - 生成随机位移值，验证边界包含所有位置
3. **组合变换属性测试** - 生成随机组合变换，验证边界正确

### 集成测试

1. **视觉测试** - 运行动画测试用例，验证无裁剪
2. **性能测试** - 验证边界计算不影响动画性能

### Property-Based Testing Framework

使用 C++ 的 RapidCheck 库进行属性测试。

```cpp
// 示例属性测试
RC_GTEST_PROP(AnimationBoundsTest, RotationBoundsContainAllCorners,
              (float width, float height, float angle)) {
    RC_PRE(width > 0 && width < 10000);
    RC_PRE(height > 0 && height < 10000);
    
    SkSize size = SkSize::Make(width, height);
    SkPoint origin = SkPoint::Make(width / 2, height / 2);
    
    auto bounds = AnimationBoundsCalculator::CalculateRotationBounds(
        size, 0, angle, origin);
    
    // 验证所有旋转后的角点都在边界内
    SkMatrix rotation;
    rotation.setRotate(angle, origin.x(), origin.y());
    
    SkPoint corners[4] = {
        {0, 0}, {width, 0}, {width, height}, {0, height}
    };
    rotation.mapPoints(corners, 4);
    
    for (const auto& corner : corners) {
        RC_ASSERT(bounds.bounds.contains(corner.x() - bounds.offset.x(),
                                         corner.y() - bounds.offset.y()));
    }
}
```

