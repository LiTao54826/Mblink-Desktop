# Design Document: Code Structure Refactoring

## Overview

本设计文档定义了 LightUI 项目的代码重构方案和后续开发的强制规范。重构分为四个阶段执行，确保渐进式改进而不破坏现有功能。

## Architecture

### 当前架构问题

```
core/
├── dom/           # 80+ 文件，HTML元素混杂
├── event/         # 38 文件，职责混杂（事件循环+编辑+选择）
├── render/        # 60+ 文件，已有部分子目录
├── window/        # 窗口管理，包含平台特定代码
├── layout/        # 布局引擎，结构较好
└── ...
```

### 目标架构

```
core/
├── dom/
│   ├── elements/
│   │   ├── form/          # input, textarea, button, select, option, form
│   │   ├── text/          # span, paragraph, heading, anchor, label
│   │   ├── container/     # div, li, ul, ol, table
│   │   └── media/         # image, canvas, svg
│   ├── node.h/cpp
│   ├── element.h/cpp
│   ├── document.h/cpp
│   └── ...
├── event/
│   ├── loop/              # 事件循环核心
│   │   ├── event_loop.h/cpp
│   │   ├── frame_controller.h/cpp
│   │   └── task_scheduler.h/cpp
│   ├── dispatch/          # 事件分发
│   │   ├── mouse_event_dispatcher.h/cpp
│   │   ├── keyboard_event_dispatcher.h/cpp
│   │   └── event_dispatcher_base.h/cpp
│   ├── input/             # 输入处理
│   │   ├── input_handler.h/cpp
│   │   ├── hit_testing.h/cpp
│   │   └── focus_manager.h/cpp
│   └── types/             # 事件类型定义
│       ├── event.h/cpp
│       ├── mouse_event.h/cpp
│       └── keyboard_event.h/cpp
├── editing/               # 新建：编辑子系统
│   ├── selection_manager.h/cpp
│   ├── clipboard_manager.h/cpp
│   ├── contenteditable_handler.h/cpp
│   ├── contenteditable_controller.h/cpp
│   └── drag_manager.h/cpp
├── render/
│   ├── object/            # 渲染对象核心
│   │   ├── render_object.h/cpp      # 基类，保持精简
│   │   ├── render_object_layout.h/cpp
│   │   ├── render_object_paint.h/cpp
│   │   ├── render_object_scroll.h/cpp
│   │   └── render_object_transform.h/cpp
│   ├── paint/             # 绘制相关
│   │   ├── box_renderer.h/cpp
│   │   ├── text_renderer.h/cpp
│   │   ├── gradient_renderer.h/cpp
│   │   └── shadow_renderer.h/cpp
│   ├── animation/         # 已存在
│   ├── canvas/            # 已存在
│   ├── css/               # 已存在
│   ├── image/             # 已存在
│   └── text/              # 已存在
├── window/
│   ├── window.h/cpp       # 核心窗口类
│   ├── window_renderer.h/cpp
│   ├── window_observer.h/cpp
│   ├── window_manager.h/cpp
│   └── platform/
│       ├── win32_window.h/cpp
│       └── sdl_window_base.h/cpp
└── ...
```

## Components and Interfaces

### 1. Event Dispatch Subsystem

```cpp
// core/event/dispatch/event_dispatcher_base.h
class EventDispatcherBase {
public:
    virtual ~EventDispatcherBase() = default;
    virtual bool HandleEvent(const SDL_Event& event, Window* window) = 0;
    virtual void SetEnabled(bool enabled) = 0;
};

// core/event/dispatch/mouse_event_dispatcher.h
class MouseEventDispatcher : public EventDispatcherBase {
public:
    explicit MouseEventDispatcher(EventLoop* event_loop);
    
    bool HandleEvent(const SDL_Event& event, Window* window) override;
    
    // 从 event_loop.cpp 提取的方法
    void HandleMouseButtonDown(const SDL_Event& event, Window* window);
    void HandleMouseButtonUp(const SDL_Event& event, Window* window);
    void HandleMouseMotion(const SDL_Event& event, Window* window);
    void UpdateHoverChain(Uint32 window_id, float x, float y, const HitTestResult& result);
    void UpdateMouseCursor(const HitTestResult& result, Uint32 window_id);
    
private:
    EventLoop* event_loop_;
    std::weak_ptr<RenderObject> scrollbar_dragging_element_;
    Uint32 scrollbar_dragging_window_id_ = 0;
    // ... 其他从 event_loop 提取的状态
};

// core/event/dispatch/keyboard_event_dispatcher.h
class KeyboardEventDispatcher : public EventDispatcherBase {
public:
    explicit KeyboardEventDispatcher(EventLoop* event_loop);
    
    bool HandleEvent(const SDL_Event& event, Window* window) override;
    
    void HandleKeyDown(const SDL_Event& event, Window* window);
    void HandleKeyUp(const SDL_Event& event, Window* window);
    void HandleTextInput(const SDL_Event& event, Window* window);
    
private:
    EventLoop* event_loop_;
};
```

