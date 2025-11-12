# MBink 修复计划（基于 RmlUi 生产级架构）

**创建时间：** 2025-11-12
**参考框架：** RmlUi - 生产级 HTML/CSS 渲染引擎
**目标：** 修复表单交互和布局问题

---

## 📖 文档说明

本文档整合了所有修复计划，包括：
- ✅ 详细的实施步骤和代码示例
- ✅ RmlUi 架构参考和学习要点
- ✅ 时间估算和优先级建议
- ✅ 验收标准和常见陷阱

**配套文档：**
- `REMAINING_ISSUES.md` - 完整的问题清单（32 个未修复问题）

---

## 🎯 快速开始

### **立即执行（30 分钟）- 阶段 1：视觉修复**

#### 任务 1.1：修复按钮水平排列（10 分钟）
**文件：** `core/render/style_resolver.cpp`

```cpp
// 找到按钮样式部分（约 275 行）
if (tag_name == "button") {
    style.display = RenderObjectType::INLINE;  // 临时改为 INLINE
    style.background_color = "#F0F0F0";
    style.width = CSSLength(120, CSSUnit::PX);
    // ... 保持其他样式不变
}

// 同样修改 input[type="button"] 和 input[type="submit"]
if (tag_name == "input" && element) {
    std::string type = element->GetAttribute("type");
    if (type == "button" || type == "submit") {
        style.display = RenderObjectType::INLINE;  // 添加这行
    }
}
```

#### 任务 1.2：增强按钮阴影（5 分钟）
```cpp
// 找到按钮的 box-shadow 部分（约 295 行）
CSSBoxShadow shadow;
shadow.offset_y = 4;           // 改为 4（原来是 2）
shadow.blur_radius = 8;        // 改为 8（原来是 4）
shadow.color = SkColorSetARGB(80, 0, 0, 0);  // 改为 80（原来是 40）
```

#### 任务 1.3-1.5：添加细节样式（15 分钟）
```cpp
// 等宽字体
if (tag_name == "code" || tag_name == "pre") {
    style.font_family = "Consolas, Monaco, Courier New, monospace";
}

// blockquote 左边框
if (tag_name == "blockquote") {
    style.border.left.width = CSSLength(4, CSSUnit::PX);
    style.border.left.color = SkColorSetRGB(200, 200, 200);
    style.padding.left = CSSLength(16, CSSUnit::PX);
}

// 表格边框
if (tag_name == "table" || tag_name == "td" || tag_name == "th") {
    style.border.width = CSSLength(1, CSSUnit::PX);
    style.border.color = SkColorSetRGB(200, 200, 200);
    style.padding = CSSLength(8, CSSUnit::PX);
}
```

**编译测试：**
```bash
cmake --build build --config Debug --target html_window_example -j8
./build/bin/Debug/html_window_example.exe
```

**验收标准：**
- [ ] 按钮水平排列
- [ ] 阴影清晰可见
- [ ] 代码使用等宽字体
- [ ] Blockquote 有左边框
- [ ] 表格有边框

---

## 📊 总体架构对比

| 模块 | MBink 当前状态 | RmlUi 架构 | 差距 |
|------|---------------|-----------|------|
| **布局引擎** | 简单两遍布局 | FormattingContext 架构 | 🔴 大 |
| **事件系统** | 无 | EventDispatcher + 事件冒泡 | 🔴 完全缺失 |
| **表单控件** | 静态渲染 | Element + InputType + Widget 三层架构 | 🔴 大 |
| **文本输入** | 无 | WidgetTextInput + 光标管理 | 🔴 完全缺失 |
| **样式系统** | 基础 CSS | 完整 CSS2.1 + 部分 CSS3 | 🟡 中等 |
| **渲染系统** | Skia 直接渲染 | Geometry + RenderInterface | 🟢 小 |

---

## 🎯 修复计划总览

| 阶段 | 内容 | 时间 | 优先级 |
|------|------|------|--------|
| **阶段 1** | 快速视觉修复 | 0.5 小时 | 🔴 最高 |
| **阶段 2** | 事件系统基础 | 10-13 小时 | 🔴 最高 |
| **阶段 3** | 表单交互 | 12-17 小时 | 🔴 最高 |
| **阶段 4** | 布局引擎改进 | 12-18 小时 | 🟡 中等 |
| **阶段 5** | 高级功能 | 20-40 小时 | 🟢 低 |
| **总计** | | **54-88 小时** | |

---

## 📋 详细修复清单

---

## 🚀 **阶段 1：快速视觉修复（30 分钟）**

### ✅ **任务 1.1：修复按钮水平排列**
**优先级：** 🔴 最高  
**预计时间：** 10 分钟

**问题：**
- 按钮设置了固定宽度，但仍然垂直堆叠
- 原因：`INLINE_BLOCK` 被映射到 `RenderBlock`

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/Layout/InlineLevelBox.cpp
// inline-block 元素被包装为 InlineLevelBox_Atomic
// 可以在行内水平排列，但可以设置宽高
```

**临时解决方案（10 分钟）：**
```cpp
// core/render/style_resolver.cpp
if (tag_name == "button" || (tag_name == "input" && 
    (type == "button" || type == "submit"))) {
    style.display = RenderObjectType::INLINE;  // 临时改为 INLINE
    // 保持固定宽度和高度
}
```

**正确解决方案（2-3 小时，阶段 4 实现）：**
- 创建 `RenderInlineBlock` 类
- 实现 shrink-to-fit 宽度计算
- 在行内布局中支持 inline-block

---

### ✅ **任务 1.2：增强按钮阴影**
**优先级：** 🟡 中等  
**预计时间：** 5 分钟

**问题：**
- 阴影已设置但不明显

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/GeometryBoxShadow.cpp:45
// 使用离屏渲染和高斯模糊
// 阴影参数：offset-y: 4px, blur: 8px, alpha: 80
```

