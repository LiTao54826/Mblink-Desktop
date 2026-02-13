# Changelog

All notable changes to MBink will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- 测试重建（单元测试、集成测试、渲染测试）
- macOS 和 Linux 跨平台支持
- Rust、Go、Node.js 语言绑定
- v1.0 正式发布

---

## [0.92.0] - 2026-02-13

### Added - 生产就绪特性
- **Compositor 图层合成系统** - 完整的图层合成架构（参考 Chromium Blink）
  - PropertyTree 属性树系统（Transform/Clip/Effect/ScrollTree）
  - LayerTreeBuilder 图层树构建器
  - LayerTreeManager 图层树管理器
  - Rasterizer 光栅化器
  - 增量更新支持
  - 动画图层桥接
  
- **DevTools 开发者工具** - 完整的 F12 调试面板
  - 元素检查器（DOM 树导航、元素拾取）
  - 样式面板（内联样式、计算样式、Box Model）
  - 样式编辑器（实时编辑）
  - 属性编辑器
  - 元素搜索
  - DOM 序列化
  - 快捷键支持（F12、Ctrl+Shift+C）
  
- **Network 网络模块** - 完整的网络请求支持
  - HTTPClient HTTP 客户端
  - Fetch API JavaScript 绑定
  - Promise 异步处理
  - GET/POST/PUT/DELETE 方法支持
  - JSON 响应解析
  - Windows WinHTTP 支持
  
- **Fluent Design 组件库** - 基于 Microsoft Fluent UI
  - 完整主题系统（颜色、圆角、阴影、间距、字体）
  - 15+ 组件实现
    - 按钮组件：Button, CompoundButton, ToggleButton
    - 输入组件：Input, SearchBox, Textarea, Select, Checkbox, Switch, Radio
    - 数据展示：Text, Badge, Avatar, Card, Divider 等
    - 反馈组件：Spinner, LoadingDots
    - 导航组件：Link
  - 工具函数（mergeStyles, classNames, useId, debounce, throttle）
  - 轻量级纯 JavaScript 实现

### Changed
- 项目进度从 91% 提升到 95%
- Python 绑定完成度从 20% 提升到 85%
- Preact 生态从 90% 完成到 100% 完成
- 文档完成度从 80% 提升到 90%

### Improved
- 完善了多个示例应用（fetch_demo, fluent_demo, terminal_logview_demo）
- 优化了图层合成性能
- 改进了开发者体验（DevTools）

---

## [0.91.0] - 2025-12-16

### Added - 原生布局引擎
- **Native Layout Engine** - 完全原生的 C++ 布局引擎
  - Block 布局模式
  - IFC (Inline Formatting Context) 行内格式化上下文
  - text-align 支持（left/center/right/justify）
  - vertical-align 支持（top/middle/bottom/baseline）
  - inline-block 元素支持
  - 换行算法（CJK 字符、连字符断行）
  
- **布局测试套件**
  - IFC 单元测试：32 个（100% 通过）
  - 基础布局比较：121 个（100% 通过）
  - 高级布局比较：159 个（100% 通过）
  - 性能测试：8 个（全部达标）

### Changed
- 项目整理完成，清理历史测试
- 创建 LEGACY_TEST_LIST.md 记录历史测试清单

---

## [0.90.0] - 2025-11-28

### Added - Taffy 布局引擎
- **Taffy CSS 布局引擎** - 替代 Yoga
  - 完整 Flexbox 支持
  - CSS Grid 布局支持
  - grid-template-columns/rows 模板
  - Position (relative, absolute, fixed)
  - Overflow 处理
  - 滚动条支持
  
- **渲染优化**
  - D3D11 DisplayBackend（无闪烁 CPU 渲染）
  - DPI 缩放检测
  - font-family 解析支持
  
- **Flexbox 修复**
  - 启动黑屏问题修复
  - resize 性能优化
  - weak_ptr 正确跟踪渲染树

---

## [0.5.0-alpha] - 2025-11-15

### Added - HTML/CSS 完整支持
- **HTML5 解析增强**
  - 错误处理和警告系统
  - 文档模式检测（quirks/standards）
  - DOCTYPE 处理（HTML5, HTML4, XHTML）
  - HTML 实体解析（200+ 实体）
  - 特殊元素处理（script, style, template, SVG）
  
- **CSS3 选择器支持**
  - 所有基本选择器
  - 所有组合选择器
  - 所有属性选择器（7 种变体）
  - 所有结构伪类（14 种）
  - 所有表单伪类（3 种）
  - 所有动态伪类
  - 所有伪元素
  
- **表单元素支持**
  - 所有 HTML5 input 类型
  - 完整表单验证
  - 表单状态管理

### Tests
- 283 单元测试 + 50 性能测试 = 333 个测试（98% 通过）

---

## [0.3.0-alpha] - 2025-11-14

### Added - CSS 高级特性
- **CSS Shadows & Gradients**
  - Box Shadow（内外阴影、模糊、扩展）
  - Text Shadow（多重阴影）
  - Linear Gradient（角度、方向、多色）
  - Radial Gradient（圆形、椭圆形）
  
- **CSS Transform**
  - translate, rotate, scale, skew, matrix
  - transform-origin 支持
  
- **CSS Transition**
  - 12 种缓动函数
  - 动画时间线管理
  
- **CSS Animation**
  - @keyframes 规则解析
  - 动画控制器
  - 属性插值系统
  
- **CSS Variables & Filters**
  - CSS Custom Properties (--var)
  - var() 函数
  - 10 种 CSS Filters

### Tests
- 357 个测试全部通过

---

## [0.1.0] - 2025-11-08

### Added - 基础架构
- CMake 构建系统
- SDL3 集成（窗口、事件）
- Skia 集成（渲染引擎）
- QuickJS 集成（JavaScript 引擎）
- Yoga 集成（布局引擎）
- Lexbor 集成（HTML/CSS 解析）
- 基础 DOM API
- 事件系统
- 渲染管线

---

[Unreleased]: https://github.com/yourusername/mbink/compare/v0.92.0...HEAD
[0.92.0]: https://github.com/yourusername/mbink/compare/v0.91.0...v0.92.0
[0.91.0]: https://github.com/yourusername/mbink/compare/v0.90.0...v0.91.0
[0.90.0]: https://github.com/yourusername/mbink/compare/v0.5.0-alpha...v0.90.0
[0.5.0-alpha]: https://github.com/yourusername/mbink/compare/v0.3.0-alpha...v0.5.0-alpha
[0.3.0-alpha]: https://github.com/yourusername/mbink/compare/v0.1.0...v0.3.0-alpha
[0.1.0]: https://github.com/yourusername/mbink/releases/tag/v0.1.0

