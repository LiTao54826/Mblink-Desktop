# LightUI 已知问题

本文档记录框架中已知但尚未修复的问题，供开发者参考和后续修复。

---

## 1. ~~CSS 类名变化后增量渲染不生效~~ [已修复]

**状态**：✅ 已修复

**修复日期**：2026-01-12

**修复版本**：commit 44df44f 之后

**问题描述**：
通过原生 JS 修改元素的 `className` 属性后，如果 CSS 类对应的样式包含 `display` 等布局属性变化，增量渲染系统不会正确更新 UI。

**修复说明**：
在 `WindowDOMObserver::OnAttributeChanged` 中添加了 display 属性变化检测逻辑：
1. 在处理 `class` 属性变化前，保存旧的 `display` 值
2. 重新解析样式后，比较新旧 `display` 值
3. 如果可见性发生变化（`none` <-> 非 `none`），调用 `InvalidateRenderTree()` 重建渲染树

**修复文件**：
- `core/window/window_dom_observer.cpp` - `OnAttributeChanged` 方法

**测试用例**：
- `tests/js/test_class_switch.js`

<details>
<summary>原问题详情（已归档）</summary>

**复现步骤**：
```javascript
// 切换页面显示（修复前不生效）
element.className = 'page';        // display: none
element.className = 'page active'; // display: block
```

**影响范围**：
- 原生 JS DOM 操作
- 依赖 CSS 类切换的 UI 交互（如标签页、侧边栏导航等）

**根本原因**：
`WindowDOMObserver::OnAttributeChanged` 在处理 `class` 属性变化时，没有正确触发样式重新计算和渲染树更新。当 CSS 类变化导致 `display` 属性从 `none` 变为 `block`（或反之）时，需要重建对应元素的 RenderObject。

</details>

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
**padding 计算不一致问题**：

在 `RenderObject::GetMaxScrollY()` 中计算 `visible_height` 时，只减去了 border，没有减去 padding：
```cpp
float visible_height = effective_height - border_top - border_bottom;
// 缺少：- padding_top - padding_bottom
```

而 `CalculateContentHeight()` 计算的子元素位置是相对于内容区域的（已考虑 padding-top），最后只添加了 `padding_bottom`。

这导致 `max_scroll = content_height - visible_height` 计算结果偏小，无法滚动到真正的底部。

**示例**：
- 容器高度 = 200px, padding = 10px (上下), border = 0
- 当前计算：`visible_height` = 200px, `content_height` = 310px, `max_scroll` = 110px
- 正确计算：`visible_height` = 180px, `content_height` = 310px, `max_scroll` = 130px

**修复方向**：
在 `GetMaxScrollY()` 中，`visible_height` 应该减去 padding：
```cpp
float padding_top = style.padding.top.ToPx();
float padding_bottom = style.padding.bottom.ToPx();
float visible_height = effective_height - border_top - border_bottom - padding_top - padding_bottom;
```

**相关文件**：
- `core/render/objects/render_object.cpp` - `GetMaxScrollY()`, `CalculateContentHeight()`
- `core/compositor/scroll_layer_manager.cpp` - `UpdateContentSize()`, `CalculateScrollBounds()`
- `core/event/dispatch/wheel_event_dispatcher.cpp` - 滚轮事件处理

**优先级**：低

**记录日期**：2026-01-12

**分析日期**：2026-01-12

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
