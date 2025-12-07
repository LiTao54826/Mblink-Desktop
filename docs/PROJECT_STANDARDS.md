# MBink 项目开发规范

> **版本**: 3.0
> **生效日期**: 2025-12-07
> **状态**: 强制执行
> **适用范围**: 所有后续开发

---

## 📋 目录

1. [项目定位](#项目定位)
2. [强制规范](#强制规范)
3. [架构规范](#架构规范)
4. [代码规范](#代码规范)
5. [文档规范](#文档规范)
6. [测试规范](#测试规范)
7. [Git规范](#git规范)

---

## 🎯 项目定位

### 核心定位

**MBink 是一个轻量级的跨平台桌面应用框架，目标是成为 Electron 的轻量级替代品**

### 与 RmlUi 的区别

| 维度 | **RmlUi** | **MBink** |
|------|-----------|-----------|
| **目标场景** | 游戏UI、实时渲染 | 桌面应用开发 |
| **渲染方式** | 用户提供渲染器 | 内置Skia渲染 |
| **JavaScript** | 可选Lua插件 | 核心QuickJS引擎 |
| **生态系统** | 自定义标记 | **React生态** |
| **布局引擎** | 自研CSS布局 | **Native + Taffy** |
| **HTML解析** | 自研解析器 | **Lexbor (HTML5)** |

### 核心价值主张

1. ✅ **浏览器级渲染质量** - Skia引擎，与Chrome同源
2. ✅ **React生态支持** - 利用丰富的React组件库
3. ✅ **轻量级** - 约50MB，比Electron小50-70%
4. ✅ **跨语言绑定** - Python/Rust/Go/C++都能使用
5. ✅ **高性能** - QuickJS轻量级引擎，启动快

---

## ⚠️ 强制规范

### 规范1: 技术栈锁定

**以下技术栈不得更改，除非有充分理由并经过团队讨论**：

| 组件 | 技术选型 | 版本 | 原因 |
|------|---------|------|------|
| **JavaScript引擎** | QuickJS | 2024-01-13 | 轻量级(600KB)，符合定位 |
| **渲染引擎** | Skia | m116 | 浏览器级质量，Chrome同源 |
| **窗口系统** | SDL3 | 3.1.6+ | 跨平台，稳定 |
| **布局引擎** | Native Layout Engine | 0.91.0 | 原生实现 Block + IFC + Flex + Grid |
| **HTML解析** | Lexbor | 2.6.0+ | 完整HTML5/CSS3支持 |
| **JSON库** | nlohmann/json | 3.11.0+ | 现代C++ API |

**❌ 禁止行为**：
- ❌ 不得引入V8引擎（太重，违背轻量级定位）
- ❌ 不得自研HTML/CSS解析器（Lexbor已足够）
- ❌ 不得引入Electron/Chromium（违背项目定位）

### 规范2: 模块边界严格

**模块依赖关系（单向依赖，不得循环）**：

```
┌─────────────────────────────────────────┐
│  Application Layer (用户代码)            │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Language Bindings (Python/Rust/Go)     │
│  bindings/python, bindings/rust, etc.   │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  C API Layer (core/api)                 │
│  lightui.h, lightui.cpp                 │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  JavaScript Runtime (core/quickjs)      │
│  quickjs_runtime, window_bindings       │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Core Modules (core/*)                  │
│  ├─ DOM (core/dom)                      │
│  ├─ Event (core/event)                  │
│  ├─ Render (core/render)                │
│  ├─ Layout (core/layout)                │
│  ├─ Window (core/window)                │
│  └─ Utils (core/utils)                  │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Third Party (third_party/*)            │
│  QuickJS, Skia, SDL3, Lexbor            │
└─────────────────────────────────────────┘
```

**✅ 允许的依赖**：
- ✅ 上层可以依赖下层
- ✅ 同层模块可以相互依赖（但需文档说明）
- ✅ 所有模块可以依赖 `core/utils`

**❌ 禁止的依赖**：
- ❌ 下层不得依赖上层
- ❌ 不得跨层依赖（如 bindings 直接依赖 core/dom）
- ❌ 不得循环依赖

### 规范3: 文件组织规范

**目录结构（强制）**：

```
MBink/
├── core/                    # 核心C++代码
│   ├── api/                 # C API接口
│   ├── dom/                 # DOM实现
│   ├── event/               # 事件系统
│   ├── layout/              # 布局引擎
│   ├── lexbor/              # Lexbor包装
│   ├── quickjs/             # QuickJS运行时
│   ├── render/              # 渲染引擎
│   ├── utils/               # 工具类
│   └── window/              # 窗口管理
├── bindings/                # 语言绑定
│   ├── python/
│   ├── rust/
│   ├── go/
│   └── nodejs/
├── js/                      # JavaScript运行时
│   ├── polyfills/           # Polyfills
│   ├── preact/              # Preact库
│   └── runtime/             # 运行时脚本
├── examples/                # 示例代码
├── tests/                   # 测试代码
│   ├── unit/                # 单元测试
│   ├── integration/         # 集成测试
│   └── benchmarks/          # 性能测试
├── docs/                    # 长期文档
│   ├── ARCHITECTURE.md      # 架构设计
│   ├── API_DESIGN.md        # API设计
│   ├── CODING_STANDARDS.md  # 代码规范
│   └── ...
├── history_task_docs/       # 历史文档（只增不改）
├── ReferenceProject/        # 参考项目（只读）
│   └── RmlUi/               # RmlUi参考
├── third_party/             # 第三方库
├── README.md                # 项目主页
├── ROADMAP.md               # 开发路线图
└── CMakeLists.txt           # 构建配置
```

**✅ 文件命名规范**：
- ✅ C++源文件: `snake_case.cpp`, `snake_case.h`
- ✅ 类名: `PascalCase` (如 `QuickJSRuntime`)
- ✅ 函数名: `PascalCase` (如 `CreateElement`)
- ✅ 变量名: `snake_case_` (成员变量加下划线)
- ✅ 常量: `UPPER_CASE`

**❌ 禁止行为**：
- ❌ 不得在根目录创建临时文件
- ❌ 不得在 `docs/` 创建会话报告（应放 `history_task_docs/`）
- ❌ 不得在 `core/` 创建测试文件（应放 `tests/`）

### 规范4: 依赖管理规范

**✅ 必须使用包管理器**：
- ✅ C++依赖: 使用 CMake FetchContent 或 vcpkg
- ✅ Python依赖: 使用 pip/poetry
- ✅ Rust依赖: 使用 cargo
- ✅ Node.js依赖: 使用 npm/pnpm

**❌ 禁止行为**：
- ❌ 不得手动编辑 `package.json` 添加依赖（必须用 `npm install`）
- ❌ 不得手动编辑 `Cargo.toml` 添加依赖（必须用 `cargo add`）
- ❌ 不得手动编辑 `requirements.txt`（必须用 `pip install` + `pip freeze`）
- ❌ 不得提交 `node_modules/`, `target/`, `build/` 到Git

### 规范5: 前端框架集成规范

**核心原则：C++层只提供浏览器级别的API，不实现框架特定功能**

**✅ 正确的做法**：
```cpp
// ✅ 提供标准的浏览器DOM API
class Document {
public:
    std::shared_ptr<Element> CreateElement(const std::string& tag_name);
    std::shared_ptr<Element> GetElementById(const std::string& id);
    std::shared_ptr<Element> QuerySelector(const std::string& selector);
};

class Element {
public:
    void SetAttribute(const std::string& name, const std::string& value);
    void AppendChild(std::shared_ptr<Node> child);
    void RemoveChild(std::shared_ptr<Node> child);
    void AddEventListener(const std::string& type, EventListener listener);
};

// ✅ 让React/Preact在JavaScript层运行
// js/preact/preact.js - 原生Preact库，无需修改
// 应用代码通过标准DOM API操作DOM
```

**❌ 错误的做法**：
```cpp
// ❌ 不要在C++层实现React特定功能
class PreactRenderer {
    void RenderComponent(JSValue component);     // ❌ 不要实现组件渲染
    void UpdateVirtualDOM(JSValue vnode);        // ❌ 不要实现Virtual DOM
    void ReconcileChildren(JSValue children);    // ❌ 不要实现Reconciliation
};

// ❌ 不要在C++层实现Hooks
class HooksManager {
    JSValue UseState(JSValue initial);           // ❌ 不要实现useState
    void UseEffect(JSValue callback);            // ❌ 不要实现useEffect
};

// ❌ 不要在C++层实现组件生命周期
class ComponentManager {
    void ComponentDidMount(JSValue component);   // ❌ 不要实现生命周期
    void ComponentWillUnmount(JSValue component);// ❌ 不要实现生命周期
};
```

**设计理念**：

1. **通用性优先** - C++层提供的API应该能支持任何前端框架（React、Vue、Angular、Svelte等）
2. **标准化** - 遵循W3C DOM标准，而不是特定框架的API
3. **框架无关** - 框架特定的逻辑应该在JavaScript层实现
4. **可替换性** - 用户应该能够轻松切换前端框架，而无需修改C++代码

**正确的集成方式**：

```
┌─────────────────────────────────────────┐
│  Application Code (app.js)              │  用户应用
│  - React/Vue/Angular/Svelte             │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Frontend Framework (preact.js)         │  前端框架
│  - Virtual DOM                          │  (JavaScript层)
│  - Component System                     │
│  - Hooks / Lifecycle                    │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Browser DOM API (dom_bindings.cpp)    │  标准DOM API
│  - document.createElement()             │  (C++层)
│  - element.appendChild()                │
│  - element.addEventListener()           │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  MBink Core (core/dom, core/render)    │  核心实现
└─────────────────────────────────────────┘
```

**允许的C++层功能**：
- ✅ 标准DOM API（createElement, appendChild, setAttribute等）
- ✅ 标准事件API（addEventListener, removeEventListener等）
- ✅ 标准选择器API（querySelector, querySelectorAll等）
- ✅ 标准样式API（style, classList, className等）
- ✅ 标准定时器API（setTimeout, setInterval等）
- ✅ 标准Console API（console.log, console.error等）

**禁止的C++层功能**：
- ❌ 框架特定的组件系统
- ❌ 框架特定的状态管理（useState, Redux等）
- ❌ 框架特定的生命周期钩子
- ❌ 框架特定的Virtual DOM实现
- ❌ 框架特定的Reconciliation算法
- ❌ 框架特定的JSX/模板编译

**示例：正确的Preact集成**：

```javascript
// ✅ 在JavaScript层使用原生Preact
import { h, render } from './preact.js';

function App() {
    const [count, setCount] = useState(0);  // Preact的useState

    return h('div', null,
        h('h1', null, 'Counter'),
        h('p', null, `Count: ${count}`),
        h('button', { onClick: () => setCount(count + 1) }, 'Increment')
    );
}

// Preact通过标准DOM API操作DOM
render(h(App), document.body);
```

```cpp
// ✅ C++层只提供标准DOM API
// core/dom/dom_bindings.cpp
JSValue js_create_element(JSContext* ctx, JSValueConst this_val,
                          int argc, JSValueConst* argv) {
    const char* tag_name = JS_ToCString(ctx, argv[0]);
    auto element = document->CreateElement(tag_name);
    return WrapElement(ctx, element);  // 返回标准Element对象
}

JSValue js_append_child(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv) {
    auto parent = UnwrapElement(ctx, this_val);
    auto child = UnwrapNode(ctx, argv[0]);
    parent->AppendChild(child);
    return JS_UNDEFINED;
}
```

**违规检查清单**：

在添加新功能前，问自己：
1. ❓ 这个功能是否是标准浏览器API？
2. ❓ 这个功能是否只对特定框架有用？
3. ❓ 如果用户想用Vue而不是React，这个功能还有用吗？
4. ❓ 这个功能是否可以在JavaScript层实现？

如果答案是"否、是、否、是"，那么这个功能**不应该**在C++层实现。

### 规范6: 文档规范

**文档分类**：

| 类型 | 位置 | 生命周期 | 示例 |
|------|------|---------|------|
| **长期文档** | `docs/` | 持续更新 | ARCHITECTURE.md, API_DESIGN.md |
| **历史文档** | `history_task_docs/` | 只增不改 | PHASE_2_1_REPORT.md |
| **项目主页** | 根目录 | 持续更新 | README.md, ROADMAP.md |
| **代码注释** | 源文件 | 随代码更新 | Doxygen注释 |

**✅ 必须遵守**：
- ✅ 每个模块必须有 README.md
- ✅ 每个公开API必须有Doxygen注释
- ✅ 每个Phase完成后必须写完成报告（放 `history_task_docs/`）
- ✅ 架构变更必须更新 `docs/ARCHITECTURE.md`

**❌ 禁止行为**：
- ❌ 不得在根目录创建临时文档（如 `temp.md`, `notes.txt`）
- ❌ 不得修改 `history_task_docs/` 中的文档（只能新增）
- ❌ 不得创建重复文档（如同时有 `PROJECT_STATUS.md` 和 `PROJECT_PROGRESS.md`）

---

## 🏗️ 架构规范

### 核心架构原则

1. **分层架构** - 严格的层次依赖关系
2. **模块化** - 每个模块职责单一
3. **接口隔离** - 通过C API暴露功能
4. **依赖注入** - 避免硬编码依赖

### 模块职责定义

| 模块 | 职责 | 不得包含 |
|------|------|---------|
| **core/dom** | DOM树管理、节点操作 | ❌ 渲染逻辑、事件处理 |
| **core/event** | 事件循环、事件分发 | ❌ DOM操作、渲染逻辑 |
| **core/render** | Skia渲染、样式计算 | ❌ DOM操作、事件处理 |
| **core/layout** | 原生布局引擎 (Block/IFC/Flex/Grid) | ❌ 渲染逻辑、事件处理 |
| **core/window** | SDL窗口管理 | ❌ DOM操作、渲染逻辑 |
| **core/quickjs** | JS运行时、绑定 | ❌ 业务逻辑 |
| **core/lexbor** | HTML/CSS解析 | ❌ 渲染逻辑、布局计算 |

### 接口设计原则

**✅ 好的接口设计**：
```cpp
// ✅ 清晰的职责
class Element {
public:
    void SetAttribute(const std::string& name, const std::string& value);
    std::string GetAttribute(const std::string& name) const;
    void AppendChild(std::shared_ptr<Node> child);
};

// ✅ 依赖注入
class Renderer {
public:
    Renderer(SkCanvas* canvas, StyleResolver* resolver);
    void Render(Element* element);
};
```

**❌ 不好的接口设计**：
```cpp
// ❌ 职责混乱
class Element {
public:
    void Render(SkCanvas* canvas);  // ❌ Element不应该知道渲染
    void HandleClick();             // ❌ Element不应该处理事件
};

// ❌ 硬编码依赖
class Renderer {
public:
    Renderer() {
        canvas_ = GetGlobalCanvas();  // ❌ 硬编码全局依赖
    }
};
```

---

## 💻 代码规范

### C++ 代码规范

**遵循 Google C++ Style Guide，但有以下调整**：

1. **命名规范**：
   - 类名: `PascalCase`
   - 函数名: `PascalCase`
   - 变量名: `snake_case_` (成员变量加下划线)
   - 常量: `UPPER_CASE`

2. **文件组织**：
   ```cpp
   // element.h
   #pragma once
   
   #include <memory>
   #include <string>
   
   namespace lightui {
   
   class Element {
   public:
       Element();
       ~Element();
       
       // 禁止拷贝
       Element(const Element&) = delete;
       Element& operator=(const Element&) = delete;
       
       // 允许移动
       Element(Element&&) noexcept = default;
       Element& operator=(Element&&) noexcept = default;
       
   private:
       std::string tag_name_;
   };
   
   } // namespace lightui
   ```

3. **内存管理**：
   - ✅ 优先使用 `std::shared_ptr` / `std::unique_ptr`
   - ✅ 使用 RAII 管理资源
   - ❌ 禁止使用裸指针（除非与C库交互）
   - ❌ 禁止手动 `new` / `delete`

4. **错误处理**：
   - ✅ 使用异常处理错误（C++层）
   - ✅ 使用错误码（C API层）
   - ❌ 禁止忽略错误

### JavaScript 代码规范

**遵循 Airbnb JavaScript Style Guide**：

```javascript
// ✅ 好的代码
function createButton(text, onClick) {
    const button = document.createElement('button');
    button.textContent = text;
    button.addEventListener('click', onClick);
    return button;
}

// ❌ 不好的代码
function createButton(text, onClick) {
    var button = document.createElement('button');  // ❌ 使用var
    button.innerText = text;                        // ❌ 使用innerText
    button.onclick = onClick;                       // ❌ 使用onclick
    return button;
}
```

---

## 📝 测试规范

### 测试覆盖率要求

| 模块 | 最低覆盖率 | 当前覆盖率 |
|------|-----------|-----------|
| core/dom | 90% | 95% ✅ |
| core/event | 85% | 待测试 |
| core/render | 80% | 待测试 |
| core/layout | 85% | 待测试 |
| core/quickjs | 90% | 待测试 |

### 测试命名规范

```cpp
// tests/unit/test_dom_element.cpp
TEST_CASE("Element::SetAttribute should set attribute correctly") {
    auto elem = std::make_shared<Element>("div");
    elem->SetAttribute("id", "test");
    REQUIRE(elem->GetAttribute("id") == "test");
}

TEST_CASE("Element::AppendChild should add child to children list") {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);
    REQUIRE(parent->GetChildCount() == 1);
}
```

**✅ 必须遵守**：
- ✅ 每个公开API必须有单元测试
- ✅ 每个Bug修复必须有回归测试
- ✅ 每个新功能必须有集成测试

---

## 📚 参考资料

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Airbnb JavaScript Style Guide](https://github.com/airbnb/javascript)
- [RmlUi Documentation](https://mikke89.github.io/RmlUiDoc/)
- [React Documentation](https://react.dev/)

---

---

## 📋 版本历史

### v3.0 (2025-12-07)
- ✅ 布局引擎更新为原生实现 (Native Layout Engine)
- ✅ 移除 Yoga 依赖，Taffy 移至 ReferenceProject 作为参考
- ✅ 支持 Block + IFC + Flexbox + Grid 布局

### v2.1 (2025-11-14)
- ✅ 新增规范5：前端框架集成规范
- ✅ 明确C++层只提供浏览器级别API的原则
- ✅ 禁止在C++层实现框架特定功能

### v2.0 (2025-11-11)
- 初始版本

---

**最后更新**: 2025-12-07
**维护者**: MBink Team

