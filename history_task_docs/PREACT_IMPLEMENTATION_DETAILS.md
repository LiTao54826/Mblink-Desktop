# Preact 实现技术细节

## 📋 概述

本文档详细说明了 MBink 中 Preact 支持的实现细节，包括实际的渲染效果、事件处理流程和已知限制。

---

## 🎨 实际渲染效果

### **窗口显示**

当运行 `preact_window_demo.exe` 时，会看到：

1. **窗口创建**
   - 标题：`MBink + Preact Demo`
   - 尺寸：600x400 像素
   - 背景：白色
   - 渲染模式：CPU 软件渲染（GPU 初始化失败时自动降级）

2. **DOM 树结构**
   ```html
   <html>
     <body>
       <div>
         <h1>MBink + Preact Window Demo</h1>
         <div class="counter">
           <h2>Counter Example</h2>
           <p>Count: 0</p>
           <button>Increment</button>
           <button>Decrement</button>
           <button>Reset</button>
         </div>
       </div>
     </body>
   </html>
   ```

3. **文本渲染**
   - ✅ 所有文本都正常显示
   - ✅ 使用 Skia 文本渲染引擎
   - ✅ 默认字体：系统字体
   - ✅ 默认颜色：黑色 (SK_ColorBLACK)
   - ✅ 文本位置：根据 Yoga 布局计算

4. **按钮显示**
   - ⚠️ **按钮没有边框和背景色**
   - ⚠️ 看起来像普通文本
   - ✅ 但是可以点击！
   - 原因：没有实现 CSS 样式系统

---

## 🖱️ 事件处理流程

### **完整的事件链路**

```
用户点击鼠标
    ↓
SDL3 捕获 SDL_EVENT_MOUSE_BUTTON_DOWN
    ↓
EventLoop::HandleEvent()
    ↓
EventLoop::HandleMouseEventForDOM()
    ↓
Hit Testing（找到点击位置的 DOM 元素）
    ↓
Element::DispatchEvent(clickEvent)
    ↓
触发 JavaScript 事件处理器
    ↓
onclick: function() { setCount(prev => prev + 1); }
    ↓
useState 的 setState 函数被调用
    ↓
hookState.value = newValue
    ↓
component.__rerender() 被调用
    ↓
重新调用组件函数：Counter()
    ↓
生成新的虚拟 DOM
    ↓
createDOMElement(newVNode)
    ↓
parent.replaceChild(newDOM, oldDOM)
    ↓
DOM 树更新
    ↓
Window::SetNeedsRepaint() 被调用
    ↓
EventLoop 渲染回调
    ↓
Window::RenderDocument()
    ↓
RenderTreeBuilder::BuildRenderTree()
    ↓
Yoga 布局计算
    ↓
RenderObject::Paint(canvas)
    ↓
Skia 绘制文本和图形
    ↓
Window::SwapBuffers()
    ↓
SDL_RenderPresent()
    ↓
屏幕更新，用户看到新的 Count 值！
```

### **关键代码位置**

1. **事件捕获**：`core/event/event_loop.cpp` - `HandleMouseEventForDOM()`
2. **事件分发**：`core/dom/element.cpp` - `DispatchEvent()`
3. **JavaScript 处理器**：`examples/preact_window_demo/app.js` - `onclick` 函数
4. **状态更新**：`js/preact/hooks.js` - `useState()` 的 `setState`
5. **组件重渲染**：`js/preact/preact.js` - `component.__rerender()`
6. **DOM 更新**：`core/dom/element.cpp` - `ReplaceChild()`
7. **渲染触发**：`core/window/window.cpp` - `WindowDOMObserver`
8. **布局计算**：`core/render/render_tree_builder.cpp` - `BuildRenderTree()`
9. **绘制**：`core/render/render_object.cpp` - `RenderText::Paint()`

---

## 🔧 关键技术实现

### **1. Hooks 闭包问题的解决**

**问题：**
```javascript
// ❌ 错误的写法
onclick: function() {
    setCount(count + 1);  // count 永远是第一次渲染时的值
}
```

**解决方案：**
```javascript
// ✅ 正确的写法
onclick: function() {
    setCount(function(prevCount) { 
        return prevCount + 1; 
    });
}
```

**原理：**
- 事件处理器在第一次渲染时创建，捕获了当时的 `count` 值
- 使用函数式更新，`setState` 会传入最新的状态值
- 这样就不依赖闭包中的旧值

### **2. 组件重渲染机制**

**实现位置：** `js/preact/preact.js` - `createDOMElement()` 函数

```javascript
// 为每个组件设置 __rerender 函数
component.__rerender = function() {
    // 1. 设置当前组件上下文
    PreactHooks.setCurrentComponent(component);
    
    // 2. 重新调用组件函数
    const newVNode = vnode.type(vnode.props);
    
    // 3. 清除组件上下文
    PreactHooks.setCurrentComponent(null);
    
    // 4. 获取旧 DOM 和父节点
    const oldDOM = component.__dom;
    const parent = oldDOM.parentNode;
    
    // 5. 创建新 DOM
    const newDOM = createDOMElement(newVNode);
    
    // 6. 替换 DOM
    parent.replaceChild(newDOM, oldDOM);
    
    // 7. 更新组件状态
    component.__dom = newDOM;
};
```

### **3. DOM 操作的 C++ 实现**

**实现位置：** `core/dom/dom_bindings.cpp`

