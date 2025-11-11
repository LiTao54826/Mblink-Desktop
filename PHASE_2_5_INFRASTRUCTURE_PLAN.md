# Phase 2.5: JavaScript基础设施完善 - 实施计划

> **创建日期**: 2025-11-11  
> **参考项目**: RmlUi (ReferenceProject/RmlUi)  
> **目标**: 完成所有JavaScript基础设施，使MBink能够开发完整的交互式应用  
> **预计时间**: 3-4周

---

## 🎯 总体目标

基于RmlUi的成熟实现，完善MBink的事件系统、DOM API和CSS功能，达到以下目标：

1. ✅ **完整的事件系统** - 鼠标、键盘、焦点、拖拽事件
2. ✅ **完善的DOM API** - 查询选择器、属性操作、DOM操作
3. ✅ **CSS伪类支持** - :hover, :active, :focus, :drag
4. ✅ **焦点管理** - Tab导航、焦点事件
5. ✅ **拖拽系统** - 完整的拖拽生命周期

---

## 📋 任务清单

### P0: 核心事件系统（必须完成，1周）

#### Task 1: 完善鼠标事件系统 ✅ (已有基础)
**当前状态**: 
- ✅ HitTesting已实现
- ✅ MouseEvent类已实现
- ✅ 基础事件分发已实现

**需要完善**:
- [ ] 参考RmlUi实现鼠标坐标投影（支持transform）
- [ ] 实现mouseover/mouseout事件
- [ ] 实现mouseenter/mouseleave事件（不冒泡）
- [ ] 实现dblclick事件

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/Context.cpp` (ProcessMouseMove, ProcessMouseButtonDown)
- `ReferenceProject/RmlUi/Source/Core/Event.cpp` (ProjectMouse)

#### Task 2: 完善JavaScript事件绑定 ✅ (已有基础)
**当前状态**:
- ✅ addEventListener已实现（core/dom/dom_bindings.cpp）
- ⚠️ removeEventListener未完全实现

**需要完善**:
- [ ] 实现removeEventListener（需要listener ID机制）
- [ ] 实现事件捕获阶段（useCapture参数）
- [ ] 实现once选项（addEventListener第三个参数）
- [ ] 绑定Event对象到JavaScript（clientX, clientY, target, currentTarget等）

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/Element.cpp` (AddEventListener, RemoveEventListener)
- `ReferenceProject/RmlUi/Include/RmlUi/Core/EventListener.h`

---

### P1: DOM API完善（重要，1周）

#### Task 3: 查询选择器 ✅ (已有基础)
**当前状态**:
- ✅ SelectorEngine已实现（简单版本）
- ✅ querySelector/querySelectorAll已实现
- ⚠️ 仅支持简单选择器（#id, .class, tag, [attr]）

**需要完善**:
- [ ] 集成Lexbor CSS选择器引擎
- [ ] 支持复杂选择器（后代、子、相邻、兄弟）
- [ ] 支持伪类选择器（:hover, :active, :focus, :nth-child等）
- [ ] 实现matches()方法
- [ ] 实现closest()方法

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/ElementUtilities.cpp` (QuerySelector相关)
- `core/lexbor/lexbor_document.h` (Lexbor集成)

#### Task 4: 元素属性和样式操作
**需要实现**:
- [ ] setAttribute/getAttribute/removeAttribute/hasAttribute
- [ ] classList.add/remove/toggle/contains
- [ ] style.setProperty/getPropertyValue/removeProperty
- [ ] dataset属性（data-*）
- [ ] 绑定到JavaScript

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/Element.cpp` (SetAttribute, GetAttribute)
- `core/dom/element.h` (已有部分实现)

#### Task 5: DOM操作API
**需要实现**:
- [ ] appendChild/removeChild/insertBefore
- [ ] replaceChild
- [ ] cloneNode（深拷贝/浅拷贝）
- [ ] innerHTML/outerHTML/textContent
- [ ] 绑定到JavaScript

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/Element.cpp` (AppendChild, RemoveChild)
- `core/dom/element.cpp` (已有部分实现)

---

### P2: CSS伪类和焦点管理（重要，1周）

#### Task 6: CSS伪类支持 ⭐⭐⭐⭐⭐
**需要实现**:
- [ ] :hover伪类（鼠标悬停时自动设置）
- [ ] :active伪类（鼠标按下时自动设置）
- [ ] :focus伪类（元素获得焦点时自动设置）
- [ ] :focus-visible伪类（键盘导航时显示）
- [ ] :drag伪类（拖拽时自动设置）
- [ ] SetPseudoClass/HasPseudoClass方法
- [ ] 伪类变化时触发样式重新计算

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/Element.cpp` (ProcessDefaultAction - 设置伪类)
- `ReferenceProject/RmlUi/Source/Core/Context.cpp` (鼠标事件中设置:hover)

