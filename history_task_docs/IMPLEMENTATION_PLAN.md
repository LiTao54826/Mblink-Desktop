# MBink 修复优化实施计划

**创建时间**: 2025-11-12  
**状态**: 待执行  
**遵循规范**: `docs/PROJECT_STANDARDS.md`  
**参考文档**: `COMPREHENSIVE_FIX_PLAN.md`, `REMAINING_ISSUES.md`

---

## 📋 执行摘要

本计划基于项目规范和现有修复文档，制定了一个**分阶段、可执行**的修复优化方案。

### 核心原则
1. ✅ **严格遵守项目规范** - 不违反技术栈锁定、模块边界等强制规范
2. ✅ **利用现有基础** - 事件系统和表单控件已有基础实现
3. ✅ **优先级驱动** - 先修复最紧急的视觉和交互问题
4. ✅ **增量交付** - 每个阶段都有可验证的成果

### 当前状态评估

| 模块 | 完成度 | 说明 |
|------|--------|------|
| **事件系统** | 70% | ✅ EventLoop、InputHandler、FocusManager已实现<br>❌ 缺少表单控件的事件集成 |
| **表单控件** | 60% | ✅ HTMLInputElement、HTMLTextAreaElement已实现<br>❌ 缺少渲染层集成和交互反馈 |
| **布局引擎** | 40% | ✅ 基础块级和行内布局<br>❌ inline-block、表格布局未完成 |
| **渲染系统** | 50% | ✅ 基础样式渲染<br>❌ 表单控件状态渲染、光标渲染未完成 |

---

## 🎯 修复计划总览

| 阶段 | 内容 | 时间 | 优先级 | 状态 |
|------|------|------|--------|------|
| **阶段 0** | 代码审查和准备 | 2小时 | 🔴 最高 | [ ] |
| **阶段 1** | 快速视觉修复 | 0.5小时 | 🔴 最高 | [ ] |
| **阶段 2** | 表单控件集成 | 8-12小时 | 🔴 最高 | [ ] |
| **阶段 3** | 布局引擎改进 | 12-18小时 | 🟡 中等 | [ ] |
| **阶段 4** | 高级功能 | 20-40小时 | 🟢 低 | [ ] |
| **总计** | | **42-72小时** | | |

---

## 🔍 阶段 0: 代码审查和准备（2小时）

### 目标
- 确认现有事件系统和表单控件的实现状态
- 识别需要集成的接口
- 制定详细的集成方案

### 任务清单

#### 0.1 审查事件系统实现（30分钟）
**文件**: `core/event/`

- [ ] 确认 `EventLoop::HandleMouseEventForDOM()` 的实现
- [ ] 确认 `EventLoop::HandleKeyboardEventForDOM()` 的实现
- [ ] 确认 `FocusManager` 的焦点管理逻辑
- [ ] 确认 `InputHandler` 的文本输入处理

**验证点**:
- 鼠标事件是否正确分发到 Element
- 键盘事件是否正确分发到焦点元素
- 焦点切换是否正常工作

#### 0.2 审查表单控件实现（30分钟）
**文件**: `core/dom/html_input_element.cpp`, `core/dom/html_textarea_element.cpp`

- [ ] 确认 `HTMLInputElement::HandleTextInput()` 的实现
- [ ] 确认 `HTMLInputElement::HandleKeyPress()` 的实现
- [ ] 确认 checkbox/radio 的状态管理
- [ ] 确认事件触发机制（change/input）

**验证点**:
- 文本输入逻辑是否完整
- 状态管理是否正确
- 事件是否正确触发

#### 0.3 审查渲染系统（30分钟）
**文件**: `core/render/style_resolver.cpp`, `core/render/render_object.cpp`

- [ ] 确认按钮样式定义
- [ ] 确认输入框样式定义
- [ ] 确认 checkbox/radio 样式定义
- [ ] 确认渲染流程

**验证点**:
- 样式是否正确应用
- 是否支持伪类（:focus, :hover, :checked）
- 是否支持状态渲染

#### 0.4 制定集成方案（30分钟）

**需要集成的部分**:
1. EventLoop → HTMLInputElement/HTMLTextAreaElement
2. RenderObject → 表单控件状态渲染
3. StyleResolver → 伪类支持
4. FocusManager → 焦点视觉反馈

