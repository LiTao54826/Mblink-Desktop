# 统一输入管理系统 - 需求文档

## 简介

本文档定义了 LightUI 统一输入管理系统的需求。该系统将重构当前分散的焦点管理、选区管理、文本编辑等功能，参考 Chromium/Blink 的架构设计，建立统一的输入处理机制。

## 术语表

- **InputController**: 统一的输入控制器，负责协调所有输入相关的子系统
- **FocusController**: 焦点控制器，管理焦点状态和焦点导航
- **FrameSelection**: 选区管理器，管理文本选区和光标位置
- **EditContext**: 编辑上下文，管理可编辑元素的编辑状态
- **Caret**: 文本光标，表示插入点位置
- **Selection**: 选区，表示选中的文本范围
- **IME**: 输入法编辑器 (Input Method Editor)
- **Editable**: 可编辑接口，所有可编辑元素必须实现的接口

## 需求

### 需求 1: 统一焦点管理

**用户故事:** 作为开发者，我希望焦点管理由统一的控制器处理，以便焦点行为一致且可预测。

#### 验收标准

1. WHEN 用户点击可聚焦元素 THEN InputController SHALL 将焦点转移到该元素并触发相应的焦点事件
2. WHEN 用户点击非可聚焦元素 THEN InputController SHALL 保持当前焦点不变
3. WHEN JavaScript 调用 element.focus() THEN InputController SHALL 将焦点转移到目标元素
4. WHEN JavaScript 调用 element.blur() THEN InputController SHALL 从目标元素移除焦点
5. WHEN 用户按 Tab 键 THEN InputController SHALL 按 tabindex 顺序导航到下一个可聚焦元素
6. WHEN 焦点元素被从 DOM 移除 THEN InputController SHALL 自动清除焦点并触发 blur 事件

### 需求 2: 统一选区管理

**用户故事:** 作为开发者，我希望文本选区由统一的管理器处理，以便选区操作在所有可编辑元素中行为一致。

#### 验收标准

1. WHEN 用户在可编辑元素中点击 THEN FrameSelection SHALL 将光标定位到点击位置
2. WHEN 用户拖动鼠标选择文本 THEN FrameSelection SHALL 更新选区范围
3. WHEN 用户按 Shift+方向键 THEN FrameSelection SHALL 扩展或收缩选区
4. WHEN 用户按 Ctrl+A THEN FrameSelection SHALL 选中所有内容
5. WHEN 用户双击单词 THEN FrameSelection SHALL 选中整个单词
6. WHEN 用户三击 THEN FrameSelection SHALL 选中整行或整段

### 需求 3: 统一文本输入处理

**用户故事:** 作为开发者，我希望文本输入由统一的处理器处理，以便输入行为在所有可编辑元素中一致。

#### 验收标准

1. WHEN 用户输入字符 THEN InputController SHALL 在光标位置插入字符并更新光标位置
2. WHEN 用户按 Backspace THEN InputController SHALL 删除光标前的字符或删除选中内容
3. WHEN 用户按 Delete THEN InputController SHALL 删除光标后的字符或删除选中内容
4. WHEN 用户粘贴文本 THEN InputController SHALL 在光标位置插入粘贴内容
5. WHEN 用户剪切文本 THEN InputController SHALL 复制选中内容到剪贴板并删除
6. WHEN 用户使用 IME 输入 THEN InputController SHALL 正确处理组合输入和确认输入

### 需求 4: 可编辑元素接口统一

**用户故事:** 作为开发者，我希望所有可编辑元素实现统一的接口，以便 InputController 可以统一处理。

#### 验收标准

1. THE HTMLInputElement SHALL 实现 Editable 接口
2. THE HTMLTextAreaElement SHALL 实现 Editable 接口
3. THE contentEditable 元素 SHALL 通过 EditContext 实现 Editable 接口
4. WHEN 新增可编辑元素类型 THEN 该元素 SHALL 实现 Editable 接口以获得统一的输入处理

### 需求 5: 光标渲染统一

**用户故事:** 作为开发者，我希望光标渲染由统一的系统处理，以便光标样式和闪烁行为一致。

#### 验收标准

1. WHEN 可编辑元素获得焦点 THEN InputController SHALL 显示闪烁的光标
2. WHEN 可编辑元素失去焦点 THEN InputController SHALL 隐藏光标
3. WHEN 用户输入文本 THEN InputController SHALL 重置光标闪烁计时器
4. THE 光标闪烁周期 SHALL 为 500ms 亮 / 500ms 暗
5. WHEN 存在文本选区 THEN InputController SHALL 隐藏光标并显示选区高亮

### 需求 6: 事件分发统一

**用户故事:** 作为开发者，我希望输入事件由统一的入口处理，以避免事件处理的竞争条件。

#### 验收标准

1. THE InputController SHALL 作为所有输入事件的统一入口
2. WHEN 收到鼠标事件 THEN InputController SHALL 按顺序处理焦点、选区、元素交互
3. WHEN 收到键盘事件 THEN InputController SHALL 按顺序处理快捷键、文本输入、导航
4. WHEN 收到 IME 事件 THEN InputController SHALL 正确处理组合状态和最终输入
5. THE 事件处理顺序 SHALL 保证不会产生竞争条件

### 需求 7: JavaScript API 兼容性

**用户故事:** 作为开发者，我希望重构后的系统保持与现有 JavaScript API 的兼容性。

#### 验收标准

1. THE element.focus() 方法 SHALL 保持现有行为
2. THE element.blur() 方法 SHALL 保持现有行为
3. THE document.activeElement 属性 SHALL 返回当前焦点元素
4. THE Selection API (getSelection, Range) SHALL 保持现有行为
5. THE input/change/focus/blur 事件 SHALL 按 W3C 标准触发
6. THE selectionStart/selectionEnd 属性 SHALL 保持现有行为

### 需求 8: 渐进式迁移支持

**用户故事:** 作为开发者，我希望能够渐进式地迁移现有代码到新系统，以降低风险。

#### 验收标准

1. THE InputController SHALL 支持与现有代码并存
2. WHEN 元素未迁移到新系统 THEN 该元素 SHALL 继续使用旧的处理逻辑
3. WHEN 元素迁移到新系统 THEN 该元素 SHALL 使用 InputController 处理输入
4. THE 迁移过程 SHALL 不影响未迁移元素的功能