**解决方案：**
```cpp
// core/render/style_resolver.cpp
CSSBoxShadow shadow;
shadow.offset_x = 0;
shadow.offset_y = 4;           // 增加到 4px
shadow.blur_radius = 8;        // 增加到 8px
shadow.color = SkColorSetARGB(80, 0, 0, 0);  // 增加不透明度
```

---

### ✅ **任务 1.3：添加等宽字体**
**优先级：** 🟢 低  
**预计时间：** 5 分钟

**解决方案：**
```cpp
if (tag_name == "code" || tag_name == "pre") {
    style.font_family = "Consolas, Monaco, Courier New, monospace";
}
```

---

### ✅ **任务 1.4：添加 blockquote 左边框**
**优先级：** 🟢 低  
**预计时间：** 5 分钟

**解决方案：**
```cpp
if (tag_name == "blockquote") {
    style.border.left.width = CSSLength(4, CSSUnit::PX);
    style.border.left.color = SkColorSetRGB(200, 200, 200);
    style.padding.left = CSSLength(16, CSSUnit::PX);
}
```

---

### ✅ **任务 1.5：添加表格边框**
**优先级：** 🟢 低  
**预计时间：** 5 分钟

**解决方案：**
```cpp
if (tag_name == "table" || tag_name == "td" || tag_name == "th") {
    style.border.width = CSSLength(1, CSSUnit::PX);
    style.border.color = SkColorSetRGB(200, 200, 200);
    style.border.style = CSSBorderStyle::SOLID;
}
```

---

## 🎮 **阶段 2：事件系统基础（1-2 天）**

### ✅ **任务 2.1：实现事件分发器**
**优先级：** 🔴 最高  
**预计时间：** 4-6 小时

**RmlUi 架构：**
```cpp
// RmlUi/Source/Core/EventDispatcher.h
class EventDispatcher {
    void AttachEvent(EventId id, EventListener* listener, bool in_capture_phase);
    void DetachEvent(EventId id, EventListener* listener, bool in_capture_phase);
    bool DispatchEvent(Event& event);
    
private:
    Element* element;
    Vector<EventListenerEntry> listeners;  // 按 (id, phase) 排序
};
```

**MBink 实现计划：**

**文件结构：**
```
core/event/
├── event.h              // Event 类定义
├── event.cpp
├── event_dispatcher.h   // EventDispatcher 类
├── event_dispatcher.cpp
├── event_listener.h     // EventListener 接口
└── event_types.h        // 事件类型枚举
```

**核心类设计：**

```cpp
// core/event/event_types.h
enum class EventType {
    // 鼠标事件
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_MOVE,
    CLICK,
    DOUBLE_CLICK,
    
    // 键盘事件
    KEY_DOWN,
    KEY_UP,
    
    // 焦点事件
    FOCUS,
    BLUR,
    
    // 表单事件
    CHANGE,
    INPUT,
    SUBMIT
};

// core/event/event.h
class Event {
public:
    Event(EventType type, Element* target);
    
    EventType GetType() const { return type_; }
    Element* GetTarget() const { return target_; }
    Element* GetCurrentTarget() const { return current_target_; }
    
    void StopPropagation() { propagation_stopped_ = true; }
    void PreventDefault() { default_prevented_ = true; }
    
    bool IsPropagationStopped() const { return propagation_stopped_; }
    bool IsDefaultPrevented() const { return default_prevented_; }
    
    // 鼠标事件数据
    int GetMouseX() const { return mouse_x_; }
    int GetMouseY() const { return mouse_y_; }
    
    // 键盘事件数据
    int GetKeyCode() const { return key_code_; }
    
private:
    EventType type_;
    Element* target_;
    Element* current_target_;
    bool propagation_stopped_ = false;
    bool default_prevented_ = false;
    
    // 事件数据
    int mouse_x_ = 0;
    int mouse_y_ = 0;
    int key_code_ = 0;
};

// core/event/event_listener.h
class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void HandleEvent(Event& event) = 0;
};

// core/event/event_dispatcher.h
class EventDispatcher {
public:
    EventDispatcher(Element* element);
    
    void AddEventListener(EventType type, EventListener* listener);
    void RemoveEventListener(EventType type, EventListener* listener);
    
    bool DispatchEvent(Event& event);
    
private:
    Element* element_;
    std::unordered_map<EventType, std::vector<EventListener*>> listeners_;
};
```

**集成到 Element：**
```cpp
// core/dom/element.h
class Element : public Node {
public:
    EventDispatcher* GetEventDispatcher() { return &event_dispatcher_; }
    
    void AddEventListener(EventType type, EventListener* listener) {
        event_dispatcher_.AddEventListener(type, listener);
    }
    
    bool DispatchEvent(Event& event) {
        return event_dispatcher_.DispatchEvent(event);
    }
    
private:
    EventDispatcher event_dispatcher_{this};
};
```

