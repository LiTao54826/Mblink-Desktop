# LightUI 架构设计文档

## 1. 总体架构

### 1.1 分层架构

```
┌─────────────────────────────────────────────────────────┐
│  Layer 5: 应用层 (Application Layer)                    │
│  - Python/C++/Rust/Go应用代码                           │
│  - 业务逻辑                                             │
└────────────────────┬────────────────────────────────────┘
                     │ FFI调用
┌────────────────────▼────────────────────────────────────┐
│  Layer 4: 语言绑定层 (Language Binding Layer)           │
│  - Python Binding (ctypes/pybind11)                     │
│  - Rust Binding (bindgen)                               │
│  - Go Binding (cgo)                                     │
└────────────────────┬────────────────────────────────────┘
                     │ C API
┌────────────────────▼────────────────────────────────────┐
│  Layer 3: C API层 (C API Layer)                         │
│  - lightui.h (统一的C接口)                              │
│  - 窗口管理、UI加载、函数绑定                           │
└────────────────────┬────────────────────────────────────┘
                     │ C++调用
┌────────────────────▼────────────────────────────────────┐
│  Layer 2: JavaScript运行时层 (JS Runtime Layer)         │
│  - QuickJS引擎                                          │
│  - DOM API实现                                          │
│  - Event系统                                            │
│  - JS <-> C++桥接                                       │
└────────────────────┬────────────────────────────────────┘
                     │ 渲染指令
┌────────────────────▼────────────────────────────────────┐
│  Layer 1: 渲染层 (Rendering Layer)                      │
│  - Yoga布局引擎                                         │
│  - Skia图形渲染                                         │
│  - SDL3窗口系统                                         │
└─────────────────────────────────────────────────────────┘
```

### 1.2 数据流

```
用户交互 (鼠标/键盘)
    │
    ▼
SDL3事件 ──────────────────────────────────┐
    │                                      │
    ▼                                      │
事件系统 (C++)                             │
    │                                      │
    ▼                                      │
DOM事件分发                                │
    │                                      │
    ▼                                      │
JavaScript事件处理器                       │
    │                                      │
    ▼                                      │
状态更新 (Preact)                          │
    │                                      │
    ▼                                      │
虚拟DOM diff                               │
    │                                      │
    ▼                                      │
DOM操作                                    │
    │                                      │
    ▼                                      │
标记脏节点                                 │
    │                                      │
    ▼                                      │
布局计算 (Yoga) ◄──────────────────────────┘
    │
    ▼
样式计算
    │
    ▼
渲染 (Skia)
    │
    ▼
显示到屏幕
```

---

## 2. 核心模块设计

### 2.1 窗口管理模块 (Window Module)

**职责**: 管理应用窗口的生命周期

```cpp
// window/window.h

class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();
    
    // 窗口操作
    void Show();
    void Hide();
    void Close();
    void SetTitle(const std::string& title);
    void SetSize(int width, int height);
    
    // 事件循环
    void Run();
    void Stop();
    
    // 获取内部对象
    SDL_Window* GetSDLWindow() const { return sdl_window_; }
    SkCanvas* GetCanvas() const { return canvas_; }
    
private:
    SDL_Window* sdl_window_;
    SDL_GLContext gl_context_;
    sk_sp<GrDirectContext> gr_context_;
    sk_sp<SkSurface> surface_;
    SkCanvas* canvas_;
    
    bool running_;
};
```

---

### 2.2 QuickJS运行时模块 (QuickJS Runtime Module)

**职责**: 管理JavaScript引擎，执行JS代码

```cpp
// quickjs/quickjs_runtime.h

class QuickJSRuntime {
public:
    QuickJSRuntime();
    ~QuickJSRuntime();
    
    // 执行JavaScript
    JSValue Eval(const std::string& code, const std::string& filename = "<eval>");
    JSValue EvalFile(const std::string& filepath);
    
    // 模块系统
    void RegisterModule(const std::string& name, const std::string& code);
    JSValue LoadModule(const std::string& name);
    
    // 全局对象
    void SetGlobalProperty(const std::string& name, JSValue value);
    JSValue GetGlobalProperty(const std::string& name);
    
    // 函数注册
    void RegisterFunction(const std::string& name, JSCFunction* func, int argc);
    
    // 数据转换
    JSValue ToJSValue(const json& data);
    json FromJSValue(JSValue value);
    
    // 错误处理
    std::string GetLastError();
    
    // 获取上下文
    JSContext* GetContext() const { return ctx_; }
    
private:
    JSRuntime* rt_;
    JSContext* ctx_;
    std::unordered_map<std::string, std::string> modules_;
};
```

