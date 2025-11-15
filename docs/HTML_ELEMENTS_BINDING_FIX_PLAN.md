# 特殊 HTML 元素 JavaScript 绑定修复计划

## 📋 问题概述

**当前问题**：
- window_demo 应用中，TODO List 的 ADD 按钮点击后无法添加项目
- 错误信息：`TypeError: cannot read property 'trim' of undefined`
- 根本原因：`HTMLInputElement.value` 属性没有绑定到 JavaScript

**影响范围**：
- 所有特殊 HTML 元素（HTMLInputElement, HTMLTextAreaElement, HTMLSelectElement 等）的特殊属性都没有 JavaScript 绑定
- JavaScript 只能访问基础 Element 类的属性（id, className, textContent 等）
- 无法通过 JavaScript 读取或设置表单元素的 value、checked 等特殊属性

---

## 🎯 修复目标

为所有特殊 HTML 元素添加完整的 JavaScript 属性和方法绑定，使其符合 W3C 标准，确保 JavaScript 可以正确访问和操作这些元素的特殊属性。

---

## 📊 当前架构分析

### 1. DOM 类继承结构

```
Node (基类)
  └── Element (元素基类)
        ├── HTMLInputElement (input 元素)
        ├── HTMLTextAreaElement (textarea 元素)
        ├── HTMLSelectElement (select 元素)
        ├── HTMLOptionElement (option 元素)
        ├── HTMLButtonElement (button 元素)
        ├── HTMLFormElement (form 元素)
        ├── HTMLAnchorElement (a 元素)
        ├── HTMLImageElement (img 元素)
        ├── HTMLLabelElement (label 元素)
        └── ... (其他元素)
```

### 2. JavaScript 绑定现状

**已绑定的属性（Element 基类）**：
- ✅ `tagName`, `id`, `className`, `textContent`
- ✅ `parentNode`, `children`, `childNodes`
- ✅ `innerHTML`, `outerHTML`
- ✅ `classList`, `style`, `dataset`

**未绑定的属性（特殊元素）**：
- ❌ `HTMLInputElement.value`, `checked`, `type`, `placeholder`, `disabled`, `readonly`, `required`, `maxLength`
- ❌ `HTMLTextAreaElement.value`, `placeholder`, `disabled`, `readonly`, `required`, `maxLength`, `rows`, `cols`
- ❌ `HTMLSelectElement.value`, `selectedIndex`, `options`, `disabled`, `multiple`, `size`
- ❌ `HTMLOptionElement.value`, `text`, `selected`, `disabled`, `index`
- ❌ `HTMLButtonElement.type`, `disabled`, `value`, `name`
- ❌ `HTMLFormElement.elements`, `length`, `name`, `method`, `action`, `submit()`, `reset()`
- ❌ `HTMLAnchorElement.href`, `target`
- ❌ `HTMLImageElement.src`, `alt`, `width`, `height`
- ❌ `HTMLLabelElement.htmlFor`

### 3. 绑定实现方式

**当前方式**：所有元素都使用同一个 `element_class_id`，通过 `js_element_proto_funcs` 数组定义属性和方法。

**问题**：无法为不同类型的元素添加不同的属性。

**解决方案**：
1. **方案 A（推荐）**：在 Element 的属性 getter/setter 中使用 `dynamic_pointer_cast` 检查元素类型，根据类型返回不同的值
2. **方案 B**：为每个特殊元素创建独立的 JSClassID 和原型（工作量大，不推荐）

---

## 🔧 修复计划

### Phase 1: HTMLInputElement 绑定 ⏳ IN PROGRESS

**目标**：为 `<input>` 元素添加所有特殊属性的 JavaScript 绑定

**需要绑定的属性**：

| 属性 | 类型 | 读写 | C++ 方法 | 优先级 |
|------|------|------|----------|--------|
| `value` | string | R/W | `GetValue()`, `SetValue()` | 🔴 高 |
| `checked` | boolean | R/W | `GetChecked()`, `SetChecked()` | 🔴 高 |
| `type` | string | R/W | `GetInputType()`, `SetInputType()` | 🟡 中 |
| `placeholder` | string | R/W | `GetPlaceholder()`, `SetPlaceholder()` | 🟡 中 |
| `disabled` | boolean | R/W | `GetDisabled()`, `SetDisabled()` | 🟡 中 |
| `readonly` | boolean | R/W | `GetReadOnly()`, `SetReadOnly()` | 🟢 低 |
| `required` | boolean | R/W | `GetRequired()`, `SetRequired()` | 🟢 低 |
| `maxLength` | number | R/W | `GetMaxLength()`, `SetMaxLength()` | 🟢 低 |

