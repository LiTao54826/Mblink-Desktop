# Animation System

CSS 动画和过渡系统。

## 模块列表

| 文件 | 描述 |
|------|------|
| `animation_controller.h/cpp` | 动画控制器，管理所有动画 |
| `animation.h/cpp` | 动画基类 |
| `css_animation.h/cpp` | CSS @keyframes 动画 |
| `css_transition.h/cpp` | CSS 过渡动画 |
| `animation_timeline.h/cpp` | 动画时间线 |
| `animation_optimizer.h/cpp` | 动画性能优化 |
| `keyframe.h/cpp` | 关键帧定义 |
| `easing.h/cpp` | 缓动函数 |

## 功能说明

支持的 CSS 动画特性：
- @keyframes 动画
- animation-* 属性
- transition-* 属性
- 多种缓动函数
- 动画合成优化

## 架构

```
AnimationController
├── CSSAnimation (keyframes)
├── CSSTransition (property changes)
└── AnimationTimeline (timing)
```