### 2. Render Object Decomposition

```cpp
// core/render/object/render_object.h - 精简后的基类
class RenderObject : public std::enable_shared_from_this<RenderObject> {
public:
    // 核心属性和树操作
    RenderObjectType GetType() const;
    void AppendChild(std::shared_ptr<RenderObject> child);
    void RemoveChild(std::shared_ptr<RenderObject> child);
    
    // 委托给专门的类
    RenderObjectLayout& GetLayoutHelper();
    RenderObjectPainter& GetPainter();
    RenderObjectScroll& GetScrollHelper();
    
    // 简化的公共接口
    void Layout(float parent_width, float parent_height);
    void Paint(SkCanvas* canvas);
    
protected:
    std::unique_ptr<RenderObjectLayout> layout_helper_;
    std::unique_ptr<RenderObjectPainter> painter_;
    std::unique_ptr<RenderObjectScroll> scroll_helper_;
};

// core/render/object/render_object_layout.h
class RenderObjectLayout {
public:
    explicit RenderObjectLayout(RenderObject* owner);
    
    void UpdateLayoutStyle();
    void PerformLayout(float parent_width, float parent_height);
    void MarkNeedsLayout(bool propagate = false);
    
    const LayoutInfo& GetLayoutInfo() const;
    
private:
    RenderObject* owner_;
    LayoutInfo layout_info_;
    Style layout_style_;
    bool needs_layout_ = true;
};

// core/render/object/render_object_paint.h
class RenderObjectPainter {
public:
    explicit RenderObjectPainter(RenderObject* owner);
    
    void Paint(SkCanvas* canvas);
    void PaintBackground(SkCanvas* canvas);
    void PaintBorder(SkCanvas* canvas);
    void PaintOutline(SkCanvas* canvas);
    void UpdatePaintCache();
    void InvalidatePaintCache();
    
private:
    RenderObject* owner_;
    PaintCache paint_cache_;
    bool needs_paint_ = true;
};

// core/render/object/render_object_scroll.h
class RenderObjectScroll {
public:
    explicit RenderObjectScroll(RenderObject* owner);
    
    void ScrollTo(float x, float y);
    void ScrollBy(float dx, float dy);
    float GetScrollX() const;
    float GetScrollY() const;
    float GetMaxScrollX() const;
    float GetMaxScrollY() const;
    bool IsScrollable() const;
    
    // 滚动条相关
    ScrollbarHitArea HitTestScrollbar(float x, float y) const;
    void PaintScrollbars(SkCanvas* canvas);
    
private:
    RenderObject* owner_;
    float scroll_x_ = 0;
    float scroll_y_ = 0;
    float content_width_ = 0;
    float content_height_ = 0;
};
```

### 3. Window Decomposition

```cpp
// core/window/window_observer.h
class WindowDOMObserver : public DOMObserver {
public:
    explicit WindowDOMObserver(Window* window);
    
    void OnNodeAdded(Node* node, Node* parent) override;
    void OnNodeRemoved(Node* node, Node* parent) override;
    void OnAttributeChanged(Element* element, const std::string& name,
                           const std::string& old_value, const std::string& new_value) override;
    void OnStyleChanged(Element* element, const std::string& property,
                       const std::string& old_value, const std::string& new_value) override;
    void OnTextChanged(Node* node, const std::string& old_text, const std::string& new_text) override;
    void OnSubtreeModified(Node* root) override;
    void OnPseudoClassChanged(std::shared_ptr<Element> element,
                             const std::string& pseudo_class, bool activate) override;
    
private:
    bool IsInBatch(Node* node) const;
    Window* window_;
};

// core/window/window_renderer.h
class WindowRenderer {
public:
    explicit WindowRenderer(Window* window);
    
    void Render();
    void SetNeedsRepaint();
    bool NeedsRepaint() const;
    void AddDirtyRect(const SkRect& rect);
    
    RenderPipeline* GetRenderPipeline();
    
private:
    Window* window_;
    std::unique_ptr<RenderPipeline> render_pipeline_;
    bool needs_repaint_ = true;
    std::vector<SkRect> dirty_rects_;
};

// core/window/platform/win32_window.h
#ifdef _WIN32
class Win32WindowHelper {
public:
    static void SubclassWindow(HWND hwnd, Window* window);
    static void UnsubclassWindow(HWND hwnd);
    static LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
private:
    static std::unordered_map<HWND, WNDPROC> original_wndprocs_;
    static std::unordered_map<HWND, Window*> hwnd_to_window_;
};
#endif
```

### 4. Editing Subsystem

```cpp
// core/editing/editing_controller.h
class EditingController {
public:
    EditingController();
    
    SelectionManager* GetSelectionManager();
    ClipboardManager* GetClipboardManager();
    ContentEditableHandler* GetContentEditableHandler();
    ContentEditableController* GetContentEditableController();
    DragManager* GetDragManager();
    
    // 统一的编辑操作接口
    void HandleKeyboardInput(const SDL_Event& event, std::shared_ptr<Element> target);
    void HandleMouseSelection(const SDL_Event& event, std::shared_ptr<Element> target);
    
private:
    std::unique_ptr<SelectionManager> selection_manager_;
    std::unique_ptr<ClipboardManager> clipboard_manager_;
    std::unique_ptr<ContentEditableHandler> contenteditable_handler_;
    std::unique_ptr<ContentEditableController> contenteditable_controller_;
    std::unique_ptr<DragManager> drag_manager_;
};
```

