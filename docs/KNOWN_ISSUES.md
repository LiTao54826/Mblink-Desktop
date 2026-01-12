# LightUI 已知问题

本文档记录框架中已知但尚未修复的问题，供开发者参考和后续修复。

---

## 1. CSS 类名变化后增量渲染不生效

**问题描述**：
通过原生 JS 修改元素的 `className` 属性后，如果 CSS 类对应的样式包含 `display` 等布局属性变化，增量渲染系统不会正确更新 UI。

**复现步骤**：
```javascript
// 切换页面显示（不生效）
element.className = 'page';        // display: none
element.className = 'page active'; // display: block
```

**影响范围**：
- 原生 JS DOM 操作
- 依赖 CSS 类切换的 UI 交互（如标签页、侧边栏导航等）

**不受影响**：
- Preact/React 等虚拟 DOM 框架（每次 render 会重新计算）
- 直接修改 `element.style.display` 等内联样式

**临时解决方案**：
使用内联样式而不是 CSS 类切换：
```javascript
// 推荐方式（生效）
element.style.display = 'none';
element.style.display = 'block';

// 或者直接设置多个样式属性
element.style.background = isActive ? '#e94560' : 'transparent';
element.style.color = isActive ? '#fff' : '#888';
```

**根本原因**：
`WindowDOMObserver::OnAttributeChanged` 在处理 `class` 属性变化时，没有正确触发样式重新计算和渲染树更新。当 CSS 类变化导致 `display` 属性从 `none` 变为 `block`（或反之）时，需要重建对应元素的 RenderObject。

**修复方向**：
1. 在 `OnAttributeChanged` 中检测 `class` 属性变化
2. 重新解析元素的 CSS 样式
3. 比较新旧样式的 `display` 属性
4. 如果 `display` 变化涉及 `none`，调用 `InvalidateRenderTree()` 重建渲染树

**相关文件**：
- `core/window/window_dom_observer.cpp` - `OnAttributeChanged` 方法
- `core/render/css/style_resolver.cpp` - 样式解析
- `core/dom/element.cpp` - `SetAttribute` 方法

**优先级**：中

**记录日期**：2026-01-12

---

## 2. 鼠标滚轮无法滚动到容器最底部

**问题描述**：
在带有 `overflow-y: auto` 的可滚动容器中，使用鼠标滚轮滚动时无法滚动到内容的最底部，但拖动滚动条可以正常到达底部。

**复现步骤**：
1. 运行 `python bindings/python/examples/test_timers.py`
2. 点击按钮多次，让日志区域产生足够多的内容
3. 使用鼠标滚轮向下滚动日志区域
4. 观察：滚轮滚动无法到达最底部，但拖动滚动条可以

**复现示例**：
- `bindings/python/examples/test_timers.py` - 日志区域 `.log`

**影响范围**：
- 所有使用 `overflow-y: auto` 或 `overflow-y: scroll` 的可滚动容器
- 鼠标滚轮滚动

**不受影响**：
- 拖动滚动条滚动
- 滚动条位置计算（显示正确）

**临时解决方案**：
使用拖动滚动条代替鼠标滚轮滚动到底部。

**根本原因**：
待分析。可能是滚轮事件处理中的滚动距离计算或边界检测问题。

**修复方向**：
1. 检查 `core/event/` 中的滚轮事件处理逻辑
2. 检查滚动距离计算是否有边界限制问题
3. 对比拖动滚动条和滚轮滚动的代码路径差异

**相关文件**：
- `core/event/` - 事件处理
- `core/layout/` - 滚动计算
- `core/render/` - 滚动条渲染

**优先级**：低

**记录日期**：2026-01-12

---

## 问题模板

```markdown
## N. 问题标题

**问题描述**：
简要描述问题现象。

**复现步骤**：
1. 步骤一
2. 步骤二

**影响范围**：
- 受影响的功能

**临时解决方案**：
如果有的话。

**根本原因**：
分析问题的根本原因。

**修复方向**：
建议的修复方案。

**相关文件**：
- 相关源文件列表

**优先级**：高/中/低

**记录日期**：YYYY-MM-DD
```
