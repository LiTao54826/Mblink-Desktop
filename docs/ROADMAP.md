# MBink 开发路线图

> **最后更新**: 2025-11-29
> **当前版本**: 0.90.0
> **当前进度**: 85%
> **构建状态**: ✅ 核心模块编译成功
> **测试状态**: ✅ 81 个测试用例通过 (4个 Preact 测试待修复)
> **当前阶段**: Phase 9 进行中 - Preact C++ 绑定待完成 🔄

## 总体时间规划

**总计**: 8-10个月完成核心功能
**目标**: 2026年Q2发布v1.0
**当前状态**: 核心功能已完成，进入稳定化和文档完善阶段
**项目定位**: 轻量级跨平台桌面应用框架 - Electron的轻量级替代品

---

## ✅ 已完成阶段

### Phase 1: 基础架构 (100%) ✅

**完成时间**: 2025-11-08

#### 已完成任务

- [x] 创建Git仓库，设置分支策略
- [x] 配置CMake构建系统
- [x] 集成SDL3库（6.5 MB）
- [x] 集成Skia库（36.5 MB）
- [x] 集成QuickJS库（1.1 MB）
- [x] 集成Yoga布局引擎（2.1 MB）
- [x] 集成nlohmann/json库
- [x] 创建基础窗口示例
- [x] 编写构建文档

#### 交付物

- ✅ 可编译的基础项目
- ✅ 9个核心模块编译成功（~558 KB）
- ✅ 完整的构建文档

---

### Phase 2.1: JavaScript Runtime (100%) ✅

**完成时间**: 2025-11-09

#### 已完成任务

- [x] 编译QuickJS为静态库
- [x] 创建QuickJS包装类
- [x] 实现JS代码执行（Eval, EvalFile）
- [x] 实现JS <-> C++数据转换（JSValue ↔ JSON）
- [x] 实现Console API（log, error, warn, info）
- [x] 实现原生函数绑定（RegisterFunction）
- [x] 实现JavaScript函数调用（CallFunction）
- [x] 实现全局属性管理（SetGlobalProperty, GetGlobalProperty）
- [x] 实现模块加载系统（ES6 import/export）
- [x] 实现异步任务队列（setTimeout, setInterval, Promise）
- [x] 编写14个单元测试（100%通过）

#### 交付物

- ✅ QuickJS运行时封装（core/quickjs/quickjs_runtime.h）
- ✅ 能执行JavaScript代码
- ✅ 完整的Console API支持
- ✅ 异步编程支持（Promise, setTimeout, setInterval）
- ✅ ES6模块系统支持
- ✅ 14个测试全部通过

#### 详细文档

- [PHASE_2_1_COMPLETION_REPORT.md](../PHASE_2_1_COMPLETION_REPORT.md)
- [QUICK_START_PHASE_2_1.md](../QUICK_START_PHASE_2_1.md)

---

### Phase 2.2: DOM API (100%) ✅

**完成时间**: 2025-11-09

#### 已完成任务

- [x] 设计DOM节点类层次结构（Node, Element, Text, Document）
- [x] 实现Node基类（节点关系、子节点管理、遍历、克隆）
- [x] 实现Element类（属性、样式、查询、innerHTML、事件）
- [x] 实现Text类（文本数据管理）
- [x] 实现Document类（工厂方法、查询、ID映射）
- [x] 实现事件系统（Event, MouseEvent, KeyboardEvent）
- [x] 实现事件传播（冒泡、捕获、preventDefault）
- [x] 实现CSS选择器（QuerySelector, QuerySelectorAll, Matches, Closest）
- [x] 实现QuickJS绑定（所有核心API）
- [x] 实现性能优化（ID缓存、Hash Map、脏标记）
- [x] 编写102个单元测试（100%通过）
- [x] 编写17个性能基准测试
- [x] 编写完整文档（API文档、性能文档、示例代码）

#### 交付物

- ✅ 完整的DOM API实现（遵循W3C标准）
- ✅ 事件系统（事件冒泡、捕获、preventDefault）
- ✅ CSS选择器支持
- ✅ QuickJS绑定（JavaScript可直接操作DOM）
- ✅ 高性能优化（生产级别性能）
- ✅ 102个测试全部通过
- ✅ 完整文档和示例

#### 性能指标

- 节点创建: 3-4M ops/sec
- GetElementById: 146ns/op（极快）
- 事件系统: 1-6M ops/sec
- 所有操作达到生产级别性能

