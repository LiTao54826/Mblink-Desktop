# QuickJS 模块

## 📋 概述

QuickJS 模块是 MBink 的 JavaScript 运行时核心，基于 QuickJS 引擎实现。它负责 JavaScript 代码的执行、DOM API 的 JavaScript 绑定、Preact 框架集成，以及 JavaScript 与 C++ 之间的双向通信。

## 🎯 主要功能

- **JavaScript 执行**: 高性能的 ES2020 JavaScript 引擎
- **DOM 绑定**: 完整的 DOM API JavaScript 绑定
- **Window API**: `window`、`document`、`console` 等全局对象
- **Preact 集成**: 内置 Preact 框架支持
- **事件系统**: JavaScript 事件监听器绑定
- **定时器**: `setTimeout`、`setInterval`、`requestAnimationFrame`
- **模块系统**: ES6 模块和 CommonJS 支持
- **异常处理**: JavaScript 异常捕获和报告

## 📁 文件结构

```
quickjs/
├── CMakeLists.txt                # 构建配置
├── quickjs_runtime.h/cpp         # QuickJS 运行时
├── window_bindings.h/cpp         # Window API 绑定
├── dom_bindings.h/cpp            # DOM API 绑定
├── event_bindings.h/cpp          # 事件 API 绑定
├── console_bindings.h/cpp        # Console API 绑定
├── timer_bindings.h/cpp          # 定时器 API 绑定
├── preact_renderer.h/cpp         # Preact 渲染器
├── preact_bindings.h/cpp         # Preact API 绑定
├── module_loader.h/cpp           # 模块加载器
└── js_utils.h/cpp                # JavaScript 工具函数
```

## 🔌 核心类

### QuickJSRuntime (JavaScript 运行时)

```cpp
class QuickJSRuntime {
public:
    QuickJSRuntime();
    ~QuickJSRuntime();
    
    // 脚本执行
    bool EvaluateScript(const std::string& script, 
                       const std::string& filename = "<eval>");
    
    JSValue EvaluateExpression(const std::string& expr);
    
    // 模块加载
    bool LoadModule(const std::string& module_path);
    
    // 全局对象
    JSValue GetGlobalObject();
    void SetGlobalProperty(const std::string& name, JSValue value);
    
    // 异常处理
    bool HasException();
    std::string GetExceptionString();
    void ClearException();
    
    // 垃圾回收
    void RunGC();
    
    // 获取上下文
    JSContext* GetContext();
};
```

### WindowBindings (Window API)

```cpp
class WindowBindings {
public:
    static void Register(JSContext* ctx, 
                        std::shared_ptr<Window> window);
    
    // Window 属性
    static JSValue GetInnerWidth(JSContext* ctx);
    static JSValue GetInnerHeight(JSContext* ctx);
    static JSValue GetDocument(JSContext* ctx);
    
    // Window 方法
    static JSValue Alert(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv);
    
    static JSValue SetTimeout(JSContext* ctx, JSValueConst this_val,
                             int argc, JSValueConst* argv);
    
    static JSValue RequestAnimationFrame(JSContext* ctx, 
                                        JSValueConst this_val,
                                        int argc, JSValueConst* argv);
};
```

### DOMBindings (DOM API)

```cpp
class DOMBindings {
public:
    static void Register(JSContext* ctx);
    
    // Document 方法
    static JSValue CreateElement(JSContext* ctx, JSValueConst this_val,
                                int argc, JSValueConst* argv);
    
    static JSValue QuerySelector(JSContext* ctx, JSValueConst this_val,
                                int argc, JSValueConst* argv);
    
    // Element 方法
    static JSValue SetAttribute(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv);
    
    static JSValue AddEventListener(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv);
    
    // Node 方法
    static JSValue AppendChild(JSContext* ctx, JSValueConst this_val,
                              int argc, JSValueConst* argv);
};
```

### PreactRenderer (Preact 渲染)

```cpp
class PreactRenderer {
public:
    PreactRenderer(std::shared_ptr<QuickJSRuntime> runtime,
                  std::shared_ptr<Document> document);
    
    // 渲染 Preact 组件
    void Render(JSValue component, std::shared_ptr<Element> container);
    
    // 更新组件
    void Update();
    
    // 卸载组件
    void Unmount();
    
    // 虚拟 DOM 差异计算
    void Diff(JSValue old_vnode, JSValue new_vnode);
};
```

## 💡 使用示例

### 初始化运行时

```cpp
auto runtime = std::make_shared<QuickJSRuntime>();
auto window = std::make_shared<Window>(800, 600, "Test");

// 注册 Window API
WindowBindings::Register(runtime->GetContext(), window);

// 注册 DOM API
DOMBindings::Register(runtime->GetContext());
```

### 执行 JavaScript

```cpp
// 执行脚本
std::string script = R"(
    console.log('Hello from JavaScript!');
    
    const div = document.createElement('div');
    div.textContent = 'Hello World';
    document.body.appendChild(div);
)";

if (!runtime->EvaluateScript(script)) {
    std::cerr << "Error: " << runtime->GetExceptionString() << std::endl;
}
```

### DOM 操作

```cpp
std::string script = R"(
    // 创建元素
    const button = document.createElement('button');
    button.textContent = 'Click Me';
    button.id = 'myButton';
    
    // 添加事件监听器
    button.addEventListener('click', () => {
        console.log('Button clicked!');
    });
    
    // 添加到 DOM
    document.body.appendChild(button);
    
    // 查询元素
    const btn = document.querySelector('#myButton');
    btn.style.color = 'red';
)";

runtime->EvaluateScript(script);
```

