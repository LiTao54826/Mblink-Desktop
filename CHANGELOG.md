# Changelog

All notable changes to MBink will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.90.0] - 2025-11-28

### Added - Phase 8: Taffy CSS 布局引擎 (2025-11-28) ✅ 已完成
- **Taffy 布局引擎集成** (替代 Yoga)
  - 完整 CSS Flexbox 支持
  - CSS Grid 布局支持
  - grid-template-columns/rows 模板解析
  - Position (relative, absolute, fixed) 支持
  - Overflow 处理
- **渲染优化**
  - D3D11 DisplayBackend (无闪烁 CPU 渲染)
  - DPI 缩放检测和支持
  - 滚动条渲染支持
  - font-family 解析支持
- **Flexbox 修复**
  - 启动黑屏问题修复
  - resize 性能优化
  - weak_ptr 正确跟踪渲染树

### Added - Phase 7: HTML/CSS 完整支持 (2025-11-15) ✅ 已完成
- **HTML5 解析增强**
  - 错误处理和警告系统
  - 文档模式检测 (quirks/standards)
  - DOCTYPE 处理 (HTML5, HTML4, XHTML)
  - HTML 实体解析 (200+ 实体)
  - 特殊元素处理 (script, style, template, SVG)
  - 健壮的错误恢复机制
- **CSS3 选择器支持**
  - 所有基本选择器 (type, class, ID, universal)
  - 所有组合选择器 (descendant, child, sibling)
  - 所有属性选择器 (7 种变体)
  - 所有结构伪类 (14 种)
  - 所有表单伪类 (3 种)
  - 所有动态伪类 (hover, active, focus 等)
  - 所有伪元素 (::before, ::after 等)
- **表单元素支持**
  - 所有 HTML5 input 类型
  - 完整表单验证
  - 表单状态管理
- **测试**: 283 单元测试 + 50 性能测试 = 333 个测试

## [0.5.0-alpha] - 2025-11-14

### Added - Phase 4: CSS Animation (2025-11-14) ✅ 已完成
- **CSS @keyframes**: Complete support for keyframe animations
  - `@keyframes` rule parsing with regex
  - Keyframe offset parsing (0%, 50%, 100%, from, to)
  - Property declarations within keyframes
  - KeyframesManager singleton for global keyframe storage
- **CSS Animation Properties**: Full animation control
  - `animation-name`: Reference to @keyframes rule
  - `animation-duration`: Animation duration in seconds or milliseconds
  - `animation-timing-function`: Easing functions (linear, ease, ease-in, ease-out, ease-in-out, cubic-bezier)
  - `animation-delay`: Delay before animation starts
  - `animation-iteration-count`: Number of iterations (number or infinite)
  - `animation-direction`: normal, reverse, alternate, alternate-reverse
  - `animation-fill-mode`: none, forwards, backwards, both
  - `animation-play-state`: running, paused
  - `animation` shorthand property parsing
- **AnimationController**: Centralized animation management
  - RegisterKeyframes: Register @keyframes rules
  - StartAnimation: Start animation on render object
  - StopAnimation/StopAllAnimations: Stop animations
  - PauseAnimation/ResumeAnimation: Pause/resume control
  - Update: Update all running animations
  - GetCurrentProperties: Get interpolated properties
  - State tracking (IDLE, DELAYED, RUNNING, PAUSED, FINISHED)
  - Iteration counting and direction handling
  - Fill mode support (forwards, backwards, both)