**实现要点**:
```cpp
// 在鼠标事件中自动设置伪类
void Element::ProcessDefaultAction(Event& event) {
    if (event == EventId::Mouseover) 
        SetPseudoClass("hover", true);
    if (event == EventId::Mouseout) 
        SetPseudoClass("hover", false);
    if (event == EventId::Mousedown) 
        SetPseudoClass("active", true);
    if (event == EventId::Mouseup) 
        SetPseudoClass("active", false);
    if (event == EventId::Focus) 
        SetPseudoClass("focus", true);
    if (event == EventId::Blur) 
        SetPseudoClass("focus", false);
}
```

#### Task 7: 焦点管理系统 ⭐⭐⭐⭐⭐
**需要实现**:
- [ ] FocusManager类（管理全局焦点）
- [ ] Focus()/Blur()方法
- [ ] Tab键导航（tabindex支持）
- [ ] focus/blur事件
- [ ] focusin/focusout事件（冒泡版本）
- [ ] autofocus属性支持

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/Element.cpp` (Focus, Blur)
- `ReferenceProject/RmlUi/Source/Core/Context.cpp` (GetFocusElement, ProcessKeyDown)

**实现要点**:
```cpp
class FocusManager {
public:
    void SetFocus(Element* element);
    Element* GetFocusedElement() const;
    void HandleTabKey(bool shift);  // Tab导航
    
private:
    Element* focused_element_ = nullptr;
    std::vector<Element*> GetFocusableElements();  // 按tabindex排序
};
```

---

### P3: 拖拽系统（高优先级，1-2周）⭐⭐⭐⭐⭐

#### Task 8: 完整的拖拽系统
**需要实现**:
- [ ] DragManager类（管理拖拽状态）
- [ ] 拖拽事件类型：
  - dragstart - 开始拖拽
  - drag - 拖拽中（持续触发）
  - dragend - 拖拽结束
  - dragover - 拖拽经过目标元素
  - dragenter - 拖拽进入目标元素
  - dragleave - 拖拽离开目标元素
  - drop - 放置到目标元素
- [ ] CSS drag属性支持（drag: none | block | drag | drag-drop | clone）
- [ ] 拖拽克隆支持（drag: clone）
- [ ] 拖拽数据传输（DataTransfer对象）

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/Context.cpp` (拖拽相关代码，行706-1382)
- `ReferenceProject/RmlUi/Samples/basic/drag/` (拖拽示例)
- `ReferenceProject/RmlUi/Tests/Data/VisualTests/drag.rml`

**实现要点**:
```cpp
class DragManager {
public:
    void StartDrag(Element* element, const MouseEvent& event);
    void UpdateDrag(const MouseEvent& event);
    void EndDrag(const MouseEvent& event);
    
    Element* GetDragElement() const { return drag_element_; }
    Element* GetDragHoverElement() const { return drag_hover_; }
    
private:
    Element* drag_element_ = nullptr;      // 被拖拽的元素
    Element* drag_hover_ = nullptr;        // 当前悬停的目标元素
    Element* drag_clone_ = nullptr;        // 拖拽克隆（如果使用clone模式）
    bool drag_started_ = false;
    Vector2f drag_start_position_;
    
    void CreateDragClone(Element* element);
    void ReleaseDragClone();
    void SendDragEvents();
};
```

---

### P4: 键盘事件和表单元素（可选，1周）

