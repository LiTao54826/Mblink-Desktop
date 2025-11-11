# Phase 2.5: JavaScript 基础设施完善

## 📋 概述

**目标**: 完善 JavaScript 基础设施，为支持 React 和完整桌面 UI 开发打好基础  
**优先级**: 🔥 高优先级（阻塞后续开发）  
**预计时间**: 2-3 周  
**开始时间**: 2025-11-11

---

## 🎯 核心目标

在 Phase 2.4 开发示例应用时，我们发现基础设施不完善导致开发困难：
- 按钮无法点击（缺少事件系统）
- DOM API 不完整（无 querySelector, classList 等）
- CSS 支持有限（无选择器、无伪类）
- HTML 元素类型少（无 input, textarea 等）

**Phase 2.5 的目标**是系统性地完善这些基础功能，使得：
1. ✅ 可以开发完整的交互式应用（按钮、表单、输入框）
2. ✅ 支持现代 Web 开发模式（事件监听、DOM 操作、CSS 选择器）
3. ✅ 为后续集成 React 做好准备

---

## 📊 任务清单

### 🔴 P0: 核心事件系统（必须完成）

#### Task 1: 鼠标事件基础设施
- [ ] **1.1** 实现 Hit Testing（点击检测）
  - 遍历渲染树，检测鼠标位置是否在元素边界内
  - 考虑 z-index 和元素层叠顺序
  - 返回被点击的最上层元素
  
- [ ] **1.2** 实现 MouseEvent 类
  - 继承自 `Event` 基类
  - 属性：`clientX`, `clientY`, `screenX`, `screenY`, `button`, `buttons`
  - 方法：`preventDefault()`, `stopPropagation()`
  
- [ ] **1.3** 集成到 EventLoop
  - 在 `ProcessEvents()` 中处理 SDL 鼠标事件
  - 执行 Hit Testing 找到目标元素
  - 创建 MouseEvent 并分发到目标元素
  
- [ ] **1.4** 支持的鼠标事件类型
  - `click` - 鼠标点击（mousedown + mouseup）
  - `mousedown` - 鼠标按下
  - `mouseup` - 鼠标释放
  - `mousemove` - 鼠标移动
  - `mouseenter` - 鼠标进入元素
  - `mouseleave` - 鼠标离开元素

**相关文件**:
- `core/event/mouse_event.h` (新建)
- `core/event/mouse_event.cpp` (新建)
- `core/event/hit_testing.h` (新建)
- `core/event/hit_testing.cpp` (新建)
- `core/event/event_loop.cpp` (修改)

**测试**:
- 创建 `tests/test_mouse_events.cpp`
- 测试点击检测准确性
- 测试事件冒泡和捕获

---

#### Task 2: JavaScript 事件绑定

- [ ] **2.1** 实现 `element.addEventListener` JavaScript 绑定
  - 支持事件类型：`click`, `mousedown`, `mouseup`, `mousemove`
  - 支持 `useCapture` 参数
  - 返回可用于移除的句柄
  
- [ ] **2.2** 实现 `element.removeEventListener` JavaScript 绑定
  - 根据事件类型和回调函数移除监听器
  
- [ ] **2.3** 实现 `window.addEventListener` 绑定
  - 支持全局事件监听
  
- [ ] **2.4** 事件对象传递到 JavaScript
  - 将 C++ `MouseEvent` 转换为 JavaScript 对象
  - 包含所有必要属性（target, clientX, clientY 等）
  - 支持 `preventDefault()` 和 `stopPropagation()`

**相关文件**:
- `core/quickjs/window_bindings.cpp` (修改)
- `core/quickjs/event_bindings.h` (新建)
- `core/quickjs/event_bindings.cpp` (新建)

**测试**:
- 更新 `examples/animation_demo.cpp` - 让按钮可点击
- 更新 `examples/counter_app.cpp` - 让所有按钮可点击
- 创建 `examples/event_demo.cpp` - 演示各种事件

---

### 🟠 P1: DOM API 完善（重要）

#### Task 3: 查询选择器

- [ ] **3.1** 实现 CSS 选择器解析器
  - 支持标签选择器：`div`, `button`, `p`
  - 支持 ID 选择器：`#my-id`
  - 支持 class 选择器：`.my-class`
  - 支持组合选择器：`div.my-class`, `#id.class`
  - 支持后代选择器：`div p`, `#container .item`
  
- [ ] **3.2** 实现 `document.querySelector`
  - 返回第一个匹配的元素
  - 返回 JavaScript 代理对象（支持属性访问）
  
- [ ] **3.3** 实现 `document.querySelectorAll`
  - 返回所有匹配的元素数组
  - 支持数组方法（forEach, map 等）
  
- [ ] **3.4** 实现 `element.querySelector` 和 `element.querySelectorAll`
  - 在元素的子树中查询

**相关文件**:
- `core/dom/selector_parser.h` (新建)
- `core/dom/selector_parser.cpp` (新建)
- `core/dom/document.cpp` (修改)
- `core/quickjs/window_bindings.cpp` (修改)

**测试**:
- 创建 `tests/test_selector_parser.cpp`
- 测试各种选择器组合