- **Property Interpolation**: Advanced value interpolation
  - Number interpolation with units (px, %, em, rem, etc.)
  - Color interpolation (#RGB, #RRGGBB, rgb(), rgba(), named colors)
  - Transform interpolation (placeholder for future)
  - Property map interpolation
  - Type detection and validation
- **Animation Events**: CSS animation event support
  - AnimationEvent class (animation_name, elapsed_time, pseudo_element)
  - animationstart: Fired when animation starts (after delay)
  - animationend: Fired when animation completes
  - animationiteration: Fired on each iteration (except last)
  - Event bubbling support
  - Integration with DOM event system
- **Window Integration**: AnimationController integrated into Window class
  - AnimationController instance in Window
  - Automatic animation updates in render loop
  - GetAnimationController() accessor method
  - Fixed AnimationState naming conflict (renamed to CSSAnimationState)
  - Fixed AnimationDirection enum conflict with Windows macros (ANIM_* prefix)
- New animation classes:
  - `KeyframesRule`: @keyframes rule parsing and storage
  - `Keyframe`: Individual keyframe with offset and properties
  - `KeyframesManager`: Global keyframe registry
  - `CSSAnimation`: Animation property parsing and storage
  - `AnimationController`: Animation lifecycle management
  - `RunningAnimation`: Running animation state tracking
  - `PropertyInterpolation`: Property value interpolation
- Comprehensive test suite:
  - 20 tests for @keyframes parsing
  - 24 tests for animation property parsing
  - 15 tests for animation controller
  - 24 tests for property interpolation
  - Performance tests (10000 interpolations in ~2-5s)
- Integration with rendering pipeline:
  - PropertyInterpolation integrated into AnimationController
  - ComputeCurrentFrame uses interpolation instead of step function
- Documentation:
  - Inline code documentation with detailed comments

### Added - Phase 3: CSS Transition (2025-11-14)
- **CSS Transition**: Complete support for CSS transitions
  - `transition` shorthand property parsing
  - `transition-property`: Specify properties to animate
  - `transition-duration`: Animation duration in seconds or milliseconds
  - `transition-timing-function`: Easing functions (linear, ease, ease-in, ease-out, ease-in-out)
  - `transition-delay`: Delay before animation starts
  - Custom cubic-bezier curves with Newton's method solver
- **Animation Timeline**: Centralized animation management system
  - State tracking (IDLE, DELAYED, RUNNING, FINISHED)
  - Multiple concurrent transitions per object
  - Automatic cleanup of finished animations
- **Property Interpolation**: Support for multiple value types
  - Float values (opacity, width, height, etc.)
  - Color values (SkColor with RGBA interpolation)
  - Transform values (CSSTransform interpolation)
- **Easing Functions**: Complete set of timing functions
  - Linear, Ease, EaseIn, EaseOut, EaseInOut
  - Custom cubic-bezier with configurable control points
  - High-precision curve evaluation (epsilon 1e-6, max 8 iterations)
- New animation classes:
  - `CSSTransition`: Transition property parsing and storage
  - `AnimationTimeline`: Timeline management and state tracking
  - `EasingFunctions`: Timing function implementations
  - `CubicBezier`: Bezier curve evaluation
- Comprehensive test suite:
  - 26 tests for transition parsing and easing functions
  - 13 tests for animation timeline and interpolation
  - Performance tests (1000 transitions parsed in ~20-30ms)
- Integration with rendering pipeline:
  - Window class integration with AnimationTimeline
  - Automatic animation updates in render loop
  - Style resolver integration for transition property
- Documentation:
  - `CSS_TRANSITION_API.md`: Complete API reference
  - `CSS_TRANSITION_GUIDE.md`: Usage guide with examples
  - `examples/css_transition.html`: Interactive examples

### Added - Phase 2: CSS Transform (2025-11-14)
- **CSS Transform**: Complete support for 2D transformations
  - `translate(x, y)`: Translation transformations
  - `rotate(angle)`: Rotation transformations
  - `scale(x, y)`: Scaling transformations
  - `skew(x, y)`: Skew transformations
  - `matrix(a,b,c,d,e,f)`: Matrix transformations
  - Support for multiple transform functions
  - `transform-origin` support with keywords, percentages, and pixels
- New transform classes:
  - `CSSTransform`: Transform parsing and matrix conversion
  - `TransformOrigin`: Transform origin handling
- Comprehensive test suite:
  - 19 tests for transform parsing and rendering
  - Performance tests (1000 parses in ~30-50ms, 10000 matrix conversions in ~10ms)
- Updated `CSSValue` with public utility methods (`Trim`, `Split`)

### Added - Phase 1: Shadows and Gradients (2025-11-14)
- **CSS Box Shadow**: Complete support for box shadows including outset and inset shadows
  - Single and multiple shadows
  - Blur radius and spread radius
  - Support for rounded corners
  - Hardware-accelerated rendering with Skia
- **CSS Text Shadow**: Complete support for text shadows
  - Single and multiple text shadows
  - Blur effects
  - Correct rendering order
- **CSS Linear Gradient**: Complete support for linear gradients
  - Angle-based gradients (e.g., `45deg`)
  - Direction keywords (`to top`, `to right`, `to bottom`, `to left`)
  - Multiple color stops
  - Color stop positions
- **CSS Radial Gradient**: Complete support for radial gradients
  - Circle and ellipse shapes
  - Multiple color stops
  - Color stop positions
- New renderer classes:
  - `ShadowRenderer`: Renders box and text shadows
  - `GradientRenderer`: Renders linear and radial gradients
- Comprehensive test suite:
  - 12 tests for box shadow rendering
  - 12 tests for text shadow
  - 15 tests for gradient rendering
  - 11 integration tests for CSS parsing and rendering
- Documentation:
  - API documentation for shadows and gradients
  - User guide with examples and best practices
  - HTML example page demonstrating all features

### Changed
- Updated `ComputedStyle` to include shadow, gradient, and transform properties
- Enhanced `StyleResolver` to parse `box-shadow`, `text-shadow`, `background-image` (gradients), `transform`, and `transform-origin`
- Modified `RenderObject` and `RenderInlineBlock` to render shadows, gradients, and transforms
- Made `CSSValue::Trim()` and `CSSValue::Split()` public for broader use

### Performance
- **Transform**:
  - Parse 1000 transforms: ~30-50ms
  - Convert 10000 matrices: ~10ms
- **Box Shadow**: ~100 shadows/991ms
- **Text Shadow**: 100 renders with 5 shadows in 49ms
- **Linear Gradient**: 263 gradients/second
- Radial Gradient: 203 gradients/second
- Style Resolution: 1000 resolutions in 170ms

## [0.5.0-alpha] - 2025-11-14

### Added
- Initial alpha release
- Basic rendering engine with Skia
- Preact integration
- Yoga layout engine
- Lexbor HTML/CSS parsing

[Unreleased]: https://github.com/yourusername/mbink/compare/v0.5.0-alpha...HEAD
[0.5.0-alpha]: https://github.com/yourusername/mbink/releases/tag/v0.5.0-alpha