**输出**: 详细的集成接口文档

---

## 🚀 阶段 1: 快速视觉修复（0.5小时）

### 目标
立即改善视觉效果，提升用户体验

### 任务清单

#### 1.1 修复按钮水平排列（10分钟）
**文件**: `core/render/style_resolver.cpp`

**问题**: 按钮垂直堆叠  
**临时方案**: 改为 INLINE 类型

```cpp
// 找到按钮样式部分（约 275 行）
if (tag_name == "button") {
    style.display = RenderObjectType::INLINE;  // 临时改为 INLINE
    style.background_color = "#F0F0F0";
    style.width = CSSLength(120, CSSUnit::PX);
    // ... 保持其他样式不变
}

// 同样修改 input[type="button"] 和 input[type="submit"]
```

**验收**: 按钮水平排列

#### 1.2 增强按钮阴影（5分钟）
**文件**: `core/render/style_resolver.cpp`

```cpp
// 找到按钮的 box-shadow 部分（约 295 行）
CSSBoxShadow shadow;
shadow.offset_y = 4;           // 改为 4（原来是 2）
shadow.blur_radius = 8;        // 改为 8（原来是 4）
shadow.color = SkColorSetARGB(80, 0, 0, 0);  // 改为 80（原来是 40）
```

**验收**: 阴影清晰可见

#### 1.3 添加等宽字体（5分钟）
**文件**: `core/render/style_resolver.cpp`

```cpp
if (tag_name == "code" || tag_name == "pre") {
    style.font_family = "Consolas, Monaco, Courier New, monospace";
}
```

**验收**: 代码使用等宽字体

#### 1.4 添加 blockquote 左边框（5分钟）
**文件**: `core/render/style_resolver.cpp`

```cpp
if (tag_name == "blockquote") {
    style.border.left.width = CSSLength(4, CSSUnit::PX);
    style.border.left.color = SkColorSetRGB(200, 200, 200);
    style.padding.left = CSSLength(16, CSSUnit::PX);
}
```

**验收**: Blockquote 有左边框

#### 1.5 添加表格边框（5分钟）
**文件**: `core/render/style_resolver.cpp`

```cpp
if (tag_name == "table" || tag_name == "td" || tag_name == "th") {
    style.border.width = CSSLength(1, CSSUnit::PX);
    style.border.color = SkColorSetRGB(200, 200, 200);
    style.padding = CSSLength(8, CSSUnit::PX);
}
```

**验收**: 表格有边框

### 编译测试
```bash
cmake --build build --config Debug --target html_window_example -j8
./build/bin/Debug/html_window_example.exe
```

---

## ⌨️ 阶段 2: 表单控件集成（8-12小时）

### 目标
将现有的事件系统和表单控件实现集成起来，实现完整的交互功能

### 2.1 集成文本输入（3-4小时）

#### 任务 2.1.1: 连接 EventLoop 和 HTMLInputElement（1小时）
**文件**: `core/event/event_loop.cpp`

**修改点**:
```cpp
void EventLoop::HandleKeyboardEventForDOM(const SDL_Event& event) {
    // 获取焦点元素
    auto focused_element = focus_manager_->GetFocusedElement();
    if (!focused_element) return;
    
    // 检查是否是 HTMLInputElement 或 HTMLTextAreaElement
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(focused_element);
    if (input_element) {
        if (event.type == SDL_EVENT_TEXT_INPUT) {
            input_element->HandleTextInput(event.text.text);
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            std::string key = KeyCodeToString(event.key.keysym.sym);
            bool ctrl = (event.key.keysym.mod & SDL_KMOD_CTRL) != 0;
            input_element->HandleKeyPress(key, ctrl);
        }
        return;
    }
    
    auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(focused_element);
    if (textarea_element) {
        // 类似处理
    }
}
```

**验收**: 
- [ ] 焦点输入框可以接收文本输入
- [ ] Backspace/Delete 可以删除字符
- [ ] Ctrl+A 可以全选

#### 任务 2.1.2: 实现光标渲染（2-3小时）
**文件**: `core/render/render_object.cpp`