**实现步骤：**
1. ✅ 创建事件类型枚举（30 分钟）
2. ✅ 实现 Event 类（1 小时）
3. ✅ 实现 EventListener 接口（30 分钟）
4. ✅ 实现 EventDispatcher（2 小时）
5. ✅ 集成到 Element（1 小时）
6. ✅ 编写测试（1 小时）

---

### ✅ **任务 2.2：实现鼠标事件处理**
**优先级：** 🔴 最高  
**预计时间：** 3-4 小时

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/Context.cpp:ProcessMouseMove()
// 1. 命中测试（hit testing）
// 2. 更新 hover 状态
// 3. 分发鼠标事件
```

**MBink 实现：**

```cpp
// core/window/window.h
class Window {
public:
    void HandleSDLEvent(const SDL_Event& event);
    
private:
    void HandleMouseDown(const SDL_MouseButtonEvent& event);
    void HandleMouseUp(const SDL_MouseButtonEvent& event);
    void HandleMouseMove(const SDL_MouseMotionEvent& event);
    
    Element* HitTest(int x, int y);  // 命中测试
    
    Element* hovered_element_ = nullptr;
    Element* focused_element_ = nullptr;
};

// core/window/window.cpp
void Window::HandleSDLEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            HandleMouseDown(event.button);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            HandleMouseUp(event.button);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            HandleMouseMove(event.motion);
            break;
    }
}

void Window::HandleMouseDown(const SDL_MouseButtonEvent& event) {
    Element* target = HitTest(event.x, event.y);
    if (target) {
        Event mouse_event(EventType::MOUSE_DOWN, target);
        mouse_event.SetMousePosition(event.x, event.y);
        target->DispatchEvent(mouse_event);
    }
}

Element* Window::HitTest(int x, int y) {
    // 从根元素开始，递归查找包含 (x, y) 的最深层元素
    return document_->HitTest(x, y);
}
```

**Element 命中测试：**
```cpp
// core/dom/element.cpp
Element* Element::HitTest(int x, int y) {
    auto render_obj = GetRenderObject();
    if (!render_obj) return nullptr;
    
    auto& layout = render_obj->GetLayoutInfo();
    
    // 检查是否在边界内
    if (x < layout.x || x > layout.x + layout.width ||
        y < layout.y || y > layout.y + layout.height) {
        return nullptr;
    }
    
    // 递归检查子元素（从后往前，因为后面的元素在上层）
    for (int i = children_.size() - 1; i >= 0; i--) {
        if (auto child_elem = std::dynamic_pointer_cast<Element>(children_[i])) {
            if (Element* hit = child_elem->HitTest(x, y)) {
                return hit;
            }
        }
    }
    
    // 没有子元素命中，返回自己
    return this;
}
```

**实现步骤：**
1. ✅ 实现命中测试（2 小时）
2. ✅ 集成 SDL 事件处理（1 小时）
3. ✅ 测试鼠标点击（1 小时）

---

### ✅ **任务 2.3：实现焦点管理**
**优先级：** 🔴 最高  
**预计时间：** 2-3 小时

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/Context.cpp
// 焦点管理：
// - 只有一个元素可以有焦点
// - Tab 键切换焦点
// - 点击元素获得焦点
// - 焦点元素有 :focus 伪类
```

**MBink 实现：**

```cpp
// core/window/window.h
class Window {
public:
    void SetFocus(Element* element);
    Element* GetFocusedElement() const { return focused_element_; }
    
private:
    Element* focused_element_ = nullptr;
};

// core/window/window.cpp
void Window::SetFocus(Element* element) {
    if (focused_element_ == element) return;
    
    // 失去焦点
    if (focused_element_) {
        Event blur_event(EventType::BLUR, focused_element_);
        focused_element_->DispatchEvent(blur_event);
        focused_element_->SetPseudoClass("focus", false);
    }
    
    // 获得焦点
    focused_element_ = element;
    if (focused_element_) {
        Event focus_event(EventType::FOCUS, focused_element_);
        focused_element_->DispatchEvent(focus_event);
        focused_element_->SetPseudoClass("focus", true);
    }
}

void Window::HandleMouseDown(const SDL_MouseButtonEvent& event) {
    Element* target = HitTest(event.x, event.y);
    if (target) {
        // 设置焦点
        if (target->IsFocusable()) {
            SetFocus(target);
        }
        
        // 分发点击事件
        Event mouse_event(EventType::MOUSE_DOWN, target);
        target->DispatchEvent(mouse_event);
    }
}
```

**Element 焦点支持：**
```cpp
// core/dom/element.h
class Element : public Node {
public:
    bool IsFocusable() const;
    void SetPseudoClass(const std::string& name, bool active);
    
private:
    std::set<std::string> pseudo_classes_;
};

// core/dom/element.cpp
bool Element::IsFocusable() const {
    // 可聚焦元素：input, button, textarea, select, a
    std::string tag = GetTagName();
    return tag == "input" || tag == "button" || 
           tag == "textarea" || tag == "select" || tag == "a";
}
```

**实现步骤：**
1. ✅ 实现焦点管理器（1 小时）
2. ✅ 实现伪类系统（1 小时）
3. ✅ 测试焦点切换（30 分钟）

---