#### 详细文档

- [PHASE_2_2_SESSION_5_REPORT.md](../PHASE_2_2_SESSION_5_REPORT.md)
- [docs/DOM_API.md](DOM_API.md)
- [docs/PERFORMANCE.md](PERFORMANCE.md)
- [examples/dom_example.js](../examples/dom_example.js)
- [examples/dom_example.cpp](../examples/dom_example.cpp)

---

### Phase 2.3: 渲染引擎 (100%) ✅

**完成时间**: 2025-11-10

#### 已完成任务

- [x] 启用 Skia 渲染引擎
- [x] 实现完整的渲染管线
- [x] 实现 CSS 样式渲染
- [x] 实现文本渲染系统
- [x] 实现图像加载和缓存
- [x] 实现渲染优化（脏区域、层级、缓存）
- [x] 修复构建配置（运行时库匹配）
- [x] 编写并通过所有测试（83+ 个测试）

#### 交付物

- ✅ Skia 渲染引擎完全集成
- ✅ 完整的 CSS 渲染支持
- ✅ 文本和图像渲染
- ✅ 渲染优化系统
- ✅ 83+ 个测试全部通过
- ✅ 构建系统稳定

#### 详细文档

- [BUILD_AND_TEST_REPORT.md](../BUILD_AND_TEST_REPORT.md)
- [PHASE_2_3_COMPLETION_SUMMARY.md](../PHASE_2_3_COMPLETION_SUMMARY.md)

---

### Phase 2.4: 窗口和事件系统 (100%) ✅

**完成时间**: 2025-11-10

#### 已完成任务

- [x] 完善 SDL3 窗口集成
- [x] 实现完整的事件循环 (60 FPS)
- [x] 实现键盘和鼠标事件
- [x] 实现窗口生命周期管理
- [x] 编写集成测试

#### 交付物

- ✅ SDL3 窗口管理完整实现
- ✅ 60 FPS 主循环
- ✅ 定时器调度系统
- ✅ 任务队列

---

### Phase 2.5: 高级事件系统 (100%) ✅

**完成时间**: 2025-11-12

#### 已完成任务

- [x] 实现 mouseenter/mouseleave 事件
- [x] 实现 mouseover/mouseout 事件和 hover 链追踪
- [x] 实现 removeEventListener 和事件捕获阶段
- [x] 实现 FocusManager 焦点管理系统
- [x] 实现 DragManager 拖拽管理系统
- [x] 实现 DataTransfer 拖拽数据传输
- [x] 实现 KeyboardEvent 和键盘事件处理
- [x] 实现 classList API (DOMTokenList)
- [x] 实现 CSSStyleDeclaration 和 style API
- [x] 实现 cloneNode 和 dataset API
- [x] 实现 dblclick 事件和 addEventListener once 选项
- [x] 完整集成 Lexbor CSS 选择器引擎

#### 交付物

- ✅ 完整的事件系统 (10+ 种事件类型)
- ✅ 焦点管理系统
- ✅ 拖拽系统
- ✅ DOM API 扩展

---

### Phase 2.6: Lexbor 集成 (100%) ✅

**完成时间**: 2025-11-13

#### 已完成任务

- [x] 实现 innerHTML/outerHTML 解析和序列化
- [x] 实现 HTMLInputElement 和 HTMLTextAreaElement
- [x] 实现表单验证和状态管理
- [x] focusin/focusout 事件集成
- [x] 文本输入支持

#### 交付物

- ✅ innerHTML/outerHTML 完整支持
- ✅ 表单元素实现
- ✅ Lexbor HTML5 解析器集成

---

### Phase 3: CSS 阴影和渐变 (100%) ✅

**完成时间**: 2025-11-14

#### 已完成任务

- [x] CSS Box Shadow (内外阴影、模糊、扩展)
- [x] CSS Text Shadow (多重阴影)
- [x] CSS Linear Gradient (角度、方向、多色)
- [x] CSS Radial Gradient (圆形、椭圆形)
- [x] 50 个测试全部通过

#### 交付物

- ✅ ShadowRenderer 阴影渲染器
- ✅ GradientRenderer 渐变渲染器
- ✅ 完整的 CSS 阴影和渐变支持

---

### Phase 4: CSS Transform/Transition/Animation (100%) ✅

**完成时间**: 2025-11-14

#### 已完成任务

