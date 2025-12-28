# 大文件拆分设计文档

## 概述

本文档描述了对大型源文件进行拆分的详细设计方案。

## 1. event_loop.cpp 拆分设计 (3920 行)

### 1.1 当前结构分析

```
event_loop.cpp 主要包含:
├── EventLoop 类实现
│   ├── 构造/析构 (~100 行)
│   ├── Run/Stop/RunOnce (~200 行)
│   ├── ProcessEvents (~50 行)
│   ├── HandleMouseEventForDOM (~1800 行) ← 主要拆分目标
│   │   ├── DevTools 鼠标事件处理 (~200 行)
│   │   ├── 滚动条拖动处理 (~100 行)
│   │   ├── Select 下拉菜单处理 (~50 行)
│   │   ├── Hit Testing (~50 行)
│   │   ├── Hover 链更新 (~150 行)
│   │   ├── 表单元素交互 (~800 行)
│   │   └── ContentEditable 拖动选择 (~450 行)
│   ├── HandleKeyboardEventForDOM (~300 行) ← 拆分目标
│   ├── HandleMouseWheelEventForDOM (~400 行) ← 拆分目标
│   ├── UpdateHoverChain/SendEvents (~200 行)
│   ├── HandleInputMouseInteraction (~300 行)
│   ├── HandleTextAreaMouseInteraction (~400 行)
│   └── 辅助方法 (~200 行)
```

### 1.2 拆分方案

#### 1.2.1 MouseEventDispatcher (已创建框架)

**职责:** 处理所有鼠标事件
**目标行数:** ~800 行

```cpp
class MouseEventDispatcher {
public:
    // 主入口
    bool HandleMouseEvent(const SDL_Event& event, ...);
    
    // Hover 链管理
    void UpdateHoverChain(...);
    bool SendEvents(...);
    void UpdateMouseCursor(...);
    
    // 表单元素交互
    void HandleInputMouseInteraction(...);
    void HandleTextAreaMouseInteraction(...);
    
    // 滚动条处理
    void HandleScrollbarDrag(...);
    
private:
    // 状态
    std::vector<std::weak_ptr<Element>> hover_chain_;
    std::weak_ptr<Element> hover_element_;
    std::weak_ptr<RenderObject> scrollbar_dragging_element_;
};
```

#### 1.2.2 KeyboardEventDispatcher (已创建框架)

**职责:** 处理所有键盘事件
**目标行数:** ~400 行

```cpp
class KeyboardEventDispatcher {
public:
    bool HandleKeyboardEvent(const SDL_Event& event, ...);
    bool HandleTextInputEvent(const SDL_Event& event, ...);
    bool HandleShortcut(...);
    
    static std::string SDLKeycodeToDOMKey(SDL_Keycode);
    static std::string SDLScancodeToDOMCode(SDL_Scancode);
};
```

#### 1.2.3 WheelEventDispatcher (新建)

**职责:** 处理滚轮事件
**目标行数:** ~300 行

```cpp
class WheelEventDispatcher {
public:
    bool HandleWheelEvent(const SDL_Event& event, ...);
    
private:
    void HandleTextAreaScroll(...);
    void HandleElementScroll(...);
};
```

### 1.3 迁移步骤

1. 将 `UpdateHoverChain`, `SendEvents`, `UpdateMouseCursor` 迁移到 MouseEventDispatcher
2. 将 `HandleInputMouseInteraction`, `HandleTextAreaMouseInteraction` 迁移到 MouseEventDispatcher
3. 将 `HandleMouseEventForDOM` 的主逻辑迁移到 MouseEventDispatcher
4. 将 `HandleKeyboardEventForDOM` 迁移到 KeyboardEventDispatcher
5. 创建 WheelEventDispatcher 并迁移 `HandleMouseWheelEventForDOM`
6. EventLoop 调用各 Dispatcher 的方法

---

## 2. render_object.cpp 拆分设计 (5360 行)

### 2.1 当前结构分析

```
render_object.cpp 主要包含:
├── RenderObject 类实现
│   ├── 构造/析构/基础方法 (~200 行)
│   ├── 布局相关 (~500 行)
│   ├── Paint 方法 (~2000 行) ← 主要拆分目标
│   │   ├── 背景绘制 (~400 行)
│   │   ├── 边框绘制 (~300 行)
│   │   ├── 滚动条绘制 (~400 行)
│   │   ├── 表单元素绘制 (~600 行)
│   │   └── 其他绘制 (~300 行)
│   ├── 滚动条逻辑 (~800 行) ← 拆分目标
│   ├── Hit Testing (~300 行)
│   └── 辅助方法 (~500 行)
```