---

#### Task 4: 元素属性和样式操作

- [ ] **4.1** 实现 `element.setAttribute(name, value)`
  - 设置元素属性
  - 触发重新渲染（如果需要）
  
- [ ] **4.2** 实现 `element.getAttribute(name)`
  - 获取元素属性
  
- [ ] **4.3** 实现 `element.removeAttribute(name)`
  - 移除元素属性
  
- [ ] **4.4** 实现 `element.classList`
  - `classList.add(className)` - 添加 class
  - `classList.remove(className)` - 移除 class
  - `classList.toggle(className)` - 切换 class
  - `classList.contains(className)` - 检查是否包含 class
  
- [ ] **4.5** 实现 `element.style.setProperty(name, value)`
  - 设置内联样式
  - 触发重新渲染

**相关文件**:
- `core/dom/element.cpp` (修改)
- `core/quickjs/window_bindings.cpp` (修改)

**测试**:
- 创建 `tests/test_dom_attributes.cpp`
- 创建 `examples/style_demo.cpp` - 演示动态样式修改

---

#### Task 5: DOM 操作 API

- [ ] **5.1** 实现 `element.appendChild(child)`
  - 添加子元素
  - 触发重新渲染
  
- [ ] **5.2** 实现 `element.removeChild(child)`
  - 移除子元素
  - 触发重新渲染
  
- [ ] **5.3** 实现 `element.insertBefore(newNode, referenceNode)`
  - 在指定位置插入元素
  
- [ ] **5.4** 实现 `element.replaceChild(newChild, oldChild)`
  - 替换子元素
  
- [ ] **5.5** 实现 `element.cloneNode(deep)`
  - 克隆元素（深拷贝或浅拷贝）

**相关文件**:
- `core/dom/node.cpp` (修改)
- `core/quickjs/window_bindings.cpp` (修改)

**测试**:
- 创建 `tests/test_dom_manipulation.cpp`
- 创建 `examples/dynamic_dom_demo.cpp` - 演示动态 DOM 操作

---

### 🟡 P2: HTML 元素扩展（重要）

#### Task 6: 表单元素

- [ ] **6.1** 实现 `<input>` 元素
  - 类型：`text`, `password`, `number`, `checkbox`, `radio`
  - 属性：`value`, `checked`, `disabled`, `placeholder`
  - 事件：`input`, `change`, `focus`, `blur`
  - 默认样式
  
- [ ] **6.2** 实现 `<textarea>` 元素
  - 属性：`value`, `rows`, `cols`, `placeholder`
  - 事件：`input`, `change`
  - 默认样式
  
- [ ] **6.3** 实现 `<select>` 和 `<option>` 元素
  - 属性：`value`, `selected`, `disabled`
  - 事件：`change`
  - 默认样式
  
- [ ] **6.4** 实现 `<label>` 元素
  - 属性：`for` (关联到 input)
  - 点击 label 聚焦关联的 input

**相关文件**:
- `core/dom/html_input_element.h` (新建)
- `core/dom/html_input_element.cpp` (新建)
- `core/dom/html_textarea_element.h` (新建)
- `core/dom/html_textarea_element.cpp` (新建)
- `core/dom/html_select_element.h` (新建)
- `core/dom/html_select_element.cpp` (新建)
- `core/render/style_resolver.cpp` (修改 - 添加默认样式)

**测试**:
- 创建 `examples/form_demo.cpp` - 演示表单元素

---

#### Task 7: 其他常用元素

- [ ] **7.1** 实现 `<img>` 元素
  - 属性：`src`, `alt`, `width`, `height`
  - 支持加载本地图片
  - 使用 Skia 渲染图片
  
- [ ] **7.2** 实现 `<a>` 元素（链接）
  - 属性：`href`, `target`
  - 默认样式（蓝色、下划线）
  - 点击事件
  
- [ ] **7.3** 实现 `<span>` 元素
  - 内联元素
  - 默认样式
  
- [ ] **7.4** 实现 `<ul>`, `<ol>`, `<li>` 元素
  - 列表样式
  - 默认缩进和项目符号

**相关文件**:
- `core/dom/html_image_element.h` (新建)
- `core/dom/html_image_element.cpp` (新建)
- `core/render/render_image.h` (新建)
- `core/render/render_image.cpp` (新建)
- `core/render/style_resolver.cpp` (修改)

**测试**:
- 创建 `examples/image_demo.cpp`
- 创建 `examples/list_demo.cpp`

---

### 🟢 P3: CSS 功能扩展（可选）

#### Task 8: CSS 选择器和样式表

- [ ] **8.1** 实现 `<style>` 标签支持
  - 解析 `<style>` 标签内的 CSS
  - 构建样式表
  - 应用到匹配的元素
  
- [ ] **8.2** 实现 CSS 规则匹配
  - 根据选择器匹配元素
  - 计算样式优先级（specificity）
  - 合并多个规则
  
- [ ] **8.3** 支持外部样式表
  - `<link rel="stylesheet" href="...">`
  - 加载和解析 CSS 文件