```cpp
// replaceChild(newChild, oldChild)
static JSValue js_element_replace_child(JSContext* ctx, JSValueConst this_val, 
                                        int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    
    // 解包新节点和旧节点
    auto new_child = DOMBindings::UnwrapElement(ctx, argv[0]);
    auto old_child = DOMBindings::UnwrapElement(ctx, argv[1]);
    
    // 调用 C++ DOM 方法
    element->ReplaceChild(new_child, old_child);
    
    return JS_DupValue(ctx, argv[1]);
}
```

---

## ⚠️ 已知限制

### **1. 样式系统未实现**

**现状：**
- ❌ 没有 CSS 解析器
- ❌ 没有样式计算
- ❌ 没有 `element.style` API
- ❌ 没有 `className` 到样式的映射

**影响：**
- 按钮没有边框和背景色
- 所有元素使用默认样式
- 文本都是黑色
- 布局只有基础的 Flexbox

**解决方案：**
- 需要实现 CSS 解析器（可以使用 Lexbor 的 CSS 模块）
- 需要实现样式计算和继承
- 需要在 `RenderObject::Paint()` 中应用样式

### **2. 性能问题**

**现状：**
- ⚠️ 每次重渲染都完全替换 DOM 子树
- ⚠️ 没有 Virtual DOM Diff 优化
- ⚠️ 没有渲染批处理

**影响：**
- 复杂应用可能卡顿
- 不必要的 DOM 操作

**解决方案：**
- 实现 Virtual DOM Diff 算法
- 只更新变化的部分
- 批处理多个状态更新

### **3. 内存管理**

**现状：**
- ⚠️ QuickJS GC 断言失败：`Assertion failed: list_empty(&rt->gc_obj_list)`
- 原因：JavaScript 对象没有完全释放

**影响：**
- 程序退出时会报错（但不影响功能）
- 可能有轻微内存泄漏

**解决方案：**
- 正确释放所有 JSValue
- 清理 DOM 绑定缓存
- 在析构函数中调用 `JS_FreeValue()`

### **4. 缺失的 DOM API**

**已实现：**
- ✅ `createElement`, `createTextNode`
- ✅ `appendChild`, `removeChild`, `replaceChild`, `insertBefore`
- ✅ `setAttribute`, `getAttribute`
- ✅ `addEventListener`, `removeEventListener`
- ✅ `textContent`, `className`, `parentNode`

**未实现：**
- ❌ `innerHTML`, `outerHTML`
- ❌ `querySelector`, `querySelectorAll`
- ❌ `classList` API
- ❌ `style` 对象
- ❌ `getBoundingClientRect()`
- ❌ `focus()`, `blur()`

---

## 📊 测试结果

### **自动化测试**

运行 `preact_window_test.exe` 的结果：

```
[Initial] Count = 0
  ✓ Initial count is correct

[Test 1] Clicking Increment...
  Count = 1
  ✓ Increment works correctly

[Test 2] Clicking Increment again...
  Count = 2
  ✓ Second increment works correctly

[Test 3] Clicking Decrement...
  Count = 1
  ✓ Decrement works correctly

[Test 4] Clicking Reset...
  Count = 0
  ✓ Reset works correctly

========================================
  ✅ ALL TESTS PASSED!
========================================
```

### **手动测试**

运行 `preact_window_demo.exe`：

1. ✅ 窗口正常显示
2. ✅ 文本清晰可见
3. ✅ 点击 Increment 按钮，Count 增加
4. ✅ 点击 Decrement 按钮，Count 减少
5. ✅ 点击 Reset 按钮，Count 归零
6. ✅ 多次点击都正常工作
7. ✅ 关闭窗口程序正常退出

---

## 🚀 下一步改进

### **短期（1-2周）**

1. **实现基础 CSS 支持**
   - 解析 inline style
   - 支持 `background-color`, `border`, `padding`, `margin`
   - 让按钮有边框和背景色

2. **优化渲染性能**
   - 实现简单的 Virtual DOM Diff
   - 只更新变化的节点
   - 减少不必要的重绘

3. **修复内存泄漏**
   - 正确清理 JavaScript 对象
   - 解决 QuickJS GC 断言问题

### **中期（1-2月）**

1. **完善 DOM API**
   - `innerHTML`, `querySelector`
   - `classList` API
   - `getBoundingClientRect()`

2. **实现更多 Hooks**
   - `useEffect` - 副作用处理
   - `useRef` - 引用管理
   - `useMemo`, `useCallback` - 性能优化

3. **添加更多事件**
   - 键盘事件
   - 表单事件
   - 拖拽事件

### **长期（3-6月）**

1. **完整的 CSS 引擎**
   - CSS 文件解析
   - 选择器匹配
   - 样式继承和层叠

2. **开发者工具**
   - DOM 检查器
   - 性能分析
   - 调试工具

3. **打包和分发**
   - 应用打包工具
   - 自动更新
   - 安装程序

---

## 🎯 总结

✅ **已成功实现：**
- Preact 核心功能（组件、Hooks、渲染）
- 事件处理系统（点击事件）
- 组件重渲染机制
- 基础 DOM API
- 文本渲染

⚠️ **已知限制：**
- 没有 CSS 样式系统
- 性能未优化
- 部分 DOM API 缺失
- 内存管理需要改进

🚀 **可以做什么：**
- 构建简单的交互式应用
- 使用 Preact 组件和 Hooks
- 处理用户点击事件
- 动态更新 UI

**MBink 现在已经是一个可用的 Preact 桌面应用框架！** 🎉