## ⌨️ **阶段 3：表单交互（3-5 天）**

### ✅ **任务 3.1：实现文本输入**
**优先级：** 🔴 最高  
**预计时间：** 6-8 小时

**RmlUi 架构：**
```cpp
// RmlUi 的三层架构：
// 1. ElementFormControlInput - DOM 元素
// 2. InputTypeText - 类型处理器
// 3. WidgetTextInput - 实际的输入控件（光标、选择、编辑）
```

**MBink 简化实现：**

```cpp
// core/widgets/text_input_widget.h
class TextInputWidget {
public:
    TextInputWidget(Element* element);
    
    void HandleKeyDown(const SDL_KeyboardEvent& event);
    void HandleTextInput(const SDL_TextInputEvent& event);
    
    void Render(SkCanvas* canvas);
    
    std::string GetValue() const { return value_; }
    void SetValue(const std::string& value);
    
private:
    void UpdateCursor();
    void RenderCursor(SkCanvas* canvas);
    
    Element* element_;
    std::string value_;
    int cursor_position_ = 0;  // 光标位置
    int selection_start_ = 0;  // 选择起始
    int selection_end_ = 0;    // 选择结束
    
    bool cursor_visible_ = true;
    float cursor_blink_time_ = 0.0f;
};
```

**实现步骤：**
1. ✅ 创建 TextInputWidget 类（2 小时）
2. ✅ 实现键盘输入处理（2 小时）
3. ✅ 实现光标渲染和闪烁（2 小时）
4. ✅ 集成到 Element（1 小时）
5. ✅ 测试（1 小时）

---

### ✅ **任务 3.2：实现按钮点击**
**优先级：** 🔴 最高  
**预计时间：** 2-3 小时

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/Elements/InputTypeButton.cpp
void InputTypeButton::ProcessDefaultAction(Event& event) {
    if (event == EventId::Click && !element->IsDisabled()) {
        // 按钮被点击
    }
}
```

**MBink 实现：**
```cpp
// core/dom/element.cpp
void Element::HandleClick(Event& event) {
    if (tag_name_ == "button" || 
        (tag_name_ == "input" && GetAttribute("type") == "button")) {
        
        // 触发点击效果
        std::cout << "Button clicked: " << GetAttribute("value") << std::endl;
        
        // 可以添加回调
        if (on_click_callback_) {
            on_click_callback_(this);
        }
    }
}
```

---

### ✅ **任务 3.3：实现 Checkbox 交互**
**优先级：** 🔴 最高  
**预计时间：** 2-3 小时

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/Elements/InputTypeCheckbox.cpp:61
void InputTypeCheckbox::ProcessDefaultAction(Event& event) {
    if (event == EventId::Click && !element->IsDisabled()) {
        if (element->HasAttribute("checked"))
            element->RemoveAttribute("checked");
        else
            element->SetAttribute("checked", "");
    }
}
```

**MBink 实现：**
```cpp
// core/dom/element.cpp
void Element::HandleClick(Event& event) {
    if (tag_name_ == "input" && GetAttribute("type") == "checkbox") {
        // 切换选中状态
        if (HasAttribute("checked")) {
            RemoveAttribute("checked");
        } else {
            SetAttribute("checked", "true");
        }
        
        // 触发 change 事件
        Event change_event(EventType::CHANGE, this);
        DispatchEvent(change_event);
        
        // 请求重绘
        RequestRepaint();
    }
}

// core/render/render_object.cpp - 渲染勾选标记
void RenderObject::Paint(SkCanvas* canvas) {
    // ... 现有渲染代码 ...
    
    // 如果是选中的 checkbox，绘制勾选标记
    auto node = GetNode();
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        if (element->GetTagName() == "input" && 
            element->GetAttribute("type") == "checkbox" &&
            element->HasAttribute("checked")) {
            
            // 绘制勾选标记（✓）
            SkPaint check_paint;
            check_paint.setColor(SK_ColorBLACK);
            check_paint.setStrokeWidth(2);
            check_paint.setStyle(SkPaint::kStroke_Style);
            check_paint.setAntiAlias(true);
            
            float cx = layout_info_.x + layout_info_.width / 2;
            float cy = layout_info_.y + layout_info_.height / 2;
            
            // 绘制勾选路径
            SkPath check_path;
            check_path.moveTo(cx - 4, cy);
            check_path.lineTo(cx - 1, cy + 3);
            check_path.lineTo(cx + 4, cy - 3);
            canvas->drawPath(check_path, check_paint);
        }
    }
}
```

---

### ✅ **任务 3.4：实现 Radio 交互**
**优先级：** 🔴 最高  
**预计时间：** 2-3 小时

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/Elements/InputTypeRadio.cpp
// Radio 需要同组互斥逻辑
```

**MBink 实现：**
```cpp
void Element::HandleClick(Event& event) {
    if (tag_name_ == "input" && GetAttribute("type") == "radio") {
        std::string name = GetAttribute("name");
        
        // 取消同组其他 radio
        if (!name.empty()) {
            auto root = GetRootElement();
            root->ForEachDescendant([&](Element* elem) {
                if (elem->GetTagName() == "input" &&
                    elem->GetAttribute("type") == "radio" &&
                    elem->GetAttribute("name") == name) {
                    elem->RemoveAttribute("checked");
                }
            });
        }
        
        // 选中当前 radio
        SetAttribute("checked", "true");
        
        // 触发 change 事件
        Event change_event(EventType::CHANGE, this);
        DispatchEvent(change_event);
        
        RequestRepaint();
    }
}