**相关文件**:
- `core/render/css_parser.h` (新建)
- `core/render/css_parser.cpp` (新建)
- `core/render/stylesheet.h` (新建)
- `core/render/stylesheet.cpp` (新建)
- `core/render/style_resolver.cpp` (修改)

---

#### Task 9: CSS 伪类支持

- [ ] **9.1** 实现 `:hover` 伪类
  - 鼠标悬停时应用样式
  - 鼠标离开时移除样式
  
- [ ] **9.2** 实现 `:active` 伪类
  - 鼠标按下时应用样式
  - 鼠标释放时移除样式
  
- [ ] **9.3** 实现 `:focus` 伪类
  - 元素获得焦点时应用样式
  - 元素失去焦点时移除样式
  
- [ ] **9.4** 实现 `:disabled` 伪类
  - 元素禁用时应用样式

**相关文件**:
- `core/render/style_resolver.cpp` (修改)
- `core/dom/element.cpp` (修改 - 添加状态管理)

---

#### Task 10: CSS 动画和过渡

- [ ] **10.1** 实现 CSS Transitions
  - 属性：`transition-property`, `transition-duration`, `transition-timing-function`
  - 支持常见属性过渡（opacity, transform, color 等）
  
- [ ] **10.2** 实现 CSS Animations
  - `@keyframes` 规则
  - 属性：`animation-name`, `animation-duration`, `animation-iteration-count`
  
- [ ] **10.3** 集成到渲染循环
  - 在每帧更新动画状态
  - 插值计算

**相关文件**:
- `core/render/animation.h` (新建)
- `core/render/animation.cpp` (新建)
- `core/event/event_loop.cpp` (修改)

---

### 🔵 P4: 键盘和焦点管理（可选）

#### Task 11: 键盘事件

- [ ] **11.1** 实现 KeyboardEvent 类
  - 属性：`key`, `code`, `keyCode`, `shiftKey`, `ctrlKey`, `altKey`
  
- [ ] **11.2** 集成到 EventLoop
  - 处理 SDL 键盘事件
  - 分发到焦点元素
  
- [ ] **11.3** 支持的键盘事件
  - `keydown` - 按键按下
  - `keyup` - 按键释放
  - `keypress` - 字符输入（已废弃，但可选支持）

**相关文件**:
- `core/event/keyboard_event.h` (新建)
- `core/event/keyboard_event.cpp` (新建)
- `core/event/event_loop.cpp` (修改)

---

#### Task 12: 焦点管理

- [ ] **12.1** 实现焦点管理器
  - 跟踪当前焦点元素
  - 支持 Tab 键切换焦点
  
- [ ] **12.2** 实现焦点事件
  - `focus` - 元素获得焦点
  - `blur` - 元素失去焦点
  
- [ ] **12.3** 实现 `element.focus()` 和 `element.blur()` 方法
  - JavaScript 绑定

**相关文件**:
- `core/event/focus_manager.h` (新建)
- `core/event/focus_manager.cpp` (新建)
- `core/dom/element.cpp` (修改)

---

## 📈 开发顺序建议

### Week 1: 核心事件系统
1. Task 1: 鼠标事件基础设施（3 天）
2. Task 2: JavaScript 事件绑定（2 天）
3. 测试和修复 animation_demo, counter_app（2 天）

### Week 2: DOM API 完善
4. Task 3: 查询选择器（2 天）
5. Task 4: 元素属性和样式操作（2 天）
6. Task 5: DOM 操作 API（2 天）
7. 创建综合示例（1 天）

### Week 3: HTML 元素扩展
8. Task 6: 表单元素（3 天）
9. Task 7: 其他常用元素（2 天）
10. 创建表单示例应用（2 天）

### Week 4+: CSS 和高级功能（可选）
11. Task 8-12: 根据需求选择性实现

---

## 🎯 成功标准

Phase 2.5 完成后，应该能够：

1. ✅ **交互式应用**
   - 按钮可以点击并触发 JavaScript 回调
   - 表单可以输入和提交
   - 动态修改 DOM 和样式

2. ✅ **现代 Web API**
   - 使用 `querySelector` 查询元素
   - 使用 `addEventListener` 绑定事件
   - 使用 `classList` 操作 class
   - 使用 `appendChild` 等操作 DOM

3. ✅ **完整示例应用**
   - TODO 应用（增删改查）
   - 表单验证应用
   - 动态样式演示
   - 事件处理演示

4. ✅ **为 React 做好准备**
   - DOM API 足够完整
   - 事件系统符合标准
   - 性能可接受

---

## 📝 文档要求

每个任务完成后需要：
1. 更新 API 文档（`docs/DOM_API.md`, `docs/EVENT_API.md` 等）
2. 创建示例代码
3. 编写单元测试
4. 更新 CHANGELOG

---

## 🔗 相关文档

- [Phase 2.4 完成报告](docs/PHASE_2_4_COMPLETION_REPORT.md)
- [项目架构](docs/ARCHITECTURE.md)
- [DOM API 文档](docs/DOM_API.md)
- [开发规范](docs/CODING_STANDARDS.md)

---

**创建时间**: 2025-11-11  
**预计完成时间**: 2025-12-02  
**负责人**: 开发团队

