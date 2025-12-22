# 需求文档

## 简介

Layer 分层系统是一个完整的 CSS Stacking Context 实现，用于解决当前 OverlayManager 只处理绘制顺序而忽略事件处理的问题。该系统将支持正确的绘制顺序、Hit Testing、事件路由和滚动隔离，确保 dropdown、modal 等高 z-index 元素能够正确响应用户交互。

## 术语表

- **Layer**: 渲染层级，管理特定 z-index 范围内的元素
- **LayerManager**: 层管理器，负责管理多个 Layer 并提供统一的绘制、Hit Testing 和事件处理接口
- **Hit Testing**: 点击测试，根据鼠标坐标查找对应的 DOM 元素
- **Stacking Context**: CSS 层叠上下文，决定元素的绘制顺序和事件响应优先级
- **Overlay**: 覆盖层元素，如 dropdown、tooltip、popover（z-index 100-999）
- **Modal**: 模态层元素，如 modal、dialog（z-index >= 1000）
- **RenderObject**: 渲染对象，DOM 元素在渲染树中的表示
- **EventLoop**: 事件循环，处理用户输入事件的核心组件

## 需求

### 需求 1

**用户故事:** 作为开发者，我希望 Layer 系统能够正确管理元素的绘制顺序，以便高 z-index 元素始终显示在低 z-index 元素之上。

#### 验收标准

1. WHEN LayerManager 开始新的渲染帧 THEN LayerManager SHALL 清除所有 Layer 中的元素
2. WHEN RenderObject 的 z-index >= 100 且 position 为 absolute/fixed/relative THEN LayerManager SHALL 将该元素收集到对应的 Layer 中
3. WHEN LayerManager 绘制所有 Layer THEN LayerManager SHALL 按 Base -> Overlay -> Modal 的顺序绘制
4. WHEN 同一 Layer 内有多个元素 THEN Layer SHALL 按 z-index 升序绘制元素

### 需求 2

**用户故事:** 作为用户，我希望点击 dropdown 选项时能够正确响应，以便我可以正常使用下拉菜单功能。

#### 验收标准

1. WHEN 用户点击 Overlay 层的元素 THEN LayerManager SHALL 优先返回 Overlay 层的 Hit Testing 结果
2. WHEN 用户点击 Modal 层的元素 THEN LayerManager SHALL 优先返回 Modal 层的 Hit Testing 结果
3. WHEN 高层 Layer 没有命中元素 THEN LayerManager SHALL 继续在低层 Layer 中进行 Hit Testing
4. WHEN Layer 内进行 Hit Testing THEN Layer SHALL 从高 z-index 到低 z-index 顺序测试元素
5. WHEN 元素设置 pointer-events: none THEN Layer SHALL 跳过该元素并继续测试其他元素

### 需求 3

**用户故事:** 作为用户，我希望在 dropdown 内滚动时不会触发页面滚动，以便我可以在下拉菜单中浏览长列表。

#### 验收标准

1. WHEN 用户在 Overlay 层的可滚动元素上滚动 THEN LayerManager SHALL 将滚动事件路由到该元素
2. WHEN Overlay 层的元素处理了滚动事件 THEN LayerManager SHALL 阻止事件传递到 Base 层
3. WHEN Overlay 层没有可滚动元素命中 THEN LayerManager SHALL 将滚动事件传递到 Base 层
4. WHEN 元素的 overflow 为 scroll 或 auto 且有滚动空间 THEN Layer SHALL 处理该元素的滚动事件

### 需求 4

**用户故事:** 作为用户，我希望 hover 效果在 dropdown 选项上正常工作，以便我可以看到当前悬停的选项。

#### 验收标准

1. WHEN 鼠标移动到 Overlay 层元素上 THEN EventLoop SHALL 使用 LayerManager 的 Hit Testing 结果更新 hover 状态
2. WHEN 鼠标从 Base 层移动到 Overlay 层 THEN EventLoop SHALL 正确触发 mouseenter 和 mouseleave 事件
3. WHEN 鼠标在 Overlay 层元素之间移动 THEN EventLoop SHALL 正确更新 hover chain

### 需求 5

**用户故事:** 作为开发者，我希望 Layer 系统能够支持多层级嵌套，以便我可以在 Modal 中打开另一个 Modal 或 dropdown。

#### 验收标准

1. WHEN Modal 层有元素 THEN LayerManager SHALL 在 Hit Testing 时优先检查 Modal 层
2. WHEN 多个 Modal 层元素重叠 THEN Layer SHALL 按 z-index 顺序确定命中优先级
3. WHEN 检查是否有 Overlay 元素 THEN LayerManager SHALL 返回 Overlay 层或 Modal 层是否有元素

### 需求 6

**用户故事:** 作为开发者，我希望能够平滑地从 OverlayManager 迁移到 LayerManager，以便现有功能不受影响。

#### 验收标准

1. WHEN LayerManager 替代 OverlayManager THEN LayerManager SHALL 保持相同的元素收集逻辑
2. WHEN LayerManager 绘制元素 THEN LayerManager SHALL 产生与 OverlayManager 相同的绘制结果
3. WHEN 代码调用 OverlayManager 的接口 THEN 开发者 SHALL 能够使用 LayerManager 的等效接口替代