// 渲染选中的圆点
if (element->GetAttribute("type") == "radio" &&
    element->HasAttribute("checked")) {
    
    SkPaint dot_paint;
    dot_paint.setColor(SK_ColorBLACK);
    dot_paint.setStyle(SkPaint::kFill_Style);
    dot_paint.setAntiAlias(true);
    
    float cx = layout_info_.x + layout_info_.width / 2;
    float cy = layout_info_.y + layout_info_.height / 2;
    
    canvas->drawCircle(cx, cy, 4, dot_paint);
}
```

---

## 📐 **阶段 4：布局引擎改进（1-2 周）**

### ✅ **任务 4.1：实现真正的 inline-block**
**优先级：** 🔴 最高  
**预计时间：** 4-6 小时

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/Layout/InlineLevelBox.cpp
// InlineLevelBox_Atomic - 代表 inline-block 元素
// 特点：
// 1. 在行内水平排列
// 2. 可以设置宽高
// 3. 宽度为 shrink-to-fit（如果未指定）
```

**MBink 实现：**

创建 `RenderInlineBlock` 类：

```cpp
// core/render/render_inline_block.h
class RenderInlineBlock : public RenderObject {
public:
    RenderInlineBlock(std::shared_ptr<Node> node);
    
    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;
    
    RenderObjectType GetType() const override { 
        return RenderObjectType::INLINE_BLOCK; 
    }
    
private:
    float CalculateShrinkToFitWidth(float available_width);
};

// core/render/render_inline_block.cpp
void RenderInlineBlock::Layout(float parent_width, float parent_height) {
    auto& style = GetComputedStyle();
    
    // 计算宽度
    if (style.width.unit != CSSUnit::AUTO) {
        layout_info_.width = style.width.ToPx();
    } else {
        // Shrink-to-fit: min(max(preferred minimum width, available width), preferred width)
        layout_info_.width = CalculateShrinkToFitWidth(parent_width);
    }
    
    // 计算高度
    if (style.height.unit != CSSUnit::AUTO) {
        layout_info_.height = style.height.ToPx();
    } else {
        // 根据内容计算高度
        layout_info_.height = CalculateContentHeight();
    }
    
    // 布局子元素
    LayoutChildren();
}
```

**修改行内布局：**
```cpp
// core/render/render_inline.cpp
void RenderInline::LayoutChildren() {
    float x = layout_info_.x + padding_left;
    float y = layout_info_.y + padding_top;
    float line_height = 0;
    float line_width = 0;
    
    for (auto& child : children_) {
        child->Layout(layout_info_.width, layout_info_.height);
        
        // inline-block 元素可以在行内排列
        if (child->GetType() == RenderObjectType::INLINE_BLOCK ||
            child->GetType() == RenderObjectType::INLINE) {
            
            // 检查是否需要换行
            if (x + child->GetLayoutInfo().width > layout_info_.x + layout_info_.width) {
                x = layout_info_.x + padding_left;
                y += line_height;
                line_height = 0;
            }
            
            child->SetPosition(x, y);
            x += child->GetLayoutInfo().width;
            line_height = std::max(line_height, child->GetLayoutInfo().height);
        }
    }
}
```

---

### ✅ **任务 4.2：实现表格布局**
**优先级：** 🟡 中等  
**预计时间：** 8-12 小时

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/Layout/TableFormattingContext.cpp
// 表格布局算法：
// 1. 确定列宽（DetermineColumnWidths）
// 2. 初始化单元格盒子（InitializeCellBoxes）
// 3. 确定行高（DetermineRowHeights）
// 4. 格式化行（FormatRows）
// 5. 格式化列（FormatColumns）
// 6. 格式化单元格（FormatCells）
```

**MBink 简化实现：**

```cpp
// core/render/render_table.h
class RenderTable : public RenderBlock {
public:
    void Layout(float parent_width, float parent_height) override;
    
private:
    struct Column {
        float width = 0;
        float min_width = 0;
        float max_width = 0;
    };
    
    struct Row {
        float height = 0;
        std::vector<RenderObject*> cells;
    };
    
    void CalculateColumnWidths(std::vector<Column>& columns);
    void CalculateRowHeights(std::vector<Row>& rows);
    void PositionCells(const std::vector<Column>& columns, 
                      const std::vector<Row>& rows);
    
    std::vector<RenderObject*> rows_;  // <tr> 元素
};

// core/render/render_table.cpp
void RenderTable::Layout(float parent_width, float parent_height) {
    // 1. 收集所有行
    CollectRows();
    
    // 2. 计算列宽
    std::vector<Column> columns;
    CalculateColumnWidths(columns);
    
    // 3. 计算行高
    std::vector<Row> rows;
    CalculateRowHeights(rows);
    
    // 4. 定位单元格
    PositionCells(columns, rows);
}

void RenderTable::CalculateColumnWidths(std::vector<Column>& columns) {
    // 简化算法：平均分配宽度
    int num_columns = GetMaxColumnsInRow();
    float available_width = layout_info_.width - padding_left - padding_right;
    float column_width = available_width / num_columns;
    
    columns.resize(num_columns);
    for (auto& col : columns) {
        col.width = column_width;
    }
}

