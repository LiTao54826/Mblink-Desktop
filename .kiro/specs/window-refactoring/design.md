# window.cpp 拆分重构设计

## Overview

本设计文档描述了将 `core/window/window.cpp` (3368行) 拆分为多个独立文件的方案。目标是将文件大小控制在 1500 行以内，同时保持代码的功能完整性和可维护性。

## Architecture

### 当前结构分析

`window.cpp` 当前包含以下主要组件：

1. **WindowDOMObserver** (约 450 行) - DOM 观察者类，监听 DOM 变化
2. **Windows 平台代码** (约 200 行) - Windows 子类化窗口过程
3. **Window 构造/析构** (约 100 行) - 窗口生命周期管理
4. **SDL/OpenGL/Skia 初始化** (约 400 行) - 渲染后端初始化
5. **窗口属性方法** (约 150 行) - Show/Hide/SetSize 等
6. **渲染方法** (约 1000 行) - Render/RenderDevTools/UpdateAnimations
7. **辅助方法** (约 500 行) - 脏区域收集、滚动位置保存等
8. **事件处理** (约 100 行) - 事件分发

### 拆分方案

```
core/window/
├── window.cpp              (~1400 行) - 核心窗口管理
├── window.h                (保持不变)
├── window_dom_observer.cpp (~450 行) - DOM 观察者 [新建]
├── window_dom_observer.h   (~50 行) - DOM 观察者头文件 [新建]
├── window_renderer.cpp     (~800 行) - 渲染逻辑 [扩展]
├── window_renderer.h       (保持不变)
├── window_win32.cpp        (~200 行) - Windows 平台代码 [新建]
├── display_backend.cpp     (保持不变)
└── ...
```

## Components and Interfaces

### WindowDOMObserver

从 window.cpp 提取的 DOM 观察者类。

```cpp
// window_dom_observer.h
class WindowDOMObserver : public DOMObserver {
public:
    explicit WindowDOMObserver(Window* window);
    
    void OnNodeAdded(Node* node, Node* parent) override;
    void OnNodeRemoved(Node* node, Node* parent) override;
    void OnAttributeChanged(Element* element, ...) override;
    void OnStyleChanged(Element* element, ...) override;
    void OnTextChanged(Node* node, ...) override;
    void OnSubtreeModified(Node* root) override;
    void OnPseudoClassChanged(std::shared_ptr<Element> element, ...) override;
    
private:
    bool IsInBatch(Node* node) const;
    Window* window_;
};
```

### WindowRenderer (扩展)

扩展现有的 WindowRenderer 类，迁移更多渲染逻辑。

```cpp
// 新增方法
class WindowRenderer {
public:
    // 主渲染方法（从 Window 迁移）
    void RenderFrame(SkCanvas* canvas, float width, float height);
    void RenderDevToolsOverlay(SkCanvas* canvas, float width, float height);
    
    // 动画相关
    void UpdateAnimations(double current_time);
    void ApplyAnimationsToRenderTree(RenderObject* root);
    bool HasPendingAnimations(RenderObject* root) const;
    
    // 已有方法保持不变
    // ...
};
```

### Windows 平台代码

提取 Windows 特定的窗口子类化代码。

```cpp
// window_win32.cpp
#ifdef _WIN32
namespace lightui {
namespace win32 {

void SubclassWindow(HWND hwnd, Window* window);
void UnsubclassWindow(HWND hwnd);
LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

} // namespace win32
} // namespace lightui
#endif
```

## Data Models

无新增数据模型，保持现有结构。

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: DOM 观察功能保持

*For any* DOM 变化操作（添加节点、删除节点、修改属性、修改样式、修改文本），WindowDOMObserver 应该正确触发窗口重绘标记。

**Validates: Requirements 1.2, 6.3**

### Property 2: 渲染功能保持

*For any* 渲染调用，WindowRenderer 应该产生与原 Window::Render() 相同的渲染结果。

**Validates: Requirements 2.2, 6.2**

### Property 3: 初始化功能保持

*For any* Window 创建操作，所有子系统（SDL、OpenGL、Skia）应该正确初始化。

**Validates: Requirements 4.2, 4.3**

## Error Handling

- 文件提取过程中保持原有的错误处理逻辑
- 编译错误通过增量验证及时发现和修复
- 运行时错误保持原有的异常处理机制

## Testing Strategy

### 编译验证

每个阶段完成后进行编译验证，确保无编译错误。

### 功能验证

- 运行示例程序验证窗口创建和渲染
- 验证 DOM 变化能正确触发重绘
- 验证动画功能正常工作

### 文件大小验证

使用 `find /c /v ""` 命令验证各文件行数符合目标。
