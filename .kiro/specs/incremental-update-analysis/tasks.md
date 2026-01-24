# Tasks Document: 增量更新系统分析与修复

## Task 1: 修复 CSS 类切换 display 属性不触发渲染树更新的 Bug

**Status**: done

**Requirements**: 2.4, 7.2

**Description**: 
在 `WindowDOMObserver::OnAttributeChanged` 中添加 display 属性变化检测逻辑。当 `class` 属性变化导致 `display` 属性从 `none` 变为其他值（或反之）时，触发渲染树重建。

**Implementation Steps**:
1. 在处理 `class` 属性前，保存旧的 display 值
2. 重新解析样式后，比较新旧 display 值
3. 如果可见性发生变化，调用 `InvalidateRenderTree()`

**Files to Modify**:
- `core/window/window_dom_observer.cpp`

**Acceptance Criteria**:
- [ ] `element.className = 'xxx'` 切换 CSS 类时，display 属性变化能正确触发 UI 更新
- [ ] 编译通过
- [ ] 测试通过

---

## Task 2: 创建 CSS 类切换测试用例

**Status**: done

**Requirements**: 2.4, 7.2

**Description**: 
创建 JavaScript 测试脚本验证 CSS 类切换功能，特别是涉及 display 属性变化的场景。

**Implementation Steps**:
1. 创建测试文件 `tests/js/test_class_switch.js`
2. 测试 className 切换导致 display 变化的场景
3. 测试 classList.add/remove 操作

**Files to Create**:
- `tests/js/test_class_switch.js`

**Acceptance Criteria**:
- [x] 测试覆盖 className 切换场景
- [x] 测试覆盖 classList 操作场景
- [x] 所有测试通过

---

## Task 3: 更新 KNOWN_ISSUES.md 文档

**Status**: done

**Requirements**: 7.1

**Description**: 
更新已知问题文档，标记 Bug 1 已修复，并添加详细的修复说明。

**Files to Modify**:
- `docs/KNOWN_ISSUES.md`

**Acceptance Criteria**:
- [x] Bug 1 状态更新为已修复
- [x] 添加修复版本和日期
- [x] 添加修复说明

---

## Task 4: 分析鼠标滚轮滚动问题（可选）

**Status**: done

**Requirements**: 7.3

**Description**: 
分析鼠标滚轮无法滚动到容器最底部的问题，定位根本原因。

**Implementation Steps**:
1. 分析 `core/event/` 中的滚轮事件处理代码
2. 检查滚动边界计算逻辑
3. 对比滚轮滚动和滚动条拖动的代码路径差异

**Files Analyzed**:
- `core/event/dispatch/wheel_event_dispatcher.cpp`
- `core/render/objects/render_object.cpp` (GetMaxScrollY, CalculateContentHeight)
- `core/compositor/scroll_layer_manager.cpp`
- `core/compositor/layer_tree_manager.cpp`

**Analysis Result**:

### 排除的方向
1. ✅ 滚轮和滚动条拖动使用相同的 `RenderPipeline::HandleScroll()` 方法
2. ✅ 两者都会调用 `UpdateContentSize()` 更新内容尺寸
3. ✅ `CalculateContentHeight()` 在缓存为 0 时会被动态调用

### 可能的根本原因
**padding 计算不一致问题**：

在 `GetMaxScrollY()` 中：
- `visible_height = effective_height - border_top - border_bottom`
- 这里 `visible_height` 包括了 padding，但没有减去 padding

在 `CalculateContentHeight()` 中：
- 子元素的 `layout_info_.y` 是相对于内容区域的（已考虑 padding-top）
- 最后只添加了 `padding_bottom`

**示例**：
- 容器高度 = 200px, padding-top = 10px, padding-bottom = 10px, border = 0
- `visible_height` = 200px（包括 padding）
- 子元素高度 = 300px，`content_height` = 300 + 10 = 310px
- `max_scroll` = 310 - 200 = 110px
- **实际应该**：内容区域 = 180px，`max_scroll` = 300 - 180 = 120px

### 修复方向
在 `GetMaxScrollY()` 中，`visible_height` 应该减去 padding：
```cpp
float padding_top = style.padding.top.ToPx();
float padding_bottom = style.padding.bottom.ToPx();
float visible_height = effective_height - border_top - border_bottom - padding_top - padding_bottom;
```

或者在 `CalculateContentHeight()` 中添加 `padding_top`。

**Acceptance Criteria**:
- [x] 定位问题根本原因
- [x] 记录分析结果到设计文档