#### Task 9: 键盘事件系统
**需要实现**:
- [ ] KeyboardEvent类（key, code, keyCode, charCode）
- [ ] keydown/keyup/keypress事件
- [ ] 修饰键状态（ctrlKey, shiftKey, altKey, metaKey）
- [ ] 文本输入事件（textinput, input, beforeinput）
- [ ] 绑定到JavaScript

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/Context.cpp` (ProcessKeyDown, ProcessKeyUp)
- `ReferenceProject/RmlUi/Include/RmlUi/Core/Input.h` (KeyIdentifier枚举)

#### Task 10: 表单元素支持
**需要实现**:
- [ ] HTMLInputElement（text, checkbox, radio, button）
- [ ] HTMLTextAreaElement
- [ ] HTMLSelectElement
- [ ] value属性
- [ ] change/input事件
- [ ] 表单验证（required, pattern等）

**参考文件**:
- `ReferenceProject/RmlUi/Source/Core/Elements/` (ElementFormControl系列)

---

## 🔧 实施策略

### 1. 参考RmlUi的最佳实践

**事件系统**:
- ✅ 使用EventId枚举而不是字符串（性能优化）
- ✅ 支持捕获和冒泡阶段
- ✅ 事件参数使用Dictionary存储
- ✅ 事件监听器支持OnAttach/OnDetach生命周期

**焦点管理**:
- ✅ Context级别的焦点管理器
- ✅ Tab键导航按tabindex排序
- ✅ 焦点事件自动设置:focus伪类

**拖拽系统**:
- ✅ CSS drag属性控制拖拽行为
- ✅ 支持clone模式（拖拽克隆）
- ✅ 完整的拖拽事件生命周期
- ✅ 拖拽时自动设置:drag伪类

### 2. 测试驱动开发

每个Task完成后必须编写测试：

```cpp
// 示例：测试焦点管理
TEST(FocusManager, TabNavigation) {
    auto doc = CreateTestDocument();
    auto input1 = doc->CreateElement("input");
    auto input2 = doc->CreateElement("input");
    auto input3 = doc->CreateElement("input");
    
    input1->SetAttribute("tabindex", "1");
    input2->SetAttribute("tabindex", "2");
    input3->SetAttribute("tabindex", "3");
    
    doc->GetBody()->AppendChild(input1);
    doc->GetBody()->AppendChild(input2);
    doc->GetBody()->AppendChild(input3);
    
    auto focus_mgr = doc->GetFocusManager();
    focus_mgr->SetFocus(input1.get());
    EXPECT_EQ(focus_mgr->GetFocusedElement(), input1.get());
    
    focus_mgr->HandleTabKey(false);  // Tab
    EXPECT_EQ(focus_mgr->GetFocusedElement(), input2.get());
    
    focus_mgr->HandleTabKey(false);  // Tab
    EXPECT_EQ(focus_mgr->GetFocusedElement(), input3.get());
    
    focus_mgr->HandleTabKey(true);   // Shift+Tab
    EXPECT_EQ(focus_mgr->GetFocusedElement(), input2.get());
}
```

### 3. 渐进式实现

**Week 1**: P0任务（核心事件系统）
- Day 1-2: 完善鼠标事件（mouseover/mouseout/mouseenter/mouseleave）
- Day 3-4: 完善JavaScript事件绑定（removeEventListener, useCapture）
- Day 5: 测试和文档

**Week 2**: P1任务（DOM API）+ P2任务（CSS伪类）
- Day 1-2: 查询选择器（Lexbor集成）
- Day 3: 属性和样式操作
- Day 4: CSS伪类支持
- Day 5: 测试和文档

**Week 3**: P2任务（焦点管理）+ P3任务（拖拽系统）
- Day 1-2: 焦点管理系统
- Day 3-5: 拖拽系统基础

**Week 4**: P3任务（拖拽系统完善）+ P4任务（可选）
- Day 1-2: 拖拽系统完善（clone模式、DataTransfer）
- Day 3-4: 键盘事件（如果时间允许）
- Day 5: 集成测试和文档

---

## 📊 成功标准

### 功能完整性
- [ ] 所有P0和P1任务100%完成
- [ ] P2任务（焦点管理）100%完成
- [ ] P3任务（拖拽系统）至少80%完成
- [ ] 所有功能有对应的测试用例

### 测试覆盖率
- [ ] 事件系统测试覆盖率 > 90%
- [ ] DOM API测试覆盖率 > 85%
- [ ] 焦点管理测试覆盖率 > 85%
- [ ] 拖拽系统测试覆盖率 > 80%

### 示例应用
- [ ] Counter App可以正常运行
- [ ] Animation Demo可以正常运行
- [ ] 创建Todo App示例（使用焦点管理）
- [ ] 创建Drag & Drop示例（使用拖拽系统）

### React生态兼容性
- [ ] 可以使用react-dnd库（拖拽系统）
- [ ] 可以使用React表单库（焦点管理）
- [ ] 可以使用React动画库（CSS伪类）

---

## 📚 参考资源

### RmlUi核心文件
- `ReferenceProject/RmlUi/Source/Core/Context.cpp` - 事件处理主逻辑
- `ReferenceProject/RmlUi/Source/Core/Element.cpp` - 元素事件处理
- `ReferenceProject/RmlUi/Source/Core/EventDispatcher.cpp` - 事件分发
- `ReferenceProject/RmlUi/Include/RmlUi/Core/Event.h` - 事件接口

### RmlUi示例
- `ReferenceProject/RmlUi/Samples/basic/drag/` - 拖拽示例
- `ReferenceProject/RmlUi/Samples/basic/data_binding/` - 事件监听示例

### Web标准
- [DOM Level 3 Events](https://www.w3.org/TR/DOM-Level-3-Events/)
- [HTML Drag and Drop API](https://developer.mozilla.org/en-US/docs/Web/API/HTML_Drag_and_Drop_API)
- [CSS Pseudo-classes](https://developer.mozilla.org/en-US/docs/Web/CSS/Pseudo-classes)

---

**创建日期**: 2025-11-11  
**下次审查**: 每周五  
**负责人**: MBink Team

