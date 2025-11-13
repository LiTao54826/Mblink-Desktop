# API 模块

## 📋 概述

C API 接口层，对外暴露 MBink 的核心功能。这是 MBink 框架与外部世界交互的主要接口，提供了 C 语言风格的 API，使得其他语言（Python、Rust、Go 等）可以通过语言绑定来使用 MBink。

## 🎯 主要功能

- **C 语言接口**: 提供纯 C API，确保跨语言兼容性
- **封装 C++ 实现**: 将内部 C++ 实现封装为 C 接口
- **支持跨语言绑定**: 为 Python、Rust、Go、Node.js 等语言提供绑定基础
- **资源管理**: 管理窗口、文档等资源的生命周期
- **错误处理**: 将 C++ 异常转换为 C 风格的错误码

## 📁 文件结构

```
api/
├── CMakeLists.txt    # 构建配置
├── lightui.h         # C API 头文件（公开接口）
└── lightui.cpp       # C API 实现
```

## 🔌 API 列表

### 核心函数

#### 初始化和清理

```c
// 初始化 LightUI 框架
int lightui_init(void);

// 关闭 LightUI 框架
void lightui_shutdown(void);
```

#### 窗口管理

```c
// 创建窗口
LightUIWindow* lightui_create_window(int width, int height, const char* title);

// 销毁窗口
void lightui_destroy_window(LightUIWindow* window);

// 运行事件循环
void lightui_run(void);

// 停止事件循环
void lightui_stop(void);
```

#### 文档操作

```c
// 加载 HTML 内容
int lightui_load_html(LightUIWindow* window, const char* html);

// 加载 HTML 文件
int lightui_load_file(LightUIWindow* window, const char* filepath);

// 执行 JavaScript 代码
int lightui_eval_js(LightUIWindow* window, const char* script);
```

## 💡 使用示例

### 基础示例

```c
#include "core/api/lightui.h"

int main() {
    // 初始化框架
    if (lightui_init() != 0) {
        return 1;
    }
    
    // 创建窗口
    LightUIWindow* window = lightui_create_window(800, 600, "Hello MBink");
    if (!window) {
        lightui_shutdown();
        return 1;
    }
    
    // 加载 HTML 内容
    const char* html = "<html><body><h1>Hello World!</h1></body></html>";
    lightui_load_html(window, html);
    
    // 运行事件循环
    lightui_run();
    
    // 清理资源
    lightui_destroy_window(window);
    lightui_shutdown();
    
    return 0;
}
```

### JavaScript 集成示例

```c
#include "core/api/lightui.h"

int main() {
    lightui_init();
    
    LightUIWindow* window = lightui_create_window(800, 600, "JS Example");
    
    // 加载 HTML
    lightui_load_html(window, 
        "<html><body><div id='app'></div></body></html>");
    
    // 执行 JavaScript
    lightui_eval_js(window, 
        "document.getElementById('app').textContent = 'Hello from JS!';");
    
    lightui_run();
    
    lightui_destroy_window(window);
    lightui_shutdown();
    
    return 0;
}
```

## 🔗 依赖关系

### 依赖的模块

- `core/window` - 窗口管理
- `core/quickjs` - JavaScript 运行时
- `core/dom` - DOM 实现
- `core/event` - 事件系统
- `core/render` - 渲染引擎
- `core/bridge` - C++/C 桥接层

### 被依赖的模块

- `bindings/python` - Python 语言绑定
- `bindings/rust` - Rust 语言绑定
- `bindings/go` - Go 语言绑定
- `bindings/nodejs` - Node.js 语言绑定

## 🏗️ 架构说明

API 模块位于整个架构的顶层，作为对外接口：

```
┌─────────────────────────────────────────┐
│  Language Bindings (Python/Rust/Go)     │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  C API Layer (core/api) ← 当前模块       │
│  lightui.h, lightui.cpp                 │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  C++ Core Modules                       │
└─────────────────────────────────────────┘
```

## 📚 相关文档

- [API 设计文档](../../docs/API_DESIGN.md)
- [架构设计文档](../../docs/ARCHITECTURE.md)
- [Python API 文档](../../docs/PYTHON_API.md)
- [使用示例](../../docs/EXAMPLES.md)

## 🔧 开发指南

### 添加新的 API

1. 在 `lightui.h` 中声明新函数
2. 在 `lightui.cpp` 中实现函数
3. 确保使用 `extern "C"` 包装
4. 添加错误处理
5. 更新文档和示例

### 错误处理规范

- 成功返回 `0`
- 失败返回非零错误码
- 使用 `NULL` 表示无效指针
- 捕获 C++ 异常并转换为错误码

## ⚠️ 注意事项

1. **线程安全**: 当前 API 不是线程安全的，所有调用应在主线程
2. **资源管理**: 调用者负责释放创建的资源
3. **字符串编码**: 所有字符串使用 UTF-8 编码
4. **错误检查**: 始终检查返回值和指针有效性

## 📊 版本历史

- **v0.1.0**: 初始版本，基础窗口和 HTML 加载功能
- **v0.2.0**: 添加 JavaScript 执行支持
- **v0.3.0**: 完善错误处理和资源管理

---

**维护者**: MBink Team  
**最后更新**: 2025-11-12