- [x] CSS Transform (translate, rotate, scale, skew, matrix)
- [x] transform-origin 支持
- [x] CSS Transition 属性
- [x] 12 种缓动函数 (ease, linear, cubic-bezier 等)
- [x] @keyframes 规则解析
- [x] CSS Animation 属性
- [x] AnimationController (启动、停止、暂停、恢复)
- [x] 属性插值系统
- [x] 动画事件 (animationstart, animationend, animationiteration)

#### 交付物

- ✅ CSSTransform 变换系统
- ✅ AnimationTimeline 动画时间线
- ✅ AnimationController 动画控制器
- ✅ PropertyInterpolation 属性插值
- ✅ 150+ 个测试全部通过

---

### Phase 5: CSS 变量和滤镜 (100%) ✅

**完成时间**: 2025-11-14

#### 已完成任务

- [x] CSS Custom Properties (--var)
- [x] var() 函数解析和求值
- [x] 10 种 CSS Filters (blur, brightness, contrast, grayscale, etc.)
- [x] 滤镜链支持
- [x] 性能优化系统集成

#### 交付物

- ✅ CSSVariables 变量系统
- ✅ CSSFilters 滤镜系统
- ✅ 114 个测试全部通过

---

### Phase 6: 性能优化系统 (100%) ✅

**完成时间**: 2025-11-15

#### 已完成任务

- [x] 关键帧插值缓存 (KeyframeInterpolationCache)
- [x] 动画脏标记系统 (AnimationDirtyTracker)
- [x] 批量动画更新器 (BatchAnimationUpdater)
- [x] CSS 滤镜缓存 (FilterCache)
- [x] 变换矩阵缓存 (TransformMatrixCache)
- [x] 泛型对象池 (ObjectPool<T>)
- [x] 增量渲染系统
- [x] 脏区域收集器

#### 交付物

- ✅ animation_optimizer 动画优化器
- ✅ filter_cache 渲染优化器
- ✅ object_pool 对象池
- ✅ 44 个测试全部通过
- ✅ 性能提升 1.5-2x

---

### Phase 7: HTML/CSS 完整支持 (100%) ✅

**完成时间**: 2025-11-15 (v0.90.0)

#### 已完成任务

- [x] 完整的 HTML5 解析增强
  - 错误处理和警告系统
  - 文档模式检测 (quirks/standards)
  - DOCTYPE 处理 (HTML5, HTML4, XHTML)
  - HTML 实体解析 (200+ 实体)
  - 特殊元素处理 (script, style, template, SVG)
  - 健壮的错误恢复机制
- [x] 完整的 CSS3 选择器支持
  - 所有基本选择器 (type, class, ID, universal)
  - 所有组合选择器 (descendant, child, sibling)
  - 所有属性选择器 (7 种变体)
  - 所有结构伪类 (14 种)
  - 所有表单伪类 (3 种)
  - 所有动态伪类 (hover, active, focus 等)
  - 所有伪元素 (::before, ::after 等)
- [x] 增强表单元素支持
  - 所有 HTML5 input 类型
  - 完整表单验证
  - 表单状态管理

#### 交付物

- ✅ 283 个单元测试 (100% 通过)
- ✅ 50 个性能测试 (84% 通过)
- ✅ 总计 333 个测试 (98% 通过)

---

### Phase 8: Taffy CSS 布局引擎 (100%) ✅

**完成时间**: 2025-11-28

#### 已完成任务

- [x] 集成 Taffy CSS 布局引擎 (替代 Yoga)
- [x] 完整 Flexbox 布局支持
- [x] CSS Grid 布局支持
  - grid-template-columns/rows
  - grid 模板支持
- [x] Position 和 Overflow 支持
- [x] DPI 缩放检测
- [x] font-family 解析支持
- [x] D3D11 DisplayBackend 实现 (无闪烁 CPU 渲染)
- [x] 滚动条支持
- [x] Flexbox 布局修复和 resize 性能优化

#### 交付物

- ✅ Taffy 布局引擎完整集成
- ✅ CSS Grid 支持
- ✅ DisplayBackend 抽象层
- ✅ Flexbox 测试应用

---

## 🚧 进行中阶段

### Phase 9: Preact 生态系统 (50%)

**目标**: 完整支持 Preact 和 React 生态

#### 已完成 (纯 JavaScript 实现)