### 2.2 拆分方案

#### 2.2.1 BackgroundPainter

**职责:** 绘制背景（颜色、渐变、图片）
**目标行数:** ~400 行

```cpp
class BackgroundPainter {
public:
    void Paint(SkCanvas* canvas, const RenderObject& obj);
    
private:
    void PaintBackgroundColor(...);
    void PaintBackgroundImage(...);
    void PaintBackgroundGradient(...);
};
```

#### 2.2.2 BorderPainter

**职责:** 绘制边框
**目标行数:** ~300 行

```cpp
class BorderPainter {
public:
    void Paint(SkCanvas* canvas, const RenderObject& obj);
    
private:
    void PaintSolidBorder(...);
    void PaintRoundedBorder(...);
};
```

#### 2.2.3 ScrollbarPainter

**职责:** 绘制滚动条
**目标行数:** ~400 行

```cpp
class ScrollbarPainter {
public:
    void Paint(SkCanvas* canvas, const RenderObject& obj);
    
private:
    void PaintVerticalScrollbar(...);
    void PaintHorizontalScrollbar(...);
    void PaintScrollbarThumb(...);
};
```

#### 2.2.4 FormElementPainter

**职责:** 绘制表单元素（input, textarea, select, checkbox, radio）
**目标行数:** ~600 行

```cpp
class FormElementPainter {
public:
    void Paint(SkCanvas* canvas, const RenderObject& obj);
    
private:
    void PaintInputElement(...);
    void PaintTextAreaElement(...);
    void PaintSelectElement(...);
    void PaintCheckbox(...);
    void PaintRadio(...);
};
```

#### 2.2.5 ScrollbarController

**职责:** 滚动条逻辑（非绘制）
**目标行数:** ~500 行

```cpp
class ScrollbarController {
public:
    ScrollbarHitArea HitTestScrollbar(float x, float y);
    void StartScrollbarDrag(...);
    void UpdateScrollbarDrag(...);
    void EndScrollbarDrag();
    
private:
    // 滚动条状态
};
```

---

## 3. window.cpp 拆分设计 (3368 行)

### 3.1 当前结构分析

```
window.cpp 主要包含:
├── Window 类实现
│   ├── 构造/析构 (~200 行)
│   ├── 渲染相关 (~1500 行) ← 主要拆分目标
│   │   ├── Render 方法 (~800 行)
│   │   ├── 渲染树构建 (~400 行)
│   │   └── 增量渲染 (~300 行)
│   ├── 事件处理 (~500 行)
│   ├── 窗口管理 (~400 行)
│   └── 辅助方法 (~300 行)
```

### 3.2 拆分方案

#### 3.2.1 WindowRenderer

**职责:** 窗口渲染逻辑
**目标行数:** ~800 行

```cpp
class WindowRenderer {
public:
    void Render(Window& window);
    void RenderIncrementally(Window& window);
    
private:
    void BuildRenderTree(...);
    void PaintRenderTree(...);
};
```

---

## 正确性属性

*属性是系统在所有有效执行中应保持为真的特征或行为——本质上是关于系统应该做什么的正式声明。属性作为人类可读规范和机器可验证正确性保证之间的桥梁。*

### Property 1: 功能等价性
*对于任何* 输入事件序列，拆分后的代码应产生与拆分前完全相同的输出和副作用
**验证: 需求 1.5, 2.6, 3.4**

### Property 2: 编译通过
*对于任何* 拆分操作，编译必须通过且无新增错误
**验证: 需求 4.4**

### Property 3: 文件大小约束
*对于任何* 新创建的文件，行数不应超过 1500 行
**验证: 需求 4.2**

---

## 错误处理

- 如果迁移过程中发现循环依赖，需要重新设计接口
- 如果编译失败，需要检查 include 路径和前向声明
- 如果功能异常，需要回滚并分析原因

---

## 测试策略

### 单元测试
- 为每个新的 Painter/Dispatcher 类编写单元测试
- 测试边界条件和错误处理

### 集成测试
- 运行现有的示例程序验证功能
- 对比拆分前后的渲染结果

### 回归测试
- 确保所有现有功能正常工作
- 检查性能是否有明显下降
