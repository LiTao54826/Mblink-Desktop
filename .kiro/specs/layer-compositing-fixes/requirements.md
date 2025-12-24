# Requirements Document: Layer Compositing Bug Fixes

## Introduction

本文档定义了 LightUI 分层合成系统的 Bug 修复需求。经过全面分析，发现当前实现存在以下核心问题导致重复绘画、重影和高 CPU/GPU 占用：

1. **每帧强制全量光栅化** - 增量渲染完全失效
2. **层提升逻辑未实现** - 动画元素无法获得独立层
3. **动画桥接未集成** - AnimationLayerBridge 未与 Window 层连接
4. **RenderObject 层关联断裂** - 无法追踪元素所属层

## Problem Analysis

### 问题 1: 每帧强制全量光栅化 (P0)

**根因分析:**
- `render_pipeline_v2.cpp:360-362`: `CheckRenderObjectNeedsPaint()` 检测到任何动画就触发 `root_layer_->MarkFullDirty()`
- `window.cpp:1523-1526`: 有活动动画时调用 `compositor_adapter_->ForceRasterize()`
- 结果：增量光栅化完全被绕过，每帧都重绘整个层树

**影响:**
- CPU 占用极高（每帧全量光栅化）
- 违反设计文档 Property 4: "Incremental rasterization preserves unchanged pixels"
- 违反设计文档 Property 7: "Transform/opacity animation without re-rasterization"

### 问题 2: 层提升逻辑未实现 (P1)

**根因分析:**
- `layer_tree_builder.cpp:186-195`: `HasWillChangeTransform()` 返回 `false`（TODO 未实现）
- `layer_tree_builder.cpp:197-202`: `HasWillChangeOpacity()` 返回 `false`（TODO 未实现）
- `layer_tree_builder.cpp:214-228`: `HasTransformAnimation()` 逻辑过于简化，只检查 `style.transform.has_value()`

**影响:**
- 动画元素无法获得独立层，全部在根层渲染
- 违反设计文档 Property 2: "Layer promotion based on CSS properties"
- 动画性能差，无法利用 GPU 合成优化

### 问题 3: AnimationLayerBridge 未集成 (P1)

**根因分析:**
- `AnimationLayerBridge::OnAnimationStart()` 和 `OnAnimationEnd()` 从未被调用
- Window 层的动画系统没有通知 AnimationLayerBridge
- 动画触发的层提升/降级机制完全失效

**影响:**
- 动画开始时无法自动提升元素到独立层
- 动画结束时无法降级层
- 违反设计文档 Requirements 2.3, 7.4

### 问题 4: 重影/残影 (P0)

**根因分析:**
- 动画元素在根层渲染，移动时旧位置内容未被正确清除
- `compositor.cpp:CompositeToCanvas()` 依赖调用者清除画布
- 但 `window.cpp` 中的清除逻辑在某些路径下不完整

**影响:**
- 动画元素留下残影
- 视觉效果严重受损

## Requirements

### Requirement 1: 修复增量光栅化

**User Story:** As a developer, I want the layer compositing system to only re-rasterize changed regions, so that CPU usage is minimized during animations.

#### Acceptance Criteria

1. WHEN an animation updates only transform/opacity THEN the system SHALL NOT re-rasterize the layer content
2. WHEN `CheckRenderObjectNeedsPaint()` detects animation THEN the system SHALL mark only the animated element's layer as dirty, NOT the root layer
3. WHEN `ForceRasterize()` is called THEN the system SHALL only force rasterize layers with actual content changes
4. WHEN measuring CPU usage during transform animation THEN the usage SHALL be significantly lower than full rasterization

### Requirement 2: 实现 will-change 层提升

**User Story:** As a developer, I want elements with `will-change: transform` or `will-change: opacity` to be promoted to their own compositing layer, so that animations can be GPU-accelerated.

#### Acceptance Criteria

1. WHEN an element has `will-change: transform` THEN the system SHALL create a dedicated compositing layer
2. WHEN an element has `will-change: opacity` THEN the system SHALL create a dedicated compositing layer
3. WHEN checking layer promotion THEN the system SHALL parse the `will-change` CSS property from ComputedStyle
4. WHEN a promoted layer animates THEN the system SHALL only update transform/opacity without re-rasterizing

### Requirement 3: 修复动画检测逻辑

**User Story:** As a developer, I want the system to correctly detect which elements have active animations, so that layer promotion works correctly.

#### Acceptance Criteria

1. WHEN an element has CSS animation on transform THEN `HasTransformAnimation()` SHALL return true
2. WHEN an element has CSS animation on opacity THEN `HasOpacityAnimation()` SHALL return true
3. WHEN checking animation THEN the system SHALL query the AnimationController for running animations
4. WHEN animation targets are determined THEN the system SHALL check keyframe definitions, not just current style values

### Requirement 4: 集成 AnimationLayerBridge

**User Story:** As a developer, I want the animation system to notify the layer system when animations start/end, so that layers can be promoted/demoted automatically.

#### Acceptance Criteria

1. WHEN a CSS animation starts THEN the system SHALL call `AnimationLayerBridge::OnAnimationStart()`
2. WHEN a CSS animation ends THEN the system SHALL call `AnimationLayerBridge::OnAnimationEnd()`
3. WHEN `OnAnimationStart()` is called THEN the system SHALL promote the element to its own layer if needed
4. WHEN `OnAnimationEnd()` is called THEN the system SHALL demote the layer if no other promotion reasons exist

### Requirement 5: 修复重影问题

**User Story:** As a developer, I want animated elements to not leave visual artifacts at their previous positions, so that the UI looks correct.

#### Acceptance Criteria

1. WHEN an animated element moves THEN the previous position SHALL be correctly cleared
2. WHEN compositing layers THEN the system SHALL ensure proper clearing of affected regions
3. WHEN using independent layers THEN the system SHALL handle layer bounds correctly during animation
4. WHEN testing animation THEN there SHALL be no visible ghosting or artifacts

### Requirement 6: 修复 RenderObject 层关联

**User Story:** As a developer, I want RenderObjects to correctly track their associated compositing layer, so that dirty region marking works correctly.

#### Acceptance Criteria

1. WHEN `LayerTreeBuilder::CreateLayer()` creates a layer THEN it SHALL set the `layer_info_.compositor_layer` weak_ptr on the RenderObject
2. WHEN `RenderObject::HasOwnCompositorLayer()` is called THEN it SHALL return true if the element has a dedicated layer
3. WHEN marking dirty regions THEN the system SHALL mark the correct layer, not always the root layer

## Glossary

- **will-change**: CSS 属性，提示浏览器元素即将发生变化，应该提前优化
- **Layer Promotion**: 将元素提升到独立合成层的过程
- **Layer Demotion**: 将独立层合并回父层的过程
- **Incremental Rasterization**: 只重绘变化区域的光栅化策略
- **AnimationLayerBridge**: 连接动画系统和层系统的桥接组件