### 使用 Preact

```cpp
// 加载 Preact 库
runtime->LoadModule("preact.js");

// 渲染 Preact 组件
std::string script = R"(
    import { h, render } from 'preact';
    import { useState } from 'preact/hooks';
    
    function Counter() {
        const [count, setCount] = useState(0);
        
        return h('div', null,
            h('h1', null, `Count: ${count}`),
            h('button', { 
                onClick: () => setCount(count + 1) 
            }, 'Increment')
        );
    }
    
    render(h(Counter), document.body);
)";

runtime->EvaluateScript(script);
```

### 定时器

```cpp
std::string script = R"(
    // setTimeout
    setTimeout(() => {
        console.log('Delayed message');
    }, 1000);
    
    // setInterval
    let count = 0;
    const intervalId = setInterval(() => {
        console.log('Count:', count++);
        if (count >= 5) {
            clearInterval(intervalId);
        }
    }, 500);
    
    // requestAnimationFrame
    function animate(timestamp) {
        console.log('Frame:', timestamp);
        requestAnimationFrame(animate);
    }
    requestAnimationFrame(animate);
)";

runtime->EvaluateScript(script);
```

## 🔗 依赖关系

### 依赖的模块

- `third_party/quickjs` - QuickJS 引擎
- `core/dom` - DOM 对象
- `core/window` - Window 对象
- `core/event` - 事件系统

### 被依赖的模块

- `core/api` - C API 层（通过 JavaScript 调用）

## 🏗️ 架构说明

QuickJS 模块在架构中的位置：

```
┌─────────────────────────────────────────┐
│  JavaScript Code / Preact Components    │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  QuickJS Module (core/quickjs) ← 当前模块│
│  Runtime + Bindings + Preact            │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  C++ Core (DOM, Window, Event)          │
└─────────────────────────────────────────┘
```

## 📊 JavaScript API 支持

### 全局对象
- `window` - 窗口对象
- `document` - 文档对象
- `console` - 控制台对象

### Window API
- `window.innerWidth`, `window.innerHeight`
- `window.alert()`, `window.confirm()`, `window.prompt()`
- `window.setTimeout()`, `window.setInterval()`
- `window.requestAnimationFrame()`

### Document API
- `document.createElement()`
- `document.createTextNode()`
- `document.querySelector()`
- `document.querySelectorAll()`
- `document.getElementById()`
- `document.body`, `document.head`

### Element API
- `element.setAttribute()`, `element.getAttribute()`
- `element.addEventListener()`, `element.removeEventListener()`
- `element.appendChild()`, `element.removeChild()`
- `element.querySelector()`, `element.querySelectorAll()`
- `element.innerHTML`, `element.textContent`
- `element.style`, `element.classList`

### Console API
- `console.log()`, `console.error()`, `console.warn()`
- `console.info()`, `console.debug()`
- `console.time()`, `console.timeEnd()`

## 🔧 C++ 到 JavaScript 绑定

### 注册 C++ 函数

```cpp
// 定义 C++ 函数
static JSValue MyFunction(JSContext* ctx, JSValueConst this_val,
                         int argc, JSValueConst* argv) {
    // 获取参数
    const char* str = JS_ToCString(ctx, argv[0]);
    int num = 0;
    JS_ToInt32(ctx, &num, argv[1]);
    
    // 执行逻辑
    std::cout << "Called with: " << str << ", " << num << std::endl;
    
    // 释放字符串
    JS_FreeCString(ctx, str);
    
    // 返回值
    return JS_NewInt32(ctx, 42);
}

// 注册到 JavaScript
JSValue global = JS_GetGlobalObject(ctx);
JS_SetPropertyStr(ctx, global, "myFunction", 
                 JS_NewCFunction(ctx, MyFunction, "myFunction", 2));
JS_FreeValue(ctx, global);
```

### JavaScript 调用 C++

```javascript
// JavaScript 代码
const result = myFunction("hello", 123);
console.log(result);  // 输出: 42
```

## ⚠️ 注意事项

1. **内存管理**: 使用 `JS_FreeValue()` 释放 JSValue
2. **异常处理**: 检查 `JS_IsException()` 处理异常
3. **线程安全**: QuickJS 不是线程安全的，只在主线程使用
4. **垃圾回收**: 定期调用 `RunGC()` 释放内存

## 🚀 性能优化

### 减少 C++/JavaScript 边界跨越

```cpp
// ❌ 慢：频繁调用
for (int i = 0; i < 1000; i++) {
    runtime->EvaluateScript("doSomething()");
}

// ✅ 快：批量处理
runtime->EvaluateScript(R"(
    for (let i = 0; i < 1000; i++) {
        doSomething();
    }
)");
```

### 缓存 JavaScript 对象

```cpp
// 缓存常用对象
JSValue document = JS_GetPropertyStr(ctx, global, "document");
// 使用 document...
JS_FreeValue(ctx, document);
```

## 📚 相关文档

- [QuickJS 官方文档](https://bellard.org/quickjs/)
- [Preact 官方文档](https://preactjs.com/)
- [DOM API 文档](../../docs/DOM_API.md)
- [JavaScript 绑定指南](../../docs/JS_BINDINGS.md)

---

**维护者**: MBink Team  
**最后更新**: 2025-11-12