## Data Models

### 目录结构元数据

每个子系统目录应包含 README.md：

```markdown
# core/event/dispatch/README.md

## Event Dispatch Subsystem

负责将 SDL 事件分发到 DOM 元素。

### 模块列表
- `event_dispatcher_base.h/cpp` - 分发器基类
- `mouse_event_dispatcher.h/cpp` - 鼠标事件分发
- `keyboard_event_dispatcher.h/cpp` - 键盘事件分发

### 依赖关系
- 依赖: core/dom, core/render, core/window
- 被依赖: core/event/loop

### 使用示例
```cpp
auto dispatcher = std::make_unique<MouseEventDispatcher>(event_loop);
dispatcher->HandleEvent(sdl_event, window);
```
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: 大文件需要文档说明
*For any* source file exceeding 2000 lines, there should exist a comment block at the file header or a corresponding entry in the directory README explaining why the file remains unified.
**Validates: Requirements 1.5**

### Property 2: 长函数检测
*For any* function in the codebase, if it exceeds 150 lines, it should be flagged for review (can be verified by static analysis).
**Validates: Requirements 1.4**

### Property 3: 目录文件数量限制
*For any* directory containing source files, if it contains more than 15 .cpp/.h files, it should have subdirectories for logical grouping.
**Validates: Requirements 2.1**

### Property 4: 子系统 CMakeLists 存在性
*For any* subdirectory under core/ that contains source files, there should exist a CMakeLists.txt file.
**Validates: Requirements 2.2**

### Property 5: 测试文件命名规范
*For any* test file, it should either be in a tests/ subdirectory or named with _test.cpp suffix adjacent to the source file.
**Validates: Requirements 2.4**

### Property 6: HTML 元素分类正确性
*For any* HTML element class file (html_*_element.cpp), it should be located in the appropriate category subdirectory (form/, text/, container/, media/).
**Validates: Requirements 9.1-9.5**

### Property 7: 头文件大小限制
*For any* header file, if it exceeds 300 lines, it should be reviewed for potential splitting (implementation should be in .cpp).
**Validates: Requirements 5.2**

### Property 8: 模块依赖数量限制
*For any* source file, the number of project-internal #include directives should not exceed 15.
**Validates: Requirements 3.3, 12.5**

### Property 9: 循环依赖检测
*For any* two modules A and B in the dependency graph, if A depends on B, then B should not depend on A (directly or transitively).
**Validates: Requirements 12.2**

### Property 10: Include 顺序规范
*For any* source file, includes should be ordered as: own header, project headers (alphabetically), third-party headers, system headers.
**Validates: Requirements 5.4**

### Property 11: 编辑模块位置正确性
*For any* editing-related file (selection_manager, clipboard_manager, contenteditable_*), it should be located in core/editing/ directory.
**Validates: Requirements 10.1-10.5**

### Property 12: 目录 README 存在性
*For any* subdirectory under core/ that contains more than 3 source files, there should exist a README.md file describing the subsystem.
**Validates: Requirements 11.4**

### Property 13: 公共类文档存在性
*For any* public class definition in a header file, there should exist a Doxygen-style comment block (/** ... */) preceding the class declaration.
**Validates: Requirements 11.1**

### Property 14: 重构后测试通过
*For any* refactored module, all existing tests that previously passed should continue to pass after refactoring.
**Validates: Requirements 6-10**

## Error Handling

### 重构过程中的错误处理

1. **编译错误**: 每次重构后立即编译验证，确保无语法错误
2. **链接错误**: 更新 CMakeLists.txt 确保所有新文件被正确包含
3. **运行时错误**: 运行现有测试套件验证功能正确性
4. **回归问题**: 使用 git 分支管理，便于回滚

### 代码审查检查清单

- [ ] 文件大小是否合理？
- [ ] 是否遵循命名规范？
- [ ] 是否有适当的文档注释？
- [ ] 是否引入了循环依赖？
- [ ] 是否更新了相关的 CMakeLists.txt？
- [ ] 是否更新了目录 README.md？

## Testing Strategy

### 单元测试

- 每个新提取的类应有对应的单元测试
- 测试文件命名: `{class_name}_test.cpp`
- 测试应覆盖公共接口的所有方法

### 集成测试

- 重构后运行完整的集成测试套件
- 验证事件分发、渲染、编辑等核心流程

### 回归测试

- 使用现有的 examples/ 目录中的示例验证功能
- 特别关注 rich_text_editor 示例的编辑功能

### 属性测试

- 使用静态分析工具检测循环依赖
- 使用脚本检测文件大小和目录结构合规性
