# MBink 修复优化快速参考

**创建时间**: 2025-11-12  
**用途**: 快速查阅关键信息和代码片段

---

## 🎯 当前状态

| 模块 | 完成度 | 关键文件 |
|------|--------|---------|
| **事件系统** | 70% | `core/event/event_loop.cpp` |
| **表单控件** | 60% | `core/dom/html_input_element.cpp` |
| **渲染系统** | 50% | `core/render/style_resolver.cpp` |
| **布局引擎** | 40% | `core/render/render_block.cpp` |

---

## 📋 执行顺序

```
阶段 0 (2h)   → 代码审查和准备
阶段 1 (0.5h) → 快速视觉修复 ✨ 立即见效
阶段 2 (8-12h) → 表单控件集成 🎯 核心功能
阶段 3 (12-18h) → 布局引擎改进
阶段 4 (20-40h) → 高级功能
```

---

## 🔧 阶段 1: 快速视觉修复（0.5小时）

### 文件: `core/render/style_resolver.cpp`

#### 1.1 修复按钮水平排列（10分钟）
```cpp
// 找到按钮样式部分（约 275 行）
if (tag_name == "button") {
    style.display = RenderObjectType::INLINE;  // 改这里
    // ... 其他样式保持不变
}
```

#### 1.2 增强按钮阴影（5分钟）
```cpp
// 找到 box-shadow 部分（约 295 行）
CSSBoxShadow shadow;
shadow.offset_y = 4;           // 改为 4
shadow.blur_radius = 8;        // 改为 8
shadow.color = SkColorSetARGB(80, 0, 0, 0);  // 改为 80
```

#### 1.3 添加等宽字体（5分钟）
```cpp
if (tag_name == "code" || tag_name == "pre") {
    style.font_family = "Consolas, Monaco, Courier New, monospace";
}
```

#### 1.4 添加 blockquote 左边框（5分钟）
```cpp
if (tag_name == "blockquote") {
    style.border.left.width = CSSLength(4, CSSUnit::PX);
    style.border.left.color = SkColorSetRGB(200, 200, 200);
    style.padding.left = CSSLength(16, CSSUnit::PX);
}
```

#### 1.5 添加表格边框（5分钟）
```cpp
if (tag_name == "table" || tag_name == "td" || tag_name == "th") {
    style.border.width = CSSLength(1, CSSUnit::PX);
    style.border.color = SkColorSetRGB(200, 200, 200);
    style.padding = CSSLength(8, CSSUnit::PX);
}
```

### 编译测试
```bash
cmake --build build --config Debug --target html_window_example -j8
./build/bin/Debug/html_window_example.exe
```

---

## ⌨️ 阶段 2: 表单控件集成（8-12小时）

### 2.1 连接 EventLoop 和 HTMLInputElement

#### 文件: `core/event/event_loop.cpp`

```cpp
void EventLoop::HandleKeyboardEventForDOM(const SDL_Event& event) {
    // 获取焦点元素
    auto focused_element = focus_manager_->GetFocusedElement();
    if (!focused_element) return;
    
    // 检查是否是 HTMLInputElement
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(focused_element);
    if (input_element) {
        if (event.type == SDL_EVENT_TEXT_INPUT) {
            // 文本输入
            input_element->HandleTextInput(event.text.text);
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            // 键盘按键
            std::string key = KeyCodeToString(event.key.keysym.sym);
            bool ctrl = (event.key.keysym.mod & SDL_KMOD_CTRL) != 0;
            input_element->HandleKeyPress(key, ctrl);
        }
        return;
    }
    
    // 检查是否是 HTMLTextAreaElement
    auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(focused_element);
    if (textarea_element) {
        // 类似处理
    }
}
```

### 2.2 实现光标渲染

#### 文件: `core/render/render_object.cpp`

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

bool RenderObject::ShouldShowCursor() {
    // 光标闪烁周期：0.7秒
    static const float CURSOR_BLINK_TIME = 0.7f;
    static float cursor_time = 0.0f;
    
    cursor_time += GetDeltaTime();
    if (cursor_time >= CURSOR_BLINK_TIME) {
        cursor_time = 0.0f;
    }
    
    return cursor_time < CURSOR_BLINK_TIME / 2;
}
```

### 2.3 实现 Checkbox/Radio 状态渲染

#### 文件: `core/render/render_object.cpp`

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

### 2.4 实现焦点视觉反馈

#### 文件: `core/render/style_resolver.cpp`

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

---

## 🔍 关键接口

### HTMLInputElement 已有方法
```cpp
// 文本输入
void HandleTextInput(const std::string& text);
void HandleKeyPress(const std::string& key, bool ctrl_key);

// 状态管理
bool IsChecked() const;
void SetChecked(bool checked, bool trigger_events = false);
std::string GetValue() const;
void SetValue(const std::string& value, bool trigger_events = false);

// 选择管理
int GetSelectionStart() const;
int GetSelectionEnd() const;
void SetSelectionRange(int start, int end);
void Select();

// 验证
bool IsDisabled() const;
bool IsReadOnly() const;
int GetMaxLength() const;
```

### EventLoop 已有方法
```cpp
// 事件处理
void HandleMouseEventForDOM(const SDL_Event& event);
void HandleKeyboardEventForDOM(const SDL_Event& event);

// 焦点管理
FocusManager* GetFocusManager();
```

### Element 已有方法
```cpp
// 伪类管理
void SetPseudoClass(const std::string& name, bool active);
bool HasPseudoClass(const std::string& name) const;

// 事件分发
void DispatchEvent(std::shared_ptr<Event> event);
void AddEventListener(const std::string& type, EventListener listener);
```

---

## ✅ 验收标准

### 阶段 1 完成
- [ ] 按钮水平排列
- [ ] 阴影清晰可见
- [ ] 代码使用等宽字体
- [ ] Blockquote 有左边框
- [ ] 表格有边框

### 阶段 2 完成
- [ ] 输入框可以输入文字
- [ ] 光标闪烁可见
- [ ] Backspace 可以删除字符
- [ ] Checkbox 可以点击切换
- [ ] Radio 可以点击选中
- [ ] 焦点元素有蓝色边框
- [ ] Tab 键可以切换焦点

---

## 🚨 常见问题

### Q1: 光标不显示？
**检查**:
1. 元素是否有焦点？
2. `ShouldShowCursor()` 是否返回 true？
3. 光标位置计算是否正确？

### Q2: 文本输入无响应？
**检查**:
1. `EventLoop::HandleKeyboardEventForDOM()` 是否被调用？
2. 焦点元素是否是 HTMLInputElement？
3. `HandleTextInput()` 是否被调用？

### Q3: Checkbox 点击无反应？
**检查**:
1. `EventLoop::HandleMouseEventForDOM()` 是否被调用？
2. 命中测试是否正确？
3. `SetChecked()` 是否被调用？

---

## 📚 参考文档

1. **完整计划**: `IMPLEMENTATION_PLAN.md`
2. **执行摘要**: `EXECUTION_SUMMARY.md`
3. **详细修复**: `COMPREHENSIVE_FIX_PLAN.md`
4. **问题清单**: `REMAINING_ISSUES.md`
5. **项目规范**: `docs/PROJECT_STANDARDS.md`

---

**快速开始**: 执行阶段 1，立即见效！ ✨

