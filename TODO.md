# LightUI 开发任务清单

## 🔥 当前阶段：Phase 2.5 - JavaScript 基础设施完善

详细计划请查看：[PHASE_2_5_PLAN.md](PHASE_2_5_PLAN.md)

---

## 📋 本周任务（Week 1: 核心事件系统）

### 🔴 P0: 必须完成

- [ ] **Task 1.1**: 实现 Hit Testing（点击检测）
  - [ ] 创建 `core/event/hit_testing.h` 和 `.cpp`
  - [ ] 实现矩形边界检测
  - [ ] 考虑 z-index 和层叠顺序
  - [ ] 编写单元测试

- [ ] **Task 1.2**: 实现 MouseEvent 类
  - [ ] 创建 `core/event/mouse_event.h` 和 `.cpp`
  - [ ] 继承自 Event 基类
  - [ ] 添加鼠标位置和按钮属性
  - [ ] 实现 preventDefault() 和 stopPropagation()

- [ ] **Task 1.3**: 集成到 EventLoop
  - [ ] 修改 `core/event/event_loop.cpp`
  - [ ] 处理 SDL 鼠标事件
  - [ ] 执行 Hit Testing
  - [ ] 分发 MouseEvent 到目标元素

- [ ] **Task 1.4**: 支持基本鼠标事件
  - [ ] `click` 事件
  - [ ] `mousedown` 事件
  - [ ] `mouseup` 事件
  - [ ] `mousemove` 事件

---

### 🟠 P1: 重要

- [ ] **Task 2.1**: 实现 `element.addEventListener` JavaScript 绑定
  - [ ] 创建 `core/quickjs/event_bindings.h` 和 `.cpp`
  - [ ] 支持 click, mousedown, mouseup, mousemove
  - [ ] 支持 useCapture 参数

- [ ] **Task 2.2**: 实现 `element.removeEventListener`
  - [ ] 根据事件类型和回调移除监听器

- [ ] **Task 2.3**: 事件对象传递到 JavaScript
  - [ ] 将 C++ MouseEvent 转换为 JS 对象
  - [ ] 包含 target, clientX, clientY 等属性

---

### 🟢 P2: 测试和验证

- [ ] **测试 animation_demo**
  - [ ] 让 Start/Stop 按钮可点击
  - [ ] 验证动画启动和停止功能

- [ ] **测试 counter_app**
  - [ ] 让所有按钮可点击
  - [ ] 验证计数器功能

- [ ] **创建 event_demo**
  - [ ] 演示各种鼠标事件
  - [ ] 显示事件信息（位置、按钮等）

---

## 📅 未来任务（Week 2+）

### Week 2: DOM API 完善
- [ ] Task 3: 查询选择器（querySelector, querySelectorAll）
- [ ] Task 4: 元素属性和样式操作（setAttribute, classList）
- [ ] Task 5: DOM 操作 API（appendChild, removeChild）

### Week 3: HTML 元素扩展
- [ ] Task 6: 表单元素（input, textarea, select）
- [ ] Task 7: 其他常用元素（img, a, span, ul/ol/li）

### Week 4+: CSS 和高级功能（可选）
- [ ] Task 8: CSS 选择器和样式表
- [ ] Task 9: CSS 伪类支持（:hover, :active, :focus）
- [ ] Task 10: CSS 动画和过渡
- [ ] Task 11: 键盘事件
- [ ] Task 12: 焦点管理

---

## 🐛 已知问题

### 高优先级
- ⚠️ 按钮显示但无法点击（Phase 2.5 Task 1-2 解决）
- ⚠️ 缺少 querySelector 等 DOM API（Phase 2.5 Task 3 解决）

### 中优先级
- ⚠️ 缺少表单元素（Phase 2.5 Task 6 解决）
- ⚠️ 缺少 CSS 选择器支持（Phase 2.5 Task 8 解决）

### 低优先级
- ⚠️ 缺少键盘事件（Phase 2.5 Task 11 解决）
- ⚠️ 缺少焦点管理（Phase 2.5 Task 12 解决）

---

## ✅ 最近完成

### 2025-11-11
- ✅ 修复 requestAnimationFrame 实现
  - 修正类型签名（float -> double）
  - 修复 TaskScheduler 实例不匹配
  - 修复 ProcessAnimationFrames bug
- ✅ 修复 DOM 更新问题（RemoveAllChildren 观察者通知）
- ✅ 添加按钮默认样式
- ✅ 项目文档整理
- ✅ 创建 Phase 2.5 计划

### 2025-11-10
- ✅ 修复文本渲染问题（基线计算、背景渲染、布局 margin）
- ✅ 修复 CSS 层叠顺序
- ✅ 创建文本渲染 Bug 修复文档

---

## 📝 备注

- 详细的任务说明和技术细节请查看 [PHASE_2_5_PLAN.md](PHASE_2_5_PLAN.md)
- 每个任务完成后需要更新相关文档和测试
- 遇到问题及时记录到 `KNOWN_ISSUES.md`（已移至 history_task_docs）

---

**最后更新**: 2025-11-11  
**当前阶段**: Phase 2.5 - Week 1

