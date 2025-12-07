# MBink 架构设计文档

> **版本**: 5.0
> **最后更新**: 2025-12-07
> **项目定位**: 轻量级跨平台桌面应用框架 - Electron的轻量级替代品

> **实现状态** (2025-12-07):
> - ✅ 所有 5 层架构已实现
> - ✅ Skia 渲染层完全集成并测试
> - ✅ QuickJS 运行时层完全实现
> - ✅ DOM 和事件系统完全实现
> - ✅ 原生布局引擎 (Block + IFC + Flexbox + Grid)
> - ✅ IFC 行内格式化上下文 (text-align, vertical-align)
> - ✅ CSS 高级特性完成 (动画、变换、滤镜)
> - ✅ 541 个测试验证架构正确性

---

## 0. 项目定位

### 0.1 核心定位

**MBink 是一个轻量级的跨平台桌面应用框架，目标是成为 Electron 的轻量级替代品**

**不是**: 游戏UI库（如RmlUi）
**是**: 桌面应用开发框架（如Electron、Tauri）

### 0.2 与竞品对比

| 维度 | **MBink** | **Electron** | **Tauri** | **RmlUi** |
|------|-----------|-------------|-----------|-----------|
| **目标场景** | 桌面应用 | 桌面应用 | 桌面应用 | 游戏UI |
| **体积** | ~50MB | ~150MB | ~10MB | ~5MB |
| **JS引擎** | QuickJS | V8 | JavaScriptCore | ❌ |
| **渲染** | Skia | Chromium | WebView | 用户提供 |
| **React支持** | ✅ | ✅ | ✅ | ❌ |
| **启动速度** | 快 (~200ms) | 慢 (~1s) | 快 (~100ms) | 极快 (~50ms) |
| **内存占用** | 中 (~100MB) | 高 (~300MB) | 低 (~50MB) | 极低 (~20MB) |

### 0.3 核心价值主张

1. ✅ **浏览器级渲染质量** - Skia引擎，与Chrome同源
2. ✅ **React生态支持** - 利用丰富的React组件库（Ant Design、Material-UI等）
3. ✅ **轻量级** - 约50MB，比Electron小50-70%
4. ✅ **跨语言绑定** - Python/Rust/Go/C++/Node.js都能使用
5. ✅ **高性能** - QuickJS轻量级引擎，启动快

### 0.4 技术选型说明

| 组件 | 技术选型 | 原因 | 不选择的方案 |
|------|---------|------|-------------|
| **JS引擎** | QuickJS | 轻量级(600KB)，启动快 | ❌ V8 (太重，70MB+) |
| **渲染引擎** | Skia | 浏览器级质量，Chrome同源 | ❌ 自研渲染器 |
| **布局引擎** | Native + Taffy | 原生 Block/IFC + Taffy Flexbox/Grid | ❌ 单独使用 Yoga |
| **HTML解析** | Lexbor | 完整HTML5/CSS3支持 | ❌ 自研解析器 |
| **窗口系统** | SDL3 | 跨平台，稳定 | ❌ GLFW, Qt |

### 0.5 从RmlUi借鉴的内容

**借鉴** ✅:
- 拖拽系统 (drag-and-drop)
- 焦点管理 (focus management)
- CSS动画和过渡 (animations & transitions)
- 键盘事件完善 (keyboard events)
- 事件规范化系统 (event specification)

**不借鉴** ❌:
- 数据绑定系统 (React已提供)
- 装饰器系统 (不符合定位)
- 自研HTML/CSS解析器 (Lexbor已足够)

---

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
│  - 原生布局引擎 (Block + IFC)                           │
│  - Taffy布局引擎 (Flexbox + CSS Grid)                   │
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
布局计算 (Taffy) ◄─────────────────────────┘
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

**职责**: 计算 CSS 布局 (Block + IFC + Flexbox + Grid)

MBink 使用混合布局引擎架构:
- **原生布局引擎**: 处理 Block 布局和 IFC (Inline Formatting Context)
- **Taffy 布局引擎**: 处理 Flexbox 和 CSS Grid 布局

```cpp
// layout/native_layout_engine.h

class NativeLayoutEngine {
public:
    // 执行完整布局
    void PerformLayout(RenderObject* root, float available_width, float available_height);

    // 计算尺寸（不设置位置）
    LayoutResult ComputeSize(RenderObject* root, float available_width, float available_height);

private:
    // Block 布局
    LayoutResult LayoutBlock(RenderObject* obj, float available_width);

    // IFC 布局 (行内格式化上下文)
    LayoutResult LayoutIFC(RenderObject* container, float available_width);

    // 委托给 Taffy 处理 Flexbox/Grid
    LayoutResult LayoutFlex(RenderObject* obj, float available_width);
    LayoutResult LayoutGrid(RenderObject* obj, float available_width);
};
```

#### IFC (Inline Formatting Context) 架构

```cpp
// layout/ifc/ifc_layout.h

class IFCLayout {
public:
    // 执行 IFC 布局
    IFCLayoutResult Layout(RenderObject* container, float available_width, RunMode mode);

private:
    // 收集行内内容
    void CollectInlineContent(RenderObject* container);

    // 换行算法
    void BreakIntoLines(float available_width);

    // 行内对齐
    void AlignLine(Line& line, TextAlign align, float container_width);

    // 垂直对齐
    void VerticalAlignBoxes(Line& line);
};

// 行内盒子类型
enum class InlineBoxType {
    TEXT,           // 文本内容
    ATOMIC,         // inline-block, img 等
    OPEN_TAG,       // 开始标签 (span, a 等)
    CLOSE_TAG       // 结束标签
};
```

**IFC 特性**:
- ✅ text-align: left/center/right/justify
- ✅ vertical-align: top/middle/bottom/baseline
- ✅ inline-block 元素支持
- ✅ CJK 字符换行
- ✅ 连字符断行

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
   └─> Taffy计算所有节点位置 (Flexbox + CSS Grid)

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

