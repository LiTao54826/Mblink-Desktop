# Requirements Document

## Introduction

本文档定义了将动画系统从关联 RenderObject 改为关联 Element 的重构需求。当前实现中，动画通过 `RenderObject*` 指针关联，当渲染树重建时（如添加 Modal），旧的 RenderObject 被销毁，导致动画被中断。参考 Blink 的实现，动画应该关联到 DOM Element，这样渲染树重建不会影响动画的持续运行。

### 受影响的组件

以下组件使用 `RenderObject*` 关联动画，需要重构：

1. **AnimationController** - CSS 动画控制器，管理 @keyframes 动画
2. **AnimationTimeline** - CSS 过渡动画时间轴
3. **AnimationApplicator** - 动画值应用器
4. **AnimationOptimizer** - 动画性能优化器
5. **AnimationLayerBridge** - 动画与合成层的桥接

### 不需要重构的组件

以下组件虽然使用 `RenderObject*`，但不需要重构：

1. **LayerTreeBuilder** - 层树构建器，每帧从渲染树重建，不存储持久状态
2. **RenderPipeline** - 渲染管线，只是传递参数，不存储动画状态
3. **PaintLayer** - 绘制层，与 RenderObject 生命周期一致
4. **ClipOptimizer** - 裁剪优化器，只是查询，不存储状态

## Glossary

- **Element**: DOM 元素节点，在渲染树重建时保持稳定
- **RenderObject**: 渲染对象，负责布局和绘制，可能在 DOM 变化时被销毁重建
- **RunningAnimation**: 运行中的 CSS 动画实例
- **RunningTransition**: 运行中的 CSS 过渡实例
- **AnimationController**: 动画控制器，管理所有运行中的 CSS 动画
- **AnimationTimeline**: 过渡时间轴，管理所有运行中的 CSS 过渡
- **InvalidateRenderTree**: 标记渲染树需要重建的操作

## Requirements

### Requirement 1

**User Story:** As a user, I want animations to continue running when DOM changes occur (like opening a modal), so that the UI remains smooth and responsive.

#### Acceptance Criteria

1. WHEN a modal is opened while a spinner animation is running THEN the AnimationController SHALL continue the spinner animation without interruption
2. WHEN the render tree is rebuilt due to DOM changes THEN the AnimationController SHALL preserve all running animation states
3. WHEN a new RenderObject is created for an Element with running animations THEN the AnimationController SHALL apply animation values to the new RenderObject
4. WHEN an Element with running animations is removed from DOM THEN the AnimationController SHALL stop and clean up those animations

### Requirement 2

**User Story:** As a developer, I want the AnimationController to use Element references instead of RenderObject pointers, so that animations survive render tree rebuilds.

#### Acceptance Criteria

1. THE RunningAnimation structure SHALL store a weak reference to Element instead of a raw RenderObject pointer
2. WHEN AnimationController::Update is called THEN the AnimationController SHALL obtain the current RenderObject from Element for each animation
3. WHEN AnimationController::StartAnimation is called with a RenderObject THEN the AnimationController SHALL extract and store the associated Element
4. IF an Element's RenderObject is null during animation update THEN the AnimationController SHALL skip applying values but preserve the animation state

### Requirement 3

**User Story:** As a developer, I want the AnimationTimeline to use Element references instead of RenderObject pointers, so that CSS transitions survive render tree rebuilds.

#### Acceptance Criteria

1. THE RunningTransition structure SHALL store a weak reference to Element instead of a raw RenderObject pointer
2. WHEN AnimationTimeline::Update is called THEN the AnimationTimeline SHALL obtain the current RenderObject from Element for each transition
3. WHEN AnimationTimeline::StartTransition is called with a RenderObject THEN the AnimationTimeline SHALL extract and store the associated Element
4. IF an Element's RenderObject is null during transition update THEN the AnimationTimeline SHALL skip applying values but preserve the transition state

### Requirement 4

**User Story:** As a developer, I want the AnimationApplicator to work with Element references, so that it can apply animations to newly created RenderObjects.

#### Acceptance Criteria

1. WHEN AnimationApplicator::ApplyAnimationValues is called THEN the AnimationApplicator SHALL obtain the current RenderObject from Element
2. THE AnimationApplicator SHALL provide methods that accept Element references in addition to RenderObject pointers
3. WHEN applying animation values THEN the AnimationApplicator SHALL check if the Element has a valid RenderObject before applying

### Requirement 5

**User Story:** As a developer, I want the AnimationOptimizer and AnimationLayerBridge to use Element references, so that optimization state survives render tree rebuilds.

#### Acceptance Criteria

1. THE AnimationOptimizer SHALL use Element references for dirty tracking instead of RenderObject pointers
2. THE AnimationLayerBridge SHALL use Element references for animation tracking instead of RenderObject pointers
3. WHEN checking animation state THEN these components SHALL obtain the current RenderObject from Element

### Requirement 6

**User Story:** As a developer, I want the InvalidateRenderTree function to not clear animations, so that DOM changes don't interrupt running animations.

#### Acceptance Criteria

1. WHEN InvalidateRenderTree is called THEN the Window SHALL NOT clear running animations from AnimationController
2. WHEN InvalidateRenderTree is called THEN the Window SHALL NOT clear running transitions from AnimationTimeline
3. WHEN InvalidateRenderTree is called THEN the Window SHALL only invalidate the render tree and layer tree
