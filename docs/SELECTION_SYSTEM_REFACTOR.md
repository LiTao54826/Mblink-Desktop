# 选择系统重构计划

## 当前问题

当前项目中，Terminal、LogView、TextArea 等元素各自实现选择逻辑，存在大量冗余：

- 每个元素有自己的 `SelectionManager`
- 鼠标事件需要单独分发到每个元素
- 选择高亮各自绘制
- 无法跨元素选择
- 每个新元素都需要重复实现相同的选择、复制、光标逻辑

## Chrome/Blink 的做法

1. **统一的选择系统** - `FrameSelection` 类管理整个 frame 的选择状态
2. **基于 Range 的选择** - 使用 DOM Range API，选择跨元素
3. **渲染层统一处理** - 选择高亮在渲染层统一绘制
4. **Hit Testing 统一** - 点击位置转换为 DOM 位置由统一系统完成

## 目标架构

```
Document
  └── Selection (统一管理)
        ├── anchor: {node, offset}  // 选择起点
        └── focus: {node, offset}   // 选择终点

MouseEventDispatcher
  └── 统一的 hit testing → DOM 位置
        └── 更新 Document::Selection

KeyboardEventDispatcher
  └── 统一处理 Ctrl+A/C/V/X 等快捷键
        └── 操作 Document::Selection

RenderTree
  └── 统一绘制选择高亮
```

## 重构步骤

### Phase 1: 创建统一选择类
- [ ] 创建 `core/dom/selection/document_selection.h/cpp`
- [ ] 实现 anchor/focus 位置管理
- [ ] 实现 Range 接口
- [ ] 统一 GetSelectedText() 接口

### Phase 2: 统一 Hit Testing
- [ ] 扩展 HitTestResult 返回 DOM 位置（node + offset）
- [ ] 支持文本节点内的字符级定位
- [ ] 支持虚拟滚动元素（Terminal、LogView）的行列定位

### Phase 3: 统一事件处理
- [ ] MouseEventDispatcher 统一处理选择
  - 鼠标按下：开始选择
  - 鼠标移动：更新选择
  - 鼠标释放：结束选择
  - 双击：选词
  - 三击：选行
- [ ] KeyboardEventDispatcher 统一处理快捷键
  - Ctrl+A：全选
  - Ctrl+C：复制
  - Ctrl+X：剪切
  - Ctrl+V：粘贴
- [ ] 移除各元素的单独选择/键盘处理

### Phase 4: 统一渲染
- [ ] 在 RenderObject 层统一绘制选择高亮
- [ ] 移除各元素的单独高亮绘制
- [ ] 统一光标渲染

### Phase 5: 统一剪贴板
- [ ] 创建 `core/editing/clipboard.h/cpp`
- [ ] 统一 Windows/macOS/Linux 剪贴板操作
- [ ] 支持富文本复制（可选）

### Phase 6: 清理
- [ ] 移除 Terminal/LogView/TextArea 中的 SelectionManager
- [ ] 移除各元素中的 CopySelection/SelectAll 等方法
- [ ] 移除各元素中的 OnKeyDown 选择相关处理

## 受影响的文件

- `core/dom/elements/terminal/html_terminal_element.cpp`
- `core/dom/elements/logview/html_logview_element.cpp`
- `core/dom/elements/html_textarea_element.cpp`
- `core/dom/elements/html_input_element.cpp`
- `core/dom/elements/virtual_text/selection_manager.h/cpp`
- `core/event/dispatch/mouse_event_dispatcher.cpp`
- `core/event/dispatch/keyboard_event_dispatcher.cpp`
- `core/render/objects/render_object.cpp`

## 优先级

中等 - 当前各元素独立实现可用，但长期维护成本高。建议在核心功能稳定后进行重构。

## 备注

当前实现中发现的问题：
1. 每个元素需要单独处理 ForceRasterize() 才能实时显示选择高亮
2. 键盘事件需要传递 key 字符串而非 key_code 才能正确匹配
3. Windows 的 GetMessage 宏与代码冲突，需要 #undef
4. 选择高亮是整行的，但 SelectionManager 仍然记录列位置（冗余）