**新增**: 光标渲染逻辑

```cpp
void RenderObject::PaintTextInputCursor(SkCanvas* canvas) {
    auto node = GetNode();
    if (!node || node->GetNodeType() != NodeType::ELEMENT_NODE) return;
    
    auto element = std::static_pointer_cast<Element>(node);
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(element);
    
    if (!input_element || !input_element->IsFocused()) return;
    
    // 计算光标位置
    std::string value = input_element->GetValue();
    int cursor_pos = input_element->GetSelectionStart();
    
    // 使用 SkFont 测量文本宽度
    SkFont font;
    float text_width = font.measureText(
        value.substr(0, cursor_pos).c_str(),
        value.substr(0, cursor_pos).length(),
        SkTextEncoding::kUTF8
    );
    
    // 绘制光标（闪烁效果由定时器控制）
    if (ShouldShowCursor()) {
        SkPaint cursor_paint;
        cursor_paint.setColor(SK_ColorBLACK);
        cursor_paint.setStrokeWidth(1);
        
        float cursor_x = layout_info_.x + padding_left + text_width;
        float cursor_y_top = layout_info_.y + padding_top;
        float cursor_y_bottom = cursor_y_top + line_height;
        
        canvas->drawLine(cursor_x, cursor_y_top, cursor_x, cursor_y_bottom, cursor_paint);
    }
}
```

**验收**:
- [ ] 焦点输入框显示光标
- [ ] 光标位置正确
- [ ] 光标闪烁（0.7秒周期）

### 2.2 集成 Checkbox/Radio（2-3小时）

#### 任务 2.2.1: 实现点击切换（1小时）
**文件**: `core/event/event_loop.cpp`

```cpp
void EventLoop::HandleMouseEventForDOM(const SDL_Event& event) {
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        // ... 命中测试 ...
        
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(target_element);
        if (input_element) {
            InputType type = input_element->GetInputType();
            
            if (type == InputType::Checkbox) {
                input_element->SetChecked(!input_element->IsChecked(), true);
            } else if (type == InputType::Radio) {
                // 取消同组其他 radio
                UncheckRadioGroup(input_element);
                input_element->SetChecked(true, true);
            }
        }
    }
}
```

**验收**:
- [ ] Checkbox 可以点击切换
- [ ] Radio 可以点击选中
- [ ] 同组 Radio 互斥

#### 任务 2.2.2: 实现状态渲染（1-2小时）
**文件**: `core/render/render_object.cpp`

```cpp
void RenderObject::PaintCheckboxRadio(SkCanvas* canvas) {
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(GetNode());
    if (!input_element) return;
    
    InputType type = input_element->GetInputType();
    bool checked = input_element->IsChecked();
    
    if (type == InputType::Checkbox && checked) {
        // 绘制勾选标记（✓）
        SkPaint check_paint;
        check_paint.setColor(SK_ColorBLACK);
        check_paint.setStrokeWidth(2);
        check_paint.setStyle(SkPaint::kStroke_Style);
        check_paint.setAntiAlias(true);
        
        float cx = layout_info_.x + layout_info_.width / 2;
        float cy = layout_info_.y + layout_info_.height / 2;
        
        SkPath check_path;
        check_path.moveTo(cx - 4, cy);
        check_path.lineTo(cx - 1, cy + 3);
        check_path.lineTo(cx + 4, cy - 3);
        canvas->drawPath(check_path, check_paint);
    }
    
    if (type == InputType::Radio && checked) {
        // 绘制选中的圆点
        SkPaint dot_paint;
        dot_paint.setColor(SK_ColorBLACK);
        dot_paint.setStyle(SkPaint::kFill_Style);
        dot_paint.setAntiAlias(true);
        
        float cx = layout_info_.x + layout_info_.width / 2;
        float cy = layout_info_.y + layout_info_.height / 2;
        
        canvas->drawCircle(cx, cy, 4, dot_paint);
    }
}
```

**验收**:
- [ ] Checkbox 选中时显示勾选标记
- [ ] Radio 选中时显示圆点

### 2.3 实现焦点视觉反馈（2-3小时）

#### 任务 2.3.1: 添加 :focus 伪类支持（1-2小时）
**文件**: `core/render/style_resolver.cpp`