**实现步骤**：
1. ✅ 添加 `js_element_get_value()` 和 `js_element_set_value()` 函数
2. ✅ 添加 `js_element_get_checked()` 和 `js_element_set_checked()` 函数
3. ⏳ 在 `js_element_proto_funcs` 数组中注册这些属性
4. ⏳ 编译测试，验证 TODO List ADD 按钮功能

**代码位置**：`core/dom/dom_bindings.cpp` (已添加 getter/setter，待注册)

---

### Phase 2: HTMLTextAreaElement 绑定

**目标**：为 `<textarea>` 元素添加所有特殊属性的 JavaScript 绑定

**需要绑定的属性**：

| 属性 | 类型 | 读写 | C++ 方法 | 优先级 |
|------|------|------|----------|--------|
| `value` | string | R/W | `GetValue()`, `SetValue()` | 🔴 高 |
| `placeholder` | string | R/W | `GetPlaceholder()`, `SetPlaceholder()` | 🟡 中 |
| `disabled` | boolean | R/W | `GetDisabled()`, `SetDisabled()` | 🟡 中 |
| `readonly` | boolean | R/W | `GetReadOnly()`, `SetReadOnly()` | 🟢 低 |
| `required` | boolean | R/W | `GetRequired()`, `SetRequired()` | 🟢 低 |
| `maxLength` | number | R/W | `GetMaxLength()`, `SetMaxLength()` | 🟢 低 |
| `rows` | number | R/W | `GetRows()`, `SetRows()` | 🟢 低 |
| `cols` | number | R/W | `GetCols()`, `SetCols()` | 🟢 低 |

**实现步骤**：
1. 修改 `js_element_get_value()` 支持 textarea 元素（使用 `HTMLTextAreaElement::GetValue()`）
2. 添加其他属性的 getter/setter
3. 测试验证

---

### Phase 3: HTMLSelectElement 绑定

**目标**：为 `<select>` 元素添加所有特殊属性的 JavaScript 绑定

**需要绑定的属性**：

| 属性 | 类型 | 读写 | C++ 方法 | 优先级 |
|------|------|------|----------|--------|
| `value` | string | R/W | `GetValue()`, `SetValue()` | 🔴 高 |
| `selectedIndex` | number | R/W | `GetSelectedIndex()`, `SetSelectedIndex()` | 🔴 高 |
| `options` | HTMLOptionsCollection | R | `GetOptions()` | 🟡 中 |
| `disabled` | boolean | R/W | `GetDisabled()`, `SetDisabled()` | 🟡 中 |
| `multiple` | boolean | R/W | `GetMultiple()`, `SetMultiple()` | 🟢 低 |
| `size` | number | R/W | `GetSize()`, `SetSize()` | 🟢 低 |

**实现步骤**：
1. 添加 `js_element_get_selected_index()` 和 `js_element_set_selected_index()`
2. 添加 `js_element_get_options()` 返回 options 数组
3. 添加其他属性的 getter/setter
4. 测试验证

---

### Phase 4: HTMLOptionElement 绑定

**目标**：为 `<option>` 元素添加所有特殊属性的 JavaScript 绑定

**需要绑定的属性**：

| 属性 | 类型 | 读写 | C++ 方法 | 优先级 |
|------|------|------|----------|--------|
| `value` | string | R/W | `GetValue()`, `SetValue()` | 🔴 高 |
| `text` | string | R/W | `GetText()`, `SetText()` | 🔴 高 |
| `selected` | boolean | R/W | `GetSelected()`, `SetSelected()` | 🔴 高 |
| `disabled` | boolean | R/W | `GetDisabled()`, `SetDisabled()` | 🟡 中 |
| `index` | number | R | `GetIndex()` | 🟢 低 |

---

### Phase 5: HTMLButtonElement 绑定

**目标**：为 `<button>` 元素添加所有特殊属性的 JavaScript 绑定

**需要绑定的属性**：

| 属性 | 类型 | 读写 | C++ 方法 | 优先级 |
|------|------|------|----------|--------|
| `type` | string | R/W | `GetType()`, `SetType()` | 🟡 中 |
| `disabled` | boolean | R/W | `GetDisabled()`, `SetDisabled()` | 🟡 中 |
| `value` | string | R/W | `GetValue()`, `SetValue()` | 🟢 低 |
| `name` | string | R/W | `GetName()`, `SetName()` | 🟢 低 |

---

### Phase 6: HTMLFormElement 绑定

**目标**：为 `<form>` 元素添加所有特殊属性和方法的 JavaScript 绑定

**需要绑定的属性和方法**：