---

### 2.3 DOM模块 (DOM Module)

**职责**: 实现轻量级DOM API

#### 2.3.1 节点类层次

```cpp
// dom/node.h

enum class NodeType {
    ELEMENT_NODE = 1,
    TEXT_NODE = 3,
    COMMENT_NODE = 8,
    DOCUMENT_NODE = 9
};

class Node {
public:
    virtual ~Node() = default;
    
    NodeType nodeType;
    std::string nodeName;
    Node* parentNode;
    std::vector<Node*> childNodes;
    
    // DOM操作
    virtual void AppendChild(Node* child);
    virtual void InsertBefore(Node* newNode, Node* refNode);
    virtual void RemoveChild(Node* child);
    virtual void ReplaceChild(Node* newNode, Node* oldNode);
    
    // 查询
    Node* FirstChild() const;
    Node* LastChild() const;
    Node* NextSibling() const;
    Node* PreviousSibling() const;
    
    // 渲染相关
    virtual void Layout(YGNodeRef yogaNode) = 0;
    virtual void Render(SkCanvas* canvas, const LayoutBox& box) = 0;
};
```

#### 2.3.2 Element类

```cpp
// dom/element.h

class Element : public Node {
public:
    Element(const std::string& tagName);
    ~Element() override;
    
    std::string tagName;
    std::map<std::string, std::string> attributes;
    std::map<std::string, std::string> styles;
    std::vector<std::string> classList;
    
    // 属性操作
    void SetAttribute(const std::string& name, const std::string& value);
    std::string GetAttribute(const std::string& name) const;
    void RemoveAttribute(const std::string& name);
    bool HasAttribute(const std::string& name) const;
    
    // 样式操作
    void SetStyle(const std::string& property, const std::string& value);
    std::string GetStyle(const std::string& property) const;
    
    // Class操作
    void AddClass(const std::string& className);
    void RemoveClass(const std::string& className);
    bool HasClass(const std::string& className) const;
    
    // 事件
    void AddEventListener(const std::string& type, JSValue handler);
    void RemoveEventListener(const std::string& type, JSValue handler);
    void DispatchEvent(Event* event);
    
    // 查询
    Element* GetElementById(const std::string& id);
    Element* QuerySelector(const std::string& selector);
    std::vector<Element*> QuerySelectorAll(const std::string& selector);
    
    // 渲染
    void Layout(YGNodeRef yogaNode) override;
    void Render(SkCanvas* canvas, const LayoutBox& box) override;
    
private:
    std::vector<EventListener> eventListeners_;
    YGNodeRef yogaNode_;
    LayoutBox layoutBox_;
};
```

#### 2.3.3 Document类

```cpp
// dom/document.h

class Document : public Node {
public:
    Document();
    ~Document() override;
    
    Element* documentElement;  // <html>
    Element* body;             // <body>
    
    // 创建节点
    Element* CreateElement(const std::string& tagName);
    Text* CreateTextNode(const std::string& data);
    Comment* CreateComment(const std::string& data);
    
    // 查询
    Element* GetElementById(const std::string& id);
    Element* QuerySelector(const std::string& selector);
    std::vector<Element*> QuerySelectorAll(const std::string& selector);
    
    // 事件
    void DispatchEvent(Event* event);
    
    // 渲染
    void Layout();
    void Render(SkCanvas* canvas);
    
private:
    std::unordered_map<std::string, Element*> idMap_;
};
```

---

### 2.4 事件系统模块 (Event System Module)

**职责**: 处理用户输入事件