```cpp
void StyleResolver::ApplyPseudoClasses(Element* element, ComputedStyle& style) {
    // 检查 :focus 伪类
    if (element->HasPseudoClass("focus")) {
        // 添加焦点样式
        style.border.width = CSSLength(2, CSSUnit::PX);
        style.border.color = SkColorSetRGB(66, 153, 225);  // 蓝色
        style.outline.width = CSSLength(2, CSSUnit::PX);
        style.outline.color = SkColorSetARGB(128, 66, 153, 225);  // 半透明蓝色
    }
    
    // 检查 :hover 伪类
    if (element->HasPseudoClass("hover")) {
        // 添加悬停样式
        if (element->GetTagName() == "button") {
            style.background_color = "#E0E0E0";  // 稍深的背景
        }
    }
    
    // 检查 :checked 伪类
    if (element->HasPseudoClass("checked")) {
        // 已在状态渲染中处理
    }
}
```

**验收**:
- [ ] 焦点元素有蓝色边框
- [ ] 按钮悬停时背景变深
- [ ] Tab 键可以切换焦点

#### 任务 2.3.2: 集成 FocusManager（1小时）
**文件**: `core/event/focus_manager.cpp`

确认以下功能正常:
- [ ] 点击元素设置焦点
- [ ] Tab 键切换焦点
- [ ] 焦点元素设置 :focus 伪类
- [ ] 失去焦点时移除 :focus 伪类

---

## 📐 阶段 3: 布局引擎改进（12-18小时）

### 3.1 实现真正的 inline-block（4-6小时）

**参考**: `COMPREHENSIVE_FIX_PLAN.md` 阶段 4 任务 4.1

**文件**: 新建 `core/render/render_inline_block.h/cpp`

**验收**:
- [ ] 按钮真正水平排列
- [ ] 可以设置宽高
- [ ] 支持 shrink-to-fit 宽度

### 3.2 实现表格布局（8-12小时）

**参考**: `COMPREHENSIVE_FIX_PLAN.md` 阶段 4 任务 4.2

**文件**: 新建 `core/render/render_table.h/cpp`

**验收**:
- [ ] 表格单元格正确对齐
- [ ] 列宽合理分配
- [ ] 行高自动计算

---

## 🎨 阶段 4: 高级功能（20-40小时）

### 4.1 图片加载（4-6小时）
### 4.2 Select 下拉菜单（4-6小时）
### 4.3 :hover 伪类（2-3小时）
### 4.4 overflow 和滚动（3-4小时）

**详见**: `COMPREHENSIVE_FIX_PLAN.md` 阶段 5

---

## ✅ 验收标准

### 阶段 1 完成标准
- [ ] 按钮水平排列
- [ ] 阴影清晰可见
- [ ] 代码使用等宽字体
- [ ] Blockquote 有左边框
- [ ] 表格有边框

### 阶段 2 完成标准
- [ ] 输入框可以输入文字
- [ ] 光标闪烁可见
- [ ] Backspace 可以删除字符
- [ ] Checkbox 可以点击切换
- [ ] Radio 可以点击选中
- [ ] 焦点元素有蓝色边框
- [ ] Tab 键可以切换焦点

### 阶段 3 完成标准
- [ ] 按钮真正水平排列（inline-block）
- [ ] 表格单元格正确对齐

---

## 📝 注意事项

### 遵守项目规范
1. ✅ **不得修改技术栈** - 保持 QuickJS、Skia、SDL3、Yoga、Lexbor
2. ✅ **不得违反模块边界** - 严格遵守依赖关系
3. ✅ **使用包管理器** - 不手动编辑依赖文件
4. ✅ **文档同步更新** - 修改代码后更新相关文档

### 测试要求
1. ✅ 每个阶段完成后运行测试
2. ✅ 编写单元测试（如果修改核心逻辑）
3. ✅ 手动测试交互功能

### Git 提交规范
1. ✅ 每个任务完成后提交
2. ✅ 提交信息格式: `[阶段X.Y] 任务描述`
3. ✅ 不提交 build/ 目录

---

**下一步**: 执行阶段 0 - 代码审查和准备