| 属性/方法 | 类型 | 读写 | C++ 方法 | 优先级 |
|-----------|------|------|----------|--------|
| `elements` | HTMLFormControlsCollection | R | `GetElements()` | 🟡 中 |
| `length` | number | R | `GetLength()` | 🟡 中 |
| `name` | string | R/W | `GetName()`, `SetName()` | 🟢 低 |
| `method` | string | R/W | `GetMethod()`, `SetMethod()` | 🟢 低 |
| `action` | string | R/W | `GetAction()`, `SetAction()` | 🟢 低 |
| `submit()` | method | - | `Submit()` | 🟢 低 |
| `reset()` | method | - | `Reset()` | 🟢 低 |

---

### Phase 7: 其他元素绑定

**HTMLAnchorElement** (`<a>`):
- `href` (string, R/W)
- `target` (string, R/W)

**HTMLImageElement** (`<img>`):
- `src` (string, R/W)
- `alt` (string, R/W)
- `width` (number, R/W)
- `height` (number, R/W)

**HTMLLabelElement** (`<label>`):
- `htmlFor` (string, R/W) - 对应 `for` 属性

---

### Phase 8: 输入框 focus 样式修复

**目标**：实现输入框聚焦时的边框高亮效果

**问题**：
- FocusManager 已经正确设置了 `:focus` 伪类
- 但 StyleResolver 没有处理 `:focus` 伪类样式（只有 `:focus-visible`）

**解决方案**：
在 `core/render/style_resolver.cpp` 的 `ApplyPseudoClassStyles()` 方法中添加 `:focus` 伪类处理：

```cpp
// ========== :focus 伪类样式 ==========
if (element->HasPseudoClass("focus")) {
    if (tag_name == "input" || tag_name == "textarea") {
        // 焦点样式：改变边框颜色
        style.border.color = SkColorSetRGB(102, 126, 234);  // #667eea
        style.border.width = CSSLength(1, CSSUnit::PX);
    }
}
```

**代码位置**：`core/render/style_resolver.cpp` (约 line 875)

---

### Phase 9: 光标闪烁效果实现

**目标**：为聚焦的输入框实现光标闪烁效果

**实现方案**：
1. 在 `HTMLInputElement` 和 `HTMLTextAreaElement` 中添加光标位置信息
2. 在 render layer 中添加光标绘制逻辑
3. 使用定时器实现光标闪烁（500ms 间隔）

**优先级**：低（可以在后续版本实现）

---

### Phase 10: 测试和验证

**测试项目**：
1. ✅ Timer 功能（已验证正常）
2. ✅ Checkbox 切换（已验证正常）
3. ⏳ TODO List ADD 按钮（待验证）
4. ⏳ 输入框 value 读取和设置
5. ⏳ 输入框 focus 样式
6. ⏳ 下拉选择框 value 和 selectedIndex
7. ⏳ 所有表单元素的 disabled 属性

---

## 📝 实现注意事项

### 1. 类型转换安全性

使用 `std::dynamic_pointer_cast` 进行类型转换，并检查返回值：

```cpp
auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
if (input) {
    return JS_NewString(ctx, input->GetValue().c_str());
}
```

### 2. 事件触发控制

设置属性时，通常不触发事件（`trigger_events = false`），因为这是程序化设置：

```cpp
input->SetValue(value, false);  // 不触发 input/change 事件
```

### 3. 属性同步

某些属性需要同步到 HTML 属性（attribute）和 IDL 属性（property）：
- `value` 属性：IDL 属性是当前值，HTML 属性是默认值
- `checked` 属性：IDL 属性是当前状态，HTML 属性是默认状态

### 4. 性能考虑

- 使用 `tagName` 字符串比较来判断元素类型（简单高效）
- 避免在每次属性访问时都进行复杂的类型检查

---

## 🎯 下一步行动

1. **立即执行**：完成 Phase 1（HTMLInputElement 绑定），在 `js_element_proto_funcs` 中注册 `value` 和 `checked` 属性
2. **编译测试**：验证 TODO List ADD 按钮功能是否正常
3. **修复 focus 样式**：执行 Phase 8，添加 `:focus` 伪类样式处理
4. **逐步完成**：按优先级依次完成 Phase 2-7
5. **全面测试**：执行 Phase 10，确保所有功能正常

---

## 📊 进度跟踪

- [x] Phase 0: 问题分析和计划制定
- [/] Phase 1: HTMLInputElement 绑定（进行中）
- [ ] Phase 2: HTMLTextAreaElement 绑定
- [ ] Phase 3: HTMLSelectElement 绑定
- [ ] Phase 4: HTMLOptionElement 绑定
- [ ] Phase 5: HTMLButtonElement 绑定
- [ ] Phase 6: HTMLFormElement 绑定
- [ ] Phase 7: 其他元素绑定
- [ ] Phase 8: 输入框 focus 样式修复
- [ ] Phase 9: 光标闪烁效果实现
- [ ] Phase 10: 测试和验证

---

**文档创建时间**：2025-11-15  
**最后更新时间**：2025-11-15  
**状态**：Phase 1 进行中