- [x] Preact 核心库 (`js/preact/preact.js` - 370 行)
- [x] Hooks 系统 (`js/preact/hooks.js` - 255 行)
  - useState, useEffect, useLayoutEffect
  - useRef, useMemo, useCallback
  - useContext, useReducer, createContext
- [x] Virtual DOM (h() / createElement() VNode 创建)
- [x] 函数组件支持 (props 传递)
- [x] 事件绑定 (onclick, onChange, onSubmit)
- [x] DOM 渲染 (createDOMElement → MBink DOM)

#### 可运行示例

- [x] preact_counter - 计数器应用 (useState 演示)
- [x] preact_todo_app - Todo 应用 (完整 CRUD)
- [x] preact_hello_world - 基础示例
- [x] preact_window_demo - 窗口渲染
- [ ] preact_form_demo - 目录为空

#### 测试状态

- [x] PreactBasicTest (8 个测试) - 通过
- [ ] PreactIntegrationTest - 缺少可执行文件
- [ ] PreactRenderTest - 引用不存在的 PreactRenderer
- [ ] PreactComponentsTest - 引用不存在的 PreactBindings

#### 待完成 (C++ 绑定)

- [ ] 实现 `core/quickjs/preact_renderer.h/cpp`
- [ ] 实现 `core/quickjs/preact_bindings.h/cpp`
- [ ] 修复 PreactRenderTest, PreactComponentsTest, PreactIntegrationTest
- [ ] Virtual DOM Diffing 优化 (当前为简单重渲染)
- [ ] Preact Router 集成
- [ ] Ant Design 组件库测试

---

## 📅 后续计划

### Phase 10: 多语言绑定 (20%)

**预计时间**: 2-3 周

#### 任务清单

- [x] C API 基础框架
- [ ] Python 绑定完善
- [ ] Rust 绑定
- [ ] Go 绑定
- [ ] Node.js 绑定

### Phase 11: 工具链和发布

**预计时间**: 2-4 周

#### 任务清单

- [ ] CLI 工具开发
- [ ] 项目模板生成器
- [ ] 打包工具
- [ ] 热重载支持
- [ ] 跨平台测试 (macOS, Linux)
- [ ] v1.0 发布准备

---

## 里程碑

| 里程碑 | 计划时间 | 实际完成 | 描述 |
|--------|----------|---------|------|
| M1: 核心框架 | 第12周 | ✅ 2025-11-10 | 基础框架完成，能运行简单UI |
| M2: CSS 高级特性 | 第16周 | ✅ 2025-11-14 | 阴影、渐变、变换、动画 |
| M3: HTML/CSS 完整 | 第20周 | ✅ 2025-11-15 | 完整 HTML5/CSS3 支持 |
| M4: Taffy 布局 | 第24周 | ✅ 2025-11-28 | CSS Grid + Flexbox |
| M5: Preact 生态 | 第28周 | 🔄 50% | JS 实现完成，C++ 绑定待完成 |
| M6: v1.0发布 | 第32周 | ⏳ 待完成 | 正式发布v1.0 |

---

## 版本历史

| 版本 | 日期 | 主要变更 |
|------|------|---------|
| 0.90.0 | 2025-11-15 | HTML/CSS 完整支持，333 个测试 |
| 0.5.0-alpha | 2025-11-14 | CSS 动画、变换、滤镜 |
| 0.3.0-alpha | 2025-11-11 | 渲染引擎、DOM API |
| 0.1.0 | 2025-11-08 | 基础架构 |

---

## 风险和应对

### 技术风险

1. **性能不达标**
   - 风险: QuickJS性能可能不足
   - 应对: ✅ 已实现性能优化系统，1.5-2x 提升
   - 状态: 已解决

2. **组件库兼容性**
   - 风险: 某些组件库可能无法兼容
   - 应对: 优先支持核心组件，提供替代方案
   - 状态: 进行中

3. **跨平台问题**
   - 风险: 不同平台行为不一致
   - 应对: ✅ SDL3 跨平台，D3D11 无闪烁渲染
   - 状态: Windows 已完成，其他平台待测试

### 资源风险

1. **开发人力不足**
   - 应对: 合理分配任务，寻求社区贡献

2. **时间延期**
   - 应对: 灵活调整优先级，核心功能优先
   - 状态: 核心功能按计划完成

---

## 后续规划 (v2.0+)

- WebGL支持
- 高级动画系统
- 插件系统
- 移动平台支持
- 云端UI编辑器