```cpp
// event/event_system.h

class Event {
public:
    std::string type;
    Element* target;
    Element* currentTarget;
    bool bubbles;
    bool cancelable;
    bool defaultPrevented;
    
    void PreventDefault();
    void StopPropagation();
    void StopImmediatePropagation();
};

class MouseEvent : public Event {
public:
    int clientX;
    int clientY;
    int screenX;
    int screenY;
    int button;
    bool ctrlKey;
    bool shiftKey;
    bool altKey;
    bool metaKey;
};

class KeyboardEvent : public Event {
public:
    std::string key;
    std::string code;
    int keyCode;
    bool ctrlKey;
    bool shiftKey;
    bool altKey;
    bool metaKey;
};

class EventSystem {
public:
    EventSystem(Document* document);
    
    // 处理SDL事件
    void HandleSDLEvent(const SDL_Event& sdlEvent);
    
    // 分发DOM事件
    void DispatchEvent(Element* target, Event* event);
    
    // 命中测试
    Element* HitTest(int x, int y);
    
private:
    Document* document_;
    Element* focusedElement_;
    Element* hoveredElement_;
    
    // 事件转换
    MouseEvent* ConvertMouseEvent(const SDL_Event& sdlEvent);
    KeyboardEvent* ConvertKeyboardEvent(const SDL_Event& sdlEvent);
    
    // 事件传播
    void CapturePhase(Element* target, Event* event);
    void BubblePhase(Element* target, Event* event);
};
```

---

### 2.5 布局引擎模块 (Layout Engine Module)

**职责**: 使用Yoga计算布局

```cpp
// layout/layout_engine.h

struct LayoutBox {
    float x, y;
    float width, height;
    float paddingTop, paddingRight, paddingBottom, paddingLeft;
    float marginTop, marginRight, marginBottom, marginLeft;
};

class LayoutEngine {
public:
    LayoutEngine();
    ~LayoutEngine();
    
    // 布局计算
    void Layout(Element* root, float availableWidth, float availableHeight);
    
    // 获取布局结果
    LayoutBox GetLayoutBox(Element* element);
    
    // 样式应用
    void ApplyStyles(Element* element, YGNodeRef yogaNode);
    
private:
    YGConfigRef config_;
    std::unordered_map<Element*, YGNodeRef> nodeMap_;
    
    // 样式解析
    YGFlexDirection ParseFlexDirection(const std::string& value);
    YGJustify ParseJustifyContent(const std::string& value);
    YGAlign ParseAlignItems(const std::string& value);
    YGValue ParseSize(const std::string& value);
};
```

---

### 2.6 渲染引擎模块 (Rendering Engine Module)

**职责**: 使用Skia渲染DOM树

```cpp
// render/renderer.h

class Renderer {
public:
    Renderer(SkCanvas* canvas);
    
    // 渲染DOM树
    void Render(Document* document);
    
    // 渲染单个元素
    void RenderElement(Element* element, const LayoutBox& box);
    
    // 渲染文本
    void RenderText(Text* textNode, const LayoutBox& box);
    
private:
    SkCanvas* canvas_;
    sk_sp<SkTypeface> defaultTypeface_;
    
    // 样式渲染
    void RenderBackground(const LayoutBox& box, const std::string& color);
    void RenderBorder(const LayoutBox& box, const std::map<std::string, std::string>& styles);
    void RenderShadow(const LayoutBox& box, const std::string& shadow);
    
    // 文本渲染
    void RenderTextContent(const std::string& text, const LayoutBox& box, 
                          const std::map<std::string, std::string>& styles);
    
    // 工具函数
    SkColor ParseColor(const std::string& colorStr);
    sk_sp<SkTypeface> GetTypeface(const std::string& fontFamily);
};
```

---

### 2.7 桥接层模块 (Bridge Module)

**职责**: 连接宿主语言和JavaScript

```cpp
// bridge/bridge.h

class Bridge {
public:
    Bridge(QuickJSRuntime* runtime);
    
    // 注册宿主函数（让JS调用）
    void RegisterHostFunction(const std::string& name, HostFunction func);
    
    // 调用JS函数（让宿主调用JS）
    json CallJSFunction(const std::string& name, const json& args);
    
    // 事件通知
    void EmitEvent(const std::string& eventName, const json& data);
    
private:
    QuickJSRuntime* runtime_;
    std::unordered_map<std::string, HostFunction> hostFunctions_;
    
    // 回调包装
    static JSValue HostFunctionWrapper(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv, void* opaque);
};

// 宿主函数类型
using HostFunction = std::function<json(const json& args)>;
```

---

## 3. 关键流程

### 3.1 应用启动流程