void RenderTable::PositionCells(const std::vector<Column>& columns, 
                                const std::vector<Row>& rows) {
    float y = layout_info_.y + padding_top;
    
    for (size_t row_idx = 0; row_idx < rows.size(); row_idx++) {
        float x = layout_info_.x + padding_left;
        
        for (size_t col_idx = 0; col_idx < rows[row_idx].cells.size(); col_idx++) {
            auto cell = rows[row_idx].cells[col_idx];
            cell->SetPosition(x, y);
            cell->SetSize(columns[col_idx].width, rows[row_idx].height);
            
            x += columns[col_idx].width;
        }
        
        y += rows[row_idx].height;
    }
}
```

---

## 🎨 **阶段 5：高级功能（2-4 周）**

### ✅ **任务 5.1：实现图片加载**
**优先级：** 🟡 中等  
**预计时间：** 4-6 小时

**RmlUi 参考：**
```cpp
// RmlUi/Source/Core/Elements/ElementImage.cpp
// 使用 TextureDatabase 加载和缓存图片
```

**MBink 实现：**
```cpp
// core/render/image_loader.h
class ImageLoader {
public:
    static sk_sp<SkImage> LoadImage(const std::string& url);
    
private:
    static std::unordered_map<std::string, sk_sp<SkImage>> cache_;
};

// core/render/render_image.cpp
void RenderImage::Paint(SkCanvas* canvas) {
    auto element = std::static_pointer_cast<Element>(GetNode());
    std::string src = element->GetAttribute("src");
    
    if (!image_ && !src.empty()) {
        image_ = ImageLoader::LoadImage(src);
    }
    
    if (image_) {
        SkRect dest = SkRect::MakeXYWH(
            layout_info_.x, layout_info_.y,
            layout_info_.width, layout_info_.height
        );
        canvas->drawImageRect(image_, dest, SkSamplingOptions());
    } else {
        // 显示 alt 文本或占位符
        DrawPlaceholder(canvas);
    }
}
```

---

## 📊 时间估算总结

| 阶段 | 任务数 | 预计时间 | 优先级 |
|------|--------|----------|--------|
| **阶段 1：快速视觉修复** | 5 | 0.5 小时 | 🔴 最高 |
| **阶段 2：事件系统基础** | 3 | 10-13 小时 | 🔴 最高 |
| **阶段 3：表单交互** | 4 | 12-17 小时 | 🔴 最高 |
| **阶段 4：布局引擎改进** | 2 | 12-18 小时 | 🟡 中等 |
| **阶段 5：高级功能** | 1+ | 20-40 小时 | 🟢 低 |
| **总计** | 15+ | **54-88 小时** | - |

---

## 🎯 建议的实施顺序

### **本周（第 1 周）：**
1. ✅ 阶段 1：快速视觉修复（0.5 小时）
2. ✅ 阶段 2：事件系统基础（10-13 小时）

### **下周（第 2 周）：**
3. ✅ 阶段 3：表单交互（12-17 小时）

### **第 3-4 周：**
4. ✅ 阶段 4：布局引擎改进（12-18 小时）

### **第 5-8 周：**
5. ✅ 阶段 5：高级功能（按需实现）

---

---

## 📚 RmlUi 架构学习要点

### **1. 事件系统架构**

**RmlUi 的事件流程：**
```
用户操作 (SDL/GLFW)
    ↓
Context::ProcessInput()
    ↓
EventDispatcher::DispatchEvent()
    ↓
捕获阶段 (从根到目标)
    ↓
目标阶段
    ↓
冒泡阶段 (从目标到根)
    ↓
默认动作 (ProcessDefaultAction)
```

**关键文件：**
- `EventDispatcher.cpp` - 事件分发核心
- `Context.cpp` - 输入处理和命中测试
- `Element.cpp` - 事件监听器管理

**MBink 可以简化的部分：**
- ❌ 不需要捕获阶段（只实现冒泡）
- ❌ 不需要复杂的事件优先级
- ✅ 只需要基础的鼠标和键盘事件

---

### **2. 表单控件架构**

**RmlUi 的三层架构：**

```
┌─────────────────────────────────────┐
│  ElementFormControlInput            │  ← DOM 层
│  - 管理属性 (value, checked, etc)  │
│  - 处理事件监听                     │
└──────────────┬──────────────────────┘
               │
               ↓
┌─────────────────────────────────────┐
│  InputType (抽象基类)               │  ← 类型层
│  ├─ InputTypeText                   │
│  ├─ InputTypeCheckbox               │
│  ├─ InputTypeRadio                  │
│  └─ InputTypeButton                 │
└──────────────┬──────────────────────┘
               │
               ↓