```
1. 初始化SDL3
   └─> SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)

2. 创建窗口
   └─> SDL_CreateWindow()
   └─> SDL_GL_CreateContext()

3. 初始化Skia
   └─> GrDirectContext::MakeGL()
   └─> SkSurface::MakeFromBackendRenderTarget()

4. 初始化QuickJS
   └─> JS_NewRuntime()
   └─> JS_NewContext()

5. 注册DOM API
   └─> 绑定document对象
   └─> 绑定Element类
   └─> 绑定Event类

6. 注册宿主函数
   └─> bridge.RegisterHostFunction()

7. 加载UI代码
   └─> runtime.Eval(uiCode)

8. 进入事件循环
   └─> window.Run()
```

### 3.2 渲染流程

```
1. 事件触发状态更新
   └─> JavaScript事件处理器

2. Preact更新虚拟DOM
   └─> diff算法

3. 应用DOM操作
   └─> element.appendChild()
   └─> element.setAttribute()
   └─> textNode.data = "..."

4. 标记脏节点
   └─> element.MarkDirty()

5. 请求重绘
   └─> window.RequestRedraw()

6. 布局计算（下一帧）
   └─> layoutEngine.Layout(document.body)
   └─> Yoga计算所有节点位置

7. 渲染（下一帧）
   └─> renderer.Render(document)
   └─> 遍历DOM树
   └─> 调用Skia绘制

8. 交换缓冲区
   └─> SDL_GL_SwapWindow()
```

### 3.3 事件处理流程

```
1. SDL捕获系统事件
   └─> SDL_PollEvent()

2. 转换为DOM事件
   └─> eventSystem.HandleSDLEvent()
   └─> 创建MouseEvent/KeyboardEvent

3. 命中测试
   └─> eventSystem.HitTest(x, y)
   └─> 找到目标元素

4. 捕获阶段
   └─> 从document到target
   └─> 调用捕获监听器

5. 目标阶段
   └─> 调用target的监听器

6. 冒泡阶段
   └─> 从target到document
   └─> 调用冒泡监听器

7. 默认行为
   └─> 如果未preventDefault()
   └─> 执行默认行为
```

---

## 4. 性能优化策略

### 4.1 渲染优化

- **脏矩形**: 只重绘变化的区域
- **图层缓存**: 缓存静态内容
- **批量更新**: 合并多次DOM操作
- **虚拟滚动**: 大列表只渲染可见部分

### 4.2 布局优化

- **增量布局**: 只重新计算变化的子树
- **布局缓存**: 缓存布局结果
- **延迟布局**: 使用requestAnimationFrame

### 4.3 内存优化

- **对象池**: 复用Event对象
- **弱引用**: 避免循环引用
- **及时释放**: 清理不用的资源

---

## 5. 线程模型

```
主线程 (UI Thread):
├── 事件循环
├── JavaScript执行
├── DOM操作
├── 布局计算
└── 渲染

工作线程 (可选):
├── 图片解码
├── 网络请求
└── 文件I/O
```

**注意**: 当前版本为单线程模型，未来可扩展为多线程。

---

## 6. 内存管理

### 6.1 JavaScript对象

- QuickJS自动GC
- 使用引用计数

### 6.2 C++对象

- 智能指针（shared_ptr/unique_ptr）
- RAII原则
- 明确的生命周期管理

### 6.3 跨语言对象

- JS对象持有C++对象的指针
- C++对象持有JS对象的引用
- 使用弱引用避免循环

---

## 7. 错误处理

### 7.1 JavaScript错误

```cpp
try {
    runtime.Eval(code);
} catch (const JSException& e) {
    std::cerr << "JS Error: " << e.what() << std::endl;
}
```

### 7.2 C++异常

- 使用异常处理关键错误
- 返回错误码处理预期错误
- 日志记录所有错误

---

## 8. 扩展性设计

### 8.1 插件系统（未来）

```cpp
class Plugin {
public:
    virtual void OnLoad() = 0;
    virtual void OnUnload() = 0;
};
```

### 8.2 自定义渲染器（未来）

```cpp
class CustomRenderer : public Renderer {
public:
    void RenderElement(Element* element, const LayoutBox& box) override {
        // 自定义渲染逻辑
    }
};
```

---

## 9. 安全性考虑

- **沙箱**: QuickJS运行在受限环境
- **输入验证**: 验证所有外部输入
- **资源限制**: 限制内存和CPU使用
- **权限控制**: 限制文件系统访问