┌─────────────────────────────────────┐
│  Widget (实际控件)                  │  ← 控件层
│  ├─ WidgetTextInput                 │
│  │   ├─ WidgetTextInputSingleLine   │
│  │   └─ WidgetTextInputMultiLine    │
│  └─ WidgetDropDown                  │
└─────────────────────────────────────┘
```

**MBink 简化方案：**
```
┌─────────────────────────────────────┐
│  Element                            │  ← 直接在 Element 处理
│  - HandleClick()                    │
│  - HandleInput()                    │
│  - widget_ (可选)                   │
└─────────────────────────────────────┘
```

**关键文件：**
- `ElementFormControlInput.cpp` - 输入元素基类
- `InputTypeCheckbox.cpp` - Checkbox 逻辑（61 行，非常简单！）
- `WidgetTextInput.cpp` - 文本输入控件（1609 行，复杂）

**MBink 实现建议：**
- ✅ Checkbox/Radio：直接在 Element 中实现（简单）
- ✅ Button：直接在 Element 中实现（简单）
- ⚠️ TextInput：需要单独的 Widget 类（复杂）

---

### **3. 布局引擎架构**

**RmlUi 的 FormattingContext 架构：**

```
FormattingContext (抽象基类)
    ├─ BlockFormattingContext      ← 块级布局
    ├─ InlineFormattingContext     ← 行内布局
    ├─ FlexFormattingContext       ← Flexbox
    └─ TableFormattingContext      ← 表格布局
```

**每个 FormattingContext 负责：**
1. 创建 LayoutBox
2. 计算尺寸
3. 定位子元素
4. 处理浮动和定位

**MBink 当前状态：**
- ✅ 有基础的块级布局
- ✅ 有简单的行内布局
- ❌ 没有 FormattingContext 抽象
- ❌ 没有表格布局
- ❌ 没有 Flexbox

**MBink 改进建议：**
1. **短期（阶段 4）：** 不引入 FormattingContext，直接在 RenderTable 中实现表格布局
2. **长期：** 如果需要 Flexbox，再考虑重构为 FormattingContext 架构

---

### **4. 文本输入架构**

**RmlUi 的 WidgetTextInput 功能：**

```cpp
// 核心功能：
1. 光标管理
   - 光标位置 (cursor_position)
   - 光标闪烁 (CURSOR_BLINK_TIME = 0.7s)
   - 光标渲染

2. 文本选择
   - 选择范围 (selection_start, selection_end)
   - 鼠标拖拽选择
   - Shift+方向键选择
   - 双击选择单词

3. 文本编辑
   - 插入字符
   - 删除字符 (Backspace, Delete)
   - 剪切/复制/粘贴
   - Undo/Redo

4. 文本导航
   - 方向键移动
   - Home/End
   - Ctrl+方向键（按单词移动）
   - 鼠标点击定位

5. 滚动
   - 自动滚动到光标位置
   - 处理 overflow

6. IME 支持
   - 组合文本 (composition)
   - 下划线显示
```

**MBink 最小实现（阶段 3）：**
```cpp
// 只实现核心功能：
1. ✅ 光标位置和闪烁
2. ✅ 基础文本输入（SDL_TextInputEvent）
3. ✅ Backspace 删除
4. ❌ 暂不支持选择
5. ❌ 暂不支持剪切/复制/粘贴
6. ❌ 暂不支持 IME
```

**关键代码参考：**
```cpp
// RmlUi/Source/Core/Elements/WidgetTextInput.cpp:53
static constexpr float CURSOR_BLINK_TIME = 0.7f;  // 光标闪烁周期

// 光标渲染 (WidgetTextInput.cpp:1450)
void WidgetTextInput::GenerateCursor() {
    // 绘制一条垂直线
    const float cursor_width = 1.0f;
    const float cursor_height = line_height;

    // 使用 text_color
    Geometry geometry = MakeGeometry(cursor_width, cursor_height, text_color);
}

// 文本输入处理 (WidgetTextInput.cpp:800)
void WidgetTextInput::OnTextInput(const String& text) {
    // 在光标位置插入文本
    value.insert(cursor_position, text);
    cursor_position += text.size();

    // 触发 input 事件
    element->DispatchEvent(EventId::Input);
}
```

---

## 🔧 实现技巧和注意事项

### **1. 事件系统实现技巧**

**命中测试优化：**
```cpp
// 从后往前遍历（后面的元素在上层）
for (int i = children.size() - 1; i >= 0; i--) {
    if (Element* hit = children[i]->HitTest(x, y)) {
        return hit;
    }
}
```

**事件冒泡：**
```cpp
bool Element::DispatchEvent(Event& event) {
    event.SetCurrentTarget(this);

    // 调用监听器
    for (auto& listener : event_listeners_[event.GetType()]) {
        listener->HandleEvent(event);
        if (event.IsPropagationStopped()) {
            return !event.IsDefaultPrevented();
        }
    }

    // 冒泡到父元素
    if (parent_ && !event.IsPropagationStopped()) {
        return parent_->DispatchEvent(event);
    }

    return !event.IsDefaultPrevented();
}
```

---

### **2. 文本输入实现技巧**

**光标闪烁：**
```cpp
void TextInputWidget::Update(float delta_time) {
    cursor_blink_time_ += delta_time;
    if (cursor_blink_time_ >= CURSOR_BLINK_TIME) {
        cursor_blink_time_ = 0;
        cursor_visible_ = !cursor_visible_;
        RequestRepaint();
    }
}

void TextInputWidget::RenderCursor(SkCanvas* canvas) {
    if (!cursor_visible_) return;

    // 计算光标位置
    float cursor_x = CalculateCursorX();

    SkPaint cursor_paint;
    cursor_paint.setColor(SK_ColorBLACK);
    cursor_paint.setStrokeWidth(1);

    canvas->drawLine(
        cursor_x, layout_info_.y + 2,
        cursor_x, layout_info_.y + layout_info_.height - 2,
        cursor_paint
    );
}
```

**SDL 文本输入：**
```cpp
// 启用文本输入
SDL_StartTextInput();

// 处理文本输入事件
case SDL_EVENT_TEXT_INPUT:
    if (focused_element_ && focused_element_->GetWidget()) {
        focused_element_->GetWidget()->HandleTextInput(event.text.text);
    }
    break;

// 处理键盘事件
case SDL_EVENT_KEY_DOWN:
    if (event.key.keysym.sym == SDLK_BACKSPACE) {
        // 删除光标前的字符
    }
    break;
```

---

### **3. 表格布局实现技巧**

**简化的列宽计算：**
```cpp
// 方案 1：平均分配
float column_width = table_width / num_columns;

// 方案 2：根据第一行的单元格宽度
for (auto& cell : first_row_cells) {
    if (cell->HasFixedWidth()) {
        columns[i].width = cell->GetWidth();
    } else {
        columns[i].width = -1;  // 标记为自动
    }
}

// 将剩余宽度平均分配给自动列
float remaining_width = table_width - fixed_width_sum;
float auto_column_width = remaining_width / num_auto_columns;
```

**单元格定位：**
```cpp
float y = table_y;
for (auto& row : rows) {
    float x = table_x;
    for (size_t col = 0; col < row.cells.size(); col++) {
        row.cells[col]->SetPosition(x, y);
        row.cells[col]->SetSize(columns[col].width, row.height);
        x += columns[col].width;
    }
    y += row.height;
}
```

---

## 🎯 快速开始指南

### **立即执行（30 分钟）：**

```bash
# 1. 修复按钮水平排列（临时方案）
# 编辑 core/render/style_resolver.cpp
# 将按钮的 display 改为 INLINE

# 2. 增强阴影效果
# 修改 shadow 参数：offset_y: 4, blur: 8, alpha: 80

# 3. 添加等宽字体、blockquote 边框、表格边框

# 4. 编译测试
cmake --build build --config Debug --target html_window_example -j8
./build/bin/Debug/html_window_example.exe
```

### **本周完成（10-13 小时）：**

```bash
# 1. 创建事件系统文件结构
mkdir -p core/event
touch core/event/{event.h,event.cpp,event_dispatcher.h,event_dispatcher.cpp}

# 2. 实现基础事件类
# 参考 COMPREHENSIVE_FIX_PLAN.md 中的代码

# 3. 集成到 Window 和 Element

# 4. 测试鼠标点击和焦点
```

---

## 📖 参考资源

### **RmlUi 关键文件清单：**

**事件系统：**
- `EventDispatcher.h/cpp` - 事件分发器（286 行）
- `Event.h/cpp` - 事件类
- `Context.cpp::ProcessMouseMove()` - 鼠标处理示例

**表单控件：**
- `InputTypeCheckbox.cpp` - Checkbox 实现（82 行，简单！）
- `InputTypeRadio.cpp` - Radio 实现（类似 Checkbox）
- `InputTypeButton.cpp` - 按钮实现（简单）
- `WidgetTextInput.cpp` - 文本输入（1609 行，复杂）

**布局引擎：**
- `InlineContainer.cpp` - 行内布局（317 行）
- `TableFormattingContext.cpp` - 表格布局（495 行）
- `InlineLevelBox.cpp` - inline-block 实现

**渲染：**
- `GeometryBoxShadow.cpp` - 阴影渲染
- `ElementBackgroundBorder.cpp` - 背景和边框

---

## ✅ 验收标准

### **阶段 1 完成标准：**
- ✅ 按钮水平排列（不占满整行）
- ✅ 阴影清晰可见
- ✅ 代码使用等宽字体
- ✅ Blockquote 有左边框
- ✅ 表格有边框

### **阶段 2 完成标准：**
- ✅ 点击按钮有控制台输出
- ✅ 点击输入框可以获得焦点
- ✅ 焦点元素有视觉指示（蓝色边框）
- ✅ Tab 键可以切换焦点

### **阶段 3 完成标准：**
- ✅ 输入框可以输入文字
- ✅ 光标闪烁可见
- ✅ Backspace 可以删除字符
- ✅ Checkbox 可以点击切换
- ✅ Checkbox 选中时显示勾选标记
- ✅ Radio 可以点击选中
- ✅ 同组 Radio 互斥

### **阶段 4 完成标准：**
- ✅ 按钮真正水平排列（inline-block）
- ✅ 表格单元格正确对齐
- ✅ 表格列宽合理分配

---

## 🚨 常见陷阱和解决方案

### **陷阱 1：事件冒泡导致重复触发**
**问题：** 点击按钮时，父元素也收到点击事件
**解决：** 在按钮的点击处理中调用 `event.StopPropagation()`

### **陷阱 2：光标位置计算错误**
**问题：** 光标位置不在正确的字符后面
**解决：** 使用 `SkFont::measureText()` 精确计算每个字符的宽度

### **陷阱 3：焦点丢失**
**问题：** 点击其他地方后，输入框失去焦点但光标仍然显示
**解决：** 在 `BLUR` 事件中隐藏光标

### **陷阱 4：表格列宽不均**
**问题：** 某些列太宽，某些列太窄
**解决：** 先计算所有单元格的最小/最大宽度，再分配

---

**文档结束 - 准备开始实施！** 🚀

